from textual.app import App, ComposeResult
from textual.containers import Vertical, Horizontal
from textual.widgets import Input, Static, Button

class InputDemoApp(App):

    CSS_PATH = "pin.tcss"
    def compose(self) -> ComposeResult:
        # Create a layout: input field, button, and output display
        yield Vertical(
            Static("", id="output"),
            Horizontal(
                Input(placeholder="Type your message here...", id="input"),
            ),
        )

    async def on_button_pressed(self, event: Button.Pressed) -> None:
        # Handle "Enter" button click
        if event.button.id == "enter_button":
            input_widget = self.query_one("#input", Input)
            output = self.query_one("#output", Static)
            if input_widget.value.strip():
                output.update(f"Submitted: {input_widget.value}")
                input_widget.value = ""  # Clear input after submission

    async def on_input_submitted(self, event: Input.Submitted) -> None:
        # Optional: Handle Enter keypress in the input field
        # Evaluates the data being passed through to the output
        input_widget = self.query_one("#input", Input)
        if (input_widget.value == "exit"):
            self.exit()
        output = self.query_one("#output", Static)
        output.update(f"Submitted: {event.value}")
        input_widget.value = ""  # Clear input after submission



# Run the app
if __name__ == "__main__":
    app = InputDemoApp()
    app.run()
