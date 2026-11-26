package cui

import "errors"

// AttachmentSlotRole identifies one independently composed part of a file card.
type AttachmentSlotRole uint32

const (
	AttachmentSlotMedia AttachmentSlotRole = iota + 1
	AttachmentSlotOverlay
	AttachmentSlotMetadata
	AttachmentSlotActions
)

// ErrAttachmentSlotsUnavailable indicates that the provider lacks the slot factory.
var ErrAttachmentSlotsUnavailable = errors.New("Attachment slots require a newer CUI bridge")

// AttachmentSlot creates a normal container for a direct Attachment child.
// Only actions takes a preferred width, in (0,4096]; other roles require zero.
// Its descendants use ordinary controls and callbacks. A decoded preview takes
// precedence over fallback media; overlay content remains independent.
func AttachmentSlot(key string, role AttachmentSlotRole, preferredWidth float32) *Node {
	n := Column(key)
	if role == AttachmentSlotActions {
		n = Row(key)
	}
	if role < AttachmentSlotMedia || role > AttachmentSlotActions || !finite(preferredWidth) ||
		(role == AttachmentSlotActions && (preferredWidth <= 0 || preferredWidth > 4096)) ||
		(role != AttachmentSlotActions && preferredWidth != 0) {
		n.err = errors.New("invalid Attachment slot role or preferred width")
	}
	n.attachmentSlot, n.attachmentSlotWidth = role, preferredWidth
	return n
}

func (a *App) validateAttachmentSlots(n, parent *Node) error {
	if n.attachmentSlot != 0 {
		if parent == nil || parent.kind != 52 {
			return errors.New("Attachment slot requires a direct Attachment parent")
		}
		supported, err := a.SupportsAttachmentSlots()
		if err != nil {
			return err
		}
		if !supported {
			return ErrAttachmentSlotsUnavailable
		}
	}
	if n.kind == 52 {
		roles := map[AttachmentSlotRole]bool{}
		for _, child := range n.children {
			if child == nil || child.attachmentSlot == 0 {
				return errors.New("Attachment children must be slots")
			}
			if roles[child.attachmentSlot] {
				return errors.New("duplicate Attachment slot role")
			}
			roles[child.attachmentSlot] = true
		}
	}
	return nil
}
