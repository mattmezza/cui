package cui

import (
	"errors"
	"math"
	"os"
	"testing"
)

func TestAttachmentSlotsValidation(t *testing.T) {
	for _, n := range []*Node{
		AttachmentSlot("s", 0, 0), AttachmentSlot("s", 5, 0),
		AttachmentSlot("s", AttachmentSlotMedia, 1), AttachmentSlot("s", AttachmentSlotActions, 0),
		AttachmentSlot("s", AttachmentSlotActions, 4097), AttachmentSlot("s", AttachmentSlotActions, float32(math.NaN())),
		AttachmentSlot("s", AttachmentSlotActions, float32(math.Inf(1))),
	} {
		if n.Err() == nil {
			t.Fatal("accepted invalid slot")
		}
	}
	a := app(t)
	for _, root := range []*Node{
		AttachmentSlot("s", AttachmentSlotMetadata, 0),
		Column("root").Children(AttachmentSlot("s", AttachmentSlotMedia, 0)),
		Column("root").Children(Attachment("file", "file", "ready").Children(Button("wrong"))),
		Column("root").Children(Attachment("file", "file", "ready").Children(
			AttachmentSlot("a", AttachmentSlotMedia, 0), AttachmentSlot("b", AttachmentSlotMedia, 0))),
	} {
		if a.Mount(root) == nil {
			t.Fatal("accepted invalid topology")
		}
		if a.root != nil || len(a.nodes) != 0 {
			t.Fatal("failed preflight mutated tree")
		}
	}
}

func TestAttachmentSlots(t *testing.T) {
	a := app(t)
	supported, err := a.SupportsAttachmentSlots()
	check(t, err)
	if supported == (os.Getenv("CUI_TEST_LEGACY_ATTACHMENT_SLOTS") == "1") {
		t.Fatalf("unexpected slot support %t", supported)
	}
	clicks := 0
	var button *Node
	button = Button("publish").Text("Publish").OnClick(func(event Event) {
		if event.Node != button {
			t.Fatal("slot click lost ordinary child identity")
		}
		clicks++
	})
	actions := AttachmentSlot("actions", AttachmentSlotActions, 100).GapPx(4).Children(button)
	card := Attachment("file", "report.pdf", "Ready").VisualSize(350, 0).AttachmentStatus(AttachmentComplete).Children(actions,
		AttachmentSlot("metadata", AttachmentSlotMetadata, 0).Children(Label("Local file")),
		AttachmentSlot("overlay", AttachmentSlotOverlay, 0).Children(Label("Preview")),
		AttachmentSlot("media", AttachmentSlotMedia, 0).Children(Label("PDF")))
	root := Column("root").Children(card)
	if !supported {
		if err := a.Mount(root); !errors.Is(err, ErrAttachmentSlotsUnavailable) {
			t.Fatalf("old provider mount: %v", err)
		}
		if root.app != nil || card.app != nil || button.app != nil || len(a.nodes) != 0 {
			t.Fatal("old provider partially mounted")
		}
		card.children = nil
		check(t, a.Mount(root))
		check(t, a.Step(.016))
		return
	}
	check(t, a.Mount(root))
	check(t, a.Step(.016))
	click := func(want int) {
		t.Helper()
		check(t, a.Step(.016))
		b, e := a.Bounds(button)
		check(t, e)
		if b.Width <= 0 || b.Height <= 0 {
			t.Fatal("missing child bounds")
		}
		check(t, a.QueuePointer(PointerDown, b.X+b.Width/2, b.Y+b.Height/2))
		check(t, a.QueuePointer(PointerUp, b.X+b.Width/2, b.Y+b.Height/2))
		check(t, a.Step(.016))
		_, e = a.Dispatch()
		check(t, e)
		if clicks != want {
			t.Fatalf("clicks=%d want %d", clicks, want)
		}
	}
	click(1)
	check(t, actions.GapPx(8).Disabled(true).Err())
	click(1)
	check(t, actions.Disabled(false).Err())
	check(t, card.Disabled(true).Err())
	click(1)
	check(t, card.Disabled(false).Err())
	click(2)
	check(t, actions.Remove())
	if !button.removed || !actions.removed {
		t.Fatal("slot removal failed to remove descendants")
	}
	if _, err := a.Bounds(button); err == nil {
		t.Fatal("removed child remains live")
	}
	check(t, card.Remove())
	for _, child := range card.children {
		if !child.removed {
			t.Fatal("card removal retained slot")
		}
	}
}

func TestAttachmentSlotsRejectNotification(t *testing.T) {
	a := app(t)
	supported, err := a.SupportsAttachmentSlots()
	check(t, err)
	if !supported {
		t.Skip("slot factory unavailable")
	}
	card := Attachment("file", "file", "ready").Children(
		AttachmentSlot("metadata", AttachmentSlotMetadata, 0).Children(
			Column("nested").Children(Notification("notice", "Notice", "message"))))
	if a.Mount(Column("root").Children(card)) == nil {
		t.Fatal("accepted Notification inside slot")
	}
	if len(a.nodes) != 0 {
		t.Fatal("invalid Notification partially mounted")
	}
}
