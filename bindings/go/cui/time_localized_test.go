package cui

import (
	"encoding/binary"
	"os"
	"reflect"
	"testing"
)

func TestLocalizedTimeWireOwnedAndValidated(t *testing.T) {
	c := ArabicTimeField()
	min, max := Time{Hour: 8}, Time{Hour: 18}
	c.Minimum, c.Maximum = &min, &max
	c.EditGeneration = 0xfedcba9876543210
	b, e := timeWire(c)
	check(t, e)
	saved := timeCopy(c)
	c.Tokens[0] = "X"
	min.Hour = 9
	got, e := timeConfigRead(b)
	check(t, e)
	if !reflect.DeepEqual(got, saved) {
		t.Fatal(got, saved)
	}
	bad := EnglishTimeField()
	bad.Tokens[1] = "00"
	if _, e = timeWire(bad); e == nil {
		t.Fatal("prefix alias accepted")
	}
	bad = EnglishTimeField()
	bad.Minimum = &Time{Hour: 8, Second: 1}
	if _, e = timeWire(bad); e == nil {
		t.Fatal("minute bound seconds accepted")
	}
	b[615] = 1
	if _, e = timeConfigRead(b); e == nil {
		t.Fatal("unused name bytes accepted")
	}
}
func TestLocalizedTimeAppliedCallbacksAndAtomicSet(t *testing.T) {
	a := app(t)
	check(t, a.EnableLocalizedTimeFields(4))
	c := EnglishTimeField()
	c.EditGeneration = 1
	n := TimeField("local").Time(Time{Hour: 9, Minute: 30}).LocalizedTime(c)
	var copies []TimeFieldChange
	n.OnTimeFieldChange(func(v TimeFieldChange) { copies = append(copies, v) })
	check(t, a.Mount(Column("root").Children(n)))
	check(t, a.Step(.016))
	check(t, a.Step(.016))
	if _, e := a.LocalizedTimeResult(n); e != nil {
		t.Fatal("ordinary admission", e)
	}
	b, e := a.Bounds(n)
	check(t, e)
	check(t, a.QueuePointer(PointerDown, b.X+b.Width/4, b.Y+b.Height/2))
	check(t, a.QueuePointer(PointerUp, b.X+b.Width/4, b.Y+b.Height/2))
	check(t, a.QueueText("10:45"))
	check(t, a.Step(.016))
	if len(copies) != 0 {
		t.Fatal("callback inside step")
	}
	_, e = a.DispatchTimeFieldChanges()
	check(t, e)
	t.Logf("bounds=%+v", b)
	diag, de := a.LocalizedTimeResult(n)
	t.Logf("result=%+v error=%v", diag, de)
	if len(copies) != 1 || copies[0].Previous != (Time{Hour: 9, Minute: 30}) || copies[0].Value != (Time{Hour: 10, Minute: 45}) {
		t.Fatal(copies)
	}
	_, ok, e := a.PollTimeFieldChange()
	check(t, e)
	if ok {
		t.Fatal("replayed result")
	}
	before, e := a.LocalizedTimeConfig(n)
	check(t, e)
	bad := before
	bad.Tokens[0] = ""
	if n.SetLocalizedTime(bad) == nil {
		t.Fatal("invalid replacement accepted")
	}
	after, e := a.LocalizedTimeConfig(n)
	check(t, e)
	if !reflect.DeepEqual(before, after) {
		t.Fatal("failed setter changed config")
	}
	check(t, n.Remove())
	if copies[0].Value.Hour != 10 {
		t.Fatal("copied observation invalidated")
	}
}
func TestLocalizedTimeWrongKindAndMountRollback(t *testing.T) {
	if Button("bad").SetLocalizedTime(EnglishTimeField()) == nil {
		t.Fatal("wrong kind")
	}
	a := app(t)
	check(t, a.EnableLocalizedTimeFields(1))
	one := TimeField("one").LocalizedTime(EnglishTimeField())
	two := TimeField("two").LocalizedTime(ArabicTimeField())
	if a.Mount(Column("root").Children(one, two)) == nil {
		t.Fatal("capacity accepted")
	}
	if one.app != nil || two.app != nil || a.root != nil {
		t.Fatal("mount left partial ownership")
	}
}
func TestLocalizedTimeCopiedFailureProtocol(t *testing.T) {
	b := timeHeader(64)
	timePut(b, 8, 10)
	binary.NativeEndian.PutUint64(b[24:], 1)
	timePut(b, 16, 1)
	timePut(b, 20, 2)
	timeWrite(b, 32, Time{Hour: 8})
	timeWrite(b, 44, Time{Hour: 9})
	c, e := timeChangeRead(b)
	check(t, e)
	if c.FrameStatus != 10 || c.Value.Hour != 9 {
		t.Fatal(c)
	}
	timePut(b, 12, 1)
	if _, e = timeChangeRead(b); e == nil {
		t.Fatal("reserved accepted")
	}
}

