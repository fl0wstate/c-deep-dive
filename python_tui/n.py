from textual.app import App, ComposeResult
from textual.widgets import Input, Static
from textual.containers import ScrollableContainer, Vertical

class ChatApp(App):
    CSS = """
    #scroll-view {
        height: 80vh;
        border: solid yellow;
    }
    
    #message-container {
        width: 100%;
        margin-bottom: 1;
        overflow-y: auto; # added when you want horizontal scrolling
    }
    
    .message {
        margin-bottom: 11;
        background: $primary-background;
        color: $text;
        padding: 1;
        height: 80%;
        width: 100%;
    }
        """
    def compose(self) -> ComposeResult:
        with ScrollableContainer(id="scroll-view"):
            yield Vertical(id="message-container")
        yield Input(placeholder="Type a message...", id="input")

    def on_input_submitted(self, message: Input.Submitted) -> None:
        input_widget = message.input
        text = input_widget.value.strip()
        
        if text:
            if text != "exit":
                # Create message
                message_widget = Static(text, classes="message")
                
                # Get container
                messages_container = self.query_one("#message-container", Vertical)
                
                # Debug print
                print(f"Adding message: {text}")
                print(f"Current message count: {len(messages_container.children)}")
                
                # Mount message
                messages_container.mount(message_widget)
                
                # Attempt multiple scrolling methods
                scroll_view = self.query_one("#scroll-view", ScrollableContainer)
                scroll_view.scroll_to(10000, 0)
                # Clear input
                input_widget.value = ""
            else:
                self.exit()


if __name__ == "__main__":
    app = ChatApp()
    app.run()
