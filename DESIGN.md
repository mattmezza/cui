---
name: CUI native workbench
description: Restrained native controls with explicit state and host-owned themes.
colors:
  light-background: "#f6f8fa"
  light-surface: "#ffffff"
  light-text: "#18202a"
  light-muted: "#576370"
  light-primary: "#175ba9"
  light-on-primary: "#ffffff"
  light-border: "#b5bfcc"
  light-danger: "#b32327"
  light-focus: "#175ba9"
  dark-background: "#12171e"
  dark-surface: "#1e2630"
  dark-text: "#ebf0f6"
  dark-muted: "#a4b0be"
  dark-primary: "#74b2fa"
  dark-on-primary: "#0e1826"
  dark-border: "#667689"
  dark-danger: "#ff898e"
  dark-focus: "#a4d1ff"
  high-contrast-background: "#000000"
  high-contrast-surface: "#000000"
  high-contrast-text: "#ffffff"
  high-contrast-muted: "#ffffff"
  high-contrast-primary: "#ffff00"
  high-contrast-on-primary: "#000000"
  high-contrast-border: "#ffffff"
  high-contrast-danger: "#ff7878"
  high-contrast-focus: "#00ffff"
  terminal-background: "#1f1d17"
  terminal-surface: "#2d2a21"
  terminal-text: "#ffe3a4"
  terminal-muted: "#cdb68c"
  terminal-primary: "#ffc04d"
  terminal-on-primary: "#1f1d17"
  terminal-border: "#947e54"
  terminal-danger: "#ff8967"
  terminal-focus: "#ffe3a4"
typography:
  headline:
    fontFamily: "Noto Sans"
    fontSize: "24px"
  title:
    fontFamily: "Noto Sans"
    fontSize: "18px"
  body:
    fontFamily: "Noto Sans"
    fontSize: "16px"
  high-contrast-body:
    fontFamily: "Noto Sans"
    fontSize: "18px"
  context:
    fontFamily: "Noto Sans"
    fontSize: "13px"
  field-label:
    fontFamily: "Noto Sans"
    fontSize: "12.8px"
  guidance:
    fontFamily: "Noto Sans"
    fontSize: "12px"
  resize-guidance:
    fontFamily: "Noto Sans"
    fontSize: "14px"
rounded:
  control: "6px"
  square: "0px"
spacing:
  badge: "4px"
  terminal-gap: "6px"
  control-gap: "8px"
  compact-group: "10px"
  action-group: "12px"
  compact-inset: "16px"
  content-gap: "18px"
  workspace-inset: "24px"
  navigation-gap: "32px"
components:
  button-light:
    backgroundColor: "{colors.light-primary}"
    textColor: "{colors.light-on-primary}"
    rounded: "{rounded.control}"
    padding: "10px"
    height: "42px"
    typography: "{typography.body}"
  button-dark:
    backgroundColor: "{colors.dark-primary}"
    textColor: "{colors.dark-on-primary}"
    rounded: "{rounded.control}"
    padding: "10px"
    height: "42px"
    typography: "{typography.body}"
  button-high-contrast:
    backgroundColor: "{colors.high-contrast-primary}"
    textColor: "{colors.high-contrast-on-primary}"
    rounded: "{rounded.square}"
    padding: "10px"
    height: "46px"
    typography: "{typography.high-contrast-body}"
  button-terminal:
    backgroundColor: "{colors.terminal-primary}"
    textColor: "{colors.terminal-on-primary}"
    rounded: "{rounded.square}"
    padding: "12px"
    height: "44px"
    typography: "{typography.body}"
  button-light-disabled:
    backgroundColor: "{colors.light-surface}"
    textColor: "{colors.light-muted}"
    rounded: "{rounded.control}"
    padding: "10px"
    height: "42px"
  input-light:
    backgroundColor: "{colors.light-surface}"
    textColor: "{colors.light-text}"
    rounded: "{rounded.control}"
    padding: "10px"
    width: "280px"
  badge-light:
    backgroundColor: "{colors.light-primary}"
    textColor: "{colors.light-on-primary}"
    rounded: "{rounded.control}"
    padding: "{spacing.badge}"
    typography: "{typography.field-label}"
