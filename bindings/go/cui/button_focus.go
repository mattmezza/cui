package cui

import "errors"

// ButtonFocusMode controls the retained Button's keyboard focus presentation.
type ButtonFocusMode uint32

const (
	ButtonFocusLegacy ButtonFocusMode = iota
	ButtonFocusVariant
	ButtonFocusOuterRing
)

var ErrButtonFocusUnavailable = errors.New("button focus treatment requires a newer CUI bridge")
