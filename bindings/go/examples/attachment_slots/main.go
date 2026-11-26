// A file card and its Inspect button. Pass -font FONT.ttf to open an SDL window.
package main

import (
	"flag"
	"fmt"

	"github.com/mattmezza/cui/bindings/go/cui"
)

func main() {
	font := flag.String("font", "", "TrueType font for the native window")
	flag.Parse()
	app, err := cui.New(cui.Options{
		Title: "Project files", Width: 500, Height: 240,
		Native: *font != "", FontPath: *font,
	})
	must(err)
	defer app.Close()

	supported, err := app.SupportsAttachmentSlots()
	must(err)
	if !supported {
		panic("This example requires Attachment slots")
	}
	const message = "Quarterly report.pdf · Finance team · 1.4 MB"
	summary := cui.Label("Select Inspect for file details.")
	inspect := cui.Button("inspect").Text("Inspect").OnClick(func(cui.Event) {
		must(summary.SetText(message))
	})
	card := cui.Attachment("report", "Quarterly report.pdf", "1.4 MB").
		VisualSize(380, 0).AttachmentStatus(cui.AttachmentComplete).Children(
		cui.AttachmentSlot("preview", cui.AttachmentSlotMedia, 0).Children(cui.Label("PDF")),
		cui.AttachmentSlot("metadata", cui.AttachmentSlotMetadata, 0).Children(cui.Label("Finance team")),
		cui.AttachmentSlot("actions", cui.AttachmentSlotActions, 100).GapPx(4).Children(inspect),
	)
	must(app.Mount(cui.Column("files").GapUnits(3).Children(
		cui.Label("Project files"), card, summary,
	)))

	if *font != "" {
		must(app.Run(.016))
		return
	}
	must(app.Step(.016))
	bounds, err := app.Bounds(inspect)
	must(err)
	must(app.QueuePointer(cui.PointerDown, bounds.X+bounds.Width/2, bounds.Y+bounds.Height/2))
	must(app.QueuePointer(cui.PointerUp, bounds.X+bounds.Width/2, bounds.Y+bounds.Height/2))
	must(app.Step(.016))
	clicks, err := app.Dispatch()
	must(err)
	text, err := app.Text(summary)
	must(err)
	if clicks != 1 || text != message {
		panic(fmt.Sprintf("expected one Inspect activation, got %d and summary %q", clicks, text))
	}
	fmt.Println("Inspect updated the file summary once.")
}

func must(err error) {
	if err != nil {
		panic(err)
	}
}
