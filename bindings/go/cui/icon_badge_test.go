// SPDX-License-Identifier: MIT
package cui

import (
	"errors"
	"os"
	"strings"
	"testing"
)

func makeArtwork(t *testing.T, a *App) *IconResource {
	t.Helper()
	resource, err := a.NewIconResource(IconSVG(iconSquareSVG), IconLimits{})
	check(t, err)
	return resource
}

func checkReferences(t *testing.T, resource *IconResource, want uint32) {
	t.Helper()
	info, err := resource.Info()
	check(t, err)
	if info.References != want {
		t.Fatalf("got %d references, want %d", info.References, want)
	}
}

func TestConfiguredIconRoundTripAndReset(t *testing.T) {
	a := app(t)
	art := makeArtwork(t, a)
	original := art.alias()
	options := IconOptions{
		Width: 24, Height: 18, HasWidth: true, HasHeight: true, InheritedTextSize: 30, RemSize: 16,
		HasOpacity: true, HasColor: true, HasInheritedColor: true, InheritedColorRGBA: 0x6080a0ff,
		HasTransform: true, Transform: IconRotation(.5), Decorative: true, DebugPlaceholder: true,
		HasStyle: true, Style: IconBoxStyle{
			Width: .5, Height: .5, MaxWidth: 30, MaxHeight: 33, BackgroundRGBA: 0xd5d5d5ff,
			Radius: 6, OffsetX: -1.5, OffsetY: 2, BorderRGBA: 0xb9b9b9ff,
			AlignX: IconAlignEnd, AlignY: IconAlignCenter, Clip: true, Row: true,
			CustomCorners: true, CornerRadii: [4]float32{1, 2, 3, 4},
			CustomBorderEdges: true, BorderEdges: [4]float32{1, 2, 1, 2},
			CustomPadding: true, PaddingEdges: [4]float32{1, 1, 1, 1},
			WidthPercent: true, HeightPercent: true, FixedWidth: true, FixedHeight: true,
			Inert: true, CustomOpacity: true,
		},
		HasConstraints: true, Constraints: IconConstraints{MaxWidth: 22, MinHeight: 19, HasMaxWidth: true, HasMinHeight: true},
	}
	icon := Icon("status", "check").VisualSize(18, 16)
	check(t, icon.ConfigureIcon(IconConfig{Resource: art, Options: options, Name: "Étoile"}))
	// A caller can overwrite its own wrapper or edit a snapshot without
	// modifying the resource reference or options stored on the node.
	*art = IconResource{}
	draft, ok, err := icon.IconConfig()
	check(t, err)
	if !ok || draft.Options != options || draft.Resource.owner != original.owner {
		t.Fatal("incorrect detached Icon configuration")
	}
	*draft.Resource = IconResource{}
	draft.Options.Style.CornerRadii[0] = 80
	check(t, a.Mount(Column("root").Children(icon)))
	checkReferences(t, original, 1)
	if err := original.Close(); err == nil {
		t.Fatal("closed a referenced resource")
	}
	if _, ok, err := icon.IconResult(); err != nil || ok {
		t.Fatalf("pre-frame Icon result: %t %v", ok, err)
	}
	check(t, a.Step(.016))
	bounds, err := a.Bounds(icon)
	check(t, err)
	if bounds.Width != 22 || bounds.Height != 19 {
		t.Fatalf("configured bounds: %+v", bounds)
	}
	previous, ok, err := icon.IconResult()
	check(t, err)
	if !ok || previous.IconError != IconOK || previous.FrameSequence == 0 || previous.FrameStatus != 0 {
		t.Fatalf("configured result: %+v", previous)
	}
	config, ok, err := icon.IconConfig()
	check(t, err)
	if !ok || config.Options != options || config.Name != "Étoile" || config.Resource.owner != original.owner {
		t.Fatal("incorrect mounted Icon round trip")
	}
	invalid := config
	invalid.Options.Style = IconBoxStyle{Padding: 3000}
	if err := icon.ConfigureIcon(invalid); err == nil {
		t.Fatal("admitted box whose required padding exceeded C bounds")
	}
	got, ok, err := icon.IconResult()
	check(t, err)
	if !ok || got != previous {
		t.Fatal("failed replacement changed the observation")
	}
	if err := icon.SetText("missing"); err == nil {
		t.Fatal("legacy setter modified configured Icon")
	}
	check(t, icon.ResetIcon())
	checkReferences(t, original, 0)
	if _, ok, err := icon.IconConfig(); err != nil || ok {
		t.Fatalf("reset config: %t %v", ok, err)
	}
	check(t, a.Step(.016))
	legacyBounds, err := a.Bounds(icon)
	check(t, err)
	if legacyBounds.Width != 18 || legacyBounds.Height != 16 {
		t.Fatalf("reset did not recover named, sized legacy Icon: %+v", legacyBounds)
	}
	check(t, icon.ConfigureIcon(IconConfig{Resource: original, Options: IconOptions{Size: IconCustom, CustomSize: 33}}))
	check(t, a.Step(.016))
	bounds, err = a.Bounds(icon)
	check(t, err)
	if bounds.Width != 33 || bounds.Height != 33 {
		t.Fatalf("custom Icon size: %+v", bounds)
	}
	check(t, icon.Remove())
	checkReferences(t, original, 0)
	check(t, original.Close())
}

