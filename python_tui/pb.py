from textual import on
from textual.app import App, ComposeResult
from textual.widgets import Button

class Btx(App[str | None]):
    """Demo of how to make different handlers for a specific events while using the concept of decoraters"""
    CSS_PATH = "pb.tcss"

    def compose(self) -> ComposeResult:
        yield Button("Press Me", id="press")
        yield Button("Toggle theme", classes="toggle dark")
        yield Button("Quit", id="quit")

    @on(Button.Pressed, "#press")
    def pressed_btn(self) -> None:
        self.bell()

    @on(Button.Pressed, ".toggle.dark")
    def toggle_theme(self) -> None:
        self.theme = (
                "textual-dark" if self.theme == "textual-light" else "textual-light"
        )

    @on(Button.Pressed, "#quit")
    def quit(self) -> None:
        self.exit()

if __name__ == "__main__":
    app = Btx()
    app.run()
