# Component reference and full parity backlog

Baseline: GPUI Kit 0.7.1. This is an original functional breakdown informed by the upstream catalog and pinned source. It is not a claim of equivalent implementation. Read [machine-readable status](gpui-parity.json) for evidence and limitations.

Upstream reference documentation: https://gpui-kit.com/component/ (GPUI Kit, CC BY 4.0). No upstream prose or illustrations reproduced here.

## Accordion

Reference: https://gpui-kit.com/component/accordion/

- single/multiple expanded items
- controlled state and disabled items
- keyboard focus and disclosure semantics

## AlertDialog

Reference: https://gpui-kit.com/component/alert-dialog/

- modal focus trap and restore
- confirm/cancel actions
- dismissal policies and semantics

## Alert

Reference: https://gpui-kit.com/component/alert/

- severity variants
- title/content/action slots
- accessible announcement

## Attachment

Reference: https://gpui-kit.com/component/attachment/

- file/media metadata
- preview/remove actions
- loading/error states

## Avatar

Reference: https://gpui-kit.com/component/avatar/

- image source and fallback initials
- size variants
- image failures and accessible name

## Badge

Reference: https://gpui-kit.com/component/badge/

- count and dot variants
- overflow count
- theme and accessible content

## Bubble

Reference: https://gpui-kit.com/component/bubble/

- incoming/outgoing alignment
- message/reaction slots
- theme variants

## Button

Reference: https://gpui-kit.com/component/button/

- pointer press/release/capture
- keyboard activation and focus
- disabled/loading/error states
- variants/sizes/icons/slots

## Checkbox

Reference: https://gpui-kit.com/component/checkbox/

- unchecked/checked/mixed state
- controlled value and activation
- disabled/keyboard/semantic state

## Clipboard

Reference: https://gpui-kit.com/component/clipboard/

- OS read/write adapter
- capability failure handling
- headless adapter tests

## Collapsible

Reference: https://gpui-kit.com/component/collapsible/

- controlled disclosure
- keyboard trigger
- semantic expanded relation

## DropdownButton

Reference: https://gpui-kit.com/component/dropdown-button/

- split primary and menu actions
- keyboard opening and focus restore
- disabled/loading state

## Empty

Reference: https://gpui-kit.com/component/empty/

- title/description/media/action slots
- themed layout
- accessible content

## Focus Trap

Reference: https://gpui-kit.com/component/focus-trap/

- tab and reverse cycling
- nested scope and restore
- disabled/removed target cleanup

## GroupBox

Reference: https://gpui-kit.com/component/group-box/

- title/content slots
- themed border/layout
- group semantics

## HoverCard

Reference: https://gpui-kit.com/component/hover-card/

- hover/focus delay
- popup placement/collision
- dismissal and accessible relationship

## Icon

Reference: https://gpui-kit.com/component/icon/

- original/licensed vector set
- size/stroke/theme
- accessible/decorative distinction

## Image

Reference: https://gpui-kit.com/component/image/

- host resource lifecycle
- scaling/clipping/fallback
- loading/error/accessibility

## Kbd

Reference: https://gpui-kit.com/component/kbd/

- platform key labels
- grouped shortcut rendering
- accessible description

## Label

Reference: https://gpui-kit.com/component/label/

- UTF-8 text and theme size
- control association
- disabled/error presentation

## Marker

Reference: https://gpui-kit.com/component/marker/

- status/separator modes
- semantic content
- themed styling

## Message

Reference: https://gpui-kit.com/component/message/

- sender/time/content slots
- attachments and reactions
- accessibility and alignment

## MessageScroller

Reference: https://gpui-kit.com/component/message-scroller/

- variable-height virtualization
- stable keys and incremental updates
- tail following/scroll restore

## Notification

Reference: https://gpui-kit.com/component/notification/

- toast queue/lifetimes
- actions/dismissal
- accessible live region

## Pagination

Reference: https://gpui-kit.com/component/pagination/

- controlled page bounds
- keyboard and disabled actions
- page-count/ellipsis presentation

## Progress

Reference: https://gpui-kit.com/component/progress/

- determinate and indeterminate
- value semantics
- theme variants and reduced motion

## Radio

