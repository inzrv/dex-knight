from __future__ import annotations

import json
import os
import subprocess
import time
from pathlib import Path
from typing import Any, Optional
from urllib.error import HTTPError, URLError
from urllib.request import Request, urlopen


SCRIPT_DIR = Path(__file__).resolve().parent
REPO_ROOT = SCRIPT_DIR.parent
SERVICE_DIR = REPO_ROOT / "services" / "block-builder"
BLOCKCHAIN_DIR = REPO_ROOT / "blockchain"
DEPLOYMENT_FILE = BLOCKCHAIN_DIR / "deployments" / "local.json"
ENV_FILE = BLOCKCHAIN_DIR / "config" / "local.anvil.env"

BUILDER_URL = os.environ.get("BLOCK_BUILDER_URL", "http://127.0.0.1:9001")
DEFAULT_TOKEN_DECIMALS = 18
TOKEN_DECIMALS = 10**DEFAULT_TOKEN_DECIMALS


# Ensures a local blockchain deployment exists and returns its metadata.
def ensure_deployment() -> dict[str, Any]:
    if DEPLOYMENT_FILE.exists():
        deployment = read_json(DEPLOYMENT_FILE)
        if rpc_is_ready(deployment["rpcUrl"]):
            print(f"Using deployment: {DEPLOYMENT_FILE}")
            return deployment

        print("Saved deployment exists, but the local chain is not running.")

    run([str(BLOCKCHAIN_DIR / "bin" / "deploy-local.zsh")], cwd=REPO_ROOT)
    deployment = read_json(DEPLOYMENT_FILE)
    print(f"Using deployment: {DEPLOYMENT_FILE}")
    return deployment


# Reads a named local role from deployment metadata or the legacy env file.
def deployment_role(deployment: dict[str, Any], name: str) -> dict[str, str]:
    roles = deployment.get("roles")
    if isinstance(roles, dict):
        role = roles.get(name)
        if isinstance(role, dict):
            return {
                key: value
                for key, value in role.items()
                if isinstance(key, str) and isinstance(value, str)
            }

    env = read_env_file(ENV_FILE)
    prefix = name.upper()
    role = {
        "address": env.get(f"{prefix}_ADDRESS", ""),
        "privateKey": env.get(f"{prefix}_PRIVATE_KEY", ""),
    }
    if role["address"] != "" or role["privateKey"] != "":
        return role

    raise ScenarioError(f"deployment role '{name}' not found")


# Starts the local block builder and waits until its health endpoint responds.
def start_block_builder() -> None:
    run([str(SERVICE_DIR / "bin" / "start-local.zsh")], cwd=REPO_ROOT)
    wait_for_builder()


# Stops the local block builder managed by the local script.
def cleanup_block_builder() -> None:
    run([str(SERVICE_DIR / "bin" / "cleanup-local.zsh")], cwd=REPO_ROOT)


# Restarts the local block builder with a fresh in-memory state.
def restart_block_builder() -> None:
    cleanup_block_builder()
    start_block_builder()


# Waits for the block builder health endpoint to become available.
def wait_for_builder() -> None:
    for _ in range(30):
        try:
            builder_request("GET", "/health")
            return
        except ScenarioError:
            time.sleep(0.2)

    raise ScenarioError(f"block builder did not become healthy at {BUILDER_URL}/health")


# Reads the current chain head through the block builder.
def chain_head() -> dict[str, Any]:
    return builder_request("GET", "/chain/head")


# Parses a block number from a chain head response.
def block_number(head: dict[str, Any]) -> int:
    block_number_hex = head.get("blockNumber")
    if not isinstance(block_number_hex, str):
        raise ScenarioError("chain head response does not contain blockNumber")

    return int(block_number_hex, 16)


# Reads a block by number from the local chain.
def block_by_number(rpc_url: str, number: int, include_transactions: bool) -> dict[str, Any]:
    block = rpc(rpc_url, "eth_getBlockByNumber", [hex(number), include_transactions])
    if not isinstance(block, dict):
        raise ScenarioError(f"block {number} not found")
    return block


# Formats a chain head response for scenario output.
def chain_head_label(head: dict[str, Any]) -> str:
    block_hash = head.get("blockHash")
    if not isinstance(block_hash, str):
        block_hash = "<missing>"

    return f"{block_number(head)} ({block_hash})"


# Requires an already deployed and running local chain without starting services.
def require_running_deployment() -> dict[str, Any]:
    if not DEPLOYMENT_FILE.exists():
        raise ScenarioError(
            f"deployment file does not exist: {DEPLOYMENT_FILE}; run scenarios/ready-env/bin/start-clean.zsh first"
        )

    deployment = read_json(DEPLOYMENT_FILE)
    rpc_url = deployment.get("rpcUrl")
    if not isinstance(rpc_url, str) or rpc_url == "":
        raise ScenarioError("deployment does not contain rpcUrl")

    if not rpc_is_ready(rpc_url):
        raise ScenarioError(f"local chain RPC is not reachable at {rpc_url}")

    return deployment


