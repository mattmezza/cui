"""Headless retained defaults and compatibility; CUI_EXPECT_TEXT_DEFAULTS_UNSUPPORTED=1 for legacy."""
import os
from cui_bridge import App, column, row, label, icon, spinner
with App() as app:
    if os.environ.get('CUI_EXPECT_TEXT_DEFAULTS_UNSUPPORTED') == '1':
        assert not app.supports_text_defaults()
        try: app.mount(column().text_defaults(24))
        except RuntimeError as error: assert 'unavailable' in str(error)
        else: raise AssertionError('legacy provider accepted defaults')
        app.mount(column().children(label('Legacy still works'))).step()
    else:
        assert app.supports_text_defaults()
        inherited = icon('inherited', 'check')
        sibling = icon('sibling', 'check')
        nested = row().text_defaults(30, 0).children(inherited, label('Nested'), spinner('busy'))
        app.mount(column().text_defaults(20, 0x336699ff).children(nested, row().children(sibling)))
        app.step()
        assert app.bounds(inherited)[2] == 30
        assert app.bounds(sibling)[2] == 20
        nested.text_defaults()
        app.step()
        assert app.bounds(inherited)[2] == 20
        assert app.bounds(sibling)[2] == 20
        nested.text_defaults(16)
        app.step()
        assert app.bounds(inherited)[2] == 16
print('text defaults live tests passed')
