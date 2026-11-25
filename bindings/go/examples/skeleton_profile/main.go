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

	avatar := cui.ResponsiveSkeleton("avatar", cui.SkeletonConfig{Width: 40, Height: 40, Radius: 20})
	nameStyle := cui.DefaultSkeletonConfig()
	nameStyle.Width, nameStyle.Radius = .65, 6
	name := cui.ResponsiveSkeleton("name", nameStyle)
	roleStyle := nameStyle
	roleStyle.Width, roleStyle.Height, roleStyle.Secondary = .4, 12, true
	role := cui.ResponsiveSkeleton("role", roleStyle)
	descriptionStyle := cui.DefaultSkeletonConfig()
	descriptionStyle.Height, descriptionStyle.Radius = 36, 8
	description := cui.ResponsiveSkeleton("description", descriptionStyle)

	title, status := cui.Label("Workspace profile"), cui.Label("Loading profile…")
	finish := cui.Button("finish").Variant(cui.Primary).Text("Finish loading")
	finish.OnClick(func(cui.Event) {
		for _, n := range []*cui.Node{avatar, name, role, description} {
			must(n.Remove())
		}
		must(title.SetText("Alex Morgan"))
		must(status.SetText("Workspace admin · alex@example.com"))
		must(finish.Disabled(true).Err())
		must(finish.SetText("Profile loaded"))
	})
	must(app.Mount(cui.Column("root").GapUnits(2).Children(title, avatar, name, role, description, status, finish)))
	if *font != "" && !*smoke {
		must(app.Run(.016))
		return
	}

	must(app.Step(.25))
	b, err := app.Bounds(name)
	must(err)
	sample, ok, err := name.SkeletonPresentation()
	must(err)
	if math.Abs(float64(b.Width)-448*.65) > .001 || !ok || sample.NextWakeSeconds <= sample.CoreTime {
		panic("responsive profile geometry or motion missing")
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
		panic("profile content did not take the placeholders' place")
	}
	fmt.Println("Responsive Skeleton profile: loaded and placeholders released")
}

func must(err error) {
	if err != nil {
		panic(err)
	}
}
