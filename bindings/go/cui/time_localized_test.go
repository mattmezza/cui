package cui

import (
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
	b, e := a.Bounds(n)
	check(t, e)
	check(t, a.QueuePointer(PointerDown, b.X+b.Width/2, b.Y+b.Height/2))
	check(t, a.QueuePointer(PointerUp, b.X+b.Width/2, b.Y+b.Height/2))
	check(t, a.QueueText("10:45"))
	check(t, a.Step(.016))
	if len(copies) != 0 {
		t.Fatal("callback inside step")
	}
	_, e = a.DispatchTimeFieldChanges()
	check(t, e)
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
