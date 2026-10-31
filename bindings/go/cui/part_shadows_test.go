package cui

import (
	"os"
	"testing"
)

func TestPartShadowAuthoring(t *testing.T) {
	a := app(t)
	source := []BoxShadow{{Sigma: 1, RGBA: 0x00000080}}
	check := Checkbox("check").Text("Check").PartShadows(ShadowMain, ShadowStyle{Layers: source})
	source[0].Sigma = 100
	sw := Switch("sw").Text("Switch").PartShadows(ShadowTrack, ShadowStyle{Layers: []BoxShadow{{Sigma: 1}}}).PartShadows(ShadowThumb, ShadowStyle{})
	item := Item("a", "A").PartShadows(ShadowIndicator, ShadowStyle{})
	if e := a.Mount(Column("root").Children(check, sw, RadioGroup("g").Children(item))); e != nil {
		t.Fatal(e)
	}
	if e := a.Step(.016); e != nil {
		t.Fatal(e)
	}
	info, e := check.ShadowTarget(ShadowMain)
	if e != nil || !info.Supported() || info.CanonicalPart != ShadowIndicator {
		t.Fatal(info, e)
	}
	if check.partShadows[ShadowIndicator].Layers[0].Sigma != 1 {
		t.Fatal("borrowed layer slice")
	}
	before, e := a.Bounds(check)
	if e != nil {
		t.Fatal(e)
	}
	calls := 0
	check.OnChange(func(Event) { calls++; check.PartShadows(ShadowIndicator, ShadowStyle{}) })
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
	if calls != 1 || check.Err() != nil {
		t.Fatal("callback mutation failed", check.Err())
	}
	check.PartShadowPreset(ShadowRoot, ShadowSM, CornerRadii{}).PartShadows(ShadowIndicator, ShadowStyle{})
	check.ResetPartShadows(ShadowMain)
	if check.Err() != nil {
		t.Fatal(check.Err())
	}
	if _, ok := check.partShadows[ShadowIndicator]; ok {
		t.Fatal("alias reset retained state")
	}
	if e := a.Step(.016); e != nil {
		t.Fatal(e)
	}
	after, e := a.Bounds(check)
	if e != nil || before != after {
		t.Fatal("geometry changed", e)
	}
	wrong := Item("wrong", "Wrong").PartShadows(ShadowIndicator, ShadowStyle{})
	other := app(t)
	if e := other.Mount(Column("other").Children(Tabs("tabs").Children(wrong))); e == nil {
		t.Fatal("wrong ITEM owner accepted")
	}
	if wrong.app != nil {
		t.Fatal("mount rollback leaked ownership")
	}
	if e := check.Remove(); e != nil {
		t.Fatal(e)
	}
	check.PartShadows(ShadowMain, ShadowStyle{})
	if check.Err() == nil {
		t.Fatal("stale target accepted")
	}
}
func TestPartShadowOldLibraryDefaults(t *testing.T) {
	if os.Getenv("CUI_TEST_OLD_SHADOW_BRIDGE") != "1" {
		t.Skip("actual old library fixture")
	}
	a := app(t)
	plain := Checkbox("plain").Text("Plain")
	if e := a.Mount(Column("root").Children(plain)); e != nil {
		t.Fatal(e)
	}
	if e := a.Step(.016); e != nil {
		t.Fatal(e)
	}
	plain.PartShadows(ShadowIndicator, ShadowStyle{})
	if plain.Err() == nil || len(plain.partShadows) != 0 {
		t.Fatal("missing symbol committed local style")
	}
	if e := a.Step(.016); e != nil {
		t.Fatal(e)
	}
}
