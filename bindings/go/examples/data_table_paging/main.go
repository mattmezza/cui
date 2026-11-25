// The inbox requests and appends a page of local messages as you scroll.
package main

import (
	"flag"
	"fmt"
	"os"
	"time"

	"github.com/mattmezza/cui/bindings/go/cui"
)

const initialRows = 8

type pageTicket struct {
	generation uint64
	frame      uint64
}

type inbox struct {
	table      *cui.Node
	status     *cui.Node
	headers    []string
	rows       [][]string
	keys       []uint32
	capacity   uint32
	generation uint64
	pending    pageTicket
}

func newInbox(rowCount int, capacity uint32) *inbox {
	status := cui.Label("8 messages")
	owner := &inbox{
		headers: []string{"Subject", "Sender"}, rows: make([][]string, rowCount),
		keys: make([]uint32, rowCount), status: status, capacity: capacity, generation: 1,
	}
	for i := range owner.keys {
		owner.keys[i] = uint32(101 + i)
		owner.rows[i] = []string{fmt.Sprintf("Project update %d", i+1), "Workspace"}
	}
	owner.table = cui.DataTable("inbox").VisualSize(360, 160).LoadMore(0)
	if capacity != 0 {
		owner.table.TableDataKeyedCapacity(owner.headers, owner.rows[:initialRows], owner.keys[:initialRows], capacity)
	} else {
		owner.table.TableDataKeyed(owner.headers, owner.rows[:initialRows], owner.keys[:initialRows])
	}
	return owner
}

// Run these methods on the UI thread. A transport owns only the copied ticket.
func (i *inbox) beginPage() (pageTicket, bool, error) {
	if i.pending != (pageTicket{}) {
		return pageTicket{}, false, nil
	}
	request, ok, err := i.table.PollLoadMore()
	if err != nil || !ok {
		return pageTicket{}, false, err
	}
	if err := i.table.ConfigureLoadMore(false, 0); err != nil {
		return pageTicket{}, false, err
	}
	i.pending = pageTicket{generation: i.generation, frame: request.FrameSequence}
	return i.pending, true, i.status.SetText("Loading the next page…")
}

func (i *inbox) retry() error {
	// Invalidate the logical request so any late transport result is ignored.
	i.generation++
	i.pending = pageTicket{}
	return i.table.ConfigureLoadMore(true, 0)
}

func (i *inbox) completePage(ticket pageTicket) (bool, error) {
	if ticket == (pageTicket{}) || i.pending != ticket {
		return false, nil
	}
	// Apply the final local page in one replacement, keeping the old row keys.
	var err error
	if i.capacity != 0 {
		err = i.table.SetTableKeyedCapacity(i.headers, i.rows, i.keys, i.capacity)
	} else {
		err = i.table.SetTableKeyed(i.headers, i.rows, i.keys)
	}
	if err != nil {
		return false, err
	}
	i.pending = pageTicket{}
	return true, i.status.SetText(fmt.Sprintf("%d messages · End of inbox", len(i.rows)))
}

