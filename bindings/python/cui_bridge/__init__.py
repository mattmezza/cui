"""Small retained Python authoring layer over CUI's portable bridge ABI v1.

The widget tree lives in Python. ``step`` submits it through real CUI components;
``dispatch`` invokes Python callbacks only after the C frame has returned.
"""
from __future__ import annotations

import ctypes as C
import datetime
from dataclasses import dataclass
from itertools import islice
import math
import os
from pathlib import Path
from threading import current_thread
from typing import Callable, ClassVar, Iterable, Iterator, Literal

from .radio_paint import RadioPaint, RadioPaintLayer, bind_radio_paint, radio_paint_wire

_DATA_TABLE_LEGACY_ROWS = 32
_DATA_TABLE_MAX_ROWS = 10_000


def _utf8(value: str, limit: int = 4096) -> bytes:
    if not isinstance(value, str):
        raise TypeError("expected a string")
    if "\x00" in value:
        raise ValueError("NUL is not allowed in bridge strings")
    encoded = value.encode("utf-8")
    if len(encoded) > limit:
        raise ValueError(f"bridge strings are limited to {limit} UTF-8 bytes")
    return encoded


class BridgeError(RuntimeError):
    """A rejected bridge operation with its C status code."""

    def __init__(self, code: int, message: str):
        super().__init__(f"CUI bridge {code}: {message}")
        self.code = code


class _Handle(C.Structure):
    _fields_ = [("slot", C.c_uint32), ("generation", C.c_uint32)]


class _VirtualListRow(C.Structure):
    _fields_ = [("item", _Handle), ("extent", C.c_float)]


class _Rect(C.Structure):
    _fields_ = [("x", C.c_float), ("y", C.c_float),
                ("width", C.c_float), ("height", C.c_float)]


class _Event(C.Structure):
    _fields_ = [("type", C.c_int), ("node", _Handle),
                ("frame_sequence", C.c_uint64)]


class _EventEx(C.Structure):
    _fields_ = _Event._fields_ + [("checked", C.c_uint32), ("value", C.c_double),
                                 ("text_length", C.c_uint32), ("selected", _Handle)]


class _TableNotificationV1(C.Structure):
    _fields_ = [(name, C.c_uint32) for name in
                ("reason", "row", "column", "sort_column", "sort_direction")]


class _DataTableLoadMoreV1(C.Structure):
    _fields_ = [("frame", C.c_uint64)] + [(name, C.c_uint32) for name in
                ("row_count", "visible_begin", "visible_end", "last_row_key", "capacity",
                 "reserved")]


class _NotificationValueV1(C.Union):
    _fields_ = [("boolean_value", C.c_uint32), ("scalar_value", C.c_double),
                ("selected", _Handle), ("table", _TableNotificationV1),
                ("surface_result", C.c_uint32)]


class _NotificationV1(C.Structure):
    _fields_ = [(name, C.c_uint32) for name in
                ("struct_size", "schema_version", "kind", "payload_kind", "flags", "node_kind")] + [
        ("node", _Handle), ("frame_sequence", C.c_uint64), ("text_length", C.c_uint32),
        ("payload", _NotificationValueV1), ("legacy", _EventEx)]


class _ChartDatum(C.Structure):
    _fields_ = [("label", C.c_char_p), ("value", C.c_double)]

class _PlotPoint(C.Structure):
    _fields_ = [("x", C.c_double), ("y", C.c_double)]

class _Time(C.Structure):
    _fields_ = [("hour", C.c_uint32), ("minute", C.c_uint32), ("second", C.c_uint32)]


class _Date(C.Structure):
    _fields_ = [("year", C.c_int32), ("month", C.c_int32), ("day", C.c_int32)]


class _Options(C.Structure):
    _fields_ = [("struct_size", C.c_uint32), ("abi_version", C.c_uint32),
                ("backend", C.c_int), ("max_nodes", C.c_uint32),
                ("max_events", C.c_uint32), ("width", C.c_float),
                ("height", C.c_float), ("title_utf8", C.c_char_p),
                ("font_path_utf8", C.c_char_p)]


class _WindowOptions(C.Structure):
    _fields_ = [("struct_size", C.c_uint32), ("version", C.c_uint32),
                ("initially_hidden", C.c_uint32)]


from .shadows import BoxShadow, ShadowStyle, ShadowKind, ShadowPreset, CornerRadii, CSSBoxShadow, PresetShadowStyle, ShadowPart, ShadowTargetInfo, canonical_part, target_value, _TargetInfo

class _API:
    def __init__(self, path: str | os.PathLike[str] | None):
        if path is None:
            path = os.environ.get("CUI_BRIDGE_LIBRARY")
        if not path:
            bundled = Path(__file__).resolve().parent / "_native" / "libcui_bridge.so"
            if bundled.is_file():
                path = bundled
            else:
                raise FileNotFoundError("CUI packaged bridge is missing; install a built wheel, "
                                        "pass library=, or set CUI_BRIDGE_LIBRARY")
        self.lib = C.CDLL(os.fspath(path))
        lib = self.lib
        def bind(name: str, restype, *argtypes):
            fn = getattr(lib, f"cui_bridge_{name}")
            fn.restype = restype
            fn.argtypes = list(argtypes)
            return fn
        self.create_window = self.set_window_visible = self.get_window_visible = None
        if all(hasattr(lib, "cui_bridge_" + name) for name in
               ("create_with_window_options", "set_window_visible", "get_window_visible")):
            self.create_window = bind("create_with_window_options", C.c_void_p,
                                      C.POINTER(_Options), C.POINTER(_WindowOptions), C.POINTER(C.c_int))
            self.set_window_visible = bind("set_window_visible", C.c_int, C.c_void_p, C.c_uint32)
            self.get_window_visible = bind("get_window_visible", C.c_int, C.c_void_p, C.POINTER(C.c_uint32))

        from .shadows import bind_shadow_api, bind_part_api
        bind_shadow_api(self, bind, _Handle)
        bind_part_api(self, bind, _Handle)
        from .slider_model import bind_api
        bind_api(self, bind)
        from .time_field_localized import bind_api as bind_time_api
        bind_time_api(self, bind)
        from .skeleton_presentation import bind_api as bind_skeleton_api
        bind_skeleton_api(self, bind)
        from .pagination_options import bind_api as bind_pagination_api
        bind_pagination_api(self, bind)
        from .rating_options import bind_api as bind_rating_api
        bind_rating_api(self, bind)
        from .disclosure import bind_api as bind_disclosure_api
        bind_disclosure_api(self, bind)
        bind_radio_paint(self, bind, _Handle)
        self.radio_set_motion = None
        if hasattr(lib, "cui_bridge_radio_set_motion_v1"):
            self.radio_set_motion = bind("radio_set_motion_v1", C.c_int, C.c_void_p,
                                         _Handle, C.c_uint32)
        self.radio_set_tab_index = None
        if hasattr(lib, "cui_bridge_radio_set_tab_index_v1"):
            self.radio_set_tab_index = bind("radio_set_tab_index_v1", C.c_int, C.c_void_p,
                                            _Handle, C.c_uint32, C.c_int32)
        self.radio_item_set_content = None
        if hasattr(lib, "cui_bridge_radio_item_set_content_v1"):
            self.radio_item_set_content = bind("radio_item_set_content_v1", C.c_int,
                                               C.c_void_p, _Handle, C.c_uint32)
        self.radio_set_tooltip = None
        if hasattr(lib, "cui_bridge_radio_set_tooltip_v1"):
            self.radio_set_tooltip = bind("radio_set_tooltip_v1", C.c_int, C.c_void_p,
                                          _Handle, C.c_char_p, C.c_uint32)
        self.clipboard_set = bind("clipboard_set",C.c_int,C.c_void_p,C.c_char_p)
        self.clipboard_get = bind("clipboard_get",C.c_int,C.c_void_p,C.c_void_p,C.c_uint32,C.POINTER(C.c_uint32))
        self.theme_count = bind("theme_preset_count", C.c_uint32)
        self.theme_name = bind("theme_preset_name", C.c_int, C.c_uint32, C.c_void_p, C.c_uint32, C.POINTER(C.c_uint32))
        self.set_theme = bind("set_theme_preset", C.c_int, C.c_void_p, C.c_uint32)
        self.get_theme = bind("get_theme_preset", C.c_int, C.c_void_p, C.POINTER(C.c_uint32))
        self.set_selection_mode = bind("data_table_set_selection_mode", C.c_int, C.c_void_p, _Handle, C.c_uint32)
        self.configure_table_paging = self.poll_table_paging = None
        names = ("data_table_load_more_size_v1", "data_table_configure_load_more_v1",
                 "data_table_poll_load_more_v1")
        if all(hasattr(lib, "cui_bridge_" + name) for name in names):
            size = bind(names[0], C.c_uint32)
            if size() == C.sizeof(_DataTableLoadMoreV1) == 32:
                self.configure_table_paging = bind(names[1], C.c_int, C.c_void_p, _Handle,
                                                   C.c_uint32, C.c_uint32)
                self.poll_table_paging = bind(names[2], C.c_int, C.c_void_p, _Handle,
                                              C.c_uint32, C.POINTER(_DataTableLoadMoreV1))
        self.command_bounds = bind("command_item_bounds", C.c_int, C.c_void_p, _Handle, _Handle, C.POINTER(_Rect))
        self.virtual_bounds = bind("virtual_list_item_bounds", C.c_int, C.c_void_p, _Handle, _Handle, C.POINTER(_Rect))
        self.virtual_rows = self.virtual_anchor = None
        if all(hasattr(lib, "cui_bridge_" + name) for name in
               ("virtual_list_set_rows_v1", "virtual_list_preserve_anchor_v1")):
            self.virtual_rows = bind("virtual_list_set_rows_v1", C.c_int, C.c_void_p, _Handle,
                                     C.c_uint32, C.POINTER(_VirtualListRow), C.c_uint32)
            self.virtual_anchor = bind("virtual_list_preserve_anchor_v1", C.c_int,
                                       C.c_void_p, _Handle, C.c_uint32)

        self.set_chart = bind("chart_set_data", C.c_int, C.c_void_p, _Handle, C.POINTER(_ChartDatum), C.c_uint32)
        self.set_chart_kind = bind("chart_set_kind", C.c_int, C.c_void_p, _Handle, C.c_uint32)
        self.get_chart = bind("chart_get_datum", C.c_int, C.c_void_p, _Handle, C.c_uint32, C.c_void_p, C.c_uint32, C.POINTER(C.c_uint32), C.POINTER(C.c_double))
        self.set_plot = bind("plot_set_points", C.c_int, C.c_void_p, _Handle, C.POINTER(_PlotPoint), C.c_uint32)
        self.get_plot = bind("plot_get_point", C.c_int, C.c_void_p, _Handle, C.c_uint32, C.POINTER(_PlotPoint))
        self.set_matrix = bind("data_table_set", C.c_int, C.c_void_p, _Handle, C.POINTER(C.c_char_p), C.c_uint32, C.POINTER(C.c_char_p), C.c_uint32)
        self.set_matrix_keyed = None
        if hasattr(lib, "cui_bridge_data_table_set_keyed"):
            self.set_matrix_keyed = bind(
                "data_table_set_keyed", C.c_int, C.c_void_p, _Handle,
                C.POINTER(C.c_char_p), C.c_uint32, C.POINTER(C.c_char_p),
                C.POINTER(C.c_uint32), C.c_uint32,
            )
        self.table_capacity_limit = None
        self.set_matrix_capacity = None
        names = ("data_table_max_row_capacity_v1", "data_table_set_keyed_capacity_v1")
        if all(hasattr(lib, "cui_bridge_" + name) for name in names):
            maximum = bind(names[0], C.c_uint32)()
            if maximum > 0:
                self.table_capacity_limit = min(maximum, _DATA_TABLE_MAX_ROWS)
                self.set_matrix_capacity = bind(
                    names[1], C.c_int, C.c_void_p, _Handle,
                    C.POINTER(C.c_char_p), C.c_uint32, C.POINTER(C.c_char_p),
                    C.POINTER(C.c_uint32), C.c_uint32, C.c_uint32,
                )
        self.get_matrix_cell = bind("data_table_get_cell", C.c_int, C.c_void_p, _Handle, C.c_uint32, C.c_uint32, C.c_void_p, C.c_uint32, C.POINTER(C.c_uint32))
        self.get_matrix_state = bind("data_table_get_state", C.c_int, C.c_void_p, _Handle, *([C.POINTER(C.c_uint32)]*4))
        self.matrix_cell_bounds = bind("data_table_cell_bounds", C.c_int, C.c_void_p, _Handle, C.c_uint32, C.c_uint32, C.POINTER(_Rect))
        self.matrix_header_bounds = bind("data_table_header_bounds", C.c_int, C.c_void_p, _Handle, C.c_uint32, C.POINTER(_Rect))
        self.set_root_padding = bind("set_root_padding",C.c_int,C.c_void_p,_Handle,C.c_float)
        self.settings_query = bind("settings_set_query",C.c_int,C.c_void_p,_Handle,C.c_char_p)
        self.settings_page = bind("settings_get_page",C.c_int,C.c_void_p,_Handle,C.POINTER(_Handle))
        self.settings_select = bind("settings_set_page",C.c_int,C.c_void_p,_Handle,_Handle)
        self.settings_reset = bind("settings_reset_page",C.c_int,C.c_void_p,_Handle,_Handle)
        self.settings_bounds = bind("settings_field_bounds",C.c_int,C.c_void_p,_Handle,_Handle,C.POINTER(_Rect))
        self.dock_activate = bind("dock_activate",C.c_int,C.c_void_p,_Handle,_Handle)
        self.dock_move = bind("dock_move_split",C.c_int,C.c_void_p,_Handle,_Handle,_Handle,C.c_uint32,C.c_double,C.c_uint32)
        self.dock_active = bind("dock_get_active",C.c_int,C.c_void_p,_Handle,C.POINTER(_Handle))
        self.dock_bounds = bind("dock_panel_bounds",C.c_int,C.c_void_p,_Handle,_Handle,C.POINTER(_Rect))
        self.dock_serialize = bind("dock_serialize",C.c_int,C.c_void_p,_Handle,C.c_void_p,C.c_uint32,C.POINTER(C.c_uint32))
        self.question_config = bind("questionnaire_set_question",C.c_int,C.c_void_p,_Handle,C.c_uint32,C.c_uint32)
        self.question_state = bind("questionnaire_get_state",C.c_int,C.c_void_p,_Handle,C.POINTER(_Handle),C.POINTER(C.c_uint32))
        self.question_choice = bind("questionnaire_choice_selected",C.c_int,C.c_void_p,_Handle,C.POINTER(C.c_uint32))
        self.question_submit = bind("questionnaire_submit",C.c_int,C.c_void_p,_Handle)
        self.question_next = bind("questionnaire_next",C.c_int,C.c_void_p,_Handle)
        self.question_previous = bind("questionnaire_previous",C.c_int,C.c_void_p,_Handle)
        self.speech_start = bind("speech_start",C.c_int,C.c_void_p,_Handle)
        self.speech_finish = bind("speech_finish",C.c_int,C.c_void_p,_Handle)
        self.speech_report = bind("speech_report",C.c_int,C.c_void_p,_Handle,C.c_char_p,C.c_uint32)
        self.speech_transcript = bind("speech_transcript",C.c_int,C.c_void_p,_Handle,C.c_void_p,C.c_uint32,C.POINTER(C.c_uint32))
        self.set_font_fallbacks = bind("set_font_fallbacks", C.c_int, C.c_void_p, C.POINTER(C.c_char_p), C.c_uint32)
        self.abi_version = bind("abi_version", C.c_uint32)
        self.options_size = bind("options_size", C.c_uint32)
        self.status_message = bind("status_message", C.c_char_p, C.c_int)
        self.create = bind("create", C.c_void_p, C.POINTER(_Options), C.POINTER(C.c_int))
        self.destroy = bind("destroy", None, C.c_void_p)
        self.node_create = bind("node_create", C.c_int, C.c_void_p, _Handle,
                                C.c_int, C.c_char_p, C.POINTER(_Handle))
        self.node_remove = bind("node_remove", C.c_int, C.c_void_p, _Handle)
        self.set_text = bind("set_text", C.c_int, C.c_void_p, _Handle, C.c_char_p)
        self.set_gap = bind("set_gap", C.c_int, C.c_void_p, _Handle, C.c_float)
        self.set_variant = bind("set_button_variant", C.c_int, C.c_void_p, _Handle, C.c_int)
        self.set_stepper_fill = None
        if hasattr(lib, "cui_bridge_set_stepper_fill"):
            self.set_stepper_fill = bind("set_stepper_fill", C.c_int, C.c_void_p, _Handle,
                                         C.c_uint32, C.c_uint32, C.c_float, C.c_float)
        self.set_stepper_visual = bind("set_stepper_visual", C.c_int, C.c_void_p, _Handle, C.c_uint32, C.c_uint32, C.c_uint32)
        self.set_design_size = bind("set_design_size", C.c_int, C.c_void_p, _Handle, C.c_int)
        self.set_avatar_identity = bind("set_avatar_identity", C.c_int, C.c_void_p, _Handle, C.c_uint32, C.c_uint32, C.c_uint32)
        self.set_avatar_placeholder = bind("set_avatar_placeholder", C.c_int, C.c_void_p, _Handle, C.c_uint32, C.c_char_p)
        self.set_size = bind("set_button_size", C.c_int, C.c_void_p, _Handle, C.c_int)
        self.set_rounding = bind("set_button_rounding", C.c_int, C.c_void_p,
                                 _Handle, C.c_int, C.c_float)
        self.set_disabled = bind("set_disabled", C.c_int, C.c_void_p, _Handle,
                                  C.c_uint32)
        self.set_checked = bind("set_checked", C.c_int, C.c_void_p, _Handle, C.c_uint32)
        self.set_value = bind("set_value", C.c_int, C.c_void_p, _Handle, C.c_double)
        self.set_range = bind("set_range", C.c_int, C.c_void_p, _Handle,
                              C.c_double, C.c_double, C.c_double)
        self.get_checked = bind("get_checked", C.c_int, C.c_void_p, _Handle, C.POINTER(C.c_uint32))
        self.get_value = bind("get_value", C.c_int, C.c_void_p, _Handle, C.POINTER(C.c_double))
        self.set_detail = bind("set_detail_text", C.c_int, C.c_void_p, _Handle, C.c_char_p)
        self.set_open = bind("set_open", C.c_int, C.c_void_p, _Handle, C.c_uint32)
        self.set_selected = bind("set_selected", C.c_int, C.c_void_p, _Handle, _Handle)
        self.get_selected = bind("get_selected", C.c_int, C.c_void_p, _Handle, C.POINTER(_Handle))
        self.get_open = bind("get_open", C.c_int, C.c_void_p, _Handle, C.POINTER(C.c_uint32))
        self.set_anchor = bind("set_anchor", C.c_int, C.c_void_p, _Handle, _Handle)
        self.set_visual = bind("set_visual_size", C.c_int, C.c_void_p, _Handle, C.c_float, C.c_float)
        self.set_total = bind("set_total", C.c_int, C.c_void_p, _Handle, C.c_uint32)
        self.set_step_status = bind("set_step_status", C.c_int, C.c_void_p, _Handle, C.c_uint32)
        self.set_scroll_extent = bind("set_scroll_extent", C.c_int, C.c_void_p, _Handle, C.c_float, C.c_float)
        self.queue_scroll = bind("queue_scroll", C.c_int, C.c_void_p, C.c_float, C.c_float, C.c_float, C.c_float)
        self.numeric_bounds = bind("numeric_part_bounds", C.c_int, C.c_void_p, _Handle,
                                  C.c_uint32, C.POINTER(_Rect))
        self.set_attachment = bind("set_attachment_status", C.c_int, C.c_void_p, _Handle, C.c_int)
        self.set_image = bind("set_image_rgba", C.c_int, C.c_void_p, _Handle, C.c_void_p,
                              C.c_uint32, C.c_uint32, C.c_uint32, C.c_uint32)
        self.set_time = bind("set_time", C.c_int, C.c_void_p, _Handle, _Time)
        self.get_time = bind("get_time", C.c_int, C.c_void_p, _Handle, C.POINTER(_Time))
        self.set_date = bind("set_date", C.c_int, C.c_void_p, _Handle, _Date)
        self.get_date = bind("get_date", C.c_int, C.c_void_p, _Handle, C.POINTER(_Date))
        self.set_color = bind("set_color_rgba", C.c_int, C.c_void_p, _Handle, C.c_uint32)
        self.get_color = bind("get_color_rgba", C.c_int, C.c_void_p, _Handle, C.POINTER(C.c_uint32))
        self.color_bounds = bind("color_part_bounds", C.c_int, C.c_void_p, _Handle,
                                C.c_uint32, C.POINTER(_Rect))
        self.day_bounds = bind("calendar_day_bounds", C.c_int, C.c_void_p, _Handle,
                              _Date, C.POINTER(_Rect))
        self.resizable_bounds = bind("resizable_handle_bounds", C.c_int, C.c_void_p, _Handle, _Handle, C.POINTER(_Rect))
        self.set_text_format = bind("set_text_view_format", C.c_int, C.c_void_p, _Handle, C.c_uint32)
        self.select_text = bind("text_view_select", C.c_int, C.c_void_p, _Handle, C.c_uint32, C.c_uint32)
        self.copy_selection = bind("text_view_copy_selection", C.c_int, C.c_void_p, _Handle, C.c_void_p, C.c_uint32, C.POINTER(C.c_uint32))
        self.set_hover = bind("set_button_hover_rgba", C.c_int, C.c_void_p, _Handle,
                              C.c_uint8, C.c_uint8, C.c_uint8, C.c_uint8)
        self.queue_pointer = bind("queue_pointer", C.c_int, C.c_void_p, C.c_int,
                                  C.c_float, C.c_float)
        self.queue_key = bind("queue_key", C.c_int, C.c_void_p, C.c_int, C.c_int,
                              C.c_uint32, C.c_uint32)
        self.step = bind("step", C.c_int, C.c_void_p, C.c_float)
        self.poll_event = bind("poll_event", C.c_int, C.c_void_p, C.POINTER(_Event))
        self.poll_event_ex = bind("poll_event_ex", C.c_int, C.c_void_p, C.POINTER(_EventEx),
                                   C.c_void_p, C.c_uint32, C.POINTER(C.c_uint32))
        self.get_text = bind("get_text", C.c_int, C.c_void_p, _Handle,
                             C.c_void_p, C.c_uint32, C.POINTER(C.c_uint32))
        self.queue_text = bind("queue_text", C.c_int, C.c_void_p, C.c_char_p)
        self.dropdown_bounds = bind("dropdown_part_bounds", C.c_int, C.c_void_p, _Handle,
                                    C.c_uint32, C.POINTER(_Rect))
        self.bounds = bind("node_bounds", C.c_int, C.c_void_p, _Handle, C.POINTER(_Rect))
        self.request_close = bind("request_close", C.c_int, C.c_void_p)
        if self.abi_version() != 1 or self.options_size() != C.sizeof(_Options):
            raise RuntimeError("Incompatible CUI bridge ABI")
        from .icons import bind_api as bind_icon_api
        bind_icon_api(self, bind)

    def check(self, status: int) -> None:
        if status != 0:
            message = self.status_message(status)
            raise BridgeError(status, message.decode("utf-8") if message else "unknown")


