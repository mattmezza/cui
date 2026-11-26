package cui

import (
	"errors"
	"strings"
	"unicode/utf8"
)

// ErrKbdStrokeUnavailable means the provider lacks typed shortcut presentation.
var ErrKbdStrokeUnavailable = errors.New("Kbd stroke requires cui_bridge_set_kbd_stroke_v1")

// KbdModifiers combines modifier keys using bitwise OR.
type KbdModifiers uint32

const (
	KbdShift KbdModifiers = 1 << iota
	KbdCtrl
	KbdAlt
	KbdMeta
)

// KbdStrokeOptions controls shortcut formatting. AccessibleName overrides the
// generated spoken label. Plain takes precedence over Outline.
type KbdStrokeOptions struct {
	Modifiers                  KbdModifiers
	Plain, Outline, MacSymbols bool
	AccessibleName             string
}
type kbdStrokeConfig struct {
	key     string
	options KbdStrokeOptions
}

// KbdStroke creates a typed shortcut. strokeKey is one nonempty UTF-8 key label
// of at most 128 bytes; AccessibleName is optional and at most 1024 bytes.
func KbdStroke(key, strokeKey string, options KbdStrokeOptions) *Node {
	n := Kbd(key, "")
	n.err = n.ConfigureKbdStroke(strokeKey, options)
	return n
}

// ConfigureKbdStroke replaces a shortcut's owned configuration. Failed updates
// preserve the previous configuration. Use on the app owner thread when mounted.
func (n *Node) ConfigureKbdStroke(key string, options KbdStrokeOptions) error {
	if n == nil || n.kind != 34 || n.removed {
		return errors.New("stroke requires a live Kbd")
	}
	if err := validateKbdStroke(key, options); err != nil {
		return err
	}
	c := kbdStrokeConfig{key, options}
	if n.app != nil {
		if err := n.setKbdStroke(c); err != nil {
			return err
		}
	}
	n.kbdStroke = &c
	return nil
}
func validateKbdStroke(key string, options KbdStrokeOptions) error {
	if key == "" || !validKbdLine(key, 128) || options.Modifiers & ^(KbdShift|KbdCtrl|KbdAlt|KbdMeta) != 0 ||
		!validKbdLine(options.AccessibleName, 1024) {
		return errors.New("invalid Kbd stroke key, modifiers or accessible name")
	}
	return nil
}

func validKbdLine(text string, limit int) bool {
	return len(text) <= limit && utf8.ValidString(text) &&
		strings.IndexFunc(text, func(r rune) bool {
			return r < 32 || r == 127 || r == 0x85 || r == 0x2028 || r == 0x2029
		}) < 0
}
