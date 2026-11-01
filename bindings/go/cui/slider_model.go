package cui

import (
	"errors"
	"math"
)

// SliderValue is an owned double Single/Range pair; its tag/inactive fields are private.
type SliderValue struct {
	kind       uint32
	start, end float64
}

func NewSliderSingle(end float64) (SliderValue, error) {
	v := SliderValue{end: end}
	return v, v.validate()
}
func NewSliderRange(start, end float64) (SliderValue, error) {
	v := SliderValue{kind: 1, start: start, end: end}
	return v, v.validate()
}
func (v SliderValue) IsRange() bool  { return v.kind == 1 }
func (v SliderValue) Start() float64 { return v.start }
func (v SliderValue) End() float64   { return v.end }
func (v SliderValue) validate() error {
	if v.kind > 1 || !sliderFinite(v.start) || !sliderFinite(v.end) || (v.kind == 0 && v.start != 0) || (v.kind == 1 && v.start > v.end) {
		return errors.New("invalid Slider pair")
	}
	return nil
}
func sliderFinite(v float64) bool { return !math.IsNaN(v) && !math.IsInf(v, 0) }

type SliderScale uint32

const (
	SliderLinear SliderScale = iota
	SliderLogarithmic
)

type SliderOrientation uint32

const (
	SliderHorizontal SliderOrientation = iota
	SliderVertical
)

type SliderLayout uint32

const (
	SliderFixed SliderLayout = iota
	SliderParent
)

type SliderPhase uint32

const (
	SliderBegin SliderPhase = iota
	SliderUpdate
	SliderCommit
	SliderCancel
)

type SliderPart uint32

const (
	SliderRoot SliderPart = iota
	SliderTrack
	SliderFill
	SliderStartThumb
	SliderEndThumb
)

type SliderDomain struct {
	Minimum, Maximum, Step float64
	Scale                  SliderScale
}
type SliderConfig struct {
	Domain                                                    SliderDomain
	SourceGeneration                                          uint64
	Orientation                                               SliderOrientation
	Layout                                                    SliderLayout
	Rem, Length                                               float32
	Requests, Readonly, ReverseFill, ReducedMotion, HideFocus bool
}

func DefaultSliderConfig() SliderConfig {
	return SliderConfig{Domain: SliderDomain{Maximum: 100, Step: 1}, Rem: 16, Length: 200, Requests: true}
}
func (d SliderDomain) validate() error {
	if !sliderFinite(d.Minimum) || !sliderFinite(d.Maximum) || !sliderFinite(d.Step) || d.Minimum > d.Maximum || !sliderFinite(d.Maximum-d.Minimum) || d.Step < 0 || d.Scale > SliderLogarithmic || (d.Scale == SliderLogarithmic && (d.Minimum <= 0 || d.Maximum <= d.Minimum)) {
		return errors.New("invalid Slider domain")
	}
	return nil
}
func sliderConfig(c SliderConfig) (SliderConfig, error) {
	if c.Layout == SliderParent {
		c.Length = 0
	}
	if e := c.Domain.validate(); e != nil {
		return SliderConfig{}, e
	}
	if c.Orientation > SliderVertical || c.Layout > SliderParent || !finite(c.Rem) || c.Rem <= 0 || !finite(c.Length) || (c.Layout == SliderFixed && c.Length <= 0) {
		return SliderConfig{}, errors.New("invalid Slider geometry/config")
	}
	return c, nil
}
func sliderPair(v SliderValue, d SliderDomain) error {
	if e := v.validate(); e != nil {
		return e
	}
	if v.end < d.Minimum || v.end > d.Maximum || (v.kind == 1 && (v.start < d.Minimum || v.start > d.Maximum)) {
		return errors.New("Slider pair outside domain")
	}
	return nil
}

// SliderReceiptInfo is inert readable metadata, not acceptance authority.
type SliderReceiptInfo struct {
	AppEpoch                                                                  uint64
	Slot, Generation                                                          uint32
	SourceGeneration, ConfigurationRevision, SubmissionFrame, RequestSequence uint64
}
type sliderNamespace struct{ nonzero byte }
type SliderReceipt struct {
	info      SliderReceiptInfo
	namespace *sliderNamespace
}

func (r SliderReceipt) Info() SliderReceiptInfo { return r.info }

type SliderInput struct {
	Type, Flags, Key, Modifiers                             uint32
	X, Y                                                    float32
	Repeat, PointerType, PointerButton, AccessibilityAction uint32
	PointerID, PointerDevice, AccessibilityTarget           uint64
	AccessibilityValue                                      float64
}
type SliderRequest struct {
	Phase                                             SliderPhase
	Reason, Thumb, EventKind, EventSource, EventFlags uint32
	Receipt                                           SliderReceipt
	EventTarget, EventFrame, EventIndex               uint64
	Current, Requested                                SliderValue
	Domain, GestureDomain                             SliderDomain
	Input                                             SliderInput
}
type SliderPresentation struct {
	RenderStatus              uint32
	CoreTime, NextWakeSeconds float64
}
type sliderNode struct {
	config    SliderConfig
	confirmed SliderValue
	callback  func(SliderRequest) error
}

