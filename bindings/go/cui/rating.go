// SPDX-License-Identifier: MIT
package cui

/*
#include <cui/cui_bridge.h>
*/
import "C"

import "errors"

// ErrRatingOptionsUnavailable indicates that the provider lacks the complete
// compatible Rating V1 group. Ordinary Rating nodes still work.
var ErrRatingOptionsUnavailable = errors.New("Rating V1 options are unavailable")

// RatingConfig is copied per node. Zero width uses natural layout. Each nil
// mask uses a built-in star. Non-nil masks must belong to the mounted app.
// HasFill, HasOutline and HasPreview distinguish inherited colors from explicit 0xRRGGBBAA colors.
type RatingConfig struct {
	HalfSteps, HoverPreview, StepDownFilled, Clearable, Readonly bool
	DesignSize                                                   DesignSize
	Width                                                        float32
	HasFill, HasOutline, HasPreview                              bool
	FillRGBA, OutlineRGBA, PreviewRGBA                           uint32
	OutlineIcon, FillIcon                                        *IconResource
}

// ConfiguredRating creates a Rating with copied presentation options. Probe
// SupportsRatingOptions before choosing it over an ordinary Rating.
func ConfiguredRating(key string, config RatingConfig) *Node {
	n := Rating(key)
	n.err = n.ConfigureRating(config)
	return n
}

func (n *Node) checkRating() error {
	if n == nil || n.kind != int(C.CUI_BRIDGE_RATING) || n.removed {
		return errors.New("options require a live Rating")
	}
	return nil
}

// ConfigureRating updates presentation without dispatching queued changes.
func (n *Node) ConfigureRating(config RatingConfig) error {
	if err := n.checkRating(); err != nil {
		return err
	}
	wire, err := config.wire(n.app)
	if err != nil {
		return err
	}
	if n.app != nil {
		if err := n.setRatingWire(&wire); err != nil {
			return err
		}
	}
	config = config.copy()
	n.ratingConfig = &config
	return nil
}

// ResetRating restores the integer presenter. The next step may normalize
// the current value and queue its usual scalar change.
func (n *Node) ResetRating() error {
	if err := n.checkRating(); err != nil {
		return err
	}
	if n.app != nil {
		if err := n.setRatingWire(nil); err != nil {
			return err
		}
	}
	n.ratingConfig = nil
	return nil
}

// RatingConfig returns a copy and whether an override is present. The Icon
// values share their original app ownership. Earlier providers return no override.
func (n *Node) RatingConfig() (RatingConfig, bool, error) {
	if err := n.checkRating(); err != nil {
		return RatingConfig{}, false, err
	}
	if n.app == nil {
		if n.ratingConfig == nil {
			return RatingConfig{}, false, nil
		}
		return n.ratingConfig.copy(), true, nil
	}
	wire := newRatingWire()
	ok, err := n.readRatingWire(&wire)
	if err != nil || !ok {
		return RatingConfig{}, false, err
	}
	config, err := decodeRating(wire, n.ratingConfig)
	return config, err == nil, err
}

func (c RatingConfig) copy() RatingConfig {
	c.OutlineIcon, c.FillIcon = c.OutlineIcon.alias(), c.FillIcon.alias()
	return c
}

type ratingWire = C.cui_bridge_rating_config_v1

func newRatingWire() ratingWire {
	return ratingWire{struct_size: C.sizeof_cui_bridge_rating_config_v1, schema_version: 1}
}

func (c RatingConfig) valid() bool {
	return c.DesignSize >= Inherit && c.DesignSize <= Size9XL &&
		(c.Width == 0 || c.Width >= 1 && c.Width <= 4096) &&
		(c.HasFill || c.FillRGBA == 0) && (c.HasOutline || c.OutlineRGBA == 0) &&
		(c.HasPreview || c.PreviewRGBA == 0)
}

