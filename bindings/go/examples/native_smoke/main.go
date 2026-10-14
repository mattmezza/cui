package main

import (
	"github.com/mattmezza/cui/bindings/go/cui"
	"log"
	"os"
	"strings"
)

func main() {
	font := os.Getenv("CUI_TEST_FONT")
	a, e := cui.New(cui.Options{Native: true, Width: 320, Height: 160, Title: "Go smoke", FontPath: font, FontFallbacks: []string{font}})
	if e != nil {
		log.Fatal(e)
	}
	defer a.Close()
	if e = a.SetClipboard("Go clipboard ✓"); e != nil {
		log.Fatal(e)
	}
	clip, e := a.Clipboard()
	if e != nil {
		log.Fatal(e)
	}
	if clip != "Go clipboard ✓" {
		log.Fatalf("clipboard mismatch: %q", clip)
	}
	for _, want := range []string{"", strings.Repeat("é", 32768)} {
		if e = a.SetClipboard(want); e != nil {
			log.Fatal(e)
		}
		clip, e = a.Clipboard()
		if e != nil {
			log.Fatal(e)
		}
		if clip != want {
			log.Fatalf("clipboard round trip mismatch: got %d bytes, want %d", len(clip), len(want))
		}
	}
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
