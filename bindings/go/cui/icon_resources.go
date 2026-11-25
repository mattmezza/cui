// SPDX-License-Identifier: MIT
package cui

/*
#include "icon_resources_ffi.h"
*/
import "C"

import (
	"errors"
	"math"
	"strings"
	"unicode/utf8"
	"unsafe"
)

const (
	iconTextMax   = 4096
	iconSVGMax    = 1 << 20
	iconPixelMax  = 16 << 20
	iconBitmapMax = 64 << 20
)

// ErrIconResourcesUnavailable describes a bridge provider without the complete
// supported owned SVG resource group. Legacy named Icons remain available.
var ErrIconResourcesUnavailable = errors.New("owned Icon resources V1 are unavailable")

// IconSourceKind describes where an app-owned resource gets its SVG.
type IconSourceKind uint32

const (
	IconSourceEmpty IconSourceKind = iota
	IconSourceName
	IconSourcePath
	IconSourceData
)

// IconSource selects empty artwork, an embedded catalog name, a local asset
// key, or raw strict UTF-8 SVG. The bridge copies Text when assigned.
type IconSource struct {
	Kind IconSourceKind
	Text string
}

func IconName(name string) IconSource { return IconSource{IconSourceName, name} }
func IconPath(path string) IconSource { return IconSource{IconSourcePath, path} }
func IconSVG(svg string) IconSource   { return IconSource{IconSourceData, svg} }

// IconLimits controls SVG and raster cache admission. Each zero uses the
// corresponding C default. Limits() reports the normalized values.
type IconLimits struct {
	MaxSVGBytes, MaxPixels, MaxCacheBytes, MaxVersions uint64
}

// IconError describes artwork preparation independently of bridge status.
type IconError uint32

const (
	IconOK IconError = iota
	IconInvalid
	IconNotFound
	IconParse
	IconLimit
	IconAllocation
)

// IconAvailability is the loaded C provider's answer to a catalog lookup.
type IconAvailability uint32

const (
	IconUnknown IconAvailability = iota
	IconCatalogNotEmbedded
	IconEmbedded
)

// IconCatalogEntry is a copy of one catalog entry.
type IconCatalogEntry struct {
	Name, Path      string
	DefaultEmbedded bool
}

// IconInfo is a snapshot of cached resource state and active widget references.
type IconInfo struct {
	Error                     IconError
	References                uint32
	ParseCount, RasterCount   uint64
	CacheBytes, CacheVersions uint64
	SourceRevision            uint64
	SourceKind                IconSourceKind
}

// IconBitmap owns a copy of packed straight RGBA8 coverage pixels.
type IconBitmap struct {
	Width, Height, Stride uint32
	Revision              uint64
	Paint                 Rect
	RGBA                  []byte
}

// IconTransform stores affine coefficients in a, b, c, d, tx, ty order.
type IconTransform [6]float32

// IconRotation constructs an affine rotation about the Icon's origin.
func IconRotation(radians float64) IconTransform {
	sin, cos := math.Sincos(radians)
	return IconTransform{float32(cos), float32(sin), -float32(sin), float32(cos), 0, 0}
}

type iconOwner struct {
	app    *App
	handle C.cui_bridge_icon_handle_v1
}

// IconResource is an app-owned SVG. Use it on the app's owner goroutine and
// call Close for early release. Copying the Go value shares the same ownership.
type IconResource struct {
	owner *iconOwner
}

func iconTextValid(text string, max int) bool {
	return len(text) <= max && utf8.ValidString(text) && strings.IndexByte(text, 0) < 0
}

func (s IconSource) validate() error {
	max := iconTextMax
	if s.Kind == IconSourceData {
		max = iconSVGMax
	}
	if s.Kind > IconSourceData || (s.Kind == IconSourceEmpty && s.Text != "") ||
		!iconTextValid(s.Text, max) {
		return errors.New("invalid Icon source: use bounded UTF-8 without NUL")
	}
	return nil
}

// The bridge borrows these addresses only during the call. The Go allocations
// have no embedded pointers and remain pinned until the cgo call returns.
func iconSpan(s string) *C.char  { return (*C.char)(unsafe.Pointer(unsafe.StringData(s))) }
func iconBytes(b []byte) *C.char { return (*C.char)(unsafe.Pointer(unsafe.SliceData(b))) }