DesignSize = Literal["inherit", "xs", "sm", "base", "md", "lg", "xl", "2xl", "3xl", "4xl", "5xl", "6xl", "7xl", "8xl", "9xl"]
_SIZE = {name: i for i, name in enumerate(("inherit", "xs", "sm", "base", "md", "lg",
                                           "xl", "2xl", "3xl", "4xl", "5xl", "6xl",
                                           "7xl", "8xl", "9xl"))}
_KIND = {"row": 1, "column": 2, "label": 3, "button": 4, "checkbox": 5,
         "radio": 6, "switch": 7, "toggle": 8, "slider": 9, "progress": 10, "badge": 11, "input": 12, "textarea": 13, "number_input": 14, "collapsible": 15, "alert": 16, "radio_group": 17, "tabs": 18, "accordion": 19, "item": 20, "select": 21, "combobox": 22, "dropdown_button": 23, "dialog": 24, "alert_dialog": 25, "sheet": 26, "popover": 27, "tooltip": 28, "hover_card": 29, "avatar": 30, "bubble": 31, "empty_view": 32, "group_box": 33, "kbd": 34, "skeleton": 35, "spinner": 36, "tag": 37, "marker": 38, "shimmer": 39, "list_view": 40, "menu": 41, "pagination": 42, "rating": 43, "stepper": 44, "description_list": 45, "scrollable": 46, "status_bar": 47, "toolbar": 48, "title_bar": 49, "icon": 50, "message": 51, "attachment": 52, "image": 53, "tree": 54, "time_field": 55, "color_picker": 56, "calendar": 57, "date_picker": 58, "otp_input": 59, "form": 60, "input_group": 61, "notification": 62, "focus_trap": 63, "message_scroller": 64, "carousel": 65, "resizable": 66, "text_view": 67, "editor": 68, "chart": 69, "table": 70, "data_table": 71, "plot": 72, "command": 73, "virtual_list": 74, "sidebar": 75, "root_view": 76, "speech": 77, "questionnaire": 78, "dock": 79, "settings": 80}
_BOOLEAN = {"checkbox", "radio", "switch", "toggle"}
_NUMERIC = {"slider", "progress", "number_input", "pagination", "rating"}
_KIND["slider_model"] = 81
_KIND["configured_badge"] = 82
_RADIO_CONTENT_KINDS = {"row", "column", "label", "badge", "avatar", "kbd", "tag",
                        "icon", "image"}
_EDITABLE = {"input", "textarea", "number_input", "combobox"}
_GROUP = {"radio_group", "tabs", "accordion", "select", "combobox", "dropdown_button", "list_view", "menu", "stepper", "description_list", "status_bar", "toolbar", "title_bar"}
_SURFACE = {"dialog", "alert_dialog", "sheet", "popover"}
_HOVER = {"tooltip", "hover_card"}
_DETAIL = {"alert", "select", "combobox", "dropdown_button", "empty_view", "item", "message"} | _SURFACE
_VISUAL = {"avatar", "empty_view", "group_box", "skeleton", "spinner", "scrollable", "icon", "message"}
_DISABLED = {"button", "slider", "pagination", "rating", "collapsible", "item"} | _BOOLEAN | _EDITABLE | (_GROUP - {"description_list", "status_bar", "title_bar"}) | _HOVER
_NUMERIC |= {"attachment"}
_EDITABLE |= {"command", "otp_input", "input_group", "editor"}
_GROUP |= {"settings", "dock", "questionnaire", "sidebar", "command", "virtual_list", "table", "tree", "form", "input_group", "message_scroller", "carousel", "resizable"}
_DETAIL |= {"input", "number_input", "attachment", "input_group", "notification", "editor"}
_VISUAL |= {"settings", "dock", "questionnaire", "sidebar", "command", "virtual_list", "chart", "table", "data_table", "plot", "attachment", "image", "tree", "color_picker", "date_picker", "form", "input_group", "message_scroller", "carousel", "resizable", "text_view", "editor"}
_DISABLED |= {"dock", "questionnaire", "speech", "sidebar", "command", "virtual_list", "chart", "data_table", "tree", "time_field", "color_picker", "calendar", "date_picker", "otp_input", "toolbar", "form", "input_group", "carousel", "resizable", "text_view", "editor"}
_SELECTED = {"sidebar","radio_group", "tabs", "select", "combobox", "dropdown_button", "list_view", "menu", "stepper", "tree", "carousel"}
_SURFACE |= {"notification"}
_CHANGE = (_DISABLED - {"button", "accordion", "form"}) | {"message_scroller", "settings"}
_DISABLED.add("slider_model")  # No legacy CHANGE callback for the controlled route.
_DISABLED.add("configured_badge")
_CONSTRUCTOR_CONTAINER = {"root_view","row", "column", "collapsible", "item", "dialog", "sheet", "popover", "hover_card", "group_box", "scrollable", "focus_trap"}
_CONTAINER = _CONSTRUCTOR_CONTAINER | _GROUP | {"configured_badge"}
_VARIANT = {"primary": 0, "secondary": 1, "outline": 2, "ghost": 3}
_ROUND = {"theme": 0, "none": 1, "small": 2, "large": 3, "custom": 4}


@dataclass(frozen=True)
class Event:
    type: str
    node: "Widget"
    frame_sequence: int
    checked: bool = False
    value: float = 0.0
    text: str = ""
    selected_handle: tuple[int, int] = (0, 0)


@dataclass(frozen=True)
class LegacySnapshotV1:
    type: int
    checked_raw: int
    value: float
    text: str
    selected_handle: tuple[int, int]


@dataclass(frozen=True)
class NotificationV1:
    handle: tuple[int, int]
    node: "Widget | None"
    frame_sequence: int
    has_frame: bool
    node_kind: int
    legacy: LegacySnapshotV1
    kind: ClassVar[str] = "legacy"


@dataclass(frozen=True)
class ActivationNotificationV1(NotificationV1):
    kind: ClassVar[Literal["activation"]] = "activation"


@dataclass(frozen=True)
class BooleanChangeNotificationV1(NotificationV1):
    value: bool
    kind: ClassVar[Literal["boolean-change"]] = "boolean-change"


@dataclass(frozen=True)
class TableChangeNotificationV1(NotificationV1):
    reason: Literal["selection", "sort"]
    row: int
    column: int
    sort_column: int
    sort_direction: int
    kind: ClassVar[Literal["table-change"]] = "table-change"


@dataclass(frozen=True)
class DataTableLoadMore:
    """Copied viewport demand from a successful frame, with an exclusive end index."""

    frame_sequence: int
    row_count: int
    visible_begin: int
    visible_end: int
    last_row_key: int
    capacity: int


@dataclass(frozen=True)
class SurfaceCloseNotificationV1(NotificationV1):
    result: int
    kind: ClassVar[Literal["surface-close"]] = "surface-close"


@dataclass(frozen=True)
class LegacyNotificationV1(NotificationV1):
    selected_node: "Widget | None"
    kind: ClassVar[Literal["legacy"]] = "legacy"


@dataclass(frozen=True)
class ScalarChangeNotificationV1(NotificationV1):
    value: float
    kind: ClassVar[Literal["scalar-change"]] = "scalar-change"


@dataclass(frozen=True)
class TextChangeNotificationV1(NotificationV1):
    text: str
    kind: ClassVar[Literal["text-change"]] = "text-change"


@dataclass(frozen=True)
class SelectionChangeNotificationV1(NotificationV1):
    selected_handle: tuple[int, int]
    selected_node: "Widget | None"
    kind: ClassVar[Literal["selection-change"]] = "selection-change"


ValueNotificationV1 = (NotificationV1 | ScalarChangeNotificationV1 |
                       TextChangeNotificationV1 | SelectionChangeNotificationV1)