# Requires Anvil automine to be disabled so the builder controls block production.
def require_builder_controlled_mining(rpc_url: str) -> None:
    automine = rpc(rpc_url, "anvil_getAutomine", [])
    if automine is not False:
        raise ScenarioError(
            "local chain automine must be disabled so the block builder controls mining"
        )


# Extracts pending transaction records from a public mempool snapshot.
def pending_records(snapshot: dict[str, Any]) -> list[dict[str, Any]]:
    transactions = snapshot.get("transactions")
    if not isinstance(transactions, list):
        raise ScenarioError("pending snapshot does not contain transactions list")

    records: list[dict[str, Any]] = []
    for transaction in transactions:
        if not isinstance(transaction, dict):
            raise ScenarioError("pending snapshot contains a non-object transaction record")
        records.append(transaction)

    return records


# Reads an ERC-20 token balance from the local chain.
def token_balance(rpc_url: str, token: str, account: str) -> int:
    return cast_int(
        [
            "cast",
            "call",
            token,
            "balanceOf(address)(uint256)",
            account,
            "--rpc-url",
            rpc_url,
        ],
        cwd=BLOCKCHAIN_DIR,
    )


# Formats an integer token amount using the default sandbox decimals.
def format_token_amount(amount: int, decimals: int = DEFAULT_TOKEN_DECIMALS) -> str:
    scale = 10**decimals
    whole = amount // scale
    fraction = amount % scale
    if fraction == 0:
        return str(whole)

    fraction_text = str(fraction).rjust(decimals, "0").rstrip("0")
    return f"{whole}.{fraction_text}"


# Reads Pool reserveA/reserveB values from the local chain.
def pool_reserves(rpc_url: str, pool: str) -> tuple[int, int]:
    reserve_a = cast_int(
        ["cast", "call", pool, "reserveA()(uint256)", "--rpc-url", rpc_url],
        cwd=BLOCKCHAIN_DIR,
    )
    reserve_b = cast_int(
        ["cast", "call", pool, "reserveB()(uint256)", "--rpc-url", rpc_url],
        cwd=BLOCKCHAIN_DIR,
    )
    return reserve_a, reserve_b


# Reads the current transaction nonce for an account.
def account_nonce(rpc_url: str, account: str) -> int:
    return cast_int(["cast", "nonce", account, "--rpc-url", rpc_url], cwd=BLOCKCHAIN_DIR)


# Encodes contract call calldata with cast calldata.
def contract_calldata(signature: str, *args: str) -> str:
    return run(
        [
            "cast",
            "calldata",
            signature,
            *args,
        ],
        cwd=BLOCKCHAIN_DIR,
    ).stdout.strip()


# Quotes Pool1-style TokenA to TokenB output for an exact input amount.
def quote_amount_out_a_for_b(rpc_url: str, pool: str, amount_in: int) -> int:
    return cast_int(
        [
            "cast",
            "call",
            pool,
            "getAmountOutAForB(uint256)(uint256)",
            str(amount_in),
            "--rpc-url",
            rpc_url,
        ],
        cwd=BLOCKCHAIN_DIR,
    )


# Quotes Pool1-style TokenB to TokenA output for an exact input amount.
def quote_amount_out_b_for_a(rpc_url: str, pool: str, amount_in: int) -> int:
    return cast_int(
        [
            "cast",
            "call",
            pool,
            "getAmountOutBForA(uint256)(uint256)",
            str(amount_in),
            "--rpc-url",
            rpc_url,
        ],
        cwd=BLOCKCHAIN_DIR,
    )


# Computes SandboxDex exact-input output with the local 0.3% fee.
def sandbox_amount_out(amount_in: int, reserve_in: int, reserve_out: int) -> int:
    if amount_in <= 0:
        raise ScenarioError("amount_in must be positive")
    if reserve_in <= 0 or reserve_out <= 0:
        raise ScenarioError("reserves must be positive")

    amount_in_with_fee = amount_in * 997
    return (amount_in_with_fee * reserve_out) // ((reserve_in * 1000) + amount_in_with_fee)


# Seeds a pool with equal TokenA/TokenB liquidity if it has no reserves.
def ensure_pool_liquidity(
    rpc_url: str,
    deployer_key: str,
    deployer: str,
    token_a: str,
    token_b: str,
    pool: str,
    amount: int,
) -> None:
    reserve_a, reserve_b = pool_reserves(rpc_url, pool)
    if reserve_a > 0 and reserve_b > 0:
        print(f"Pool already seeded: reserveA={reserve_a}, reserveB={reserve_b}")
        return

    print_step("Seeding pool liquidity")
    add_pool_liquidity(
        rpc_url,
        deployer_key,
        deployer,
        token_a,
        token_b,
        pool,
        amount,
    )


