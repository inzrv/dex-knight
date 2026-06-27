from __future__ import annotations

from fastapi import WebSocket


class PendingTransactionBroadcaster:
    def __init__(self) -> None:
        self._subscribers: set[WebSocket] = set()

    async def connect(self, websocket: WebSocket) -> None:
        await websocket.accept()
        self._subscribers.add(websocket)

    def disconnect(self, websocket: WebSocket) -> None:
        self._subscribers.discard(websocket)

    async def broadcast(self, message: dict) -> None:
        disconnected: list[WebSocket] = []

        for websocket in list(self._subscribers):
            try:
                await websocket.send_json(message)
            except Exception:
                disconnected.append(websocket)

        for websocket in disconnected:
            self.disconnect(websocket)
