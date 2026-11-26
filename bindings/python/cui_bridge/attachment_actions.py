"""Optional retained Attachment actions; no callbacks or second queue."""
import ctypes as C
from . import _Handle, _NotificationV1

def supported(app):
    app._ensure_open()
    lib = app._api.lib
    names = ("attachment_set_actions_v1", "notification_v1_size", "notification_versions",
             "notification_value_payloads_v1", "poll_value_notification_v1")
    if not all(hasattr(lib, "cui_bridge_" + name) for name in names):
        return False
    queries = [getattr(lib, "cui_bridge_" + name) for name in names[1:4]]
    for query in queries:
        query.argtypes = []
        query.restype = C.c_uint32
    return queries[0]() == C.sizeof(_NotificationV1) and bool(queries[1]() & 1) and bool(queries[2]() & 256)

def apply(node, mask):
    if not supported(node._app):
        raise RuntimeError("Attachment actions unavailable")
    fn = node._app._api.lib.cui_bridge_attachment_set_actions_v1
    fn.restype = C.c_int
    fn.argtypes = [C.c_void_p, _Handle, C.c_uint32]
    node._app._api.check(fn(node._app._ptr, node._handle, mask))

def configure(node, open, remove, retry):
    from .attachment_options import target
    target(node)
    if any(type(value) is not bool for value in (open, remove, retry)):
        raise TypeError("Attachment action choices must be bool")
    mask = int(open) | (int(remove) << 1) | (int(retry) << 2)
    if node._app:
        apply(node, mask)
    node._attachment_actions = mask or None
    return node

def preflight(node, app):
    if getattr(node, "_attachment_actions", None) is not None and not supported(app):
        raise RuntimeError("Attachment actions unavailable")

def mount(node):
    mask = getattr(node, "_attachment_actions", None)
    if mask is not None:
        apply(node, mask)
