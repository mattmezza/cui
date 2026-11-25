// SPDX-License-Identifier: MIT
package main

import (
	"flag"
	"fmt"
	"os"

	"github.com/mattmezza/cui/bindings/go/cui"
)

const checkMark = `<svg width="20" height="20">
  <path d="M3 10l4 4 10-10" fill="none" stroke="white" stroke-width="2"/>
</svg>`

func main() {
	font := flag.String("font", "", "font path for a native SDL window")
	smoke := flag.Bool("smoke", false, "step a hidden window on the private test display")
	flag.Parse()
	if *smoke && (*font == "" || os.Getenv("CUI_TEST_VIRTUAL_DISPLAY") != "1") {
		panic("--smoke requires a font and the private display from scripts/test_local.py")
	}
	app, err := cui.New(cui.Options{
		Width: 480, Height: 260, Title: "CUI inbox", Native: *font != "",
		FontPath: *font, InitiallyHidden: *smoke,
	})
	must(err)
	defer app.Close()

	artwork, err := app.NewIconResource(cui.IconSVG(checkMark), cui.IconLimits{})
	must(err)
	icon := cui.ConfiguredIcon("status-icon", cui.IconConfig{
		Resource: artwork, Name: "Synced",
		Options: cui.IconOptions{Width: 24, Height: 24, HasWidth: true, HasHeight: true, ColorRGBA: 0x34a170ff, HasColor: true},
	})
	unread := cui.ConfiguredBadge("unread", cui.BadgeConfig{Count: 8, Maximum: 99, Name: "Unread messages"})
	synced := cui.ConfiguredBadge("sync", cui.BadgeConfig{
		Variant: cui.BadgeIcon, Artwork: artwork, Name: "Synced",
		ArtworkOptions: cui.IconOptions{Width: 12, Height: 12, HasWidth: true, HasHeight: true},
	})
	status := cui.Label("8 unread messages")
	mark := cui.Button("mark").Variant(cui.Primary).Text("Mark all as read").OnClick(func(cui.Event) {
		config, err := unread.BadgeConfig()
		must(err)
		config.Count = 0
		must(unread.ConfigureBadge(config))
		must(status.SetText("Inbox is up to date"))
	})
	outbox := cui.Button("outbox").Variant(cui.Secondary).Text("Outbox")
	must(app.Mount(cui.Column("inbox").GapUnits(4).Children(
		cui.Row("header").GapUnits(3).Children(icon, cui.Label("Workspace inbox")),
		status, cui.Row("actions").GapUnits(5).Children(unread.Children(mark), synced.Children(outbox)),
	)))
	if *font != "" && !*smoke {
		must(app.Run(.016))
		return
	}

	must(app.Step(.016))
	info, err := artwork.Info()
	must(err)
	paint, observed, err := synced.BadgeResult()
	must(err)
	_, visible, err := unread.BadgeIndicatorBounds()
	must(err)
	if info.References != 2 || !observed || !paint.HasIconError || paint.IconError != cui.IconOK || !visible {
		panic("shared SVG or initial unread indicator missing")
	}
	before, err := app.Bounds(mark)
	must(err)
	x, y := before.X+before.Width/2, before.Y+before.Height/2
	must(app.QueuePointer(cui.PointerDown, x, y))
	must(app.QueuePointer(cui.PointerUp, x, y))
	must(app.Step(.016))
	dispatched, err := app.Dispatch()
	must(err)
	must(app.Step(.016))
	after, err := app.Bounds(mark)
	must(err)
	_, visible, err = unread.BadgeIndicatorBounds()
	must(err)
	text, err := app.Text(status)
	must(err)
	if dispatched != 1 || before != after || visible || text != "Inbox is up to date" {
		panic("unread count did not clear without moving the button")
	}
	fmt.Println("Inbox verified shared SVG, child activation and stable layout")
}

func must(err error) {
	if err != nil {
		panic(err)
	}
}
