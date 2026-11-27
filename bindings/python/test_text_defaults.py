"""Offline authoring and optional-provider compatibility checks."""
import ctypes as C
from types import SimpleNamespace
from cui_bridge import App, row, label, _Handle

node = row().text_defaults(24, 0)
assert node._text_defaults == (24, 0, 1)
assert node.text_defaults(tint=0x11223344) is node
assert node._text_defaults == (0, 0x11223344, 1)
for size in (float('nan'), float('inf'), -1, 4097, True, 1e-100):
    before = node._text_defaults
    try: node.text_defaults(size)
    except ValueError: pass
    else: raise AssertionError(size)
    assert node._text_defaults == before
try: label('no').text_defaults(20)
except TypeError: pass
else: raise AssertionError('Label accepted defaults')
app = App.__new__(App)
app._ptr = C.c_void_p(1)
app._icons_active = False
app._api = SimpleNamespace(set_text_defaults=None)
assert not app.supports_text_defaults()
node._app = app
try: node.text_defaults()
except RuntimeError as error: assert 'unavailable' in str(error)
else: raise AssertionError('older provider accepted defaults')
assert node._text_defaults == (0, 0x11223344, 1)
calls = []
app._api.set_text_defaults = lambda *values: calls.append(values) or 0
app._api.check = lambda status: None
node._handle = _Handle()
assert app.supports_text_defaults()
assert node.text_defaults() is node
assert calls[-1][2:] == (0, 0, 0)
assert node._text_defaults == (0, 0, 0)
app._ptr = None
print('text defaults offline tests passed')
