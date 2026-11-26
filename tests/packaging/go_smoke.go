package main

import (
	"fmt"
	"github.com/mattmezza/cui/bindings/go/cui"
	"os"
)

func main() {
	native := os.Getenv("CUI_PACKAGE_SMOKE_BACKEND") == "sdl"
	app, err := cui.New(cui.Options{Native: native, FontPath: os.Getenv("CUI_PACKAGE_SMOKE_FONT"), Title: "CUI Installed Go Smoke"})
	if err != nil {
		panic(err)
	}
	defer app.Close()
	if supported, err := app.SupportsRadioTabIndex(); err != nil || !supported {
		panic("installed bridge lacks Radio ordering")
	}
	if supported, err := app.SupportsRadioPaint(); err != nil || !supported {
		panic(fmt.Sprintf("installed bridge lacks Radio paint: %v", err))
	}
	if supported, err := app.SupportsRatingOptions(); err != nil || !supported {
		panic(fmt.Sprintf("installed bridge lacks Rating options: %v", err))
	}
	if supported, err := app.SupportsStepperFill(); err != nil || !supported {
		panic(fmt.Sprintf("installed bridge lacks Stepper Fill: %v", err))
	}
	if supported, err := app.SupportsVirtualListRows(); err != nil || !supported {
		panic(fmt.Sprintf("installed bridge lacks VirtualList live rows: %v", err))
	}
	if supported, err := app.SupportsDisclosurePresentation(); err != nil || !supported {
		panic(fmt.Sprintf("installed bridge lacks disclosure presentation: %v", err))
	}
	if supported, err := app.SupportsDisclosureHeading(); err != nil || !supported {
		panic(fmt.Sprintf("installed bridge lacks disclosure headings: %v", err))
	}
	if supported, err := app.SupportsDisclosureDivider(); err != nil || !supported {
		panic(fmt.Sprintf("installed bridge lacks disclosure dividers: %v", err))
	}
	if supported, err := app.SupportsCarouselBasis(); err != nil || !supported {
		panic(fmt.Sprintf("installed bridge lacks fractional Carousel slides: %v", err))
	}
	clicks := 0
	button := cui.Button("save").Text("Save ✓").OnClick(func(cui.Event) { clicks++ })
	overview := cui.Item("overview", "Overview").CarouselBasis(0.5)
	activity := cui.Item("activity", "Activity").CarouselBasis(0.5)
	slides := cui.Carousel("slides").VisualSize(240, 100).Children(overview, activity)
	if err := app.Mount(cui.Column("root").Children(button, slides)); err != nil {
		panic(err)
	}
	if err := app.Step(.016); err != nil {
		panic(err)
	}
	a, firstErr := app.Bounds(overview)
	b, secondErr := app.Bounds(activity)
	if firstErr != nil || secondErr != nil || b.X-a.X != 128 {
		panic("installed Carousel did not lay out fractional slides")
	}
	bounds, err := app.Bounds(button)
	if err != nil || bounds.Width <= 0 || bounds.Height <= 0 {
		panic("missing Button geometry")
	}
	if err := app.QueuePointer(cui.PointerDown, bounds.X+bounds.Width/2, bounds.Y+bounds.Height/2); err != nil {
		panic(err)
	}
	if err := app.QueuePointer(cui.PointerUp, bounds.X+bounds.Width/2, bounds.Y+bounds.Height/2); err != nil {
		panic(err)
	}
	if err := app.Step(.016); err != nil {
		panic(err)
	}
	delivered, err := app.Dispatch()
	if err != nil || delivered != 1 || clicks != 1 {
		panic("installed callback failed")
	}
	fmt.Println("installed Go bridge smoke passed")
}
