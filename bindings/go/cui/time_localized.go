package cui

// Owned localized TimeField V1. The C widget remains the only reducer.
import (
	"encoding/binary"
	"errors"
	"strings"
	"unicode/utf8"
)

type TimeCycle uint32

const (
	TimeH23 TimeCycle = iota
	TimeH12
)

type TimePrecision uint32

const (
	TimeMinute TimePrecision = iota
	TimeSecond
)

// Tokens are digits0..9, separator, period separator, AM, PM. All are explicit.
type TimeFieldConfig struct {
	Cycle                            TimeCycle
	Precision                        TimePrecision
	Disabled, ReadOnly, OwnerInvalid bool
	Minimum, Maximum                 *Time
	EditGeneration                   uint64
	Tokens                           [14]string
	Names                            [4]string
}

func EnglishTimeField() TimeFieldConfig {
	return TimeFieldConfig{Tokens: [14]string{"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", ":", " ", "AM", "PM"}}
}
func ArabicTimeField() TimeFieldConfig {
	c := EnglishTimeField()
	c.Cycle = TimeH12
	c.Tokens = [14]string{"٠", "١", "٢", "٣", "٤", "٥", "٦", "٧", "٨", "٩", ":", " ", "ص", "م"}
	c.Names = [4]string{"الساعة", "الدقيقة", "الثانية", "الفترة"}
	return c
}
func timeCopy(c TimeFieldConfig) TimeFieldConfig {
	if c.Minimum != nil {
		v := *c.Minimum
		c.Minimum = &v
	}
	if c.Maximum != nil {
		v := *c.Maximum
		c.Maximum = &v
	}
	return c
}
func timeValid(t Time, p TimePrecision) bool {
	return t.Hour < 24 && t.Minute < 60 && t.Second < 60 && (p == TimeSecond || t.Second == 0)
}
func timeSeconds(t Time) uint32 { return t.Hour*3600 + t.Minute*60 + t.Second }
func (c TimeFieldConfig) validate() error {
	bad := errors.New("invalid localized TimeField configuration")
	if c.Cycle > TimeH12 || c.Precision > TimeSecond {
		return bad
	}
	for i, t := range c.Tokens {
		if len(t) == 0 || len(t) > 16 || !utf8.ValidString(t) || strings.IndexByte(t, 0) >= 0 {
			return bad
		}
		for j, u := range c.Tokens {
			if i != j && strings.HasPrefix(u, t) {
				return bad
			}
		}
	}
	for _, n := range c.Names {
		if len(n) > 64 || !utf8.ValidString(n) || strings.IndexByte(n, 0) >= 0 {
			return bad
		}
	}
	if c.Minimum != nil && !timeValid(*c.Minimum, c.Precision) {
		return bad
	}
	if c.Maximum != nil && !timeValid(*c.Maximum, c.Precision) {
		return bad
	}
	if c.Minimum != nil && c.Maximum != nil && timeSeconds(*c.Minimum) > timeSeconds(*c.Maximum) {
		return bad
	}
	return nil
}
func timePut(b []byte, i int, v uint32) { binary.NativeEndian.PutUint32(b[i:], v) }
func timeGet(b []byte, i int) uint32    { return binary.NativeEndian.Uint32(b[i:]) }
func timeWrite(b []byte, i int, v Time) {
	timePut(b, i, v.Hour)
	timePut(b, i+4, v.Minute)
	timePut(b, i+8, v.Second)
}
func timeRead(b []byte, i int) Time { return Time{timeGet(b, i), timeGet(b, i+4), timeGet(b, i+8)} }
func timeWire(c TimeFieldConfig) ([]byte, error) {
	if e := c.validate(); e != nil {
		return nil, e
	}
	b := make([]byte, 616)
	timePut(b, 0, 616)
	timePut(b, 4, 1)
	var flags uint32
	if c.Disabled {
		flags |= 1
	}
	if c.ReadOnly {
		flags |= 2
	}
	if c.OwnerInvalid {
		flags |= 4
	}
	timePut(b, 8, flags)
	timePut(b, 12, uint32(c.Cycle))
	timePut(b, 16, uint32(c.Precision))
	var bounds uint32
	if c.Minimum != nil {
		bounds |= 1
		timeWrite(b, 32, *c.Minimum)
	}
	if c.Maximum != nil {
		bounds |= 2
		timeWrite(b, 44, *c.Maximum)
	}
	timePut(b, 20, bounds)
	binary.NativeEndian.PutUint64(b[56:], c.EditGeneration)
	for i, t := range c.Tokens {
		at := 64 + i*20
		timePut(b, at, uint32(len(t)))
		copy(b[at+4:at+20], t)
	}
	for i, t := range c.Names {
		at := 344 + i*68
		timePut(b, at, uint32(len(t)))
		copy(b[at+4:at+68], t)
	}
	return b, nil
}
func timeConfigRead(b []byte) (TimeFieldConfig, error) {
	c := TimeFieldConfig{}
	bad := errors.New("invalid localized TimeField wire configuration")
	if len(b) != 616 || timeGet(b, 0) != 616 || timeGet(b, 4) != 1 || timeGet(b, 8)&^7 != 0 || timeGet(b, 20)&^3 != 0 || timeGet(b, 24) != 0 || timeGet(b, 28) != 0 {
		return c, bad
	}
	flags := timeGet(b, 8)
	c.Disabled = flags&1 != 0
	c.ReadOnly = flags&2 != 0
	c.OwnerInvalid = flags&4 != 0
	c.Cycle = TimeCycle(timeGet(b, 12))
	c.Precision = TimePrecision(timeGet(b, 16))
	c.EditGeneration = binary.NativeEndian.Uint64(b[56:])
	min, max := timeRead(b, 32), timeRead(b, 44)
	if timeGet(b, 20)&1 != 0 {
		c.Minimum = &min
	} else if min != (Time{}) {
		return c, bad
	}
	if timeGet(b, 20)&2 != 0 {
		c.Maximum = &max
	} else if max != (Time{}) {
		return c, bad
	}
	for i := range c.Tokens {
		at := 64 + i*20
		n := int(timeGet(b, at))
		if n > 16 {
			return c, bad
		}
		c.Tokens[i] = string(b[at+4 : at+4+n])
		for _, x := range b[at+4+n : at+20] {
			if x != 0 {
				return c, bad
			}
		}
	}
	for i := range c.Names {
		at := 344 + i*68
		n := int(timeGet(b, at))
		if n > 64 {
			return c, bad
		}
		c.Names[i] = string(b[at+4 : at+4+n])
		for _, x := range b[at+4+n : at+68] {
			if x != 0 {
				return c, bad
			}
		}
	}
	return c, c.validate()
}

