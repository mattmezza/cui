package main

import (
	"flag"
	"fmt"

	"github.com/mattmezza/cui/bindings/go/cui"
)

func main() {
	font := flag.String("font", "", "TrueType font for an SDL window")
	flag.Parse()

	app, err := cui.New(cui.Options{
		Title: "Document results", Width: 440, Height: 320,
		Native: *font != "", FontPath: *font,
	})
	must(err)
	defer app.Close()
	const pageCount = 24

	summary := cui.Label("")
	results := []*cui.Node{cui.Label(""), cui.Label(""), cui.Label("")}
	renderResults := func(page int) {
		must(summary.SetText(fmt.Sprintf("Page %d of %d", page, pageCount)))
		for index, row := range results {
			must(row.SetText(fmt.Sprintf("Document %02d", (page-1)*len(results)+index+1)))
		}
	}
	renderResults(1)
	pager := cui.ConfiguredPagination("result-pages", cui.PaginationConfig{
		Chevrons: true, JumpMenu: true, VisiblePages: 5, AvailableWidth: 320,
		Group: "Results pages",
	}).Total(pageCount).DesignSize(cui.Base).OnChange(func(event cui.Event) {
		renderResults(int(event.Value))
	})
	must(pager.SetPaginationCustomSize(52.5))
	content := append([]*cui.Node{cui.Label("Documents"), summary}, results...)
	must(app.Mount(cui.Column("results").GapUnits(2).Children(append(content, pager)...)))
	if *font != "" {
		must(app.Run(.016))
		return
	}

	must(app.Step(.016))
	next, err := app.NumericPartBounds(pager, 3)
	must(err)
	if next.Height != 52.5 {
		panic("unexpected custom height")
	}
	x, y := next.X+next.Width/2, next.Y+next.Height/2
	must(app.QueuePointer(cui.PointerDown, x, y))
	must(app.QueuePointer(cui.PointerUp, x, y))
	must(app.Step(.016))
	count, err := app.Dispatch()
	must(err)
	page, err := app.Text(summary)
	must(err)
	first, err := app.Text(results[0])
	must(err)
	if count != 1 || page != "Page 2 of 24" || first != "Document 04" {
		panic(fmt.Sprintf("unexpected results: %d events, %s, %s", count, page, first))
	}
	fmt.Println(page, first)
}

func must(err error) {
	if err != nil {
		panic(err)
	}
}