# Adds equal TokenA/TokenB liquidity to a pool.
def add_pool_liquidity(
    rpc_url: str,
    deployer_key: str,
    deployer: str,
    token_a: str,
    token_b: str,
    pool: str,
    amount: int,
    manage_automine: bool = True,
) -> None:
    if manage_automine:
        rpc(rpc_url, "evm_setAutomine", [True])
    try:
        mint_and_approve(
            rpc_url,
            deployer_key,
            deployer_key,
            deployer,
            token_a,
            pool,
            amount,
            manage_automine=False,
        )
        mint_and_approve(
            rpc_url,
            deployer_key,
            deployer_key,
            deployer,
            token_b,
            pool,
            amount,
            manage_automine=False,
        )
        send_contract_transaction(
            rpc_url,
            deployer_key,
            pool,
            "seedLiquidity(uint256,uint256)",
            str(amount),
            str(amount),
        )
    finally:
        if manage_automine:
            rpc(rpc_url, "evm_setAutomine", [False])


# Ensures a pool has exactly the expected equal TokenA/TokenB liquidity.
def ensure_exact_pool_liquidity(
    rpc_url: str,
    deployer_key: str,
    deployer: str,
    token_a: str,
    token_b: str,
    pool: str,
    amount: int,
    label: str,
) -> tuple[int, int]:
    reserves = pool_reserves(rpc_url, pool)
    if reserves == (0, 0):
        ensure_pool_liquidity(
            rpc_url,
            deployer_key,
            deployer,
            token_a,
            token_b,
            pool,
            amount,
        )
        reserves = pool_reserves(rpc_url, pool)

    if reserves != (amount, amount):
        raise ScenarioError(
            f"{label} reserves must be exactly {amount}/{amount}; "
            "clean and redeploy the local chain before rerunning this scenario"
        )

    return reserves


# Mints a token to an account without approving a spender.
def mint_token(
    rpc_url: str,
    minter_key: str,
    token: str,
    recipient: str,
    amount: int,
    manage_automine: bool = True,
) -> None:
    if manage_automine:
        rpc(rpc_url, "evm_setAutomine", [True])
    try:
        send_contract_transaction(
            rpc_url,
            minter_key,
            token,
            "mint(address,uint256)",
            recipient,
            str(amount),
        )
    finally:
        if manage_automine:
            rpc(rpc_url, "evm_setAutomine", [False])


# Mints a token to an owner and approves a spender for that amount.
def mint_and_approve(
    rpc_url: str,
    minter_key: str,
    owner_key: str,
    owner: str,
    token: str,
    spender: str,
    amount: int,
    manage_automine: bool = True,
) -> None:
    if manage_automine:
        rpc(rpc_url, "evm_setAutomine", [True])
    try:
        send_contract_transaction(
            rpc_url,
            minter_key,
            token,
            "mint(address,uint256)",
            owner,
            str(amount),
        )
        send_contract_transaction(
            rpc_url,
            owner_key,
            token,
            "approve(address,uint256)",
            spender,
            str(amount),
        )
    finally:
        if manage_automine:
            rpc(rpc_url, "evm_setAutomine", [False])


# Builds an EIP-1559 transaction payload for the block builder public mempool.
def public_transaction_payload(
    chain_id: int,
    nonce: int,
    sender: str,
    to: str,
    calldata: str,
    gas: int = 300_000,
) -> dict[str, str]:
    return {
        "type": "0x2",
        "chainId": hex(chain_id),
        "nonce": hex(nonce),
        "from": sender,
        "to": to,
        "value": "0x0",
        "gas": hex(gas),
        "maxFeePerGas": hex(2_000_000_000),
        "maxPriorityFeePerGas": hex(1),
        "input": calldata,
    }


# Builds a block-builder transaction payload for a contract call.
def contract_transaction_payload(
    rpc_url: str,
    chain_id: int,
    sender: str,
    to: str,
    signature: str,
    *args: str,
    gas: int = 300_000,
) -> dict[str, str]:
    return public_transaction_payload(
        chain_id=chain_id,
        nonce=account_nonce(rpc_url, sender),
        sender=sender,
        to=to,
        calldata=contract_calldata(signature, *args),
        gas=gas,
    )


# Builds a block-builder transaction payload for an ERC-20 transfer.
def token_transfer_payload(
    rpc_url: str,
    chain_id: int,
    sender: str,
    token: str,
    recipient: str,
    amount: int,
    gas: int = 200_000,
) -> dict[str, str]:
    return contract_transaction_payload(
        rpc_url,
        chain_id,
        sender,
        token,
        "transfer(address,uint256)",
        recipient,
        str(amount),
        gas=gas,
    )


