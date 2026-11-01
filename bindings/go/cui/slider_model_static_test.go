//go:build linux && !cui_dynamic

package cui

import "testing"

// Run separately without CUI_BRIDGE_LIBRARY: the matching bundled bridge object
// must actually be extracted despite the optional weak references.
func TestSliderStaticClosure(t *testing.T) {
	t.Setenv("CUI_BRIDGE_LIBRARY", "")
	a := app(t)
	yes, e := a.SupportsSliderModel()
	check(t, e)
	if !yes {
		t.Fatal("matching bundled optional symbols were not extracted")
	}
	value, e := NewSliderRange(20, 60)
	check(t, e)
	n := SliderModel("static", DefaultSliderConfig(), value)
	check(t, a.Mount(Column("root").Children(Row("row").Children(n))))
	check(t, a.Step(.016))
	_, present, e := n.SliderPartBounds(SliderEndThumb)
	check(t, e)
	if !present {
		t.Fatal("static typed UI missing")
	}
}