func (limits IconLimits) valid() bool {
	return limits.MaxSVGBytes <= iconSVGMax && limits.MaxPixels <= iconPixelMax &&
		limits.MaxCacheBytes <= iconBitmapMax && limits.MaxVersions <= 128
}

func iconLimitsRecord(limits IconLimits) C.cui_bridge_icon_limits_v1 {
	return C.cui_bridge_icon_limits_v1{
		struct_size: C.sizeof_cui_bridge_icon_limits_v1, schema_version: 1,
		max_svg_bytes: C.uint64_t(limits.MaxSVGBytes), max_pixels: C.uint64_t(limits.MaxPixels),
		max_cache_bytes: C.uint64_t(limits.MaxCacheBytes), max_versions: C.uint64_t(limits.MaxVersions),
	}
}

func iconReadLimits(record C.cui_bridge_icon_limits_v1) (IconLimits, error) {
	config := IconLimits{
		MaxSVGBytes: uint64(record.max_svg_bytes), MaxPixels: uint64(record.max_pixels),
		MaxCacheBytes: uint64(record.max_cache_bytes), MaxVersions: uint64(record.max_versions),
	}
	if record.struct_size != C.sizeof_cui_bridge_icon_limits_v1 || record.schema_version != 1 ||
		record.reserved[0] != 0 || record.reserved[1] != 0 || !config.valid() ||
		config.MaxSVGBytes == 0 || config.MaxPixels == 0 || config.MaxCacheBytes == 0 || config.MaxVersions == 0 {
		return IconLimits{}, errors.New("invalid copied Icon limits")
	}
	return config, nil
}

// SupportsIconResources checks the active provider without creating resources.
func (a *App) SupportsIconResources() (bool, error) {
	if err := a.check(); err != nil {
		return false, err
	}
	return C.cui_go_icon_resources_supported() != 0, nil
}

func (a *App) requireIconResources() error {
	supported, err := a.SupportsIconResources()
	if err == nil && !supported {
		return ErrIconResourcesUnavailable
	}
	return err
}

// DefaultIconLimits returns the provider's actual resource defaults.
func (a *App) DefaultIconLimits() (IconLimits, error) {
	if err := a.requireIconResources(); err != nil {
		return IconLimits{}, err
	}
	record := iconLimitsRecord(IconLimits{})
	if err := result(C.cui_go_icon_default_limits_v1(&record)); err != nil {
		return IconLimits{}, err
	}
	return iconReadLimits(record)
}

// NewIconResource creates an owned SVG with optional native limits. Empty
// options use native defaults. The supplied source is copied before returning.
func (a *App) NewIconResource(source IconSource, limits IconLimits) (*IconResource, error) {
	if err := a.requireIconResources(); err != nil {
		return nil, err
	}
	if err := source.validate(); err != nil {
		return nil, err
	}
	if !limits.valid() {
		return nil, errors.New("Icon limits exceed bridge caps")
	}
	record := iconLimitsRecord(limits)
	var handle C.cui_bridge_icon_handle_v1
	if err := result(C.cui_go_icon_create_v1(a.ptr, &record, &handle)); err != nil {
		return nil, err
	}
	if handle.slot == 0 || handle.generation == 0 {
		return nil, errors.New("invalid created Icon handle")
	}
	resource := &IconResource{owner: &iconOwner{app: a, handle: handle}}
	if source.Kind != IconSourceEmpty {
		if err := resource.SetSource(source); err != nil {
			return nil, errors.Join(err, resource.Close())
		}
	}
	return resource, nil
}

func (r *IconResource) live() (*iconOwner, error) {
	if r == nil || r.owner == nil {
		return nil, errors.New("invalid Icon resource")
	}
	owner := r.owner
	if err := owner.app.check(); err != nil {
		return nil, err
	}
	if owner.handle.generation == 0 {
		return nil, errors.New("Icon resource is closed")
	}
	return owner, nil
}

// Close releases an unreferenced resource. The call is idempotent. The
// resource remains usable when C reports that a widget still references it.
func (r *IconResource) Close() error {
	if r == nil || r.owner == nil {
		return nil
	}
	owner := r.owner
	if err := owner.app.check(); err != nil {
		if errors.Is(err, ErrClosed) {
			return nil
		}
		return err
	}
	if owner.handle.generation == 0 {
		return nil
	}
	if err := result(C.cui_go_icon_destroy_v1(owner.app.ptr, owner.handle)); err != nil {
		return err
	}
	owner.handle = C.cui_bridge_icon_handle_v1{}
	return nil
}

