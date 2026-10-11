# Component workbench

Run the built `cui_gallery` executable for the native gallery. Select Controls,
Selection & values, Text input, or About this build. The sidebar switches light/dark theme,
high contrast and disabled examples. Controls work with primary pointer clicks,
Tab/Shift+Tab, Enter/Space and slider arrow keys.

The gallery demonstrates the current implemented slice rather than every entry
in the 78-component target. It uses actual CUI components and the same base
behavior as headless tests. It does not pretend incomplete catalog entries have
working stories.

```sh
cui_gallery --smoke 3 --screenshot gallery.bmp
cui_gallery --headless --screenshot gallery.ppm
cui_gallery --headless --dark --screenshot gallery-dark.ppm
cui_gallery --headless --section values --width 680 --height 740 --screenshot values.ppm
cui_gallery --headless --section text --screenshot text.ppm
cui_gallery --font /path/to/font.ttf
```

Native captures are BMP; headless captures are PPM. `--smoke N` exits after N
rendered native frames. A font may also be selected with `CUI_FONT_PATH`; default
search paths cover common Linux, macOS and Windows installs without bundling a
font. The native loop redraws on events and application state changes and sleeps
briefly while idle.

The text input preview has actual scalar-based editing and undo/redo, but does
not yet paint a caret or selection, position a caret with pointer input, or
support IME composition. Tiny windows clip content; scrolling remains planned.
