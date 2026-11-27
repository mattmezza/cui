// A headless retained Button with optional focus treatment and a live reset.
package main

import "github.com/mattmezza/cui/bindings/go/cui"

func main() {
	app, err := cui.New(cui.Options{Width: 320, Height: 200})
	if err != nil {
		panic(err)
	}
	defer app.Close()
	button := cui.Button("save").Text("Save")
	if supported, err := app.SupportsButtonFocus(); err != nil {
		panic(err)
	} else if supported {
		button.ButtonFocus(cui.ButtonFocusOuterRing)
	}
	if err := app.Mount(cui.Column("root").Children(button)); err != nil {
		panic(err)
	}
	if err := app.Step(1.0 / 60); err != nil {
		panic(err)
	}
	if err := button.ButtonFocus(cui.ButtonFocusLegacy).Err(); err != nil {
		panic(err)
	}
}
