import uvicorn


def main() -> None:
    uvicorn.run(
        "block_builder.app:create_app",
        factory=True,
        host="127.0.0.1",
        port=9001,
        reload=False,
    )


if __name__ == "__main__":
    main()
