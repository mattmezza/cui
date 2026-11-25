package cui

import (
	"errors"
	"fmt"
	"os"
	"strings"
	"testing"
)

func TestDataTableCapacity(t *testing.T) {
	a := app(t)
	headers := []string{"Subject"}
	rows, keys := make([][]string, 40), make([]uint32, 40)
	for i := range rows {
		rows[i] = []string{fmt.Sprintf("Message %d", i)}
		keys[i] = uint32(100 + i)
	}
	keys[31] = 0xffffffff
	table := DataTable("inbox").TableDataKeyedCapacity(headers, rows[:32], keys[:32], 48).
		LoadMore(0).VisualSize(320, 160)
	var changes []Event
	table.OnChange(func(e Event) { changes = append(changes, e) })
	check(t, a.Mount(Column("root").Children(table)))
	check(t, a.Step(.016))
	if _, ok, err := table.PollLoadMore(); err != nil || ok {
		t.Fatalf("initial viewport requested a page: ok=%t, err=%v", ok, err)
	}
	bounds := func(row uint32) Rect {
		t.Helper()
		rect, err := a.DataTableCellBounds(table, row, 0)
		check(t, err)
		return rect
	}
	first := bounds(0)
	check(t, a.QueueScroll(first.X+10, first.Y+10, 0, -40))
	check(t, a.Step(.016))
	selected := bounds(30)
	check(t, a.QueuePointer(PointerDown, selected.X+10, selected.Y+10))
	check(t, a.QueuePointer(PointerUp, selected.X+10, selected.Y+10))
	check(t, a.Step(.016))

	if err := table.SetTableKeyedCapacity(headers, rows, keys, 39); err == nil {
		t.Fatal("update exceeded capacity")
	}
	request, ok, err := table.PollLoadMore()
	check(t, err)
	if !ok || request.FrameSequence == 0 || request.Capacity != 48 || request.RowCount != 32 ||
		request.VisibleEnd != 32 || request.VisibleBegin > 30 || request.LastRowKey != 0xffffffff {
		t.Fatalf("unexpected request after rejected replacement: %+v, ok=%t", request, ok)
	}
	count, err := a.Dispatch()
	check(t, err)
	if count != 1 || len(changes) != 1 || changes[0].Value != float64(keys[30]) {
		t.Fatalf("expected one selected row callback, got %v", changes)
	}
	check(t, table.SetTableKeyedCapacity(headers, rows, keys, 64))
	check(t, a.Step(.016))
	state, err := a.DataTableState(table)
	check(t, err)
	if state.Row != keys[30] || bounds(30) != selected {
		t.Fatal("append changed selection or viewport")
	}
	cell, err := a.DataTableCell(table, 39, 0)
	check(t, err)
	if cell != rows[39][0] {
		t.Fatalf("last row is %q", cell)
	}
	if count, err := a.Dispatch(); err != nil || count != 0 {
		t.Fatalf("programmatic append dispatched %d events: %v", count, err)
	}
	check(t, a.QueueScroll(first.X+10, first.Y+10, 0, -40))
	check(t, a.Step(.016))
	after, ok, err := table.PollLoadMore()
	check(t, err)
	if !ok || after.FrameSequence <= request.FrameSequence || after.Capacity != 64 ||
		after.RowCount != 40 || after.VisibleEnd != 40 || after.LastRowKey != keys[39] {
		t.Fatalf("unexpected request after appended page: %+v, ok=%t", after, ok)
	}
	check(t, table.SetTableKeyed(headers, nil, nil))
	check(t, a.Step(.016))
	legacy, ok, err := table.PollLoadMore()
	check(t, err)
	if !ok || legacy.Capacity != 32 || legacy.RowCount != 0 || legacy.LastRowKey != 0 {
		t.Fatalf("ordinary replacement did not restore the default: %+v, ok=%t", legacy, ok)
	}
	check(t, a.Close())
	if request.Capacity != 48 || request.RowCount != 32 {
		t.Fatal("copied request changed when closing the app")
	}
}

