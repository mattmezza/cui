"""Nested retained text defaults; headless by default, --font FONT opens SDL."""
import argparse
from cui_bridge import App, column, row, icon, label, spinner

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--font')
args = parser.parse_args()
inner = row().text_defaults(tint=0xE09020FF).children(
    icon('nested-icon', 'check'), label('Inherited size, amber tint'), spinner('busy'))
root = column().text_defaults(24, 0x3060B0FF).children(
    row().children(icon('outer-icon', 'check'), label('Blue, 24 pixels')),
    inner,
    row().children(label('Sibling remains blue')))
with App(backend='sdl' if args.font else 'none', font_path=args.font) as app:
    app.mount(root)
    if args.font:
        app.run()
    else:
        app.step()
        inner.text_defaults(18, 0)  # Transparent is explicit, including on Icons and Spinners.
        app.step()
        inner.text_defaults()  # Remove both refinements and inherit the outer defaults.
        app.step()
        print('Nested text defaults mounted, updated and reset')
