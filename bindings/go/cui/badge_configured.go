// SPDX-License-Identifier: MIT
package cui

/*
#include "icon_badge_ffi.h"
*/
import "C"

import (
	"errors"
	"strings"
)

// ErrConfiguredBadgeUnavailable means the provider lacks configured Badge V1.
var ErrConfiguredBadgeUnavailable = errors.New("configured Badge V1 is unavailable")

const configuredBadgeKind = int(C.CUI_BRIDGE_BADGE_CONFIGURED)

// BadgeVariant chooses a count, an unlabelled dot, or an Icon.
type BadgeVariant uint32

const (
	BadgeCount BadgeVariant = iota
	BadgeDot
	BadgeIcon
)

// BadgeSize is the component preset. Use DesignSize to apply a design token.
type BadgeSize uint32

const (
	BadgeMedium BadgeSize = iota
	BadgeSmall
	BadgeLarge
	BadgeExtraSmall
)

// BadgeConfig configures the indicator around retained child content.
// BadgeIcon accepts either NamedIcon or Artwork. A zero count hides the
// marker. A zero maximum formats a positive count as 0+.
type BadgeConfig struct {
	Variant         BadgeVariant
	Size            BadgeSize
	DesignSize      DesignSize
	Count, Maximum  uint64
	Name, NamedIcon string
	Artwork         *IconResource
	ArtworkOptions  IconOptions
	ColorRGBA       uint32
	HasColor        bool
}

// BadgeResult reports whether the Badge was submitted, the whole-frame
// bridge status, and artwork preparation when HasIconError is true.
type BadgeResult struct {
	FrameSequence uint64
	FrameStatus   uint32
	Submitted     bool
	IconError     IconError
	HasIconError  bool
}

func badgeConfigHeader() C.cui_bridge_badge_config_v1 {
	return C.cui_bridge_badge_config_v1{struct_size: C.sizeof_cui_bridge_badge_config_v1, schema_version: 1}
}

func (c BadgeConfig) wire(app *App) (C.cui_bridge_badge_config_v1, *C.cui_bridge_icon_options_v1, error) {
	wire := badgeConfigHeader()
	limit := uint64(^C.size_t(0))
	if c.Variant > BadgeIcon || c.Size > BadgeExtraSmall || c.DesignSize < Inherit || c.DesignSize > Size9XL ||
		(!c.HasColor && c.ColorRGBA != 0) || c.Count > limit || c.Maximum > limit {
		return wire, nil, errors.New("invalid Badge variant, size, color or count")
	}
	if err := valid(c.Name); err != nil {
		return wire, nil, err
	}
	if err := valid(c.NamedIcon); err != nil {
		return wire, nil, err
	}
	hasNamed := c.NamedIcon != ""
	hasArtwork := c.Artwork != nil
	if (c.Variant == BadgeIcon && hasNamed == hasArtwork) || (c.Variant != BadgeIcon && (hasNamed || hasArtwork)) {
		return wire, nil, errors.New("BadgeIcon requires exactly one named or owned Icon")
	}
	if !hasArtwork && c.ArtworkOptions != (IconOptions{}) {
		return wire, nil, errors.New("Badge artwork options require an owned Icon resource")
	}
	wire.variant, wire.size, wire.design_size = C.uint32_t(c.Variant), C.uint32_t(c.Size), C.uint32_t(c.DesignSize)
	wire.count, wire.maximum = C.uint64_t(c.Count), C.uint64_t(c.Maximum)
	wire.color_rgba = C.uint32_t(c.ColorRGBA)
	iconFlag(&wire.flags, c.HasColor, C.CUI_BRIDGE_BADGE_CUSTOM_COLOR_V1)
	if !hasArtwork {
		return wire, nil, nil
	}
	handle, err := c.Artwork.handleFor(app)
	if err != nil {
		return wire, nil, err
	}
	artwork, err := c.ArtworkOptions.wire()
	if err != nil {
		return wire, nil, err
	}
	wire.artwork = handle
	return wire, &artwork, nil
}

