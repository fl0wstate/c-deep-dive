from textual.app import App, ComposeResult
from textual.containers import Vertical, ScrollableContainer
from textual.widgets import Input, Static, Button
from textual.binding import Binding

class ChatApp(App):

    CSS = """
    #scroll-view {
    border: solid $primary;
    height: 80%; /* Adjust as needed */
    width: auto;
    overflow: auto; /* Horizontal hidden, vertical scrollable */
    padding: 1 2;
    }

    #message-container {
        padding: 1 2;
    }

    .message {
        background: $surface-lighten-3;
        border: solid $primary;
        color: $text-warning;
        padding: 1;
        margin: 1 0;
        width: auto;
    }
    """
    BINDINGS = [
        Binding("enter", "send_message", "Send Message"),
    ]

    def compose(self) -> ComposeResult:
        yield ScrollableContainer(
            Vertical(id="message-container"),
            id="scroll-view",
        )
        yield Input(placeholder="Type your message here...", id="input")
        yield Button("Send", id="send", action="send_message")

    def action_send_message(self) -> None:
        # Get the input widget and fetch its value
        input_widget = self.query_one("#input", Input)
        message = input_widget.value.strip()

        if message:
            # Create a new Static widget for the message
            message_widget = Static(message, classes="message")

            # Add it to the messages container
            messages_container = self.query_one("#message-container", Vertical)
            messages_container.mount(message_widget)

            # Scroll to the bottom after adding a new message
            scroll_view = self.query_one("#scroll-view", ScrollableContainer)
            scroll_view.scroll_end()

            # Clear the input field
            input_widget.value = ""


if __name__ == "__main__":
    chat = ChatApp()
    chat.run()
