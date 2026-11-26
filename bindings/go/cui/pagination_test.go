package cui

import (
	"errors"
	"math"
	"os"
	"strings"
	"testing"
)

func clickPaginationPart(t *testing.T, a *App, pager *Node, part uint32) {
	t.Helper()
	bounds, err := a.NumericPartBounds(pager, part)
	check(t, err)
	if bounds.Width <= 0 || bounds.Height <= 0 {
		t.Fatalf("missing Pagination part %d: %+v", part, bounds)
	}
	x, y := bounds.X+bounds.Width/2, bounds.Y+bounds.Height/2
	check(t, a.QueuePointer(PointerDown, x, y))
	check(t, a.QueuePointer(PointerUp, x, y))
	check(t, a.Step(.016))
}

func TestPaginationConfigAndKeyboardJump(t *testing.T) {
	a, err := New(Options{Width: 480, Height: 480})
	check(t, err)
	defer a.Close()
	supported, err := a.SupportsPaginationOptions()
	check(t, err)
	if !supported {
		t.Fatal("matching bridge must offer V1")
	}
	config := PaginationConfig{
		Chevrons: true, JumpMenu: true, AvailableWidth: 224, VisiblePages: 5,
		Group: "Seiten", First: "Erste", Previous: "Zurück", Next: "Weiter",
		Last: "Letzte", EarlierPages: "Früher", LaterPages: "Später",
	}
	pager := ConfiguredPagination("pages", config).Total(400).Value(11).DesignSize(Size2XL)
	changes := []Event{}
	pager.OnChange(func(e Event) { changes = append(changes, e) })
	original := config
	config.Next = "changed"
	read := func(want PaginationConfig, present bool) {
		t.Helper()
		got, ok, err := pager.PaginationConfig()
		check(t, err)
		if ok != present || got != want {
			t.Fatalf("configuration = %+v, %t, want %+v, %t", got, ok, want, present)
		}
	}
	read(original, true)
	check(t, a.Mount(Column("root").Children(pager)))
	read(original, true)
	check(t, a.Step(.016))
	previous, err := a.NumericPartBounds(pager, 2)
	check(t, err)
	next, err := a.NumericPartBounds(pager, 3)
	check(t, err)
	if next.Y <= previous.Y || previous.Height != 56 || next.Height != 56 {
		t.Fatalf("unexpected wrapped layout: previous %+v, next %+v", previous, next)
	}

	clickPaginationPart(t, a, pager, 6)
	count, err := a.Dispatch()
	check(t, err)
	if count != 0 {
		t.Fatal("opening the ellipsis changed the page")
	}
	check(t, a.QueueKey(KeyDown, Down, 0, false))
	check(t, a.Step(.016))
	check(t, a.QueueKey(KeyDown, Enter, 0, false))
	check(t, a.Step(.016))
	count, err = a.Dispatch()
	check(t, err)
	if count != 1 || len(changes) != 1 || changes[0].Node != pager || changes[0].Value != 14 {
		t.Fatalf("unexpected page change: %+v", changes)
	}
	count, err = a.Dispatch()
	check(t, err)
	if count != 0 || len(changes) != 1 {
		t.Fatal("callback replayed")
	}

	replacement := PaginationConfig{Counter: true, IconOnly: true, Next: strings.Repeat("é", 63) + "?"}
	check(t, pager.ConfigurePagination(replacement))
	read(replacement, true)
	if err := pager.ConfigurePagination(PaginationConfig{Group: "\n"}); err == nil {
		t.Fatal("accepted multi-line label")
	}
	read(replacement, true)
	check(t, pager.ResetPagination())
	read(PaginationConfig{}, false)
	check(t, a.Step(.016))
	after, err := a.NumericPartBounds(pager, 3)
	check(t, err)
	value, err := a.Value(pager)
	check(t, err)
	if after.Height != next.Height || value != 14 {
		t.Fatalf("reset affected size or value: %+v, %g", after, value)
	}
	check(t, pager.Remove())
	if _, _, err := pager.PaginationConfig(); err == nil || pager.ResetPagination() == nil {
		t.Fatal("removed pager still admitted")
	}
}

