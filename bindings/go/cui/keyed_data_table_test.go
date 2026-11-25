package cui

import (
	"encoding/json"
	"errors"
	"fmt"
	"os"
	"strings"
	"testing"
)

func TestKeyedDataTable(t *testing.T) {
	a := app(t)
	supported, err := a.SupportsKeyedDataTable()
	check(t, err)
	if !supported {
		t.Fatal("current provider does not expose keyed DataTable replacement")
	}
	headers := []string{"Subject", "Sender"}
	rows := make([][]string, 8)
	for i := range rows {
		rows[i] = []string{fmt.Sprintf("Message %d", i), "Team"}
	}
	keys := []uint32{11, 22, 0xffffffff, 44, 55, 66, 77, 88}
	initialKeys := append([]uint32(nil), keys...)
	table := DataTable("inbox").TableDataKeyed(headers, rows, keys).
		VisualSize(320, 160).SelectionMode(SelectCells)
	var changes []Event
	table.OnChange(func(event Event) { changes = append(changes, event) })
	keys[2] = 999
	check(t, a.Mount(Column("root").Children(table)))
	check(t, a.Step(.016))

	bounds := func(row, column uint32) Rect {
		t.Helper()
		cell, err := a.DataTableCellBounds(table, row, column)
		check(t, err)
		return cell
	}
	press := func(cell Rect) {
		t.Helper()
		x, y := cell.X+cell.Width/2, cell.Y+cell.Height/2
		check(t, a.QueuePointer(PointerDown, x, y))
		check(t, a.QueuePointer(PointerUp, x, y))
		check(t, a.Step(.016))
		_, err := a.Dispatch()
		check(t, err)
	}
	selectedKey := func(expected uint32) {
		t.Helper()
		state, err := a.DataTableState(table)
		check(t, err)
		if state.Row != expected {
			t.Fatalf("selected row = %d, want %d", state.Row, expected)
		}
	}
	expectChange := func(expected uint32) {
		t.Helper()
		if len(changes) != 1 {
			t.Fatalf("expected one change, got %+v", changes)
		}
		var change struct {
			Row uint32 `json:"row"`
		}
		check(t, json.Unmarshal([]byte(changes[0].Text), &change))
		if change.Row != expected || changes[0].Value != float64(expected) {
			t.Fatalf("selection payload = %+v, want key %d", changes[0], expected)
		}
		changes = nil
	}

	first := bounds(1, 0)
	check(t, a.QueueScroll(first.X+10, first.Y+10, 0, -1))
	check(t, a.Step(.016))
	offset := bounds(1, 0).Y
	if offset != first.Y-48 {
		t.Fatalf("wheel offset = %f, want %f", offset, first.Y-48)
	}
	selected := bounds(2, 1)
	press(selected)
	selectedKey(0xffffffff)
	expectChange(0xffffffff)

	for _, invalid := range [][]uint32{nil, {0}, {1, 2}} {
		if err := table.SetTableKeyed(headers, rows[:1], invalid); err == nil {
			t.Fatalf("accepted invalid keys %v", invalid)
		}
	}
	if err := table.SetTableKeyed(headers, rows[:2], []uint32{44, 44}); err == nil {
		t.Fatal("accepted duplicate keys")
	}
	if err := table.SetTableKeyed(headers, [][]string{{strings.Repeat("x", 129), "Team"}}, []uint32{44}); err == nil {
		t.Fatal("accepted oversized cell")
	}
	value, err := a.DataTableCell(table, 2, 0)
	check(t, err)
	if value != "Message 2" {
		t.Fatalf("invalid update replaced old data: %q", value)
	}
	selectedKey(0xffffffff)

	updatedRows := append([][]string{{"New message", "Owner"}}, rows...)
	updatedKeys := append([]uint32{9}, initialKeys...)
	check(t, table.SetTableKeyed(headers, updatedRows, updatedKeys))
	check(t, a.Step(.016))
	if bounds(2, 0).Y != offset || bounds(3, 1) != selected {
		t.Fatal("prepending rows moved the original viewport or selected cell")
	}
	selectedKey(0xffffffff)
	_, err = a.Dispatch()
	check(t, err)
	if len(changes) != 0 {
		t.Fatal("programmatic update queued a selection event")
	}

	check(t, a.QueueKey(KeyDown, Down, 0, false))
	check(t, a.Step(.016))
	_, err = a.Dispatch()
	check(t, err)
	selectedKey(44)
	expectChange(44)

	check(t, table.SetTableKeyed(headers, nil, nil))
	check(t, a.Step(.016))
	selectedKey(0)
	check(t, table.SetTable(headers, rows))
	check(t, a.Step(.016))
	if bounds(1, 0).Y != first.Y {
		t.Fatal("legacy replacement did not reset the viewport")
	}
	press(bounds(1, 1))
	selectedKey(2)
	expectChange(2)
}

func TestKeyedDataTableLegacyProvider(t *testing.T) {
	if os.Getenv("CUI_KEYED_DATA_TABLE_LEGACY") != "1" {
		t.Skip("requires the actual older bridge")
	}
	a, err := New(Options{MaxNodes: 2})
	check(t, err)
	defer func() { check(t, a.Close()) }()
	supported, err := a.SupportsKeyedDataTable()
	check(t, err)
	if supported {
		t.Fatal("legacy provider advertised the keyed setter")
	}
	headers := []string{"Subject"}
	table := DataTable("inbox").TableDataKeyed(headers, [][]string{{"Keyed"}}, []uint32{7})
	root := Column("root").Children(table)
	if err := a.Mount(root); !errors.Is(err, ErrKeyedDataTableUnavailable) {
		t.Fatalf("keyed mount returned %v", err)
	}
	if a.root != nil || table.app != nil || len(a.nodes) != 0 {
		t.Fatal("failed mount modified app or node ownership")
	}

	check(t, table.SetTable(headers, [][]string{{"Legacy"}}))
	check(t, a.Mount(root))
	check(t, a.Step(.016))
	if err := table.SetTableKeyed(headers, [][]string{{"Rejected"}}, []uint32{7}); !errors.Is(err, ErrKeyedDataTableUnavailable) {
		t.Fatalf("keyed update returned %v", err)
	}
	value, err := a.DataTableCell(table, 0, 0)
	check(t, err)
	if value != "Legacy" {
		t.Fatalf("unsupported update replaced old data: %q", value)
	}
}