---
# Design System: CUI native workbench

## Overview

**Creative North Star: "The Native Toolkit Workbench"**

The native toolkit workbench uses quiet paper or slate surfaces, direct labels and a single theme accent. Its purpose is to make component behavior inspectable, with enough space to distinguish actions, preferences and values. CUI owns its visual identity; GPUI branding is not copied.

This documents the implemented development gallery and reusable themed primitives. It does not certify the full framework, the 78-entry target inventory, accessibility, all platforms or complete text editing. The terminal theme is a separate amber and ink interpretation of the same component behavior, not a gallery appearance switch.

**Key Characteristics:**

- Restrained native tool styling and explicit control states.
- Theme-owned presentation over shared CUI Base behavior.
- Readable keyboard guidance and honest development scope.

## Colors

The light theme pairs paper neutrals with a deep tool blue; dark uses slate and a clearer blue, high contrast uses black with yellow actions and cyan focus, and terminal uses ink with amber actions.

### Primary

The `primary` and `on-primary` pair carries enabled buttons, selected toggles, badges and progress fills. The gallery navigation currently uses ordinary primary buttons for every section; it has no distinct active-section visual token.

### Neutral

`background` defines the workspace; `surface` defines controls, toolbar and footer. `text` is normal content, `muted` is context and disabled content, and `border` separates controls and sections. `danger` reports an input error; `focus` marks the focused control. These are semantic roles within each palette, not interchangeable color swatches.

**The Theme Pair Rule.** Always take foreground and background from the same theme; do not transplant light-theme white action text onto dark-theme blue.

The frontmatter records opaque RGBA colors as lossless hex RGB; native `cui_color` channels are `uint8_t` and every theme alpha is 255. The authoritative palette implementation is `src/theme/cui_theme.c`.

## Typography

The verified Linux gallery uses Noto Sans Regular. The host may pass `--font FILE`, set `CUI_FONT_PATH`, or use the implemented DejaVu Sans/platform fallback paths. This is a runtime font choice, not a required bundled web-font stack. There is no separate display family, weight scale, tracking token or line-height token in the native theme.

Headlines use `headline`, subsections use `title`, ordinary controls use `body`, explanatory text uses `context`, and keyboard/footer hints use `guidance`. Field labels and badge text are 0.8 times the current theme font size; the frontmatter `field-label` is the light/dark/terminal instance. High contrast increases themed body text and scales those labels accordingly; gallery headings and explicit hints retain their fixed sizes. Theme dimensions, including `font_size`, are C `float` values in logical pixels, not CSS layout constraints.

## Layout

The normal gallery is a toolbar, a workspace with a 210-logical-pixel navigation column and expanding content, and a quiet footer. Standard outer insets use `workspace-inset`; column separation uses `navigation-gap`; specimen spacing uses `content-gap`; action rows use `action-group`. Toolbar heights are 84 normally and 64 compact; footer heights are 44 normally and 36 compact. Compact outer insets use `compact-inset` and content spacing uses `compact-group`.

Below 800 logical pixels wide, navigation becomes a row of short section buttons in the content area; the dark-theme switch moves there too. High-contrast and disable-example controls remain available in the wide sidebar only. Selection radio labels shorten and arrange in a row in compact mode. This is a gallery composition rule, not a universal framework breakpoint.

If width is below 600 **or** height is below 700, the gallery submits only a clipped resize notice. No specimen controls or focus targets are submitted. This is a bounded fallback, not small-window scrolling. Content separators use expanding width (`-1`), while the sidebar separator is 210; they must cover their container rather than retain the component default width.

## Elevation & Depth

