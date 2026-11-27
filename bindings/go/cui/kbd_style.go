package cui

import (
	"errors"
	"math"
)

var ErrKbdStyleUnavailable = errors.New("Kbd style requires cui_bridge_set_kbd_style_v1")

type KbdWeight uint32

const (
	KbdNormal KbdWeight = iota
	KbdMedium
	KbdSemibold
	KbdBold
)

type KbdPadding struct{ X, Y float32 }
type KbdBorder struct {
	Width float32
	Color uint32
}
type KbdGradientStop struct {
	Position float32
	Color    uint32
}
type KbdTextGradient struct {
	X0, Y0, X1, Y1 float32
	Stops          []KbdGradientStop
}

// KbdStyleOptions replaces the complete retained style. Colors are 0xRRGGBBAA.
// Nil fields inherit defaults. Plain typed hints ignore style.
type KbdStyleOptions struct {
	MaxWidth, Radius, FontSize, LineHeight *float32
	Padding                                *KbdPadding
	Background, TextColor                  *uint32
	Border                                 *KbdBorder
	Weight                                 *KbdWeight
	Fill, Wrap, Italic, Monospace          bool
	TextGradient                           *KbdTextGradient
}

func validKbdNumber(v float32) bool {
	return !math.IsNaN(float64(v)) && !math.IsInf(float64(v), 0) && v >= 0
}
func (s KbdStyleOptions) validate() error {
	bad := errors.New("invalid Kbd style")
	for _, field := range []struct {
		value    *float32
		max      float32
		positive bool
	}{
		{s.MaxWidth, 4096, true}, {s.Radius, 4096, false},
		{s.FontSize, 512, true}, {s.LineHeight, 4096, false},
	} {
		if field.value != nil && (!validKbdNumber(*field.value) ||
			*field.value > field.max || field.positive && *field.value == 0) {
			return bad
		}
	}
	if s.Padding != nil && (!validKbdNumber(s.Padding.X) || s.Padding.X > 4096 ||
		!validKbdNumber(s.Padding.Y) || s.Padding.Y > 4096) {
		return bad
	}
	if s.Border != nil && (!validKbdNumber(s.Border.Width) || s.Border.Width > 4096) ||
		s.Weight != nil && *s.Weight > KbdBold {
		return bad
	}
	if g := s.TextGradient; g != nil {
		if s.TextColor != nil || len(g.Stops) < 2 || len(g.Stops) > 8 || g.X0 == g.X1 && g.Y0 == g.Y1 {
			return bad
		}
		for _, v := range []float32{g.X0, g.Y0, g.X1, g.Y1} {
			if !validKbdNumber(v) || v > 1 {
				return bad
			}
		}
		var last float32
		for i, v := range g.Stops {
			if !validKbdNumber(v.Position) || v.Position > 1 || i > 0 && v.Position < last {
				return bad
			}
			last = v.Position
		}
	}
	return nil
}

// ConfigureKbdStyle copies the complete style; failed updates preserve prior state.
func (n *Node) ConfigureKbdStyle(s KbdStyleOptions) error {
	if n == nil || n.kind != 34 || n.removed {
		return errors.New("style requires a live Kbd")
	}
	if err := s.validate(); err != nil {
		return err
	}
	if s == (KbdStyleOptions{}) {
		return n.ClearKbdStyle()
	}
	wire := makeKbdStyleWire(s)
	if n.app != nil {
		if err := n.setKbdStyle(wire); err != nil {
			return err
		}
	}
	n.kbdStyle = wire
	return nil
}

// KbdStyle is the fluent form of ConfigureKbdStyle.
func (n *Node) KbdStyle(s KbdStyleOptions) *Node {
	if n.err == nil {
		n.err = n.ConfigureKbdStyle(s)
	}
	return n
}

// ClearKbdStyle restores defaults, dropping the detached capability requirement.
func (n *Node) ClearKbdStyle() error {
	if n == nil || n.kind != 34 || n.removed {
		return errors.New("style requires a live Kbd")
	}
	if n.kbdStyle == nil {
		return nil
	}
	if n.app != nil {
		if err := n.setKbdStyle(nil); err != nil {
			return err
		}
	}
	n.kbdStyle = nil
	return nil
}
