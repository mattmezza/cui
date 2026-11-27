// Run with `go run ./examples/account_menu`; this deterministic example opens no window.
package main

import (
	"fmt"
	"github.com/mattmezza/cui/bindings/go/cui"
)

func must(e error) {
	if e != nil {
		panic(e)
	}
}
func main() {
	a, e := cui.New(cui.Options{Width: 480, Height: 320})
	must(e)
	defer a.Close()
	trigger := cui.Button("account").Text("Account")
	close := cui.Button("done").Text("Done")
	menu := cui.Popover("menu", "Account", "").Anchor(trigger).Children(close)
	trigger.OnClick(func(cui.Event) { menu.Open(true) })
	close.OnClick(func(cui.Event) { menu.Open(false) })
	supported, e := a.SupportsPopoverOptions()
	must(e)
	if supported {
		menu.PopoverOptions(&cui.PopoverOptions{Anchor: cui.PopoverTopRight, Width: 200, Height: 140, FocusContent: true, HideClose: true})
	}
	must(a.Mount(cui.Column("root").Children(trigger, menu)))
	click := func(n *cui.Node) {
		b, e := a.Bounds(n)
		must(e)
		must(a.QueuePointer(cui.PointerDown, b.X+b.Width/2, b.Y+b.Height/2))
		must(a.QueuePointer(cui.PointerUp, b.X+b.Width/2, b.Y+b.Height/2))
		must(a.Step(.016))
		_, e = a.Dispatch()
		must(e)
	}
	must(a.Step(.016))
	click(trigger)
	must(a.Step(.016))
	must(a.Step(.016))
	click(close)
	click(trigger)
	must(a.Step(.016))
	must(a.QueueKey(cui.KeyDown, cui.Escape, 0, false))
	must(a.Step(.016))
	opened, e := a.Open(menu)
	must(e)
	if opened {
		panic("Escape did not close account menu")
	}
	fmt.Println("Account menu: Button and Escape dismissal complete")
}