// SetSource replaces the resource's source. Failed updates preserve the
// previous source and any already prepared mask.
func (r *IconResource) SetSource(source IconSource) error {
	owner, err := r.live()
	if err != nil {
		return err
	}
	if err := source.validate(); err != nil {
		return err
	}
	return result(C.cui_go_icon_set_source_v1(owner.app.ptr, owner.handle, C.uint32_t(source.Kind),
		iconSpan(source.Text), C.uint32_t(len(source.Text))))
}

func iconReadString(buffer []byte, size uint32) (string, error) {
	if size == 0 || uint64(size) > uint64(len(buffer)) || buffer[size-1] != 0 {
		return "", errors.New("invalid copied Icon string length")
	}
	text := string(buffer[:size-1])
	if !iconTextValid(text, len(buffer)-1) {
		return "", errors.New("invalid copied Icon string")
	}
	return text, nil
}

type iconStringReader func(*C.char, C.uint32_t, *C.uint32_t) C.cui_bridge_status

func iconCopyString(max int, allowEmpty bool, read iconStringReader) (string, bool, error) {
	var required C.uint32_t
	code := read(nil, 0, &required)
	if allowEmpty && code == C.CUI_BRIDGE_EMPTY {
		return "", false, nil
	}
	if code != C.CUI_BRIDGE_CAPACITY || required == 0 || required > C.uint32_t(max+1) {
		if code != C.CUI_BRIDGE_CAPACITY && code != C.CUI_BRIDGE_OK {
			return "", false, result(code)
		}
		return "", false, errors.New("invalid Icon string size response")
	}
	buffer := make([]byte, int(required))
	code = read(iconBytes(buffer), required, &required)
	if err := result(code); err != nil {
		return "", false, err
	}
	if int(required) != len(buffer) {
		return "", false, errors.New("Icon string size changed during copy")
	}
	text, err := iconReadString(buffer, uint32(required))
	return text, err == nil, err
}

// Source returns the bridge's copied source descriptor.
func (r *IconResource) Source() (IconSource, error) {
	owner, err := r.live()
	if err != nil {
		return IconSource{}, err
	}
	var kind C.uint32_t
	text, _, err := iconCopyString(iconSVGMax, false,
		func(out *C.char, cap C.uint32_t, required *C.uint32_t) C.cui_bridge_status {
			return C.cui_go_icon_get_source_v1(owner.app.ptr, owner.handle, &kind, out, cap, required)
		})
	if err != nil {
		return IconSource{}, err
	}
	source := IconSource{Kind: IconSourceKind(kind), Text: text}
	if source.validate() != nil {
		return IconSource{}, errors.New("invalid copied Icon source")
	}
	return source, nil
}

// Info reports the current source, cache and reference state.
func (r *IconResource) Info() (IconInfo, error) {
	owner, err := r.live()
	if err != nil {
		return IconInfo{}, err
	}
	record := C.cui_bridge_icon_info_v1{struct_size: C.sizeof_cui_bridge_icon_info_v1, schema_version: 1}
	if err := result(C.cui_go_icon_get_info_v1(owner.app.ptr, owner.handle, &record)); err != nil {
		return IconInfo{}, err
	}
	if record.struct_size != C.sizeof_cui_bridge_icon_info_v1 || record.schema_version != 1 ||
		record.handle != owner.handle || record.flags != 0 || record.reserved != 0 ||
		record.source_revision == 0 || IconSourceKind(record.source_kind) > IconSourceData ||
		IconError(record.icon_error) > IconAllocation {
		return IconInfo{}, errors.New("invalid copied Icon info")
	}
	return IconInfo{Error: IconError(record.icon_error), References: uint32(record.reference_count),
		ParseCount: uint64(record.parse_count), RasterCount: uint64(record.raster_count),
		CacheBytes: uint64(record.cache_bytes), CacheVersions: uint64(record.cache_versions),
		SourceRevision: uint64(record.source_revision), SourceKind: IconSourceKind(record.source_kind)}, nil
}

// Limits returns normalized, copied source and cache limits.
func (r *IconResource) Limits() (IconLimits, error) {
	owner, err := r.live()
	if err != nil {
		return IconLimits{}, err
	}
	record := iconLimitsRecord(IconLimits{})
	if err := result(C.cui_go_icon_get_limits_v1(owner.app.ptr, owner.handle, &record)); err != nil {
		return IconLimits{}, err
	}
	return iconReadLimits(record)
}

