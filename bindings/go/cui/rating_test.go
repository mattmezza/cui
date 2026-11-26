package cui

import (
	"errors"
	"math"
	"os"
	"testing"
)

func TestRatingConfiguration(t *testing.T) {
	if os.Getenv("CUI_TEST_UNSUPPORTED_RATING") == "1" {
		t.Skip("requires Rating V1")
	}
	app, err := New(Options{Width: 540, Height: 300})
	check(t, err)
	defer app.Close()
	supported, err := app.SupportsRatingOptions()
	check(t, err)
	if !supported {
		t.Fatal("matching provider lacks Rating options")
	}
	mask, err := app.NewIconResource(IconSource{Kind: IconSourceName, Text: "heart"}, IconLimits{})
	check(t, err)
	config := RatingConfig{
		HalfSteps: true, HoverPreview: true, StepDownFilled: true, Clearable: true,
		DesignSize: LG, Width: 270, HasFill: true, HasOutline: true,
		FillRGBA: 0xcf4570ff, OutlineIcon: mask, FillIcon: mask,
	}
	var changes []Event
	rating := ConfiguredRating("rating", config).Text("Delivery").Value(1.5).OnChange(func(e Event) {
		changes = append(changes, e)
	})
	check(t, app.Mount(Column("root").Children(rating)))
	check(t, app.Step(.016))
	read, present, err := rating.RatingConfig()
	check(t, err)
	if !present || !read.HoverPreview || !read.HasOutline || read.OutlineRGBA != 0 ||
		read.FillRGBA != config.FillRGBA || read.Width != 270 || read.OutlineIcon.owner != mask.owner {
		t.Fatalf("unexpected Rating snapshot: %+v", read)
	}
	info, err := mask.Info()
	check(t, err)
	if info.References != 2 || mask.Close() == nil {
		t.Fatalf("Rating must retain both references: %+v", info)
	}
	bad := config
	bad.Width = float32(math.NaN())
	if rating.ConfigureRating(bad) == nil {
		t.Fatal("accepted a NaN width")
	}
	other, err := New(Options{Width: 300, Height: 300})
	check(t, err)
	foreign, err := other.NewIconResource(IconSource{}, IconLimits{})
	check(t, err)
	bad = config
	bad.FillIcon = foreign
	if rating.ConfigureRating(bad) == nil {
		t.Fatal("accepted a foreign Icon")
	}
	check(t, foreign.Close())
	check(t, other.Close())

	part, err := app.NumericPartBounds(rating, 13)
	check(t, err)
	x, y := part.X+part.Width/4, part.Y+part.Height/2
	check(t, app.QueuePointer(PointerMove, x, y))
	check(t, app.Step(.016))
	count, err := app.Dispatch()
	check(t, err)
	if count != 0 || len(changes) != 0 {
		t.Fatal("hover committed a change")
	}
	for _, phase := range []PointerPhase{PointerDown, PointerUp} {
		check(t, app.QueuePointer(phase, x, y))
	}
	check(t, app.Step(.016))
	count, err = app.Dispatch()
	check(t, err)
	if count != 1 || len(changes) != 1 || changes[0].Node != rating || changes[0].Value != 3.5 {
		t.Fatalf("unexpected fractional change: %+v", changes)
	}
	count, err = app.Dispatch()
	check(t, err)
	if count != 0 {
		t.Fatal("change replayed")
	}
	check(t, rating.ResetRating())
	if _, present, err := rating.RatingConfig(); err != nil || present {
		t.Fatalf("reset configuration is present: %t, %v", present, err)
	}
	check(t, mask.Close())
	check(t, app.Step(.016))
	value, err := app.Value(rating)
	check(t, err)
	if value != 4 {
		t.Fatalf("legacy Rating value = %g", value)
	}
	check(t, rating.Remove())
	if _, _, err := rating.RatingConfig(); err == nil {
		t.Fatal("removed Rating accepted")
	}
}

func TestRatingOptionalProvider(t *testing.T) {
	app, err := New(Options{Width: 400, Height: 250})
	check(t, err)
	defer app.Close()
	supported, err := app.SupportsRatingOptions()
	check(t, err)
	expected := os.Getenv("CUI_TEST_UNSUPPORTED_RATING") != "1"
	if supported != expected {
		t.Fatalf("Rating V1 support = %t, want %t", supported, expected)
	}
	if supported {
		return
	}
	pending := ConfiguredRating("rating", RatingConfig{HalfSteps: true})
	if err := app.Mount(Column("root").Children(pending)); !errors.Is(err, ErrRatingOptionsUnavailable) {
		t.Fatalf("unsupported presentation returned %v", err)
	}
	rating := Rating("rating").Value(2.5)
	check(t, app.Mount(Column("root").Children(rating)))
	check(t, app.Step(.016))
	value, err := app.Value(rating)
	check(t, err)
	if value != 3 {
		t.Fatalf("legacy Rating value = %g", value)
	}
	if _, present, err := rating.RatingConfig(); err != nil || present {
		t.Fatalf("legacy configuration = %t, %v", present, err)
	}
}
