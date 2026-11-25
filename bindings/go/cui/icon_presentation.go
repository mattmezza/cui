// SPDX-License-Identifier: MIT
package cui

/*
#include "icon_badge_ffi.h"
*/
import "C"

import "errors"

// ErrIconPresentationUnavailable indicates that the loaded provider lacks the
// complete retained Icon and owned SVG groups.
var ErrIconPresentationUnavailable = errors.New("retained Icon presentation V1 is unavailable")

// IconSize selects the component's preset size. Set CustomSize for IconCustom.
type IconSize uint32

const (
	IconInherited IconSize = iota
	IconExtraSmall
	IconSmall
	IconMedium
	IconLarge
	IconCustom
)

// IconAlign aligns the artwork's box content.
type IconAlign uint32

const (
	IconAlignStart IconAlign = iota
	IconAlignCenter
	IconAlignEnd
)

// IconBoxStyle configures the Icon's box. CornerRadii uses TL, TR, BL, BR
// order. BorderEdges and PaddingEdges use left, right, top, bottom order.
type IconBoxStyle struct {
	Width, Height, MaxWidth, MaxHeight                             float32
	Padding, Gap, Radius, BorderWidth                              float32
	OffsetX, OffsetY, Opacity                                      float32
	BackgroundRGBA, BorderRGBA                                     uint32
	AlignX, AlignY                                                 IconAlign
	CornerRadii, BorderEdges, PaddingEdges                         [4]float32
	Row, Clip, FixedWidth, FixedHeight, LayoutOnly, Inert          bool
	WidthPercent, HeightPercent                                    bool
	CustomCorners, CustomBorderEdges, CustomPadding, CustomOpacity bool
}

// IconConstraints sets optional border-box constraints in logical pixels.
// The corresponding Has flag distinguishes explicit zero from inheritance.
type IconConstraints struct {
	MinWidth, MinHeight, MaxWidth, MaxHeight             float32
	HasMinWidth, HasMinHeight, HasMaxWidth, HasMaxHeight bool
}

// IconOptions is a value-only presentation. Has flags distinguish explicit
// zero from inheritance. Colors are numeric 0xRRGGBBAA. Layout is in logical
// pixels; transforms affect only paint.
type IconOptions struct {
	Size                                                         IconSize
	DesignSize                                                   DesignSize
	CustomSize, InheritedTextSize, RemSize                       float32
	Width, Height, Opacity                                       float32
	ColorRGBA, InheritedColorRGBA                                uint32
	Transform                                                    IconTransform
	Style                                                        IconBoxStyle
	Constraints                                                  IconConstraints
	HasWidth, HasHeight, HasOpacity, HasColor, HasInheritedColor bool
	HasTransform, HasStyle, HasConstraints                       bool
	Decorative, DebugPlaceholder                                 bool
}

// IconConfig associates copied presentation options and a semantic name with
// one app-owned SVG. Its resource may also be used by other nodes.
type IconConfig struct {
	Resource *IconResource
	Options  IconOptions
	Name     string
}

// IconResult is the latest invocation for a configured Icon. FrameStatus is
// the whole-frame C bridge status (zero on success) independently of artwork.
type IconResult struct {
	FrameSequence uint64
	FrameStatus   uint32
	IconError     IconError
}

func iconFlag(flags *C.uint32_t, enabled bool, flag C.uint32_t) {
	if enabled {
		*flags |= flag
	}
}

func iconExtent(x float32) bool { return x >= 0 && x <= 4096 }

func iconOptionalExtent(x float32, has bool) bool {
	if has {
		return iconExtent(x)
	}
	return x == 0
}

