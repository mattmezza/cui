// Headless by default; --font /path/to/font.ttf opens a native window.
package main

import (
	"flag"
	cui "github.com/mattmezza/cui/bindings/go/cui"
)

func main() {
	font := flag.String("font", "", "native window font path")
	headless := flag.Bool("headless", false, "run one headless frame")
	flag.Parse()
	native := *font != "" && !*headless
	a, err := cui.New(cui.Options{Width: 640, Height: 360, Native: native, FontPath: *font, Title: "Scoped text defaults"})
	if err != nil {
		panic(err)
	}
	defer a.Close()
	tint := uint32(0x407adfff)
	row := cui.Row("progress").Children(cui.Icon("activity", "check"), cui.Label("Synchronizing your workspace"), cui.Spinner("busy"))
	nested := cui.Column("details").Children(row, cui.Label("Your changes stay available offline."))
	root := cui.Column("root").Children(cui.Label("Workspace status"), nested)
	supported, err := a.SupportsTextDefaults()
	if err != nil {
		panic(err)
	}
	if supported {
		root.TextDefaults(cui.TextDefaults{FontSize: 20, ColorRGBA: &tint})
		nested.TextDefaults(cui.TextDefaults{FontSize: 16})
	}
	if err := a.Mount(root); err != nil {
		panic(err)
	}
	if native {
		err = a.Run(1.0 / 60)
	} else {
		err = a.Step(1.0 / 60)
	}
	if err != nil {
		panic(err)
	}
}
