package cui

import (
	"errors"
	"math"
	"os"
	"reflect"
	"strings"
	"testing"
)

func sliderTestValue(t *testing.T, single bool) SliderValue {
	t.Helper()
	var v SliderValue
	var e error
	if single {
		v, e = NewSliderSingle(60)
	} else {
		v, e = NewSliderRange(20, 60)
	}
	check(t, e)
	return v
}
func sliderTestNode(t *testing.T, single, vertical bool) (*App, *Node) {
	t.Helper()
	a := app(t)
	c := DefaultSliderConfig()
	c.SourceGeneration = 0xfedcba9876543210
	if vertical {
		c.Orientation = SliderVertical
	}
	n := SliderModel("typed", c, sliderTestValue(t, single))
	check(t, a.Mount(Column("root").Children(Row("row").Children(n))))
	check(t, a.Step(.016))
	return a, n
}
func sliderTestFocus(t *testing.T, a *App, n *Node) {
	t.Helper()
	b, ok, e := n.SliderPartBounds(SliderEndThumb)
	check(t, e)
	if !ok || b.Width <= 0 || b.Height <= 0 {
		t.Fatal("missing actual thumb")
	}
	check(t, a.QueuePointer(PointerDown, b.X+b.Width/2, b.Y+b.Height/2))
	check(t, a.QueuePointer(PointerUp, b.X+b.Width/2, b.Y+b.Height/2))
	check(t, a.Step(.016))
	for {
		_, ok, e := a.PollSliderRequest()
		check(t, e)
		if !ok {
			break
		}
	}
}
func TestSliderControlledPairs(t *testing.T) {
	for _, single := range []bool{false, true} {
		for _, vertical := range []bool{false, true} {
			t.Run(strings.Join([]string{map[bool]string{true: "single", false: "range"}[single], map[bool]string{true: "vertical", false: "horizontal"}[vertical]}, "/"), func(t *testing.T) {
				a, n := sliderTestNode(t, single, vertical)
				sliderTestFocus(t, a, n)
				before, e := n.SliderValue()
				check(t, e)
				var requests []SliderRequest
				n.OnSliderRequest(func(r SliderRequest) error {
					requests = append(requests, r)
					if r.Phase == SliderUpdate {
						var v SliderValue
						var e error
						if single {
							v, e = NewSliderSingle(55)
						} else {
							v, e = NewSliderRange(25, 55)
						}
						if e != nil {
							return e
						}
						return a.AcceptSliderRequest(r.Receipt, v)
					}
					return nil
				})
				key := Left
				if vertical {
					key = Down
				}
				check(t, a.QueueKey(KeyDown, key, 15, true))
				check(t, a.QueueKey(KeyDown, key, 15, true))
				check(t, a.Step(.016))
				if len(requests) != 0 {
					t.Fatal("callback during step")
				}
				count, e := a.Dispatch()
				check(t, e)
				if count != 6 || len(requests) != 6 {
					t.Fatal(count, len(requests))
				}
				phases := []SliderPhase{}
				for _, r := range requests {
					phases = append(phases, r.Phase)
					if r.Current != before || r.Receipt.Info().SourceGeneration != 0xfedcba9876543210 || r.Input.Modifiers != 15 || r.Input.Repeat != 1 {
						t.Fatal(r)
					}
				}
				if !reflect.DeepEqual(phases, []SliderPhase{SliderBegin, SliderUpdate, SliderCommit, SliderBegin, SliderUpdate, SliderCommit}) {
					t.Fatal(phases)
				}
				if requests[1].Requested == requests[4].Requested {
					t.Fatal("intent did not evolve")
				}
				owner, e := n.SliderValue()
				check(t, e)
				if owner.End() != 55 {
					t.Fatal(owner)
				}
				check(t, a.Step(.016))
				if a.AcceptSliderRequest(requests[1].Receipt, before) == nil {
					t.Fatal("expired receipt accepted")
				}
			})
		}
	}
}
func TestSliderControlledCopiesAndPolicy(t *testing.T) {
	a, n := sliderTestNode(t, false, false)
	sliderTestFocus(t, a, n)
	check(t, a.QueueKey(KeyDown, Left, 0, false))
	check(t, a.Step(.016))
	var saved []SliderRequest
	for {
		r, ok, e := a.PollSliderRequest()
		check(t, e)
		if !ok {
			break
		}
		saved = append(saved, r)
	}
	if len(saved) != 3 {
		t.Fatal(len(saved))
	}
	owner, e := n.SliderValue()
	check(t, e)
	if owner.End() != 60 {
		t.Fatal("poll accepted")
	}
	v, e := NewSliderRange(20, math.Nextafter(60, math.Inf(1)))
	check(t, e)
	check(t, a.AcceptSliderRequest(saved[1].Receipt, v))
	got, e := n.SliderValue()
	check(t, e)
	if got != v {
		t.Fatal("double narrowed", got, v)
	}
	forged := saved[2].Receipt
	forged.info.RequestSequence = ^uint64(0)
	if a.AcceptSliderRequest(forged, v) == nil {
		t.Fatal("forged sequence accepted")
	}
	if a.AcceptSliderRequest(SliderReceipt{}, v) == nil {
		t.Fatal("empty authority accepted")
	}
	c, e := n.SliderConfig()
	check(t, e)
	c.Readonly = true
	check(t, n.ConfigureSlider(c, v))
	if a.AcceptSliderRequest(saved[2].Receipt, v) == nil {
		t.Fatal("reconfigured receipt accepted")
	}
	check(t, a.Step(.016))
	check(t, a.QueueKey(KeyDown, Left, 0, false))
	check(t, a.Step(.016))
	_, ok, e := a.PollSliderRequest()
	check(t, e)
	if ok {
		t.Fatal("readonly emitted")
	}
	n.Disabled(true)
	check(t, n.err)
	check(t, a.Step(.016))
	_, ok, e = n.SliderPartBounds(SliderEndThumb)
	check(t, e)
	if ok {
		t.Fatal("disabled thumb present")
	}
	for _, part := range []SliderPart{SliderTrack, SliderFill} {
		_, ok, e = n.SliderPartBounds(part)
		check(t, e)
		if !ok {
			t.Fatal("disabled track/fill missing")
		}
	}
	check(t, a.Close())
	if saved[1].Receipt.Info().SourceGeneration != 0xfedcba9876543210 || saved[1].Current.End() != 60 {
		t.Fatal("copy lifetime")
	}
}
func TestSliderControlledCallbackLifecycle(t *testing.T) {
	for _, mode := range []string{"error", "panic", "remove", "close", "request_close"} {
		t.Run(mode, func(t *testing.T) {
			a, n := sliderTestNode(t, false, false)
			sliderTestFocus(t, a, n)
			var seen []SliderPhase
			n.OnSliderRequest(func(r SliderRequest) error {
				seen = append(seen, r.Phase)
				if a.Step(.016) == nil || a.Run(.016) == nil {
					t.Fatal("recursive execution")
				}
				if _, e := a.Dispatch(); e == nil {
					t.Fatal("recursive dispatch")
				}
				if _, e := a.DispatchSliderRequests(); e == nil {
					t.Fatal("recursive typed dispatch")
				}
				if r.Phase == SliderUpdate {
					switch mode {
					case "error":
						return errors.New("sentinel")
					case "panic":
						panic("sentinel")
					case "remove":
						return n.Remove()
					case "close":
						check(t, a.AcceptSliderRequest(r.Receipt, r.Requested))
						return a.Close()
					case "request_close":
						return a.RequestClose()
					}
				}
				return nil
			})
			check(t, a.QueueKey(KeyDown, Right, 0, false))
			check(t, a.Step(.016))
			var caught any
			var e error
			func() { defer func() { caught = recover() }(); _, e = a.Dispatch() }()
			if mode == "error" && (e == nil || e.Error() != "sentinel") {
				t.Fatal(e)
			}
			if mode == "panic" && caught != "sentinel" {
				t.Fatal(caught)
			}
			if a.sliderDispatching {
				t.Fatal("guard retained")
			}
			if len(seen) != 2 {
				t.Fatal(seen)
			}
			if mode == "remove" || mode == "close" {
				if n.slider.callback != nil {
					t.Fatal("handler retained")
				}
			}
			if mode == "error" || mode == "panic" {
				r, ok, e := a.PollSliderRequest()
				check(t, e)
				if !ok || r.Phase != SliderCommit {
					t.Fatal("later queue lost", r)
				}
			}
		})
	}
}
func TestSliderControlledThreadAndPreflight(t *testing.T) {
	a := app(t)
	c := DefaultSliderConfig()
	n := SliderModel("typed", c, sliderTestValue(t, true))
	if a.Mount(RootView("root").Children(n)) == nil || a.root != nil || len(a.nodes) != 0 {
		t.Fatal("partial preflight")
	}
	errch := make(chan error, 1)
	go func() { _, e := a.SupportsSliderModel(); errch <- e }()
	if e := <-errch; e == nil {
		t.Fatal("wrong-thread typed access")
	}
	c.Layout = SliderParent
	c.Length = float32(math.NaN())
	n = SliderModel("parent", c, sliderTestValue(t, true))
	if n.err != nil || n.slider.config.Length != 0 {
		t.Fatal(n.err)
	}
	check(t, a.Mount(Column("root").Children(n)))
	check(t, a.Step(.016))
	cfg, e := n.SliderConfig()
	check(t, e)
	if cfg.Length != 0 {
		t.Fatal(cfg)
	}
}
func TestSliderOldLibraryFallback(t *testing.T) {
	if os.Getenv("CUI_TEST_OLD_SLIDER") != "1" {
		t.Skip("requires explicit actual pre-typed library in separate process")
	}
	a := app(t)
	yes, e := a.SupportsSliderModel()
	check(t, e)
	if yes {
		t.Fatal("old library advertised typed support")
	}
	n := SliderModel("typed", DefaultSliderConfig(), sliderTestValue(t, true))
	if a.Mount(Column("typedroot").Children(n)) == nil || a.root != nil || len(a.nodes) != 0 {
		t.Fatal("partial unsupported mount")
	}
	legacy := Slider("volume").Range(0, 100, 1).Value(40)
	check(t, a.Mount(Column("legacyroot").Children(legacy)))
	check(t, a.Step(.016))
	v, e := a.Value(legacy)
	check(t, e)
	if v != 40 {
		t.Fatal(v)
	}
}