func TestConfiguredBadgeZeroFlowAndCallbacks(t *testing.T) {
	a := app(t)
	art := makeArtwork(t, a)
	defaults, err := a.DefaultBadgeConfig()
	check(t, err)
	if defaults.Maximum != 99 || defaults.Variant != BadgeCount || defaults.Size != BadgeMedium {
		t.Fatalf("invalid defaults: %+v", defaults)
	}
	defaults.Count, defaults.Name = 8, "Unread messages"
	count := ConfiguredBadge("unread", defaults)
	owned := ConfiguredBadge("sync", BadgeConfig{Variant: BadgeIcon, Artwork: art,
		ArtworkOptions: IconOptions{Width: 12, Height: 12, HasWidth: true, HasHeight: true}, Name: "Synced"})
	text := Label("8 unread messages")
	clicks := 0
	clear := Button("clear").Text("Mark all as read").OnClick(func(Event) {
		clicks++
		cfg, err := count.BadgeConfig()
		check(t, err)
		cfg.Count = 0
		check(t, count.ConfigureBadge(cfg))
		check(t, text.SetText("Inbox is up to date"))
	})
	other := Button("outbox").Text("Outbox")
	check(t, a.Mount(Column("inbox").Children(text, Row("content").Children(count.Children(clear), owned.Children(other)))))
	check(t, a.Step(.016))
	checkReferences(t, art, 1)
	before, err := a.Bounds(clear)
	check(t, err)
	if _, present, err := count.BadgeIndicatorBounds(); err != nil || !present {
		t.Fatalf("count indicator unavailable: %t %v", present, err)
	}
	observation, present, err := owned.BadgeResult()
	check(t, err)
	if !present || !observation.Submitted || !observation.HasIconError || observation.IconError != IconOK || observation.FrameStatus != 0 {
		t.Fatalf("artwork observation: %+v", observation)
	}
	press(t, a, clear)
	if clicks != 0 {
		t.Fatal("click dispatched during Step")
	}
	_, _, err = count.BadgeResult()
	check(t, err)
	n, err := a.Dispatch()
	check(t, err)
	if clicks != 1 || n != 1 {
		t.Fatalf("click dispatched %d/%d times", n, clicks)
	}
	if _, observed, err := count.BadgeResult(); err != nil || observed {
		t.Fatalf("configuration must wait for the next observation: %t %v", observed, err)
	}
	check(t, a.Step(.016))
	after, err := a.Bounds(clear)
	check(t, err)
	if before != after {
		t.Fatalf("child moved when Badge count hid: %+v vs %+v", before, after)
	}
	if _, visible, err := count.BadgeIndicatorBounds(); err != nil || visible {
		t.Fatalf("zero count still visible: %t %v", visible, err)
	}
	check(t, count.Disabled(true).Err())
	check(t, a.Step(.016))
	press(t, a, clear)
	n, err = a.Dispatch()
	check(t, err)
	if clicks != 1 || n != 0 {
		t.Fatal("inert Badge subtree activated")
	}
	check(t, owned.Remove())
	checkReferences(t, art, 0)
	check(t, art.Close())
}

