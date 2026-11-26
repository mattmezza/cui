// SPDX-License-Identifier: MIT
package cui

/*
#include <cui/cui_bridge.h>
*/
import "C"

import (
	"errors"
	"strings"
	"unicode/utf8"
)

// ErrDisclosureOptionsUnavailable reports an incomplete provider. Ordinary
// Accordion, Collapsible and Item nodes still work.
var ErrDisclosureOptionsUnavailable = errors.New("Disclosure V1 options are unavailable")

// DisclosureConfig is copied on assignment. Width -1 fills, 0 fits, and a
// positive width sets logical pixels. The two icon strings name embedded catalog
// entries or paths. Empty names use chevrons.
type DisclosureConfig struct {
	DesignSize                  DesignSize
	Width                       float32
	Joined, Bordered, Single    bool // Accordion only.
	AllowCollapse               bool // Accordion only.
	Borderless                  bool // Collapsible only.
	Card                        *DisclosureCard
	CollapsedIcon, ExpandedIcon string
}

// DisclosureCard replaces the outer box style when supplied. Colors use
// numeric 0xRRGGBBAA. Dimensions are logical pixels in [0,4096].
type DisclosureCard struct {
	BackgroundRGBA, BorderRGBA uint32
	Radius, BorderWidth        float32
	Padding, Gap               float32
	Clip                       bool
}

type DisclosureCursor uint32

const (
	DisclosureCursorDefault DisclosureCursor = iota
	DisclosureCursorPointer
	DisclosureCursorText
	DisclosureCursorResizeHorizontal
	DisclosureCursorResizeVertical
	DisclosureCursorMove
	DisclosureCursorNotAllowed
	DisclosureCursorCrosshair
	DisclosureCursorGrab
	DisclosureCursorGrabbing
	DisclosureCursorZoomIn
	DisclosureCursorZoomOut
	DisclosureCursorWait
	DisclosureCursorHelp
)

// DisclosureTriggerStyle refines a Collapsible or direct Accordion Item. Nil
// fields inherit and a pointer to zero explicitly selects zero.
type DisclosureTriggerStyle struct {
	HoverRGBA, PressedRGBA, ExpandedRGBA, DisabledRGBA, FocusRGBA *uint32
	Cursor                                                        *DisclosureCursor
}

type disclosureConfigWire = C.cui_bridge_disclosure_config_v1
type disclosureStyleWire = C.cui_bridge_disclosure_trigger_style_v1

func disclosureExtent(v float32) bool { return v >= 0 && v <= 4096 }

func (c DisclosureConfig) wire(kind int) (disclosureConfigWire, error) {
	var wire disclosureConfigWire
	if c.DesignSize < Inherit || c.DesignSize > Size9XL ||
		(c.Width != -1 && !disclosureExtent(c.Width)) {
		return wire, errors.New("invalid disclosure size or width")
	}
	if (kind == C.CUI_BRIDGE_ACCORDION && c.Borderless) ||
		(kind == C.CUI_BRIDGE_COLLAPSIBLE && (c.Joined || c.Bordered || c.Single || c.AllowCollapse)) {
		return wire, errors.New("disclosure options are invalid for this node kind")
	}
	wire.struct_size = C.sizeof_cui_bridge_disclosure_config_v1
	wire.schema_version, wire.design_size, wire.width = 1, C.uint32_t(c.DesignSize), C.float(c.Width)
	for _, flag := range []struct {
		enabled bool
		bit     C.uint32_t
	}{
		{c.Joined, C.CUI_BRIDGE_DISCLOSURE_JOINED_V1},
		{c.Bordered, C.CUI_BRIDGE_DISCLOSURE_BORDERED_V1},
		{c.Single, C.CUI_BRIDGE_DISCLOSURE_SINGLE_V1},
		{c.AllowCollapse, C.CUI_BRIDGE_DISCLOSURE_ALLOW_COLLAPSE_V1},
		{c.Borderless, C.CUI_BRIDGE_DISCLOSURE_BORDERLESS_V1},
	} {
		if flag.enabled {
			wire.flags |= flag.bit
		}
	}
	if card := c.Card; card != nil {
		if !(disclosureExtent(card.Radius) && disclosureExtent(card.BorderWidth) &&
			disclosureExtent(card.Padding) && disclosureExtent(card.Gap)) {
			return wire, errors.New("invalid disclosure card geometry")
		}
		wire.flags |= C.CUI_BRIDGE_DISCLOSURE_CUSTOM_CARD_V1
		if card.Clip {
			wire.flags |= C.CUI_BRIDGE_DISCLOSURE_CLIP_CARD_V1
		}
		wire.card_radius, wire.card_border_width = C.float(card.Radius), C.float(card.BorderWidth)
		wire.card_padding, wire.card_gap = C.float(card.Padding), C.float(card.Gap)
		wire.card_background_rgba, wire.card_border_rgba = C.uint32_t(card.BackgroundRGBA), C.uint32_t(card.BorderRGBA)
	}
	for _, icon := range []struct {
		name string
		out  []C.char
	}{{c.CollapsedIcon, wire.collapsed_icon[:]}, {c.ExpandedIcon, wire.expanded_icon[:]}} {
		if len(icon.name) >= len(icon.out) || strings.ContainsRune(icon.name, 0) || !utf8.ValidString(icon.name) {
			return wire, errors.New("disclosure icon names must contain at most 63 UTF-8 bytes without NUL")
		}
		for i := 0; i < len(icon.name); i++ {
			icon.out[i] = C.char(icon.name[i])
		}
	}
	return wire, nil
}

