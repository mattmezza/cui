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

type DisclosureHeadingLayer uint32

const (
	DisclosureHeadingNormal DisclosureHeadingLayer = iota
	DisclosureHeadingExpanded
	DisclosureHeadingDisabled
)

// DisclosureTitleState names an interaction style for a built-in title.
type DisclosureTitleState uint32

const (
	DisclosureTitleHovered      DisclosureTitleState = C.CUI_BRIDGE_DISCLOSURE_TITLE_HOVERED
	DisclosureTitleFocusVisible DisclosureTitleState = C.CUI_BRIDGE_DISCLOSURE_TITLE_FOCUS_VISIBLE
	DisclosureTitlePressed      DisclosureTitleState = C.CUI_BRIDGE_DISCLOSURE_TITLE_PRESSED
)

// DisclosureTitleStyle refines typography or color for one interaction.
// Nil inherits, and a pointer to zero selects transparent text.
type DisclosureTitleStyle struct {
	Typography *DisclosureTypography
	TitleRGBA  *uint32
}

type DisclosureWeight uint32

const (
	DisclosureWeightNormal DisclosureWeight = iota
	DisclosureWeightMedium
	DisclosureWeightSemibold
	DisclosureWeightBold
)

type DisclosureAlignment uint32

const (
	DisclosureAlignLeft DisclosureAlignment = iota
	DisclosureAlignCenter
	DisclosureAlignRight
)

// DisclosureTypography uses logical pixels. FontSize is required. Zero
// LineHeight selects natural metrics. Nonzero LetterSpacing requires host support.
type DisclosureTypography struct {
	FontSize, LineHeight, LetterSpacing float32
	Weight                              DisclosureWeight
	Align                               DisclosureAlignment
	Wrap, Italic, Monospace             bool
}

// DisclosureHeadingBox replaces the complete, vertically centered trigger row.
// Width -1 fills, width 0 fits, and height 0 fits.
type DisclosureHeadingBox struct {
	Width, Height, Padding, Gap, Radius, BorderWidth float32
	BackgroundRGBA, BorderRGBA                       uint32
}

// DisclosureHeading refines one state. Nil fields inherit, and a TitleRGBA
// pointer to zero selects transparent. The builder copies the pointed-to values.
type DisclosureHeading struct {
	Box        *DisclosureHeadingBox
	Typography *DisclosureTypography
	TitleRGBA  *uint32
}

// DisclosureDivider customizes the separator in a joined Accordion. Zero
// Thickness omits the divider. Insets are measured in logical pixels.
type DisclosureDivider struct {
	Thickness, InsetLeft, InsetRight float32
	ColorRGBA                        uint32
}

type disclosureConfigWire = C.cui_bridge_disclosure_config_v1
type disclosureStyleWire = C.cui_bridge_disclosure_trigger_style_v1
type disclosureHeadingWire = C.cui_bridge_disclosure_heading_v1
type disclosureDividerWire = C.cui_bridge_disclosure_divider_v1
type disclosureHeadingLayers [DisclosureHeadingDisabled + 1]*disclosureHeadingWire
type disclosureTitleStyles [DisclosureTitlePressed + 1]*disclosureHeadingWire

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

func (s DisclosureHeading) wire() (disclosureHeadingWire, error) {
	wire := disclosureHeadingWire{struct_size: C.sizeof_cui_bridge_disclosure_heading_v1, schema_version: 1}
	if box := s.Box; box != nil {
		if (box.Width != -1 && !disclosureExtent(box.Width)) || !disclosureExtent(box.Height) ||
			!disclosureExtent(box.Padding) || !disclosureExtent(box.Gap) ||
			!disclosureExtent(box.Radius) || !disclosureExtent(box.BorderWidth) {
			return wire, errors.New("invalid disclosure heading box")
		}
		wire.fields |= C.CUI_BRIDGE_DISCLOSURE_HEADING_BOX
		wire.width, wire.height = C.float(box.Width), C.float(box.Height)
		wire.padding, wire.gap = C.float(box.Padding), C.float(box.Gap)
		wire.radius, wire.border_width = C.float(box.Radius), C.float(box.BorderWidth)
		wire.background_rgba, wire.border_rgba = C.uint32_t(box.BackgroundRGBA), C.uint32_t(box.BorderRGBA)
	}
	if title := s.Typography; title != nil {
		if title.FontSize <= 0 || !disclosureExtent(title.FontSize) ||
			!disclosureExtent(title.LineHeight) || !(title.LetterSpacing >= -4096 && title.LetterSpacing <= 4096) ||
			title.Weight > DisclosureWeightBold || title.Align > DisclosureAlignRight {
			return wire, errors.New("invalid disclosure heading typography")
		}
		wire.fields |= C.CUI_BRIDGE_DISCLOSURE_HEADING_TYPOGRAPHY
		wire.font_size, wire.line_height = C.float(title.FontSize), C.float(title.LineHeight)
		wire.letter_spacing = C.float(title.LetterSpacing)
		wire.weight, wire.align = C.uint32_t(title.Weight), C.uint32_t(title.Align)
		if title.Wrap {
			wire.text_flags |= C.CUI_BRIDGE_DISCLOSURE_TITLE_WRAP
		}
		if title.Italic {
			wire.text_flags |= C.CUI_BRIDGE_DISCLOSURE_TITLE_ITALIC
		}
		if title.Monospace {
			wire.text_flags |= C.CUI_BRIDGE_DISCLOSURE_TITLE_MONOSPACE
		}
	}
	if s.TitleRGBA != nil {
		wire.fields |= C.CUI_BRIDGE_DISCLOSURE_HEADING_COLOR
		wire.title_rgba = C.uint32_t(*s.TitleRGBA)
	}
	return wire, nil
}

