# Mempool Sequence Scenario

This scenario checks that the local block builder assigns monotonic sequence
numbers to public mempool transactions and exposes the same sequence boundary in
`GET /public/pending`.
It restarts only the local block builder to reset the in-memory mempool and does
not require contract deployment.

## Steps

- restarts the local block builder to get an empty in-memory mempool,
- verifies the initial public mempool snapshot has `snapshotSeq = 0`,
- submits one public transaction and expects `seqNum = 1`,
- checks the snapshot has `snapshotSeq = 1`,
- submits a second public transaction and expects `seqNum = 2`,
- checks the snapshot has `snapshotSeq = 2`.

## Run

```shell
scenarios/repeatable/mempool-sequence/run.zsh
```
