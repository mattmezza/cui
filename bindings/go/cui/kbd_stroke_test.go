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

func TestKbdStyleOwnershipAndClear(t *testing.T) {
	width := float32(80)
	g := &KbdTextGradient{X1: 1, Stops: []KbdGradientStop{{0, 0}, {1, 0xffffffff}}}
	n := Kbd("literal", "Ctrl+S").KbdStyle(KbdStyleOptions{MaxWidth: &width, TextGradient: g})
	if n.err != nil {
		t.Fatal(n.err)
	}
	width = 0
	g.Stops[0].Position = 2
	if n.kbdStyle.max_width != 80 || n.kbdStyle.stops[0].position != 0 {
		t.Fatal("style must be copied")
	}
	if n.ConfigureKbdStyle(KbdStyleOptions{MaxWidth: &width}) == nil {
		t.Fatal("zero cap accepted")
	}
	if n.kbdStyle.max_width != 80 {
		t.Fatal("failed update changed style")
	}
	check(t, n.ClearKbdStyle())
	if n.kbdStyle != nil {
		t.Fatal("clear retained capability requirement")
	}
}
func TestKbdStyleProvider(t *testing.T) {
	a := app(t)
	supported, err := a.SupportsKbdStyle()
	check(t, err)
	if expected, ok := os.LookupEnv("CUI_EXPECT_KBD_STYLE"); ok && supported != (expected == "1") {
		t.Fatalf("style capability is %v, expected %s", supported, expected)
	}
	n := Kbd("literal", "Ctrl+S").KbdStyle(KbdStyleOptions{Padding: &KbdPadding{0, 0}, Fill: true})
	err = a.Mount(Column("root").Children(n))
	if !supported {
		if err != ErrKbdStyleUnavailable {
			t.Fatalf("unexpected mount: %v", err)
		}
		check(t, n.ClearKbdStyle())
		check(t, a.Mount(Column("plain").Children(n)))
		return
	}
	check(t, err)
	check(t, n.ClearKbdStyle())
	check(t, n.ConfigureKbdStyle(KbdStyleOptions{Monospace: true}))
}

func TestKbdStyleGradientValidation(t *testing.T) {
	valid := KbdTextGradient{X1: 1, Stops: []KbdGradientStop{{0, 0}, {1, 0xffffffff}}}
	if err := (KbdStyleOptions{TextGradient: &valid}).validate(); err != nil {
		t.Fatal(err)
	}
	for _, g := range []KbdTextGradient{
		{X1: 1, Stops: valid.Stops[:1]},
		{Stops: valid.Stops},
		{X1: 2, Stops: valid.Stops},
		{X1: 1, Stops: []KbdGradientStop{{1, 0}, {0, 0}}},
	} {
		if (KbdStyleOptions{TextGradient: &g}).validate() == nil {
			t.Fatal("invalid gradient accepted")
		}
	}
	color := uint32(0)
	if (KbdStyleOptions{TextGradient: &valid, TextColor: &color}).validate() == nil {
		t.Fatal("solid plus gradient accepted")
	}
}