class Widget:
    def __init__(self, kind: str, key: str | None = None, text: str = ""):
        if kind not in _KIND:
            raise ValueError(kind)
        self.kind = kind
        self.key = key
        self._text = text
        self._children: list[Widget] = []
        self._gap = 0.0
        self._root_padding = 0.0
        self._question_flags = None
        self._variant = "primary"
        self._size = "base"
        self._design_size = None
        self._disclosure_config: DisclosureConfig | None = None
        self._disclosure_style: DisclosureTriggerStyle | None = None
        self._stepper_visual = None
        self._stepper_fill = None
        self._avatar_identity = None
        self._avatar_placeholder = None
        self._part_shadow_styles = {}
        self._shadow_style = None
        self._rounding = "theme"
        self._radius = 0.0
        self._disabled = False
        self._checked = False
        self._radio_motion = False
        self._radio_tab_index: int | None = None
        self._radio_paints: dict[RadioPaintLayer, RadioPaint] | None = None
        self._radio_content = False
        self._radio_tooltip: tuple[bytes, int] | None = None
        self._detail = ""
        self._open = False
        self._open_set = False
        self._checked_set = False
        self._selected: Widget | None = None
        self._anchor: Widget | None = None
        self._extent: tuple[float, float] | None = None
        self._visual: tuple[float, float] | None = None
        self._total = 5 if kind == "rating" else 6 if kind == "otp_input" else 10
        self._chart_data = ()
        self._chart_kind = 0
        self._plot_points = ()
        self._matrix = None
        self._load_more_threshold: int | None = None
        self._selection_mode = 0
        self._text_format = 0
        self._panel_size = None
        self._attachment = 0
        self._image = None
        self._time = (0, 0, 0)
        self._date = (2026, 1, 1)
        self._color = 0x000000FF
        self._step_status: int | None = None
        self._value = 1.0 if kind == "pagination" else 0.0
        self._range = ((1.0,10.0,1.0) if kind == "pagination" else
                       (0.0,5.0,1.0) if kind == "rating" else (0.0,1.0,0.01))
        self._hover: tuple[int, int, int, int] | None = None
        self._close_callback: Callable[[Event], object] | None = None
        self._change_callback: Callable[[Event], object] | None = None
        self._callback: Callable[[Event], object] | None = None
        self._app: App | None = None
        self._handle: _Handle | None = None
        self._disposed = False

    def _change(self, name: str, *args) -> None:
        if self._disposed:
            raise RuntimeError("Widget belongs to a disposed App")
        if self._app is None:
            return
        app = self._app
        app._ensure_open()
        assert self._handle is not None
        fn = getattr(app._api, name)
        app._api.check(fn(app._ptr, self._handle, *args))

    def children(self, *children: "Widget") -> "Widget":
        if self.kind not in _CONTAINER:
            raise ValueError("component does not accept children")
        if self._app is not None:
            raise RuntimeError("Mount children before attaching the tree")
        if any(not isinstance(child, Widget) for child in children):
            raise TypeError("children must be widgets")
        if self.kind in _GROUP and any(child.kind != "item" for child in children):
            raise ValueError("groups require item children")
        self._children.extend(children)
        return self

    def box_shadows(self, layers, radii=CornerRadii()) -> "Widget":
        if self.kind not in {"row","column","label","button","input","badge"}:
            raise ValueError("box shadows support row/column/label/button/input/badge roots only")
        style = layers if isinstance(layers,(ShadowStyle,PresetShadowStyle)) else ShadowStyle(layers,radii)
        if self._disposed:
            raise RuntimeError("Widget belongs to a disposed App")
        if self._app is not None:
            self._app._ensure_open()
            wire = style._wire(self._app._api)
            self._change("set_box_shadows_v1",224,C.byref(wire))
        self._part_shadow_styles.pop(ShadowPart.ROOT,None)
        self._shadow_style = style
        return self

    def shadow_preset(self, preset, radii=CornerRadii()) -> "Widget":
        return self.box_shadows(ShadowStyle.preset(preset,radii))

    def reset_box_shadows(self) -> "Widget":
        if self.kind not in {"row","column","label","button","input","badge"}:
            raise ValueError("unsupported shadow root")
        self._change("reset_box_shadows")
        self._part_shadow_styles.pop(ShadowPart.ROOT,None)
        self._shadow_style = None
        return self

    def part_shadows(self, part, layers, radii=CornerRadii()) -> "Widget":
        role=canonical_part(self.kind,part)
        style=layers if isinstance(layers,(ShadowStyle,PresetShadowStyle)) else ShadowStyle(layers,radii)
        if self._disposed:raise RuntimeError("Widget belongs to a disposed App")
        if self._app is not None:
            self._app._ensure_open()
            target=target_value(part);wire=style._wire(self._app._api)
            self._change("set_target_box_shadows_v1",16,C.byref(target),224,C.byref(wire))
        self._part_shadow_styles[role]=style
        if role==ShadowPart.ROOT:self._shadow_style=None
        return self

    def part_shadow_preset(self,part,preset,radii=CornerRadii()) -> "Widget":
        return self.part_shadows(part,ShadowStyle.preset(preset,radii))

    def reset_part_shadows(self,part) -> "Widget":
        role=canonical_part(self.kind,part);target=target_value(part)
        self._change("reset_target_box_shadows_v1",16,C.byref(target))
        self._part_shadow_styles.pop(role,None)
        if role==ShadowPart.ROOT:self._shadow_style=None
        return self

    def shadow_target(self,part) -> ShadowTargetInfo:
        target=target_value(part)
        if self._app is None:raise RuntimeError("Capability query requires a mounted Widget")
        out=_TargetInfo()
        self._change("shadow_target_query_v1",16,C.byref(target),16,C.byref(out))
        return ShadowTargetInfo(None if out.canonical_role==0xffffffff else ShadowPart(out.canonical_role),out.flags)

    def gap(self, px: float) -> "Widget":
        if self.kind not in ("row", "column"):
            raise ValueError("gap belongs to a row or column")
        if not math.isfinite(px) or px < 0 or px > 4096:
            raise ValueError("gap must be 0..4096 logical pixels")
        self._change("set_gap", float(px))
        self._gap = float(px)
        return self

    def gap_px(self, px: float) -> "Widget":
        return self.gap(px)

    def gap_units(self, units: float) -> "Widget":
        return self.gap(units * 4)

    def label(self, text: str) -> "Widget":
        if _KIND[self.kind] < 3:
            raise ValueError("label belongs to text or button")
        if not isinstance(text, str):
            raise TypeError("label must be a string")
        self._change("set_text", _utf8(text))
        self._text = text
        return self

    def variant(self, value: str) -> "Widget":
        if self.kind != "button" or value not in _VARIANT:
            raise ValueError("unsupported button variant")
        self._change("set_variant", _VARIANT[value])
        self._variant = value
        return self

    def primary(self) -> "Widget":
        return self.variant("primary")

    def secondary(self) -> "Widget":
        return self.variant("secondary")

    def outline(self) -> "Widget":
        return self.variant("outline")

    def ghost(self) -> "Widget":
        return self.variant("ghost")

    def size(self, value: str) -> "Widget":
        if self.kind != "button" or value not in _SIZE:
            raise ValueError("unsupported design size")
        self._change("set_size", _SIZE[value])
        self._size = value
        return self

    def design_size(self, value: DesignSize) -> "Widget":
        """Shared scale: currently Button, Avatar and Stepper."""
        if self.kind == "button":
            return self.size(value)
        if self.kind not in {"avatar", "stepper"} or value not in _SIZE:
            raise ValueError("design_size requires Button, Avatar or Stepper and a supported size")
        self._change("set_design_size", _SIZE[value])
        self._design_size = value
        return self

    def identity_colors(self, enabled: bool = True, hue: int | None = None) -> "Widget":
        if self.kind != "avatar" or type(enabled) is not bool:
            raise ValueError("identity_colors requires Avatar and a bool")
        if hue is not None and (type(hue) is not int or not 0 <= hue <= 11):
            raise ValueError("identity hue must be an integer 0..11 or None")
        self._change("set_avatar_identity", int(enabled), int(hue is not None), hue or 0)
        self._avatar_identity = (enabled, hue)
        return self

    def anonymous_icon(self, enabled: bool = True, name: str = "") -> "Widget":
        if self.kind != "avatar" or type(enabled) is not bool:
            raise ValueError("anonymous_icon requires Avatar and a bool")
        encoded = _utf8(name)
        self._change("set_avatar_placeholder", int(enabled), encoded)
        self._avatar_placeholder = (enabled, name)
        return self

    def rounded_md(self) -> "Widget":
        return self.rounding("theme")

    def rounding(self, value: str, radius: float = 0) -> "Widget":
        if self.kind != "button" or value not in _ROUND:
            raise ValueError("unsupported rounding")
        if value == "custom" and (not math.isfinite(radius) or radius < 0 or radius > 4096):
            raise ValueError("custom radius must be 0..4096 logical pixels")
        self._change("set_rounding", _ROUND[value], float(radius))
        self._rounding, self._radius = value, radius
        return self

    def disabled(self, value: bool = True) -> "Widget":
        if self.kind not in _DISABLED:
            raise ValueError("component does not support disabled")
        if type(value) is not bool:
            raise TypeError("disabled must be a bool")
        self._change("set_disabled", int(bool(value)))
        self._disabled = bool(value)
        return self

    def checked(self, value: bool = True) -> "Widget":
        if self.kind not in _BOOLEAN | {"item"} or type(value) is not bool:
            raise ValueError("checked requires a boolean component and bool value")
        self._change("set_checked", int(value))
        self._checked = value
        self._checked_set = True
        return self

    def animate_selection(self, enabled: bool = True) -> "Widget":
        """Use the theme spring for Radio or RadioGroup selection marks."""
        if self.kind not in {"radio", "radio_group"}:
            raise ValueError("animate_selection requires Radio or RadioGroup")
        if type(enabled) is not bool:
            raise TypeError("enabled must be a bool")
        if self._disposed:
            raise RuntimeError("Widget belongs to a disposed App")
        if self._app is not None:
            if enabled:
                self._app._require_radio_motion()
            if self._app.supports_radio_motion():
                self._change("radio_set_motion", int(enabled))
        self._radio_motion = enabled
        return self

    def tab_index(self, index: int | None) -> "Widget":
        """Set signed Tab order for Radio or RadioGroup. None restores default order."""
        if self.kind not in {"radio", "radio_group"}:
            raise ValueError("tab_index requires Radio or RadioGroup")
        if self._disposed:
            raise RuntimeError("Widget belongs to a disposed App")
        if index is not None:
            if type(index) is not int:
                raise TypeError("tab index must be an integer or None")
            if not -(1 << 31) <= index < (1 << 31):
                raise ValueError("tab index must fit a signed 32-bit integer")
        if self._app is not None:
            if index is not None:
                self._app._require_radio_tab_index()
            if self._app.supports_radio_tab_index():
                self._change("radio_set_tab_index", int(index is not None), index or 0)
        self._radio_tab_index = index
        return self

    def radio_paint(self, layer: RadioPaintLayer, paint: RadioPaint | None) -> "Widget":
        """Replace one retained Radio paint layer. None clears it."""
        if self._disposed:
            raise RuntimeError("Widget belongs to a disposed App")
        wire = radio_paint_wire(self.kind, layer, paint)
        if self._app is not None:
            if wire is not None:
                self._app._require_radio_paint()
            if self._app.supports_radio_paint():
                self._change("radio_set_paint", int(layer), None if wire is None else C.byref(wire))
        if wire is None:
            if self._radio_paints is not None:
                self._radio_paints.pop(layer, None)
        else:
            assert paint is not None
            if self._radio_paints is None:
                self._radio_paints = {}
            self._radio_paints[layer] = paint
        return self

    def decorate_content(self, enabled: bool = True) -> "Widget":
        """Show or hide a RadioGroup Item's passive children."""
        if self.kind != "item":
            raise ValueError("decorate_content requires a RadioGroup Item")
        if type(enabled) is not bool:
            raise TypeError("enabled must be a bool")
        if self._disposed:
            raise RuntimeError("Widget belongs to a disposed App")
        if self._app is not None:
            if enabled:
                self._app._require_radio_item_content()
            if self._app.supports_radio_item_content():
                self._change("radio_item_set_content", int(enabled))
        self._radio_content = enabled
        return self

    def radio_tooltip(self, text: str | None, *, delay_ms: int = 350) -> "Widget":
        """Set a short hint on a Radio or direct RadioGroup Item. None or empty clears it."""
        if self.kind not in {"radio", "item"}:
            raise ValueError("radio_tooltip requires Radio or a direct RadioGroup Item")
        if self._disposed:
            raise RuntimeError("Widget belongs to a disposed App")
        encoded = None if text is None else _utf8(text, limit=256)
        if encoded:
            if type(delay_ms) is not int or not 0 <= delay_ms <= 10000:
                raise ValueError("delay_ms must be an integer from 0 to 10000")
            config = (encoded, delay_ms)
        else:
            config = None
        if self._app is not None:
            if config is not None:
                self._app._require_radio_tooltip()
            if self._app.supports_radio_tooltip():
                self._change("radio_set_tooltip", *(config or (None, 0)))
        self._radio_tooltip = config
        return self

    def is_checked(self) -> bool:
        if self.kind not in _BOOLEAN | {"item"}:
            raise ValueError("component has no checked state")
        if self._app is None:
            if self._disposed:
                raise RuntimeError("Widget is disposed")
            return self._checked
        result = C.c_uint32()
        self._change("get_checked", C.byref(result))
        return bool(result.value)

    def value(self, value: float) -> "Widget":
        if self.kind not in _NUMERIC or not math.isfinite(value):
            raise ValueError("value requires a numeric component and finite number")
        minimum, maximum, _ = self._range
        if not minimum <= value <= maximum:
            raise ValueError("value is outside the range")
        self._change("set_value", float(value))
        self._value = float(value)
        return self

    def current_value(self) -> float:
        if self.kind not in _NUMERIC:
            raise ValueError("component has no numeric value")
        if self._app is None:
            if self._disposed:
                raise RuntimeError("Widget is disposed")
            return self._value
        result = C.c_double()
        self._change("get_value", C.byref(result))
        return result.value

    def range(self, minimum: float, maximum: float, step: float = 1) -> "Widget":
        if self.kind not in {"slider", "number_input"} or not all(math.isfinite(x) for x in (minimum, maximum, step)):
            raise ValueError("range requires a slider and finite numbers")
        if minimum >= maximum or step <= 0:
            raise ValueError("range requires minimum < maximum and step > 0")
        self._change("set_range", minimum, maximum, step)
        self._range = (minimum, maximum, step)
        self._value = min(maximum, max(minimum, self._value))
        return self

    def current_text(self) -> str:
        if self.kind not in _EDITABLE | {"text_view"}:
            raise ValueError("component has no editable text")
        if self._app is None:
            if self._disposed:
                raise RuntimeError("Widget is disposed")
            return self._text
        buffer = C.create_string_buffer(4097)
        required = C.c_uint32()
        self._change("get_text", buffer, 4097, C.byref(required))
        return buffer.value.decode("utf-8")

    def on_change(self, callback: Callable[[Event], object]) -> "Widget":
        if self.kind not in _CHANGE | {"scrollable", "toolbar"} or not callable(callback):
            raise ValueError("on_change requires an interactive component and callable")
        self._change_callback = callback
        return self

    def detail(self, text: str) -> "Widget":
        if self.kind not in _DETAIL:
            raise ValueError("detail belongs to alert")
        self._change("set_detail", _utf8(text))
        self._detail = text
        return self

    def set_disclosure_title_state(
        self, state: DisclosureTitleState, style: DisclosureTitleStyle | None
    ) -> "Widget":
        """Copy interaction title styling. None restores inheritance."""
        from .disclosure import title_state
        title_state(self, state, style)
        return self

    def set_disclosure_heading(
        self, layer: DisclosureHeadingLayer, style: DisclosureHeading | None
    ) -> "Widget":
        """Copy a heading layer. Pass None to restore inheritance."""
        from .disclosure import heading
        heading(self, layer, style)
        return self

    def set_disclosure_divider(self, style: DisclosureDivider | None) -> "Widget":
        """Copy an Accordion divider. Pass None to restore the themed divider."""
        from .disclosure import divider
        divider(self, style)
        return self

    def disclosure_configure(self, config: DisclosureConfig | None) -> "Widget":
        """Copy presentation to an Accordion or Collapsible. None clears it."""
        from .disclosure import configure
        configure(self, config)
        return self

    def set_trigger_style(self, style: DisclosureTriggerStyle | None) -> "Widget":
        """Copy feedback to a Collapsible or a direct Accordion Item. None clears it."""
        from .disclosure import trigger_style
        trigger_style(self, style)
        return self

    def open(self, value: bool = True) -> "Widget":
        if self.kind not in {"collapsible", "item", "date_picker", "focus_trap", "sidebar"} | _SURFACE or type(value) is not bool:
            raise ValueError("open requires a disclosure component and bool")
        self._change("set_open", int(value))
        self._open = value
        self._open_set = True
        return self

    def select(self, item: "Widget") -> "Widget":
        if self.kind not in _SELECTED or item not in self._children:
            raise ValueError("selection must be a direct group item")
        if self._app is not None:
            self._change("set_selected", item._handle)
        self._selected = item
        return self

    def selected(self) -> "Widget | None":
        if self.kind not in _SELECTED | {"command"}:
            raise ValueError("component has no selection")
        if self._app is None:
            if self._disposed:
                raise RuntimeError("Widget is disposed")
            return self._selected
        handle = _Handle()
        self._change("get_selected", C.byref(handle))
        return self._app._by_handle.get((handle.slot, handle.generation))

    def anchor(self, node: "Widget") -> "Widget":
        if self.kind not in {"popover"} | _HOVER or not isinstance(node, Widget):
            raise ValueError("anchor requires a popover/hover surface and widget")
        if self._app is not None:
            if node._app is not self._app:
                raise ValueError("anchor belongs to another App")
            self._change("set_anchor", node._handle)
        self._anchor = node
        return self

    def is_open(self) -> bool:
        if self.kind not in _SURFACE | _HOVER | {"collapsible", "item", "date_picker", "focus_trap", "sidebar"}:
            raise ValueError("component has no open state")
        if self._app is None:
            if self._disposed:
                raise RuntimeError("Widget is disposed")
            return self._open
        result = C.c_uint32()
        self._change("get_open", C.byref(result))
        return bool(result.value)

    def on_close(self, callback: Callable[[Event], object]) -> "Widget":
        if self.kind not in _SURFACE or not callable(callback):
            raise ValueError("on_close requires a surface and callback")
        self._close_callback = callback
        return self

    def visual_size(self, width: float, height: float) -> "Widget":
        if self.kind not in _VISUAL or not all(math.isfinite(x) and 0 <= x <= 4096 for x in (width,height)):
            raise ValueError("invalid visual size")
        self._change("set_visual", width,height)
        self._visual = (width,height)
        return self

    def total(self, total: int) -> "Widget":
        if self.kind not in {"pagination", "rating", "otp_input"} or type(total) is not int or not 1 <= total <= (10 if self.kind == "rating" else 256 if self.kind == "otp_input" else 1000000):
            raise ValueError("invalid total")
        self._change("set_total", total)
        self._total = total
        if self.kind == "otp_input":
            return self
        self._range = (1.0 if self.kind == "pagination" else 0.0,float(total),1.0)
        self._value = min(total,self._value)
        return self

    def step_status(self, status: int) -> "Widget":
        if self.kind != "item" or type(status) is not int or not 0 <= status <= 63:
            raise ValueError("step status/depth requires an item and 0..63 (parent enforces its limit)")
        self._change("set_step_status", status)
        self._step_status = status
        return self

    def scroll_extent(self, width: float, height: float) -> "Widget":
        if self.kind != "scrollable" or not all(math.isfinite(x) and 0 <= x <= 32768 for x in (width,height)):
            raise ValueError("invalid scroll extent")
        self._change("set_scroll_extent",width,height)
        self._extent = (width,height)
        return self

    def scroll_offset(self) -> float:
        if self.kind not in {"scrollable", "message_scroller", "virtual_list"}:
            raise ValueError("component has no scroll offset")
        if self._app is None:
            if self._disposed:
                raise RuntimeError("Widget is disposed")
            return 0.0
        result = C.c_double()
        self._change("get_value",C.byref(result))
        return result.value

    def attachment_status(self, status: str) -> "Widget":
        names = {"pending": 0, "uploading": 1, "processing": 2, "failed": 3, "complete": 4}
        if self.kind != "attachment" or status not in names:
            raise ValueError("invalid attachment status")
        self._change("set_attachment", names[status])
        self._attachment = names[status]
        return self

    def stepper_visual(self, enabled: bool = True, vertical: bool = False, density: str = "md") -> "Widget":
        densities = {"md": 0, "xs": 1, "sm": 2, "lg": 3}
        if self.kind != "stepper" or type(enabled) is not bool or type(vertical) is not bool or density not in densities:
            raise ValueError("stepper_visual requires Stepper, bools and md/xs/sm/lg density")
        value = (int(enabled), int(vertical), densities[density])
        self._change("set_stepper_visual", *value)
        self._stepper_visual = value
        self._stepper_fill = None
        return self

    def stepper_fill(self, vertical: bool = False, density: str = "md",
                     width: float = 0, height: float = 0) -> "Widget":
        """Use Fill steps; zero width grows and zero height uses intrinsic height."""
        densities = {"md": 0, "xs": 1, "sm": 2, "lg": 3}
        if (self.kind != "stepper" or type(vertical) is not bool or
                not isinstance(density, str) or density not in densities or
                any(type(x) not in (int, float) or not 0 <= x <= 4096
                    for x in (width, height))):
            raise ValueError("stepper_fill requires Stepper, bool, md/xs/sm/lg and finite dimensions in [0,4096]")
        if self._app is not None:
            self._app._require_stepper_fill()
        value = (int(vertical), densities[density], width, height)
        self._change("set_stepper_fill", *value)
        self._stepper_fill = value
        self._stepper_visual = None
        return self

    def image_rgba(self, pixels: bytes | bytearray | memoryview, width: int,
                   height: int, stride: int | None = None) -> "Widget":
        if self.kind not in {"image", "item"} or any(type(x) is not int or not 1 <= x <= 2048 for x in (width,height)):
            raise ValueError("invalid image dimensions")
        stride = width * 4 if stride is None else stride
        if type(stride) is not int or stride < width * 4 or stride * height > 16777216:
            raise ValueError("invalid image stride")
        if not isinstance(pixels, (bytes, bytearray, memoryview)):
            raise TypeError("pixels must be a byte buffer")
        if memoryview(pixels).nbytes != stride * height:
            raise ValueError("pixel byte count must equal stride * height")
        data = bytes(pixels)
        self._change("set_image", data,width,height,stride,len(data))
        self._image = (data,width,height,stride,len(data))
        return self

    def time_value(self, hour: int, minute: int, second: int = 0) -> "Widget":
        if self.kind != "time_field" or any(type(x) is not int for x in (hour,minute,second)):
            raise ValueError("time requires integer fields and time_field")
        datetime.time(hour,minute,second)
        self._change("set_time", _Time(hour,minute,second))
        self._time = (hour,minute,second)
        return self

    def current_time(self) -> tuple[int,int,int]:
        if self.kind != "time_field":
            raise ValueError("component has no time")
        if self._app is None:
            if self._disposed: raise RuntimeError("Widget is disposed")
            return self._time
        result = _Time()
        self._change("get_time", C.byref(result))
        return result.hour,result.minute,result.second

    def date_value(self, year: int, month: int, day: int) -> "Widget":
        if self.kind not in {"calendar", "date_picker"} or any(type(x) is not int for x in (year,month,day)):
            raise ValueError("date requires integer fields and calendar/date_picker")
        datetime.date(year,month,day)
        self._change("set_date", _Date(year,month,day))
        self._date = (year,month,day)
        return self

    def current_date(self) -> tuple[int,int,int]:
        if self.kind not in {"calendar", "date_picker"}:
            raise ValueError("component has no date")
        if self._app is None:
            if self._disposed: raise RuntimeError("Widget is disposed")
            return self._date
        result = _Date()
        self._change("get_date", C.byref(result))
        return result.year,result.month,result.day

    def color_rgba(self, value: int) -> "Widget":
        if self.kind != "color_picker" or type(value) is not int or not 0 <= value <= 0xffffffff:
            raise ValueError("packed RGBA must be an unsigned 32-bit integer")
        self._change("set_color", value)
        self._color = value
        return self

    def current_color(self) -> int:
        if self.kind != "color_picker": raise ValueError("component has no color")
        if self._app is None:
            if self._disposed: raise RuntimeError("Widget is disposed")
            return self._color
        result = C.c_uint32()
        self._change("get_color",C.byref(result))
        return result.value

    def _settings_child(self, child: "Widget") -> None:
        if self.kind != "settings" or self._app is None or not isinstance(child, Widget) or child._app is not self._app:
            raise ValueError("mounted same-app Settings child required")

    def settings_query(self, query: str) -> "Widget":
        if self.kind != "settings" or self._app is None: raise ValueError("mounted Settings required")
        encoded = _utf8(query)
        if len(encoded) > 256: raise ValueError("query exceeds 256 UTF-8 bytes")
        self._change("settings_query", encoded); return self

    def settings_page(self) -> "Widget | None":
        if self.kind != "settings" or self._app is None: raise ValueError("mounted Settings required")
        page = _Handle(); self._change("settings_page", C.byref(page))
        return self._app._by_handle.get((page.slot, page.generation))

    def settings_select(self, page: "Widget") -> "Widget":
        self._settings_child(page); self._change("settings_select", page._handle); return self

    def settings_reset(self, page: "Widget") -> "Widget":
        self._settings_child(page); self._change("settings_reset", page._handle); return self

    def settings_field_bounds(self, field: "Widget") -> tuple:
        self._settings_child(field); rect = _Rect()
        self._change("settings_bounds", field._handle, C.byref(rect))
        return rect.x, rect.y, rect.width, rect.height

    def _dock_panel(self, panel: "Widget") -> None:
        if self.kind != "dock" or self._app is None or not isinstance(panel,Widget) or panel._app is not self._app or panel not in self._children: raise ValueError("mounted same-app Dock panel required")

    def dock_activate(self, panel: "Widget") -> "Widget":
        self._dock_panel(panel);self._change("dock_activate",panel._handle);return self

    def dock_move_split(self, panel: "Widget", reference: "Widget", axis: str = "horizontal", ratio: float = .5, before: bool = False) -> "Widget":
        self._dock_panel(panel);self._dock_panel(reference);axes={"horizontal":0,"vertical":1}
        if axis not in axes or isinstance(ratio,bool) or not isinstance(ratio,(int,float)) or not math.isfinite(ratio) or not 0<ratio<1 or type(before) is not bool: raise ValueError("invalid dock split")
        self._change("dock_move",panel._handle,reference._handle,axes[axis],ratio,int(before));return self

    def dock_active(self) -> "Widget | None":
        if self.kind != "dock" or self._app is None: raise ValueError("mounted Dock required")
        active=_Handle();self._change("dock_active",C.byref(active));return self._app._by_handle.get((active.slot,active.generation))

    def dock_panel_bounds(self, panel: "Widget") -> tuple:
        self._dock_panel(panel);rect=_Rect();self._change("dock_bounds",panel._handle,C.byref(rect));return rect.x,rect.y,rect.width,rect.height

    def dock_serialize(self) -> str:
        if self.kind != "dock" or self._app is None: raise ValueError("mounted Dock required")
        needed=C.c_uint32()
        try:self._change("dock_serialize",None,0,C.byref(needed))
        except BridgeError as error:
            if error.code!=5: raise
        if not 1<=needed.value<=1048576: raise ValueError("invalid dock serialization size")
        text=C.create_string_buffer(needed.value);self._change("dock_serialize",text,len(text),C.byref(needed));return text.value.decode()

    def question(self, required: bool = False, multiple: bool = False) -> "Widget":
        if self.kind != "item" or type(required) is not bool or type(multiple) is not bool: raise ValueError("question requires Item and bool flags")
        self._change("question_config",int(required),int(multiple));self._question_flags=(required,multiple);return self

    def questionnaire_state(self) -> dict:
        if self.kind != "questionnaire" or self._app is None: raise ValueError("mounted Questionnaire required")
        current=_Handle();complete=C.c_uint32();self._change("question_state",C.byref(current),C.byref(complete));return {"current":self._app._by_handle.get((current.slot,current.generation)),"complete":bool(complete.value)}

    def choice_selected(self) -> bool:
        if self.kind != "item" or self._app is None: raise ValueError("mounted Questionnaire choice Item required")
        selected=C.c_uint32();self._change("question_choice",C.byref(selected));return bool(selected.value)

    def questionnaire_next(self) -> "Widget":
        if self.kind != "questionnaire" or self._app is None: raise ValueError("mounted Questionnaire required")
        self._change("question_next");return self

    def questionnaire_submit(self) -> "Widget":
        if self.kind != "questionnaire" or self._app is None: raise ValueError("mounted Questionnaire required")
        self._change("question_submit");return self

    def questionnaire_previous(self) -> "Widget":
        if self.kind != "questionnaire" or self._app is None: raise ValueError("mounted Questionnaire required")
        self._change("question_previous");return self

    def speech_start(self) -> "Widget":
        if self.kind != "speech" or self._app is None: raise ValueError("mounted Speech required")
        self._change("speech_start");return self

    def speech_finish(self) -> "Widget":
        if self.kind != "speech" or self._app is None: raise ValueError("mounted Speech required")
        self._change("speech_finish");return self

    def speech_report(self, text: str, final: bool = False) -> "Widget":
        if self.kind != "speech" or self._app is None or type(final) is not bool: raise ValueError("mounted Speech and bool final required")
        self._change("speech_report",_utf8(text),int(final));return self

    def speech_transcript(self) -> str:
        if self.kind != "speech" or self._app is None: raise ValueError("mounted Speech required")
        text=C.create_string_buffer(4097);length=C.c_uint32();self._change("speech_transcript",text,len(text),C.byref(length));return text.value.decode()

    def speech_active(self) -> bool:
        if self.kind != "speech" or self._app is None: raise ValueError("mounted Speech required")
        active=C.c_uint32();self._change("get_checked",C.byref(active));return bool(active.value)

    def padding(self, value: float) -> "Widget":
        if self.kind != "root_view" or isinstance(value,bool) or not isinstance(value,(int,float)) or not math.isfinite(value) or not 0<=value<=256: raise ValueError("root padding requires 0..256 pixels")
        self._change("set_root_padding",value);self._root_padding=value;return self

    def chart_data(self, data) -> "Widget":
        if self.kind != "chart": raise ValueError("chart_data requires Chart")
        copied = tuple(tuple(pair) for pair in data)
        if len(copied)>64: raise ValueError("Chart accepts at most 64 data")
        for pair in copied:
            if len(pair)!=2 or len(_utf8(pair[0]))>128 or isinstance(pair[1],bool) or not isinstance(pair[1],(int,float)) or not math.isfinite(pair[1]) or abs(pair[1])>1e9 or (self._chart_kind==3 and pair[1]<0): raise ValueError("invalid chart datum")
        native=(_ChartDatum*len(copied))(*[_ChartDatum(_utf8(label),value) for label,value in copied])
        self._change("set_chart",native,len(copied));self._chart_data=copied;return self

    def chart_kind(self, value: str) -> "Widget":
        names={"line":0,"area":1,"bar":2,"pie":3}
        if self.kind != "chart" or value not in names: raise ValueError("invalid chart kind")
        if value=="pie" and any(v<0 for _,v in self._chart_data): raise ValueError("pie values must be nonnegative")
        self._change("set_chart_kind",names[value]);self._chart_kind=names[value];return self

    def current_datum(self, index: int) -> tuple[str,float]:
        if self.kind != "chart" or isinstance(index,bool) or not isinstance(index,int) or not 0<=index<64: raise ValueError("invalid datum index")
        if self._app is None:
            if self._disposed: raise RuntimeError("disposed widget")
            return self._chart_data[index]
        text=C.create_string_buffer(4097);needed=C.c_uint32();value=C.c_double()
        self._change("get_chart",index,text,len(text),C.byref(needed),C.byref(value));return text.value.decode(),value.value

    def plot_points(self, points) -> "Widget":
        if self.kind != "plot": raise ValueError("plot_points requires Plot")
        copied=tuple(tuple(pair) for pair in points)
        if len(copied)>256 or any(len(p)!=2 or any(isinstance(v,bool) or not isinstance(v,(int,float)) or not math.isfinite(v) or abs(v)>16384 for v in p) for p in copied): raise ValueError("invalid plot points")
        native=(_PlotPoint*len(copied))(*[_PlotPoint(*p) for p in copied]);self._change("set_plot",native,len(copied));self._plot_points=copied;return self

    def current_point(self, index: int) -> tuple[float,float]:
        if self.kind != "plot" or isinstance(index,bool) or not isinstance(index,int) or not 0<=index<256: raise ValueError("invalid point index")
        if self._app is None:
            if self._disposed: raise RuntimeError("disposed widget")
            return self._plot_points[index]
        point=_PlotPoint();self._change("get_plot",index,C.byref(point));return point.x,point.y

    def table_data(self, headers: Iterable[str], rows: Iterable[Iterable[str]], *,
                   row_keys: Iterable[int] | None = None,
                   row_capacity: int | None = None) -> "Widget":
        """Copy a DataTable matrix and optionally preserve the viewport by row key.

        Supply unique nonzero uint32 keys to retain position, selection and
        keyboard navigation through updates. State and change events report
        the selected key. Cell access still takes zero-based matrix indices.
        Omitting keys resets retained table state as before.

        An explicit row_capacity opts into the provider's larger matrix API
        and requires row_keys, even for an empty matrix. Omitting capacity
        uses the ordinary 32-row setters.
        """
        if self.kind != "data_table":
            raise ValueError("table_data requires DataTable")
        if row_capacity is None:
            headers = tuple(headers)
            rows = tuple(tuple(row) for row in rows)
            limit = _DATA_TABLE_LEGACY_ROWS
        else:
            if type(row_capacity) is not int or not 1 <= row_capacity <= _DATA_TABLE_MAX_ROWS:
                raise ValueError(f"row_capacity must be an integer in 1..{_DATA_TABLE_MAX_ROWS}")
            if row_keys is None:
                raise ValueError("row_capacity requires row_keys")
            if self._app is not None:
                self._app._require_table_capacity(row_capacity)
            limit = row_capacity
            headers = tuple(islice(headers, 17))
            if not 1 <= len(headers) <= 16:
                raise ValueError("invalid matrix dimensions")
            rows = tuple(tuple(islice(row, len(headers) + 1)) for row in islice(rows, limit + 1))
        if not 1 <= len(headers) <= 16 or len(rows) > limit or any(
            len(row) != len(headers) for row in rows
        ):
            raise ValueError("invalid matrix dimensions")
        if row_keys is None:
            keys = None
        elif row_capacity is None:
            keys = tuple(row_keys)
        else:
            keys = tuple(islice(row_keys, len(rows) + 1))
        if keys is not None:
            if len(keys) != len(rows) or any(
                isinstance(key, bool) or not isinstance(key, int) or not 0 < key <= 0xffffffff
                for key in keys
            ) or len(set(keys)) != len(keys):
                raise ValueError("row_keys must contain one distinct nonzero uint32 per row")
            if (row_capacity is None and self._app is not None and
                    not self._app.supports_keyed_data_table()):
                raise RuntimeError("keyed DataTable replacement is unavailable in this bridge")

        encoded_headers = [_utf8(header, 128) for header in headers]
        encoded_cells = [_utf8(cell, 128) for row in rows for cell in row]
        if self._disposed:
            raise RuntimeError("Widget belongs to a disposed App")
        if self._app is not None:
            native_headers = (C.c_char_p * len(headers))(*encoded_headers)
            native_cells = (C.c_char_p * len(encoded_cells))(*encoded_cells)
            args = native_headers, len(headers), native_cells if rows else None
            if keys is None:
                self._change("set_matrix", *args, len(rows))
            else:
                native_keys = (C.c_uint32 * len(keys))(*keys) if rows else None
                if row_capacity is None:
                    self._change("set_matrix_keyed", *args, native_keys, len(rows))
                else:
                    self._change("set_matrix_capacity", *args, native_keys, len(rows), row_capacity)
        self._matrix = headers, rows, keys, row_capacity
        return self

    def configure_load_more(self, *, enabled: bool = True, threshold: int = 0) -> "Widget":
        """Set DataTable near-end demand. The app owns any in-flight work."""
        if self.kind != "data_table":
            raise ValueError("configure_load_more requires DataTable")
        if type(enabled) is not bool:
            raise TypeError("enabled must be bool")
        if type(threshold) is not int or not 0 <= threshold <= 32:
            raise ValueError("threshold must be an integer in 0..32")
        if self._app is not None:
            self._app._require_table_paging()
        self._change("configure_table_paging", int(enabled), threshold)
        self._load_more_threshold = threshold if enabled else None
        return self

    def poll_load_more(self) -> DataTableLoadMore | None:
        """Consume one copied DataTable paging observation after a frame, if available."""
        if self.kind != "data_table":
            raise ValueError("poll_load_more requires DataTable")
        if self._app is None:
            raise RuntimeError("poll_load_more requires a mounted DataTable")
        app = self._app
        app._require_table_paging()
        wire = _DataTableLoadMoreV1()
        status = app._api.poll_table_paging(app._ptr, self._handle, C.sizeof(wire), C.byref(wire))
        if status == 1:  # CUI_BRIDGE_EMPTY
            return None
        app._api.check(status)
        capacity = ((self._matrix[3] if self._matrix is not None else None) or
                    _DATA_TABLE_LEGACY_ROWS)
        if (wire.reserved or not wire.frame or
                wire.capacity != capacity or
                not wire.visible_begin <= wire.visible_end <= wire.row_count < wire.capacity or
                bool(wire.row_count) != bool(wire.last_row_key)):
            raise BridgeError(8, "malformed DataTable load-more record")
        return DataTableLoadMore(wire.frame, wire.row_count, wire.visible_begin,
                                 wire.visible_end, wire.last_row_key, wire.capacity)

    def current_cell(self, row: int, column: int) -> str:
        if self.kind != "data_table" or any(isinstance(v,bool) or not isinstance(v,int) or not 0<=v<=0xffffffff for v in (row,column)): raise ValueError("invalid cell indices")
        if self._app is None:
            if self._disposed: raise RuntimeError("disposed widget")
            if self._matrix is None: raise RuntimeError("no matrix")
            return self._matrix[1][row][column]
        text=C.create_string_buffer(4097);needed=C.c_uint32();self._change("get_matrix_cell",row,column,text,len(text),C.byref(needed));return text.value.decode()

    def selection_mode(self, value: str) -> "Widget":
        modes={"row":0,"column":1,"cell":2}
        if self.kind != "data_table" or value not in modes: raise ValueError("invalid table selection mode")
        self._change("set_selection_mode",modes[value]);self._selection_mode=modes[value];return self

    def table_state(self) -> dict:
        if self.kind != "data_table" or self._app is None: raise ValueError("table_state requires mounted DataTable")
        values=[C.c_uint32() for _ in range(4)];self._change("get_matrix_state",*[C.byref(v) for v in values]);return dict(zip(("row","column","sortColumn","sortDirection"),(v.value for v in values)))

    def table_cell_bounds(self, row: int, column: int) -> tuple:
        if self.kind != "data_table" or self._app is None or any(isinstance(v,bool) or not isinstance(v,int) or not 0<=v<=0xffffffff for v in (row,column)): raise ValueError("invalid mounted table cell")
        rect=_Rect();self._change("matrix_cell_bounds",row,column,C.byref(rect));return rect.x,rect.y,rect.width,rect.height

    def table_header_bounds(self, column: int) -> tuple:
        if self.kind != "data_table" or self._app is None or isinstance(column,bool) or not isinstance(column,int) or not 0<=column<=0xffffffff: raise ValueError("invalid mounted table header")
        rect=_Rect();self._change("matrix_header_bounds",column,C.byref(rect));return rect.x,rect.y,rect.width,rect.height

    def _live_virtual_list(self) -> App:
        if self.kind != "virtual_list" or self._app is None or self._disposed:
            raise ValueError("mounted VirtualList required")
        self._app._ensure_open()
        if not self._app.supports_virtual_list_rows():
            raise RuntimeError("VirtualList live rows require both V1 bridge symbols")
        return self._app

    def preserve_virtual_list_anchor(self, enabled: bool = True) -> "Widget":
        app = self._live_virtual_list()
        if type(enabled) is not bool:
            raise TypeError("enabled must be bool")
        app._api.check(app._api.virtual_anchor(app._ptr, self._handle, int(enabled)))
        return self

    def set_virtual_list_rows(self, rows: Iterable[tuple[Widget, float]]) -> "Widget":
        """Replace the complete order/extent projection; zero extent uses 32px."""
        app = self._live_virtual_list()
        rows = list(islice(rows, 65))
        if len(rows) > 64 or len(rows) != len(self._children):
            raise ValueError("projection must contain all children, at most 64")
        seen = set()
        wire = (_VirtualListRow * len(rows))()
        ordered = []
        for index, row in enumerate(rows):
            if not isinstance(row, (tuple, list)) or len(row) != 2:
                raise ValueError("rows require (Item, extent) pairs")
            item, extent = row
            if (not isinstance(item, Widget) or item.kind != "item" or
                    item._app is not app or item._disposed or item not in self._children or
                    id(item) in seen):
                raise ValueError("rows require unique live direct Items")
            if (isinstance(extent, bool) or not isinstance(extent, (int, float)) or
                    not (extent == 0 or 1 <= extent <= 4096)):
                raise ValueError("extent must be zero or finite in 1..4096")
            seen.add(id(item))
            ordered.append(item)
            wire[index] = _VirtualListRow(item._handle, extent)
        app._api.check(app._api.virtual_rows(app._ptr, self._handle, C.sizeof(_VirtualListRow),
                                            wire, len(rows)))
        self._children[:] = ordered
        return self

    def append_virtual_list_item(self, item: "Widget") -> "Widget":
        """Mount one unattached leaf Item; submit a complete projection afterward."""
        app = self._live_virtual_list()
        if (not isinstance(item, Widget) or item.kind != "item" or item._app is not None or
                item._disposed or item._children or len(self._children) >= 64):
            raise ValueError("unattached leaf Item required; capacity is 64")
        if (item._radio_content or item._radio_tooltip or item._radio_motion or
                item._radio_tab_index is not None or item._radio_paints):
            raise ValueError("RadioGroup Item options are unavailable here")
        key = item.key
        if key is None or any(node.key == key for node in app._by_handle.values()):
            raise ValueError("live Items require a unique explicit key")
        handle = _Handle()
        app._api.check(app._api.node_create(app._ptr, self._handle, _KIND["item"], _utf8(key), C.byref(handle)))
        item._app, item._handle = app, handle
        try:
            item._change("set_text", _utf8(item._text))
            if item._detail: item._change("set_detail", _utf8(item._detail))
            if item._disabled: item._change("set_disabled", 1)
            if item._checked_set: item._change("set_checked", int(item._checked))
            if item._open_set: item._change("set_open", int(item._open))
            if item._visual is not None: item._change("set_visual", *item._visual)
            if item._image is not None: item._change("set_image", *item._image)
            if item._step_status is not None: item._change("set_step_status", item._step_status)
            if item._panel_size is not None: item._change("set_value", item._panel_size)
            if item._question_flags is not None: item._change("question_config", *map(int, item._question_flags))
            for part, style in item._part_shadow_styles.items():
                target = target_value(part)
                wire = style._wire(app._api)
                item._change("set_target_box_shadows_v1", 16, C.byref(target), 224, C.byref(wire))
        except Exception:
            app._api.node_remove(app._ptr, handle)
            item._app = item._handle = None
            raise
        app._by_handle[(handle.slot, handle.generation)] = item
        self._children.append(item)
        return self

    def remove_virtual_list_item(self, item: "Widget") -> "Widget":
        app = self._live_virtual_list()
        if (not isinstance(item, Widget) or item not in self._children or
                item._app is not app or item._disposed):
            raise ValueError("live direct Item required")
        app._api.check(app._api.node_remove(app._ptr, item._handle))

        def retire(node):
            app._by_handle.pop((node._handle.slot, node._handle.generation), None)
            node._disposed = True
            for child in node._children:
                retire(child)

        retire(item)
        self._children.remove(item)
        return self

    def item_bounds(self, item: "Widget") -> tuple:
        if self.kind not in {"command","virtual_list"} or self._app is None or not isinstance(item,Widget) or item._app is not self._app or item not in self._children: raise ValueError("item_bounds requires mounted same-app child")
        rect=_Rect();self._change("command_bounds" if self.kind=="command" else "virtual_bounds",item._handle,C.byref(rect));return rect.x,rect.y,rect.width,rect.height

    def scroll_to(self, offset: float) -> "Widget":
        if self.kind not in {"message_scroller", "virtual_list"} or isinstance(offset,bool) or not math.isfinite(offset) or not 0 <= offset <= 1000000:
            raise ValueError("invalid message scroll offset")
        if self._app is None: raise RuntimeError("scroll_to requires a mounted component and its first frame")
        self._change("set_value", offset)
        return self

    def panel_size(self, size: float) -> "Widget":
        if self.kind != "item" or not math.isfinite(size) or not 100 <= size <= 4096:
            raise ValueError("panel_size requires a Resizable Item and 100..4096 pixels")
        self._change("set_value",size)
        self._panel_size = size
        return self

    def current_panel_size(self) -> float:
        if self.kind != "item": raise ValueError("panel_size belongs to a Resizable Item")
        if self._app is None:
            if self._disposed: raise RuntimeError("Widget is disposed")
            return self._panel_size or 0.0
        result = C.c_double()
        self._change("get_value",C.byref(result))
        return result.value

    def text_format(self, value: str) -> "Widget":
        names = {"markdown": 0, "html": 1, "plain": 2}
        if self.kind != "text_view" or value not in names: raise ValueError("invalid text format")
        self._change("set_text_format",names[value])
        self._text_format = names[value]
        return self

    def select_text(self, anchor: int, cursor: int) -> "Widget":
        if self.kind != "text_view" or any(type(x) is not int or not 0 <= x <= 0xffffffff for x in (anchor,cursor)):
            raise ValueError("selection requires rendered UTF-8 byte offsets")
        if self._app is None: raise RuntimeError("selection requires a mounted TextView")
        self._change("select_text",anchor,cursor)
        return self

    def selection_text(self) -> str:
        if self.kind != "text_view" or self._app is None: raise RuntimeError("selection requires a mounted TextView")
        buffer = C.create_string_buffer(4097)
        required = C.c_uint32()
        self._change("copy_selection",buffer,4097,C.byref(required))
        return buffer.value.decode("utf-8")

    def hover_color(self, color: str | tuple[int, int, int, int]) -> "Widget":
        if self.kind != "button" or self._variant == "ghost":
            raise ValueError("hover color is supported on primary, secondary and outline buttons")
        if isinstance(color, str):
            if len(color) not in (7, 9) or not color.startswith("#"):
                raise ValueError("expected #RRGGBB or #RRGGBBAA")
            try:
                pieces = [int(color[i:i + 2], 16) for i in range(1, len(color), 2)]
            except ValueError as exc:
                raise ValueError("invalid hex color") from exc
            if len(pieces) == 3:
                pieces.append(255)
            rgba = tuple(pieces)
        else:
            rgba = tuple(color)
        if len(rgba) != 4 or any(not isinstance(x, int) or x < 0 or x > 255 for x in rgba):
            raise ValueError("expected four RGBA bytes")
        self._change("set_hover", *rgba)
        self._hover = rgba
        return self

    def on_click(self, callback: Callable[[Event], object]) -> "Widget":
        if self.kind not in {"button", "dropdown_button", "item", "notification"} or not callable(callback):
            raise ValueError("button callback must be callable")
        self._callback = callback
        return self


