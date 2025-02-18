import time
import json
import zlib
from textual import log
from textual.app import App, ComposeResult
from textual.containers import ScrollableContainer, Grid
from textual.screen import Screen, ModalScreen
from textual.widgets import Input, Static, Button, Label
from rich.text import Text
import websockets

WS_URI="ws://127.0.0.1:6729"

class Message(Static):
    """A widget representing an individual chat message."""
    def __init__(self, username: str, content: str, timestamp: float):
        super().__init__()
        self.username = username
        self.content = content
        self.timestamp = timestamp

    def compose(self) -> ComposeResult:
        formatted_time = time.strftime("%H:%M:%S", time.localtime(self.timestamp))
        message_text = Text()
        message_text.append(f"[{formatted_time}] ", style="dim")
        message_text.append(f"{self.username}: ", style="bold green")
        message_text.append(self.content)
        yield Static(message_text)

class ChatScreen(Screen):
    """Main chat screen with message list and input field."""
    def compose(self) -> ComposeResult:
        """Create child widgets for the app."""
        yield ScrollableContainer(
            id="messages_container",
        )
        yield Input(placeholder="Type a message...", id="message_input")

    async def on_mount(self) -> None:
        """Start the WebSocket connection when the app mounts."""
        log("Hello there developer you are now inside the  chatscreen")
        self.websocket = await websockets.connect(WS_URI)
        self.set_interval(1, self.receive_messages)  # Poll for incoming messages

    async def receive_messages(self) -> None:
        """Receive messages from the WebSocket server."""
        try:
            messages_container = self.query_one("#messages_container")
            messages_container.border_title = self.app.room
            messages_container.border_subtitle = "CXfldajfla: 44"
            messages_container.scroll_end()
            message_data = await self.websocket.recv()

            if (isinstance(message_data, bytes)):
                decompressed_data = zlib.decompress(message_data)
                decompressed_data.decode("utf-8")
                message = json.loads(decompressed_data)
                messages_container.mount(Message(message["username"], message["message"], time.time()))

        except websockets.exceptions.ConnectionClosed:
            self.query_one("#messages_container").mount(Message("System","error",time.time()))
            self.app.exit()

    async def on_input_submitted(self, event: Input.Submitted) -> None:
        """Handle the input being submitted (e.g., pressing Enter)."""
        message = event.value
        #TODO: Capture the message, create a json object around it. Compress it send it as bytes.
        if message:
            if (message == "exit"):
                await self.websocket.close()
                self.app.exit()
            else:
                metadata = {
                    "username": self.app.username,
                    "message": message,
                }
                json_metadata = json.dumps(metadata, indent=4)
                log(json_metadata)

                # introduce the compression algorithm here
                compressed_data =  zlib.compress(json_metadata.encode("utf-8"))
                await self.websocket.send(compressed_data)

                self.query_one("#messages_container").mount(Message(metadata["username"], metadata["message"] , time.time()))
                self.query_one("#message_input", Input).value = ""  # Clear input

    async def on_unmount(self) -> None:
        """Close the WebSocket connection when the app unmounts."""
        await self.websocket.close()

class LoginScreen(Screen):
    """This will be the loginScreen displayed before anything else"""
    def compose(self) -> ComposeResult:
        """Build up this screen when the script is runned"""
        yield Grid(
            Label("Enter username: ", id="username_label"),
            Input(placeholder="Username...", id="username_input"),
            Label("Enter room name: ", id="room_label"),
            Input(placeholder="Room (default: General)", id="room_input"),
            Button("Join", variant="primary", id="join-button"),
            id="grid"
        )
 
    def on_button_pressed(self, event: Button.Pressed) ->None:
        """Handle the even when the button is pressed by the user"""
        if event.button.id == "join-button":
            username = self.query_one("#username_input").value
            room = self.query_one("#room_input").value or "General"
            if username:
                self.app.username = username
                self.app.room = room
                self.app.push_screen("chat")

class QuitScreen(ModalScreen):
    def compose(self) -> ComposeResult:
        yield Grid(
            Label("Are you sure you want to quit?", id="question"),
            Button("Quit", variant="success", id="yes_button"),
            Button("Cancel", id="no_button"),
            id="dialog"
        )
    def on_button_pressed(self, event: Button.Pressed) -> None:
        if event.button.id == "yes_button":
            self.app.exit()
        else:
            self.app.pop_screen()


class ChatApp(App):
    """The main Textual chat application."""
    SCREENS = {
        "login": LoginScreen,
        "chat": ChatScreen,
        "quit": QuitScreen,
    }
    BINDINGS = [
        ("ctrl+c", "request_quit", "quit"),
        ]
    CSS_PATH = "dialog.tcss"

    # setting up the user name should be done before mounting the chat room
    # provide some sort of a form that will allow users to fill in and make new chatroom
    def on_mount(self) ->None:
            self.push_screen("login")

    def action_request_quit(self) -> None:
        self.push_screen("quit")

if __name__ == "__main__":
    app = ChatApp()
    app.run()
