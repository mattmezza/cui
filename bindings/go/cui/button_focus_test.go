package cui

import (
	"errors"
	"os"
	"testing"
)

func TestButtonFocus(t *testing.T) {
	a := app(t)
	supported, err := a.SupportsButtonFocus()
	check(t, err)
	legacy := Button("legacy").ButtonFocus(ButtonFocusLegacy)
	if os.Getenv("CUI_TEST_UNSUPPORTED_BUTTON_FOCUS") == "1" {
		if supported {
			t.Fatal("unexpected optional support")
		}
		check(t, a.Mount(Column("root").Children(legacy)))
		check(t, legacy.ButtonFocus(ButtonFocusLegacy).Err())
		if !errors.Is(legacy.ButtonFocus(ButtonFocusOuterRing).Err(), ErrButtonFocusUnavailable) {
			t.Fatal("missing unavailable error")
		}
		check(t, a.Step(0.01))
		return
	}
	if !supported {
		t.Fatal("missing Button focus setter")
	}
	n := Button("action").Text("Save").ButtonFocus(ButtonFocusVariant)
	check(t, a.Mount(Column("root").Children(n, legacy)))
	for _, mode := range []ButtonFocusMode{ButtonFocusOuterRing, ButtonFocusLegacy, ButtonFocusVariant} {
		check(t, n.ButtonFocus(mode).Err())
		check(t, a.Step(0.01))
	}
	if n.ButtonFocus(ButtonFocusMode(3)).Err() == nil {
		t.Fatal("invalid mode accepted")
	}
	if Label("wrong").ButtonFocus(ButtonFocusOuterRing).Err() == nil {
		t.Fatal("wrong kind accepted")
	}
}
