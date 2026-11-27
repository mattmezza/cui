package main

import (
	"fmt"
	"os"

	"github.com/mattmezza/cui/bindings/go/cui"
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
	if supported, err := app.SupportsNotificationAction(); err != nil || !supported {
		panic(fmt.Sprintf("installed bridge lacks Notification actions: %v", err))
	}
	if supported, err := app.SupportsNotificationAutoHide(); err != nil || !supported {
		panic(fmt.Sprintf("installed bridge lacks Notification auto-hide: %v", err))
	}
	if supported, err := app.SupportsStepperCenteredFill(); err != nil || !supported {
		panic(fmt.Sprintf("installed bridge lacks Stepper Centered Fill: %v", err))
	}
	if supported, err := app.SupportsDialogComposition(); err != nil || !supported {
		panic(fmt.Sprintf("installed bridge lacks Dialog composition: %v", err))
	}
	if supported, err := app.SupportsPopoverSecondaryTrigger(); err != nil || !supported {
		panic(fmt.Sprintf("installed bridge lacks Popover right-click: %v", err))
	}
	if supported, err := app.SupportsStepperWrappedFill(); err != nil || !supported {
		panic(fmt.Sprintf("installed bridge lacks Stepper Wrapped Fill: %v", err))
	}
	if supported, err := app.SupportsVirtualListAxis(); err != nil || !supported {
		panic(fmt.Sprintf("installed bridge lacks horizontal VirtualList: %v", err))
	}
	if supported, err := app.SupportsKbdStyle(); err != nil || !supported {
		panic(fmt.Sprintf("installed bridge lacks Kbd styling: %v", err))
	}
	if supported, err := app.SupportsResponsiveNotifications(); err != nil || !supported {
		panic(fmt.Sprintf("installed bridge lacks responsive notifications: %v", err))
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
	if supported, err := app.SupportsCarouselNavigation(); err != nil || !supported {
		panic(fmt.Sprintf("installed bridge lacks Carousel navigation: %v", err))
	}
	if supported, err := app.SupportsKbdStroke(); err != nil || !supported {
		panic(fmt.Sprintf("installed bridge lacks typed Kbd: %v", err))
	}
	if supported, err := app.SupportsSpinnerPresentation(); err != nil || !supported {
		panic(fmt.Sprintf("installed bridge lacks rotating Spinner: %v", err))
	}
	if supported, err := app.SupportsSpinnerIconResources(); err != nil || !supported {
		panic(fmt.Sprintf("installed bridge lacks shared Spinner artwork: %v", err))
	}
	spinner := cui.RotatingSpinner("spinner", cui.DefaultSpinnerConfig())
	sharedSpinner := cui.RotatingSpinner("shared-spinner", cui.DefaultSpinnerConfig())
	clicks := 0
	button := cui.Button("save").Text("Save ✓").OnClick(func(cui.Event) { clicks++ })
	overview := cui.Item("overview", "Overview").CarouselBasis(0.5)
	activity := cui.Item("activity", "Activity").CarouselBasis(0.5)
	settings := cui.Item("settings", "Settings").CarouselBasis(0.5)
	var changes []cui.Event
	slides := cui.Carousel("slides").VisualSize(240, 100).Children(overview, activity, settings).
		OnChange(func(event cui.Event) { changes = append(changes, event) })
	shortcutStyle := cui.KbdStrokeOptions{Modifiers: cui.KbdCtrl, Outline: true, AccessibleName: "Save shortcut"}
	hint := cui.KbdStroke("shortcut", "ß", shortcutStyle)
	reference := cui.KbdStroke("reference", "sS", shortcutStyle)
	if err := app.Mount(cui.Column("root").Children(button, slides, hint, reference, spinner, sharedSpinner)); err != nil {
		panic(err)
	}
	artwork, err := app.NewIconResource(cui.IconSVG(`<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24"><path d="M2 2h20v8H2z"/></svg>`), cui.IconLimits{})
	if err != nil {
		panic(err)
	}
	for _, node := range []*cui.Node{spinner, sharedSpinner} {
		if err := node.SetSpinnerIconResource(artwork); err != nil {
			panic(err)
		}
		if got, err := node.SpinnerIconResource(); err != nil || got == nil {
			panic("installed Spinner lost shared artwork")
		}
	}
	if err := app.Step(.016); err != nil {
		panic(err)
	}
	hintBounds, hintErr := app.Bounds(hint)
	referenceBounds, referenceErr := app.Bounds(reference)
	if hintErr != nil || referenceErr != nil || hintBounds.Width <= 0 || hintBounds.Height <= 0 ||
		hintBounds.Width != referenceBounds.Width || hintBounds.Height != referenceBounds.Height {
		panic("installed typed Kbd did not lay out Ctrl+SS")
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
	if next, err := app.CarouselAdjacent(slides, true); err != nil || next != activity || len(changes) != 0 {
		panic(fmt.Sprintf("installed Carousel did not find the next slide: %v", err))
	}
	if next, err := app.CarouselStep(slides, true); err != nil || next != activity || len(changes) != 0 {
		panic(fmt.Sprintf("installed Carousel did not queue the next slide: %v", err))
	}
	delivered, err = app.Dispatch()
	if err != nil || delivered != 1 || len(changes) != 1 || changes[0].Selected != activity {
		panic("installed Carousel change did not carry the next slide")
	}
	first, ok, err := spinner.SpinnerPresentation()
	if err != nil || !ok {
		panic("installed Spinner has no animation sample")
	}
	if err := app.Step(.2); err != nil {
		panic(err)
	}
	next, ok, err := spinner.SpinnerPresentation()
	spinnerBounds, boundsErr := app.Bounds(spinner)
	if err != nil || !ok || boundsErr != nil || spinnerBounds.Width <= 0 || spinnerBounds.Height <= 0 ||
		next.CoreTime <= first.CoreTime || next.Phase == first.Phase {
		panic("installed Spinner did not lay out and rotate")
	}
	if artwork.Close() == nil {
		panic("installed Spinner released attached artwork")
	}
	for _, node := range []*cui.Node{spinner, sharedSpinner} {
		if err := node.SetSpinnerIconResource(nil); err != nil {
			panic(err)
		}
		if got, err := node.SpinnerIconResource(); err != nil || got != nil {
			panic("installed Spinner did not clear artwork")
		}
	}
	if err := artwork.Close(); err != nil {
		panic(err)
	}
	fmt.Println("installed Go bridge smoke passed")
}
