// SPDX-License-Identifier: MIT
package cui

/*
#include <cui/cui_bridge.h>
*/
import "C"

import (
	"errors"
	"strings"
)

type AttachmentTextWeight uint32

const (
	AttachmentTextNormal AttachmentTextWeight = iota
	AttachmentTextMedium
	AttachmentTextSemibold
	AttachmentTextBold
)

// AttachmentTypography selects weight, italic and monospace explicitly.
// Letter spacing is in logical pixels and requires host support when nonzero.
type AttachmentTypography struct {
	Weight            AttachmentTextWeight
	LetterSpacing     float32
	Italic, Monospace bool
}

// AttachmentTextStyle refines one metadata slot. Zero font size inherits.
// A nil color inherits; a pointer to zero selects transparent 0xRRGGBBAA.
type AttachmentTextStyle struct {
	FontSize   float32
	ColorRGBA  *uint32
	Typography *AttachmentTypography
}

// AttachmentOptions opts into one-line ellipsis with full semantic labels.
// Vertical previews use MediaAspect (width/height); zero selects square.
// Media reserves a preview slot before pixels arrive. Status overrides are
// optional and Indeterminate applies during Uploading.
type AttachmentOptions struct {
	Vertical, Media, Indeterminate bool
	MediaAspect                    float32
	TitleStatus, DescriptionStatus *AttachmentState
	Title, Description             AttachmentTextStyle
}

type attachmentOptionsWire = C.cui_bridge_attachment_config_v1

type attachmentPreview struct {
	pixels                []byte
	width, height, stride uint32
}

var (
	ErrAttachmentOptionsUnavailable = errors.New("Attachment presentation requires a newer CUI bridge")
	ErrAttachmentPreviewUnavailable = errors.New("Attachment preview requires a newer CUI bridge")
)

func (s AttachmentTextStyle) wire() (C.cui_bridge_attachment_text_v1, error) {
	wire := C.cui_bridge_attachment_text_v1{font_size: C.float(s.FontSize)}
	if !(s.FontSize >= 0 && s.FontSize <= 4096) {
		return wire, errors.New("invalid Attachment font size")
	}
	if s.ColorRGBA != nil {
		wire.flags |= C.CUI_BRIDGE_ATTACHMENT_TEXT_COLOR_V1
		wire.color_rgba = C.uint32_t(*s.ColorRGBA)
	}
	if text := s.Typography; text != nil {
		if text.Weight > AttachmentTextBold || !(text.LetterSpacing >= -4096 && text.LetterSpacing <= 4096) {
			return wire, errors.New("invalid Attachment typography")
		}
		wire.flags |= C.CUI_BRIDGE_ATTACHMENT_TEXT_STYLE_V1
		wire.flags |= C.uint32_t(btoi(text.Italic)) * C.CUI_BRIDGE_ATTACHMENT_TEXT_ITALIC_V1
		wire.flags |= C.uint32_t(btoi(text.Monospace)) * C.CUI_BRIDGE_ATTACHMENT_TEXT_MONOSPACE_V1
		wire.weight = C.uint32_t(text.Weight)
		wire.letter_spacing = C.float(text.LetterSpacing)
	}
	return wire, nil
}

func (o AttachmentOptions) wire() (attachmentOptionsWire, error) {
	wire := attachmentOptionsWire{
		struct_size: C.sizeof_cui_bridge_attachment_config_v1, schema_version: 1,
		media_aspect: C.float(o.MediaAspect),
	}
	if o.MediaAspect != 0 && !(o.MediaAspect >= 1.0/16 && o.MediaAspect <= 16) {
		return wire, errors.New("invalid Attachment media aspect")
	}
	wire.flags |= C.uint32_t(btoi(o.Vertical)) * C.CUI_BRIDGE_ATTACHMENT_VERTICAL_V1
	wire.flags |= C.uint32_t(btoi(o.Media)) * C.CUI_BRIDGE_ATTACHMENT_MEDIA_V1
	wire.flags |= C.uint32_t(btoi(o.Indeterminate)) * C.CUI_BRIDGE_ATTACHMENT_INDETERMINATE_V1
	for _, field := range []struct {
		status *AttachmentState
		target *C.uint32_t
		flag   C.uint32_t
	}{
		{o.TitleStatus, &wire.title_status, C.CUI_BRIDGE_ATTACHMENT_TITLE_STATUS_V1},
		{o.DescriptionStatus, &wire.description_status, C.CUI_BRIDGE_ATTACHMENT_DESCRIPTION_STATUS_V1},
	} {
		if field.status == nil {
			continue
		}
		if *field.status < AttachmentPending || *field.status > AttachmentComplete {
			return wire, errors.New("invalid Attachment metadata status")
		}
		wire.flags |= field.flag
		*field.target = C.uint32_t(*field.status)
	}
	var err error
	if wire.title, err = o.Title.wire(); err != nil {
		return wire, err
	}
	wire.description, err = o.Description.wire()
	return wire, err
}

