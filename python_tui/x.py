from textual.widgets import Static
from textual.app import App
from textual.containers import Horizontal, VerticalScroll

TEXT = """I must not fear.
Fear is the mind-killer.
Fear is the little-death that brings total obliteration.
I will face my fear.
I will permit it to pass over me and through me.
And when it has gone past, I will turn the inner eye to see its path.
Where the fear has gone there will be nothing. Only I will remain."""


class OverflowApp(App):
    CSS_PATH = "overflow.tcss"

    def compose(self):
        # Create the scrollable content
        self.left_scroll = VerticalScroll(
            Static(TEXT),
            Static(TEXT),
            Static(TEXT),
            Static(TEXT),
            Static(TEXT),
            Static(TEXT),
            id="left",
        )
        self.right_scroll = VerticalScroll(
            Static(TEXT), Static(TEXT), Static(TEXT), id="right"
        )

        # Add them to a horizontal container
        yield Horizontal(self.left_scroll, self.right_scroll)

    async def on_mount(self):
        # Scroll to the bottom for both VerticalScroll containers
        await self.left_scroll.scroll_end(animate=False)
        await self.right_scroll.scroll_end(animate=False)


if __name__ == "__main__":
    app = OverflowApp()
    app.run()

