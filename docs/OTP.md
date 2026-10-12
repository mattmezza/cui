# OtpInput

CUI implements the frozen 0.7.1 numeric OTP interaction and grouped presentation
with portable application-owned state. `cui_otp_init` borrows a valid
NUL-terminated buffer and a target of at most 256 cells. There is no per-frame
allocation. The buffer remains valid while state is used; only the OTP API should
mutate it. `cui_otp_set_value` validates UTF-8/capacity transactionally and preserves
arbitrary or overlength programmatic values, matching the upstream compatibility
contract. It does not emit user editing notifications; invalidate your context
after programmatic changes.

`cui_base_otp` submits a single accessible textbox and reduces actual routed
pointer/keyboard input without theme painting. ASCII digit and fullwidth digit
text events append one normalized digit. Other text and arrow/delete keys are
ignored. Each text event uses its first scalar, matching the reference's per-key
editor; bulk clipboard paste is not an OTP feature. Backspace removes the final
UTF-8 scalar, including arbitrary programmatic values. Typing stops at the target
length; storage exhaustion reports capacity without losing the prior value.
Disabled state suppresses editing and focus presentation.

The result distinguishes `changed`, `complete`, `focused`, and `blurred`, with
counts for accepted editing/completion events within one frame. This follows the
frozen source's `OtpEvent::Change` and `OtpEvent::Complete` distinction rather than
the website's older completion-only summary. Backspace is an accepted edit even
on an empty value, as in the source. Programmatic updates do not set these flags.
`cui_otp_value` and `cui_otp_count` expose the current borrowed text/scalar count.

`cui_otp_options_default()` requests two visual groups. Explicit zero clamps to
one, excessive group counts clamp to the target length, and an empty target uses
one empty group. Groups contain ceiling(target/groups) cells, with remaining
cells in later groups. Options select extra-small/small (24), medium (32), large
(44), or a custom cell size; custom pixel sizes default text to half that size.
Default within-group gap is four pixels and between-group gap is twenty. Masking
replaces every displayed scalar with `*`; draw commands and semantics do not
include the raw masked value. The focused cell is the next empty cell, or the
last cell for a full code. Focus-ring suppression is explicit.

Pass host monotonic `now_seconds` for cursor blink; the interval defaults to half
a second. Retained blink phase resets on focus and accepted editing. The result's
`next_wake_seconds` supplies an absolute deadline while an empty focused cell has
a visible caret. Schedule a host frame at that deadline. Visibility changes
invalidate the context, while unchanged frames stay idle. Filled/disabled inputs
do not request a caret wake. Window activation and native scheduling belong to
the host; this API does not pretend to provide an OS timer or IME bridge.

The styled `cui_otp` renders the same base state with theme colors, rounded square
cells, grouped gaps, focus border, and caret. The headless test links core, edit
(for shared UTF-8 validation), and OTP base only. Styled linkage adds OTP component
and a supplied theme; no separate input reducer is used.

Tests cover real pointer focus, ordered text/key events, ASCII/fullwidth digits,
rejection and target bounds, UTF-8 Backspace, completion/change/focus/blur counts,
programmatic arbitrary values, storage errors, disabled input, host-clock blink,
group clamping, all four themes/densities, exact cell/group geometry, and masking
without raw-value draw commands. Native accessibility bridge coverage remains
that of core, and OS OTP autofill is not implemented.
