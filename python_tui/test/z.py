from textual.app import App, ComposeResult
from textual.widgets import Static, Button, Label
from textual.containers import ScrollableContainer, Vertical
from textual.reactive import var

class ScrollingDemonstration(App):
    CSS = """
            #main-scroll-view {
            height: 50vh;  /* Fixed height viewport */
            border: tall $background;
            overflow-y: auto;  /* Ensure vertical scrolling */
        }
        
        #content-container {
            width: 100%;
        }
        
        .section-header {
            background: $primary;
            color: $text;
            padding: 1;
            margin-bottom: 1;
            text-align: center;
        }
        
        .content-item {
            padding: 1;
            border-bottom: tall $background-lighten-2;
        }
        
        #control-panel {
            margin-top: 2;
            padding: 1;
            background: $background-lighten-1;
        }
        
        Button {
            margin: 1;
            width: 20;
        }
        
        .header {
            text-align: center;
            color: $text-muted;
            margin-bottom: 1;
        }
    """

    scroll_position = var(0)

    def compose(self) -> ComposeResult:
        # Main layout with scroll view and control panel
        with Vertical():
            # Scrollable Container
            with ScrollableContainer(id="main-scroll-view"):
                yield Vertical(id="content-container")
            
            # Control Panel
            with Vertical(id="control-panel"):
                yield Label("Scroll Position Controls", classes="header")
                yield Vertical(
                    Button("Scroll Top", id="scroll-top", variant="primary"),
                    Button("Scroll Middle", id="scroll-middle", variant="default"),
                    Button("Scroll Bottom", id="scroll-bottom", variant="success"),
                    Button("Custom Scroll", id="scroll-custom", variant="warning")
                )
                
                # Position Display
                yield Label(
                    "Current Scroll Position: [bold]0[/bold]", 
                    id="position-display"
                )

    def on_mount(self) -> None:
        # Populate content with many items
        content_container = self.query_one("#content-container", Vertical)
        
        # Generate diverse content
        for i in range(100):
            if i % 5 == 0:
                # Every 5th item is a header
                content_container.mount(
                    Static(f"Header {i//5 + 1}", classes="section-header")
                )
            
            # Regular content
            content_container.mount(
                Static(f"Content Item {i+1}", classes="content-item")
            )

    def on_button_pressed(self, event: Button.Pressed) -> None:
        scroll_view = self.query_one("#main-scroll-view", ScrollableContainer)
        content_container = self.query_one("#content-container", Vertical)
        position_display = self.query_one("#position-display", Label)

        # Detailed scroll calculations
        if event.button.id == "scroll-top":
            # Scroll precisely to the top
            scroll_position = 0
            scroll_view.scroll_to(0, scroll_position)
        
        elif event.button.id == "scroll-middle":
            # Scroll to exact middle of content
            scroll_position = content_container.size.height // 2
            scroll_view.scroll_to(0, scroll_position)
        
        elif event.button.id == "scroll-bottom":
            # Scroll to the very bottom
            scroll_position = content_container.size.height
            scroll_view.scroll_to(0, scroll_position)
        
        elif event.button.id == "scroll-custom":
            # Custom scroll positioning
            scroll_position = content_container.size.height * 0.75  # 75% down
            scroll_view.scroll_to(0, scroll_position)

        # Update position display
        position_display.update(
            f"Current Scroll Position: [bold]{scroll_position}[/bold]"
        )

if __name__ == "__main__":
    app = ScrollingDemonstration()
    app.run()