func (s IconBoxStyle) wire() (C.cui_bridge_icon_style_v1, error) {
	for _, x := range []float32{s.Width, s.Height, s.Opacity, s.OffsetX, s.OffsetY} {
		if !finite(x) {
			return C.cui_bridge_icon_style_v1{}, errors.New("Icon box requires finite dimensions, opacity and offsets")
		}
	}
	for _, x := range []float32{s.Padding, s.Gap, s.Radius, s.BorderWidth, s.MaxWidth, s.MaxHeight} {
		if !finite(x) || x < 0 {
			return C.cui_bridge_icon_style_v1{}, errors.New("invalid Icon box size")
		}
	}
	if s.AlignX > IconAlignEnd || s.AlignY > IconAlignEnd {
		return C.cui_bridge_icon_style_v1{}, errors.New("invalid Icon box alignment")
	}
	wire := C.cui_bridge_icon_style_v1{
		struct_size: C.sizeof_cui_bridge_icon_style_v1, schema_version: 1,
		width: C.float(s.Width), height: C.float(s.Height), padding: C.float(s.Padding),
		gap: C.float(s.Gap), radius: C.float(s.Radius), border_width: C.float(s.BorderWidth),
		background_rgba: C.uint32_t(s.BackgroundRGBA), border_rgba: C.uint32_t(s.BorderRGBA),
		align_x: C.uint32_t(s.AlignX), align_y: C.uint32_t(s.AlignY),
		offset_x: C.float(s.OffsetX), offset_y: C.float(s.OffsetY), opacity: C.float(s.Opacity),
		max_width: C.float(s.MaxWidth), max_height: C.float(s.MaxHeight),
	}
	for i := range s.CornerRadii {
		corner, border, padding := s.CornerRadii[i], s.BorderEdges[i], s.PaddingEdges[i]
		if !finite(corner) || corner < 0 || !finite(border) || border < 0 || !finite(padding) || padding < 0 {
			return C.cui_bridge_icon_style_v1{}, errors.New("invalid Icon box corner or edge")
		}
		wire.corners[i], wire.border_edges[i], wire.padding_edges[i] = C.float(corner), C.float(border), C.float(padding)
	}
	iconFlag(&wire.flags, s.Row, C.CUI_BRIDGE_ICON_BOX_ROW_V1)
	iconFlag(&wire.flags, s.Clip, C.CUI_BRIDGE_ICON_BOX_CLIP_V1)
	iconFlag(&wire.flags, s.CustomCorners, C.CUI_BRIDGE_ICON_BOX_CORNERS_V1)
	iconFlag(&wire.flags, s.CustomBorderEdges, C.CUI_BRIDGE_ICON_BOX_BORDER_EDGES_V1)
	iconFlag(&wire.flags, s.FixedWidth, C.CUI_BRIDGE_ICON_BOX_FIXED_WIDTH_V1)
	iconFlag(&wire.flags, s.FixedHeight, C.CUI_BRIDGE_ICON_BOX_FIXED_HEIGHT_V1)
	iconFlag(&wire.flags, s.LayoutOnly, C.CUI_BRIDGE_ICON_BOX_LAYOUT_ONLY_V1)
	iconFlag(&wire.flags, s.Inert, C.CUI_BRIDGE_ICON_BOX_INERT_V1)
	iconFlag(&wire.flags, s.CustomOpacity, C.CUI_BRIDGE_ICON_BOX_OPACITY_V1)
	iconFlag(&wire.flags, s.WidthPercent, C.CUI_BRIDGE_ICON_BOX_WIDTH_PERCENT_V1)
	iconFlag(&wire.flags, s.HeightPercent, C.CUI_BRIDGE_ICON_BOX_HEIGHT_PERCENT_V1)
	iconFlag(&wire.flags, s.CustomPadding, C.CUI_BRIDGE_ICON_BOX_PADDING_EDGES_V1)
	return wire, nil
}

func (c IconConstraints) wire() (C.cui_bridge_icon_constraints_v1, error) {
	if !iconOptionalExtent(c.MinWidth, c.HasMinWidth) || !iconOptionalExtent(c.MinHeight, c.HasMinHeight) ||
		!iconOptionalExtent(c.MaxWidth, c.HasMaxWidth) || !iconOptionalExtent(c.MaxHeight, c.HasMaxHeight) {
		return C.cui_bridge_icon_constraints_v1{}, errors.New("invalid Icon constraints")
	}
	wire := C.cui_bridge_icon_constraints_v1{
		struct_size: C.sizeof_cui_bridge_icon_constraints_v1, schema_version: 1,
		min_width: C.float(c.MinWidth), min_height: C.float(c.MinHeight),
		max_width: C.float(c.MaxWidth), max_height: C.float(c.MaxHeight),
	}
	iconFlag(&wire.flags, c.HasMinWidth, C.CUI_BRIDGE_ICON_MIN_WIDTH_V1)
	iconFlag(&wire.flags, c.HasMinHeight, C.CUI_BRIDGE_ICON_MIN_HEIGHT_V1)
	iconFlag(&wire.flags, c.HasMaxWidth, C.CUI_BRIDGE_ICON_MAX_WIDTH_V1)
	iconFlag(&wire.flags, c.HasMaxHeight, C.CUI_BRIDGE_ICON_MAX_HEIGHT_V1)
	return wire, nil
}

func iconOptionsHeader() C.cui_bridge_icon_options_v1 {
	return C.cui_bridge_icon_options_v1{struct_size: C.sizeof_cui_bridge_icon_options_v1, schema_version: 1}
}

