package cui

import (
	"os"
	"strings"
	"testing"
)

func TestKbdStrokeValidation(t *testing.T) {
	options := KbdStrokeOptions{Modifiers: KbdCtrl | KbdShift, AccessibleName: "Save document"}
	n := KbdStroke("save", "é", options)
	check(t, n.Err())
	for _, key := range []string{"", "bad\x00key", "a\tb", "\u2028", "\xff", strings.Repeat("é", 65)} {
		if n.ConfigureKbdStroke(key, options) == nil {
			t.Fatalf("accepted key %q", key)
		}
	}
	if n.kbdStroke.key != "é" {
		t.Fatal("failed update changed key")
	}
	options.Modifiers = 16
	if n.ConfigureKbdStroke("S", options) == nil {
		t.Fatal("accepted unknown modifiers")
	}
	options.Modifiers = 0
	options.AccessibleName = strings.Repeat("x", 1025)
	if n.ConfigureKbdStroke("S", options) == nil {
		t.Fatal("accepted long name")
	}
}
func TestKbdStrokeProvider(t *testing.T) {
	a := app(t)
	supported, err := a.SupportsKbdStroke()
	check(t, err)
	if expected, ok := os.LookupEnv("CUI_EXPECT_KBD_STROKE"); ok && supported != (expected == "1") {
		t.Fatalf("typed Kbd capability %v, expected %s", supported, expected)
	}
	n := KbdStroke("save", "S", KbdStrokeOptions{Modifiers: KbdCtrl, Outline: true})
	root := Column("root").Children(n, Kbd("legacy", "Ctrl+P"))
	if !supported {
		if err := a.Mount(root); err != ErrKbdStrokeUnavailable {
			t.Fatalf("missing capability: %v", err)
		}
		check(t, a.Mount(Column("ordinary").Children(Kbd("legacy", "Ctrl+P"))))
		return
	}
	check(t, a.Mount(root))
	check(t, a.Step(0.016))
	b, err := a.Bounds(n)
	check(t, err)
	if b.Width <= 0 || b.Height <= 0 {
		t.Fatalf("empty shortcut: %+v", b)
	}
	check(t, n.ConfigureKbdStroke("Enter", KbdStrokeOptions{Modifiers: KbdMeta, MacSymbols: true, AccessibleName: "Confirm"}))
	check(t, a.Step(0.016))
}