func (s DisclosureTriggerStyle) wire() (disclosureStyleWire, error) {
	wire := disclosureStyleWire{struct_size: C.sizeof_cui_bridge_disclosure_trigger_style_v1, schema_version: 1}
	for _, field := range []struct {
		value *uint32
		out   *C.uint32_t
		bit   C.uint32_t
	}{
		{s.HoverRGBA, &wire.hover_rgba, C.CUI_BRIDGE_DISCLOSURE_HOVER_V1},
		{s.PressedRGBA, &wire.pressed_rgba, C.CUI_BRIDGE_DISCLOSURE_PRESSED_V1},
		{s.ExpandedRGBA, &wire.expanded_rgba, C.CUI_BRIDGE_DISCLOSURE_EXPANDED_V1},
		{s.DisabledRGBA, &wire.disabled_rgba, C.CUI_BRIDGE_DISCLOSURE_DISABLED_V1},
		{s.FocusRGBA, &wire.focus_rgba, C.CUI_BRIDGE_DISCLOSURE_FOCUS_V1},
	} {
		if field.value != nil {
			wire.fields |= field.bit
			*field.out = C.uint32_t(*field.value)
		}
	}
	if s.Cursor != nil {
		if *s.Cursor > DisclosureCursorHelp {
			return wire, errors.New("invalid disclosure cursor")
		}
		wire.fields |= C.CUI_BRIDGE_DISCLOSURE_CURSOR_V1
		wire.cursor = C.uint32_t(*s.Cursor)
	}
	return wire, nil
}

// DisclosureOptions applies copied presentation in a builder chain.
func (n *Node) DisclosureOptions(config DisclosureConfig) *Node {
	if n.err == nil {
		n.err = n.ConfigureDisclosure(&config)
	}
	return n
}

// DisclosureStyle applies copied trigger feedback in a builder chain.
func (n *Node) DisclosureStyle(style DisclosureTriggerStyle) *Node {
	if n.err == nil {
		n.err = n.SetDisclosureTriggerStyle(&style)
	}
	return n
}

// ConfigureDisclosure copies the configuration onto an Accordion or
// Collapsible. Nil clears it and retains independent trigger styles.
func (n *Node) ConfigureDisclosure(config *DisclosureConfig) error {
	if n == nil || n.removed || (n.kind != C.CUI_BRIDGE_ACCORDION && n.kind != C.CUI_BRIDGE_COLLAPSIBLE) {
		return errors.New("configuration requires a live Accordion or Collapsible")
	}
	var wire *disclosureConfigWire
	if config != nil {
		record, err := config.wire(n.kind)
		if err != nil {
			return err
		}
		wire = &record
	}
	if n.app != nil {
		if err := n.setDisclosureConfig(wire); err != nil {
			return err
		}
	}
	n.disclosureConfig = wire
	return nil
}

// SetDisclosureTriggerStyle copies trigger feedback onto Collapsible or a
// direct Accordion Item. Nil or an empty style clears it without queuing events.
func (n *Node) SetDisclosureTriggerStyle(style *DisclosureTriggerStyle) error {
	if n == nil || n.removed || (n.kind != C.CUI_BRIDGE_COLLAPSIBLE && n.kind != C.CUI_BRIDGE_ITEM) {
		return errors.New("trigger style requires a live Collapsible or Accordion Item")
	}
	var wire *disclosureStyleWire
	if style != nil {
		record, err := style.wire()
		if err != nil {
			return err
		}
		if record.fields != 0 {
			wire = &record
		}
	}
	if n.app != nil {
		if err := n.setDisclosureStyle(wire); err != nil {
			return err
		}
	}
	n.disclosureStyle = wire
	return nil
}