// SetLimits changes the native admission limits. Zero selects defaults.
func (r *IconResource) SetLimits(limits IconLimits) error {
	owner, err := r.live()
	if err != nil {
		return err
	}
	if !limits.valid() {
		return errors.New("Icon limits exceed bridge caps")
	}
	record := iconLimitsRecord(limits)
	return result(C.cui_go_icon_set_limits_v1(owner.app.ptr, owner.handle, &record))
}

// PutAsset copies SVG into the resource-local path map.
func (r *IconResource) PutAsset(path, svg string) error {
	owner, err := r.live()
	if err != nil {
		return err
	}
	if !iconTextValid(path, iconTextMax) || !iconTextValid(svg, iconSVGMax) {
		return errors.New("invalid Icon asset: use bounded UTF-8 without NUL")
	}
	return result(C.cui_go_icon_asset_put_v1(owner.app.ptr, owner.handle,
		iconSpan(path), C.uint32_t(len(path)), iconSpan(svg), C.uint32_t(len(svg))))
}

// RemoveAsset removes one copied path entry and invalidates preparation.
func (r *IconResource) RemoveAsset(path string) error {
	owner, err := r.live()
	if err != nil {
		return err
	}
	if !iconTextValid(path, iconTextMax) {
		return errors.New("invalid Icon asset path")
	}
	return result(C.cui_go_icon_asset_remove_v1(owner.app.ptr, owner.handle, iconSpan(path), C.uint32_t(len(path))))
}

// ClearAssets removes the resource-local overrides.
func (r *IconResource) ClearAssets() error {
	owner, err := r.live()
	if err != nil {
		return err
	}
	return result(C.cui_go_icon_asset_clear_v1(owner.app.ptr, owner.handle))
}

// Reload invalidates preparation while retaining the source.
func (r *IconResource) Reload() error {
	owner, err := r.live()
	if err != nil {
		return err
	}
	return result(C.cui_go_icon_reload_v1(owner.app.ptr, owner.handle))
}

// Collect releases inactive masks after synchronous frame consumption.
func (r *IconResource) Collect() error {
	owner, err := r.live()
	if err != nil {
		return err
	}
	return result(C.cui_go_icon_collect_v1(owner.app.ptr, owner.handle))
}

// Prepare rasterizes at logical dimensions, with an optional affine transform.
// A nil error still requires checking the returned artwork status.
func (r *IconResource) Prepare(width, height float32, transform ...IconTransform) (IconError, error) {
	owner, err := r.live()
	if err != nil {
		return IconInvalid, err
	}
	if !(width >= 0 && width <= 4096 && height >= 0 && height <= 4096) || len(transform) > 1 {
		return IconInvalid, errors.New("invalid Icon preparation dimensions or transform count")
	}
	matrix := IconTransform{1, 0, 0, 1, 0, 0}
	if len(transform) == 1 {
		matrix = transform[0]
	}
	var wire [6]C.float
	for i, coefficient := range matrix {
		if !finite(coefficient) {
			return IconInvalid, errors.New("invalid Icon transform coefficient")
		}
		wire[i] = C.float(coefficient)
	}
	var iconError C.uint32_t
	code := C.cui_go_icon_prepare_v1(owner.app.ptr, owner.handle, C.float(width), C.float(height), &wire[0], &iconError)
	if err := result(code); err != nil {
		return IconInvalid, err
	}
	if IconError(iconError) > IconAllocation {
		return IconInvalid, errors.New("invalid copied Icon preparation status")
	}
	return IconError(iconError), nil
}

