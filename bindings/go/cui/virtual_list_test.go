package cui

import (
	"errors"
	"fmt"
	"math"
	"os"
	"testing"
)

func TestVirtualListLiveRows(t *testing.T) {
	a := app(t)
	items := make([]*Node, 10)
	rows := make([]VirtualListRow, len(items))
	for i := range items {
		key := fmt.Sprintf("message-%d", i)
		items[i] = Item(key, key)
		rows[i].Item = items[i]
	}
	list := VirtualList("inbox").VisualSize(220, 96).Children(items...).Value(109.5)
	foreignItem := Item("from-other-list", "Other")
	other := VirtualList("other").VisualSize(220, 96).Children(foreignItem)
	var changes []float64
	list.OnChange(func(event Event) { changes = append(changes, event.Value) })
	check(t, a.Mount(Column("root").Children(list, other)))
	supported, err := a.SupportsVirtualListRows()
	check(t, err)
	if supported == (os.Getenv("CUI_TEST_LEGACY_VIRTUAL_LIST") == "1") {
		t.Fatalf("unexpected live row capability: %v", supported)
	}
	check(t, a.Step(.016))
	expectRow := func(item *Node, offset float64, top, extent float32) {
		t.Helper()
		listBounds, err := a.Bounds(list)
		check(t, err)
		row, err := a.VirtualListItemBounds(list, item)
		check(t, err)
		value, err := a.Value(list)
		check(t, err)
		if value != offset || row.Y-listBounds.Y != top || row.Height != extent {
			t.Fatalf("offset %.1f, row %+v relative to %+v", value, row, listBounds)
		}
	}
	expectRow(items[3], 109.5, -13.5, 32)
	if !supported {
		if !errors.Is(list.PreserveVirtualListAnchor(true), ErrVirtualListRowsUnavailable) ||
			!errors.Is(list.SetVirtualListRows(rows), ErrVirtualListRowsUnavailable) {
			t.Fatal("older bridge did not report the live rows omission")
		}
		return
	}
	check(t, list.PreserveVirtualListAnchor(true))
	if _, err := list.AppendVirtualListItem("message-0", "Duplicate"); err == nil {
		t.Fatal("accepted an existing key")
	}
	if _, err := list.AppendVirtualListItem("bad-item", "bad\x00text"); err == nil {
		t.Fatal("accepted invalid text")
	}
	newItem, err := list.AppendVirtualListItem("new-message", "New message")
	check(t, err)
	rows = append([]VirtualListRow{{Item: newItem, Extent: 48.5}}, rows...)
	invalid := append([]VirtualListRow(nil), rows...)
	invalid[len(rows)-1].Item = foreignItem
	if err := list.SetVirtualListRows(invalid); err == nil {
		t.Fatal("accepted an Item from another list")
	}
	invalid[len(rows)-1] = invalid[0]
	if err := list.SetVirtualListRows(invalid); err == nil {
		t.Fatal("accepted a duplicate Item")
	}
	invalid[len(rows)-1] = rows[len(rows)-1]
	invalid[0].Extent = float32(math.NaN())
	if err := list.SetVirtualListRows(invalid); err == nil {
		t.Fatal("accepted an invalid extent")
	}
	if err := list.SetVirtualListRows(rows[1:]); err == nil {
		t.Fatal("accepted incomplete row projection")
	}
	check(t, a.Step(.016))
	expectRow(items[3], 109.5, -13.5, 32)

	check(t, list.SetVirtualListRows(rows))
	rows[0].Extent = 62.5
	check(t, a.Step(.016))
	expectRow(items[3], 158, -13.5, 32)
	check(t, list.SetVirtualListRows(rows))
	check(t, a.Step(.016))
	expectRow(items[3], 172, -13.5, 32)
	_, err = a.Dispatch()
	check(t, err)
	if len(changes) != 0 {
		t.Fatalf("anchor corrections dispatched changes: %v", changes)
	}
	bounds, err := a.Bounds(list)
	check(t, err)
	check(t, a.QueueScroll(bounds.X+20, bounds.Y+20, 0, -1))
	check(t, a.Step(.016))
	_, err = a.Dispatch()
	check(t, err)
	expectRow(items[4], 204, -13.5, 32)
	if len(changes) != 1 || changes[0] != 204 {
		t.Fatalf("expected one user scroll change, got %v", changes)
	}
	check(t, items[4].Remove())
	if err := list.SetVirtualListRows(rows); err == nil {
		t.Fatal("accepted a removed Item")
	}
	rows = append(rows[:5], rows[6:]...)
	check(t, list.SetVirtualListRows(rows))
	check(t, a.Step(.016))
	expectRow(items[5], 190.5, 0, 32)
	list.Value(7.25)
	check(t, list.Err())
	rows[0].Extent = 80
	check(t, list.SetVirtualListRows(rows))
	check(t, a.Step(.016))
	expectRow(newItem, 7.25, -7.25, 80)
}
