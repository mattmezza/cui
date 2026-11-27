"""Optional retained Popover placement and interaction options."""
import ctypes as C
from . import Widget, _Handle

BOOLS = ("arrow", "hide_close", "keep_pointer_focus", "focus_content", "no_escape_dismiss", "no_outside_dismiss")
class _Config(C.Structure):
    _fields_ = [("anchor", C.c_uint32), ("flags", C.c_uint32)] + [(n, C.c_float) for n in ("width", "height", "margin", "offset", "reference_rem")] + [("reserved", C.c_uint32)]

def bind_api(api, bind):
    api.popover_options = (bind("popover_configure_v1", C.c_int, C.c_void_p, _Handle, C.c_uint32, C.POINTER(_Config))
                           if hasattr(api.lib, "cui_bridge_popover_configure_v1") else None)

def supported(app):
    app._ensure_open()
    return app._api.popover_options is not None

def apply(app, node, wire):
    if not supported(app):
        raise RuntimeError("Popover options require a compatible bridge")
    app._api.check(app._api.popover_options(app._ptr, node._handle, 0 if wire is None else C.sizeof(wire),
                                          None if wire is None else C.byref(wire)))

def options(self, value=None, **kwargs):
    """Set copied options (dict or kwargs), or clear with None.

    anchor is 0 top-left, 1 top-center, 2 top-right, 3 bottom-left,
    4 bottom-center, 5 bottom-right, 6 left-center, 7 right-center.
    Dimensions/margin are 0..4096; offset is -4096..4096 and its
    presence selects custom spacing. reference_rem is 0..256 (0 uses 16).
    Zero dimensions preserve component defaults. Requires the optional API.
    """
    if self.kind != "popover":
        raise TypeError("popover_options requires Popover")
    if self._disposed:
        raise RuntimeError("Widget belongs to a disposed App")
    if value is not None and (not isinstance(value, dict) or kwargs):
        raise TypeError("expected options dict or keyword arguments")
    config = value if value is not None else kwargs if kwargs else None
    wire = None
    if config is not None:
        config = dict(config)
        allowed = set(BOOLS) | {"anchor", "width", "height", "margin", "offset", "reference_rem"}
        if config.keys() - allowed:
            raise TypeError("unknown Popover option")
        anchor = config.get("anchor", 0)
        if type(anchor) is not int or not 0 <= anchor <= 7:
            raise ValueError("anchor must be an integer in 0..7")
        flags = 64 if "offset" in config else 0
        for i, name in enumerate(BOOLS):
            v = config.get(name, False)
            if type(v) is not bool:
                raise TypeError(name + " must be bool")
            flags |= int(v) << i
        numbers = []
        for name in ("width", "height", "margin", "offset", "reference_rem"):
            v = config.get(name, 0)
            lo, hi = (-4096 if name == "offset" else 0), (256 if name == "reference_rem" else 4096)
            if type(v) not in (int, float) or not lo <= v <= hi:
                raise ValueError(name + " out of range")
            numbers.append(v)
        wire = _Config(anchor, flags, *numbers, 0)
    if self._app is not None:
        apply(self._app, self, wire)
    self._popover_config = wire
    return self

def preflight(node, app):
    if getattr(node, "_popover_config", None) is not None and not supported(app):
        raise RuntimeError("Popover options require a compatible bridge")

def mount(node):
    wire = getattr(node, "_popover_config", None)
    if wire is not None:
        apply(node._app, node, wire)

Widget.popover_options = options