func TestPaginationValidationAndMountRollback(t *testing.T) {
	a, err := New(Options{MaxNodes: 2})
	check(t, err)
	defer a.Close()
	pager := Pagination("pages")
	original := PaginationConfig{Group: "Results"}
	check(t, pager.ConfigurePagination(original))
	for _, bad := range []PaginationConfig{
		{AvailableWidth: -1}, {AvailableWidth: 32769}, {AvailableWidth: float32(math.NaN())},
		{AvailableWidth: float32(math.Inf(1))}, {VisiblePages: 4}, {VisiblePages: 33},
		{Group: strings.Repeat("é", 64)}, {First: "\x00"}, {Previous: "\x7f"},
		{Next: "\u0085"}, {Last: "\u2028"}, {EarlierPages: "\u2029"}, {LaterPages: "\xff"},
	} {
		if err := pager.ConfigurePagination(bad); err == nil {
			t.Fatalf("accepted invalid config: %+v", bad)
		}
		got, ok, err := pager.PaginationConfig()
		check(t, err)
		if !ok || got != original {
			t.Fatal("invalid input replaced authored configuration")
		}
	}
	if err := Rating("rating").ConfigurePagination(original); err == nil {
		t.Fatal("accepted configuration for Rating")
	}
	if err := a.Mount(Column("full-root").Children(pager, Label("Overflow"))); err == nil ||
		a.root != nil || pager.app != nil || len(a.nodes) != 0 {
		t.Fatal("failed mount left live nodes")
	}
	check(t, a.Mount(Column("root").Children(pager)))
	got, ok, err := pager.PaginationConfig()
	check(t, err)
	if !ok || got != original {
		t.Fatal("retry did not apply original configuration")
	}
}

func TestPaginationOptionalProvider(t *testing.T) {
	a := app(t)
	unsupported := os.Getenv("CUI_TEST_UNSUPPORTED_PAGINATION") == "1"
	supported, err := a.SupportsPaginationOptions()
	check(t, err)
	if supported == unsupported {
		t.Fatalf("provider capability = %t, expected %t", supported, !unsupported)
	}
	if unsupported {
		for _, pager := range []*Node{
			ConfiguredPagination("modern", PaginationConfig{Counter: true}),
			Pagination("size").DesignSize(LG),
		} {
			err := a.Mount(Column("root").Children(pager))
			if !errors.Is(err, ErrPaginationOptionsUnavailable) || a.root != nil ||
				pager.app != nil || len(a.nodes) != 0 {
				t.Fatalf("unsupported mount affected ownership: %v", err)
			}
		}
	}
	ordinary := ConfiguredPagination("ordinary", PaginationConfig{Counter: true}).Total(20).Value(10)
	check(t, ordinary.ResetPagination())
	var changes []Event
	ordinary.OnChange(func(e Event) { changes = append(changes, e) })
	check(t, a.Mount(Column("ordinary-root").Children(ordinary)))
	_, ok, err := ordinary.PaginationConfig()
	check(t, err)
	if ok {
		t.Fatal("ordinary pager has an override")
	}
	if unsupported {
		err := ordinary.ConfigurePagination(PaginationConfig{Counter: true})
		if !errors.Is(err, ErrPaginationOptionsUnavailable) {
			t.Fatal(err)
		}
		if !errors.Is(ordinary.ResetPagination(), ErrPaginationOptionsUnavailable) {
			t.Fatal("mounted reset must negotiate the V1 provider")
		}
		if !errors.Is(ordinary.DesignSize(SM).Err(), ErrPaginationOptionsUnavailable) {
			t.Fatal("mounted size must negotiate the V1 provider")
		}
	}
	check(t, a.Step(.016))
	clickPaginationPart(t, a, ordinary, 3)
	count, err := a.Dispatch()
	check(t, err)
	value, err := a.Value(ordinary)
	check(t, err)
	if count != 1 || len(changes) != 1 || changes[0].Value != 11 || value != 11 {
		t.Fatalf("ordinary callback or page changed: %+v, %g", changes, value)
	}
}