def row(key: str | None = None) -> Widget:
    return Widget("row", key)


def column(key: str | None = None) -> Widget:
    return Widget("column", key)


def label(text: str, key: str | None = None) -> Widget:
    return Widget("label", key, text)


def button(key: str) -> Widget:
    return Widget("button", key, key)


def checkbox(key: str, text: str = "") -> Widget:
    return Widget("checkbox", key, text)


def radio(key: str, text: str = "") -> Widget:
    return Widget("radio", key, text)


def switch(key: str, text: str = "") -> Widget:
    return Widget("switch", key, text)


def toggle(key: str, text: str = "") -> Widget:
    return Widget("toggle", key, text)


def slider(key: str, text: str = "") -> Widget:
    return Widget("slider", key, text)


def progress(key: str, text: str = "") -> Widget:
    return Widget("progress", key, text)


def badge(key: str, text: str = "") -> Widget:
    return Widget("badge", key, text)


def input(key: str, text: str = "") -> Widget:
    return Widget("input", key, text)


def textarea(key: str, text: str = "") -> Widget:
    return Widget("textarea", key, text)


def number_input(key: str, text: str = "") -> Widget:
    return Widget("number_input", key, text)


def collapsible(key: str, text: str = "") -> Widget:
    return Widget("collapsible", key, text)


