package cui

import (
	"errors"
	"math"
	"os"
	"testing"
)

func TestResponsiveSkeletonGeometry(t *testing.T) {
	for _, width := range []float32{320, 480} {
		a, err := New(Options{Width: width})
		check(t, err)
		t.Cleanup(func() {
			if !a.closed {
				check(t, a.Close())
			}
		})
		supported, err := a.SupportsSkeletonPresentation()
		check(t, err)
		if !supported {
			t.Fatal("responsive Skeleton group missing")
		}
		config := DefaultSkeletonConfig()
		config.Width, config.Radius, config.Secondary, config.CustomColor = .65, 6, true, true
		line := ResponsiveSkeleton("line", config)
		capped := ResponsiveSkeleton("cap", SkeletonConfig{WidthMode: SkeletonGrow, Height: 12, MaxWidth: 128})
		circle := ResponsiveSkeleton("circle", SkeletonConfig{Width: 40, Height: 40, Radius: 20})
		legacy := Skeleton("legacy").VisualSize(72, 12)
		owned := config
		config.Width = .2
		check(t, a.Mount(Column("root").Children(line, capped, circle, legacy)))
		c, err := line.SkeletonConfig()
		check(t, err)
		if c != owned {
			t.Fatalf("config was aliased: got %+v, want %+v", c, owned)
		}
		_, ok, err := line.SkeletonPresentation()
		check(t, err)
		if ok {
			t.Fatal("sample before step")
		}
		check(t, a.Step(.25))
		for _, item := range []struct {
			node          *Node
			width, height float32
		}{{line, (width - 32) * .65, 16}, {capped, 128, 12}, {circle, 40, 40}, {legacy, 72, 12}} {
			b, err := a.Bounds(item.node)
			check(t, err)
			if math.Abs(float64(b.Width-item.width)) > .001 || b.Height != item.height {
				t.Fatalf("bounds for %q = %+v, want %g × %g", item.node.key, b, item.width, item.height)
			}
		}
		if line.VisualSize(1, 1).Err() == nil || legacy.ConfigureSkeleton(owned) == nil {
			t.Fatal("mixed sizing APIs accepted")
		}
		check(t, a.Close())
	}
}

func TestResponsiveSkeletonSampling(t *testing.T) {
	a := app(t)
	c := DefaultSkeletonConfig()
	line := ResponsiveSkeleton("line", c)
	panel := Collapsible("profile").Text("Profile").Open(true).Children(line)
	check(t, a.Mount(Column("root").Children(panel)))
	read := func(want bool) SkeletonPresentation {
		t.Helper()
		sample, ok, err := line.SkeletonPresentation()
		check(t, err)
		if ok != want {
			t.Fatalf("sample presence = %t, want %t", ok, want)
		}
		return sample
	}
	check(t, a.Step(.25))
	first := read(true)
	if first.FrameSequence != 1 || first.CoreTime != .25 || first.Phase != 0 ||
		math.Abs(first.NextWakeSeconds-first.CoreTime-1.0/60) > 1e-10 {
		t.Fatal(first)
	}
	bad := c
	bad.MaxWidth = 20
	if line.ConfigureSkeleton(bad) == nil || read(true) != first {
		t.Fatal("invalid replacement disturbed live config")
	}
	c.Secondary, c.Width = true, .5
	check(t, line.ConfigureSkeleton(c))
	read(false)
	check(t, a.Step(.5))
	next := read(true)
	if next.FrameSequence != 2 || next.Phase != .5 || next.Opacity != .75 {
		t.Fatal(next)
	}
	check(t, panel.Open(false).Err())
	check(t, a.Step(.25))
	read(false)
	check(t, panel.Open(true).Err())
	check(t, a.Step(.25))
	if resumed := read(true); resumed.Phase != 0 {
		t.Fatal(resumed)
	}
	c.ReducedMotion = true
	check(t, line.ConfigureSkeleton(c))
	check(t, a.Step(.25))
	if static := read(true); static.NextWakeSeconds != 0 || static.Opacity != 1 {
		t.Fatal(static)
	}
	check(t, line.Remove())
	if _, _, err := line.SkeletonPresentation(); err == nil {
		t.Fatal("removed node still readable")
	}
	check(t, a.Step(.25))
	if first.FrameSequence != 1 || first.Phase != 0 {
		t.Fatal("copied sample changed after removal")
	}
}

func TestResponsiveSkeletonMountRollback(t *testing.T) {
	a, err := New(Options{MaxNodes: 2})
	check(t, err)
	defer a.Close()
	config := DefaultSkeletonConfig()
	config.Height = float32(math.NaN())
	bad := ResponsiveSkeleton("invalid", config)
	if a.Mount(Column("invalid-root").Children(bad)) == nil || bad.app != nil {
		t.Fatal("bad configuration admitted")
	}
	valid := ResponsiveSkeleton("valid", DefaultSkeletonConfig())
	if a.Mount(Column("full-root").Children(valid, Label("Overflow"))) == nil ||
		a.root != nil || valid.app != nil || len(a.nodes) != 0 {
		t.Fatal("failed mount left live nodes")
	}
	check(t, a.Mount(Column("root").Children(valid)))
	check(t, a.Step(.25))
}

func TestResponsiveSkeletonOptionalProvider(t *testing.T) {
	if os.Getenv("CUI_TEST_UNSUPPORTED_SKELETON") != "1" {
		t.Skip("requires a pre-presentation or incompatible bridge")
	}
	a := app(t)
	supported, err := a.SupportsSkeletonPresentation()
	check(t, err)
	if supported {
		t.Fatal("incomplete optional group admitted")
	}
	modern := ResponsiveSkeleton("modern", DefaultSkeletonConfig())
	if err := a.Mount(Column("root").Children(modern)); !errors.Is(err, ErrSkeletonPresentationUnavailable) ||
		a.root != nil || modern.app != nil || len(a.nodes) != 0 {
		t.Fatalf("unsupported mount affected ownership: %v", err)
	}
	legacy := Skeleton("classic")
	check(t, a.Mount(Column("classic-root").Children(legacy)))
	if err := legacy.ConfigureSkeleton(DefaultSkeletonConfig()); !errors.Is(err, ErrSkeletonPresentationUnavailable) {
		t.Fatalf("unsupported replacement: %v", err)
	}
	check(t, legacy.VisualSize(72, 12).Err())
	check(t, a.Step(.25))
	b, err := a.Bounds(legacy)
	check(t, err)
	if b.Width != 72 || b.Height != 12 || uint32(legacy.handle.generation) != 1 {
		t.Fatal(b, legacy.handle)
	}
}
