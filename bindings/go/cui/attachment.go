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