def alert(key: str, text: str = "") -> Widget:
    return Widget("alert", key, text)


def radio_group(key: str, text: str = "") -> Widget:
    return Widget("radio_group", key, text)


def tabs(key: str, text: str = "") -> Widget:
    return Widget("tabs", key, text)


def accordion(key: str, text: str = "") -> Widget:
    return Widget("accordion", key, text)


def item(key: str, text: str = "") -> Widget:
    return Widget("item", key, text)


def select(key: str, text: str = "") -> Widget:
    return Widget("select", key, text)


def combobox(key: str, text: str = "") -> Widget:
    return Widget("combobox", key, text)


def dropdown_button(key: str, text: str = "") -> Widget:
    return Widget("dropdown_button", key, text)


def dialog(key: str, text: str = "") -> Widget:
    return Widget("dialog", key, text)


def alert_dialog(key: str, text: str = "") -> Widget:
    return Widget("alert_dialog", key, text)


def sheet(key: str, text: str = "") -> Widget:
    return Widget("sheet", key, text)


def popover(key: str, text: str = "") -> Widget:
    return Widget("popover", key, text)


def tooltip(key: str, text: str = "") -> Widget:
    return Widget("tooltip", key, text)


def hover_card(key: str, text: str = "") -> Widget:
    return Widget("hover_card", key, text)


