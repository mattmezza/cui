package cui

import (
	"runtime"
	"strings"
	"testing"
)

func app(t *testing.T) *App {
	t.Helper()
	a, e := New(Options{})
	if e != nil {
		t.Fatal(e)
	}
	t.Cleanup(func() {
		if !a.closed {
			if e := a.Close(); e != nil {
				t.Error(e)
			}
		}
	})
	return a
}
func check(t *testing.T, e error) {
	t.Helper()
	if e != nil {
		t.Fatal(e)
	}
}
func TestClickMutationAndDisabled(t *testing.T) {
	a := app(t)
	label := Label("Ready").Key("status")
	button := Button("save").Text("Save ✓").Size(Size9XL).Variant(Primary).RoundedMD().HoverRGBA(90, 80, 70, 255)
	clicks := 0
	button.OnClick(func(Event) { clicks++; check(t, label.SetText("Saved ✓")) })
	check(t, a.Mount(Column("root").GapUnits(2).Children(Row("row").Children(label, button))))
	runtime.GC()
	check(t, a.Step(0.016))
	b, e := a.Bounds(button)
	check(t, e)
	if b.Width <= 0 || b.Height <= 0 {
		t.Fatalf("empty bounds: %+v", b)
	}
	x, y := b.X+b.Width/2, b.Y+b.Height/2
	check(t, a.QueuePointer(PointerDown, x, y))
	check(t, a.QueuePointer(PointerUp, x, y))
	check(t, a.Step(0.016))
	n, e := a.Dispatch()
	check(t, e)
	if n != 1 || clicks != 1 || label.text != "Saved ✓" {
		t.Fatalf("dispatch %d clicks %d text %q", n, clicks, label.text)
	}
	button.Disabled(true)
	check(t, button.Err())
	check(t, a.Step(0.016))
	check(t, a.QueuePointer(PointerDown, x, y))
	check(t, a.QueuePointer(PointerUp, x, y))
	check(t, a.Step(0.016))
	n, e = a.Dispatch()
	check(t, e)
	if n != 0 {
		t.Fatalf("disabled dispatched %d", n)
	}
}
func TestValidationAndLifetimes(t *testing.T) {
	a := app(t)
	b := app(t)
	node := Button("save")
	check(t, a.Mount(Row("root").Children(node)))
	if _, e := b.Bounds(node); e == nil {
		t.Fatal("cross-app handle accepted")
	}
	if e := node.SetText("a\x00b"); e == nil {
		t.Fatal("NUL accepted")
	}
	if e := node.SetText(strings.Repeat("x", 4097)); e == nil {
		t.Fatal("oversize accepted")
	}
	if e := node.SetText("\xff"); e == nil {
		t.Fatal("invalid UTF-8 accepted")
	}
	check(t, node.Remove())
	if e := node.SetText("stale"); e == nil {
		t.Fatal("removed node accepted")
	}
	check(t, a.Close())
	if e := a.Step(0.01); e == nil {
		t.Fatal("disposed app accepted")
	}
}
func TestKeyboardAndCallbackPanic(t *testing.T) {
	a := app(t)
	button := Button("go").Text("Go")
	button.OnClick(func(Event) { panic("callback panic") })
	check(t, a.Mount(Column("root").Children(button)))
	check(t, a.Step(0.016))
	check(t, a.QueueKey(KeyDown, Tab, 0, false))
	check(t, a.Step(0.016))
	check(t, a.QueueKey(KeyDown, Enter, 0, false))
	check(t, a.Step(0.016))
	defer func() {
		if recover() == nil {
			t.Error("callback panic lost")
		}
	}()
	_, _ = a.Dispatch()
}
func TestNewComponents(t *testing.T) {
	a := app(t)
	checks := []*Node{Checkbox("c"), Radio("r"), Switch("s"), Toggle("t")}
	slider := Slider("slider").Range(0, 100, 5).Value(25)
	progress := Progress("progress").Value(0.6)
	badge := Badge("Done ✓").Key("badge")
	root := Column("root2").Children(checks[0], checks[1], checks[2], checks[3], slider, progress, badge)
	check(t, a.Mount(root))
	check(t, a.Step(0.016))
	for _, n := range append(checks, slider, progress, badge) {
		r, e := a.Bounds(n)
		check(t, e)
		if r.Width <= 0 || r.Height <= 0 {
			t.Fatalf("empty %d bounds %+v", n.kind, r)
		}
	}
	for _, n := range checks {
		n.Checked(true)
		check(t, n.Err())
		v, e := a.Checked(n)
		check(t, e)
		if !v {
			t.Fatal("checked state lost")
		}
	}
	v, e := a.Value(slider)
	check(t, e)
	if v != 25 {
		t.Fatalf("slider %v", v)
	}
	v, e = a.Value(progress)
	check(t, e)
	if v != 0.6 {
		t.Fatalf("progress %v", v)
	}
	if Slider("bad").Range(1, 1, 1).Err() == nil {
		t.Fatal("bad range accepted")
	}
}
func press(t *testing.T, a *App, n *Node) {
	t.Helper()
	r, e := a.Bounds(n)
	check(t, e)
	x, y := r.X+r.Width/2, r.Y+r.Height/2
	check(t, a.QueuePointer(PointerDown, x, y))
	check(t, a.QueuePointer(PointerUp, x, y))
	check(t, a.Step(.016))
}
func TestChangeSnapshotsAndInput(t *testing.T) {
	a := app(t)
	box := Checkbox("check").Text("Check")
	input := Input("input").Text("Hi")
	number := NumberInput("number").Text("Count").Range(0, 20, 2).Value(10)
	area := Textarea("area").Text("One\nTwo")
	var states []bool
	var texts []string
	box.OnChange(func(e Event) { states = append(states, e.Checked) })
	input.OnChange(func(e Event) { texts = append(texts, e.Text) })
	check(t, a.Mount(Column("root").Children(box, input, area, number)))
	check(t, a.Step(.016))
	press(t, a, box)
	press(t, a, box)
	n, e := a.Dispatch()
	check(t, e)
	if n != 2 || len(states) != 2 || !states[0] || states[1] {
		t.Fatalf("change snapshots %d %v", n, states)
	}
	press(t, a, input)
	check(t, a.QueueText("✓"))
	check(t, a.Step(.016))
	check(t, a.QueueText("Z"))
	check(t, a.Step(.016))
	n, e = a.Dispatch()
	check(t, e)
	if n != 2 || len(texts) != 2 || texts[0] != "Hi✓" || texts[1] != "Hi✓Z" {
		t.Fatalf("text snapshots %d %q", n, texts)
	}
	v, e := a.Text(input)
	check(t, e)
	if v != "Hi✓Z" {
		t.Fatal(v)
	}
	v, e = a.Text(area)
	check(t, e)
	if v != "One\nTwo" {
		t.Fatal(v)
	}
	number.Value(12)
	check(t, number.Err())
	num, e := a.Value(number)
	check(t, e)
	if num != 12 {
		t.Fatal(num)
	}
}
func TestCallbackCloseAndThread(t *testing.T) {
	a := app(t)
	button := Button("close").Text("Close")
	button.OnClick(func(Event) { check(t, a.Close()) })
	check(t, a.Mount(Row("root").Children(button)))
	check(t, a.Step(.016))
	other := make(chan error, 1)
	go func() { other <- a.Step(.016) }()
	if e := <-other; e == nil {
		t.Fatal("other thread accepted")
	}
	press(t, a, button)
	n, e := a.Dispatch()
	check(t, e)
	if n != 1 || !a.closed {
		t.Fatalf("close callback: %d %v", n, a.closed)
	}
}
func TestDisclosureAndGroups(t *testing.T) {
	a := app(t)
	panel := Label("Body").Key("body")
	fold := Collapsible("fold").Text("Details").Children(panel)
	alert := Alert("alert", "Heads up", "Message")
	left := Item("left", "Left")
	right := Item("right", "Right")
	group := RadioGroup("group").Children(left, right)
	tabA := Item("tab-a", "First")
	tabB := Item("tab-b", "Second").Children(Label("Panel").Key("panel"))
	tabs := Tabs("tabs").Children(tabA, tabB).Selected(tabA)
	accItem := Item("item", "Expand").Children(Label("Inside").Key("inside"))
	accordion := Accordion("accordion").Children(accItem)
	var selected []*Node
	group.OnChange(func(e Event) { selected = append(selected, e.Selected) })
	opened := false
	fold.OnChange(func(e Event) { opened = e.Checked })
	check(t, a.Mount(Column("root").Children(fold, alert, group, tabs, accordion)))
	check(t, a.Step(.016))
	if _, e := a.Bounds(panel); e == nil {
		t.Fatal("closed panel has bounds")
	}
	press(t, a, fold)
	n, e := a.Dispatch()
	check(t, e)
	if n != 1 || !opened {
		t.Fatalf("fold event %d %v", n, opened)
	}
	_, e = a.Bounds(panel)
	check(t, e)
	press(t, a, left)
	press(t, a, right)
	n, e = a.Dispatch()
	check(t, e)
	if n != 2 || len(selected) != 2 || selected[0] != left || selected[1] != right {
		t.Fatalf("selected snapshots %d %v", n, selected)
	}
	current, e := a.Selected(group)
	check(t, e)
	if current != right {
		t.Fatal("wrong current selection")
	}
	press(t, a, tabB)
	current, e = a.Selected(tabs)
	check(t, e)
	if current != tabB {
		t.Fatal("tab selection")
	}
	_, e = a.Bounds(tabB.children[0])
	check(t, e)
	press(t, a, accItem)
	v, e := a.Checked(accItem)
	check(t, e)
	if !v {
		t.Fatal("accordion item did not open")
	}
}
func TestBuilderErrorPersists(t *testing.T) {
	a := app(t)
	button := Button("button").Size(Size(-1)).Text("Valid later")
	if button.Err() == nil {
		t.Fatal("builder error disappeared")
	}
	if e := a.Mount(Row("root").Children(button)); e == nil {
		t.Fatal("invalid builder mounted")
	}
}
func TestSelectAndComboBox(t *testing.T) {
	a := app(t)
	apple := Item("apple", "Apple")
	banana := Item("banana", "Banana")
	selectNode := Select("fruit", "Select one", "Choose").Children(apple, banana)
	cherry := Item("cherry", "Cherry")
	combo := ComboBox("search", "Search").Children(cherry)
	var chosen *Node
	var queries []string
	selectNode.OnChange(func(e Event) { chosen = e.Selected })
	combo.OnChange(func(e Event) { queries = append(queries, e.Text) })
	check(t, a.Mount(Column("root").Children(selectNode, combo)))
	check(t, a.Step(.016))
	press(t, a, selectNode)
	_, e := a.Bounds(banana)
	check(t, e)
	press(t, a, banana)
	n, e := a.Dispatch()
	check(t, e)
	if n != 1 || chosen != banana {
		t.Fatalf("select event %d %v", n, chosen)
	}
	selected, e := a.Selected(selectNode)
	check(t, e)
	if selected != banana {
		t.Fatal("select state")
	}
	check(t, combo.SetText("Ch"))
	check(t, a.Step(.016))
	press(t, a, combo)
	check(t, a.QueueText("e"))
	check(t, a.Step(.016))
	n, e = a.Dispatch()
	check(t, e)
	if n != 1 || len(queries) != 1 || queries[0] != "Che" {
		t.Fatalf("combo %d %q", n, queries)
	}
	query, e := a.Text(combo)
	check(t, e)
	if query != "Che" {
		t.Fatal(query)
	}
}
func TestDropdownButton(t *testing.T) {
	a := app(t)
	entry := Item("export", "Export")
	button := DropdownButton("dropdown", "Run", "More").Children(entry)
	clicks := 0
	var selected *Node
	button.OnClick(func(Event) { clicks++ }).OnChange(func(e Event) { selected = e.Selected })
	check(t, a.Mount(Column("root").Children(button)))
	check(t, a.Step(.016))
	action, e := a.DropdownPartBounds(button, false)
	check(t, e)
	x, y := action.X+action.Width*.25, action.Y+action.Height*.5
	check(t, a.QueuePointer(PointerDown, x, y))
	check(t, a.QueuePointer(PointerUp, x, y))
	check(t, a.Step(.016))
	n, e := a.Dispatch()
	check(t, e)
	if n != 1 || clicks != 1 {
		t.Fatalf("dropdown action %d %d", n, clicks)
	}
	menu, e := a.DropdownPartBounds(button, true)
	check(t, e)
	x, y = menu.X+menu.Width*.5, menu.Y+menu.Height*.5
	check(t, a.QueuePointer(PointerDown, x, y))
	check(t, a.QueuePointer(PointerUp, x, y))
	check(t, a.Step(.016))
	_, e = a.Bounds(entry)
	check(t, e)
	press(t, a, entry)
	n, e = a.Dispatch()
	check(t, e)
	if n != 1 || selected != entry {
		t.Fatalf("dropdown menu %d %v", n, selected)
	}
}
