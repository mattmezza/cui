package main

import (
	"flag"
	"fmt"
	"math"
	"os"

	"github.com/mattmezza/cui/bindings/go/cui"
)

func main() {
	font := flag.String("font", "", "font path for a native SDL window")
	smoke := flag.Bool("smoke", false, "step a hidden window on the private test display")
	flag.Parse()
	if *smoke && (*font == "" || os.Getenv("CUI_TEST_VIRTUAL_DISPLAY") != "1") {
		panic("--smoke requires a font and the private display from scripts/test_local.py")
	}
	app, err := cui.New(cui.Options{Width: 480, Height: 320, Title: "Loading a workspace profile",
		Native: *font != "", FontPath: *font, InitiallyHidden: *smoke})
	must(err)
	defer app.Close()

	spinner := cui.RotatingSpinner("loading", cui.DefaultSpinnerConfig())

	title, status := cui.Label("Workspace profile"), cui.Label("Loading profile…")
	finish := cui.Button("finish").Variant(cui.Primary).Text("Finish loading")
	finish.OnClick(func(cui.Event) {
		must(spinner.Remove())
		must(title.SetText("Alex Morgan"))
		must(status.SetText("Workspace admin · alex@example.com"))
		must(finish.Disabled(true).Err())
		must(finish.SetText("Profile loaded"))
	})
	must(app.Mount(cui.Column("root").GapUnits(2).Children(title, spinner, status, finish)))
	// Older providers keep the catalog Loader; capable providers share custom SVG.
	artworkSupported, err := app.SupportsSpinnerIconResources()
	must(err)
	if artworkSupported {
		artwork, err := app.NewIconResource(cui.IconSVG(`<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24"><path d="M12 2a10 10 0 0 1 10 10h-3a7 7 0 0 0-7-7z"/></svg>`), cui.IconLimits{})
		must(err)
		must(spinner.SetSpinnerIconResource(artwork))
	}
	if *font != "" && !*smoke {
		must(app.Run(.016))
		return
	}

	must(app.Step(.25))
	b, err := app.Bounds(spinner)
	must(err)
	sample, ok, err := spinner.SpinnerPresentation()
	must(err)
	if math.Abs(float64(b.Width)-16) > .001 || !ok || sample.NextWakeSeconds <= sample.CoreTime {
		panic("Spinner geometry or motion missing")
	}
	before, err := app.Bounds(status)
	must(err)
	trigger, err := app.Bounds(finish)
	must(err)
	x, y := trigger.X+trigger.Width/2, trigger.Y+trigger.Height/2
	must(app.QueuePointer(cui.PointerDown, x, y))
	must(app.QueuePointer(cui.PointerUp, x, y))
	must(app.Step(.016))
	_, err = app.Dispatch()
	must(err)
	must(app.Step(.016))
	after, err := app.Bounds(status)
	must(err)
	text, err := app.Text(title)
	must(err)
	if text != "Alex Morgan" || after.Y >= before.Y {
		panic("loading status did not transition")
	}
	fmt.Println("Spinner loading profile: loaded and spinner removed")
}

func must(err error) {
	if err != nil {
		panic(err)
	}
}