func (n *Node) validateAttachmentLabel(label string) error {
	if n.attachmentOptions != nil && strings.ContainsAny(label, "\r\n") {
		return errors.New("configured Attachment labels must be single lines")
	}
	return nil
}

// AttachmentOptions copies presentation options in a builder chain.
func (n *Node) AttachmentOptions(options AttachmentOptions) *Node {
	if n.err == nil {
		n.err = n.SetAttachmentOptions(&options)
	}
	return n
}

// SetAttachmentOptions copies presentation options. Nil restores the legacy
// metadata and orientation, preserving any preview and named size.
func (n *Node) SetAttachmentOptions(options *AttachmentOptions) error {
	if n == nil || n.removed || n.kind != C.CUI_BRIDGE_ATTACHMENT {
		return errors.New("presentation options require a live Attachment")
	}
	var next *attachmentOptionsWire
	if options != nil {
		if strings.ContainsAny(n.text, "\r\n") || strings.ContainsAny(n.detail, "\r\n") {
			return errors.New("configured Attachment labels must be single lines")
		}
		record, err := options.wire()
		if err != nil {
			return err
		}
		next = &record
	}
	if n.app != nil {
		if err := n.applyAttachmentOptions(next); err != nil {
			return err
		}
	}
	n.attachmentOptions = next
	return nil
}

// AttachmentPreviewRGBA sets decoded RGBA8 pixels in a builder chain.
func (n *Node) AttachmentPreviewRGBA(pixels []byte, width, height, stride uint32) *Node {
	if n.err == nil {
		n.err = n.SetAttachmentPreviewRGBA(pixels, width, height, stride)
	}
	return n
}

// SetAttachmentPreviewRGBA copies decoded RGBA8 with a maximum of 16 MiB,
// including row padding. A later update replaces the previous preview.
func (n *Node) SetAttachmentPreviewRGBA(pixels []byte, width, height, stride uint32) error {
	if n == nil || n.removed || n.kind != C.CUI_BRIDGE_ATTACHMENT {
		return errors.New("RGBA preview requires a live Attachment")
	}
	if !validRGBALength(len(pixels), width, height, stride) {
		return errors.New("invalid Attachment RGBA dimensions or data length")
	}
	preview := &attachmentPreview{pixels: pixels, width: width, height: height, stride: stride}
	if n.app != nil {
		return n.applyAttachmentPreview(preview)
	}
	preview.pixels = append([]byte(nil), pixels...)
	n.attachmentPreview = preview
	return nil
}

// ClearAttachmentPreview releases the copied pixels and keeps the presentation.
func (n *Node) ClearAttachmentPreview() error {
	if n == nil || n.removed || n.kind != C.CUI_BRIDGE_ATTACHMENT {
		return errors.New("RGBA preview requires a live Attachment")
	}
	if n.app != nil {
		return n.applyAttachmentPreview(nil)
	}
	n.attachmentPreview = nil
	return nil
}

// AttachmentActions enables independent Open, Remove and Retry controls.
// Each notification carries exactly one of these bits.
type AttachmentActions uint32

const (
	AttachmentOpen       AttachmentActions = C.CUI_BRIDGE_ATTACHMENT_OPEN_V1
	AttachmentRemove     AttachmentActions = C.CUI_BRIDGE_ATTACHMENT_REMOVE_V1
	AttachmentRetry      AttachmentActions = C.CUI_BRIDGE_ATTACHMENT_RETRY_V1
	AttachmentAllActions                   = AttachmentOpen | AttachmentRemove | AttachmentRetry
)

// AttachmentActionPart identifies the input control when its origin is known.
type AttachmentActionPart uint32

const (
	AttachmentPartUnknown          AttachmentActionPart = C.CUI_BRIDGE_ATTACHMENT_ACTION_PART_UNKNOWN_V1
	AttachmentPartCard             AttachmentActionPart = C.CUI_BRIDGE_ATTACHMENT_ACTION_PART_CARD_V1
	AttachmentPartRemove           AttachmentActionPart = C.CUI_BRIDGE_ATTACHMENT_ACTION_PART_REMOVE_V1
	AttachmentPartMediaRetry       AttachmentActionPart = C.CUI_BRIDGE_ATTACHMENT_ACTION_PART_MEDIA_RETRY_V1
	AttachmentPartDescriptionRetry AttachmentActionPart = C.CUI_BRIDGE_ATTACHMENT_ACTION_PART_DESCRIPTION_RETRY_V1
)

// AttachmentActionSource records the source of accepted input, when known.
type AttachmentActionSource uint32

