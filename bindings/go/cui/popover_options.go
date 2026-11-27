package cui

import (
	"errors"
	"math"
)

var (
	ErrPopoverOptionsUnavailable          = errors.New("Popover options unavailable")
	ErrPopoverSecondaryTriggerUnavailable = errors.New("Popover secondary trigger unavailable")
	ErrPointerButtonUnavailable           = errors.New("non-primary pointer input unavailable")
)

type PointerButton uint32

const (
	PointerPrimary PointerButton = iota
	PointerSecondary
	PointerMiddle
)

type PopoverAnchor uint32

const (
	PopoverTopLeft PopoverAnchor = iota
	PopoverTopCenter
	PopoverTopRight
	PopoverBottomLeft
	PopoverBottomCenter
	PopoverBottomRight
	PopoverLeftCenter
	PopoverRightCenter
)

// PopoverOptions configures anchored placement. Zero dimensions use native defaults;
// ReferenceRem zero means 16. Offset is used only with CustomOffset.
type PopoverOptions struct {
	Anchor                                           PopoverAnchor
	Width, Height, Margin, Offset, ReferenceRem      float32
	Arrow, HideClose, KeepPointerFocus, FocusContent bool
	NoEscapeDismiss, NoOutsideDismiss, CustomOffset  bool
	SecondaryTrigger                                 bool // Toggle on a routed right-click.
}

func (o PopoverOptions) validate() error {
	if o.Anchor > PopoverRightCenter {
		return errors.New("invalid Popover anchor")
	}
	for _, v := range []struct{ x, lo, hi float32 }{{o.Width, 0, 4096}, {o.Height, 0, 4096}, {o.Margin, 0, 4096}, {o.Offset, -4096, 4096}, {o.ReferenceRem, 0, 256}} {
		if math.IsNaN(float64(v.x)) || math.IsInf(float64(v.x), 0) || v.x < v.lo || v.x > v.hi {
			return errors.New("invalid Popover dimension")
		}
	}
	return nil
}

// PopoverOptions copies options; nil restores legacy presentation.
func (n *Node) PopoverOptions(o *PopoverOptions) *Node {
	if n.err != nil {
		return n
	}
	if n.kind != 27 {
		n.err = errors.New("options require Popover")
		return n
	}
	if o != nil {
		if e := o.validate(); e != nil {
			n.err = e
			return n
		}
		copy := *o
		o = &copy
	}
	if n.app != nil {
		if e := n.configurePopover(o); e != nil {
			n.err = e
			return n
		}
	}
	n.popoverOptions = o
	return n
}
