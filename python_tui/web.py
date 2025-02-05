import asyncio
from textual.app import App, ComposeResult
from textual.containers import ScrollableContainer
from textual.widgets import Input, Button, Static
import websockets

# WebSocket server details
WS_URI = "ws://localhost:8080"

class WebSocketClientApp(App):
    """A Textual app with a WebSocket client."""

    CSS = """
    Screen {
        layout: vertical;
    }
    ScrollableContainer {
        height: 80%;
        border: solid green;
    }
    Input {
        width: 100%;
    }
    Button {
        width: 100%;
    }
    """

    def compose(self) -> ComposeResult:
        """Create child widgets for the app."""
        yield ScrollableContainer(Static(id="messages"))
        yield Input(placeholder="Type a message...", id="message_input")
        yield Button("Send", id="send_button")

    async def on_mount(self) -> None:
        """Start the WebSocket connection when the app mounts."""
        self.websocket = await websockets.connect(WS_URI)
        self.set_interval(1, self.receive_messages)  # Poll for incoming messages

    async def receive_messages(self) -> None:
        """Receive messages from the WebSocket server."""
        try:
            message = await self.websocket.recv()
            self.query_one("#messages", Static).update(
                f"{self.query_one('#messages', Static).renderable}\nReceived: {message}"
            )
        except websockets.exceptions.ConnectionClosed:
            self.query_one("#messages", Static).update(
                f"{self.query_one('#messages', Static).renderable}\nConnection closed by server."
            )
            self.set_interval(1, None)  # Stop polling

    async def on_button_pressed(self, event: Button.Pressed) -> None:
        """Handle the send button being pressed."""
        if event.button.id == "send_button":
            message = self.query_one("#message_input", Input).value
            if message:
                await self.websocket.send(message)
                self.query_one("#messages", Static).update(
                    f"{self.query_one('#messages', Static).renderable}\nSent: {message}"
                )
                self.query_one("#message_input", Input).value = ""  # Clear input

    async def on_input_submitted(self, event: Input.Submitted) -> None:
        """Handle the input being submitted (e.g., pressing Enter)."""
        message = event.value
        if message:
            await self.websocket.send(message)
            self.query_one("#messages", Static).update(
                f"{self.query_one('#messages', Static).renderable}\nSent: {message}"
            )
            self.query_one("#message_input", Input).value = ""  # Clear input

    async def on_unmount(self) -> None:
        """Close the WebSocket connection when the app unmounts."""
        await self.websocket.close()

if __name__ == "__main__":
    app = WebSocketClientApp()
    app.run()
