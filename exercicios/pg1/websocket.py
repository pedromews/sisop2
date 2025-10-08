import asyncio
from http import HTTPStatus

from websockets.server import serve
from websockets.http import Headers

async def http_handler(path, request_headers):
    """Serve os HTMLs estáticos via HTTP no mesmo socket."""
    if path == "/ui-chat":
        with open("chat.html", "rb") as f:
            body = f.read()
        headers = Headers(**{"Content-Type": "text/html; charset=utf-8"})
        return HTTPStatus.OK, headers, body

    if path == "/ui-echo":
        with open("echo.html", "rb") as f:
            body = f.read()
        headers = Headers(**{"Content-Type": "text/html; charset=utf-8"})
        return HTTPStatus.OK, headers, body

    return None  # deixa o upgrade WS ocorrer 

async def chat(websocket, path, sessions={}):
    remote = websocket.remote_address
    sessions[remote] = websocket
    print(f"[+] Cliente conectado: {remote}")

    try:
        async for message in websocket:
            print(f"[MSG] {remote}: {message}")
            for socket in sessions.values():
                if socket.open:
                    await socket.send(f"{remote}: {message}")
    finally:
        print(f"[-] Cliente desconectado: {remote}")
        del sessions[remote]

async def echo(websocket):
    """Echo WebSocket handler"""
    async for message in websocket:
        await websocket.send(message)

async def web_socket_router(websocket):
    """Roteia usando websocket.path (websockets >= 11)."""
    path = getattr(websocket, "path", "/")
    if path == "/":
        await websocket.close(reason="needs a path")
    elif path == "/echo":
        await echo(websocket, path)
    elif path == "/chat":
        await chat(websocket, path)
    else:
        await websocket.close(reason=f"path not found: {path}")

async def main():
    async with serve(web_socket_router, "localhost", 8080, process_request=http_handler):
        await asyncio.Future()  # roda para sempre

if __name__ == "__main__":
    asyncio.run(main())