// Changes are applied copied observations, including non-OK FrameStatus. Never accept/replay.
type TimeFieldChange struct {
	Handle                                 Handle
	Frame                                  uint64
	Previous, Value                        Time
	Validation, RejectedCount, FrameStatus uint32
}
type TimeFieldResult struct {
	TimeFieldChange
	Status            uint32
	Changed           bool
	QueuedChangeCount uint32
}

func timeHeader(size int) []byte {
	b := make([]byte, size)
	timePut(b, 0, uint32(size))
	timePut(b, 4, 1)
	return b
}
func timeChangeRead(b []byte) (TimeFieldChange, error) {
	c := TimeFieldChange{}
	if len(b) != 64 || timeGet(b, 0) != 64 || timeGet(b, 4) != 1 || timeGet(b, 12) != 0 {
		return c, errors.New("invalid TimeField change header")
	}
	c = TimeFieldChange{Handle: Handle{timeGet(b, 16), timeGet(b, 20)}, Frame: binary.NativeEndian.Uint64(b[24:]), Previous: timeRead(b, 32), Value: timeRead(b, 44), Validation: timeGet(b, 56), RejectedCount: timeGet(b, 60), FrameStatus: timeGet(b, 8)}
	if c.Handle.Slot == 0 || c.Handle.Generation == 0 || c.Validation > 6 || c.FrameStatus > 10 || !timeValid(c.Previous, TimeSecond) || !timeValid(c.Value, TimeSecond) {
		return TimeFieldChange{}, errors.New("invalid TimeField change values")
	}
	return c, nil
}
func (a *App) EnableLocalizedTimeFields(capacity uint32) error {
	if e := a.check(); e != nil {
		return e
	}
	if capacity == 0 {
		capacity = 4096
	}
	if capacity > 4096 {
		return errors.New("invalid TimeField capacity")
	}
	if a.timeEnabled && capacity != a.timeCapacity {
		return errors.New("TimeField capacity is immutable")
	}
	_, e := a.timeCall(nil, 1, capacity, nil)
	if e == nil {
		a.timeEnabled = true
		a.timeCapacity = capacity
	}
	return e
}
func (n *Node) LocalizedTime(c TimeFieldConfig) *Node {
	if n.err == nil {
		n.err = n.SetLocalizedTime(c)
	}
	return n
}
func (n *Node) SetLocalizedTime(c TimeFieldConfig) error {
	if n == nil || n.kind != 55 || n.removed {
		return errors.New("localized time requires live TimeField")
	}
	b, e := timeWire(c)
	if e != nil {
		return e
	}
	if n.app != nil {
		if !n.app.timeEnabled {
			return errors.New("enable localized TimeFields first")
		}
		if _, e = n.app.timeCall(n, 2, 0, b); e != nil {
			return e
		}
	}
	owned := timeCopy(c)
	n.timeLocalized = &owned
	return nil
}
func (n *Node) OnTimeFieldChange(fn func(TimeFieldChange)) *Node {
	if n == nil {
		return n
	}
	if n.kind != 55 || n.removed {
		n.err = errors.New("TimeField callback requires live TimeField")
		return n
	}
	if n.app != nil {
		if e := n.app.check(); e != nil {
			n.err = e
			return n
		}
	}
	n.timeChange = fn
	return n
}
func (a *App) timePreflight(root *Node) error {
	found := false
	var visit func(*Node) error
	visit = func(n *Node) error {
		if n.timeLocalized != nil {
			found = true
			if _, e := timeWire(*n.timeLocalized); e != nil {
				return e
			}
		}
		for _, ch := range n.children {
			if e := visit(ch); e != nil {
				return e
			}
		}
		return nil
	}
	if e := visit(root); e != nil {
		return e
	}
	if found && !a.timeEnabled {
		return a.EnableLocalizedTimeFields(4096)
	}
	return nil
}
func (a *App) LocalizedTimeConfig(n *Node) (TimeFieldConfig, error) {
	b := timeHeader(616)
	if _, e := a.timeCall(n, 3, 0, b); e != nil {
		return TimeFieldConfig{}, e
	}
	return timeConfigRead(b)
}
func (a *App) ClearTimeValidation(n *Node) error { _, e := a.timeCall(n, 5, 0, nil); return e }
func (a *App) LocalizedTimeResult(n *Node) (TimeFieldResult, error) {
	b := timeHeader(80)
	if _, e := a.timeCall(n, 4, 0, b); e != nil {
		return TimeFieldResult{}, e
	}
	if timeGet(b, 0) != 80 || timeGet(b, 4) != 1 || timeGet(b, 8) != 0 || timeGet(b, 12) != 0 || timeGet(b, 32) > 6 || timeGet(b, 40) > 1 || timeGet(b, 72) > 1 {
		return TimeFieldResult{}, errors.New("invalid TimeField result")
	}
	raw := timeHeader(64)
	copy(raw[16:32], b[16:32])
	copy(raw[32:56], b[48:72])
	timePut(raw, 8, timeGet(b, 76))
	timePut(raw, 56, timeGet(b, 36))
	timePut(raw, 60, timeGet(b, 44))
	c, e := timeChangeRead(raw)
	return TimeFieldResult{c, timeGet(b, 32), timeGet(b, 40) != 0, timeGet(b, 72)}, e
}
func (a *App) PollTimeFieldChange() (TimeFieldChange, bool, error) {
	b := timeHeader(64)
	code, e := a.timeCall(nil, 6, 0, b)
	if e != nil || code == 1 {
		return TimeFieldChange{}, false, e
	}
	c, e := timeChangeRead(b)
	return c, e == nil, e
}

// Dispatch is post-step. A panic propagates after consuming this copied record;
// defer restores the dispatch guard. Removal/close stop future owned callbacks.
func (a *App) DispatchTimeFieldChanges() (int, error) {
	if e := a.check(); e != nil {
		return 0, e
	}
	if a.timeDispatching {
		return 0, errors.New("nested TimeField dispatch")
	}
	a.timeDispatching = true
	defer func() { a.timeDispatching = false }()
	count := 0
	for !a.closed && !a.timeClosing {
		c, ok, e := a.PollTimeFieldChange()
		if e != nil {
			return count, e
		}
		if !ok {
			return count, nil
		}
		count++
		for _, n := range a.nodes {
			if !n.removed && uint32(n.handle.slot) == c.Handle.Slot && uint32(n.handle.generation) == c.Handle.Generation && n.timeChange != nil {
				n.timeChange(c)
				break
			}
		}
	}
	return count, nil
}
