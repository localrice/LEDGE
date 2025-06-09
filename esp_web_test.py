from fastapi import FastAPI
from fastapi.staticfiles import StaticFiles
import asyncio
import websockets

import threading

# === FastAPI Web Server (serving /data/index.html on :8000) ===
app = FastAPI()

# Serve static files from the "data" directory
app.mount("/", StaticFiles(directory="data", html=True), name="static")

# === WebSocket Server on port 81 ===
async def ws_handler(websocket):
    print(f"[WS] Client connected: {websocket.remote_address}")
    try:
        async for message in websocket:
            print(f"[WS] Received: {message}")
            await websocket.send(f"ACK: {message}")
    except websockets.exceptions.ConnectionClosed:
        print("[WS] Client disconnected")

async def start_websocket_server():
    async with websockets.serve(ws_handler, "0.0.0.0", 8080):
        print("WebSocket server running at ws://localhost:81")
        await asyncio.Future()  # Run forever

# === Start everything ===
def run_uvicorn():
    import uvicorn
    uvicorn.run(app, host="0.0.0.0", port=8000)

if __name__ == "__main__":
    # Start FastAPI web server in a thread
    threading.Thread(target=run_uvicorn, daemon=True).start()

    # Run websocket server in main thread
    asyncio.run(start_websocket_server())
