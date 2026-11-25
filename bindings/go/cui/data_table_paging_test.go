package cui

import (
	"errors"
	"fmt"
	"os"
	"strings"
	"testing"
)

func TestDataTablePaging(t *testing.T) {
	a := app(t)
	supported, err := a.SupportsDataTablePaging()
	check(t, err)
	if !supported {
		t.Fatal("current provider lacks DataTable paging")
	}
	headers := []string{"Subject"}
	keys := []uint32{11, 22, 33, 44, 55, 66, 77, 0xffffffff}
	rows := make([][]string, len(keys))
	for i := range rows {
		rows[i] = []string{fmt.Sprintf("Message %d", i)}
	}
	table := DataTable("inbox").TableDataKeyed(headers, rows, keys).VisualSize(320, 160)
	var changes []Event
	table.OnChange(func(e Event) { changes = append(changes, e) })
	check(t, a.Mount(Column("root").Children(table)))
	bounds := func(i uint32) Rect {
		t.Helper()
		r, err := a.DataTableCellBounds(table, i, 0)
		check(t, err)
		return r
	}
	poll := func(want bool) DataTableLoadMore {
		t.Helper()
		record, ok, err := table.PollLoadMore()
		check(t, err)
		if ok != want {
			t.Fatalf("poll presence = %t, want %t, record %+v", ok, want, record)
		}
		return record
	}
	check(t, a.Step(.016))
	poll(false)
	check(t, table.ConfigureLoadMore(true, 32))
	check(t, a.Step(.016))
	if early := poll(true); early.VisibleEnd >= early.RowCount {
		t.Fatalf("threshold did not request before the last row: %+v", early)
	}
	check(t, table.ConfigureLoadMore(false, 0))
	r := bounds(1)
	check(t, a.QueueScroll(r.X+10, r.Y+10, 0, -20))
	check(t, a.Step(.016))
	poll(false)
	check(t, table.ConfigureLoadMore(true, 0))
	check(t, a.Step(.016))
	check(t, table.ConfigureLoadMore(true, 0)) // Keeps an unclaimed edge.
	if err := table.ConfigureLoadMore(false, 33); err == nil {
		t.Fatal("invalid threshold was accepted")
	}

	selected := bounds(6)
	check(t, a.QueuePointer(PointerDown, selected.X+10, selected.Y+10))
	check(t, a.QueuePointer(PointerUp, selected.X+10, selected.Y+10))
	check(t, a.Step(.016))
	request := poll(true)
	if request.FrameSequence == 0 || request.RowCount != 8 || request.VisibleBegin >= 8 ||
		request.VisibleEnd != 8 || request.LastRowKey != 0xffffffff || request.Capacity != 32 {
		t.Fatalf("unexpected copied demand: %+v", request)
	}
	poll(false)
	count, err := a.Dispatch()
	check(t, err)
	if count != 1 || len(changes) != 1 || changes[0].Value != float64(keys[6]) {
		t.Fatalf("polling changed the legacy callback queue: %v", changes)
	}
	check(t, table.ConfigureLoadMore(false, 0))
	check(t, table.SetTableKeyed(headers, append(rows, []string{"Later"}), append(keys, 120)))
	check(t, a.Step(.016))
	state, err := a.DataTableState(table)
	check(t, err)
	if state.Row != keys[6] || bounds(6) != selected {
		t.Fatal("append changed selection or viewport")
	}
	poll(false)
	count, err = a.Dispatch()
	check(t, err)
	if count != 0 {
		t.Fatal("programmatic append queued a callback")
	}
	check(t, table.Remove())
	if _, ok, err := table.PollLoadMore(); err == nil || ok {
		t.Fatal("removed table can still poll")
	}
	check(t, a.Close())
	if request.LastRowKey != 0xffffffff || request.RowCount != 8 {
		t.Fatal("copied demand changed after app close")
	}
}

