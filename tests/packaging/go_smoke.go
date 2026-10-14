package main

import (
	"fmt"
	"github.com/mattmezza/cui/bindings/go/cui"
	"os"
)

func main() {
	native := os.Getenv("CUI_PACKAGE_SMOKE_BACKEND") == "sdl"
	app, err := cui.New(cui.Options{Native: native, FontPath: os.Getenv("CUI_PACKAGE_SMOKE_FONT"), Title: "CUI Installed Go Smoke"})
	if err != nil {
		panic(err)
	}
	defer app.Close()
	clicks := 0
	button := cui.Button("save").Text("Save ✓").OnClick(func(cui.Event) { clicks++ })
	if err := app.Mount(cui.Row("root").Children(button)); err != nil {
		panic(err)
	}
	if err := app.Step(.016); err != nil {
		panic(err)
	}
	bounds, err := app.Bounds(button)
	if err != nil || bounds.Width <= 0 || bounds.Height <= 0 {
		panic("missing Button geometry")
	}
	if err := app.QueuePointer(cui.PointerDown, bounds.X+bounds.Width/2, bounds.Y+bounds.Height/2); err != nil {
		panic(err)
	}
	if err := app.QueuePointer(cui.PointerUp, bounds.X+bounds.Width/2, bounds.Y+bounds.Height/2); err != nil {
		panic(err)
	}
	if err := app.Step(.016); err != nil {
		panic(err)
	}
	delivered, err := app.Dispatch()
	if err != nil || delivered != 1 || clicks != 1 {
		panic("installed callback failed")
	}
	fmt.Println("installed Go bridge smoke passed")
}