func (i *inbox) verify(app *cui.App, smoke bool) {
	var selectedKeys []uint32
	i.table.OnChange(func(e cui.Event) { selectedKeys = append(selectedKeys, uint32(e.Value)) })
	bounds := func(row uint32) cui.Rect {
		rect, err := app.DataTableCellBounds(i.table, row, 0)
		must(err)
		return rect
	}
	click := func(rect cui.Rect) {
		x, y := rect.X+rect.Width/2, rect.Y+rect.Height/2
		must(app.QueuePointer(cui.PointerDown, x, y))
		must(app.QueuePointer(cui.PointerUp, x, y))
	}
	must(app.Step(.016))
	_, ok, err := i.beginPage()
	must(err)
	require(!ok, "initial viewport requested a page")
	first := bounds(1)
	must(app.QueueScroll(first.X+10, first.Y+10, 0, -20))
	must(app.Step(.016))
	cancelled, ok, err := i.beginPage()
	must(err)
	require(ok, "missing near-end demand")
	_, ok, err = i.beginPage()
	must(err)
	require(!ok, "duplicate load while a request was pending")
	selected := bounds(6)
	click(selected)
	must(app.Step(.016))
	_, err = app.Dispatch()
	must(err)
	require(len(selectedKeys) == 1 && selectedKeys[0] == i.keys[6],
		"rows stopped receiving selection while loading")
	must(i.retry())
	must(app.Step(.016))
	next, ok, err := i.beginPage()
	must(err)
	require(ok && next != cancelled, "retry did not request a new page")
	accepted, err := i.completePage(cancelled)
	must(err)
	require(!accepted, "cancelled response was applied")
	accepted, err = i.completePage(next)
	must(err)
	require(accepted, "current response was dropped")
	must(app.Step(.016))
	s, err := app.DataTableState(i.table)
	must(err)
	require(s.Row == i.keys[6] && bounds(6) == selected, "append moved the selection or viewport")
	last := len(i.rows) - 1
	cell, err := app.DataTableCell(i.table, uint32(last), 0)
	must(err)
	require(cell == i.rows[last][0], "missing last row")
	must(app.QueueScroll(first.X+10, first.Y+10, 0, -float32(len(i.rows))))
	must(app.Step(.016))
	_, ok, err = i.beginPage()
	must(err)
	require(!ok, "demand after the final page")

	if smoke {
		var ran bool
		click(bounds(uint32(last - 1)))
		must(app.RunWith(.016, func(active *cui.App) error {
			ran = true
			if active != app || len(selectedKeys) != 2 || selectedKeys[1] != i.keys[last-1] {
				return fmt.Errorf("hook ran before selection dispatch")
			}
			return active.RequestClose()
		}))
		require(ran, "native frame hook was skipped")
	}
	fmt.Println("Appended the final page, kept the selected row, and discarded a late response.")
}

func (i *inbox) run(app *cui.App) error {
	var timer *time.Timer
	var requested pageTicket
	defer func() {
		if timer != nil {
			timer.Stop()
		}
	}()
	return app.RunWith(.016, func(*cui.App) error {
		if timer == nil {
			ticket, ok, err := i.beginPage()
			if ok {
				requested = ticket
				timer = time.NewTimer(500 * time.Millisecond)
			}
			return err
		}
		select {
		case <-timer.C:
			timer = nil
			_, err := i.completePage(requested)
			return err
		default:
			return nil
		}
	})
}

func main() {
	font := flag.String("font", "", "font path for a native SDL window")
	smoke := flag.Bool("smoke", false, "check a hidden window on the private test display")
	larger := flag.Bool("larger", false, "page past 32 rows using the optional capacity API")
	flag.Parse()
	require(!*smoke || (*font != "" && os.Getenv("CUI_TEST_VIRTUAL_DISPLAY") == "1"),
		"--smoke requires a font and the private display from scripts/test_local.py")
	app, err := cui.New(cui.Options{Width: 480, Height: 320, Title: "Paged inbox",
		Native: *font != "", FontPath: *font, InitiallyHidden: *smoke})
	must(err)
	defer app.Close()
	paging, err := app.SupportsDataTablePaging()
	must(err)
	require(paging, "the inbox requires a bridge with paging")
	rows := 12
	var capacity uint32
	if *larger {
		rows = 40
		maximum, err := app.DataTableMaxRowCapacity()
		must(err)
		require(maximum >= uint32(rows), "--larger requires an opt-in capacity of at least 40 rows")
		capacity = min(128, maximum)
	} else {
		keyed, err := app.SupportsKeyedDataTable()
		must(err)
		require(keyed, "the inbox requires a bridge with keyed replacement")
	}

	owner := newInbox(rows, capacity)
	content := cui.Column("content").GapUnits(2).Children(cui.Label("Inbox"), owner.table, owner.status)
	must(app.Mount(cui.RootView("root").RootPadding(16).Children(content)))
	if *font == "" || *smoke {
		owner.verify(app, *smoke)
	} else {
		must(owner.run(app))
	}
}

func must(err error) {
	if err != nil {
		panic(err)
	}
}

func require(ok bool, message string) {
	if !ok {
		panic(message)
	}
}