The current gallery and themed primitives have no shadow or animation tokens. Surface tone, thin borders and spacing establish grouping. Interaction changes are immediate; the per-frame 0.016-second delta is not a motion-design token. Preserve the flat presentation until a concrete component needs a tested depth or motion behavior.

## Shapes

Light and dark controls use `control` corners; high contrast and terminal use `square` corners. The theme is the source of shape as well as color. The shared control border is one logical pixel, increasing to two for focus. Progress tracks are eight logical pixels tall and inherit the theme radius. The gallery's toolbar and workspace containers are unrounded.

## Components

### Buttons

Direct and compact. Enabled buttons use `primary`/`on-primary`; disabled buttons use `surface`/`muted` with a `muted` border. Hover scales each primary RGB channel by 0.9 and press by 0.75, truncating to `uint8_t`; alpha stays unchanged. Focus replaces the border with `focus` and increases its width. The sidecar previews translate this native appearance into HTML/CSS for inspection only.

The native theme struct dimension order is `font_size`, `padding`, `gap`, `radius`, `control_height`. Light and dark use `(16, 10, 8, 6, 42)`; high contrast uses `(18, 10, 10, 0, 46)`; terminal uses `(16, 12, 6, 0, 44)`. Each member is a `float`. Values describe the existing box layout; they do not promise CSS-equivalent vertical centering.

### Inputs / Fields

Surface-colored, bordered and 280 logical pixels wide, with theme padding and automatically measured height. The muted label is above the value. Input error changes the border to `danger`, then focus overrides it with `focus` if present; textual error feedback belongs beside the field. Caret and selection painting, IME preedit and grapheme-aware cursor movement remain planned, even though keyboard selection and bounded undo behavior exist.

### Selection Controls

Checkboxes, switches and radios currently use textual markers (`[x]`, `ON`/`OFF`, `(*)`) with theme text and surface colors. A selected toggle uses primary fill and on-primary text. Preserve shared CUI Base behavior and controlled application values rather than inventing independent gallery state handling.

### Badges and Progress

Badges are small primary/on-primary labels with `badge` padding and theme radius. They are status labels, not chip controls. Progress uses the border color as track and primary as fill; finite values clamp to the unit interval. A slider combines a label and 240-logical-pixel progress track inside a 260-logical-pixel surface control. There is no card component or animated slider thumb to document.

### Navigation and Keyboard Guidance

Navigation is a group of real buttons using the same primitives as specimen actions. Wide-mode help describes Tab / Shift+Tab focus, Enter / Space activation and arrows for values. Text specimens additionally explain Shift selection, Ctrl+A, Ctrl+Z and Ctrl+Shift+Z. Compact mode retains the footer's Tab hint. Do not infer a native accessibility bridge from this guidance; bridges remain planned.

## Do's and Don'ts

### Do:

- **Do** derive control colors and dimensions from the selected cui_theme.
- **Do** keep keyboard help close to the specimen and retain explicit preview limitations.
- **Do** use the compact layout below 800 logical pixels and the resize notice below the supported specimen size.
- **Do** preserve the original CUI identity and all 78 target manifest entries.

### Don't:

- **Don’t** copy GPUI branding or describe this vertical slice as complete parity.
- **Don’t** submit hidden specimen controls below the tiny-window threshold.
- **Don’t** imply that a dark capture proves high-contrast, terminal, DPI or accessibility validation.
- **Don’t** paint a second interaction model into the gallery.

Evidence: real software captures `docs/screenshots/gallery-light.png`, `gallery-text-dark.png`, `gallery-narrow.png` and `gallery-tiny.png`, plus `src/theme/cui_theme.c`, `src/components/cui_components.c` and `gallery/main.c`. The final visual review resolved disabled-action readability and narrow/tiny composition issues and returned SHIP strictly for this development gallery. It does not establish a contrast audit, native accessibility certification, cross-platform equivalence, DPI validation or full framework parity. Keep `gallery/DESIGN.md` as the nested surface history; current native source and this extracted system govern new work.