Reference: https://gpui-kit.com/component/radio/

- group exclusivity and roving focus
- controlled selection
- disabled/keyboard semantics

## Rating

Reference: https://gpui-kit.com/component/rating/

- pointer and keyboard value
- read-only/disabled states
- fractional values and semantics

## Shimmer

Reference: https://gpui-kit.com/component/shimmer/

- animation scheduling
- reduced-motion fallback
- loading region semantics

## Skeleton

Reference: https://gpui-kit.com/component/skeleton/

- placeholder sizes/layout
- loading semantics
- theme/motion states

## Spinner

Reference: https://gpui-kit.com/component/spinner/

- animation scheduling
- sizes and theme
- reduced motion and status semantics

## Stepper

Reference: https://gpui-kit.com/component/stepper/

- steps/current/completed state
- navigation/validation
- keyboard and semantic progress

## Switch

Reference: https://gpui-kit.com/component/switch/

- controlled binary activation
- keyboard/pointer/disabled behavior
- semantic checked state and theme

## Tag

Reference: https://gpui-kit.com/component/tag/

- semantic/custom/named palettes, outlines, sizes and radius
- passive accessible text and arbitrary rich children
- hover opacity and inherited theme styling

## Toggle

Reference: https://gpui-kit.com/component/toggle/

- controlled pressed state
- keyboard/pointer/disabled behavior
- pressed semantics/theme variants

## Tooltip

Reference: https://gpui-kit.com/component/tooltip/

- hover/focus timing
- popup collision
- dismissal and accessible description

## Calendar

Reference: https://gpui-kit.com/component/calendar/

- date/leap-year/month geometry
- min/max bounds and range selection
- keyboard/localized semantics

## ColorPicker

Reference: https://gpui-kit.com/component/color-picker/

- color-space model and conversion
- picker geometry and pointer drag
- keyboard/validation/theme

## Combobox

Reference: https://gpui-kit.com/component/combobox/

- filter/single/multiple selection
- active descendant and keyboard
- popup focus/dismissal/accessibility

## DatePicker

Reference: https://gpui-kit.com/component/date-picker/

- calendar popup composition
- parse/format and bounds
- keyboard/localization/accessibility

## Editor

Reference: https://gpui-kit.com/component/editor/

- UTF-8 grapheme editing and selection
- undo/redo and multiline indentation
- gutter/wrap/find/replace
- highlight/diagnostic/completion callbacks
- 200K-line benchmark

## Form

Reference: https://gpui-kit.com/component/form/

- field registration and validation
- error/required labels
- submit/reset and keyboard

## Input

Reference: https://gpui-kit.com/component/input/

- UTF-8 insertion/deletion/selection
- clipboard and IME
- undo/redo and grapheme boundaries
- validation/disabled/error/semantic states

## Input Group

Reference: https://gpui-kit.com/component/input-group/

- prefix/suffix/button slots
- focus and disabled propagation
- validation and layout

## NumberInput

Reference: https://gpui-kit.com/component/number-input/

- numeric parsing and bounds
- increment/decrement/step
- keyboard/disabled/semantic value

## OtpInput

Reference: https://gpui-kit.com/component/otp-input/

- segmented input focus
- paste and validation
- keyboard/accessible labels

## Questionnaire

Reference: https://gpui-kit.com/component/questionnaire/

- step and answer models
- validation/navigation
- compound UI semantics

## Select

Reference: https://gpui-kit.com/component/select/

- single/multiple controlled selection
- keyboard active descendant
- popup dismissal/focus restoration

## Slider

Reference: https://gpui-kit.com/component/slider/

- pointer drag capture
- keyboard range/step/clamp
- controlled value and semantics
- disabled/theme variants

## Speech

Reference: https://gpui-kit.com/component/speech/

- capability/permission adapter
- recognition callback lifecycle
- fake adapter tests and explicit unavailability

## Textarea

Reference: https://gpui-kit.com/component/textarea/

- multiline editing/selection
- wrap and auto-grow
- clipboard/IME/undo/semantics

## TimeField

Reference: https://gpui-kit.com/component/time-field/

- segmented hour/minute/second model
- parse/validation/localization
- keyboard/semantic segments

## Carousel

