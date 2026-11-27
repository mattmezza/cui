package cui

import (
	"os"
	"testing"
)

func TestSpinnerIconResources(t *testing.T) {
	a := app(t)
	n := RotatingSpinner("spinner", DefaultSpinnerConfig())
	check(t, a.Mount(Column("root").Children(n)))
	supported, err := a.SupportsSpinnerIconResources()
	check(t, err)
	if os.Getenv("CUI_TEST_UNSUPPORTED_SPINNER_ARTWORK") == "1" {
		if supported || n.SetSpinnerIconResource(nil) == nil {
			t.Fatal("incomplete artwork accepted")
		}
		if _, err := n.SpinnerIconResource(); err == nil {
			t.Fatal("incomplete getter accepted")
		}
		check(t, a.Step(.1))
		if _, ok, err := n.SpinnerPresentation(); err != nil || !ok {
			t.Fatal("legacy presentation lost", err)
		}
		return
	}
	if !supported {
		t.Fatal("missing artwork capability")
	}
	resource, err := a.NewIconResource(IconSVG(`<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24"><path d="M2 2h20v20H2z"/></svg>`), IconLimits{})
	check(t, err)
	check(t, n.SetSpinnerIconResource(resource))
	got, err := n.SpinnerIconResource()
	check(t, err)
	if got == nil || got.owner != resource.owner {
		t.Fatal("resource ownership lost")
	}
	check(t, a.Step(.1))
	if resource.Close() == nil {
		t.Fatal("attached resource released")
	}
	other := app(t)
	foreign, err := other.NewIconResource(IconSVG(`<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24"><path d="M2 2h20v20H2z"/></svg>`), IconLimits{})
	check(t, err)
	if n.SetSpinnerIconResource(foreign) == nil {
		t.Fatal("foreign artwork accepted")
	}
	check(t, n.SetSpinnerIconResource(nil))
	got, err = n.SpinnerIconResource()
	check(t, err)
	if got != nil {
		t.Fatal("clear failed")
	}
	check(t, resource.Close())
	if n.SetSpinnerIconResource(resource) == nil {
		t.Fatal("released artwork accepted")
	}
	check(t, n.Remove())
	if _, err := n.SpinnerIconResource(); err == nil {
		t.Fatal("removed node accepted")
	}
}
