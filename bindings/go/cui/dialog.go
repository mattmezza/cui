package cui

import "errors"

// ErrDialogCompositionUnavailable indicates a provider without the retained dialog API.
var ErrDialogCompositionUnavailable = errors.New("dialog composition requires a newer CUI bridge")

// DialogOptions configures the opt-in layout for Dialog and AlertDialog.
// Zero width, height and margin mean 420 logical units, automatic height and
// 16 logical units, respectively. ShowClose and OutsideDismiss enable features
// that AlertDialog suppresses by default and apply only to AlertDialog.
type DialogOptions struct {
	Width, Height, Margin                       float32
	Centered, NoEscapeDismiss, NoOutsideDismiss bool
	HideClose, ShowClose, OutsideDismiss        bool
}

// DialogSlotRole identifies one independent child region.
type DialogSlotRole uint32

const (
	DialogHeader DialogSlotRole = iota + 1
	DialogTitle
	DialogDescription
	DialogContent
	DialogBody
	DialogFooter
)

// DialogAction specifies the result of activating a direct Footer Button.
type DialogAction uint32

const (
	DialogActionNone DialogAction = iota
	DialogActionConfirm
	DialogActionCancel
	DialogActionClose
)

func (n *Node) isDialog() bool { return n.kind == 24 || n.kind == 25 }

// DialogOptions copies options. Pass nil while closed after removing named
// slots to restore legacy presentation. Apply options before mounting slots.
func (n *Node) DialogOptions(o *DialogOptions) *Node {
	if n.err != nil {
		return n
	}
	if !n.isDialog() {
		n.err = errors.New("options require Dialog or AlertDialog")
		return n
	}
	if o != nil {
		for _, value := range []float32{o.Width, o.Height, o.Margin} {
			if !finite(value) || value < 0 || value > 4096 {
				n.err = errors.New("invalid dialog dimension")
				return n
			}
		}
		if n.kind != 25 && (o.ShowClose || o.OutsideDismiss) {
			n.err = errors.New("ShowClose and OutsideDismiss require AlertDialog")
			return n
		}
		copy := *o
		o = &copy
	}
	if n.app != nil {
		if e := n.configureDialog(o); e != nil {
			n.err = e
			return n
		}
	}
	n.dialogOptions = o
	return n
}

// DialogSlot creates one container to mount directly under a configured dialog.
// Footer is a row; the other regions are columns. Set an accessible title and
// description on the dialog even when using visual Title and Description slots.
func DialogSlot(key string, role DialogSlotRole) *Node {
	n := Column(key)
	if role == DialogFooter {
		n = Row(key)
		n.gap = 8
	}
	if role < DialogHeader || role > DialogFooter {
		n.err = errors.New("invalid dialog slot role")
	}
	n.dialogSlot = role
	return n
}

// DialogAction preserves the Button click and queues the corresponding close
// result on its owning dialog. Use an ordinary Button for app validation.
func (n *Node) DialogAction(action DialogAction) *Node {
	if n.err != nil {
		return n
	}
	if n.kind != 4 || action > DialogActionClose {
		n.err = errors.New("invalid dialog action or non-Button node")
		return n
	}
	if n.app != nil {
		if e := n.setDialogAction(action); e != nil {
			n.err = e
			return n
		}
	}
	n.dialogAction = action
	return n
}

func (a *App) validateDialogComposition(n, parent *Node) error {
	if n.dialogSlot != 0 && (parent == nil || parent.dialogOptions == nil) {
		return errors.New("dialog slot requires a configured direct Dialog or AlertDialog parent")
	}
	if n.dialogAction != DialogActionNone && (parent == nil || parent.dialogSlot != DialogFooter) {
		return errors.New("dialog action Button requires a direct Footer parent")
	}
	if n.dialogOptions == nil {
		return nil
	}
	supported, err := a.SupportsDialogComposition()
	if err != nil {
		return err
	}
	if !supported {
		return ErrDialogCompositionUnavailable
	}
	var seen [DialogFooter + 1]bool
	for _, child := range n.children {
		if child == nil || child.err != nil {
			continue // Validated when visiting the child.
		}
		role := child.dialogSlot
		if role == 0 {
			if n.kind == 25 {
				return errors.New("configured AlertDialog children must be dialog slots")
			}
			continue
		}
		if seen[role] {
			return errors.New("duplicate dialog slot role")
		}
		seen[role] = true
	}
	return nil
}
