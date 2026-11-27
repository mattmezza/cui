// Open an account dialog with a custom Body and Footer. Pass -font FONT.ttf for SDL.
package main

import (
	"flag"
	"fmt"

	"github.com/mattmezza/cui/bindings/go/cui"
)

func main() {
	font := flag.String("font", "", "TrueType font for the native window")
	flag.Parse()
	app, err := cui.New(cui.Options{Title: "Account settings", Width: 560, Height: 360,
		Native: *font != "", FontPath: *font})
	must(err)
	defer app.Close()

	const removed = "Local account preferences removed."
	status := cui.Label("Local account preferences are active.")
	cancel := cui.Button("cancel").Text("Keep account").Variant(cui.Outline).
		DialogAction(cui.DialogActionCancel)
	confirm := cui.Button("confirm").Text("Remove account").Variant(cui.Primary).
		DialogAction(cui.DialogActionConfirm)
	alert := cui.AlertDialog("remove", "Remove this account?", "Only this device is affected.").
		DialogOptions(&cui.DialogOptions{Centered: true}).Children(
		cui.DialogSlot("details", cui.DialogBody).Children(cui.Label("Remove saved preferences and account information.")),
		cui.DialogSlot("actions", cui.DialogFooter).Children(cancel, confirm),
	).OnClose(func(event cui.Event) {
		if event.CloseResult == cui.SurfaceAccepted {
			must(status.SetText(removed))
		}
	})
	trigger := cui.Button("open").Text("Manage account").OnClick(func(cui.Event) {
		must(alert.Open(true).Err())
	})
	must(app.Mount(cui.Column("account").GapUnits(3).Children(
		cui.Label("Account settings"), status, trigger, alert,
	)))
	if *font != "" {
		must(app.Run(.016))
		return
	}
	must(app.Step(.016))
	click(app, trigger)
	must(app.Step(.016))
	click(app, confirm)
	text, err := app.Text(status)
	must(err)
	if text != removed {
		panic(fmt.Sprintf("unexpected account status %q", text))
	}
	fmt.Println(text)
}

func click(app *cui.App, button *cui.Node) {
	box, err := app.Bounds(button)
	must(err)
	x, y := box.X+box.Width/2, box.Y+box.Height/2
	must(app.QueuePointer(cui.PointerDown, x, y))
	must(app.QueuePointer(cui.PointerUp, x, y))
	must(app.Step(.016))
	_, err = app.Dispatch()
	must(err)
}

func must(err error) {
	if err != nil {
		panic(err)
	}
}