func TestSliderControlledDomainPreflight(t *testing.T) {
	for _, tc := range []struct {
		name   string
		domain SliderDomain
		value  float64
	}{
		{"overflow_span", SliderDomain{Minimum: -1e308, Maximum: 1e308}, 0},
		{"constant_log", SliderDomain{Minimum: 1, Maximum: 1, Scale: SliderLogarithmic}, 1},
	} {
		t.Run(tc.name, func(t *testing.T) {
			a := app(t)
			cfg := DefaultSliderConfig()
			cfg.Domain = tc.domain
			v, e := NewSliderSingle(tc.value)
			check(t, e)
			valid, e := NewSliderSingle(0)
			check(t, e)
			pending := SliderModel("pending", DefaultSliderConfig(), valid)
			if pending.ConfigureSlider(cfg, v) == nil {
				t.Fatal("unmounted configuration accepted a domain rejected by C")
			}
			stored, e := pending.SliderConfig()
			check(t, e)
			if stored != DefaultSliderConfig() {
				t.Fatal("rejected configuration mutated pending builder")
			}
			n := SliderModel("invalid", cfg, v)
			if a.Mount(Column("untouched").Children(n)) == nil || a.root != nil || len(a.nodes) != 0 {
				t.Fatal("invalid domain must reject before creating ancestor nodes", len(a.nodes))
			}
		})
	}
	for _, d := range []SliderDomain{{Minimum: 1, Maximum: 1}, {Minimum: 1, Maximum: math.Nextafter(1, 2), Scale: SliderLogarithmic}, {Minimum: -1e307, Maximum: 1e307}} {
		if e := d.validate(); e != nil {
			t.Fatal("valid boundary rejected", d, e)
		}
	}
}