func TestLocalizedTimeZeroGenerationPreflight(t *testing.T) {
	c := EnglishTimeField()
	if c.EditGeneration != 1 || ArabicTimeField().EditGeneration != 1 {
		t.Fatal("invalid defaults")
	}
	c.EditGeneration = 0
	if _, e := timeWire(c); e == nil {
		t.Fatal("authored zero normalized")
	}
	n := TimeField("zero")
	if n.SetLocalizedTime(c) == nil || n.timeLocalized != nil {
		t.Fatal("invalid config committed")
	}
}
func TestLocalizedTimeOptionalProvider(t *testing.T) {
	if os.Getenv("CUI_TEST_TIME_PROVIDER") == "" {
		return
	}
	a := app(t)
	supported, e := a.SupportsLocalizedTimeField()
	check(t, e)
	if supported {
		t.Fatal("incomplete group advertised")
	}
	legacy := TimeField("legacy").Time(Time{Hour: 9, Minute: 30})
	check(t, a.Mount(Column("root").Children(legacy)))
	check(t, a.Step(.016))
	v, e := a.Time(legacy)
	check(t, e)
	if v.Hour != 9 {
		t.Fatal(v)
	}
	if a.EnableLocalizedTimeFields(2) == nil {
		t.Fatal("unavailable feature enabled")
	}
	if legacy.SetLocalizedTime(EnglishTimeField()) == nil || legacy.timeLocalized != nil {
		t.Fatal("failed feature setter committed")
	}
}

