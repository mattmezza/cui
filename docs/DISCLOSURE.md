# Disclosure controls

`cui_disclosure.h` provides unstyled trigger behavior and styled Accordion and
Collapsible containers. The caller owns expanded booleans. No control allocates
persistent state. Trigger IDs must remain unique and stable across frames.

```c
cui_disclosure_item sections[] = {
    {11, cui_string("Account"), false},
    {12, cui_string("Advanced"), false},
};
bool open[] = {true, false}; /* keep in application state */
cui_accordion(ui, 10, &theme, sections, 2, open,
    (cui_accordion_options){.collapsible = true, .bordered = true},
    draw_section, application);
```

`draw_section(cui_context *, size_t index, void *)` runs synchronously only for
expanded panels. It can submit any CUI children, including nested accordions.
Children of collapsed panels are omitted from rendering and focus order.
Collapsible uses the same callback shape with index zero.

Accordion supports single or multiple expanded sections, optional collapse of the
last expanded section, group and individual disabled states, and optional borders.
Click, Enter and released Space activate a trigger. Up/Down wrap among enabled
triggers; Home/End jump to the first/last enabled trigger. Arrow traversal moves
focus without opening panels. The return value reports controlled state changes.
Applications can also change the booleans themselves between frames.

The base functions submit trigger semantics without painting. Custom designs can
call them and then submit boxes with matching trigger IDs. The styled functions
use theme colors, padding, gap, radius and control height; themes provide density
and high contrast without duplicating behavior. Trigger semantics expose
expanded state and the controlled panel ID. Mounted panels expose their trigger
relationship; an Accordion group supplies a semantic parent. Native accessibility bridges are separate
unfinished infrastructure.

The implementation derives panel IDs by XORing the trigger ID with
`0x1d8f322539deca61`; Collapsible derives its outer container using
`0xb7c1c9d453922783`. Reserve those IDs when composing children. Duplicate IDs
are diagnosed by the core rather than silently reused.

`tests/unit/test_disclosure.c` exercises all four supplied themes, actual pointer
clicks, Enter and Space, disabled triggers, single/multiple expansion, required
expansion, keyboard traversal, and removal of focused hidden content. Portable
C11 and renderer-independent draw lists are supported; OS accessibility is not
claimed. Compared with GPUI Kit 0.7.1, animated measured reveal, custom styled
header parts/icons, and a demonstrated native/browser gallery remain unfinished.
Immediate mount/unmount behavior is implemented. Full family parity is not yet
verified.