func TestPaginationCustomSize(t *testing.T) {
	if os.Getenv("CUI_PAGINATION_CUSTOM_UNSUPPORTED") == "1" {
		t.Skip("fixture lacks the custom height exports")
	}
	a := app(t)
	pager := Pagination("pages").Total(20).DesignSize(LG)
	check(t, pager.SetPaginationCustomSize(52.5))
	got, err := pager.PaginationCustomSize()
	check(t, err)
	if got != 52.5 {
		t.Fatal(got)
	}
	if Rating("rating").SetPaginationCustomSize(52.5) == nil {
		t.Fatal("accepted wrong kind")
	}
	changes := 0
	pager.OnChange(func(Event) { changes++ })
	check(t, a.Mount(Column("root").Children(pager)))
	for _, bad := range []float32{-1, .5, 513, float32(math.NaN()), float32(math.Inf(1))} {
		if pager.SetPaginationCustomSize(bad) == nil {
			t.Fatal("accepted invalid height")
		}
		got, err := pager.PaginationCustomSize()
		check(t, err)
		if got != 52.5 {
			t.Fatal("invalid update changed height")
		}
	}
	check(t, a.Step(.016))
	b, err := a.NumericPartBounds(pager, 3)
	check(t, err)
	if b.Height != 52.5 {
		t.Fatalf("height = %g", b.Height)
	}
	clickPaginationPart(t, a, pager, 3)
	count, err := a.Dispatch()
	check(t, err)
	if count != 1 || changes != 1 {
		t.Fatal("activation count")
	}
	count, err = a.Dispatch()
	check(t, err)
	if count != 0 || changes != 1 {
		t.Fatal("activation replayed")
	}
	check(t, pager.SetPaginationCustomSize(0))
	check(t, a.Step(.016))
	b, err = a.NumericPartBounds(pager, 3)
	check(t, err)
	if b.Height != 40 {
		t.Fatalf("named height = %g", b.Height)
	}
	check(t, pager.Remove())
	if pager.SetPaginationCustomSize(52.5) == nil {
		t.Fatal("accepted stale node")
	}
}

func TestPaginationCustomSizeOptionalProvider(t *testing.T) {
	a := app(t)
	unsupported := os.Getenv("CUI_PAGINATION_CUSTOM_UNSUPPORTED") == "1"
	supported, err := a.SupportsPaginationCustomSize()
	check(t, err)
	if supported == unsupported {
		t.Fatalf("custom capability = %t", supported)
	}
	pager := Pagination("pages").Total(20)
	check(t, pager.SetPaginationCustomSize(52.5))
	if unsupported {
		if err := a.Mount(Column("root").Children(pager)); !errors.Is(err, ErrPaginationCustomSizeUnavailable) || a.root != nil || pager.app != nil {
			t.Fatalf("preflight: %v", err)
		}
		check(t, pager.SetPaginationCustomSize(0))
	}
	check(t, a.Mount(Column("root").Children(pager)))
	if unsupported {
		if !errors.Is(pager.SetPaginationCustomSize(0), ErrPaginationCustomSizeUnavailable) {
			t.Fatal("mounted reset must require capability")
		}
		if _, err := pager.PaginationCustomSize(); !errors.Is(err, ErrPaginationCustomSizeUnavailable) {
			t.Fatal("mounted read must require capability")
		}
	} else {
		got, err := pager.PaginationCustomSize()
		check(t, err)
		if got != 52.5 {
			t.Fatal(got)
		}
	}
	check(t, a.Step(.016))
	clickPaginationPart(t, a, pager, 3)
	count, err := a.Dispatch()
	check(t, err)
	if count != 1 {
		t.Fatal(count)
	}
}