func (s DisclosureDivider) wire() (disclosureDividerWire, error) {
	if !disclosureExtent(s.Thickness) || !disclosureExtent(s.InsetLeft) || !disclosureExtent(s.InsetRight) {
		return disclosureDividerWire{}, errors.New("invalid disclosure divider geometry")
	}
	return disclosureDividerWire{
		struct_size: C.sizeof_cui_bridge_disclosure_divider_v1, schema_version: 1,
		thickness: C.float(s.Thickness), inset_left: C.float(s.InsetLeft), inset_right: C.float(s.InsetRight),
		color_rgba: C.uint32_t(s.ColorRGBA),
	}, nil
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

// DisclosureHeading applies a copied heading layer in a builder chain.
func (n *Node) DisclosureHeading(layer DisclosureHeadingLayer, style DisclosureHeading) *Node {
	if n.err == nil {
		n.err = n.SetDisclosureHeading(layer, &style)
	}
	return n
}

// DisclosureDivider applies a copied joined divider in a builder chain.
func (n *Node) DisclosureDivider(style DisclosureDivider) *Node {
	if n.err == nil {
		n.err = n.SetDisclosureDivider(&style)
	}
	return n
}

// SetDisclosureHeading copies one layer onto Accordion, Collapsible, or a direct
// Accordion Item. Nil or an empty style clears that layer.
func (n *Node) SetDisclosureHeading(layer DisclosureHeadingLayer, style *DisclosureHeading) error {
	if n == nil || n.removed ||
		(n.kind != C.CUI_BRIDGE_ACCORDION && n.kind != C.CUI_BRIDGE_COLLAPSIBLE && n.kind != C.CUI_BRIDGE_ITEM) {
		return errors.New("heading style requires a live Accordion, Collapsible or Accordion Item")
	}
	if layer > DisclosureHeadingDisabled {
		return errors.New("invalid disclosure heading layer")
	}
	var wire *disclosureHeadingWire
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
		if err := n.setDisclosureHeading(layer, wire); err != nil {
			return err
		}
	}
	n.disclosureHeadings[layer] = wire
	return nil
}

// DisclosureTitleState copies a title style in a builder chain.
func (n *Node) DisclosureTitleState(state DisclosureTitleState, style DisclosureTitleStyle) *Node {
	if n.err == nil {
		n.err = n.SetDisclosureTitleState(state, &style)
	}
	return n
}

// SetDisclosureTitleState copies an interaction style onto Accordion,
// Collapsible, or a direct Accordion Item. Nil or an empty style clears it.
func (n *Node) SetDisclosureTitleState(state DisclosureTitleState, style *DisclosureTitleStyle) error {
	if n == nil || n.removed ||
		(n.kind != C.CUI_BRIDGE_ACCORDION && n.kind != C.CUI_BRIDGE_COLLAPSIBLE && n.kind != C.CUI_BRIDGE_ITEM) {
		return errors.New("title style requires a live Accordion, Collapsible or Accordion Item")
	}
	if state > DisclosureTitlePressed {
		return errors.New("invalid disclosure title state")
	}
	var wire *disclosureHeadingWire
	if style != nil {
		record, err := (DisclosureHeading{
			Typography: style.Typography, TitleRGBA: style.TitleRGBA,
		}).wire()
		if err != nil {
			return err
		}
		if record.fields != 0 {
			wire = &record
		}
	}
	if n.app != nil {
		if err := n.setDisclosureTitleState(state, wire); err != nil {
			return err
		}
	}
	n.disclosureTitles[state] = wire
	return nil
}

// SetDisclosureDivider copies a joined Accordion divider. Nil restores the
// theme divider. A zero thickness explicitly removes the divider.
func (n *Node) SetDisclosureDivider(style *DisclosureDivider) error {
	if n == nil || n.removed || n.kind != C.CUI_BRIDGE_ACCORDION {
		return errors.New("divider style requires a live Accordion")
	}
	var wire *disclosureDividerWire
	if style != nil {
		record, err := style.wire()
		if err != nil {
			return err
		}
		wire = &record
	}
	if n.app != nil {
		if err := n.setDisclosureDivider(wire); err != nil {
			return err
		}
	}
	n.disclosureDivider = wire
	return nil
}
