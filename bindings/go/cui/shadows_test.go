package cui

import (
	"math"
	"os"
	"strings"
	"testing"
)

func TestShadowAuthoring(t *testing.T) {
	a := app(t)
	bad := Button("bad-mount").BoxShadows(ShadowStyle{})
	bad.kind = 5
	if e := a.Mount(Column("bad-root").Children(bad)); e == nil {
		t.Fatal("wrong-kind mount accepted")
	}
	if bad.app != nil || a.root != nil {
		t.Fatal("mount rollback retained ownership")
	}
	layer, e := a.ShadowCSS(0, 2, 8, -1, 0x10203040, ShadowDrop)
	if e != nil || layer.Sigma != 4 {
		t.Fatalf("CSS=%+v %v", layer, e)
	}
	preset, e := a.ShadowPreset(ShadowMD, CornerRadii{1, 2, 3, 4})
	wire, wireErr := shadowWire(preset)
	if wireErr != nil || wire.top_left != 1 || wire.top_right != 2 || wire.bottom_left != 3 || wire.bottom_right != 4 {
		t.Fatal("radii order mismatch")
	}
	if e != nil || len(preset.Layers) != 2 {
		t.Fatal(e)
	}
	layers := []BoxShadow{layer, {Kind: ShadowInset, RGBA: 0x00000040}}
	target := Button("target").Text("Go").BoxShadows(ShadowStyle{Layers: layers})
	layers[0].Sigma = 200
	if target.shadowStyle.Layers[0].Sigma != 4 {
		t.Fatal("borrowed caller slice")
	}
	if e := a.Mount(Column("root").Children(target)); e != nil {
		t.Fatal(e)
	}
	if e := a.Step(.016); e != nil {
		t.Fatal(e)
	}
	before, e := a.Bounds(target)
	if e != nil {
		t.Fatal(e)
	}
	calls := 0
	target.OnClick(func(Event) { calls++; target.BoxShadows(ShadowStyle{}) })
	if e := a.QueuePointer(PointerDown, before.X+before.Width/2, before.Y+before.Height/2); e != nil {
		t.Fatal(e)
	}
	if e := a.QueuePointer(PointerUp, before.X+before.Width/2, before.Y+before.Height/2); e != nil {
		t.Fatal(e)
	}
	if e := a.Step(.016); e != nil {
		t.Fatal(e)
	}
	if _, e := a.Dispatch(); e != nil {
		t.Fatal(e)
	}
	if calls != 1 || len(target.shadowStyle.Layers) != 0 {
		t.Fatal("callback mutation not applied")
	}
	target.BoxShadows(preset)
	if target.Err() != nil {
		t.Fatal(target.Err())
	}
	if e := a.Step(.016); e != nil {
		t.Fatal(e)
	}
	after, e := a.Bounds(target)
	if e != nil || after != before {
		t.Fatal("decoration changed geometry", e)
	}
	target.ResetBoxShadows()
	if target.Err() != nil || target.shadowStyle != nil {
		t.Fatal("reset failed")
	}
	for _, bad := range []*Node{Checkbox("no").BoxShadows(ShadowStyle{}), Button("bad").BoxShadows(ShadowStyle{Layers: make([]BoxShadow, 9)}), Button("nan").BoxShadows(ShadowStyle{Layers: []BoxShadow{{Sigma: math.Inf(1)}}})} {
		if bad.Err() == nil {
			t.Fatal("invalid style accepted")
		}
	}
	if _, e := a.ShadowCSS(1e300, 0, 0, 0, 0, ShadowDrop); e == nil {
		t.Fatal("float overflow accepted")
	}
	if e := target.Remove(); e != nil {
		t.Fatal(e)
	}
	target.ResetBoxShadows()
	if target.Err() == nil {
		t.Fatal("stale setter accepted")
	}
}

// Run against a pre-shadow runtime with CUI_TEST_OLD_SHADOW_BRIDGE=1.
func TestShadowOldLibraryDefaults(t *testing.T) {
	if os.Getenv("CUI_TEST_OLD_SHADOW_BRIDGE") != "1" {
		t.Skip("explicit old-runtime fixture")
	}
	a := app(t)
	pending := Button("pending").BoxShadows(ShadowStyle{})
	if e := a.Mount(Column("pending-root").Children(pending)); e == nil || !strings.Contains(e.Error(), "shadow V1 unavailable") {
		t.Fatalf("missing-symbol mount: %v", e)
	}
	if pending.app != nil || a.root != nil {
		t.Fatal("missing-symbol mount retained nodes")
	}
	plain := Button("plain").Text("Old defaults")
	if e := a.Mount(Column("root").Children(plain)); e != nil {
		t.Fatal(e)
	}
	if e := a.Step(.016); e != nil {
		t.Fatal(e)
	}
	plain.BoxShadows(ShadowStyle{})
	if plain.Err() == nil || plain.shadowStyle != nil {
		t.Fatal("missing-symbol setter committed state")
	}
	if _, e := a.ShadowCSS(0, 0, 1, 0, 0, ShadowDrop); e == nil {
		t.Fatal("missing CSS symbol accepted")
	}
	if e := a.Step(.016); e != nil {
		t.Fatal(e)
	}
}
