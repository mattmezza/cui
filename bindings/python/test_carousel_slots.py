"""Retained Carousel control content and older provider preflight."""
import unittest
from unittest.mock import patch
import cui_bridge as c


class CarouselSlots(unittest.TestCase):
    def test_authoring(self):
        for role in ("", "bogus", 1):
            with self.assertRaises(ValueError):
                c.carousel_slot("slot", role)
        with self.assertRaises(ValueError):
            c.carousel_slot("", "previous")
        with self.assertRaises(ValueError):
            c.column().children(c.carousel_slot("p", "previous"))
        rail = c.carousel("rail").children(c.carousel_slot("p", "previous"))
        with self.assertRaises(ValueError):
            rail.children(c.carousel_slot("other", "previous"))
        self.assertEqual(len(rail._children), 1)

    def test_provider_and_passive_preflight(self):
        with c.App() as app:
            with patch.object(app._api, "create_carousel_slot", None):
                self.assertFalse(app.supports_carousel_slots())
                rail = c.carousel("rail").children(c.carousel_slot("p", "previous"))
                with self.assertRaisesRegex(RuntimeError, "unavailable"):
                    app.mount(c.column().children(rail))
                self.assertIsNone(rail._app)
            if not app.supports_carousel_slots():
                self.skipTest("provider lacks Carousel slots")
            slot = c.carousel_slot("p", "previous").children(c.column().children(c.button("bad")))
            with self.assertRaisesRegex(ValueError, "passive"):
                app.mount(c.column().children(c.carousel("rail").children(slot)))
            self.assertIsNone(slot._app)

    def test_retained_content(self):
        with c.App() as app:
            if not app.supports_carousel_slots():
                self.skipTest("provider lacks Carousel slots")
            previous = c.carousel_slot("p", "previous", "Go back").children(c.label("Back"))
            marker = c.carousel_slot("m", "marker").children(c.label("Slide one"))
            first, second = c.item("first").children(marker), c.item("second")
            events = []
            rail = c.carousel("rail").carousel_controls().on_change(events.append).children(previous,
                first, second,
                c.carousel_slot("n", "next").children(c.label("Forward")))
            app.mount(c.column().children(rail)).step()
            self.assertIs(previous._app, app)
            self.assertIs(marker._app, app)
            self.assertIs(rail.selected(), first)
            x, y, w, h = app.numeric_part_bounds(rail, 2)
            app.queue_pointer("down", x + w / 2, y + h / 2)
            app.queue_pointer("up", x + w / 2, y + h / 2)
            app.step(); app.dispatch()
            self.assertIs(rail.selected(), second)
            self.assertEqual(len(events), 1)


if __name__ == "__main__":
    unittest.main()
