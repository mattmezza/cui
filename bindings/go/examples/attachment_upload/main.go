// Asset uploads. Pass -font FONT.ttf to open the native SDL window.
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
		Title: "Asset uploads", Width: 440, Height: 380,
		Native: *font != "", FontPath: *font,
	})
	must(err)
	defer app.Close()

	card := cui.Attachment("hero", "Workspace landing page.png", "Uploading").
		VisualSize(240, 0).AttachmentStatus(cui.AttachmentUploading).Value(.4)
	options, err := app.SupportsAttachmentOptions()
	must(err)
	if options {
		card.DesignSize(cui.LG).AttachmentOptions(cui.AttachmentOptions{
			Vertical: true, Media: true, MediaAspect: 16. / 9,
			Title: cui.AttachmentTextStyle{
				Typography: &cui.AttachmentTypography{Weight: cui.AttachmentTextSemibold},
			},
		})
	}
	preview, err := app.SupportsAttachmentPreview()
	must(err)
	if preview {
		rgba := []byte{38, 84, 166, 255, 87, 187, 170, 255, 108, 125, 207, 255, 239, 219, 173, 255}
		card.AttachmentPreviewRGBA(rgba, 2, 2, 8)
	}
	complete := cui.Button("complete").Text("Complete upload").OnClick(func(cui.Event) {
		must(card.AttachmentStatus(cui.AttachmentComplete).Value(1).Detail("Ready to publish").Err())
	})
	must(app.Mount(cui.Column("uploads").GapUnits(3).Children(
		cui.Label("Asset uploads"), card, complete,
	)))

	if *font != "" {
		must(app.Run(.016))
		return
	}
	must(app.Step(.016))
	bounds, err := app.Bounds(complete)
	must(err)
	must(app.QueuePointer(cui.PointerDown, bounds.X+bounds.Width/2, bounds.Y+bounds.Height/2))
	must(app.QueuePointer(cui.PointerUp, bounds.X+bounds.Width/2, bounds.Y+bounds.Height/2))
	must(app.Step(.016))
	clicks, err := app.Dispatch()
	must(err)
	progress, err := app.Value(card)
	must(err)
	if clicks != 1 || progress != 1 {
		panic(fmt.Sprintf("unexpected upload state: %d clicks, progress %.2f", clicks, progress))
	}
	fmt.Println("Upload complete")
}

func must(err error) {
	if err != nil {
		panic(err)
	}
}