func (o IconOptions) wire() (C.cui_bridge_icon_options_v1, error) {
	wire := iconOptionsHeader()
	if o.Size > IconCustom || o.DesignSize < Inherit || o.DesignSize > Size9XL ||
		!iconExtent(o.CustomSize) || !iconExtent(o.InheritedTextSize) || !iconExtent(o.RemSize) ||
		!iconOptionalExtent(o.Width, o.HasWidth) || !iconOptionalExtent(o.Height, o.HasHeight) ||
		!finite(o.Opacity) || (!o.HasOpacity && o.Opacity != 0) ||
		(!o.HasColor && o.ColorRGBA != 0) || (!o.HasInheritedColor && o.InheritedColorRGBA != 0) {
		return wire, errors.New("invalid Icon size, opacity or color options")
	}
	for i, f := range o.Transform {
		bound := float32(64)
		if i >= 4 {
			bound = 1e6
		}
		if !(f >= -bound && f <= bound) || (!o.HasTransform && f != 0) {
			return wire, errors.New("invalid Icon presentation transform")
		}
		wire.transform[i] = C.float(f)
	}
	if o.HasStyle {
		style, err := o.Style.wire()
		if err != nil {
			return wire, err
		}
		wire.style = style
	} else if o.Style != (IconBoxStyle{}) {
		return wire, errors.New("Icon box style requires HasStyle")
	}
	if o.HasConstraints {
		constraints, err := o.Constraints.wire()
		if err != nil {
			return wire, err
		}
		wire.constraints = constraints
	} else if o.Constraints != (IconConstraints{}) {
		return wire, errors.New("Icon constraints require HasConstraints")
	}
	wire.size, wire.design_size = C.uint32_t(o.Size), C.uint32_t(o.DesignSize)
	wire.custom_size, wire.rem_size = C.float(o.CustomSize), C.float(o.RemSize)
	wire.inherited_text_size = C.float(o.InheritedTextSize)
	wire.width, wire.height, wire.opacity = C.float(o.Width), C.float(o.Height), C.float(o.Opacity)
	wire.color_rgba, wire.inherited_color_rgba = C.uint32_t(o.ColorRGBA), C.uint32_t(o.InheritedColorRGBA)
	iconFlag(&wire.flags, o.HasWidth, C.CUI_BRIDGE_ICON_WIDTH_V1)
	iconFlag(&wire.flags, o.HasHeight, C.CUI_BRIDGE_ICON_HEIGHT_V1)
	iconFlag(&wire.flags, o.HasColor, C.CUI_BRIDGE_ICON_COLOR_V1)
	iconFlag(&wire.flags, o.HasInheritedColor, C.CUI_BRIDGE_ICON_INHERITED_COLOR_V1)
	iconFlag(&wire.flags, o.HasOpacity, C.CUI_BRIDGE_ICON_OPACITY_V1)
	iconFlag(&wire.flags, o.HasTransform, C.CUI_BRIDGE_ICON_TRANSFORM_V1)
	iconFlag(&wire.flags, o.Decorative, C.CUI_BRIDGE_ICON_DECORATIVE_V1)
	iconFlag(&wire.flags, o.HasStyle, C.CUI_BRIDGE_ICON_STYLE_V1)
	iconFlag(&wire.flags, o.HasConstraints, C.CUI_BRIDGE_ICON_CONSTRAINTS_V1)
	iconFlag(&wire.flags, o.DebugPlaceholder, C.CUI_BRIDGE_ICON_DEBUG_PLACEHOLDER_V1)
	return wire, nil
}

// SupportsIconPresentation reports whether this app can mount configured Icons.
func (a *App) SupportsIconPresentation() (bool, error) {
	supported, err := a.SupportsIconResources()
	return supported && C.cui_go_icon_presentation_supported() != 0, err
}

func (r *IconResource) handleFor(app *App) (C.cui_bridge_icon_handle_v1, error) {
	if r == nil || r.owner == nil {
		return C.cui_bridge_icon_handle_v1{}, errors.New("Icon resource is required")
	}
	if app != nil && r.owner.app != app {
		return C.cui_bridge_icon_handle_v1{}, errors.New("Icon resource belongs to another app")
	}
	owner, err := r.live()
	if err != nil {
		return C.cui_bridge_icon_handle_v1{}, err
	}
	return owner.handle, nil
}

// Isolate the Go wrapper while sharing its single native owner and handle.
func (r *IconResource) alias() *IconResource {
	if r == nil {
		return nil
	}
	copy := *r
	return &copy
}

// ConfiguredIcon creates a retained Icon backed by an app-owned SVG. To keep a
// named fallback for ResetIcon, configure an ordinary Icon node instead.
func ConfiguredIcon(key string, config IconConfig) *Node {
	n := Icon(key, "")
	n.err = n.ConfigureIcon(config)
	return n
}

func (n *Node) checkIcon() error {
	if n == nil || n.kind != int(C.CUI_BRIDGE_ICON) || n.removed {
		return errors.New("configuration requires a live Icon node")
	}
	if n.app != nil {
		return n.app.check()
	}
	return nil
}

