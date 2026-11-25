// SPDX-License-Identifier: MIT
package cui

import (
	"bytes"
	"errors"
	"math"
	"os"
	"strings"
	"testing"
)

const iconSquareSVG = `<svg xmlns="http://www.w3.org/2000/svg" width="12" height="12"><path d="M1 1h10v10H1z"/></svg>`

func requireIconProvider(t *testing.T) *App {
	t.Helper()
	a := app(t)
	supported, err := a.SupportsIconResources()
	check(t, err)
	if !supported {
		t.Fatal("full Icon resource provider required")
	}
	return a
}

func checkIconPrepare(t *testing.T, r *IconResource, want IconError, transforms ...IconTransform) {
	t.Helper()
	got, err := r.Prepare(12, 12, transforms...)
	check(t, err)
	if got != want {
		t.Fatalf("Icon preparation: got %v, want %v", got, want)
	}
}

func TestIconResourceOptionalProvider(t *testing.T) {
	a := app(t)
	supported, err := a.SupportsIconResources()
	check(t, err)
	want := os.Getenv("CUI_TEST_ICON_RESOURCE_UNSUPPORTED") != "1"
	if supported != want {
		t.Fatalf("Icon resource support %t, want %t", supported, want)
	}
	if !want {
		if _, err := a.NewIconResource(IconSource{}, IconLimits{}); !errors.Is(err, ErrIconResourcesUnavailable) {
			t.Fatalf("unsupported resource constructor: %v", err)
		}
	}
	icon, badge := Icon("legacy", "check").VisualSize(24, 24), Badge("8").Key("badge")
	check(t, a.Mount(Row("row").Children(icon, badge)))
	check(t, a.Step(.016))
	for _, node := range []*Node{icon, badge} {
		rect, err := a.Bounds(node)
		check(t, err)
		if rect.Width <= 0 || rect.Height <= 0 {
			t.Fatalf("missing legacy widget bounds: %+v", rect)
		}
	}
}

func TestIconResourceCatalogAndDefaults(t *testing.T) {
	a := requireIconProvider(t)
	defaults, err := a.DefaultIconLimits()
	check(t, err)
	if want := (IconLimits{256 << 10, 1 << 20, 8 << 20, 16}); defaults != want {
		t.Fatalf("limits %+v, want %+v", defaults, want)
	}
	count, err := a.IconCatalogCount()
	check(t, err)
	if count < 123 {
		t.Fatalf("unexpected Icon catalog size: %d", count)
	}
	entry, err := a.IconCatalogEntry(0)
	check(t, err)
	if entry.Name == "" || entry.Path == "" {
		t.Fatalf("invalid catalog entry: %+v", entry)
	}
	if _, err := a.IconCatalogEntry(count); err == nil {
		t.Fatal("out of range catalog access accepted")
	}
	for _, name := range []string{"check", "outside-catalog-in-cui-tests"} {
		want := name == "check"
		availability, err := a.QueryIcon(name)
		check(t, err)
		if want && availability != IconEmbedded || !want && availability != IconUnknown {
			t.Fatalf("%s: unexpected availability %v", name, availability)
		}
		svg, exists, err := a.CopyIconAsset(name)
		check(t, err)
		if exists != want || (exists && !strings.Contains(svg, "<svg")) || (!exists && svg != "") {
			t.Fatalf("%s: invalid catalog copy", name)
		}
	}
	r, err := a.NewIconResource(IconName("check"), IconLimits{MaxVersions: 2})
	check(t, err)
	got, err := r.Limits()
	check(t, err)
	defaults.MaxVersions = 2
	if got != defaults {
		t.Fatalf("limits %+v, want %+v", got, defaults)
	}
	source, err := r.Source()
	check(t, err)
	if source != IconName("check") {
		t.Fatalf("wrong named source: %+v", source)
	}
	checkIconPrepare(t, r, IconOK)
	info, err := r.Info()
	check(t, err)
	if info.ParseCount != 1 || info.RasterCount != 1 || info.References != 0 || info.SourceKind != IconSourceName {
		t.Fatalf("unexpected preparation counters: %+v", info)
	}
	check(t, r.Close())
}

