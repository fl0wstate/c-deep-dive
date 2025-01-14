import time
import asyncio
from textual import log
from socket import AF_INET, SOCK_STREAM
from textual.app import App, ComposeResult
from textual.containers import ScrollableContainer, Grid
from textual.screen import Screen, ModalScreen
from textual.widgets import Header, Footer, Input, Static, Button, Label
from rich.text import Text
from textual.worker import Worker, get_current_worker

class ChatClient:
    """Handles network communication for the chat client."""

    CSS = "cli.tcss"

    # this will be where the users will insert
    def __init__(self, host: str = "127.0.0.1", port: int = 7000):
        self.host = host
        self.port = port
        self.username = None
        self.room = None
        self.socket = None
        self.connected = False

    async def connect(self, username: str, room: str = "general"):
        """Connect to the chat server."""
        try:
            reader, writer = await asyncio.open_connection(self.host, self.port)
            self.username = username
            self.room = room
            self.connected = True
 
            # Send initial connection message
            connect_msg = f"CONNECT|{username}|{room}".encode()
            writer.write(connect_msg)
            await writer.drain()
 
            return reader, writer
        except Exception as e:
            self.connected = False
            raise ConnectionError(f"Failed to connect: {str(e)}")

    async def send_message(self, writer, message: str):
        """Send a message to the server."""
        if self.connected:
            try:
                formatted_msg = f"MSG|{self.username}|{self.room}|{message}".encode()
                writer.write(formatted_msg)
                await writer.drain()
            except Exception as e:
                raise ConnectionError(f"Failed to send message: {str(e)}")

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

class LoginScreen(Screen):
    """Screen for entering username and room."""
    def compose(self) -> ComposeResult:
        yield Grid(
            Label("Enter your username:", id="username_label"),
            Input(placeholder="Username", id="username_input"),
            Label("Enter room name:", id="room_label"),
            Input(placeholder="Room (default: general)", id="room_input"),
            Button("Join", variant="primary", id="join_button"),
            id="login_form"
        )

    def on_button_pressed(self, event: Button.Pressed) -> None:
        if event.button.id == "join_button":
            username = self.query_one("#username_input").value
            room = self.query_one("#room_input").value or "general"
            if username:
                self.app.username = username
                self.app.room = room
                self.app.push_screen("chat")

class QuitScreen(ModalScreen):
    """Modal screen for quit confirmation."""
    def compose(self) -> ComposeResult:
        yield Grid(
            Label("Are you sure you want to quit?", id="question"),
            Button("Quit", variant="error", id="quit"),
            Button("Cancel", variant="primary", id="cancel"),
            id="dialog"
        )

    def on_button_pressed(self, event: Button.Pressed) -> None:
        if event.button.id == "quit":
            self.app.exit()
        elif event.button.id == "cancel":
            self.dismiss()

class ChatScreen(Screen):
    """Main chat screen with message list and input field."""
    def compose(self) -> ComposeResult:
        yield Header(show_clock=True)
        yield ScrollableContainer(id="message_container")
        yield Input(placeholder="Type your message...", id="message_input")
        yield Footer()

    def on_mount(self) -> None:
        """Initialize chat client and start message receiving worker."""
        self.chat_client = ChatClient()
        self.start_background_tasks()

    async def connect_to_server(self) -> None:
        """Connect to the chat server."""
        try:
            self.reader, self.writer = await self.chat_client.connect(
                self.app.username,
                self.app.room
            )
            self.add_message("System", f"Connected to chat room: {self.app.room}", time.time())
            # Start receiving messages
            worker = self.app.run_worker(self.receive_messages, start=True)
        except ConnectionError as e:
            self.add_message("System", f"Connection error: {str(e)}", time.time())

    async def receive_messages(self) -> None:
        """Receive and display messages from the server."""
        worker = get_current_worker()
        while worker.is_running:
            try:
                data = await self.reader.readuntil(b'^')
                if not data:
                    break
                message = data.decode()
                log(message)
                # Parse message according to your protocol
                self.add_message("Server", message, time.time())
            except Exception as e:
                self.add_message("System", f"Error receiving message: {str(e)}", time.time())
                break

    def start_background_tasks(self) -> None:
        """Start the connection and message receiving tasks."""
        worker = self.app.run_worker(self.connect_to_server, start=True)

    async def on_input_submitted(self, message: Input.Submitted) -> None:
        """Handle message submission."""
        if message.value.strip():
            try:
                await self.chat_client.send_message(self.writer, message.value)
                self.add_message(self.app.username, message.value, time.time())
                message.input.value = ""  # Clear input after sending
            except ConnectionError as e:
                self.add_message("System", f"Failed to send message: {str(e)}", time.time())

    def add_message(self, username: str, content: str, timestamp: float) -> None:
        """Add a new message to the chat container."""
        message = Message(username, content, timestamp)
        message_container = self.query_one("#message_container")
        message_container.mount(message)
        message_container.scroll_end()

class ChatApp(App):
    """The main Textual chat application."""
    SCREENS = {
        "login": LoginScreen,
        "chat": ChatScreen
    }
    
    BINDINGS = [
        ("q", "quit", "Quit"),
        ("ctrl+c", "quit", "Quit")
    ]

    def __init__(self):
        super().__init__()
        self.username = None
        self.room = None

    def on_mount(self) -> None:
        self.push_screen("login")

    def action_quit(self) -> None:
        """Show quit confirmation dialog."""
        self.push_screen(QuitScreen())

if __name__ == "__main__":
    app = ChatApp()
    app.run()
