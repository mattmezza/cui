// A headless review panel. The injected click exercises the same queue used by a host.
package main

import (
	"fmt"

	"github.com/mattmezza/cui/bindings/go/cui"
)

func main() {
	app, err := cui.New(cui.Options{Width: 420, Height: 280})
	must(err)
	defer app.Close()
	file := cui.Attachment("report", "Report.pdf", "Ready for review").VisualSize(220, 0).
		AttachmentStatus(cui.AttachmentComplete).AttachmentActions(cui.AttachmentAllActions)
	status := cui.Label("Select a file action")
	must(app.Mount(cui.Column("review").GapUnits(3).Children(file, status)))
	must(app.Step(.016))
	bounds, err := app.Bounds(file)
	must(err)
	for _, phase := range []cui.PointerPhase{cui.PointerDown, cui.PointerUp} {
		must(app.QueuePointer(phase, bounds.X+bounds.Width/2, bounds.Y+bounds.Height/2))
	}
	must(app.Step(.016))
	for {
		notice, present, err := app.PollComponentNotificationV1()
		must(err)
		if !present {
			break
		}
		if action := notice.AttachmentAction; action != nil {
			must(applyFileAction(file, status, action.Action))
		}
	}
	text, err := app.Text(status)
	must(err)
	if text != "Preview requested for Report.pdf" {
		panic(text)
	}
	fmt.Println(text)
}

func applyFileAction(file, status *cui.Node, action cui.AttachmentActions) error {
	switch action {
	case cui.AttachmentOpen:
		return status.SetText("Preview requested for Report.pdf")
	case cui.AttachmentRemove:
		if err := file.Remove(); err != nil {
			return err
		}
		return status.SetText("Removed Report.pdf from the review")
	case cui.AttachmentRetry:
		if err := file.AttachmentStatus(cui.AttachmentUploading).Value(0).Detail("Retry queued").Err(); err != nil {
			return err
		}
		return status.SetText("Upload retry requested")
	}
	return nil
}

func must(err error) {
	if err != nil {
		panic(err)
	}
}