func TestConfiguredIconBadgeOmissionAndRecovery(t *testing.T) {
	a := app(t)
	const path = "inbox/sync.svg"
	art, err := a.NewIconResource(IconPath(path), IconLimits{})
	check(t, err)
	icon := ConfiguredIcon("image", IconConfig{Resource: art, Name: "Status"})
	badge := ConfiguredBadge("badge", BadgeConfig{Variant: BadgeIcon, Artwork: art, Name: "Synced"})
	panel := Collapsible("panel").Text("Panel").Open(true).Children(icon, badge.Children(Button("open").Text("Open")))
	check(t, a.Mount(Column("root").Children(panel)))
	check(t, a.Step(.016))
	checkReferences(t, art, 2)
	bad, ok, err := icon.IconResult()
	check(t, err)
	badBadge, badgeOK, err := badge.BadgeResult()
	check(t, err)
	if !ok || !badgeOK || bad.IconError != IconNotFound || !badBadge.HasIconError ||
		badBadge.IconError != IconNotFound || bad.FrameStatus != 0 || badBadge.FrameStatus != 0 {
		t.Fatalf("independent artwork and frame errors: %+v / %+v", bad, badBadge)
	}
	check(t, panel.Open(false).Err())
	check(t, a.Step(.016))
	if _, ok, err := icon.IconResult(); err != nil || ok {
		t.Fatalf("omitted Icon returned result: %t %v", ok, err)
	}
	if _, ok, err := badge.BadgeResult(); err != nil || ok {
		t.Fatalf("omitted Badge returned result: %t %v", ok, err)
	}
	if _, ok, err := badge.BadgeIndicatorBounds(); err != nil || ok {
		t.Fatalf("omitted Badge returned bounds: %t %v", ok, err)
	}
	check(t, art.PutAsset(path, iconSquareSVG))
	check(t, panel.Open(true).Err())
	check(t, a.Step(.016))
	good, ok, err := icon.IconResult()
	check(t, err)
	if !ok || good.IconError != IconOK || good.FrameSequence <= bad.FrameSequence {
		t.Fatalf("no fresh recovered Icon: %+v", good)
	}
	update := BadgeConfig{Variant: BadgeIcon, Name: "check", NamedIcon: "check"}
	check(t, badge.ConfigureBadge(update))
	copied, err := badge.BadgeConfig()
	check(t, err)
	if copied != update {
		t.Fatal("same-name named-icon update failed to round trip")
	}
	checkReferences(t, art, 1)
	check(t, a.Step(.016))
	named, ok, err := badge.BadgeResult()
	check(t, err)
	if !ok || named.HasIconError {
		t.Fatal("named icon reported an owned-artwork error")
	}
	update = BadgeConfig{Variant: BadgeDot, HasColor: true}
	check(t, badge.ConfigureBadge(update))
	copied, err = badge.BadgeConfig()
	check(t, err)
	if copied != update {
		t.Fatal("explicit transparent color was lost")
	}
	check(t, a.Step(.016))
	if _, ok, err := badge.BadgeIndicatorBounds(); err != nil || !ok {
		t.Fatalf("dot indicator bounds: %t %v", ok, err)
	}
}

func TestConfiguredIconBadgeForeignAndFailedMount(t *testing.T) {
	a := app(t)
	foreign := app(t)
	art := makeArtwork(t, a)
	icon := ConfiguredIcon("image", IconConfig{Resource: art})
	badge := ConfiguredBadge("badge", BadgeConfig{Variant: BadgeIcon, Artwork: art})
	for _, node := range []*Node{icon, badge} {
		err := foreign.Mount(Column("foreign").Children(node))
		if err == nil || !strings.Contains(err.Error(), "another app") {
			t.Fatalf("foreign resource: %v", err)
		}
	}
	checkReferences(t, art, 0)
	root := Column("invalid").Children(icon, badge.Children(Button("inside")), Item("orphan", "Invalid"))
	if err := a.Mount(root); err == nil {
		t.Fatal("legacy construction failure did not trigger rollback")
	}
	checkReferences(t, art, 0)
	check(t, a.Mount(Column("retry").Children(icon, badge)))
	check(t, a.Step(.016))
	checkReferences(t, art, 2)
	check(t, a.Close())
	if _, err := art.Info(); !errors.Is(err, ErrClosed) {
		t.Fatalf("artwork after shutdown: %v", err)
	}
	check(t, art.Close())
}

func TestConfiguredIconBadgeProvider(t *testing.T) {
	a := app(t)
	mode := os.Getenv("CUI_TEST_ICON_BADGE_PROVIDER")
	wantIcon := mode != "alpha4" && mode != "legacy"
	wantBadge := wantIcon && mode != "partial-badge"
	iconSupport, err := a.SupportsIconPresentation()
	check(t, err)
	badgeSupport, err := a.SupportsConfiguredBadge()
	check(t, err)
	if iconSupport != wantIcon || badgeSupport != wantBadge {
		t.Fatalf("provider %q supported Icon/Badge %t/%t, want %t/%t", mode, iconSupport, badgeSupport, wantIcon, wantBadge)
	}
	if !wantBadge {
		modern := ConfiguredBadge("needs-badge", BadgeConfig{Variant: BadgeDot})
		if err := a.Mount(Column("missing").Children(modern)); !errors.Is(err, ErrConfiguredBadgeUnavailable) {
			t.Fatalf("configured badge rejection: %v", err)
		}
	}
	icon := Icon("legacy", "check")
	root := Column("compatible").Children(icon, Badge("New"))
	if mode != "legacy" {
		art := makeArtwork(t, a)
		modern := ConfiguredIcon("art", IconConfig{Resource: art})
		if !wantIcon {
			if err := a.Mount(Column("missing").Children(modern)); !errors.Is(err, ErrIconPresentationUnavailable) {
				t.Fatalf("configured Icon rejection: %v", err)
			}
			checkReferences(t, art, 0)
		} else {
			root.Children(modern)
		}
	}
	check(t, a.Mount(root))
	check(t, a.Step(.016))
}
