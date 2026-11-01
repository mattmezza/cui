// Authored English/Arabic controls. Default is windowless; --native is manual UI.
package main

import (
	"flag"
	"fmt"
	cui "github.com/mattmezza/cui/bindings/go/cui"
	"os"
)

func main() {
	native := flag.Bool("native", false, "manual native window")
	flag.Parse()
	a, e := cui.New(cui.Options{Native: *native, Width: 800, Height: 400, FontPath: os.Getenv("CUI_TEST_FONT")})
	must(e)
	defer a.Close()
	must(a.EnableLocalizedTimeFields(16))
	min, max := cui.Time{Hour: 8}, cui.Time{Hour: 18}
	english, arabic := cui.EnglishTimeField(), cui.ArabicTimeField()
	english.Minimum, english.Maximum = &min, &max
	arabic.Minimum, arabic.Maximum = &min, &max
	english.EditGeneration = 1
	arabic.EditGeneration = 1
	summary := cui.Label("No applied edits").Key("summary")
	en := cui.TimeField("english").Time(cui.Time{Hour: 9, Minute: 30}).LocalizedTime(english)
	ar := cui.TimeField("arabic").Time(cui.Time{Hour: 14, Minute: 15}).LocalizedTime(arabic)
	update := func(c cui.TimeFieldChange) {
		must(summary.SetText(fmt.Sprintf("Applied %02d:%02d → %02d:%02d; frame status %d", c.Previous.Hour, c.Previous.Minute, c.Value.Hour, c.Value.Minute, c.FrameStatus)))
	}
	en.OnTimeFieldChange(update)
	ar.OnTimeFieldChange(update)
	clear := cui.Button("clear").Text("Clear validation").OnClick(func(cui.Event) { must(a.ClearTimeValidation(en)); must(a.ClearTimeValidation(ar)) })
	readonly := cui.Button("readonly").Text("Toggle read-only").OnClick(func(cui.Event) {
		next := english
		next.ReadOnly = !next.ReadOnly
		must(en.SetLocalizedTime(next))
		english = next
	})
	disabled := cui.Button("disabled").Text("Toggle disabled").OnClick(func(cui.Event) {
		next := english
		next.Disabled = !next.Disabled
		must(en.SetLocalizedTime(next))
		english = next
	})
	bounds := cui.Button("bounds").Text("Toggle bounds").OnClick(func(cui.Event) {
		next := english
		if next.Minimum == nil {
			next.Minimum, next.Maximum = &min, &max
		} else {
			next.Minimum, next.Maximum = nil, nil
		}
		next.EditGeneration++
		must(en.SetLocalizedTime(next))
		english = next
	})
	must(a.Mount(cui.Column("root").Children(cui.Label("English 24h / العربية 12h"), cui.Row("fields").Children(en, ar), summary, cui.Row("controls").Children(clear, readonly, disabled, bounds))))
	if *native {
		must(a.Run(.016))
		return
	}
	must(a.Step(.016))
	for _, item := range []struct {
		n    *cui.Node
		text string
	}{{en, "10:45"}, {ar, "٠٣:٢٠ م"}} {
		r, e := a.Bounds(item.n)
		must(e)
		must(a.QueuePointer(cui.PointerDown, r.X+r.Width/2, r.Y+r.Height/2))
		must(a.QueuePointer(cui.PointerUp, r.X+r.Width/2, r.Y+r.Height/2))
		must(a.QueueText(item.text))
		must(a.Step(.016))
		_, e = a.Dispatch()
		must(e)
	}
	value, e := a.Time(en)
	must(e)
	fmt.Printf("English applied: %+v\n", value)
}
func must(e error) {
	if e != nil {
		panic(e)
	}
}