func TestDataTableCapacityLimits(t *testing.T) {
	a := app(t)
	maximum, err := a.DataTableMaxRowCapacity()
	check(t, err)
	if maximum != 10_000 {
		t.Fatalf("unexpected capacity limit %d", maximum)
	}
	headers := []string{"Subject"}
	rows, keys := make([][]string, maximum), make([]uint32, maximum)
	for i := range rows {
		rows[i], keys[i] = []string{"Inbox"}, uint32(i+1)
	}
	rows[len(rows)-1][0] = "Last"
	table := DataTable("inbox")
	if err := table.SetTableKeyed(headers, rows[:33], keys[:33]); err == nil {
		t.Fatal("ordinary setter accepted more than 32 rows")
	}
	for _, invalid := range []struct {
		rows [][]string
		keys []uint32
		cap  uint32
	}{
		{nil, nil, 0},
		{nil, nil, maximum + 1},
		{rows[:2], keys[:2], 1},
		{rows[:1], nil, 2},
		{rows[:2], []uint32{7, 7}, 2},
		{rows[:1], []uint32{0}, 2},
		{[][]string{{"A", "B"}}, keys[:1], 2},
		{[][]string{{strings.Repeat("é", 65)}}, keys[:1], 2},
		{[][]string{{"with\x00NUL"}}, keys[:1], 2},
	} {
		if err := table.SetTableKeyedCapacity(headers, invalid.rows, invalid.keys, invalid.cap); err == nil {
			t.Fatalf("accepted invalid snapshot with capacity %d", invalid.cap)
		}
	}
	check(t, table.SetTableKeyedCapacity(headers, rows, keys, maximum))
	rows[len(rows)-1][0], keys[len(keys)-1], headers[0] = "Modified", 0, "Modified"
	check(t, a.Mount(Column("root").Children(table.LoadMore(0))))
	check(t, a.Step(.016))
	cell, err := a.DataTableCell(table, maximum-1, 0)
	check(t, err)
	if cell != "Last" || table.tableHeaders[0] != "Subject" {
		t.Fatal("detached table did not copy its input")
	}

	done := make(chan [2]error, 1)
	go func() {
		_, queryErr := a.DataTableMaxRowCapacity()
		setErr := table.SetTableKeyedCapacity(headers, nil, nil, 1)
		done <- [2]error{queryErr, setErr}
	}()
	for _, err := range <-done {
		if err == nil || !strings.Contains(err.Error(), "another OS thread") {
			t.Fatalf("off-thread operation returned %v", err)
		}
	}
	check(t, table.SetTableKeyedCapacity(headers, nil, nil, 1))
	check(t, a.Step(.016))
	request, ok, err := table.PollLoadMore()
	check(t, err)
	if !ok || request.RowCount != 0 || request.Capacity != 1 {
		t.Fatalf("empty demand: %+v, ok=%t", request, ok)
	}
}

func TestDataTableCapacityProvider(t *testing.T) {
	mode := os.Getenv("CUI_DATA_TABLE_CAPACITY_FIXTURE")
	if mode == "" {
		t.Skip("requires an optional capacity provider fixture")
	}
	a, err := New(Options{MaxNodes: 2})
	check(t, err)
	defer func() { check(t, a.Close()) }()
	expect := map[string]uint32{"limited": 48, "large": 10_000, "rejected": 10_000}
	maximum, err := a.DataTableMaxRowCapacity()
	check(t, err)
	if maximum != expect[mode] {
		t.Fatalf("%s: capacity = %d, want %d", mode, maximum, expect[mode])
	}
	headers, rows, keys := []string{"Subject"}, [][]string{{"Ordinary"}}, []uint32{7}
	table := DataTable("inbox")
	root := Column("root").Children(table)
	if maximum < 64 {
		check(t, table.SetTableKeyedCapacity(headers, rows, keys, 64))
		assertUnsupported := func(err error) {
			t.Helper()
			if maximum == 0 && !errors.Is(err, ErrDataTableCapacityUnavailable) ||
				maximum == 48 && (err == nil || !strings.Contains(err.Error(), "48-row limit")) {
				t.Fatalf("capacity preflight returned %v", err)
			}
		}
		assertUnsupported(a.Mount(root))
		if a.root != nil || table.app != nil || len(a.nodes) != 0 {
			t.Fatal("preflight failure attached nodes")
		}
		check(t, table.SetTable(headers, rows))
		check(t, a.Mount(root))
		assertUnsupported(table.SetTableKeyedCapacity(headers, [][]string{{"Rejected"}}, keys, 64))
	} else {
		check(t, table.SetTable(headers, rows))
		check(t, a.Mount(root))
		err = table.SetTableKeyedCapacity(headers, [][]string{{"Updated"}}, keys, 64)
		if mode == "rejected" {
			if err == nil || table.tableRowCapacity != 0 || table.tableRows[0][0] != "Ordinary" {
				t.Fatalf("provider failure changed the binding snapshot: %v", err)
			}
		} else {
			check(t, err)
			rows = [][]string{{"Updated"}}
		}
	}
	check(t, a.Step(.016))
	cell, err := a.DataTableCell(table, 0, 0)
	check(t, err)
	if cell != rows[0][0] {
		t.Fatalf("native cell = %q, want %q", cell, rows[0][0])
	}
}
