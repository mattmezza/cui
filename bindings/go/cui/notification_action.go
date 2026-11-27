package cui

import "errors"

var ErrNotificationActionUnavailable = errors.New("Notification action requires a newer CUI bridge")

// Action adds the Notification's single ordinary Button. Its click callback leaves
// the Notification open. Removing the Notification removes the Button too.
func (n *Node) Action(button *Node) *Node {
	if n.err != nil {
		return n
	}
	if n.kind != 62 || button == nil || button.kind != 4 || len(n.children) != 0 {
		n.err = errors.New("Notification Action requires one Button")
		return n
	}
	return n.Children(button)
}

func (a *App) validateNotificationAction(n, parent *Node) error {
	if n.kind == 62 && len(n.children) != 0 {
		if len(n.children) != 1 || n.children[0] == nil || n.children[0].kind != 4 {
			return errors.New("Notification requires at most one Button child")
		}
		supported, err := a.SupportsNotificationAction()
		if err != nil {
			return err
		}
		if !supported {
			return ErrNotificationActionUnavailable
		}
	}
	// Derive this on every preflight so moving an unmounted Button stays valid.
	n.notificationAction = parent != nil && parent.kind == 62
	return nil
}

var ErrNotificationAutoHideUnavailable = errors.New("Notification auto-hide requires a newer CUI bridge")
