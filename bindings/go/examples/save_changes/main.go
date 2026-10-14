package main

import (
	"fmt"
	"github.com/mattmezza/cui/bindings/go/cui"
	"log"
	"os"
)

func main() {
	font := os.Getenv("CUI_TEST_FONT")
	if font == "" {
		log.Fatal("set CUI_TEST_FONT to a TTF file")
	}
	app, err := cui.New(cui.Options{Native: true, Width: 520, Height: 180, Title: "CUI Go builders", FontPath: font})
	if err != nil {
		log.Fatal(err)
	}
	defer app.Close()
	status := cui.Label("Ready").Key("status")
	save := cui.Button("save").Text("Save changes").Variant(cui.Primary).Size(cui.MD).RoundedMD()
	save.OnClick(func(cui.Event) {
		if err := status.SetText("Saved ✓"); err != nil {
			log.Print(err)
		}
		fmt.Println("Saved")
	})
	if err := app.Mount(cui.Column("root").GapUnits(2).Children(status, save)); err != nil {
		log.Fatal(err)
	}
	if err := app.Run(1.0 / 60); err != nil {
		log.Fatal(err)
	}
}
