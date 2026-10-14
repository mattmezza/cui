package cui

import (
	"fmt"
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
func TestSurfaces(t *testing.T) {
	a := app(t)
	trigger := Button("trigger").Text("Open")
	body := Label("Body").Key("body")
	dialog := Dialog("dialog", "Dialog", "Description").Children(body)
	alert := AlertDialog("alert", "Confirm", "Proceed?")
	sheet := Sheet("sheet", "Sheet", "Info")
	popbody := Label("Popover body").Key("popbody")
	popover := Popover("popover", "Popover", "Info").Anchor(trigger).Children(popbody)
	var result int
	alert.OnClose(func(e Event) { result = e.CloseResult })
	check(t, a.Mount(Column("root").Children(trigger, dialog, alert, sheet, popover)))
	check(t, a.Step(.016))
	dialog.Open(true)
	check(t, dialog.Err())
	check(t, a.Step(.016))
	v, e := a.Open(dialog)
	check(t, e)
	if !v {
		t.Fatal("dialog closed")
	}
	_, e = a.Bounds(body)
	check(t, e)
	dialog.Open(false)
	check(t, dialog.Err())
	check(t, a.Step(.016))
	_, e = a.Dispatch()
	check(t, e)
	alert.Open(true)
	check(t, alert.Err())
	check(t, a.Step(.016))
	check(t, a.QueueKey(KeyDown, Escape, 0, false))
	check(t, a.Step(.016))
	_, e = a.Dispatch()
	check(t, e)
	if result != SurfaceCancelled {
		t.Fatalf("alert result %d", result)
	}
	sheet.Open(true)
	check(t, sheet.Err())
	check(t, a.Step(.016))
	sheet.Open(false)
	check(t, sheet.Err())
	check(t, a.Step(.016))
	_, e = a.Dispatch()
	check(t, e)
	popover.Open(true)
	check(t, popover.Err())
	check(t, a.Step(.016))
	_, e = a.Bounds(popbody)
	check(t, e)
	popover.Open(false)
	check(t, popover.Err())
	check(t, a.Step(.016))
	_, e = a.Dispatch()
	check(t, e)
}
func TestHoverSurfaces(t *testing.T) {
	a := app(t)
	trigger := Button("trigger").Text("Hover here")
	tip := Tooltip("tip", "Helpful tip").Anchor(trigger)
	body := Label("Extra context").Key("extra")
	card := HoverCard("card", "More details").Anchor(trigger).Children(body)
	opened := 0
	tip.OnChange(func(e Event) {
		if e.Checked {
			opened++
		}
	})
	card.OnChange(func(e Event) {
		if e.Checked {
			opened++
		}
	})
	check(t, a.Mount(Column("root").Children(trigger, tip, card)))
	check(t, a.Step(.016))
	r, e := a.Bounds(trigger)
	check(t, e)
	check(t, a.QueuePointer(PointerMove, r.X+r.Width/2, r.Y+r.Height/2))
	check(t, a.Step(.016))
	check(t, a.Step(.016))
	v, e := a.Open(tip)
	check(t, e)
	if !v {
		t.Fatal("tip did not open")
	}
	v, e = a.Open(card)
	check(t, e)
	if !v {
		t.Fatal("card did not open")
	}
	_, e = a.Bounds(body)
	check(t, e)
	n, e := a.Dispatch()
	check(t, e)
	if n != 2 || opened != 2 {
		t.Fatalf("hover events %d %d", n, opened)
	}
	tip.Disabled(true)
	check(t, tip.Err())
	check(t, a.QueuePointer(PointerMove, 590, 390))
	check(t, a.Step(.016))
	check(t, a.Step(.016))
	v, e = a.Open(tip)
	check(t, e)
	if v {
		t.Fatal("disabled tip stayed open")
	}
}
func TestVisualComponents(t *testing.T) {
	a := app(t)
	child := Label("Grouped").Key("child")
	nodes := []*Node{Avatar("avatar", "A").VisualSize(32, 32), Bubble("bubble", "Hi"), EmptyView("empty", "Nothing", "No records"), GroupBox("group", "Group").Children(child), Kbd("kbd", "Ctrl+S"), Skeleton("skeleton").VisualSize(100, 12), Spinner("spinner"), Tag("tag", "New"), Marker("marker", "Item"), Shimmer("shimmer", "Loading")}
	check(t, a.Mount(Column("root").Children(nodes...)))
	check(t, a.Step(.016))
	for _, n := range append(nodes, child) {
		r, e := a.Bounds(n)
		check(t, e)
		if r.Width <= 0 || r.Height <= 0 {
			t.Fatalf("visual kind %d empty %+v", n.kind, r)
		}
	}
	if Skeleton("bad").VisualSize(-1, 12).Err() == nil {
		t.Fatal("invalid visual size accepted")
	}
}
func TestLibrarySelection(t *testing.T) {
	_ = app(t)
	if _, e := New(Options{Library: "/tmp/other-cui-bridge.so"}); e == nil {
		t.Fatal("different library silently reused")
	}
	if _, e := New(Options{Library: "bad\x00path"}); e == nil {
		t.Fatal("NUL library path accepted")
	}
}
func clickRect(t *testing.T, a *App, r Rect) {
	t.Helper()
	x, y := r.X+r.Width/2, r.Y+r.Height/2
	check(t, a.QueuePointer(PointerDown, x, y))
	check(t, a.QueuePointer(PointerUp, x, y))
	check(t, a.Step(.016))
}
func TestListMenuNumericNavigation(t *testing.T) {
	a := app(t)
	itemA := Item("a", "Alpha")
	itemB := Item("b", "Beta")
	list := List("list").Children(itemA, itemB)
	menuItem := Item("export", "Export")
	menu := Menu("menu").Text("Actions").Children(menuItem)
	pagination := Pagination("pages").Total(20).Value(10)
	rating := Rating("rating").Total(5).Value(2)
	stepA := Item("start", "Start").StepStatus(2)
	stepB := Item("finish", "Finish").Detail("Complete")
	stepper := Stepper("stepper").Children(stepA, stepB)
	check(t, a.Mount(Column("root").Children(list, menu, pagination, rating, stepper)))
	check(t, a.Step(.016))
	press(t, a, itemB)
	selected, e := a.Selected(list)
	check(t, e)
	if selected != itemB {
		t.Fatal("list selection")
	}
	_, e = a.Dispatch()
	check(t, e)
	press(t, a, menu)
	_, e = a.Bounds(menuItem)
	check(t, e)
	press(t, a, menuItem)
	selected, e = a.Selected(menu)
	check(t, e)
	if selected != menuItem {
		t.Fatal("menu selection")
	}
	_, e = a.Dispatch()
	check(t, e)
	r, e := a.NumericPartBounds(pagination, 3)
	check(t, e)
	clickRect(t, a, r)
	v, e := a.Value(pagination)
	check(t, e)
	if v != 11 {
		t.Fatalf("page %v", v)
	}
	_, e = a.Dispatch()
	check(t, e)
	r, e = a.NumericPartBounds(rating, 14)
	check(t, e)
	clickRect(t, a, r)
	v, e = a.Value(rating)
	check(t, e)
	if v != 5 {
		t.Fatalf("rating %v", v)
	}
	_, e = a.Dispatch()
	check(t, e)
	r, e = a.NumericPartBounds(stepper, 2)
	check(t, e)
	clickRect(t, a, r)
	selected, e = a.Selected(stepper)
	check(t, e)
	if selected != stepB {
		t.Fatal("stepper selection")
	}
	if Rating("bad").Total(11).Err() == nil {
		t.Fatal("bad rating total accepted")
	}
}
func TestDescriptionListScrollable(t *testing.T) {
	a := app(t)
	item := Item("name", "Name").Detail("Matteo")
	descriptions := DescriptionList("descriptions").Children(item)
	content := Label("Scrollable content").Key("content")
	scroll := Scrollable("scroll").VisualSize(180, 60).ScrollExtent(180, 300).Children(content)
	values := []float64{}
	scroll.OnChange(func(e Event) { values = append(values, e.Value) })
	check(t, a.Mount(Column("root").Children(descriptions, scroll)))
	check(t, a.Step(.016))
	_, e := a.Bounds(descriptions)
	check(t, e)
	r, e := a.Bounds(scroll)
	check(t, e)
	check(t, a.QueueScroll(r.X+20, r.Y+20, 0, -1))
	check(t, a.Step(.016))
	v, e := a.Value(scroll)
	check(t, e)
	if v <= 0 {
		t.Fatalf("offset %v", v)
	}
	n, e := a.Dispatch()
	check(t, e)
	if n != 1 || len(values) != 1 || values[0] <= 0 {
		t.Fatalf("scroll events %d %v", n, values)
	}
	if Scrollable("bad").ScrollExtent(-1, 10).Err() == nil {
		t.Fatal("negative extent accepted")
	}
}
func TestStatusToolbarTitle(t *testing.T) {
	a := app(t)
	left := Item("left", "Ready")
	middle := Item("middle", "12 items").StepStatus(1)
	right := Item("right", "Connected").StepStatus(2)
	status := StatusBar("status").Children(left, middle, right)
	title := TitleBar("title").Children(Item("caption", "Document"))
	button := Button("save").Text("Save")
	clicks := 0
	button.OnClick(func(Event) { clicks++ })
	toolbar := Toolbar("toolbar").Children(Item("action", "Action").Children(button))
	check(t, a.Mount(Column("root").Children(status, title, toolbar)))
	check(t, a.Step(.016))
	for _, n := range []*Node{status, title, toolbar, button} {
		r, e := a.Bounds(n)
		check(t, e)
		if r.Width <= 0 {
			t.Fatalf("empty chrome %+v", r)
		}
	}
	press(t, a, button)
	_, e := a.Dispatch()
	check(t, e)
	if clicks != 1 {
		t.Fatal("toolbar button failed")
	}
	toolbar.Disabled(true)
	check(t, toolbar.Err())
	check(t, a.Step(.016))
	press(t, a, button)
	n, e := a.Dispatch()
	check(t, e)
	if n != 0 || clicks != 1 {
		t.Fatal("disabled toolbar clicked")
	}
}
func TestMountRollback(t *testing.T) {
	a := app(t)
	bad := Row("root").Children(Item("orphan", "Orphan"))
	if e := a.Mount(bad); e == nil {
		t.Fatal("orphan Item mounted")
	}
	check(t, a.Mount(Row("retry").Children(Label("Valid").Key("label"))))
	check(t, a.Step(.016))
}
func TestIconAndMessage(t *testing.T) {
	a := app(t)
	icon := Icon("check", "check").VisualSize(24, 24)
	msg := Message("message", "Ada", "Hello").VisualSize(280, 0)
	check(t, a.Mount(Column("root").Children(icon, msg)))
	check(t, a.Step(.016))
	for _, n := range []*Node{icon, msg} {
		r, e := a.Bounds(n)
		check(t, e)
		if r.Width <= 0 || r.Height <= 0 {
			t.Fatalf("empty %+v", r)
		}
	}
	check(t, icon.SetText("missing"))
	if e := a.Step(.016); e == nil {
		t.Fatal("unknown icon accepted")
	}
	check(t, icon.SetText("check"))
	check(t, a.Step(.016))
}
func TestAttachment(t *testing.T) {
	a := app(t)
	attachment := Attachment("attachment", "report.pdf", "1.2 MB").AttachmentStatus(AttachmentUploading).Value(.4).VisualSize(240, 0)
	check(t, a.Mount(Column("root").Children(attachment)))
	check(t, a.Step(.016))
	r, e := a.Bounds(attachment)
	check(t, e)
	if r.Width <= 0 {
		t.Fatalf("empty attachment %+v", r)
	}
	v, e := a.Value(attachment)
	check(t, e)
	if v != .4 {
		t.Fatalf("progress %v", v)
	}
	press(t, a, attachment)
	n, e := a.Dispatch()
	check(t, e)
	if n != 0 {
		t.Fatal("attachment emitted action")
	}
	attachment.AttachmentStatus(AttachmentFailed)
	check(t, attachment.Err())
	check(t, a.Step(.016))
	if Attachment("bad", "file", "note").AttachmentStatus(AttachmentState(9)).Err() == nil {
		t.Fatal("invalid status accepted")
	}
}

func TestImageRGBA(t *testing.T) {
	a := app(t)
	pixels := []byte{255, 0, 0, 255, 0, 255, 0, 255, 0, 0, 255, 255, 255, 255, 255, 255}
	img := Image("image", "four pixels").RGBA(pixels, 2, 2, 8).VisualSize(32, 32)
	pixels[0] = 0
	if img.imagePixels[0] != 255 {
		t.Fatal("image data was not copied")
	}
	check(t, a.Mount(Column("root").Children(img)))
	check(t, a.Step(.016))
	r, err := a.Bounds(img)
	check(t, err)
	if r.Width <= 0 || r.Height <= 0 {
		t.Fatalf("empty image bounds: %+v", r)
	}
	replacement := make([]byte, 16)
	check(t, img.SetRGBA(replacement, 2, 2, 8))
	check(t, a.Step(.016))
	if err := img.SetRGBA(replacement[:15], 2, 2, 8); err == nil {
		t.Fatal("short image accepted")
	}
	if Image("bad", "").RGBA(replacement, 2, 2, 7).Err() == nil {
		t.Fatal("short stride accepted")
	}
	if Image("bad", "").RGBA(replacement, 2049, 1, 8196).Err() == nil {
		t.Fatal("oversized image accepted")
	}
}

func TestTree(t *testing.T) {
	a := app(t)
	parent := Item("parent", "Parent").Depth(0).Checked(true)
	child := Item("child", "Child").Depth(1)
	tree := Tree("tree").VisualSize(240, 180).Children(parent, child).Selected(child)
	var changes []Event
	tree.OnChange(func(e Event) { changes = append(changes, e) })
	check(t, a.Mount(Column("root").Children(tree)))
	check(t, a.Step(.016))
	selected, err := a.Selected(tree)
	check(t, err)
	if selected != child {
		t.Fatal("tree selection lost")
	}
	open, err := a.Checked(parent)
	check(t, err)
	if !open {
		t.Fatal("tree expansion lost")
	}
	r, err := a.Bounds(child)
	check(t, err)
	if r.Width <= 0 {
		t.Fatalf("empty child bounds: %+v", r)
	}
	press(t, a, parent)
	_, err = a.Dispatch()
	check(t, err)
	if len(changes) == 0 {
		t.Fatal("tree change not dispatched")
	}
	tooMany := make([]*Node, 65)
	for i := range tooMany {
		tooMany[i] = Item(fmt.Sprintf("item-%d", i), "item")
	}
	if err := a.Mount(Tree("bad").Children(tooMany...)); err == nil {
		t.Fatal("oversized tree accepted")
	}
	if Item("bad", "Bad").Depth(64).Err() == nil {
		t.Fatal("depth 64 accepted")
	}
}

func TestTimeField(t *testing.T) {
	a := app(t)
	field := TimeField("time").Text("Appointment").Time(Time{9, 45, 30})
	var changes []Event
	field.OnChange(func(e Event) { changes = append(changes, e) })
	check(t, a.Mount(Column("root").Children(field)))
	check(t, a.Step(.016))
	got, err := a.Time(field)
	check(t, err)
	if got != (Time{9, 45, 30}) {
		t.Fatalf("time = %+v", got)
	}
	press(t, a, field)
	check(t, a.QueueKey(KeyDown, Up, 0, false))
	check(t, a.Step(.016))
	_, err = a.Dispatch()
	check(t, err)
	if len(changes) == 0 || len(changes[0].Text) != 8 {
		t.Fatalf("time change payload: %+v", changes)
	}
	got, err = a.Time(field)
	check(t, err)
	if got == (Time{9, 45, 30}) {
		t.Fatal("time did not edit")
	}
	if err := field.SetTime(Time{24, 0, 0}); err == nil {
		t.Fatal("invalid hour accepted")
	}
	check(t, field.SetTime(Time{23, 59, 59}))
	field.Disabled(true)
	check(t, field.Err())
}

func TestColorPicker(t *testing.T) {
	a := app(t)
	picker := ColorPicker("picker").ColorRGBA(0xFF0000FF).VisualSize(240, 0)
	var changes []Event
	picker.OnChange(func(e Event) { changes = append(changes, e) })
	check(t, a.Mount(Column("root").Children(picker)))
	check(t, a.Step(.016))
	got, err := a.ColorRGBA(picker)
	check(t, err)
	if got != 0xFF0000FF {
		t.Fatalf("initial color %08x", got)
	}
	r, err := a.ColorPartBounds(picker, 1)
	check(t, err)
	if r.Width <= 0 {
		t.Fatalf("empty hue bounds: %+v", r)
	}
	if _, err := a.ColorPartBounds(picker, 4); err == nil {
		t.Fatal("invalid color part accepted")
	}
	x, y := r.X+r.Width*.75, r.Y+r.Height*.5
	check(t, a.QueuePointer(PointerDown, x, y))
	check(t, a.QueuePointer(PointerUp, x, y))
	check(t, a.Step(.016))
	_, err = a.Dispatch()
	check(t, err)
	got, err = a.ColorRGBA(picker)
	check(t, err)
	if got == 0xFF0000FF || len(changes) == 0 || len(changes[0].Text) != 9 || changes[0].Value != float64(got) {
		t.Fatalf("color change: %08x %+v", got, changes)
	}
	picker.Disabled(true)
	check(t, picker.Err())
}

func TestCalendarDatePicker(t *testing.T) {
	a := app(t)
	day := Date{2024, 3, 5}
	calendar := Calendar("calendar").Date(day)
	picker := DatePicker("picker").Text("Choose date").Date(day).VisualSize(240, 0)
	var changes []Event
	calendar.OnChange(func(e Event) { changes = append(changes, e) })
	check(t, a.Mount(Column("root").Children(calendar, picker)))
	check(t, a.Step(.016))
	r, err := a.CalendarDayBounds(calendar, Date{2024, 3, 10})
	check(t, err)
	if r.Width <= 0 {
		t.Fatalf("empty day bounds: %+v", r)
	}
	x, y := r.X+r.Width*.5, r.Y+r.Height*.5
	check(t, a.QueuePointer(PointerDown, x, y))
	check(t, a.QueuePointer(PointerUp, x, y))
	check(t, a.Step(.016))
	_, err = a.Dispatch()
	check(t, err)
	got, err := a.Date(calendar)
	check(t, err)
	if got != (Date{2024, 3, 10}) || len(changes) == 0 || changes[0].Text != "2024-03-10" || changes[0].Value != 20240310 {
		t.Fatalf("calendar change: %+v %+v", got, changes)
	}
	if err := picker.SetDate(Date{2024, 2, 30}); err == nil {
		t.Fatal("invalid date accepted")
	}
	press(t, a, picker)
	open, err := a.Open(picker)
	check(t, err)
	if !open {
		t.Fatal("date picker did not open")
	}
	picker.Open(false)
	check(t, picker.Err())
}

func TestOTPInput(t *testing.T) {
	a := app(t)
	otp := OTPInput("otp").Total(8).Text("12")
	var changes []Event
	otp.OnChange(func(e Event) { changes = append(changes, e) })
	check(t, a.Mount(Column("root").Children(otp)))
	check(t, a.Step(.016))
	got, err := a.Text(otp)
	check(t, err)
	if got != "12" {
		t.Fatalf("initial OTP %q", got)
	}
	press(t, a, otp)
	check(t, a.QueueText("9"))
	check(t, a.Step(.016))
	_, err = a.Dispatch()
	check(t, err)
	if len(changes) == 0 || changes[len(changes)-1].Text != "129" {
		t.Fatalf("OTP change %+v", changes)
	}
	if OTPInput("bad").Total(257).Err() == nil {
		t.Fatal("oversized OTP accepted")
	}
	otp.Disabled(true)
	check(t, otp.Err())
}

func TestForm(t *testing.T) {
	a := app(t)
	input := Input("name")
	button := Button("save").Text("Save")
	field := Item("name-field", "Name").Detail("Shown publicly").Children(input)
	action := Item("action", "Action").Children(button)
	form := Form("form").Text("Profile").VisualSize(360, 0).Children(field, action)
	clicks := 0
	button.OnClick(func(Event) { clicks++ })
	check(t, a.Mount(Column("root").Children(form)))
	check(t, a.Step(.016))
	r, err := a.Bounds(input)
	check(t, err)
	if r.Width <= 0 {
		t.Fatalf("empty form field bounds: %+v", r)
	}
	press(t, a, button)
	_, err = a.Dispatch()
	check(t, err)
	if clicks != 1 {
		t.Fatal("form button did not click")
	}
	action.Disabled(true)
	check(t, action.Err())
	check(t, a.Step(.016))
	press(t, a, button)
	_, err = a.Dispatch()
	check(t, err)
	if clicks != 1 {
		t.Fatal("disabled form Item action clicked")
	}
	if err := a.Mount(Form("bad").Children(Label("wrong"))); err == nil {
		t.Fatal("non-Item form child accepted")
	}
}

func TestInputGroup(t *testing.T) {
	a := app(t)
	addon := Item("go", "Go").StepStatus(1)
	clicks := 0
	addon.OnClick(func(Event) { clicks++ })
	group := InputGroup("search").Detail("Search").VisualSize(320, 0).Children(addon)
	var changes []Event
	group.OnChange(func(e Event) { changes = append(changes, e) })
	check(t, a.Mount(Column("root").Children(group)))
	check(t, a.Step(.016))
	r, err := a.Bounds(addon)
	check(t, err)
	if r.Width <= 0 {
		t.Fatalf("empty addon bounds: %+v", r)
	}
	press(t, a, addon)
	_, err = a.Dispatch()
	check(t, err)
	if clicks != 1 {
		t.Fatal("addon click missing")
	}
	press(t, a, group)
	check(t, a.QueueText("a"))
	check(t, a.Step(.016))
	_, err = a.Dispatch()
	check(t, err)
	got, err := a.Text(group)
	check(t, err)
	if got != "a" || len(changes) == 0 || changes[0].Text != "a" {
		t.Fatalf("input group edit: %q %+v", got, changes)
	}
	group.Disabled(true)
	check(t, group.Err())
	check(t, a.Step(.016))
	press(t, a, addon)
	_, err = a.Dispatch()
	check(t, err)
	if clicks != 1 {
		t.Fatal("disabled addon clicked")
	}
}

func TestNotification(t *testing.T) {
	a := app(t)
	note := Notification("note", "Saved", "Your file was saved").Open(true)
	clicks, closes := 0, 0
	note.OnClick(func(Event) { clicks++ }).OnClose(func(Event) { closes++ })
	check(t, a.Mount(Column("root").Children(note)))
	check(t, a.Step(.4))
	r, err := a.Bounds(note)
	check(t, err)
	if r.Width <= 0 || r.Height <= 0 {
		t.Fatalf("empty notification: %+v", r)
	}
	open, err := a.Open(note)
	check(t, err)
	if !open {
		t.Fatal("notification did not open")
	}
	press(t, a, note)
	_, err = a.Dispatch()
	check(t, err)
	if clicks != 1 {
		t.Fatal("notification click missing")
	}
	check(t, a.Step(.3))
	_, err = a.Dispatch()
	check(t, err)
	if closes != 1 {
		t.Fatalf("notification close count %d", closes)
	}
	open, err = a.Open(note)
	check(t, err)
	if open {
		t.Fatal("notification remained open")
	}
	check(t, note.SetText("Again"))
}

func TestFocusTrap(t *testing.T) {
	a := app(t)
	outside := Button("outside").Text("Outside")
	inside := Button("inside").Text("Inside")
	trap := FocusTrap("trap").Open(true).Children(inside)
	outsideClicks, insideClicks := 0, 0
	outside.OnClick(func(Event) { outsideClicks++ })
	inside.OnClick(func(Event) { insideClicks++ })
	check(t, a.Mount(Column("root").Children(outside, trap)))
	check(t, a.Step(.016))
	open, err := a.Open(trap)
	check(t, err)
	if !open {
		t.Fatal("trap did not open")
	}
	press(t, a, outside)
	_, err = a.Dispatch()
	check(t, err)
	if outsideClicks != 0 {
		t.Fatal("outside action bypassed trap")
	}
	press(t, a, inside)
	_, err = a.Dispatch()
	check(t, err)
	if insideClicks != 1 {
		t.Fatal("inside action blocked")
	}
	trap.Open(false)
	check(t, trap.Err())
	check(t, a.Step(.016))
	press(t, a, outside)
	_, err = a.Dispatch()
	check(t, err)
	if outsideClicks != 1 {
		t.Fatal("outside action not restored")
	}
}
func TestFontFallbackValidation(t *testing.T) {
	a := app(t)
	if err := a.SetFontFallbacks([]string{"relative.ttf"}); err == nil {
		t.Fatal("relative fallback accepted")
	}
	if err := a.SetFontFallbacks(make([]string, 33)); err == nil {
		t.Fatal("too many fallbacks accepted")
	}
	if err := a.SetFontFallbacks([]string{"/some/font.ttf"}); err == nil {
		t.Fatal("headless app accepted SDL font fallbacks")
	}
}