// SupportsConfiguredBadge reports whether the active provider can mount
// configured Badge containers.
func (a *App) SupportsConfiguredBadge() (bool, error) {
	if err := a.check(); err != nil {
		return false, err
	}
	return C.cui_go_badge_supported() != 0, nil
}

func (a *App) requireConfiguredBadge() error {
	supported, err := a.SupportsConfiguredBadge()
	if err == nil && !supported {
		return ErrConfiguredBadgeUnavailable
	}
	return err
}

// DefaultBadgeConfig reads the loaded provider's count, maximum and size.
func (a *App) DefaultBadgeConfig() (BadgeConfig, error) {
	if err := a.requireConfiguredBadge(); err != nil {
		return BadgeConfig{}, err
	}
	wire := badgeConfigHeader()
	if err := result(C.cui_go_badge_default_config_v1(&wire)); err != nil {
		return BadgeConfig{}, err
	}
	config := BadgeConfig{Variant: BadgeVariant(wire.variant), Size: BadgeSize(wire.size),
		DesignSize: DesignSize(wire.design_size), Count: uint64(wire.count), Maximum: uint64(wire.maximum)}
	expected, _, err := config.wire(a)
	if err != nil || wire != expected {
		return BadgeConfig{}, errors.New("invalid default Badge configuration")
	}
	return config, nil
}

// ConfiguredBadge wraps retained children in an optional zero-flow indicator.
func ConfiguredBadge(key string, config BadgeConfig) *Node {
	n := &Node{kind: configuredBadgeKind, key: key}
	n.err = n.ConfigureBadge(config)
	return n
}

func (n *Node) checkBadge() error {
	if n == nil || n.kind != configuredBadgeKind || n.removed {
		return errors.New("configuration requires a live configured Badge")
	}
	if n.app != nil {
		return n.app.check()
	}
	return nil
}

// ConfigureBadge replaces the indicator options while retaining its children.
// A failed update preserves the previous configuration.
func (n *Node) ConfigureBadge(config BadgeConfig) error {
	if err := n.checkBadge(); err != nil {
		return err
	}
	wire, artwork, err := config.wire(n.app)
	if err != nil {
		return err
	}
	if n.app != nil {
		// The bridge requires disjoint spans even when the strings are equal.
		name := strings.Clone(config.Name)
		code := C.cui_go_badge_configure_v1(n.app.ptr, n.handle, &wire, artwork,
			iconSpan(name), C.uint32_t(len(name)), iconSpan(config.NamedIcon), C.uint32_t(len(config.NamedIcon)))
		if err := result(code); err != nil {
			return err
		}
	}
	config.Artwork = config.Artwork.alias()
	n.badgeConfig = &config
	return nil
}

func (a *App) createNativeBadge(node *Node, parent C.cui_bridge_node, key *C.char) (C.cui_bridge_node, error) {
	config := node.badgeConfig
	wire, artwork, err := config.wire(a)
	if err != nil {
		return C.cui_bridge_node{}, err
	}
	name := strings.Clone(config.Name)
	var handle C.cui_bridge_node
	code := C.cui_go_badge_create_v1(a.ptr, parent, key, &wire, artwork,
		iconSpan(name), C.uint32_t(len(name)), iconSpan(config.NamedIcon), C.uint32_t(len(config.NamedIcon)), &handle)
	return handle, result(code)
}

