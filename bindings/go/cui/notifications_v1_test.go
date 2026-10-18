package cui

import (
	"os"
	"testing"
)

func TestNotificationV1Queue(t *testing.T) {
	if os.Getenv("CUI_NOTIFICATION_FIXTURE") != "" || os.Getenv("CUI_NOTIFICATION_UNAVAILABLE") != "" {
		t.Skip("other library")
	}
	a := app(t)
	box := Checkbox("box").Text("Check")
	b := Button("go").Text("Go")
	calls := 0
	b.OnClick(func(Event) { calls++ })
	check(t, a.Mount(Column("root").Children(box, b)))
	check(t, a.Step(.016))
	press(t, a, box)
	press(t, a, box)
	old, ok, e := a.PollEvent()
	check(t, e)
	if !ok || !old.Checked {
		t.Fatal("legacy FIFO")
	}
	n, ok, e := a.PollNotificationV1()
	check(t, e)
	if !ok || n.Kind != NotificationBooleanChangeV1 || n.Boolean.Value || n.Node != box || !n.HasFrame {
		t.Fatalf("snapshot %+v", n)
	}
	press(t, a, b)
	v, ok, e := a.PollNotificationV1()
	check(t, e)
	if !ok || v.Kind != NotificationActivationV1 || calls != 0 {
		t.Fatal("manual activation")
	}
	press(t, a, b)
	check(t, b.Remove())
	_, ok, e = a.PollNotificationV1()
	check(t, e)
	if ok {
		t.Fatal("stale origin")
	}
}
func TestNotificationV1CapacityPolicy(t *testing.T) {
	for _, required := range []uint32{0, 4097, 1048577} {
		calls := 0
		_, e := readNotificationTextV1(func([]byte) (uint32, uint32, error) { calls++; return 5, required, nil })
		if e == nil || calls != 1 {
			t.Fatal(required)
		}
	}
	calls := 0
	_, e := readNotificationTextV1(func(b []byte) (uint32, uint32, error) { calls++; return 5, uint32(len(b) + 1), nil })
	if e == nil || calls != 2 {
		t.Fatal("unbounded retry")
	}
}
func TestNotificationV1ProtocolFixture(t *testing.T) {
	mode := os.Getenv("CUI_NOTIFICATION_FIXTURE")
	if mode == "" {
		t.Skip("injected library required")
	}
	a := app(t)
	if mode != "edges" && mode != "capacity" {
		_, _, e := a.PollNotificationV1()
		if e == nil {
			t.Fatal("invalid protocol")
		}
		return
	}
	for i, f := range []uint64{9007199254740991, 9007199254740992, 9007199254740993, ^uint64(0)} {
		n, ok, e := a.PollNotificationV1()
		check(t, e)
		if !ok || n.Frame != f || n.Legacy.Frame != f || n.Handle.Slot != ^uint32(0) || n.Legacy.CheckedRaw != ^uint32(0) {
			t.Fatalf("lossy %+v", n)
		}
		if i == 2 && n.Table.Column != ^uint32(0)-1 {
			t.Fatal("column")
		}
		if mode == "capacity" && len(n.Legacy.Text) != 8192 {
			t.Fatal("text retry")
		}
	}
}
func TestNotificationV1Unavailable(t *testing.T) {
	if os.Getenv("CUI_NOTIFICATION_UNAVAILABLE") == "" {
		t.Skip("older library required")
	}
	a := app(t)
	_, _, e := a.PollNotificationV1()
	if e == nil {
		t.Fatal("missing symbols accepted")
	}
	check(t, a.Mount(Column("root")))
	check(t, a.Step(.016))
}

func TestNotificationV1TextAndTable(t *testing.T) {
	if os.Getenv("CUI_NOTIFICATION_FIXTURE") != "" || os.Getenv("CUI_NOTIFICATION_UNAVAILABLE") != "" {
		t.Skip("other library")
	}
	a := app(t)
	input := Input("text").Text("hé")
	table := DataTable("table").SelectionMode(SelectCells).TableData([]string{"A", "B", "C"}, [][]string{{"a", "b", "c"}}).VisualSize(320, 150)
	check(t, a.Mount(Column("root").Children(input, table)))
	check(t, a.Step(.016))
	press(t, a, input)
	check(t, a.QueueText("Z"))
	check(t, a.Step(.016))
	n, ok, e := a.PollNotificationV1()
	check(t, e)
	if !ok || n.Kind != NotificationLegacyV1 || n.Legacy.Text != "héZ" {
		t.Fatalf("legacy text %+v", n)
	}
	check(t, a.QueueText("Q"))
	check(t, a.Step(.016))
	_, _, e = a.PollNotificationV1()
	check(t, e)
	if n.Legacy.Text != "héZ" {
		t.Fatal("borrowed text")
	}
	cell, e := a.DataTableCellBounds(table, 0, 2)
	check(t, e)
	check(t, a.QueuePointer(PointerDown, cell.X+cell.Width/2, cell.Y+cell.Height/2))
	check(t, a.QueuePointer(PointerUp, cell.X+cell.Width/2, cell.Y+cell.Height/2))
	check(t, a.Step(.016))
	n, ok, e = a.PollNotificationV1()
	check(t, e)
	if !ok || n.Kind != NotificationTableChangeV1 || n.Table.Column != 3 {
		t.Fatalf("table %+v", n)
	}
}