func SliderModel(key string, config SliderConfig, confirmed SliderValue) *Node {
	n := &Node{kind: 81, key: key, text: "Selection"}
	c, e := sliderConfig(config)
	if e == nil {
		e = sliderPair(confirmed, c.Domain)
	}
	n.err = e
	n.slider = &sliderNode{config: c, confirmed: confirmed}
	return n
}
func (n *Node) sliderCheck() error {
	if n == nil || n.kind != 81 || n.slider == nil || n.removed {
		return errors.New("invalid controlled Slider node")
	}
	if n.err != nil {
		return n.err
	}
	if n.app != nil {
		return n.app.check()
	}
	return nil
}
func (n *Node) OnSliderRequest(callback func(SliderRequest) error) *Node {
	if n != nil {
		if e := n.sliderCheck(); e != nil {
			n.err = e
		} else {
			n.slider.callback = callback
		}
	}
	return n
}
func (n *Node) ConfigureSlider(config SliderConfig, confirmed SliderValue) error {
	if e := n.sliderCheck(); e != nil {
		return e
	}
	c, e := sliderConfig(config)
	if e != nil {
		return e
	}
	if e = sliderPair(confirmed, c.Domain); e != nil {
		return e
	}
	if n.app != nil {
		if e = n.app.sliderConfigureNative(n, c, confirmed); e != nil {
			return e
		}
	}
	n.slider.config = c
	n.slider.confirmed = confirmed
	return nil
}
func (n *Node) SetSliderValue(value SliderValue) error {
	if e := n.sliderCheck(); e != nil {
		return e
	}
	if value.kind != n.slider.confirmed.kind {
		return errors.New("SetSliderValue cannot change kind")
	}
	if e := sliderPair(value, n.slider.config.Domain); e != nil {
		return e
	}
	if n.app != nil {
		if e := n.app.sliderSetNative(n, value); e != nil {
			return e
		}
	}
	n.slider.confirmed = value
	return nil
}
func (n *Node) SliderValue() (SliderValue, error) {
	if e := n.sliderCheck(); e != nil {
		return SliderValue{}, e
	}
	if n.app == nil {
		return n.slider.confirmed, nil
	}
	return n.app.sliderGetNative(n)
}
func (n *Node) SliderConfig() (SliderConfig, error) {
	if e := n.sliderCheck(); e != nil {
		return SliderConfig{}, e
	}
	if n.app == nil {
		return n.slider.config, nil
	}
	return n.app.sliderConfigNative(n)
}
func (n *Node) SliderPartBounds(part SliderPart) (Rect, bool, error) {
	if e := n.sliderCheck(); e != nil {
		return Rect{}, false, e
	}
	if n.app == nil {
		return Rect{}, false, errors.New("Slider not mounted")
	}
	if part > SliderEndThumb {
		return Rect{}, false, errors.New("invalid Slider part")
	}
	return n.app.sliderBoundsNative(n, part)
}
func (n *Node) SliderPresentation() (SliderPresentation, bool, error) {
	if e := n.sliderCheck(); e != nil {
		return SliderPresentation{}, false, e
	}
	if n.app == nil {
		return SliderPresentation{}, false, errors.New("Slider not mounted")
	}
	return n.app.sliderPresentationNative(n)
}

// DispatchSliderRequests pops/copies once, invokes outside C, and never accepts implicitly.
func (a *App) DispatchSliderRequests() (count int, err error) {
	if e := a.check(); e != nil {
		return 0, e
	}
	if a.sliderDispatching {
		return 0, errors.New("nested controlled Slider dispatch")
	}
	a.sliderDispatching = true
	defer func() { a.sliderDispatching = false }()
	for !a.closed && !a.sliderClosing {
		request, ok, e := a.PollSliderRequest()
		if e != nil {
			return count, e
		}
		if !ok {
			return count, nil
		}
		count++
		info := request.Receipt.info
		n := a.nodes[uint64(info.Slot)<<32|uint64(info.Generation)]
		if n != nil && n.slider != nil && n.slider.callback != nil {
			callback := n.slider.callback
			if e = callback(request); e != nil {
				return count, e
			}
		}
	}
	return count, nil
}
func (a *App) sliderPreflight(root *Node) error {
	typed := false
	var walk func(*Node, bool) error
	walk = func(n *Node, ancestors bool) error {
		if n.kind == 81 {
			if !ancestors {
				return errors.New("controlled Slider requires Row/Column ancestry")
			}
			if e := n.sliderCheck(); e != nil {
				return e
			}
			typed = true
		}
		for _, child := range n.children {
			if e := walk(child, ancestors && (n.kind == 1 || n.kind == 2)); e != nil {
				return e
			}
		}
		return nil
	}
	if e := walk(root, true); e != nil {
		return e
	}
	if typed {
		supported, e := a.SupportsSliderModel()
		if e != nil {
			return e
		}
		if !supported {
			return errors.New("controlled Slider API unavailable")
		}
		if a.sliderNamespace == nil {
			return a.EnableSliderRequests(4096)
		}
	}
	return nil
}