# Checks whether a JSON-RPC endpoint is reachable.
def rpc_is_ready(rpc_url: str) -> bool:
    try:
        rpc(rpc_url, "eth_blockNumber", [])
        return True
    except ScenarioError:
        return False


# Sends a JSON-RPC request and returns the result field.
def rpc(rpc_url: str, method: str, params: list[Any]) -> Any:
    payload = json.dumps(
        {
            "jsonrpc": "2.0",
            "id": 1,
            "method": method,
            "params": params,
        }
    ).encode("utf-8")
    request = Request(
        rpc_url,
        data=payload,
        headers={"Content-Type": "application/json"},
        method="POST",
    )

    try:
        with urlopen(request, timeout=10) as response:
            result = json.loads(response.read().decode("utf-8"))
    except (HTTPError, URLError) as error:
        raise ScenarioError(f"RPC request failed: {error}") from error

    if "error" in result:
        raise ScenarioError(f"RPC error from {method}: {result['error']}")

    return result.get("result")


# Sends an HTTP request to the block builder API and returns the JSON response.
def builder_request(method: str, path: str, body: Optional[dict[str, Any]] = None) -> Any:
    data = None if body is None else json.dumps(body).encode("utf-8")
    request = Request(
        f"{BUILDER_URL}{path}",
        data=data,
        headers={"Content-Type": "application/json"},
        method=method,
    )

    try:
        with urlopen(request, timeout=10) as response:
            return json.loads(response.read().decode("utf-8"))
    except HTTPError as error:
        detail = error.read().decode("utf-8")
        raise ScenarioError(f"builder {method} {path} failed: {detail}") from error
    except URLError as error:
        raise ScenarioError(f"builder {method} {path} failed: {error}") from error


# Adds a transaction to the block builder public mempool.
def submit_public_transaction(tx_payload: dict[str, str]) -> dict[str, Any]:
    return builder_request("POST", "/public/tx", tx_payload)


# Reads one transaction record from the block builder public mempool.
def public_transaction(mempool_tx_id: str) -> dict[str, Any]:
    return builder_request("GET", f"/public/tx/{mempool_tx_id}")


# Reads a public mempool snapshot with the latest builder sequence number.
def pending_public_transactions() -> dict[str, Any]:
    return builder_request("GET", "/public/pending")


# Mines a private bundle through the block builder.
def mine_bundle(transactions: list[dict[str, Any]]) -> dict[str, Any]:
    return builder_request("POST", "/private/bundle", {"transactions": transactions})


# Simulates a private bundle through the block builder.
def simulate_bundle(transactions: list[dict[str, Any]]) -> dict[str, Any]:
    return builder_request("POST", "/private/bundle/simulate", {"transactions": transactions})


# Runs a cast command and parses the first output token as an integer.
def cast_int(command: list[str], cwd: Path) -> int:
    output = run(command, cwd=cwd).stdout.strip()
    value = output.split()[0]
    return int(value, 0)


# Sends a signed transaction to a contract with cast send.
def send_contract_transaction(
    rpc_url: str,
    private_key: str,
    contract: str,
    signature: str,
    *args: str,
) -> None:
    run(
        [
            "cast",
            "send",
            contract,
            signature,
            *args,
            "--private-key",
            private_key,
            "--rpc-url",
            rpc_url,
        ],
        cwd=BLOCKCHAIN_DIR,
    )


# Runs a subprocess and raises ScenarioError with useful command output.
def run(command: list[str], cwd: Path) -> subprocess.CompletedProcess[str]:
    try:
        return subprocess.run(
            command,
            cwd=cwd,
            check=True,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
    except FileNotFoundError as error:
        raise ScenarioError(f"required command not found: {command[0]}") from error
    except subprocess.CalledProcessError as error:
        message = error.stderr.strip() or error.stdout.strip()
        raise ScenarioError(f"command failed: {' '.join(command)}\n{message}") from error


# Reads a JSON file into a dictionary.
def read_json(path: Path) -> dict[str, Any]:
    with path.open(encoding="utf-8") as file:
        return json.load(file)


# Reads a simple KEY=VALUE env file into a dictionary.
def read_env_file(path: Path) -> dict[str, str]:
    values: dict[str, str] = {}
    with path.open(encoding="utf-8") as file:
        for raw_line in file:
            line = raw_line.strip()
            if not line or line.startswith("#") or "=" not in line:
                continue
            key, value = line.split("=", 1)
            values[key] = value.strip().strip('"')
    return values


# Prints a visible step header for scenario output.
def print_step(message: str) -> None:
    print(f"\n==> {message}")


# Represents a scenario setup, execution, or validation failure.
class ScenarioError(RuntimeError):
    pass