// ConfigureIcon replaces presentation atomically. Resource and node must use
// the same app. A failed replacement preserves the previous configuration.
func (n *Node) ConfigureIcon(config IconConfig) error {
	if err := n.checkIcon(); err != nil {
		return err
	}
	if err := valid(config.Name); err != nil {
		return err
	}
	if n.app != nil {
		if err := n.setNativeIcon(config); err != nil {
			return err
		}
	} else {
		if _, err := config.Options.wire(); err != nil {
			return err
		}
		if _, err := config.Resource.handleFor(nil); err != nil {
			return err
		}
	}
	config.Resource = config.Resource.alias()
	n.iconConfig = &config
	return nil
}

func (a *App) iconPresentationHandle(config IconConfig) (C.cui_bridge_icon_handle_v1, error) {
	supported, err := a.SupportsIconPresentation()
	if err != nil {
		return C.cui_bridge_icon_handle_v1{}, err
	}
	if !supported {
		return C.cui_bridge_icon_handle_v1{}, ErrIconPresentationUnavailable
	}
	return config.Resource.handleFor(a)
}

func (n *Node) setNativeIcon(config IconConfig) error {
	handle, err := n.app.iconPresentationHandle(config)
	if err != nil {
		return err
	}
	wire, err := config.Options.wire()
	if err != nil {
		return err
	}
	return result(C.cui_go_icon_configure_node_v1(n.app.ptr, n.handle, handle, &wire,
		iconSpan(config.Name), C.uint32_t(len(config.Name))))
}

// ResetIcon clears the current presentation and returns to named Icon drawing.
func (n *Node) ResetIcon() error {
	if err := n.checkIcon(); err != nil {
		return err
	}
	if n.app != nil && n.iconConfig != nil {
		if err := result(C.cui_go_icon_reset_node_v1(n.app.ptr, n.handle)); err != nil {
			return err
		}
	}
	n.iconConfig = nil
	return nil
}

// IconConfig reads a copied presentation. The bool is false for a legacy or
// reset node. The resource itself remains app-owned.
func (n *Node) IconConfig() (IconConfig, bool, error) {
	if err := n.checkIcon(); err != nil {
		return IconConfig{}, false, err
	}
	if n.iconConfig == nil {
		return IconConfig{}, false, nil
	}
	config := *n.iconConfig
	config.Resource = config.Resource.alias()
	if n.app == nil {
		return config, true, nil
	}
	wire := iconOptionsHeader()
	var handle C.cui_bridge_icon_handle_v1
	if err := result(C.cui_go_icon_get_node_config_v1(n.app.ptr, n.handle, &handle, &wire)); err != nil {
		return IconConfig{}, false, err
	}
	expected, err := config.Options.wire()
	if err != nil {
		return IconConfig{}, false, err
	}
	name, _, err := iconCopyString(iconTextMax, false,
		func(out *C.char, capacity C.uint32_t, required *C.uint32_t) C.cui_bridge_status {
			return C.cui_go_icon_copy_node_name_v1(n.app.ptr, n.handle, out, capacity, required)
		})
	if err != nil {
		return IconConfig{}, false, err
	}
	if handle != config.Resource.owner.handle || wire != expected || name != config.Name {
		return IconConfig{}, false, errors.New("invalid copied Icon configuration")
	}
	return config, true, nil
}

// IconResult reports an Icon observed in the latest admitted frame. Reading it
// never invokes drawing, rasterization or callbacks.
func (n *Node) IconResult() (IconResult, bool, error) {
	if err := n.checkIcon(); err != nil {
		return IconResult{}, false, err
	}
	if n.app == nil || n.iconConfig == nil {
		return IconResult{}, false, nil
	}
	wire := C.cui_bridge_icon_result_v1{struct_size: C.sizeof_cui_bridge_icon_result_v1, schema_version: 1}
	code := C.cui_go_icon_get_node_result_v1(n.app.ptr, n.handle, &wire)
	if code == C.CUI_BRIDGE_EMPTY {
		return IconResult{}, false, nil
	}
	if err := result(code); err != nil {
		return IconResult{}, false, err
	}
	if wire.struct_size != C.sizeof_cui_bridge_icon_result_v1 || wire.schema_version != 1 ||
		wire.node != n.handle || wire.frame_sequence == 0 || wire.reserved != 0 ||
		wire.frame_status > C.CUI_BRIDGE_CORE_ERROR || IconError(wire.icon_error) > IconAllocation {
		return IconResult{}, false, errors.New("invalid copied Icon result")
	}
	return IconResult{FrameSequence: uint64(wire.frame_sequence), FrameStatus: uint32(wire.frame_status),
		IconError: IconError(wire.icon_error)}, true, nil
}
