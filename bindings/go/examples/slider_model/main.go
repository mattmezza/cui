package main

import (
	"flag"
	"fmt"
	cui "github.com/mattmezza/cui/bindings/go/cui"
)

func must(err error) {
	if err != nil {
		panic(err)
	}
}
func main() {
	reject := flag.Bool("reject", false, "leave proposals unaccepted")
	single := flag.Bool("single", false, "author one thumb")
	vertical := flag.Bool("vertical", false, "vertical orientation")
	flag.Parse()
	owner, err := cui.NewSliderRange(20, 60)
	must(err)
	if *single {
		owner, err = cui.NewSliderSingle(60)
		must(err)
	}
	a, err := cui.New(cui.Options{Width: 480, Height: 400})
	must(err)
	defer a.Close()
	accepting := !*reject
	status := cui.Label("Ready")
	config := cui.DefaultSliderConfig()
	config.SourceGeneration = 0xfedcba9876543210
	if *vertical {
		config.Orientation = cui.SliderVertical
	}
	control := cui.SliderModel("selection", config, owner).OnSliderRequest(func(request cui.SliderRequest) error {
		if request.Phase == cui.SliderUpdate && accepting {
			if e := a.AcceptSliderRequest(request.Receipt, request.Requested); e != nil {
				return e
			}
			owner = request.Requested
		}
		return status.SetText(fmt.Sprintf("phase=%d requested=[%g,%g] owner=[%g,%g]", request.Phase, request.Requested.Start(), request.Requested.End(), owner.Start(), owner.End()))
	})
	toggle := cui.Button("policy").Text("Toggle acceptance").OnClick(func(cui.Event) { accepting = !accepting })
	must(a.Mount(cui.Column("root").Children(cui.Label("Controlled Go selection"), cui.Row("row").Children(cui.Column("nested").Children(control)), toggle, status)))
	must(a.Step(.016))
	bounds, ok, err := control.SliderPartBounds(cui.SliderEndThumb)
	must(err)
	if !ok {
		panic("missing actual thumb")
	}
	must(a.QueuePointer(cui.PointerDown, bounds.X+bounds.Width/2, bounds.Y+bounds.Height/2))
	must(a.QueuePointer(cui.PointerUp, bounds.X+bounds.Width/2, bounds.Y+bounds.Height/2))
	must(a.Step(.016))
	_, err = a.Dispatch()
	must(err)
	before := owner
	key := cui.Right
	if *vertical {
		key = cui.Up
	}
	must(a.QueueKey(cui.KeyDown, key, 0, false))
	must(a.Step(.016))
	_, err = a.Dispatch()
	must(err)
	if accepting && owner == before {
		panic("owner did not accept")
	}
	if !accepting && owner != before {
		panic("rejection changed owner")
	}
	fmt.Printf("PASS Go-authored controlled UI: [%g,%g]\n", owner.Start(), owner.End())
}