def avatar(key: str, text: str = "") -> Widget:
    return Widget("avatar", key, text)


def bubble(key: str, text: str = "") -> Widget:
    return Widget("bubble", key, text)


def empty_view(key: str, text: str = "") -> Widget:
    return Widget("empty_view", key, text)


def group_box(key: str, text: str = "") -> Widget:
    return Widget("group_box", key, text)


def kbd(key: str, text: str = "") -> Widget:
    return Widget("kbd", key, text)


def skeleton(key: str, text: str = "") -> Widget:
    return Widget("skeleton", key, text)


def spinner(key: str, text: str = "") -> Widget:
    return Widget("spinner", key, text)


def tag(key: str, text: str = "") -> Widget:
    return Widget("tag", key, text)


def marker(key: str, text: str = "") -> Widget:
    return Widget("marker", key, text)


def shimmer(key: str, text: str = "") -> Widget:
    return Widget("shimmer", key, text)


def list_view(key: str, text: str = "") -> Widget:
    return Widget("list_view", key, text)


def menu(key: str, text: str = "") -> Widget:
    return Widget("menu", key, text)


def pagination(key: str, text: str = "") -> Pagination:
    return Pagination(key, text)


def rating(key: str, text: str = "") -> Rating:
    return Rating(key, text)


def stepper(key: str, text: str = "") -> Widget:
    return Widget("stepper", key, text)


def description_list(key: str, text: str = "") -> Widget:
    return Widget("description_list", key, text)


def scrollable(key: str, text: str = "") -> Widget:
    return Widget("scrollable", key, text)


def status_bar(key: str, text: str = "") -> Widget:
    return Widget("status_bar", key, text)


def toolbar(key: str, text: str = "") -> Widget:
    return Widget("toolbar", key, text)


def title_bar(key: str, text: str = "") -> Widget:
    return Widget("title_bar", key, text)


def icon(key: str, text: str = "") -> Widget:
    return Widget("icon", key, text)


def message(key: str, text: str = "") -> Widget:
    return Widget("message", key, text)


def attachment(key: str, text: str = "") -> Widget:
    return Widget("attachment", key, text)


def image(key: str, text: str = "") -> Widget:
    return Widget("image", key, text)


def tree(key: str, text: str = "") -> Widget:
    return Widget("tree", key, text)


def time_field(key: str, text: str = "") -> Widget:
    return Widget("time_field", key, text)


def color_picker(key: str, text: str = "") -> Widget:
    return Widget("color_picker", key, text)


def calendar(key: str, text: str = "") -> Widget:
    return Widget("calendar", key, text)


def date_picker(key: str, text: str = "") -> Widget:
    return Widget("date_picker", key, text)


def otp_input(key: str, text: str = "") -> Widget:
    return Widget("otp_input", key, text)


def form(key: str, text: str = "") -> Widget:
    return Widget("form", key, text)


def input_group(key: str, text: str = "") -> Widget:
    return Widget("input_group", key, text)


def notification(key: str, text: str = "") -> Widget:
    return Widget("notification", key, text)


def focus_trap(key: str, text: str = "") -> Widget:
    return Widget("focus_trap", key, text)

def message_scroller(key: str, text: str = "") -> Widget:
    return Widget("message_scroller", key, text)


def carousel(key: str, text: str = "") -> Widget:
    return Widget("carousel", key, text)


def resizable(key: str, text: str = "") -> Widget:
    return Widget("resizable", key, text)


def text_view(key: str, text: str = "") -> Widget:
    return Widget("text_view", key, text)


def editor(key: str, text: str = "") -> Widget:
    return Widget("editor", key, text)


def chart(key: str, text: str = "") -> Widget:
    return Widget("chart", key, text)

def table(key: str, text: str = "") -> Widget:
    return Widget("table", key, text)

def data_table(key: str, text: str = "") -> Widget:
    return Widget("data_table", key, text)

def plot(key: str, text: str = "") -> Widget:
    return Widget("plot", key, text)

def command(key: str, text: str = "") -> Widget:
    return Widget("command", key, text)

def virtual_list(key: str, text: str = "") -> Widget:
    return Widget("virtual_list", key, text)

def sidebar(key: str, text: str = "") -> Widget:
    return Widget("sidebar", key, text)

def root_view(key: str = "root") -> Widget:
    return Widget("root_view", key)

def speech(key: str) -> Widget:
    return Widget("speech", key)

def questionnaire(key: str, text: str = "") -> Widget:
    return Widget("questionnaire", key, text)

def dock(key: str, text: str = "") -> Widget:
    return Widget("dock", key, text)

