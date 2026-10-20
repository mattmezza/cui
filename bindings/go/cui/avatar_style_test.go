package cui

import "testing"

func TestAvatarStyle(t *testing.T) {
	a := app(t)
	person := Avatar("person", "Ada Lovelace").DesignSize(MD).IdentityColors(true, 4)
	anonymous := Avatar("anonymous", "").AnonymousIcon(true, "user")
	legacy := Avatar("legacy", "L").VisualSize(60, 60)
	if e := a.Mount(Column("root").Children(person, anonymous, legacy)); e != nil {
		t.Fatal(e)
	}
	if e := a.Step(.016); e != nil {
		t.Fatal(e)
	}
	for node, size := range map[*Node]float32{person: 48, anonymous: 40, legacy: 60} {
		b, e := a.Bounds(node)
		if e != nil || b.Width != size || b.Height != size {
			t.Fatalf("bounds=%+v error=%v want %v", b, e, size)
		}
	}
	person.DesignSize(LG).IdentityColors(false).AnonymousIcon(false, "")
	if person.Err() != nil {
		t.Fatal(person.Err())
	}
	if e := a.Step(.016); e != nil {
		t.Fatal(e)
	}
	b, e := a.Bounds(person)
	if e != nil || b.Width != 80 {
		t.Fatalf("live size=%+v error=%v", b, e)
	}
	person.DesignSize(Inherit)
	if e := a.Step(.016); e != nil {
		t.Fatal(e)
	}
	b, e = a.Bounds(person)
	if e != nil || b.Width != 40 {
		t.Fatalf("inherited size=%+v error=%v", b, e)
	}
	for _, bad := range []*Node{Avatar("bad", "").IdentityColors(true, 12), Avatar("bad", "").IdentityColors(true, 1, 2), Avatar("bad", "").AnonymousIcon(true, "bad\x00"), Avatar("bad", "").AnonymousIcon(true, "\x80"), Button("bad").IdentityColors(true), Row("bad").DesignSize(MD), Avatar("bad", "").Size(MD)} {
		if bad.Err() == nil {
			t.Fatal("invalid style accepted")
		}
	}
	ordered := Button("ordered").DesignSize(LG).Size(SM)
	if ordered.Err() != nil || ordered.size != int(SM) {
		t.Fatal("Button setter order changed")
	}
	if e := person.Remove(); e != nil {
		t.Fatal(e)
	}
	if person.DesignSize(MD).Err() == nil {
		t.Fatal("stale Avatar accepted")
	}
}