func TestIconResourceCopiesAndCache(t *testing.T) {
	a := requireIconProvider(t)
	r, err := a.NewIconResource(IconSource{}, IconLimits{})
	check(t, err)
	source, err := r.Source()
	check(t, err)
	if source != (IconSource{}) {
		t.Fatalf("wrong initial source: %+v", source)
	}
	empty, err := r.Bitmap()
	check(t, err)
	if empty.Revision != 0 || empty.Width != 0 || empty.Height != 0 || len(empty.RGBA) != 0 {
		t.Fatal("unexpected initial bitmap")
	}

	const path = "profile/mark.svg"
	check(t, r.SetSource(IconPath(path)))
	checkIconPrepare(t, r, IconNotFound)
	check(t, r.PutAsset(path, iconSquareSVG))
	transform := IconRotation(0)
	checkIconPrepare(t, r, IconOK, transform)
	square, err := r.Bitmap()
	check(t, err)
	if square.Width != 12 || square.Height != 12 || square.Stride != 48 || len(square.RGBA) != 576 {
		t.Fatal("invalid packed SVG raster dimensions")
	}
	alpha := func(x, y int) int { return y*int(square.Stride) + x*4 + 3 }
	squarePixels := bytes.Clone(square.RGBA)
	clear(square.RGBA)
	copied, err := r.Bitmap()
	check(t, err)
	if !bytes.Equal(squarePixels, copied.RGBA) || copied.Revision != square.Revision || squarePixels[alpha(8, 8)] == 0 {
		t.Fatal("bitmap is aliased or missing expected coverage")
	}
	before, err := r.Info()
	check(t, err)
	checkIconPrepare(t, r, IconOK, transform)
	warm, err := r.Info()
	check(t, err)
	if warm.RasterCount != before.RasterCount || warm.ParseCount != before.ParseCount {
		t.Fatal("identical preparation missed the cache")
	}

	cornerSVG := strings.Replace(iconSquareSVG, "M1 1h10v10H1z", "M1 1h4v4H1z", 1)
	check(t, r.PutAsset(path, cornerSVG))
	check(t, r.Reload())
	checkIconPrepare(t, r, IconOK)
	corner, err := r.Bitmap()
	check(t, err)
	if corner.RGBA[alpha(3, 3)] == 0 || corner.RGBA[alpha(8, 8)] != 0 || corner.Revision == square.Revision {
		t.Fatal("replacement coverage or bitmap generation incorrect")
	}
	after, err := r.Info()
	check(t, err)
	if after.SourceRevision <= before.SourceRevision || after.CacheVersions <= 1 {
		t.Fatalf("expected retired cached mask: %+v", after)
	}
	check(t, r.Collect())
	collected, err := r.Info()
	check(t, err)
	if collected.CacheVersions != 1 || collected.CacheBytes >= after.CacheBytes {
		t.Fatalf("failed to collect retired mask: %+v", collected)
	}

	check(t, r.PutAsset(path, "<broken"))
	checkIconPrepare(t, r, IconParse)
	failed, err := r.Bitmap()
	check(t, err)
	if !bytes.Equal(failed.RGBA, corner.RGBA) || failed.Revision != corner.Revision {
		t.Fatal("failed preparation changed the previously active mask")
	}
	check(t, r.RemoveAsset(path))
	checkIconPrepare(t, r, IconNotFound)
	check(t, r.PutAsset(path, iconSquareSVG))
	check(t, r.ClearAssets())
	checkIconPrepare(t, r, IconNotFound)
	check(t, r.SetSource(IconSVG(iconSquareSVG)))
	check(t, r.SetLimits(IconLimits{MaxPixels: 64}))
	checkIconPrepare(t, r, IconLimit)
	check(t, r.SetLimits(IconLimits{}))
	checkIconPrepare(t, r, IconOK)
}

func TestIconResourceValidationAndLifetime(t *testing.T) {
	a := requireIconProvider(t)
	r, err := a.NewIconResource(IconSVG(iconSquareSVG), IconLimits{})
	check(t, err)
	for _, invalid := range []IconSource{
		{Kind: 19}, {Kind: IconSourceEmpty, Text: "unexpected"},
		IconPath("x\x00y"), IconSVG("\xc0\xaf"), IconName(strings.Repeat("é", 2049)),
	} {
		if err := r.SetSource(invalid); err == nil {
			t.Errorf("accepted invalid source kind %v with %d bytes", invalid.Kind, len(invalid.Text))
		}
	}
	if err := r.SetLimits(IconLimits{MaxVersions: 129}); err == nil {
		t.Error("accepted invalid cache limit")
	}
	if _, err := r.Prepare(12, 12, IconTransform{float32(math.NaN())}); err == nil {
		t.Error("accepted NaN transform")
	}
	src, err := r.Source()
	check(t, err)
	if src != IconSVG(iconSquareSVG) {
		t.Fatal("rejected update changed the source")
	}
	ch := make(chan error, 1)
	go func() {
		_, err := r.Info()
		ch <- err
	}()
	if err := <-ch; err == nil || !strings.Contains(err.Error(), "another OS thread") {
		t.Fatalf("non-owner access: %v", err)
	}
	for range 130 {
		if resource, err := a.NewIconResource(IconSVG(iconSquareSVG), IconLimits{MaxSVGBytes: 1}); resource != nil || err == nil {
			t.Fatal("constructor admitted source longer than resource limit")
		}
	}
	alias := *r
	check(t, alias.Close())
	check(t, r.Close())
	next, err := a.NewIconResource(IconSVG(iconSquareSVG), IconLimits{})
	check(t, err)
	if _, err := r.Info(); err == nil {
		t.Fatal("copy of closed resource gained access to reused slot")
	}
	checkIconPrepare(t, next, IconOK)
	check(t, a.Close())
	if _, err := next.Info(); !errors.Is(err, ErrClosed) {
		t.Fatalf("resource after app shutdown: %v", err)
	}
	check(t, next.Close())
}