// BadgeConfig returns a copy of the current configuration. The underlying
// artwork resource stays app-owned.
func (n *Node) BadgeConfig() (BadgeConfig, error) {
	if err := n.checkBadge(); err != nil {
		return BadgeConfig{}, err
	}
	if n.badgeConfig == nil {
		return BadgeConfig{}, errors.New("Badge configuration missing")
	}
	config := *n.badgeConfig
	config.Artwork = config.Artwork.alias()
	if n.app == nil {
		return config, nil
	}
	wire, artwork := badgeConfigHeader(), iconOptionsHeader()
	code := C.cui_go_badge_get_config_v1(n.app.ptr, n.handle, &wire, &artwork)
	if err := result(code); err != nil {
		return BadgeConfig{}, err
	}
	expected, wantArtwork, err := config.wire(n.app)
	if err != nil {
		return BadgeConfig{}, err
	}
	expectedArtwork := iconOptionsHeader()
	if wantArtwork != nil {
		expectedArtwork = *wantArtwork
	}
	if wire != expected || artwork != expectedArtwork {
		return BadgeConfig{}, errors.New("invalid copied Badge configuration")
	}
	for _, field := range []struct {
		id    C.uint32_t
		value string
	}{
		{C.CUI_BRIDGE_BADGE_NAME_V1, config.Name},
		{C.CUI_BRIDGE_BADGE_NAMED_ICON_V1, config.NamedIcon},
	} {
		value, _, err := iconCopyString(iconTextMax, false,
			func(out *C.char, capacity C.uint32_t, required *C.uint32_t) C.cui_bridge_status {
				return C.cui_go_badge_copy_text_v1(n.app.ptr, n.handle, field.id, out, capacity, required)
			})
		if err != nil {
			return BadgeConfig{}, err
		}
		if value != field.value {
			return BadgeConfig{}, errors.New("invalid copied Badge text")
		}
	}
	return config, nil
}

// BadgeResult reports a Badge observed in the latest admitted frame. Reading
// it does not render or dispatch child events.
func (n *Node) BadgeResult() (BadgeResult, bool, error) {
	if err := n.checkBadge(); err != nil {
		return BadgeResult{}, false, err
	}
	if n.app == nil {
		return BadgeResult{}, false, nil
	}
	wire := C.cui_bridge_badge_result_v1{struct_size: C.sizeof_cui_bridge_badge_result_v1, schema_version: 1}
	code := C.cui_go_badge_get_result_v1(n.app.ptr, n.handle, &wire)
	if code == C.CUI_BRIDGE_EMPTY {
		return BadgeResult{}, false, nil
	}
	if err := result(code); err != nil {
		return BadgeResult{}, false, err
	}
	hasIcon := wire.flags&C.CUI_BRIDGE_BADGE_ICON_OBSERVED_V1 != 0
	if wire.struct_size != C.sizeof_cui_bridge_badge_result_v1 || wire.schema_version != 1 ||
		wire.node != n.handle || wire.frame_sequence == 0 ||
		wire.frame_status > C.CUI_BRIDGE_CORE_ERROR || wire.submitted > 1 ||
		wire.flags&^C.CUI_BRIDGE_BADGE_ICON_OBSERVED_V1 != 0 ||
		IconError(wire.icon_error) > IconAllocation || (!hasIcon && wire.icon_error != 0) {
		return BadgeResult{}, false, errors.New("invalid copied Badge result")
	}
	return BadgeResult{FrameSequence: uint64(wire.frame_sequence), FrameStatus: uint32(wire.frame_status),
		Submitted: wire.submitted == 1, IconError: IconError(wire.icon_error), HasIconError: hasIcon}, true, nil
}

// BadgeIndicatorBounds returns the committed indicator rectangle when present.
func (n *Node) BadgeIndicatorBounds() (Rect, bool, error) {
	if err := n.checkBadge(); err != nil {
		return Rect{}, false, err
	}
	if n.app == nil {
		return Rect{}, false, nil
	}
	var wire C.cui_bridge_rect
	code := C.cui_go_badge_indicator_bounds_v1(n.app.ptr, n.handle, &wire)
	if code == C.CUI_BRIDGE_EMPTY {
		return Rect{}, false, nil
	}
	if err := result(code); err != nil {
		return Rect{}, false, err
	}
	rect := Rect{float32(wire.x), float32(wire.y), float32(wire.width), float32(wire.height)}
	if !finite(rect.X) || !finite(rect.Y) || !finite(rect.Width) || !finite(rect.Height) ||
		rect.Width <= 0 || rect.Height <= 0 {
		return Rect{}, false, errors.New("invalid Badge indicator bounds")
	}
	return rect, true, nil
}
