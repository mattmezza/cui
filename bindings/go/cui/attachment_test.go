package cui

import (
	"errors"
	"math"
	"os"
	"testing"
)

func TestAttachmentPresentation(t *testing.T) {
	a := app(t)
	options, err := a.SupportsAttachmentOptions()
	check(t, err)
	preview, err := a.SupportsAttachmentPreview()
	check(t, err)
	want := os.Getenv("CUI_TEST_LEGACY_ATTACHMENT") != "1"
	if options != want || preview != want {
		t.Fatalf("options=%t preview=%t, want both %t", options, preview, want)
	}

	pixels := []byte{255, 0, 0, 255, 0, 255, 0, 255, 0, 0, 0, 0}
	failed := AttachmentFailed
	color := uint32(0x5c39adff)
	card := Attachment("file", "Quarterly-report-with-a-very-long-name.pdf", "Uploading").
		VisualSize(200, 0).AttachmentStatus(AttachmentUploading).Value(.4).
		AttachmentOptions(AttachmentOptions{
			Vertical: true, MediaAspect: 2, TitleStatus: &failed,
			Title: AttachmentTextStyle{ColorRGBA: &color,
				Typography: &AttachmentTypography{Weight: AttachmentTextSemibold}},
		}).AttachmentPreviewRGBA(pixels, 2, 1, 12)
	check(t, card.Err())
	pixels[0], failed, color = 0, AttachmentState(99), 0
	if card.attachmentPreview.pixels[0] != 255 ||
		card.attachmentOptions.title_status != 3 ||
		card.attachmentOptions.title.color_rgba != 0x5c39adff {
		t.Fatal("pending presentation and pixels must be copies")
	}

	root := Column("root").Children(card)
	if !want {
		if err := a.Mount(root); !errors.Is(err, ErrAttachmentOptionsUnavailable) {
			t.Fatalf("options failure was %v", err)
		}
		check(t, card.SetAttachmentOptions(nil))
		if err := a.Mount(root); !errors.Is(err, ErrAttachmentPreviewUnavailable) {
			t.Fatalf("preview failure was %v", err)
		}
		check(t, card.ClearAttachmentPreview())
		check(t, a.Mount(root))
		check(t, a.Step(.016))
		if v, err := a.Value(card); err != nil || v != .4 {
			t.Fatalf("legacy progress returned %v, %v", v, err)
		}
		return
	}

	card.DesignSize(XL)
	check(t, a.Mount(root))
	if card.attachmentPreview != nil {
		t.Fatal("builder kept pending pixels after a successful mount")
	}
	check(t, a.Step(.016))
	wide, err := a.Bounds(card)
	check(t, err)
	if wide.Width != 200 || wide.Height <= 100 {
		t.Fatalf("expected a vertical preview and metadata, got %+v", wide)
	}
	if card.SetText("line\nbreak") == nil ||
		card.SetAttachmentOptions(&AttachmentOptions{MediaAspect: float32(math.NaN())}) == nil ||
		card.SetAttachmentPreviewRGBA(pixels[:4], 2, 1, 12) == nil {
		t.Fatal("accepted invalid labels, aspect or RGBA data")
	}
	check(t, a.Step(.016))
	unchanged, err := a.Bounds(card)
	check(t, err)
	if unchanged != wide {
		t.Fatalf("rejected updates changed the geometry to %+v", unchanged)
	}
	check(t, card.SetAttachmentOptions(&AttachmentOptions{Vertical: true, MediaAspect: 4}))
	check(t, card.SetAttachmentPreviewRGBA(pixels, 2, 1, 12))
	check(t, a.Step(.016))
	flat, err := a.Bounds(card)
	check(t, err)
	if flat.Height >= wide.Height-20 {
		t.Fatalf("aspect change should flatten the preview, got %+v", flat)
	}
	check(t, card.ClearAttachmentPreview())
	check(t, a.Step(.016))
	textOnly, err := a.Bounds(card)
	check(t, err)
	if textOnly.Height >= flat.Height-20 {
		t.Fatalf("clearing the preview should collapse its slot, got %+v", textOnly)
	}
	check(t, card.SetAttachmentOptions(nil))
	check(t, card.SetText("line\nbreak"))
	check(t, card.Remove())
	if card.ClearAttachmentPreview() == nil {
		t.Fatal("cleared a removed Attachment")
	}
}
