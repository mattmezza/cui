package cui

import (
	"errors"
	"strings"
	"unicode/utf8"
)

var ErrSpinnerPresentationUnavailable = errors.New("Spinner V1 is unavailable")

type SpinnerSize uint32

const (
	SpinnerCustom SpinnerSize = iota
	SpinnerXSmall
	SpinnerSmall
	SpinnerMedium
	SpinnerLarge
)

type SpinnerEasing uint32

const (
	SpinnerQuadratic SpinnerEasing = iota
	SpinnerLinear
)

// SpinnerConfig is copied. Icon is an embedded catalog key; empty selects Loader.
// Named sizes use RemSize (zero selects 16); custom sizes use Pixels.
// Duration zero selects .8 seconds. RGBA is numeric 0xRRGGBBAA.
type SpinnerConfig struct {
	Size                       SpinnerSize
	Pixels, RemSize, Duration  float32
	RGBA                       uint32
	Easing                     SpinnerEasing
	Icon                       string
	CustomColor, ReducedMotion bool
}

func DefaultSpinnerConfig() SpinnerConfig { return SpinnerConfig{Size: SpinnerMedium} }

// SpinnerPresentation is an owned painted sample. Deadlines use absolute Core time.
type SpinnerPresentation struct {
	FrameSequence             uint64
	CoreTime, NextWakeSeconds float64
	Phase                     float32
}

func RotatingSpinner(key string, config SpinnerConfig) *Node {
	n := Spinner(key)
	n.err = n.ConfigureSpinner(config)
	return n
}
func (n *Node) ConfigureSpinner(c SpinnerConfig) error {
	if n == nil || n.kind != 36 || n.removed {
		return errors.New("configuration requires a live Spinner")
	}
	if n.hasVisualSize {
		return errors.New("legacy visual size cannot be combined with Spinner presentation")
	}
	if err := c.validate(); err != nil {
		return err
	}
	if n.app != nil {
		if err := n.configureMountedSpinner(c); err != nil {
			return err
		}
	}
	n.spinnerConfig = &c
	return nil
}
func (c SpinnerConfig) validate() error {
	if c.Size > SpinnerLarge || c.Easing > SpinnerLinear || !(c.Duration >= 0 && c.Duration <= 60) || (!c.CustomColor && c.RGBA != 0) || len(c.Icon) > 63 || strings.ContainsRune(c.Icon, 0) || !utf8.ValidString(c.Icon) {
		return errors.New("invalid Spinner configuration")
	}
	if c.Size == SpinnerCustom {
		if c.Pixels > 0 && c.Pixels <= 4096 && c.RemSize == 0 {
			return nil
		}
	} else if c.Pixels == 0 && c.RemSize >= 0 && c.RemSize <= 4096 {
		rem := c.RemSize
		if rem == 0 {
			rem = 16
		}
		resolved := rem * [...]float32{0, .75, .875, 1, 1.5}[c.Size]
		if resolved > 0 && resolved <= 4096 {
			return nil
		}
	}
	return errors.New("invalid Spinner size")
}