class App:
    def __init__(self, *, backend: str = "none", width: float = 480, height: float = 240,
                 title: str = "CUI Bridge", font_path: str | None = None,
                 library: str | os.PathLike[str] | None = None,
                 max_nodes: int = 256, max_events: int = 256, initially_hidden: bool = False):
        if backend not in ("none", "sdl"):
            raise ValueError("backend must be 'none' or 'sdl'")
        if backend == "sdl" and not font_path:
            raise ValueError("SDL requires an explicit font_path")
        if type(max_nodes) is not int or not 1 <= max_nodes <= 1024:
            raise ValueError("max_nodes must be an integer in 1..1024")
        if type(max_events) is not int or not 1 <= max_events <= 4096:
            raise ValueError("max_events must be an integer in 1..4096")
        if type(initially_hidden) is not bool:
            raise TypeError("initially_hidden must be bool")
        self._api = _API(library)
        self._creation_thread = current_thread()
        self._icons_active = False
        options = _Options(C.sizeof(_Options), 1, 0 if backend == "none" else 1,
                           max_nodes, max_events, width, height,
                           _utf8(title), _utf8(font_path) if font_path else None)
        status = C.c_int()
        if initially_hidden:
            if self._api.create_window is None:
                raise RuntimeError("bridge hidden-window API unavailable")
            window = _WindowOptions(C.sizeof(_WindowOptions), 1, 1)
            self._ptr = self._api.create_window(C.byref(options), C.byref(window), C.byref(status))
        else:
            self._ptr = self._api.create(C.byref(options), C.byref(status))
        if not self._ptr:
            self._api.check(status.value)
            raise RuntimeError("bridge create failed")
        self._root: Widget | None = None
        self._by_handle: dict[tuple[int, int], Widget] = {}
        self._close_requested = False
        self._slider_enabled = False
        self._slider_dispatching = False
        self._time_field_enabled = False
        self._time_field_dispatching = False

    def _ensure_open(self) -> None:
        if not self._ptr:
            raise RuntimeError("App is disposed")
        if self._icons_active:
            self._ensure_icon_thread()

    def _ensure_icon_thread(self) -> None:
        if current_thread() is not self._creation_thread:
            raise RuntimeError("Icon and Badge App calls require the app's owner thread")

    def supports_keyed_data_table(self) -> bool:
        """Report whether the loaded bridge exports keyed DataTable replacement."""
        self._ensure_open()
        return self._api.set_matrix_keyed is not None

    def supports_radio_motion(self) -> bool:
        """Report whether the loaded bridge can animate Radio and RadioGroup."""
        self._ensure_open()
        return self._api.radio_set_motion is not None

    def _require_radio_motion(self) -> None:
        if not self.supports_radio_motion():
            raise RuntimeError("Radio motion requires cui_bridge_radio_set_motion_v1")

    def supports_stepper_fill(self) -> bool:
        """Report whether the provider offers retained Stepper Fill."""
        self._ensure_open()
        return self._api.set_stepper_fill is not None

    def _require_stepper_fill(self) -> None:
        if not self.supports_stepper_fill():
            raise RuntimeError("Stepper Fill requires cui_bridge_set_stepper_fill")

    def supports_radio_tab_index(self) -> bool:
        """Report whether the loaded bridge offers signed Radio and RadioGroup Tab order."""
        self._ensure_open()
        return self._api.radio_set_tab_index is not None

    def _require_radio_tab_index(self) -> None:
        if not self.supports_radio_tab_index():
            raise RuntimeError("Radio Tab order requires cui_bridge_radio_set_tab_index_v1")

    def supports_radio_paint(self) -> bool:
        """Report whether the loaded bridge accepts retained Radio paint layers."""
        self._ensure_open()
        return self._api.radio_set_paint is not None

    def _require_radio_paint(self) -> None:
        if not self.supports_radio_paint():
            raise RuntimeError("Radio paint requires cui_bridge_radio_set_paint_v1")

    def supports_radio_item_content(self) -> bool:
        """Report whether direct RadioGroup Items can paint passive children."""
        self._ensure_open()
        return self._api.radio_item_set_content is not None

    def _require_radio_item_content(self) -> None:
        if not self.supports_radio_item_content():
            raise RuntimeError("Radio decoration requires cui_bridge_radio_item_set_content_v1")

    def supports_radio_tooltip(self) -> bool:
        """Report whether the loaded bridge can attach a hint to a radio option."""
        self._ensure_open()
        return self._api.radio_set_tooltip is not None

    def _require_radio_tooltip(self) -> None:
        if not self.supports_radio_tooltip():
            raise RuntimeError("Radio tooltips require cui_bridge_radio_set_tooltip_v1")

    def data_table_max_row_capacity(self) -> int | None:
        """Return the opt-in row limit usable by Python, or None on older providers."""
        self._ensure_open()
        return self._api.table_capacity_limit

    def _require_table_capacity(self, capacity: int) -> None:
        if current_thread() is not self._creation_thread:
            raise RuntimeError("DataTable capacity requires the app's owner thread")
        maximum = self.data_table_max_row_capacity()
        if maximum is None:
            raise RuntimeError("DataTable capacity requires a compatible bridge symbol group")
        if capacity > maximum:
            raise ValueError(f"row_capacity exceeds the provider's {maximum}-row limit")

    def supports_data_table_paging(self) -> bool:
        """Report whether the bridge offers the complete DataTable paging V1 API."""
        self._ensure_open()
        return self._api.poll_table_paging is not None

    def _require_table_paging(self) -> None:
        if current_thread() is not self._creation_thread:
            raise RuntimeError("DataTable paging requires the app's owner thread")
        if not self.supports_data_table_paging():
            raise RuntimeError("DataTable paging requires a complete, compatible bridge symbol group")

    def font_fallbacks(self, paths: list[str] | tuple[str, ...]) -> "App":
        """Replace the SDL font's owned ordered fallback chain (at most 32)."""
        self._ensure_open()
        if not isinstance(paths, (list, tuple)) or len(paths) > 32:
            raise ValueError("fallbacks must be a list/tuple of at most 32 paths")
        encoded = [_utf8(path) for path in paths]
        if any(not path for path in encoded): raise ValueError("font paths cannot be empty")
        array = (C.c_char_p * len(encoded))(*encoded)
        self._api.check(self._api.set_font_fallbacks(self._ptr,array,len(encoded)))
        return self

    def mount(self, tree: Widget) -> "App":
        from .disclosure import preflight as disclosure_preflight, mount as disclosure_mount
        self._ensure_open()
        if self._root is not None:
            raise RuntimeError("App already has a root; dispose it before remounting")
        if not isinstance(tree, Widget) or tree.kind not in ("row", "column", "root_view"):
            raise TypeError("root must be row(), column() or root_view()")
        seen: set[int] = set()
        keys: set[str] = set()
        order: list[tuple[Widget, Widget | None, str]] = []
        def validate(node: Widget, parent: Widget | None, path: str, depth: int,
                     lineage=(), radio_content: bool = False) -> None:
            if depth >= 128:
                raise ValueError("widget tree exceeds 128 levels")
            if id(node) in seen:
                raise ValueError("widget tree contains a cycle or shared node")
            if node._app is not None and node._app is not self:
                raise ValueError("widget belongs to a different App")
            if node._disposed:
                raise ValueError("widget belongs to a disposed App")
            seen.add(id(node))
            disclosure_preflight(node, parent, self)
            if node.kind == "slider_model":
                if not isinstance(node, SliderModel):
                    raise TypeError("use slider_model() for the controlled route")
                if any(ancestor.kind not in ("row", "column") for ancestor in lineage):
                    raise ValueError("controlled Slider requires Row/Column ancestry")
            if isinstance(node, Rating):
                node._preflight_rating(self)
            if isinstance(node, Pagination):
                node._preflight_pagination(self)
            if node._stepper_fill is not None:
                self._require_stepper_fill()
            if node._radio_motion:
                self._require_radio_motion()
            if node._radio_tab_index is not None:
                self._require_radio_tab_index()
            if node._radio_paints:
                if node.kind == "item" and (parent is None or parent.kind != "radio_group"):
                    raise ValueError("radio_paint requires a direct RadioGroup Item")
                self._require_radio_paint()
            if radio_content and node.kind not in _RADIO_CONTENT_KINDS:
                raise ValueError(f"{node.kind} cannot decorate a RadioGroup Item")
            if node._radio_content:
                if parent is None or parent.kind != "radio_group":
                    raise ValueError("decorate_content requires a direct RadioGroup Item")
                self._require_radio_item_content()
            if node._radio_tooltip is not None:
                if node.kind == "item" and (parent is None or parent.kind != "radio_group"):
                    raise ValueError("radio_tooltip requires a direct RadioGroup Item")
                self._require_radio_tooltip()
            if isinstance(node, ConfiguredIcon):
                node._preflight_icon(self)
            if node.kind == "configured_badge":
                if not isinstance(node, ConfiguredBadge):
                    raise TypeError("use configured_badge() for the Badge container")
                node._preflight_badge(self)
            if node._load_more_threshold is not None:
                self._require_table_paging()
                if node._matrix is None:
                    raise ValueError("DataTable paging requires a matrix with column headers")
            if node._matrix is not None and node._matrix[3] is not None:
                self._require_table_capacity(node._matrix[3])
            key = node.key if node.key is not None else f"{node.kind}@{path}"
            _utf8(key)
            if _KIND[node.kind] >= 3:
                _utf8(node._text)
            if key in keys:
                raise ValueError(f"duplicate key: {key}")
            keys.add(key)
            order.append((node, parent, key))
            for index, child in enumerate(node._children):
                validate(child, node, f"{path}/{index}", depth + 1, lineage + (node,),
                         radio_content or node._radio_content)
        validate(tree, None, "root", 0)
        if any(isinstance(node, ResponsiveSkeleton) for node, _, _ in order):
            from .skeleton_presentation import require_api
            require_api(self)
        if any(node.kind == "slider_model" for node, _, _ in order) and not self._slider_enabled:
            self.enable_slider_requests()
        if any(isinstance(node, LocalizedTimeField) for node, _, _ in order) and not self._time_field_enabled:
            self.enable_time_changes()
        created: list[Widget] = []
        try:
            for node, parent, key in order:
                handle = _Handle()
                parent_handle = parent._handle if parent else _Handle()
                if node.kind == "slider_model":
                    from .slider_model import mount_node
                    mount_node(self, node, parent_handle, key, handle)
                elif isinstance(node, ConfiguredBadge):
                    node._create_badge(self, parent_handle, key, handle)
                else:
                    self._api.check(self._api.node_create(self._ptr, parent_handle, _KIND[node.kind],
                                                           _utf8(key), C.byref(handle)))
                node._app, node._handle = self, handle
                created.append(node)
                self._by_handle[(handle.slot, handle.generation)] = node
                if node.kind == "root_view": node._change("set_root_padding",node._root_padding)
                if node.kind == "otp_input": node._change("set_total",node._total)
                if _KIND[node.kind] >= 3 and node.kind != "configured_badge":
                    node._change("set_text", _utf8(node._text))
                if isinstance(node, ConfiguredIcon):
                    node._set_icon(self, handle, node._icon_config)
                if node.kind in ("row", "column"):
                    node._change("set_gap", node._gap)
                if node.kind in _BOOLEAN or (node.kind == "item" and node._checked_set):
                    node._change("set_checked", int(node._checked))
                if node._radio_motion:
                    node._change("radio_set_motion", 1)
                if node._radio_tab_index is not None:
                    node._change("radio_set_tab_index", 1, node._radio_tab_index)
                if node._radio_paints:
                    for layer, paint in node._radio_paints.items():
                        wire = radio_paint_wire(node.kind, layer, paint)
                        assert wire is not None
                        node._change("radio_set_paint", int(layer), C.byref(wire))
                if node._radio_content:
                    node._change("radio_item_set_content", 1)
                if node._radio_tooltip is not None:
                    node._change("radio_set_tooltip", *node._radio_tooltip)
                if node.kind in {"slider", "number_input"}:
                    node._change("set_range", *node._range)
                if node.kind in {"pagination", "rating"}:
                    node._change("set_total", node._total)
                if isinstance(node, Pagination):
                    node._mount_pagination()
                if isinstance(node, Rating):
                    node._mount_rating()
                disclosure_mount(node)
                if node.kind in _NUMERIC:
                    node._change("set_value", node._value)
                if node.kind == "attachment": node._change("set_attachment",node._attachment)
                if node._image is not None: node._change("set_image",*node._image)
                if node.kind == "time_field": node._change("set_time",_Time(*node._time))
                if isinstance(node, LocalizedTimeField):
                    from .time_field_localized import mount_node as mount_time_node
                    mount_time_node(self, node)
                if isinstance(node, ResponsiveSkeleton):
                    from .skeleton_presentation import mount_node as mount_skeleton_node
                    mount_skeleton_node(node)
                if node.kind in {"calendar", "date_picker"}: node._change("set_date",_Date(*node._date))
                if node.kind == "color_picker": node._change("set_color",node._color)
                if node.kind == "chart": node.chart_kind(("line","area","bar","pie")[node._chart_kind]);node.chart_data(node._chart_data)
                if node.kind == "plot": node.plot_points(node._plot_points)
                if node.kind == "data_table": node._change("set_selection_mode",node._selection_mode)
                if node.kind == "sidebar" and node._open_set: node._change("set_open",int(node._open))
                if node.kind == "data_table" and node._matrix is not None:
                    headers, rows, keys, capacity = node._matrix
                    node.table_data(headers, rows, row_keys=keys, row_capacity=capacity)
                    if node._load_more_threshold is not None:
                        node.configure_load_more(threshold=node._load_more_threshold)
                if node.kind == "text_view": node._change("set_text_format",node._text_format)
                if node._question_flags is not None: node._change("question_config",*[int(v) for v in node._question_flags])
                if node._panel_size is not None: node._change("set_value",node._panel_size)
                if node._visual is not None:
                    node._change("set_visual", *node._visual)
                if node._extent is not None:
                    node._change("set_scroll_extent", *node._extent)
                if node._step_status is not None:
                    node._change("set_step_status", node._step_status)
                if node.kind in _DISABLED and (node.kind != "item" or node._disabled):
                    node._change("set_disabled", int(node._disabled))
                if node.kind == "collapsible" or (node.kind == "item" and node._open_set):
                    node._change("set_open", int(node._open))
                if node.kind in _DETAIL:
                    node._change("set_detail", _utf8(node._detail))
                if node._stepper_fill is not None:
                    node._change("set_stepper_fill", *node._stepper_fill)
                if node._stepper_visual is not None:
                    node._change("set_stepper_visual", *node._stepper_visual)
                if node._design_size is not None:
                    node._change("set_design_size", _SIZE[node._design_size])
                if node._shadow_style is not None:
                    wire = node._shadow_style._wire(self._api)
                    node._change("set_box_shadows_v1",224,C.byref(wire))
                for part,style in node._part_shadow_styles.items():
                    target=target_value(part);wire=style._wire(self._api)
                    node._change("set_target_box_shadows_v1",16,C.byref(target),224,C.byref(wire))
                if node._avatar_identity is not None:
                    enabled, hue = node._avatar_identity
                    node._change("set_avatar_identity", int(enabled), int(hue is not None), hue or 0)
                if node._avatar_placeholder is not None:
                    enabled, name = node._avatar_placeholder
                    node._change("set_avatar_placeholder", int(enabled), _utf8(name))
                if node.kind == "button":
                    node._change("set_variant", _VARIANT[node._variant])
                    node._change("set_size", _SIZE[node._size])
                    node._change("set_rounding", _ROUND[node._rounding], node._radius)
                    node._change("set_disabled", int(node._disabled))
                    if node._hover:
                        node._change("set_hover", *node._hover)
            for node, _, _ in order:
                if node._anchor is not None:
                    if node._anchor._app is not self:
                        raise ValueError("anchor belongs to another App or unmounted tree")
                    node._change("set_anchor", node._anchor._handle)
            for node, _, _ in order:
                if node.kind in _SURFACE | {"date_picker", "focus_trap", "sidebar"} and node._open:
                    node._change("set_open", 1)
                if node._selected is not None:
                    node._change("set_selected", node._selected._handle)
        except Exception:
            if created:
                self._api.node_remove(self._ptr, created[0]._handle)
            for node in created:
                node._app = node._handle = None
            self._by_handle.clear()
            raise
        self._root = tree
        return self

    def step(self, dt: float = 1 / 60) -> None:
        if self._time_field_dispatching:
            raise RuntimeError("step is unavailable during localized TimeField dispatch")
        if self._slider_dispatching:
            raise RuntimeError("step is unavailable during controlled Slider dispatch")
        if getattr(self, "_running", False):
            raise RuntimeError("step is unavailable during run")
        self._ensure_open()
        self._api.check(self._api.step(self._ptr, dt))

    def queue_pointer(self, phase: str, x: float, y: float) -> None:
        self._ensure_open()
        phases = {"move": 0, "down": 1, "up": 2}
        if phase not in phases:
            raise ValueError("phase must be move/down/up")
        self._api.check(self._api.queue_pointer(self._ptr, phases[phase], x, y))

    def queue_key(self, phase: str, key: str, modifiers: int = 0, repeat: bool = False) -> None:
        self._ensure_open()
        phases = {"down": 0, "up": 1}
        keys = {"tab": 1, "enter": 2, "space": 3, "escape": 4,
                "left": 5, "right": 6, "up": 7, "down": 8}
        if phase not in phases or key not in keys:
            raise ValueError("unsupported key or phase")
        if type(modifiers) is not int or not 0 <= modifiers <= 15:
            raise ValueError("modifiers must contain only the four supported bits")
        self._api.check(self._api.queue_key(self._ptr, phases[phase], keys[key],
                                            modifiers, int(repeat)))

    def queue_text(self, text: str) -> None:
        self._ensure_open()
        self._api.check(self._api.queue_text(self._ptr, _utf8(text)))

    def bounds(self, node: Widget) -> tuple[float, float, float, float]:
        self._ensure_open()
        if node._app is not self or node._handle is None:
            raise ValueError("widget does not belong to this App")
        rect = _Rect()
        self._api.check(self._api.bounds(self._ptr, node._handle, C.byref(rect)))
        return rect.x, rect.y, rect.width, rect.height

    def dropdown_bounds(self, node: Widget, part: str) -> tuple[float, float, float, float]:
        self._ensure_open()
        if node._app is not self or part not in {"action", "menu"}:
            raise ValueError("invalid dropdown node or part")
        rect = _Rect()
        self._api.check(self._api.dropdown_bounds(self._ptr,node._handle,
                                                 0 if part == "action" else 1,C.byref(rect)))
        return rect.x,rect.y,rect.width,rect.height

    def queue_scroll(self, x: float, y: float, dx: float, dy: float) -> None:
        self._ensure_open()
        self._api.check(self._api.queue_scroll(self._ptr,x,y,dx,dy))

    def numeric_part_bounds(self, node: Widget, part: int) -> tuple[float,float,float,float]:
        self._ensure_open()
        if node._app is not self or type(part) is not int or not 0 <= part <= 0xffffffff:
            raise ValueError("invalid numeric node or part")
        rect = _Rect()
        self._api.check(self._api.numeric_bounds(self._ptr,node._handle,part,C.byref(rect)))
        return rect.x,rect.y,rect.width,rect.height

    def color_part_bounds(self, node: Widget, part: int) -> tuple[float,float,float,float]:
        self._ensure_open()
        if node._app is not self or type(part) is not int or part not in (1,2,3):
            raise ValueError("invalid color picker node or part")
        rect = _Rect()
        self._api.check(self._api.color_bounds(self._ptr,node._handle,part,C.byref(rect)))
        return rect.x,rect.y,rect.width,rect.height

    def calendar_day_bounds(self, node: Widget, year: int, month: int, day: int) -> tuple[float,float,float,float]:
        self._ensure_open()
        if node._app is not self or any(type(x) is not int for x in (year,month,day)):
            raise ValueError("invalid calendar node or date")
        datetime.date(year,month,day)
        rect = _Rect()
        self._api.check(self._api.day_bounds(self._ptr,node._handle,_Date(year,month,day),C.byref(rect)))
        return rect.x,rect.y,rect.width,rect.height

    def set_window_visible(self, visible: bool) -> "App":
        self._ensure_open()
        if type(visible) is not bool: raise TypeError("visible must be bool")
        if self._api.set_window_visible is None: raise RuntimeError("bridge visibility API unavailable")
        self._api.check(self._api.set_window_visible(self._ptr, int(visible)))
        return self

    def window_visible(self) -> bool:
        self._ensure_open()
        if self._api.get_window_visible is None: raise RuntimeError("bridge visibility API unavailable")
        visible = C.c_uint32()
        self._api.check(self._api.get_window_visible(self._ptr, C.byref(visible)))
        return bool(visible.value)

    def clipboard_set(self, text: str) -> "App":
        self._ensure_open();encoded=_utf8(text,65536);self._api.check(self._api.clipboard_set(self._ptr,encoded));return self

    def clipboard_get(self) -> str:
        self._ensure_open();text=C.create_string_buffer(65537);length=C.c_uint32();self._api.check(self._api.clipboard_get(self._ptr,text,len(text),C.byref(length)));return text.value.decode()

    def theme_presets(self) -> tuple[str,...]:
        self._ensure_open();names=[]
        for index in range(self._api.theme_count()):
            text=C.create_string_buffer(128);length=C.c_uint32();self._api.check(self._api.theme_name(index,text,len(text),C.byref(length)));names.append(text.value.decode())
        return tuple(names)

    def theme_preset(self, index: int) -> "App":
        self._ensure_open()
        if type(index) is not int or not 0<=index<=0xffffffff: raise ValueError("invalid theme index")
        self._api.check(self._api.set_theme(self._ptr,index));return self

    def current_theme_preset(self) -> int | None:
        self._ensure_open();index=C.c_uint32();code=self._api.get_theme(self._ptr,C.byref(index))
        if code==1: return None
        self._api.check(code);return index.value

    def resizable_handle_bounds(self, group: Widget, preceding: Widget) -> tuple[float,float,float,float]:
        self._ensure_open()
        if group._app is not self or preceding._app is not self:
            raise ValueError("resizable nodes must belong to this App")
        rect = _Rect()
        self._api.check(self._api.resizable_bounds(self._ptr,group._handle,preceding._handle,C.byref(rect)))
        return rect.x,rect.y,rect.width,rect.height

    def poll_events(self) -> Iterator[Event]:
        self._ensure_open()
        while True:
            if not self._ptr:
                return
            raw = _EventEx()
            buffer = C.create_string_buffer(4097)
            required = C.c_uint32()
            status = self._api.poll_event_ex(self._ptr, C.byref(raw), buffer, 4097,
                                             C.byref(required))
            if status in (1, 2):
                return
            self._api.check(status)
            node = self._by_handle.get((raw.node.slot, raw.node.generation))
            if node is not None:
                yield Event({1:"click", 2:"change", 3:"close"}[raw.type], node, raw.frame_sequence,
                            bool(raw.checked), raw.value, buffer.value.decode("utf-8"),
                            (raw.selected.slot, raw.selected.generation))

    def poll_notifications_v1(self) -> Iterator[NotificationV1]:
        """Consume owning V1 snapshots without callbacks; shares the legacy queue.

        Requires optional V1 symbols, exact native layout and a <=1MiB payload
        including NUL. Capacity failures retain the event for a capable consumer.
        Existing step/dispatch behavior is unchanged.
        """
        self._ensure_open()
        try:
            size = self._api.lib.cui_bridge_notification_v1_size
            versions = self._api.lib.cui_bridge_notification_versions
            poll = self._api.lib.cui_bridge_poll_notification_v1
        except AttributeError as error:
            raise BridgeError(9, "V1 notification symbols unavailable") from error
        size.argtypes = versions.argtypes = []
        size.restype = versions.restype = C.c_uint32
        if not (versions() & 1) or size() != C.sizeof(_NotificationV1):
            raise BridgeError(8, "V1 notification capability/layout mismatch")
        poll.restype = C.c_int
        poll.argtypes = [C.c_void_p, C.c_uint32, C.POINTER(_NotificationV1),
                         C.c_void_p, C.c_uint32, C.POINTER(C.c_uint32)]
        while self._ptr:
            raw = _NotificationV1()
            capacity = 4097
            for attempt in range(2):
                buffer = C.create_string_buffer(capacity)
                required = C.c_uint32()
                status = poll(self._ptr, C.sizeof(raw), C.byref(raw), buffer, capacity,
                              C.byref(required))
                if status != 5:
                    break
                if attempt or required.value <= capacity or required.value > 1024 * 1024:
                    raise BridgeError(5, "V1 notification payload exceeds limit or invalid capacity retry")
                capacity = required.value
            if status in (1, 2):
                return
            self._api.check(status)
            if raw.schema_version != 1 or raw.struct_size != C.sizeof(raw) or raw.text_length >= capacity:
                raise BridgeError(8, "invalid V1 notification output")
            text = bytes(buffer.raw[:raw.text_length]).decode("utf-8")
            handle = (raw.node.slot, raw.node.generation)
            selected = (raw.legacy.selected.slot, raw.legacy.selected.generation)
            legacy = LegacySnapshotV1(raw.legacy.type, raw.legacy.checked, raw.legacy.value,
                                      text, selected)
            common = (handle, self._by_handle.get(handle), raw.frame_sequence,
                      bool(raw.flags & 1), raw.node_kind, legacy)
            if raw.payload_kind == 0:
                yield ActivationNotificationV1(*common)
            elif raw.payload_kind == 1:
                yield BooleanChangeNotificationV1(*common, bool(raw.payload.boolean_value))
            elif raw.payload_kind == 5:
                table = raw.payload.table
                yield TableChangeNotificationV1(*common,
                    "selection" if table.reason == 1 else "sort", table.row, table.column,
                    table.sort_column, table.sort_direction)
            elif raw.payload_kind == 6:
                yield SurfaceCloseNotificationV1(*common, raw.payload.surface_result)
            else:
                yield LegacyNotificationV1(*common, self._by_handle.get(selected))

    def poll_value_notifications_v1(self) -> Iterator[ValueNotificationV1]:
        """Owning opt-in value views on the shared queue; never dispatch callbacks.

        Requires optional value symbols and exact V1 layout. Requests only known
        advertised scalar/text/selection categories. Old libraries fail without
        fallback draining; text retries once up to 1MiB including NUL.
        """
        self._ensure_open()
        try:
            size = self._api.lib.cui_bridge_notification_v1_size
            versions = self._api.lib.cui_bridge_notification_versions
            categories = self._api.lib.cui_bridge_notification_value_payloads_v1
            poll = self._api.lib.cui_bridge_poll_value_notification_v1
        except AttributeError as error:
            raise BridgeError(9, "V1 value notification symbols unavailable") from error
        for query in (size, versions, categories):
            query.argtypes = []
            query.restype = C.c_uint32
        if not (versions() & 1) or size() != C.sizeof(_NotificationV1):
            raise BridgeError(8, "V1 value notification capability/layout mismatch")
        mask = categories() & ((1 << 2) | (1 << 3) | (1 << 4))
        if not mask:
            raise BridgeError(9, "V1 value notification categories unavailable")
        poll.restype = C.c_int
        poll.argtypes = [C.c_void_p, C.c_uint32, C.c_uint32, C.POINTER(_NotificationV1),
                         C.c_void_p, C.c_uint32, C.POINTER(C.c_uint32)]
        while self._ptr:
            raw = _NotificationV1()
            capacity = 4097
            for attempt in range(2):
                buffer = C.create_string_buffer(capacity)
                required = C.c_uint32()
                status = poll(self._ptr, C.sizeof(raw), mask, C.byref(raw), buffer,
                              capacity, C.byref(required))
                if status != 5:
                    break
                if attempt or required.value <= capacity or required.value > 1024 * 1024:
                    raise BridgeError(5, "V1 value payload exceeds limit or invalid capacity retry")
                capacity = required.value
            if status in (1, 2):
                return
            self._api.check(status)
            if (raw.schema_version != 1 or raw.struct_size != C.sizeof(raw) or
                    raw.text_length >= capacity or buffer.raw[raw.text_length] != 0 or
                    raw.flags & ~1 or bool(raw.flags & 1) != bool(raw.frame_sequence) or
                    required.value != raw.text_length + 1 or
                    raw.legacy.text_length != raw.text_length or
                    raw.legacy.frame_sequence != raw.frame_sequence or
                    (raw.node.slot, raw.node.generation) !=
                    (raw.legacy.node.slot, raw.legacy.node.generation) or
                    raw.kind not in (1, 2, 3) or raw.legacy.type != raw.kind):
                raise BridgeError(8, "invalid V1 value notification output")
            try:
                text = bytes(buffer.raw[:raw.text_length]).decode("utf-8")
            except UnicodeDecodeError as error:
                raise BridgeError(8, "invalid V1 value notification UTF-8") from error
            handle = (raw.node.slot, raw.node.generation)
            selected = (raw.legacy.selected.slot, raw.legacy.selected.generation)
            legacy = LegacySnapshotV1(raw.legacy.type, raw.legacy.checked, raw.legacy.value,
                                      text, selected)
            common = (handle, self._by_handle.get(handle), raw.frame_sequence,
                      bool(raw.flags & 1), raw.node_kind, legacy)
            tag = raw.payload_kind
            if ((tag == 0 and raw.kind != 1) or
                    (tag in (1, 2, 3, 4, 5) and raw.kind != 2) or
                    (tag == 6 and raw.kind != 3) or tag not in range(8) or
                    (tag in (2, 3, 4) and not mask & (1 << tag))):
                raise BridgeError(8, "invalid V1 value notification kind/tag")
            if tag == 0:
                yield ActivationNotificationV1(*common)
            elif tag == 1:
                if raw.payload.boolean_value > 1:
                    raise BridgeError(8, "invalid V1 boolean payload")
                yield BooleanChangeNotificationV1(*common, bool(raw.payload.boolean_value))
            elif tag == 2:
                value = raw.payload.scalar_value
                if not math.isfinite(value) or value != raw.legacy.value:
                    raise BridgeError(8, "invalid V1 scalar payload")
                yield ScalarChangeNotificationV1(*common, value)
            elif tag == 3:
                yield TextChangeNotificationV1(*common, text)
            elif tag == 4:
                value = (raw.payload.selected.slot, raw.payload.selected.generation)
                if value != selected or bool(value[0]) != bool(value[1]):
                    raise BridgeError(8, "invalid V1 selection payload")
                yield SelectionChangeNotificationV1(*common, value, self._by_handle.get(value))
            elif tag == 5:
                table = raw.payload.table
                if table.reason not in (1, 2):
                    raise BridgeError(8, "invalid V1 table payload")
                yield TableChangeNotificationV1(*common,
                    "selection" if table.reason == 1 else "sort", table.row, table.column,
                    table.sort_column, table.sort_direction)
            elif tag == 6:
                if raw.payload.surface_result not in (1, 2, 3):
                    raise BridgeError(8, "invalid V1 surface payload")
                yield SurfaceCloseNotificationV1(*common, raw.payload.surface_result)
            else:
                yield LegacyNotificationV1(*common, self._by_handle.get(selected))

    def dispatch(self) -> int:
        """Invoke callbacks after step; Python exceptions propagate outside C frames."""
        if self._time_field_dispatching:
            raise RuntimeError("nested localized TimeField dispatch is unavailable")
        if self._slider_dispatching:
            raise RuntimeError("nested controlled Slider dispatch is unavailable")
        count = 0
        for event in self.poll_events():
            callback = (event.node._callback if event.type == "click" else
                        event.node._close_callback if event.type == "close" else event.node._change_callback)
            if callback:
                callback(event)
            count += 1
        if self._slider_enabled and self._ptr:
            count += self.dispatch_slider_requests()
        if self._time_field_enabled and self._ptr:
            count += self.dispatch_time_changes()
        return count

    def supports_virtual_list_rows(self) -> bool:
        self._ensure_open()
        return self._api.virtual_rows is not None and self._api.virtual_anchor is not None

    def supports_slider_model(self) -> bool:
        from .slider_model import supported
        return supported(self)

    def enable_slider_requests(self, capacity: int = 4096) -> "App":
        from .slider_model import enable
        return enable(self, capacity)

    def poll_slider_requests(self) -> Iterator["SliderRequest"]:
        from .slider_model import poll
        return poll(self)

    def accept_slider_request(self, receipt: "SliderReceipt", value: "SliderSingle | SliderRange") -> None:
        from .slider_model import accept
        accept(self, receipt, value)

    def dispatch_slider_requests(self) -> int:
        if self._time_field_dispatching:
            raise RuntimeError("nested typed dispatch is unavailable")
        from .slider_model import dispatch
        return dispatch(self)

    def supports_pagination_custom_size(self) -> bool:
        from .pagination_options import custom_supported
        return custom_supported(self)

    def supports_disclosure_title_state(self) -> bool:
        """Probe the optional title state setter independently."""
        from .disclosure import supports_optional
        return supports_optional(self, "disclosure_title_state")

    def supports_disclosure_heading(self) -> bool:
        """Return whether the optional retained heading setter is available."""
        from .disclosure import supports_optional
        return supports_optional(self, "disclosure_heading")

    def supports_disclosure_divider(self) -> bool:
        """Return whether the optional retained divider setter is available."""
        from .disclosure import supports_optional
        return supports_optional(self, "disclosure_divider")

    def supports_disclosure_presentation(self) -> bool:
        """Return whether both optional retained disclosure setters are available."""
        from .disclosure import supported
        return supported(self)

    def supports_rating_options(self) -> bool:
        from .rating_options import supported
        return supported(self)

    def supports_pagination_options(self) -> bool:
        from .pagination_options import supported
        return supported(self)

    def supports_skeleton_presentation(self) -> bool:
        from .skeleton_presentation import supported
        return supported(self)

    def supports_icon_resources(self) -> bool:
        from .icons import supported
        return supported(self, "icon_resources")

    def supports_icon_presentation(self) -> bool:
        from .icons import supported
        return supported(self, "icon_presentation")

    def supports_configured_badge(self) -> bool:
        from .icons import supported
        return supported(self, "configured_badge")

    def icon_resource(self, source: IconSource | None = None, *,
                      limits: IconLimits | None = None) -> IconResource:
        """Create an explicitly owned resource. A default source is empty."""
        return IconResource(self, IconSource() if source is None else source,
                            limits=IconLimits() if limits is None else limits)

    def default_icon_limits(self) -> IconLimits:
        from .icons import default_limits
        return default_limits(self)

    def icon_catalog_count(self) -> int:
        from .icons import catalog_entry_count
        return catalog_entry_count(self)

    def icon_catalog_entry(self, index: int) -> IconCatalogEntry:
        from .icons import catalog_entry
        return catalog_entry(self, index)

    def query_icon(self, name: str) -> IconAvailability:
        from .icons import query_icon
        return query_icon(self, name)

    def copy_icon_asset(self, name: str) -> str | None:
        from .icons import copy_asset
        return copy_asset(self, name)

    def supports_time_field(self) -> bool:
        from .time_field_localized import supported
        return supported(self)

    def enable_time_changes(self, capacity: int = 4096) -> "App":
        from .time_field_localized import enable
        return enable(self, capacity)

    def poll_time_changes(self) -> Iterator["TimeFieldChange"]:
        from .time_field_localized import poll
        return poll(self)

    def dispatch_time_changes(self) -> int:
        from .time_field_localized import dispatch
        return dispatch(self)

    def run(self, dt: float = 1 / 60, *,
            after_frame: Callable[["App"], object] | None = None) -> None:
        """Run the native event loop; dispatch after paint, including live resize.

        after_frame, if provided, runs after regular callbacks on the owner thread.
        dt is retained for source compatibility and validated; native scheduling
        supplies actual frame timing. Older bridges fail clearly without fallback.
        """
        if self._time_field_dispatching:
            raise RuntimeError("run is unavailable during localized TimeField dispatch")
        if self._slider_dispatching:
            raise RuntimeError("run is unavailable during controlled Slider dispatch")
        self._ensure_open()
        if not math.isfinite(dt) or dt <= 0:
            raise ValueError("run requires positive finite dt")
        if after_frame is not None and not callable(after_frame):
            raise TypeError("after_frame must be callable")
        if getattr(self, "_running", False):
            raise RuntimeError("app is already running")
        try:
            runner = self._api.lib.cui_bridge_run
        except AttributeError as exc:
            raise RuntimeError("native callback runner unavailable in this bridge") from exc
        callback_type = C.CFUNCTYPE(C.c_int, C.c_void_p)
        runner.restype = C.c_int
        runner.argtypes = [C.c_void_p, callback_type, C.c_void_p]
        failure = []
        @callback_type
        def dispatch_frame(_user):
            try:
                self.dispatch()
                if after_frame is not None and self._ptr and not self._close_requested:
                    after_frame(self)
                return 0
            except BaseException as exc:
                failure.append((exc, exc.__traceback__))
                return 3  # Stop C first; foreign exceptions must never cross C.
        self._running = True
        try:
            status = runner(self._ptr, dispatch_frame, None)
        finally:
            self._running = False
        if failure:
            exc, traceback = failure[0]
            raise exc.with_traceback(traceback)
        if status != 2:  # CLOSED is normal runner termination.
            self._api.check(status)

    def request_close(self) -> None:
        self._ensure_open()
        self._api.check(self._api.request_close(self._ptr))
        self._close_requested = True

    def close(self) -> None:
        if self._ptr:
            if self._icons_active:
                self._ensure_icon_thread()
            if self._time_field_enabled:
                import threading
                if threading.get_ident() != self._time_field_owner_thread:
                    raise RuntimeError("localized TimeField App must close on its owner thread")
            if self._slider_enabled:
                import threading
                if threading.get_ident() != self._slider_owner_thread:
                    raise RuntimeError("controlled Slider App must close on its owner thread")
            self._api.destroy(self._ptr)
            self._ptr = None
            for node in self._by_handle.values():
                node._app = node._handle = None
                node._disposed = True
                if isinstance(node, SliderModel):
                    node._request_callback = None
                if isinstance(node, LocalizedTimeField):
                    node._time_callback = None
                if isinstance(node, (ConfiguredIcon, ConfiguredBadge)):
                    node._release_icon_config()
                if isinstance(node, Rating):
                    node._rating_config = None
            self._by_handle.clear()

    def __enter__(self) -> "App":
        return self

    def __exit__(self, *_exc) -> None:
        self.close()

    def __del__(self):
        if getattr(self, "_ptr", None):
            self.close()


