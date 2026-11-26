package main

import (
	"flag"
	"fmt"

	"github.com/mattmezza/cui/bindings/go/cui"
)

func main() {
	font := flag.String("font", "", "TrueType font for a native SDL window")
	flag.Parse()
	app, err := cui.New(cui.Options{
		Title: "Live inbox", Width: 480, Height: 320, Native: *font != "", FontPath: *font,
	})
	must(err)
	defer app.Close()
	supported, err := app.SupportsVirtualListRows()
	must(err)

	items := make([]*cui.Node, 12)
	rows := make([]cui.VirtualListRow, len(items))
	for i := range items {
		label := fmt.Sprintf("Inbox message %d", i+1)
		items[i] = cui.Item(fmt.Sprintf("message-%d", i), label)
		rows[i].Item = items[i]
	}
	list := cui.VirtualList("messages").VisualSize(380, 176).Children(items...).Value(105.5)
	prepend := cui.Button("prepend").Text("Prepend message").Disabled(!supported)
	resize := cui.Button("resize").Text("Resize first row").Disabled(!supported)
	prepend.OnClick(func(cui.Event) {
		if len(rows) == 64 {
			return
		}
		label := fmt.Sprintf("New message %d", len(rows)-11)
		newItem, err := list.AppendVirtualListItem(label, label)
		must(err)
		rows = append([]cui.VirtualListRow{{Item: newItem, Extent: 60}}, rows...)
		must(list.SetVirtualListRows(rows))
		if len(rows) == 64 {
			prepend.Disabled(true)
		}
	})
	resize.OnClick(func(cui.Event) {
		if rows[0].Extent == 86 {
			rows[0].Extent = 60
		} else {
			rows[0].Extent = 86
		}
		must(list.SetVirtualListRows(rows))
	})
	must(app.Mount(cui.Column("inbox").GapUnits(2).Children(
		cui.Label("Live inbox"), cui.Row("actions").GapUnits(2).Children(prepend, resize), list,
	)))
	if supported {
		must(list.PreserveVirtualListAnchor(true))
	}
	if *font != "" {
		must(app.Run(.016))
		return
	}

	must(app.Step(.016))
	expected := 105.5
	if supported {
		for _, control := range []*cui.Node{prepend, resize} {
			bounds, err := app.Bounds(control)
			must(err)
			x, y := bounds.X+bounds.Width/2, bounds.Y+bounds.Height/2
			must(app.QueuePointer(cui.PointerDown, x, y))
			must(app.QueuePointer(cui.PointerUp, x, y))
			must(app.Step(.016))
			events, err := app.Dispatch()
			must(err)
			if events != 1 {
				panic("expected one event from the clicked button")
			}
		}
		must(app.Step(.016))
		expected = 191.5
	}
	value, err := app.Value(list)
	must(err)
	bounds, err := app.Bounds(list)
	must(err)
	row, err := app.VirtualListItemBounds(list, items[3])
	must(err)
	if value != expected || row.Y-bounds.Y != -9.5 {
		panic(fmt.Sprintf("inbox offset %.1f, expected %.1f", value, expected))
	}
	fmt.Println("Inbox message 4 is still first, 9.5 pixels into the row")
}

func must(err error) {
	if err != nil {
		panic(err)
	}
}
