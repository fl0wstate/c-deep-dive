import time
from textual.app import App, ComposeResult
from textual.containers import ScrollableContainer, Grid
from textual.screen import Screen, ModalScreen
from textual.widgets import Header, Footer, Input, Static, Button, Label
from rich.text import Text

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
        # yield Header(show_clock=True)
        yield ScrollableContainer(id="message_container")
        yield Input(placeholder="Type your message...", id="message_input")
        yield Footer()

    def on_mount(self) -> None:
        # Add some initial messages
        self.add_message("System", "Welcome to the chat room!", time.time())
        self.add_message("Alice", "Hey everyone!", time.time() + 1)
        self.add_message("Bob", "Hi there!", time.time() + 2)

    def on_input_submitted(self, message: Input.Submitted) -> None:
        """Handle message submission."""
        if message.value.strip():
            self.add_message("You", message.value, time.time())
            message.input.value = ""  # Clear input after sending

    def add_message(self, username: str, content: str, timestamp: float) -> None:
        """Add a new message to the chat container."""

        # applying some modes to the input box
        chat_box = self.query_one("#message_input")
        chat_box.border_title = "Message"
        chat_box.border_subtitle = "number of characters typed"

        message = Message(username, content, timestamp)
        message_container = self.query_one("#message_container")
        # adding details about the specified chat room
        message_container.border_title = "Room Name"
        message_container.border_subtitle = "44"
        message_container.mount(message)
        # Automatically scroll to the bottom
        message_container.scroll_end()

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
        "chat": ChatScreen,
        "quit": QuitScreen,
    }
    BINDINGS = [
        ("ctrl+c", "request_quit", "Quit"),
        ]
    CSS_PATH = "dialog.tcss"

    # setting up the user name should be done before mounting the chat room
    # provide some sort of a form that will allow users to fill in and make new chatroom

    def on_mount(self) -> None:
        self.push_screen("chat")

    def action_request_quit(self) -> None:
        self.push_screen("quit")

if __name__ == "__main__":
    app = ChatApp()
    app.run()
