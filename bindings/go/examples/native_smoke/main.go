package main

import (
	"cui.local/bindings/go/cui"
	"log"
	"os"
)

func main() {
	font := os.Getenv("CUI_TEST_FONT")
	a, e := cui.New(cui.Options{Native: true, Width: 320, Height: 160, Title: "Go smoke", FontPath: font, FontFallbacks: []string{font}})
	if e != nil {
		log.Fatal(e)
	}
	defer a.Close()
	b := cui.Button("save").Text("Save")
	hit := false
	b.OnClick(func(cui.Event) { hit = true })
	if e = a.Mount(cui.Column("root").Children(b)); e != nil {
		log.Fatal(e)
	}
	if e = a.Step(.016); e != nil {
		log.Fatal(e)
	}
	r, e := a.Bounds(b)
	if e != nil {
		log.Fatal(e)
	}
	x, y := r.X+r.Width/2, r.Y+r.Height/2
	if e = a.QueuePointer(cui.PointerDown, x, y); e != nil {
		log.Fatal(e)
	}
	if e = a.QueuePointer(cui.PointerUp, x, y); e != nil {
		log.Fatal(e)
	}
	if e = a.Step(.016); e != nil {
		log.Fatal(e)
	}
	if _, e = a.Dispatch(); e != nil {
		log.Fatal(e)
	}
	if !hit {
		log.Fatal("native button did not click")
	}
}
