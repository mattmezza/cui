"""Headless real-component bridge behavior and Python callback isolation."""
import unittest
from cui_bridge import App, BridgeError, button, column, label, row


def click(app, widget):
    x, y, w, h = app.bounds(widget)
    app.queue_pointer("down", x + w / 2, y + h / 2)
    app.queue_pointer("up", x + w / 2, y + h / 2)
    app.step()


class BridgeTests(unittest.TestCase):
    def test_callback_can_close_or_request_close(self):
        for dispose in (False, True):
            with App() as app:
                save = button("close").on_click(
                    lambda event: app.close() if dispose else app.request_close())
                app.mount(row().children(save))
                app.step()
                click(app, save)
                self.assertEqual(app.dispatch(), 1)

    def test_real_click_callback_mutation_and_disabled(self):
        seen = []
        caption = label("Ready ✓", key="caption")
        save = button("save").primary().rounded_md().size("2xl").label("Save changes")
        save.hover_color("#7799CC")
        save.on_click(lambda event: (seen.append(event.frame_sequence),
                                     caption.label("Saved — café")))
        root = row().gap_units(2).children(save, caption)
        with App() as app:
            app.mount(root)
            app.step()
            click(app, save)
            self.assertEqual(seen, [])
            self.assertEqual(app.dispatch(), 1)
            self.assertEqual(len(seen), 1)
            app.step()
            save.disabled()
            app.step()
            click(app, save)
            self.assertEqual(app.dispatch(), 0)
            self.assertEqual(len(seen), 1)
            save.disabled(False)
            app.step()
            click(app, save)
            self.assertEqual(app.dispatch(), 1)
            self.assertEqual(len(seen), 2)

    def test_callback_exception_is_outside_frame(self):
        save = button("save").on_click(lambda event: (_ for _ in ()).throw(ValueError("boom")))
        with App() as app:
            app.mount(column().children(save))
            app.step()
            click(app, save)
            with self.assertRaisesRegex(ValueError, "boom"):
                app.dispatch()
            save.label("Still live")
            app.step()

    def test_cross_app_and_dispose(self):
        widget = button("save")
        first = App()
        second = App()
        try:
            first.mount(row().children(widget))
            with self.assertRaises(ValueError):
                second.mount(row().children(widget))
            with self.assertRaises(ValueError):
                second.bounds(widget)
        finally:
            first.close()
            second.close()
        with self.assertRaises(RuntimeError):
            first.step()
        with self.assertRaises(RuntimeError):
            widget.label("after dispose")

    def test_invalid_and_unsupported(self):
        for value in (-1, 0, 2**32, 2**32 + 1, True, 1.5):
            with self.assertRaises(ValueError):
                App(max_nodes=value)
            with self.assertRaises(ValueError):
                App(max_events=value)
        with self.assertRaises(ValueError):
            button("x").size("20xl")
        with self.assertRaises(ValueError):
            row().gap(-1)
        with self.assertRaises(ValueError):
            button("x").ghost().hover_color("#000000")
        with self.assertRaises(ValueError):
            button("x").rounded_md().children(label("bad"))
        with self.assertRaises(ValueError):
            App(backend="sdl")
        with self.assertRaises(UnicodeEncodeError):
            button("bad").label("\ud800")
        with self.assertRaises(ValueError):
            App(title="bad\x00title")
        with self.assertRaises(ValueError):
            button("bad").label("a" * 4097)
        with self.assertRaises(ValueError):
            button("bad").label("bad\x00label")
        with App() as app:
            good = button("good").label("Before")
            with self.assertRaises(ValueError):
                app.mount(row().children(button("bad\x00key")))
            cycle = row("cycle")
            cycle.children(cycle)
            with self.assertRaises(ValueError):
                app.mount(cycle)
            app.mount(row().children(good))
            app.step()
            with self.assertRaises(ValueError):
                good.label("After\x00truncated")
            self.assertEqual(good._text, "Before")
            good.label("After")
            app.step()


if __name__ == "__main__":
    unittest.main()
