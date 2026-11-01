package cui

import (
	"os"
	"strings"
	"testing"
)

// These values come from a tiny test provider over a real old bridge, not UI input.
func TestSliderOptionalProtocol(t *testing.T) {
	mode := os.Getenv("CUI_SLIDER_PROTOCOL")
	if mode == "" {
		t.Skip("requires explicit protocol provider in a separate process")
	}
	a := app(t)
	supported, e := a.SupportsSliderModel()
	check(t, e)
	unavailable := mode == "missing" || mode == "unavailable" || strings.HasSuffix(mode, "_size")
	if unavailable {
		if supported {
			t.Fatal("partial/unsupported group advertised")
		}
		n := SliderModel("typed", DefaultSliderConfig(), sliderTestValue(t, true))
		if a.Mount(Column("root").Children(n)) == nil || len(a.nodes) != 0 || a.root != nil {
			t.Fatal("partial unsupported mount")
		}
		legacy := Slider("old").Range(0, 100, 1).Value(40)
		check(t, a.Mount(Column("legacy").Children(legacy)))
		check(t, a.Step(.016))
		return
	}
	if !supported {
		t.Fatal("complete group unavailable")
	}
	check(t, a.EnableSliderRequests(4096))
	r, ok, e := a.PollSliderRequest()
	if mode != "edges" {
		if e == nil || ok || r.Receipt.namespace != nil {
			t.Fatal("malformed copy published", mode, r, e)
		}
		return
	}
	check(t, e)
	if !ok {
		t.Fatal("missing synthetic request")
	}
	i := r.Receipt.Info()
	if i.AppEpoch != ^uint64(0) || i.SourceGeneration != ^uint64(0) || i.Slot != ^uint32(0) || i.Generation != ^uint32(0) || i.ConfigurationRevision != ^uint64(0) || i.SubmissionFrame != ^uint64(0) || i.RequestSequence != ^uint64(0) || r.EventTarget != ^uint64(0) || r.EventFrame != ^uint64(0) || r.EventIndex != ^uint64(0) || r.Input.PointerID != ^uint64(0) || r.Input.PointerDevice != ^uint64(0) {
		t.Fatal("identity narrowed", r)
	}
	if r.Current.Start() != 20 || r.Current.End() != 60 || r.Requested.End() != 59 || r.Input.Modifiers != 15 {
		t.Fatal(r)
	}
	info := r.Receipt.Info()
	info.AppEpoch = 0
	if r.Receipt.Info().AppEpoch != ^uint64(0) {
		t.Fatal("borrowed receipt metadata")
	}
	check(t, a.Close())
	if r.Current.End() != 60 || r.Receipt.Info().AppEpoch != ^uint64(0) {
		t.Fatal("copied lifetime")
	}
}