func TestDataTablePagingEmptyAndOwnerThread(t *testing.T) {
	a := app(t)
	table := DataTable("inbox").LoadMore(0)
	root := Column("root").Children(table)
	if _, _, err := table.PollLoadMore(); err == nil {
		t.Fatal("detached table can poll")
	}
	if err := Label("other").ConfigureLoadMore(true, 0); err == nil {
		t.Fatal("wrong component accepted")
	}
	if err := a.Mount(root); err == nil || !strings.Contains(err.Error(), "column headers") ||
		a.root != nil || table.app != nil {
		t.Fatalf("missing headers mount was not rejected transactionally: %v", err)
	}
	check(t, table.SetTableKeyed([]string{"Subject"}, nil, nil))
	check(t, a.Mount(root))
	check(t, a.Step(.016))

	later := make(chan [3]error, 1)
	go func() {
		_, support := a.SupportsDataTablePaging()
		configure := table.ConfigureLoadMore(false, 0)
		_, _, poll := table.PollLoadMore()
		later <- [3]error{support, configure, poll}
	}()
	for _, err := range <-later {
		if err == nil || !strings.Contains(err.Error(), "another OS thread") {
			t.Fatalf("off-thread operation returned %v", err)
		}
	}
	request, ok, err := table.PollLoadMore()
	check(t, err)
	if !ok || request != (DataTableLoadMore{FrameSequence: 1, Capacity: 32}) {
		t.Fatalf("empty initial demand: %+v, ok=%t", request, ok)
	}
}

func TestDataTablePagingFallback(t *testing.T) {
	mode := os.Getenv("CUI_DATA_TABLE_PAGING_FIXTURE")
	if mode != "legacy" && mode != "partial" && mode != "size" {
		t.Skip("requires an older or incompatible bridge")
	}
	a, err := New(Options{MaxNodes: 2})
	check(t, err)
	defer func() { check(t, a.Close()) }()
	supported, err := a.SupportsDataTablePaging()
	check(t, err)
	if supported {
		t.Fatal("incompatible provider advertised paging")
	}
	table := DataTable("inbox").TableData([]string{"Subject"}, [][]string{{"Legacy"}}).LoadMore(0)
	root := Column("root").Children(table)
	if err := a.Mount(root); !errors.Is(err, ErrDataTablePagingUnavailable) {
		t.Fatalf("incompatible mount returned %v", err)
	}
	if a.root != nil || table.app != nil || len(a.nodes) != 0 {
		t.Fatal("failed mount changed ownership")
	}
	check(t, table.ConfigureLoadMore(false, 0))
	check(t, a.Mount(root))
	check(t, a.Step(.016))
	if err := table.ConfigureLoadMore(true, 0); !errors.Is(err, ErrDataTablePagingUnavailable) {
		t.Fatalf("incompatible configure returned %v", err)
	}
	if _, _, err := table.PollLoadMore(); !errors.Is(err, ErrDataTablePagingUnavailable) {
		t.Fatalf("incompatible poll returned %v", err)
	}
	cell, err := a.DataTableCell(table, 0, 0)
	check(t, err)
	if cell != "Legacy" {
		t.Fatalf("ordinary table data changed: %q", cell)
	}
}

func TestDataTablePagingProtocol(t *testing.T) {
	mode := os.Getenv("CUI_DATA_TABLE_PAGING_FIXTURE")
	if mode == "" || mode == "legacy" || mode == "partial" || mode == "size" {
		t.Skip("requires a synthetic paging record")
	}
	a := app(t)
	table := DataTable("inbox").LoadMore(0)
	if mode == "explicit_capacity" {
		table.TableDataKeyedCapacity([]string{"Subject"}, [][]string{{"Fixture"}}, []uint32{7}, 48)
	} else {
		table.TableData([]string{"Subject"}, [][]string{{"Fixture"}})
	}
	check(t, a.Mount(Column("root").Children(table)))
	check(t, a.Step(.016))
	got, ok, err := table.PollLoadMore()
	if mode == "exact" {
		check(t, err)
		want := DataTableLoadMore{0xfffffffffffffffe, 8, 4, 8, 0xffffffff, 32}
		if !ok || got != want {
			t.Fatalf("record = %+v, want %+v", got, want)
		}
	} else if err == nil || !strings.Contains(err.Error(), "malformed DataTable") || ok {
		t.Fatalf("invalid wire accepted: %+v, %t, %v", got, ok, err)
	}
}
