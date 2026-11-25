package main

import (
	"flag"
	"fmt"
	"os"

	"github.com/mattmezza/cui/bindings/go/cui"
)

func main() {
	font := flag.String("font", "", "font path for a native SDL window")
	smoke := flag.Bool("smoke", false, "step a hidden window on the private test display")
	flag.Parse()
	if *smoke && (*font == "" || os.Getenv("CUI_TEST_VIRTUAL_DISPLAY") != "1") {
		panic("--smoke requires a font and the private display from scripts/test_local.py")
	}
	app, err := cui.New(cui.Options{
		Width: 480, Height: 320, Title: "Live inbox",
		Native: *font != "", FontPath: *font, InitiallyHidden: *smoke,
	})
	must(err)
	defer app.Close()

	headers := []string{"Subject", "Sender"}
	keys := make([]uint32, 12)
	rows := make([][]string, len(keys))
	for i := range keys {
		keys[i] = uint32(100 + i)
		rows[i] = []string{fmt.Sprintf("Project update %d", keys[i]), "Workspace"}
	}
	inbox := cui.DataTable("inbox").TableDataKeyed(headers, rows, keys).VisualSize(360, 184)
	status := cui.Label("12 messages")
	load := cui.Button("load").Text("Load latest")
	load.OnClick(func(cui.Event) {
		page := [][]string{{"Release notes", "Build team"}, {"Design review", "Morgan"}}
		must(inbox.SetTableKeyed(headers, append(page, rows...), append([]uint32{203, 204}, keys...)))
		must(load.Disabled(true).Err())
		must(status.SetText("14 messages · Updated just now"))
	})

	content := cui.Column("content").GapUnits(2).Children(cui.Label("Inbox"), load, inbox, status)
	must(app.Mount(cui.RootView("root").RootPadding(16).Children(content)))
	if *font != "" && !*smoke {
		must(app.Run(.016))
		return
	}

	must(app.Step(.016))
	initial, err := app.DataTableCellBounds(inbox, 1, 0)
	must(err)
	must(app.QueueScroll(initial.X+10, initial.Y+10, 0, -1))
	must(app.Step(.016))
	before, err := app.DataTableCellBounds(inbox, 1, 0)
	must(err)
	if before.Y >= initial.Y {
		panic("inbox did not scroll")
	}

	trigger, err := app.Bounds(load)
	must(err)
	x, y := trigger.X+trigger.Width/2, trigger.Y+trigger.Height/2
	must(app.QueuePointer(cui.PointerDown, x, y))
	must(app.QueuePointer(cui.PointerUp, x, y))
	must(app.Step(.016))
	_, err = app.Dispatch()
	must(err)
	must(app.Step(.016))
	title, err := app.DataTableCell(inbox, 0, 0)
	must(err)
	after, err := app.DataTableCellBounds(inbox, 3, 0)
	must(err)
	if title != "Release notes" || after.Y != before.Y {
		panic("page arrival moved the visible row")
	}
	fmt.Println("Live inbox updated and kept the visible row in place.")
}

func must(err error) {
	if err != nil {
		panic(err)
	}
}
