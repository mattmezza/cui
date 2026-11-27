package cui

import (
	"errors"
	"math"
	"os"
	"testing"
)

func TestRotatingSpinner(t *testing.T) {
	a := app(t)
	supported, err := a.SupportsSpinnerPresentation()
	check(t, err)
	c := DefaultSpinnerConfig()
	n := RotatingSpinner("loading", c)
	if os.Getenv("CUI_TEST_UNSUPPORTED_SPINNER") == "1" {
		if supported {
			t.Fatal("optional group accepted")
		}
		if err := a.Mount(Column("root").Children(n)); !errors.Is(err, ErrSpinnerPresentationUnavailable) {
			t.Fatal(err)
		}
		check(t, a.Mount(Column("legacy").Children(Spinner("classic"))))
		check(t, a.Step(.25))
		return
	}
	if !supported {
		t.Fatal("missing group")
	}
	c.Icon = "loader"
	if got, _ := n.SpinnerConfig(); got.Icon != "" {
		t.Fatal("aliased config")
	}
	check(t, a.Mount(Column("root").Children(n, Spinner("legacy"))))
	got, err := n.SpinnerConfig()
	check(t, err)
	if got != DefaultSpinnerConfig() {
		t.Fatalf("round trip %+v", got)
	}
	if _, ok, e := n.SpinnerPresentation(); e != nil || ok {
		t.Fatal("premature sample", e)
	}
	check(t, a.Step(.25))
	first, ok, err := n.SpinnerPresentation()
	check(t, err)
	if !ok || first.Phase != 0 {
		t.Fatal(first)
	}
	check(t, a.Step(.2))
	next, ok, err := n.SpinnerPresentation()
	check(t, err)
	if !ok || next.Phase <= 0 || first.Phase != 0 {
		t.Fatal(next)
	}
	bad := got
	bad.Duration = float32(math.NaN())
	if n.ConfigureSpinner(bad) == nil {
		t.Fatal("NaN accepted")
	}
	kept, _, err := n.SpinnerPresentation()
	check(t, err)
	if kept != next {
		t.Fatal("invalid update changed sample")
	}
	got.ReducedMotion = true
	got.CustomColor = true
	got.RGBA = 0
	check(t, n.ConfigureSpinner(got))
	check(t, a.Step(.2))
	stopped, ok, err := n.SpinnerPresentation()
	check(t, err)
	if !ok || stopped.NextWakeSeconds != 0 || stopped.Phase != 0 {
		t.Fatal(stopped)
	}
	if n.VisualSize(2, 2).Err() == nil {
		t.Fatal("legacy sizing accepted")
	}
	check(t, n.Remove())
	if _, _, err := n.SpinnerPresentation(); err == nil {
		t.Fatal("removed sample")
	}
}

func TestSpinnerConfigurationValidation(t *testing.T) {
	for _, size := range []SpinnerSize{SpinnerXSmall, SpinnerSmall, SpinnerMedium, SpinnerLarge} {
		c := DefaultSpinnerConfig()
		c.Size = size
		if err := c.validate(); err != nil {
			t.Fatal(err)
		}
	}
	for _, c := range []SpinnerConfig{{Size: SpinnerLarge, RemSize: 3000}, {Size: SpinnerMedium, Icon: "a\x00b"}, {Size: SpinnerCustom, Pixels: 0}, {Size: SpinnerMedium, Easing: 9}} {
		if c.validate() == nil {
			t.Fatalf("invalid accepted %+v", c)
		}
	}
}