const (
	AttachmentSourceUnknown       AttachmentActionSource = C.CUI_BRIDGE_ACTIVATION_SOURCE_UNKNOWN_V1
	AttachmentSourcePointer       AttachmentActionSource = C.CUI_BRIDGE_ACTIVATION_SOURCE_POINTER_V1
	AttachmentSourceKeyboard      AttachmentActionSource = C.CUI_BRIDGE_ACTIVATION_SOURCE_KEYBOARD_V1
	AttachmentSourceAccessibility AttachmentActionSource = C.CUI_BRIDGE_ACTIVATION_SOURCE_ACCESSIBILITY_V1
)

// AttachmentActionNotificationV1 owns the action and its historical evidence.
// Node is an optional live reference. EventIndex is local to Frame.
type AttachmentActionNotificationV1 struct {
	Handle     Handle
	Node       *Node
	Frame      uint64
	Action     AttachmentActions
	Part       AttachmentActionPart
	Source     AttachmentActionSource
	EventIndex *uint32
}

// ComponentNotificationV1 is the opt-in view of the shared event queue. For
// each successfully polled event, exactly one field is nonnil.
type ComponentNotificationV1 struct {
	Value            *ValueNotificationV1
	AttachmentAction *AttachmentActionNotificationV1
}

var ErrAttachmentActionsUnavailable = errors.New("typed Attachment actions require a newer CUI bridge")

// AttachmentActions enables the specified independent controls in a builder chain.
func (n *Node) AttachmentActions(actions AttachmentActions) *Node {
	if n.err == nil {
		n.err = n.SetAttachmentActions(actions)
	}
	return n
}

// SetAttachmentActions enables these actions or clears them all with zero.
// File operations and lifecycle state belong to the app.
func (n *Node) SetAttachmentActions(actions AttachmentActions) error {
	if n == nil || n.removed || n.kind != C.CUI_BRIDGE_ATTACHMENT {
		return errors.New("actions require a live Attachment")
	}
	if actions & ^AttachmentAllActions != 0 {
		return errors.New("invalid Attachment action mask")
	}
	if n.app != nil {
		if err := n.applyAttachmentActions(actions); err != nil {
			return err
		}
	}
	n.attachmentActions = actions
	return nil
}

func decodeAttachmentAction(raw C.cui_bridge_notification_v1, wire C.cui_bridge_attachment_action_v1, node *Node) (AttachmentActionNotificationV1, error) {
	var out AttachmentActionNotificationV1
	const hasIndex = C.CUI_BRIDGE_ACTIVATION_HAS_INDEX_V1
	const hasSource = C.CUI_BRIDGE_ACTIVATION_HAS_SOURCE_V1
	if raw.kind != C.CUI_BRIDGE_NOTIFY_ACTIVATE || raw.node_kind != C.CUI_BRIDGE_ATTACHMENT || raw.frame_sequence == 0 ||
		raw.node.slot == 0 || raw.node.generation == 0 || raw.text_length != 0 || raw.legacy.checked != 0 ||
		raw.legacy.selected != (C.cui_bridge_node{}) || float64(raw.legacy.value) != float64(wire.action) ||
		wire.flags & ^C.uint32_t(hasIndex|hasSource) != 0 {
		return out, errors.New("invalid Attachment action V1 metadata")
	}
	action, part, source := AttachmentActions(wire.action), AttachmentActionPart(wire.part), AttachmentActionSource(wire.source)
	switch action {
	case AttachmentOpen:
		if part != AttachmentPartUnknown && part != AttachmentPartCard {
			return out, errors.New("invalid Attachment Open part")
		}
	case AttachmentRemove:
		if part != AttachmentPartUnknown && part != AttachmentPartRemove {
			return out, errors.New("invalid Attachment Remove part")
		}
	case AttachmentRetry:
		if part != AttachmentPartUnknown && part != AttachmentPartMediaRetry && part != AttachmentPartDescriptionRetry {
			return out, errors.New("invalid Attachment Retry part")
		}
	default:
		return out, errors.New("invalid Attachment action V1 value")
	}
	hasKnownSource := source >= AttachmentSourcePointer && source <= AttachmentSourceAccessibility
	if (wire.flags&hasSource != 0 && !hasKnownSource) || (wire.flags&hasSource == 0 && source != AttachmentSourceUnknown) ||
		(wire.flags&hasIndex == 0 && wire.event_index != 0) {
		return out, errors.New("invalid Attachment action V1 evidence")
	}
	out = AttachmentActionNotificationV1{
		Handle: Handle{uint32(raw.node.slot), uint32(raw.node.generation)}, Node: node,
		Frame: uint64(raw.frame_sequence), Action: action, Part: part, Source: source,
	}
	if wire.flags&hasIndex != 0 {
		index := uint32(wire.event_index)
		out.EventIndex = &index
	}
	return out, nil
}