Reference: https://gpui-kit.com/component/carousel/

- controlled current item
- keyboard/drag navigation
- accessible position and theme

## DescriptionList

Reference: https://gpui-kit.com/component/description-list/

- key/value composition
- responsive layout
- list semantics

## Dialog

Reference: https://gpui-kit.com/component/dialog/

- modal overlay/dismissal
- focus trap/restore
- title/description/action slots

## Dock

Reference: https://gpui-kit.com/component/dock/

- nested splits and tab groups
- drag reorder/resize/edge docks
- versioned serialization/restore
- focus and keyboard behavior

## List

Reference: https://gpui-kit.com/component/list/

- selection and item keys
- keyboard navigation
- item rendering and semantics

## Menu

Reference: https://gpui-kit.com/component/menu/

- roving item focus/actions
- nested popup collision
- dismissal/restore/disabled semantics

## Popover

Reference: https://gpui-kit.com/component/popover/

- anchor placement/collision
- controlled open/dismissal
- focus and accessible relationship

## Resizable

Reference: https://gpui-kit.com/component/resizable/

- split geometry/min/max
- pointer drag and keyboard resize
- semantic separator and persistence

## Root View

Reference: https://gpui-kit.com/component/root/

- theme/overlay/notification providers
- window lifecycle and focus scopes
- safe area/scaling

## Scrollable

Reference: https://gpui-kit.com/component/scrollable/

- scroll bounds and nested clipping
- wheel/drag scrollbar
- keyboard and coordinate hit testing

## Settings

Reference: https://gpui-kit.com/component/settings/

- grouped settings models
- validation and persistence hooks
- keyboard/accessibility composition

## Sheet

Reference: https://gpui-kit.com/component/sheet/

- edge placement and transition
- focus trap/restore/dismissal
- theme/reduced motion

## Sidebar

Reference: https://gpui-kit.com/component/sidebar/

- hierarchical navigation
- collapsed/selected/disabled state
- keyboard and accessible navigation

## StatusBar

Reference: https://gpui-kit.com/component/status-bar/

- left/center/right slots
- density/theme variants
- accessible status

## Tabs

Reference: https://gpui-kit.com/component/tabs/

- controlled selected tab
- roving keyboard focus
- tab/panel semantic relationship

## Theme

Reference: https://gpui-kit.com/component/theme/

- runtime light/dark and tokens
- per-component overrides/density
- system preference/high contrast/reduced motion

## TitleBar

Reference: https://gpui-kit.com/component/title-bar/

- window controls and host chrome
- drag/maximize integration
- platform semantics/theme

## Toolbar

Reference: https://gpui-kit.com/component/toolbar/

- left/center/right slots
- keyboard group navigation
- size/theme variants

## Tree

Reference: https://gpui-kit.com/component/tree/

- hierarchical model/expand/selection
- keyboard parent/child navigation
- virtualization and tree semantics

## VirtualList

Reference: https://gpui-kit.com/component/virtual-list/

- variable-height efficient offsets
- stable keyed incremental items
- viewport overscan and scroll restore
- 1K/10K/100K benchmark

## Chart

Reference: https://gpui-kit.com/component/chart/

- line/bar/area/pie/candlestick series
- axes/scales/ticks/legend
- hover and accessible data
- theme and headless models

## Command

Reference: https://gpui-kit.com/component/command/

- fuzzy filtering and action model
- keyboard navigation and focus restore
- shortcut contexts and dismissal

## DataTable

Reference: https://gpui-kit.com/component/data-table/

- column models/sort/resize/pinning
- cell/row selection keyboard and mouse
- row virtualization/remote callbacks
- header/footer/table semantics

## Plot

Reference: https://gpui-kit.com/component/plot/

- linear/log/category/time scales
- axes/ticks/series/legend
- zoom/hover/selection
- headless geometry and themed output

## Table

Reference: https://gpui-kit.com/component/table/

- semantic row/column/header/footer
- selection/sorting/resizing
- themed cells and keyboard behavior

## TextView

Reference: https://gpui-kit.com/component/text-view/

- safe Markdown/HTML subset parser
- Unicode text selection and links
- inline code/lists/tables
- no script execution and parser fuzzing
