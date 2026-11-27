package cui

import (
	"errors"
	"math"
	"os"
	"testing"
)

func TestTextDefaults(t *testing.T) {
	a := app(t)
	supported, err := a.SupportsTextDefaults()
	check(t, err)
	if expected, set := os.LookupEnv("CUI_EXPECT_TEXT_DEFAULTS"); set && supported != (expected == "1") {
		t.Fatal("unexpected capability")
	}
	for _, size := range []float32{-1, 4097, float32(math.NaN()), float32(math.Inf(1))} {
		if Column("bad").TextDefaults(TextDefaults{FontSize: size}).Err() == nil {
			t.Fatal("invalid size accepted")
		}
	}
	if Label("bad").TextDefaults(TextDefaults{}).Err() == nil {
		t.Fatal("invalid kind accepted")
	}
	tint := uint32(0)
	root := Column("root")
	label := Label("Inherited label")
	row := Row("nested").Children(Icon("icon", "check"), label, Spinner("busy"))
	root.Children(row)
	if !supported {
		if err := a.Mount(Column("unavailable").TextDefaults(TextDefaults{})); !errors.Is(err, ErrTextDefaultsUnavailable) {
			t.Fatalf("unavailable: %v", err)
		}
		if a.root != nil {
			t.Fatal("partially mounted unavailable tree")
		}
	} else {
		root.TextDefaults(TextDefaults{FontSize: 24, ColorRGBA: &tint})
	}
	check(t, a.Mount(root))
	check(t, a.Step(.016))
	if !supported {
		return
	}
	before, err := a.Bounds(label)
	check(t, err)
	check(t, row.TextDefaults(TextDefaults{FontSize: 40}).Err())
	check(t, a.Step(.016))
	larger, err := a.Bounds(label)
	check(t, err)
	if larger.Height <= before.Height {
		t.Fatalf("size did not inherit: %v %v", before, larger)
	}
	check(t, row.TextDefaults(TextDefaults{}).Err())
	check(t, a.Step(.016))
	reset, err := a.Bounds(label)
	check(t, err)
	if reset.Height != before.Height {
		t.Fatal("reset did not restore inherited size")
	}
}
