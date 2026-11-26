package main

import (
	"flag"
	"fmt"

	"github.com/mattmezza/cui/bindings/go/cui"
)

func main() {
	font := flag.String("font", "", "TrueType font for a native SDL window")
	flag.Parse()
	app, err := cui.New(cui.Options{
		Title: "Delivery feedback", Width: 480, Height: 280,
		Native: *font != "", FontPath: *font,
	})
	must(err)
	defer app.Close()
	supported, err := app.SupportsRatingOptions()
	must(err)
	initial := 2.0
	rating := cui.Rating("rating")
	if supported {
		initial = 2.5
		heart, err := app.NewIconResource(cui.IconSource{Kind: cui.IconSourceName, Text: "heart"}, cui.IconLimits{})
		must(err)
		rating = cui.ConfiguredRating("rating", cui.RatingConfig{
			HalfSteps: true, HoverPreview: true, Clearable: true, DesignSize: cui.LG,
			HasFill: true, FillRGBA: 0xd53975ff, OutlineIcon: heart, FillIcon: heart,
		})
	}
	summary := cui.Label(fmt.Sprintf("Your rating is %.1f of 5", initial))
	rating.Text("Rate this delivery").Value(initial).OnChange(func(event cui.Event) {
		must(summary.SetText(fmt.Sprintf("Your rating is %.1f of 5", event.Value)))
	})
	must(app.Mount(cui.Column("feedback").GapUnits(2).Children(
		cui.Label("How was your delivery?"), rating, summary,
	)))
	if *font != "" {
		must(app.Run(.016))
		return
	}
	must(app.Step(.016))
	mark, err := app.NumericPartBounds(rating, 13)
	must(err)
	x, y := mark.X+mark.Width/4, mark.Y+mark.Height/2
	must(app.QueuePointer(cui.PointerDown, x, y))
	must(app.QueuePointer(cui.PointerUp, x, y))
	must(app.Step(.016))
	changes, err := app.Dispatch()
	must(err)
	text, err := app.Text(summary)
	must(err)
	want := "Your rating is 4.0 of 5"
	if supported {
		want = "Your rating is 3.5 of 5"
	}
	if changes != 1 || text != want {
		panic(fmt.Sprintf("unexpected feedback UI: %d changes, %q", changes, text))
	}
	fmt.Println(text)
}

func must(err error) {
	if err != nil {
		panic(err)
	}
}
