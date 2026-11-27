"""Optional retained Dialog and AlertDialog configuration."""
import ctypes as C
from typing import Literal, TypedDict

from . import Widget, _Handle, _utf8

DialogSlotRole = Literal['header', 'title', 'description', 'content', 'body', 'footer']
DialogAction = Literal['none', 'confirm', 'cancel', 'close']


class DialogOptions(TypedDict, total=False):
    centered: bool
    no_escape_dismiss: bool
    no_outside_dismiss: bool
    hide_close: bool
    show_close: bool
    outside_dismiss: bool
    width: float
    height: float
    margin: float


_KINDS = ('dialog', 'alert_dialog')
_FLAGS = ('centered', 'no_escape_dismiss', 'no_outside_dismiss', 'hide_close',
          'show_close', 'outside_dismiss')
_DIMENSIONS = ('width', 'height', 'margin')
_ROLES = {name: index for index, name in enumerate(
    ('header', 'title', 'description', 'content', 'body', 'footer'), start=1)}
_ACTIONS = {name: index for index, name in enumerate(('none', 'confirm', 'cancel', 'close'))}
_FOOTER = _ROLES['footer']


class _Config(C.Structure):
    _fields_ = [('flags', C.c_uint32), ('reserved', C.c_uint32),
                ('width', C.c_float), ('height', C.c_float), ('margin', C.c_float)]


_OPERATIONS = (
    ('dialog_configure', 'dialog_configure_v1', (C.c_uint32, C.POINTER(_Config))),
    ('dialog_slot', 'dialog_slot_create_v1', (C.c_uint32, C.c_char_p, C.POINTER(_Handle))),
    ('dialog_action', 'dialog_set_action_v1', (C.c_uint32,)),
)


def bind_api(api, bind):
    for name, symbol, args in _OPERATIONS:
        function = None
        if hasattr(api.lib, 'cui_bridge_' + symbol):
            function = bind(symbol, C.c_int, C.c_void_p, _Handle, *args)
        setattr(api, name, function)


def supported(app):
    app._ensure_open()
    return all(getattr(app._api, name) is not None for name, _, _ in _OPERATIONS)


def _require(app):
    if not supported(app):
        raise RuntimeError('Dialog composition requires a compatible bridge')


def _wire(kind, config):
    if config.keys() - set((*_FLAGS, *_DIMENSIONS)):
        raise TypeError('unknown Dialog option')
    flags = 0
    for bit, name in enumerate(_FLAGS):
        enabled = config.get(name, False)
        if type(enabled) is not bool:
            raise TypeError(f'{name} must be bool')
        flags |= int(enabled) << bit
    if kind == 'dialog' and (config.get('show_close') or config.get('outside_dismiss')):
        raise ValueError('show_close and outside_dismiss require AlertDialog')
    numbers = []
    for name in _DIMENSIONS:
        number = config.get(name, 0)
        if type(number) not in (int, float) or not 0 <= number <= 4096:
            raise ValueError(f'{name} out of range')
        numbers.append(number)
    return _Config(flags, 0, *numbers)


def _configure(node, config):
    app = node._app
    size, pointer = (0, None) if config is None else (C.sizeof(config), C.byref(config))
    app._api.check(app._api.dialog_configure(app._ptr, node._handle, size, pointer))


def options(node, value, kwargs):
    if node.kind not in _KINDS:
        raise TypeError('dialog_options requires Dialog or AlertDialog')
    if node._disposed:
        raise RuntimeError('Widget belongs to a disposed App')
    if value is not None and (not isinstance(value, dict) or kwargs):
        raise TypeError('expected options dict or keywords')
    config = value if value is not None else kwargs or None
    wire = _wire(node.kind, config) if config is not None else None
    if node._app:
        _require(node._app)
        _configure(node, wire)
    node._dialog_config = wire
    return node


def _set_action(node, value):
    app = node._app
    app._api.check(app._api.dialog_action(app._ptr, node._handle, value))


def action(node, name):
    if node.kind != 'button' or not isinstance(name, str) or name not in _ACTIONS:
        raise ValueError('dialog_action requires Button and a known action')
    if node._disposed:
        raise RuntimeError('Widget belongs to a disposed App')
    value = _ACTIONS[name]
    if node._app:
        _require(node._app)
        _set_action(node, value)
    node._dialog_action = value
    return node


def dialog_slot(key: str, role: DialogSlotRole) -> Widget:
    """Create one direct child container under a configured Dialog or AlertDialog."""
    if not isinstance(role, str) or role not in _ROLES:
        raise ValueError('unknown Dialog slot role')
    if not _utf8(key):
        raise ValueError('Dialog slot key must be nonempty')
    node = Widget('row' if role == 'footer' else 'column', key)
    node._dialog_slot = _ROLES[role]
    if node._dialog_slot == _FOOTER:
        node._gap = 8
    return node


def validate_children(owner, children):
    slots = [child._dialog_slot for child in (*owner._children, *children)
             if isinstance(child, Widget) and child._dialog_slot is not None]
    if slots and owner.kind not in _KINDS:
        raise ValueError('Dialog slots require a direct Dialog owner')
    if owner.kind == 'alert_dialog' and len(slots) != len(owner._children) + len(children):
        raise ValueError('AlertDialog children must be dialog slots')
    if len(slots) != len(set(slots)):
        raise ValueError('duplicate Dialog slot role')


def preflight(node, parent, app):
    if node._dialog_slot is not None and (parent is None or parent._dialog_config is None):
        raise ValueError('Dialog slots require a configured direct Dialog or AlertDialog parent')
    if node._dialog_action and (parent is None or parent._dialog_slot != _FOOTER):
        raise ValueError('Dialog actions require a direct Footer Button')
    if node._dialog_config is not None:
        _require(app)


def mount(node):
    if node._dialog_config is not None:
        _configure(node, node._dialog_config)
    if node._dialog_action:
        _set_action(node, node._dialog_action)