func timeTestPaste(t *testing.T, a *App, n *Node, text string) {
	t.Helper()
	b, e := a.Bounds(n)
	check(t, e)
	check(t, a.QueuePointer(PointerDown, b.X+b.Width/4, b.Y+b.Height/2))
	check(t, a.QueuePointer(PointerUp, b.X+b.Width/4, b.Y+b.Height/2))
	check(t, a.QueueText(text))
	check(t, a.Step(.016))
}
func TestLocalizedTimeArabicReadonlyDisabledBounds(t *testing.T) {
	a := app(t)
	c := ArabicTimeField()
	min, max := Time{Hour: 8}, Time{Hour: 18}
	c.Minimum, c.Maximum = &min, &max
	n := TimeField("arabic").Time(Time{Hour: 14, Minute: 15}).LocalizedTime(c)
	check(t, a.Mount(Column("root").Children(n)))
	check(t, a.Step(.016))
	check(t, a.Step(.016))
	timeTestPaste(t, a, n, "٠٣:٢٠ م")
	v, e := a.Time(n)
	check(t, e)
	if v != (Time{Hour: 15, Minute: 20}) {
		t.Fatal(v)
	}
	_, e = a.DispatchTimeFieldChanges()
	check(t, e)
	for _, mode := range []string{"readonly", "disabled"} {
		next := c
		next.ReadOnly = mode == "readonly"
		next.Disabled = mode == "disabled"
		check(t, n.SetLocalizedTime(next))
		check(t, a.Step(.016))
		check(t, a.Step(.016))
		timeTestPaste(t, a, n, "٠٤:٣٠ م")
		v, e = a.Time(n)
		check(t, e)
		if v != (Time{Hour: 15, Minute: 20}) {
			t.Fatal(mode, v)
		}
	}
	check(t, n.SetLocalizedTime(c))
	check(t, a.Step(.016))
	check(t, a.Step(.016))
	timeTestPaste(t, a, n, "١١:٣٠ م")
	r, e := a.LocalizedTimeResult(n)
	check(t, e)
	if r.Validation != 5 || r.Changed {
		t.Fatal(r)
	}
	check(t, a.ClearTimeValidation(n))
	check(t, a.Step(.016))
	r, e = a.LocalizedTimeResult(n)
	check(t, e)
	if r.Validation != 0 {
		t.Fatal(r)
	}
}
func TestLocalizedTimeOwnerPanicCloseLifecycle(t *testing.T) {
	a := app(t)
	n := TimeField("local").Time(Time{Hour: 9}).LocalizedTime(EnglishTimeField())
	check(t, a.Mount(Column("root").Children(n)))
	check(t, a.Step(.016))
	check(t, a.Step(.016))
	done := make(chan error, 1)
	go func() { _, e := a.LocalizedTimeConfig(n); done <- e }()
	if e := <-done; e == nil {
		t.Fatal("wrong owner accepted")
	}
	n.OnTimeFieldChange(func(TimeFieldChange) { panic("application callback") })
	timeTestPaste(t, a, n, "10:30")
	panicked := false
	func() { defer func() { panicked = recover() != nil }(); _, _ = a.DispatchTimeFieldChanges() }()
	if !panicked || a.timeDispatching {
		t.Fatal("panic/guard")
	}
	_, ok, e := a.PollTimeFieldChange()
	check(t, e)
	if ok {
		t.Fatal("panic record replay")
	}
	var owned TimeFieldChange
	n.OnTimeFieldChange(func(c TimeFieldChange) {
		owned = c
		if a.Step(.016) == nil {
			t.Fatal("nested step accepted")
		}
		check(t, a.Close())
	})
	timeTestPaste(t, a, n, "11:45")
	_, e = a.DispatchTimeFieldChanges()
	check(t, e)
	if !a.closed || owned.Value.Hour != 11 {
		t.Fatal(owned)
	}
}

func TestLocalizedTimeMalformedObservationsAndUnchangedResult(t *testing.T) {
	b := timeHeader(64)
	timePut(b, 16, 1)
	timePut(b, 20, 2)
	binary.NativeEndian.PutUint64(b[24:], 1)
	timeWrite(b, 32, Time{Hour: 8})
	timeWrite(b, 44, Time{Hour: 9})
	for _, poison := range []struct {
		offset int
		value  uint32
	}{{24, 0}, {8, 1}, {56, 1}, {56, 2}, {56, 6}} {
		bad := append([]byte(nil), b...)
		timePut(bad, poison.offset, poison.value)
		if _, e := timeChangeRead(bad); e == nil {
			t.Fatal("malformed accepted", poison)
		}
	}
	same := append([]byte(nil), b...)
	timeWrite(same, 44, Time{Hour: 8})
	if _, e := timeChangeRead(same); e == nil {
		t.Fatal("unchanged applied record")
	}
	r := timeHeader(80)
	copy(r[16:32], b[16:32])
	timeWrite(r, 48, Time{Hour: 8})
	timeWrite(r, 60, Time{Hour: 8})
	out, e := timeResultRead(r, Handle{1, 2})
	check(t, e)
	if out.Changed || out.QueuedChangeCount != 0 {
		t.Fatal(out)
	}
	for _, poison := range []struct {
		offset int
		value  uint32
	}{{40, 1}, {72, 1}, {76, 1}, {16, 3}} {
		bad := append([]byte(nil), r...)
		timePut(bad, poison.offset, poison.value)
		if _, e := timeResultRead(bad, Handle{1, 2}); e == nil {
			t.Fatal("inconsistent result", poison)
		}
	}
}
