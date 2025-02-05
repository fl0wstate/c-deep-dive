from textual.app import App, ComposeResult
from textual.widgets import Placeholder, Header, Footer

class Application(App):
    def compose(self) -> ComposeResult:
        yield Header(show_clock=True)
        yield Placeholder("Hello World")
        yield Footer()

def main() -> None:
    app = Application()
    app.run()

if __name__ == "__main__":
    main()
