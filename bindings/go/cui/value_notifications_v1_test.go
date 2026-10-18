package cui

import (
	"os"
	"testing"
)

func TestValueNotificationV1RealQueue(t *testing.T) {
	if os.Getenv("CUI_VALUE_REAL") == "" {
		t.Skip("explicit current library required")
	}
	a := app(t)
	number := NumberInput("number").Range(0, 100000000, .25).Value(16777216.25)
	calls := 0
	number.OnChange(func(Event) { calls++ })
	check(t, a.Mount(Column("root").Children(number)))
	check(t, a.Step(.016))
	press(t, a, number)
	check(t, a.QueueKey(KeyDown, Up, 0, false))
	check(t, a.Step(.016))
	scalar, ok, e := a.PollValueNotificationV1()
	check(t, e)
	if !ok || scalar.Kind != ValueNotificationScalarChangeV1 || scalar.Scalar.Value != 16777216.5 || calls != 0 {
		t.Fatalf("scalar %+v", scalar)
	}
	number.Value(42)
	check(t, a.Step(.016))
	_, ok, e = a.PollValueNotificationV1()
	check(t, e)
	if ok {
		t.Fatal("setter emitted notification")
	}
	number.Disabled(true)
	press(t, a, number)
	check(t, a.QueueKey(KeyDown, Up, 0, false))
	check(t, a.Step(.016))
	_, ok, e = a.PollValueNotificationV1()
	check(t, e)
	if ok {
		t.Fatal("disabled control emitted notification")
	}
	check(t, a.Close())
	if scalar.Scalar.Value != 16777216.5 {
		t.Fatal("copy")
	}
}
func TestValueNotificationV1TextFIFO(t *testing.T) {
	if os.Getenv("CUI_VALUE_REAL") == "" && os.Getenv("CUI_VALUE_OLD") == "" {
		t.Skip("explicit library required")
	}
	a := app(t)
	field := Input("text")
	check(t, a.Mount(Column("root").Children(field)))
	check(t, a.Step(.016))
	press(t, a, field)
	check(t, a.QueueText("héllo ✓"))
	check(t, a.Step(.016))
	if os.Getenv("CUI_VALUE_OLD") != "" {
		_, _, e := a.PollValueNotificationV1()
		if e == nil {
			t.Fatal("expected unavailable")
		}
		old, ok, e := a.PollNotificationV1()
		check(t, e)
		if !ok || old.Legacy.Text != "héllo ✓" {
			t.Fatal("unavailable drained queue")
		}
		return
	}
	old, ok, e := a.PollNotificationV1()
	check(t, e)
	if !ok || old.Kind != NotificationLegacyV1 || old.Legacy.Text != "héllo ✓" {
		t.Fatal("old view")
	}
	check(t, a.QueueText("!"))
	check(t, a.Step(.016))
	check(t, a.QueueText("?"))
	check(t, a.Step(.016))
	n, ok, e := a.PollValueNotificationV1()
	check(t, e)
	if !ok || n.Kind != ValueNotificationTextChangeV1 || n.Text.Text != "héllo ✓!" {
		t.Fatal("text")
	}
	old, ok, e = a.PollNotificationV1()
	check(t, e)
	if !ok || old.Legacy.Text != "héllo ✓!?" {
		t.Fatal("FIFO")
	}
	check(t, a.Close())
	if n.Text.Text != "héllo ✓!" {
		t.Fatal("copy")
	}
}
func TestValueNotificationV1Selection(t *testing.T) {
	if os.Getenv("CUI_VALUE_REAL") == "" {
		t.Skip("explicit current library required")
	}
	a := app(t)
	first, second := Item("first", "First"), Item("second", "Second")
	group := RadioGroup("group").Children(first, second).Selected(first)
	check(t, a.Mount(Column("root").Children(group)))
	check(t, a.Step(.016))
	press(t, a, second)
	live, ok, e := a.PollValueNotificationV1()
	check(t, e)
	if !ok || live.Kind != ValueNotificationSelectionChangeV1 || live.Selection.Node != second {
		t.Fatal("live selection")
	}
	press(t, a, first)
	_, _, e = a.PollNotificationV1()
	check(t, e)
	press(t, a, second)
	handle := live.Selection.Handle
	check(t, second.Remove())
	// Actual removal leaves a copied historical identity. Public Go cannot append replacements after mount.
	n, ok, e := a.PollValueNotificationV1()
	check(t, e)
	if !ok || n.Selection.Handle != handle || n.Selection.Node != nil {
		t.Fatal("historical handle")
	}
}
func TestValueNotificationV1Protocol(t *testing.T) {
	mode := os.Getenv("CUI_VALUE_FIXTURE")
	if mode == "" {
		t.Skip("synthetic provider required")
	}
	a := app(t)
	if mode != "edges" && mode != "capacity" {
		_, _, e := a.PollValueNotificationV1()
		if e == nil {
			t.Fatal("invalid protocol accepted")
		}
		return
	}
	// Synthetic registry replacement: same slot, different generation must not resolve.
	a.nodes[uint64(^uint32(0))<<32|uint64(^uint32(0)-1)] = &Node{key: "replacement-generation"}
	for i, f := range []uint64{9007199254740991, 9007199254740992, 9007199254740993, ^uint64(0)} {
		n, ok, e := a.PollValueNotificationV1()
		check(t, e)
		if !ok || n.Frame != f || n.Handle.Slot != ^uint32(0) {
			t.Fatal("precision")
		}
		if i == 0 && (n.Kind != ValueNotificationScalarChangeV1 || n.Scalar.Value != 16777216.5) {
			t.Fatal("scalar")
		}
		if i == 1 {
			length := 3
			if mode == "capacity" {
				length = 8192
			}
			if n.Kind != ValueNotificationTextChangeV1 || len(n.Text.Text) != length {
				t.Fatal("text")
			}
		}
		if i >= 2 && (n.Selection.Handle.Slot != ^uint32(0) || n.Selection.Node != nil) {
			t.Fatal("selection")
		}
	}
}
