package main

import cui "github.com/mattmezza/cui/bindings/go/cui"

func main() {
	a, e := cui.New(cui.Options{Width: 800, Height: 400})
	if e != nil {
		panic(e)
	}
	defer a.Close()
	first := cui.Item("first", "First").RGBA([]byte{255, 255, 255, 255}, 1, 1, 4)
	steps := cui.Stepper("steps").StepperVisual(true, false, cui.StepperSM).DesignSize(cui.LG).Children(first, cui.Item("second", "Second"))
	if e = a.Mount(cui.Column("root").Children(steps)); e != nil {
		panic(e)
	}
	if e = a.Step(.016); e != nil {
		panic(e)
	}
	r, e := a.NumericPartBounds(steps, 2)
	if e != nil || r.Width <= 0 {
		panic("missing step geometry")
	}
}
