package cui

import (
	"errors"
	"math"
)

// TextDefaults refines scoped text size and tint on retained Row/Column nodes.
// ColorRGBA uses packed RGBA; a pointer to zero explicitly selects transparent.
type TextDefaults struct {
	FontSize  float32
	ColorRGBA *uint32
}

var ErrTextDefaultsUnavailable = errors.New("text defaults require a newer CUI bridge")

func (o TextDefaults) validate() error {
	if math.IsNaN(float64(o.FontSize)) || math.IsInf(float64(o.FontSize), 0) || o.FontSize < 0 || o.FontSize > 4096 {
		return errors.New("text default font size must be finite and within [0,4096]")
	}
	return nil
}
func (o TextDefaults) rawColor() (uint32, uint32) {
	if o.ColorRGBA != nil {
		return *o.ColorRGBA, 1
	}
	return 0, 0
}
