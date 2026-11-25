import ctypes as C
import os
from threading import Thread
import unittest
from unittest.mock import patch

import cui_bridge as c


class DataTablePagingTests(unittest.TestCase):
    def test_builder_captures_wheel_demand_and_copies_the_record(self):
        rows = [[f"Message {i}"] for i in range(8)]
        keys = [*range(101, 108), 0xffffffff]
        events = []
        table = (c.data_table("inbox").table_data(["Subject"], rows, row_keys=keys)
                 .visual_size(300, 160).configure_load_more().on_change(events.append))
        with c.App() as app:
            self.assertTrue(app.supports_data_table_paging())
            app.mount(c.column("root").children(table))
            app.step()
            self.assertIsNone(table.poll_load_more())
            x, y, _, _ = table.table_cell_bounds(1, 0)
            app.queue_scroll(x + 10, y + 10, 0, -20)
            app.step()
            request = table.poll_load_more()
            self.assertIsInstance(request, c.DataTableLoadMore)
            self.assertGreater(request.frame_sequence, 0)
            self.assertEqual((request.row_count, request.visible_end), (8, 8))
            self.assertGreater(request.visible_begin, 0)
            self.assertEqual((request.last_row_key, request.capacity), (0xffffffff, 32))
            self.assertIsNone(table.poll_load_more())
            self.assertEqual(app.dispatch(), 0)
            self.assertEqual(events, [])

            # Loading leaves the rows interactive. The selection persists on append.
            table.configure_load_more(enabled=False)
            x, y, width, height = table.table_cell_bounds(6, 0)
            app.queue_pointer("down", x + width / 2, y + height / 2)
            app.queue_pointer("up", x + width / 2, y + height / 2)
            app.step()
            app.dispatch()
            self.assertEqual([event.value for event in events], [keys[6]])
            table.table_data(["Subject"], [*rows, ["New"]], row_keys=[*keys, 109])
            app.step()
            self.assertEqual(table.table_cell_bounds(6, 0)[1], y)
            self.assertEqual(table.table_state()["row"], keys[6])
            self.assertIsNone(table.poll_load_more())
            self.assertEqual(app.dispatch(), 0)
        self.assertEqual((request.row_count, request.last_row_key), (8, 0xffffffff))
        with self.assertRaisesRegex(RuntimeError, "mounted"):
            table.poll_load_more()

    def test_empty_startup_validation_and_owner_thread(self):
        table = c.data_table("empty").configure_load_more()
        root = c.column("root").children(table)
        with c.App(max_nodes=2) as app:
            with self.assertRaisesRegex(ValueError, "column headers"):
                app.mount(root)
            table.table_data(["Subject"], [], row_keys=[])
            app.mount(root)
            app.step()
            for invalid in (-1, 33, 1.0, True, 2**32):
                with self.subTest(invalid=invalid), self.assertRaisesRegex(ValueError, "threshold"):
                    table.configure_load_more(threshold=invalid)
            with self.assertRaises(TypeError):
                table.configure_load_more(enabled=1)
            self.assertEqual(table.poll_load_more().row_count, 0)
            table.configure_load_more()
            app.step()
            self.assertIsNone(table.poll_load_more())
            errors = []

            def other_thread():
                for operation in (table.configure_load_more, table.poll_load_more):
                    try:
                        operation()
                    except RuntimeError as error:
                        errors.append(str(error))

            worker = Thread(target=other_thread)
            worker.start()
            worker.join()
            self.assertEqual(len(errors), 2)
            self.assertTrue(all("owner thread" in error for error in errors))

    def test_wire_validation_and_exact_unsigned_values(self):
        table = c.data_table("wire").table_data(["Subject"], [["Example"]], row_keys=[0xffffffff])
        with c.App() as app:
            app.mount(c.column("root").children(table))
            fields = dict(frame=2**63 + 1, row_count=1, visible_begin=0, visible_end=1,
                          last_row_key=0xffffffff, capacity=32, reserved=0)

            def fill_record(_app, _node, size, out):
                self.assertEqual(size, 32)
                record = C.cast(out, C.POINTER(c._DataTableLoadMoreV1)).contents
                for name, value in fields.items():
                    setattr(record, name, value)
                return 0

            with patch.object(app._api, "poll_table_paging", side_effect=fill_record):
                request = table.poll_load_more()
                self.assertEqual(request.frame_sequence, 2**63 + 1)
                self.assertEqual(request.last_row_key, 0xffffffff)
                for field, value in (("frame", 0), ("reserved", 1), ("capacity", 1),
                                     ("visible_end", 2), ("visible_begin", 2),
                                     ("last_row_key", 0)):
                    with self.subTest(field=field):
                        previous, fields[field] = fields[field], value
                        with self.assertRaisesRegex(c.BridgeError, "malformed"):
                            table.poll_load_more()
                        fields[field] = previous

    def check_unsupported(self, library, *, keyed):
        with c.App(library=library, max_nodes=2) as app:
            self.assertFalse(app.supports_data_table_paging())
            self.assertEqual(app.supports_keyed_data_table(), keyed)
            keys = [7] if keyed else None
            table = c.data_table("inbox").table_data(["Subject"], [["Kept"]], row_keys=keys)
            root = c.column("root").children(table.configure_load_more())
            with patch.object(app._api, "node_create", wraps=app._api.node_create) as create:
                with self.assertRaisesRegex(RuntimeError, "compatible bridge symbol group"):
                    app.mount(root)
                create.assert_not_called()
            # A legacy app can still mount positional or supported keyed data.
            table.configure_load_more(enabled=False)
            app.mount(root)
            app.step()
            for operation in (table.poll_load_more, table.configure_load_more):
                with self.assertRaisesRegex(RuntimeError, "compatible bridge symbol group"):
                    operation()
            table.table_data(["Subject"], [["Updated"]], row_keys=keys)
            self.assertEqual(table.current_cell(0, 0), "Updated")

    def test_incomplete_or_wrong_size_provider_preserves_keyed_setter(self):
        library = os.environ["CUI_BRIDGE_LIBRARY"]
        provider = C.CDLL(library)
        size_symbol = "cui_bridge_data_table_load_more_size_v1"
        for omitted in (size_symbol, "cui_bridge_data_table_configure_load_more_v1",
                        "cui_bridge_data_table_poll_load_more_v1", None):
            class IncompatibleBridge:
                def __getattr__(self, name):
                    if name == omitted:
                        raise AttributeError(name)
                    if omitted is None and name == size_symbol:
                        return lambda: 24
                    return getattr(provider, name)

            with self.subTest(omitted=omitted), patch.object(C, "CDLL", return_value=IncompatibleBridge()):
                self.check_unsupported(library, keyed=True)

    def test_published_legacy_bridge(self):
        library = os.environ.get("CUI_DATA_TABLE_LEGACY_LIBRARY")
        if not library:
            self.skipTest("published pre-paging bridge not configured")
        self.check_unsupported(library, keyed=False)


if __name__ == "__main__":
    unittest.main()