// Bitmap copies the active mask. A failed Prepare can leave an older mask
// active. Check the artwork status before treating the bitmap as new.
func (r *IconResource) Bitmap() (IconBitmap, error) {
	owner, err := r.live()
	if err != nil {
		return IconBitmap{}, err
	}
	record := C.cui_bridge_icon_bitmap_v1{struct_size: C.sizeof_cui_bridge_icon_bitmap_v1, schema_version: 1}
	var required C.uint32_t
	code := C.cui_go_icon_bitmap_copy_v1(owner.app.ptr, owner.handle, &record, nil, 0, &required)
	var data []byte
	if code == C.CUI_BRIDGE_CAPACITY {
		if required == 0 || required > iconBitmapMax {
			return IconBitmap{}, errors.New("invalid Icon bitmap capacity")
		}
		data = make([]byte, int(required))
		code = C.cui_go_icon_bitmap_copy_v1(owner.app.ptr, owner.handle, &record,
			(*C.uchar)(unsafe.Pointer(&data[0])), required, &required)
	}
	if err := result(code); err != nil {
		return IconBitmap{}, err
	}
	box := Rect{float32(record.paint_x), float32(record.paint_y), float32(record.paint_width), float32(record.paint_height)}
	if record.struct_size != C.sizeof_cui_bridge_icon_bitmap_v1 || record.schema_version != 1 ||
		record.format != 0 || record.reserved != 0 || record.reserved2 != 0 ||
		record.width > 4096 || record.height > 4096 || record.stride != record.width*4 ||
		uint64(record.stride)*uint64(record.height) != uint64(len(data)) ||
		uint64(record.required_bytes) != uint64(len(data)) || uint64(required) != uint64(len(data)) ||
		(len(data) == 0 && (record.width != 0 || record.height != 0 || record.revision != 0)) ||
		(len(data) != 0 && record.revision == 0) ||
		!finite(box.X) || !finite(box.Y) || !finite(box.Width) || !finite(box.Height) ||
		box.Width < 0 || box.Height < 0 {
		return IconBitmap{}, errors.New("invalid copied Icon bitmap")
	}
	return IconBitmap{Width: uint32(record.width), Height: uint32(record.height),
		Stride: uint32(record.stride), Revision: uint64(record.revision), Paint: box, RGBA: data}, nil
}

// IconCatalogCount returns the active provider's catalog size.
func (a *App) IconCatalogCount() (uint32, error) {
	if err := a.requireIconResources(); err != nil {
		return 0, err
	}
	return uint32(C.cui_go_icon_catalog_count_v1()), nil
}

// IconCatalogEntry returns independent copies of a catalog name and path.
func (a *App) IconCatalogEntry(index uint32) (IconCatalogEntry, error) {
	if err := a.requireIconResources(); err != nil {
		return IconCatalogEntry{}, err
	}
	var name, path [iconTextMax + 1]byte
	var embedded, nameLen, pathLen C.uint32_t
	if err := result(C.cui_go_icon_catalog_entry_v1(C.uint32_t(index), &embedded, iconBytes(name[:]),
		C.uint32_t(len(name)), &nameLen, iconBytes(path[:]), C.uint32_t(len(path)), &pathLen)); err != nil {
		return IconCatalogEntry{}, err
	}
	if embedded > 1 || nameLen <= 1 || pathLen <= 1 {
		return IconCatalogEntry{}, errors.New("invalid copied Icon catalog entry")
	}
	label, err := iconReadString(name[:], uint32(nameLen))
	if err != nil {
		return IconCatalogEntry{}, err
	}
	key, err := iconReadString(path[:], uint32(pathLen))
	if err != nil {
		return IconCatalogEntry{}, err
	}
	return IconCatalogEntry{Name: label, Path: key, DefaultEmbedded: embedded == 1}, nil
}

// QueryIcon distinguishes unknown, catalog-only and embedded artwork.
func (a *App) QueryIcon(name string) (IconAvailability, error) {
	if err := a.requireIconResources(); err != nil {
		return IconUnknown, err
	}
	if !iconTextValid(name, iconTextMax) {
		return IconUnknown, errors.New("invalid Icon name")
	}
	var value C.uint32_t
	if err := result(C.cui_go_icon_query_v1(iconSpan(name), C.uint32_t(len(name)), &value)); err != nil {
		return IconUnknown, err
	}
	if IconAvailability(value) > IconEmbedded {
		return IconUnknown, errors.New("invalid copied Icon availability")
	}
	return IconAvailability(value), nil
}

// CopyIconAsset copies built-in SVG. The bool is false when the asset is absent.
func (a *App) CopyIconAsset(name string) (string, bool, error) {
	if err := a.requireIconResources(); err != nil {
		return "", false, err
	}
	if !iconTextValid(name, iconTextMax) {
		return "", false, errors.New("invalid Icon name")
	}
	return iconCopyString(iconSVGMax, true,
		func(out *C.char, cap C.uint32_t, required *C.uint32_t) C.cui_bridge_status {
			return C.cui_go_icon_asset_copy_v1(iconSpan(name), C.uint32_t(len(name)), out, cap, required)
		})
}
