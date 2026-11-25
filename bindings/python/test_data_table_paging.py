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
                for field, value in (("frame", 0), ("reserved", 1), ("capacity", 1), ("capacity", 33),
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


class DataTableCapacityTests(unittest.TestCase):
    def test_opted_in_append_checks_wire_capacity_and_preserves_viewport(self):
        headers = ["Subject"]
        rows = [[f"Message {i}"] for i in range(40)]
        keys = [*range(101, 140), 0xffffffff]
        events = []
        table = (c.data_table("inbox")
                 .table_data(headers, rows[:32], row_keys=keys[:32], row_capacity=96)
                 .visual_size(300, 160).configure_load_more().on_change(events.append))
        with c.App() as app:
            self.assertEqual(app.data_table_max_row_capacity(), 10_000)
            app.mount(c.column("root").children(table))
            app.step()
            x, y, _, _ = table.table_cell_bounds(1, 0)
            app.queue_scroll(x + 10, y + 10, 0, -100)
            app.step()
            cell = table.table_cell_bounds(30, 0)
            app.queue_pointer("down", cell[0] + 10, cell[1] + 10)
            app.queue_pointer("up", cell[0] + 10, cell[1] + 10)
            app.step()
            app.dispatch()
            self.assertEqual([event.value for event in events], [keys[30]])
            with patch.object(app._api, "set_matrix_capacity", return_value=5):
                with self.assertRaises(c.BridgeError) as failure:
                    table.table_data(headers, rows, row_keys=keys, row_capacity=64)
            self.assertEqual(failure.exception.code, 5)
            self.assertEqual(table.current_cell(31, 0), "Message 31")
            request = table.poll_load_more()
            self.assertEqual((request.row_count, request.capacity), (32, 96))
            self.assertEqual(table.table_state()["row"], keys[30])

            source = [row[:] for row in rows]
            table.table_data(headers, source, row_keys=iter(keys), row_capacity=64)
            source[-1][0] = "Modified in caller"
            app.step()
            self.assertEqual(table.table_cell_bounds(30, 0), cell)
            self.assertEqual(table.table_state()["row"], keys[30])
            self.assertEqual(table.current_cell(39, 0), "Message 39")
            self.assertEqual(app.dispatch(), 0)
            self.assertIsNone(table.poll_load_more())
            app.queue_scroll(x + 10, y + 10, 0, -100)
            app.step()
            request = table.poll_load_more()
            self.assertEqual((request.row_count, request.visible_end, request.capacity),
                             (40, 40, 64))
            self.assertEqual(request.last_row_key, 0xffffffff)
            self.assertIsNone(table.poll_load_more())

            # An ordinary replacement drops explicit capacity and keeps the load policy.
            table.table_data(headers, [], row_keys=[])
            app.step()
            self.assertEqual(table.poll_load_more().capacity, 32)

    def test_accepts_provider_limit_and_validates_inputs(self):
        table = c.data_table("big")
        for invalid in (True, 2.0, 0, -1, 10_001, 2**32):
            with self.subTest(invalid=invalid), self.assertRaisesRegex(ValueError, "row_capacity"):
                table.table_data(["Subject"], [], row_keys=[], row_capacity=invalid)
        with self.assertRaisesRegex(ValueError, "requires row_keys"):
            table.table_data(["Subject"], [], row_capacity=72)
        with self.assertRaisesRegex(ValueError, "matrix dimensions"):
            table.table_data(["Subject"], [["Data"]] * 33, row_keys=range(1, 34))
        with self.assertRaisesRegex(ValueError, "128 UTF-8 bytes"):
            table.table_data(["Subject"], [["é" * 65]], row_keys=[1], row_capacity=72)
        with self.assertRaisesRegex(ValueError, "distinct nonzero"):
            table.table_data(["Subject"], [["One"], ["Two"]], row_keys=[1, 1], row_capacity=72)

        def capped_stream(value, count):
            yield from [value] * count
            self.fail("iterator consumed beyond its dimension limit")

        for headers, rows, keys in (
                (capped_stream("H", 17), [], []),
                (["H"], capped_stream(["R"], 3), []),
                (["H"], [capped_stream("R", 2)], [1]),
                (["H"], [["R"]], capped_stream(1, 2))):
            with self.assertRaises(ValueError):
                table.table_data(headers, rows, row_keys=keys, row_capacity=2)

        with c.App() as app:
            limit = app.data_table_max_row_capacity()
            table.table_data(["Subject"], ([str(i)] for i in range(limit)),
                             row_keys=range(1, limit + 1), row_capacity=limit)
            app.mount(c.column("root").children(table))
            app.step()
            self.assertEqual(table.current_cell(limit - 1, 0), str(limit - 1))
            errors = []

            def other_thread():
                try:
                    table.table_data(["Subject"], [], row_keys=[], row_capacity=limit)
                except RuntimeError as error:
                    errors.append(str(error))

            worker = Thread(target=other_thread)
            worker.start()
            worker.join()
            self.assertEqual(len(errors), 1)
            self.assertIn("owner thread", errors[0])

    def check_provider(self, library, maximum):
        table = c.data_table("inbox").table_data(["Subject"], [], row_keys=[], row_capacity=96)
        root = c.column("root").children(table)
        with c.App(library=library, max_nodes=2) as app:
            self.assertEqual(app.data_table_max_row_capacity(), maximum)
            error = RuntimeError if maximum is None else ValueError
            with patch.object(app._api, "node_create", wraps=app._api.node_create) as create:
                with self.assertRaisesRegex(error, "capacity"):
                    app.mount(root)
                create.assert_not_called()
            table.table_data(["Subject"], [["Ordinary"]])
            app.mount(root)
            app.step()

            def rows():
                self.fail("provider preflight must precede iteration")
                yield []

            with self.assertRaisesRegex(error, "capacity"):
                table.table_data(["Subject"], rows(), row_keys=[], row_capacity=96)
            self.assertEqual(table.current_cell(0, 0), "Ordinary")
            if maximum is not None:
                table.table_data(["Subject"], [["Smaller"]], row_keys=[1], row_capacity=maximum)
                self.assertEqual(table.current_cell(0, 0), "Smaller")

    def test_complete_optional_group_and_provider_limit(self):
        library = os.environ["CUI_BRIDGE_LIBRARY"]
        provider = C.CDLL(library)
        query = "cui_bridge_data_table_max_row_capacity_v1"
        for omitted, size, limit in (
                (query, None, None),
                ("cui_bridge_data_table_set_keyed_capacity_v1", None, None),
                (None, 0, None), (None, 48, 48)):
            class LimitedBridge:
                def __getattr__(self, name):
                    if name == omitted:
                        raise AttributeError(name)
                    if name == query and size is not None:
                        return lambda: size
                    return getattr(provider, name)

            with self.subTest(omitted=omitted, size=size), patch.object(
                    C, "CDLL", return_value=LimitedBridge()):
                self.check_provider(library, maximum=limit)

    def test_published_legacy_provider_keeps_ordinary_tables_usable(self):
        library = os.environ.get("CUI_DATA_TABLE_LEGACY_LIBRARY")
        if not library:
            self.skipTest("published pre-capacity bridge not configured")
        self.check_provider(library, maximum=None)


if __name__ == "__main__":
    unittest.main()
