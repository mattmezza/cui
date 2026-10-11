# Gallery design

Mode: operate. The workbench follows a native developer tool: one navigation
column, a direct specimen area and a quiet footer. It uses the component theme
tokens rather than painted examples that bypass their behavior.

The original surface has a restrained dark slate / light paper palette, a single
blue action accent and Noto Sans on the verified Linux install. Appearance is
host configurable through the font path and runtime theme switches. Typography
steps are 24 px headings, 18 px subsection titles, 16 px controls and 12–13 px
context. The main composition uses 24 px outer padding, a 32 px navigation gap,
18 px content spacing and 10–12 px control groups.

Below 800 logical pixels the sidebar collapses into an inline section selector
and theme switch. Pointer, keyboard focus, disabled state, controlled values and
high contrast come from CUI Base and the component theme; the gallery does not
implement a second interaction model.

Light and dark real software captures and an Xorg native capture were inspected.
The bounded verification found two material issues: disabled button contrast
and content separator width. Both were corrected in the components worker, then
the dark capture confirmed the disabled surface and full-width separator. Future
work includes full catalog navigation/search, scrolling for very small windows,
native composition and font-scale/DPI validation.
