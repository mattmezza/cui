package cui

import "errors"

// ShadowPart selects a declared paint surface, never a raw paint identity.
type ShadowPart uint32

const (
	ShadowRoot ShadowPart = iota
	ShadowMain
	ShadowIndicator
	ShadowTrack
	ShadowThumb
)

// ShadowTargetInfo reports declared owner capability, not current-frame presence.
type ShadowTargetInfo struct {
	CanonicalPart ShadowPart
	Flags         uint32
}

func (i ShadowTargetInfo) Supported() bool { return i.Flags&1 != 0 }
func canonicalPart(kind int, part ShadowPart) (ShadowPart, error) {
	main := ShadowRoot
	supported := part == ShadowRoot
	switch {
	case shadowKind(kind):
		supported = part == ShadowRoot || part == ShadowMain
	case kind == 5 || kind == 6 || kind == 20:
		main = ShadowIndicator
		supported = part <= ShadowIndicator
	case kind == 7:
		main = ShadowTrack
		supported = part == ShadowRoot || part == ShadowMain || part == ShadowTrack || part == ShadowThumb
	case kind == 9:
		main = ShadowTrack
		supported = part == ShadowRoot || part == ShadowMain || part == ShadowTrack
	case kind == 17:
		supported = part == ShadowRoot
	default:
		supported = false
	}
	if !supported {
		return 0, errors.New("unsupported shadow part")
	}
	if part == ShadowMain {
		return main, nil
	}
	return part, nil
}

// PartShadows owns a copied list; empty suppresses defaults, reset restores them.
// A generic ITEM requires a RadioGroup parent at mount; other owner paths reject.
func (n *Node) PartShadows(part ShadowPart, style ShadowStyle) *Node {
	if n.err != nil {
		return n
	}
	role, e := canonicalPart(n.kind, part)
	if e != nil {
		n.err = e
		return n
	}
	if e = n.setPartNative(part, style); e != nil {
		n.err = e
		return n
	}
	copied := style
	copied.Layers = append([]BoxShadow(nil), style.Layers...)
	if n.partShadows == nil {
		n.partShadows = make(map[ShadowPart]ShadowStyle)
	}
	n.partShadows[role] = copied
	if role == ShadowRoot {
		n.shadowStyle = nil
	}
	return n
}
func (n *Node) ResetPartShadows(part ShadowPart) *Node {
	if n.err != nil {
		return n
	}
	role, e := canonicalPart(n.kind, part)
	if e != nil {
		n.err = e
		return n
	}
	if e = n.resetPartNative(part); e != nil {
		n.err = e
		return n
	}
	delete(n.partShadows, role)
	if role == ShadowRoot {
		n.shadowStyle = nil
	}
	return n
}

// PartShadowPreset requires a mounted node so canonical factories use its App.
func (n *Node) PartShadowPreset(part ShadowPart, preset ShadowPreset, radii CornerRadii) *Node {
	if n.err != nil {
		return n
	}
	if n.app == nil {
		n.err = errors.New("preset requires owning mounted App; use app.ShadowPreset for unmounted values")
		return n
	}
	style, e := n.app.ShadowPreset(preset, radii)
	if e != nil {
		n.err = e
		return n
	}
	return n.PartShadows(part, style)
}