func (c RatingConfig) wire(app *App) (ratingWire, error) {
	if !c.valid() {
		return ratingWire{}, errors.New("invalid Rating size, width or color")
	}
	wire := newRatingWire()
	wire.design_size, wire.width = C.uint32_t(c.DesignSize), C.float(c.Width)
	wire.fill_rgba, wire.outline_rgba, wire.preview_rgba = C.uint32_t(c.FillRGBA), C.uint32_t(c.OutlineRGBA), C.uint32_t(c.PreviewRGBA)
	for _, flag := range []struct {
		enabled bool
		bit     C.uint32_t
	}{
		{c.HalfSteps, C.CUI_BRIDGE_RATING_HALF_STEPS_V1},
		{c.HoverPreview, C.CUI_BRIDGE_RATING_HOVER_PREVIEW_V1},
		{c.StepDownFilled, C.CUI_BRIDGE_RATING_STEP_DOWN_FILLED_V1},
		{c.Clearable, C.CUI_BRIDGE_RATING_CLEARABLE_V1},
		{c.Readonly, C.CUI_BRIDGE_RATING_READONLY_V1},
		{c.HasFill, C.CUI_BRIDGE_RATING_CUSTOM_FILL_V1},
		{c.HasOutline, C.CUI_BRIDGE_RATING_CUSTOM_OUTLINE_V1},
		{c.HasPreview, C.CUI_BRIDGE_RATING_CUSTOM_PREVIEW_V1},
	} {
		if flag.enabled {
			wire.flags |= flag.bit
		}
	}
	for _, mask := range []struct {
		resource *IconResource
		out      *C.cui_bridge_icon_handle_v1
	}{{c.OutlineIcon, &wire.outline_icon}, {c.FillIcon, &wire.fill_icon}} {
		if mask.resource != nil {
			handle, err := mask.resource.handleFor(app)
			if err != nil {
				return ratingWire{}, err
			}
			*mask.out = handle
		}
	}
	return wire, nil
}

func decodeRating(wire ratingWire, stored *RatingConfig) (RatingConfig, error) {
	const known = C.CUI_BRIDGE_RATING_HALF_STEPS_V1 | C.CUI_BRIDGE_RATING_HOVER_PREVIEW_V1 |
		C.CUI_BRIDGE_RATING_STEP_DOWN_FILLED_V1 | C.CUI_BRIDGE_RATING_CLEARABLE_V1 |
		C.CUI_BRIDGE_RATING_READONLY_V1 | C.CUI_BRIDGE_RATING_CUSTOM_FILL_V1 |
		C.CUI_BRIDGE_RATING_CUSTOM_OUTLINE_V1 | C.CUI_BRIDGE_RATING_CUSTOM_PREVIEW_V1
	invalid := errors.New("invalid copied Rating options")
	if wire.struct_size != C.sizeof_cui_bridge_rating_config_v1 || wire.schema_version != 1 ||
		wire.flags&^known != 0 || wire.design_size > C.uint32_t(Size9XL) || stored == nil {
		return RatingConfig{}, invalid
	}
	saved, err := stored.wire(nil)
	if err != nil {
		return RatingConfig{}, err
	}
	if wire.outline_icon != saved.outline_icon || wire.fill_icon != saved.fill_icon {
		return RatingConfig{}, invalid
	}
	config := RatingConfig{
		HalfSteps:      wire.flags&C.CUI_BRIDGE_RATING_HALF_STEPS_V1 != 0,
		HoverPreview:   wire.flags&C.CUI_BRIDGE_RATING_HOVER_PREVIEW_V1 != 0,
		StepDownFilled: wire.flags&C.CUI_BRIDGE_RATING_STEP_DOWN_FILLED_V1 != 0,
		Clearable:      wire.flags&C.CUI_BRIDGE_RATING_CLEARABLE_V1 != 0,
		Readonly:       wire.flags&C.CUI_BRIDGE_RATING_READONLY_V1 != 0,
		HasFill:        wire.flags&C.CUI_BRIDGE_RATING_CUSTOM_FILL_V1 != 0,
		HasOutline:     wire.flags&C.CUI_BRIDGE_RATING_CUSTOM_OUTLINE_V1 != 0,
		HasPreview:     wire.flags&C.CUI_BRIDGE_RATING_CUSTOM_PREVIEW_V1 != 0,
		DesignSize:     DesignSize(wire.design_size), Width: float32(wire.width),
		FillRGBA: uint32(wire.fill_rgba), OutlineRGBA: uint32(wire.outline_rgba), PreviewRGBA: uint32(wire.preview_rgba),
		OutlineIcon: stored.OutlineIcon.alias(), FillIcon: stored.FillIcon.alias(),
	}
	if !config.valid() {
		return RatingConfig{}, invalid
	}
	return config, nil
}