__all__ = ["App", "Widget", "Event", "BridgeError", "row", "column", "label", "button", "checkbox", "radio", "switch", "toggle", "slider", "progress", "badge", "input", "textarea", "number_input", "collapsible", "alert", "radio_group", "tabs", "accordion", "item", "select", "combobox", "dropdown_button"]

__all__ += ['dialog', 'alert_dialog', 'sheet', 'popover', 'tooltip', 'hover_card', 'avatar', 'bubble', 'empty_view', 'group_box', 'kbd', 'skeleton', 'spinner', 'tag', 'marker', 'shimmer', 'list_view', 'menu', 'pagination', 'rating', 'stepper']

__all__ += ["description_list", "scrollable"]

__all__ += ["status_bar", "toolbar", "title_bar", "icon", "message"]

__all__ += ["attachment", "image", "tree", "time_field", "color_picker", "calendar", "date_picker", "otp_input"]

__all__ += ["form", "input_group", "notification"]

__all__ += ["focus_trap"]

__all__ += ["message_scroller", "carousel", "resizable", "text_view", "editor"]

__all__ += ["chart", "table", "data_table", "DataTableLoadMore", "plot"]

__all__ += ["command", "virtual_list"]

__all__ += ["sidebar"]

__all__ += ["root_view"]

__all__ += ["speech"]

__all__ += ["questionnaire"]

__all__ += ["dock"]


def settings(key: str, text: str = "") -> Widget:
    return Widget("settings", key, text)

__all__ += ["settings"]

__all__ += ["ValueNotificationV1", "ScalarChangeNotificationV1", "TextChangeNotificationV1",
            "SelectionChangeNotificationV1"]

__all__ += ["DesignSize", "RadioPaint", "RadioPaintLayer"]

__all__ += ["BoxShadow", "CSSBoxShadow", "ShadowStyle", "PresetShadowStyle", "ShadowKind", "ShadowPreset", "CornerRadii"]

__all__ += ["ShadowPart", "ShadowTargetInfo"]

from .slider_model import (SliderSingle, SliderRange, SliderDomain, SliderConfig, SliderScale,
    SliderOrientation, SliderLayout, SliderPhase, SliderReason, SliderPart, SliderReceipt,
    SliderInput, SliderRequest, SliderPresentation, SliderModel, slider_model)
from .slider_model import __all__ as _slider_exports
__all__ += _slider_exports

from .time_field_localized import (TimeValue, TimeCycle, TimePrecision, TimeTextStatus, TimeLocale,
    TimeFieldConfig, TimeFieldChange, TimeFieldResult, LocalizedTimeField, localized_time_field)
from .time_field_localized import __all__ as _time_field_exports
__all__ += _time_field_exports

from .skeleton_presentation import (SkeletonWidth, SkeletonConfig, SkeletonPresentation,
                                    ResponsiveSkeleton, responsive_skeleton)
from .skeleton_presentation import __all__ as _skeleton_exports
__all__ += _skeleton_exports

from .pagination_options import Pagination, PaginationConfig
from .rating_options import Rating, RatingConfig
__all__ += ["Pagination", "PaginationConfig", "Rating", "RatingConfig"]

from .icons import (BadgeConfig, BadgeResult, BadgeSize, BadgeVariant, ConfiguredBadge,
    ConfiguredIcon, IconAlign, IconAvailability, IconBitmap, IconCatalogEntry, IconConfig,
    IconConstraints, IconError, IconInfo, IconLimits, IconOptions, IconResource, IconResult,
    IconSize, IconSource, IconSourceKind, IconStyle, configured_badge, configured_icon,
    icon_rotation)
from .icons import __all__ as _icon_exports
__all__ += _icon_exports

from .disclosure import DisclosureConfig, DisclosureTriggerStyle
__all__ += ["DisclosureConfig", "DisclosureTriggerStyle"]

from .disclosure import DisclosureHeadingLayer, DisclosureHeading, DisclosureHeadingBox, DisclosureTypography, DisclosureDivider

__all__ += ["DisclosureHeadingLayer", "DisclosureHeading", "DisclosureHeadingBox", "DisclosureTypography", "DisclosureDivider"]

from .disclosure import DisclosureTitleState, DisclosureTitleStyle
__all__ += ["DisclosureTitleState", "DisclosureTitleStyle"]
