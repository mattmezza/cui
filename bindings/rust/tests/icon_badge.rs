// SPDX-License-Identifier: MIT
use cui_builders::{
    App, BadgeConfig, BadgeVariant, Error, IconAlign, IconBoxStyle, IconConfig, IconConstraints,
    IconError, IconLimits, IconOptions, IconResource, IconSize, IconSource, IconTransform, Kind,
    Node, Phase, Rect, Size,
};
use std::{cell::Cell, rc::Rc, sync::Mutex};

static NATIVE: Mutex<()> = Mutex::new(());
const SVG: &str = "<svg width='16' height='16'><rect x='2' y='2' width='12' height='12'/></svg>";

fn artwork(app: &mut App) -> IconResource {
    app.create_icon_resource(&IconSource::svg(SVG), IconLimits::default())
        .unwrap()
}

fn badge_artwork(resource: IconResource) -> BadgeConfig {
    BadgeConfig {
        variant: BadgeVariant::Artwork {
            resource,
            options: IconOptions {
                width: Some(12.),
                height: Some(12.),
                ..Default::default()
            },
        },
        name: "Synced".into(),
        ..Default::default()
    }
}

fn press(app: &mut App, node: Node) -> Rect {
    let before = app.bounds(node).unwrap();
    let x = before.x + before.width / 2.;
    let y = before.y + before.height / 2.;
    app.pointer(Phase::Down, x, y).unwrap();
    app.pointer(Phase::Up, x, y).unwrap();
    app.step(0.016).unwrap();
    before
}

#[test]
fn icon_round_trip_reset_and_failed_update() {
    let _lock = NATIVE.lock().unwrap_or_else(|err| err.into_inner());
    let mut app = App::headless(640., 360.).unwrap();
    let root = app.root(Kind::Column, "root").unwrap();
    let art = artwork(&mut app);
    let options = IconOptions {
        width: Some(24.),
        height: Some(18.),
        color_rgba: Some(0),
        inherited_color_rgba: Some(0x6080a0ff),
        inherited_text_size: 30.,
        rem_size: 16.,
        opacity: Some(0.),
        transform: Some(IconTransform::rotation(0.5)),
        decorative: true,
        debug_placeholder: true,
        style: Some(IconBoxStyle {
            width: 0.5,
            height: 0.5,
            max_width: 30.,
            max_height: 33.,
            radius: 6.,
            offset_x: -1.5,
            offset_y: 2.,
            align_x: IconAlign::End,
            align_y: IconAlign::Center,
            background_rgba: 0xd5d5d5ff,
            border_rgba: 0xb9b9b9ff,
            corner_radii: Some([1., 2., 3., 4.]),
            border_edges: Some([1., 2., 1., 2.]),
            padding_edges: Some([1.; 4]),
            width_percent: true,
            height_percent: true,
            fixed_width: true,
            fixed_height: true,
            inert: true,
            clip: true,
            row: true,
            opacity: Some(0.),
            ..Default::default()
        }),
        constraints: Some(IconConstraints {
            max_width: Some(22.),
            min_height: Some(19.),
            ..Default::default()
        }),
        ..Default::default()
    };
    let config = IconConfig {
        resource: art,
        options,
        name: "Étoile".into(),
    };
    let mut draft = config.clone();
    let icon = app
        .build_child(root, Kind::Icon, "status")
        .unwrap()
        .text("check")
        .unwrap()
        .visual_size(18., 16.)
        .unwrap()
        .icon_presentation(&draft)
        .unwrap()
        .finish();
    draft.name.clear();
    draft.options.style.as_mut().unwrap().corner_radii = Some([80.; 4]);
    assert_eq!(app.icon_config(icon).unwrap(), Some(config));
    assert_eq!(app.icon_info(art).unwrap().references, 1);
    assert_eq!(app.release_icon_resource(art), Err(Error(8)));
    assert_eq!(app.icon_result(icon).unwrap(), None);
    app.step(0.016).unwrap();
    let bounds = app.bounds(icon).unwrap();
    assert_eq!((bounds.width, bounds.height), (22., 19.));
    let before = app.icon_result(icon).unwrap().unwrap();
    assert_eq!((before.icon_error, before.frame_status), (IconError::Ok, 0));
    draft.options.width = Some(f32::NAN);
    assert_eq!(app.configure_icon(icon, &draft).err(), Some(Error(3)));
    assert_eq!(app.icon_result(icon).unwrap(), Some(before));
    assert_eq!(app.text(icon, "missing").err(), Some(Error(8)));
    app.reset_icon(icon).unwrap();
    assert_eq!(app.icon_config(icon).unwrap(), None);
    assert_eq!(app.icon_result(icon).unwrap(), None);
    assert_eq!(app.icon_info(art).unwrap().references, 0);
    app.step(0.016).unwrap();
    let bounds = app.bounds(icon).unwrap();
    assert_eq!((bounds.width, bounds.height), (18., 16.));
    let mut custom = IconConfig::new(art);
    custom.options.size = IconSize::Custom(33.);
    app.configure_icon(icon, &custom).unwrap();
    app.step(0.016).unwrap();
    let bounds = app.bounds(icon).unwrap();
    assert_eq!((bounds.width, bounds.height), (33., 33.));
    app.remove(icon).unwrap();
    assert_eq!(app.icon_result(icon), Err(Error(4)));
    app.release_icon_resource(art).unwrap();
}

#[test]
fn badge_child_events_are_passive_results_and_zero_flow() {
    let _lock = NATIVE.lock().unwrap_or_else(|err| err.into_inner());
    let mut app = App::headless(640., 360.).unwrap();
    let root = app.root(Kind::Column, "root").unwrap();
    assert_eq!(app.default_badge_config().unwrap(), BadgeConfig::default());
    let count = BadgeConfig {
        name: "Unread".into(),
        design_size: Size::Md,
        ..BadgeConfig::count(8)
    };
    let badge = app.configured_badge(root, "count", &count).unwrap();
    let button = app
        .build_child(badge, Kind::Button, "open")
        .unwrap()
        .text("Mark all as read")
        .unwrap()
        .finish();
    let clicks = Rc::new(Cell::new(0));
    let captured = clicks.clone();
    app.on_click(button, move || captured.set(captured.get() + 1))
        .unwrap();
    assert_eq!(app.badge_result(badge).unwrap(), None);
    app.step(0.016).unwrap();
    assert!(app.badge_indicator_bounds(badge).unwrap().is_some());
    let before = press(&mut app, button);
    let observation = app.badge_result(badge).unwrap().unwrap();
    assert!(observation.submitted);
    assert_eq!(observation.icon_error, None);
    assert_eq!(app.badge_result(badge).unwrap(), Some(observation));
    assert_eq!(clicks.get(), 0);
    assert_eq!(app.dispatch().unwrap(), 1);
    assert_eq!(clicks.get(), 1);
    assert_eq!(app.dispatch().unwrap(), 0);
    app.configure_badge(badge, &BadgeConfig::count(0)).unwrap();
    assert_eq!(app.badge_result(badge).unwrap(), None);
    app.step(0.016).unwrap();
    assert!(app.badge_indicator_bounds(badge).unwrap().is_none());
    let after = app.bounds(button).unwrap();
    assert_eq!(
        (before.x, before.y, before.width, before.height),
        (after.x, after.y, after.width, after.height)
    );
    app.component_disabled(badge, true).unwrap();
    app.step(0.016).unwrap();
    press(&mut app, button);
    assert_eq!(app.dispatch().unwrap(), 0);
    assert_eq!(clicks.get(), 1);
}

#[test]
fn shared_artwork_omission_recovery_and_removal() {
    let _lock = NATIVE.lock().unwrap_or_else(|err| err.into_inner());
    let mut app = App::headless(640., 360.).unwrap();
    let path = "inbox/synced.svg";
    let art = app
        .create_icon_resource(&IconSource::path(path), IconLimits::default())
        .unwrap();
    let root = app.root(Kind::Column, "root").unwrap();
    let panel = app
        .build_child(root, Kind::Collapsible, "panel")
        .unwrap()
        .text("Panel")
        .unwrap()
        .open(true)
        .unwrap()
        .finish();
    let icon = app
        .configured_icon(panel, "icon", &IconConfig::new(art))
        .unwrap();
    let config = badge_artwork(art);
    let badge = app.configured_badge(panel, "badge", &config).unwrap();
    app.build_child(badge, Kind::Button, "sent")
        .unwrap()
        .text("Sent")
        .unwrap();
    assert_eq!(app.badge_config(badge).unwrap(), config);
    assert_eq!(app.icon_info(art).unwrap().references, 2);
    app.step(0.016).unwrap();
    let previous = app.icon_result(icon).unwrap().unwrap();
    assert_eq!(
        (previous.icon_error, previous.frame_status),
        (IconError::NotFound, 0)
    );
    let badge_result = app.badge_result(badge).unwrap().unwrap();
    assert_eq!(
        (badge_result.icon_error, badge_result.frame_status),
        (Some(IconError::NotFound), 0)
    );
    let invalid = BadgeConfig {
        name: "invalid\0name".into(),
        ..config.clone()
    };
    assert_eq!(app.configure_badge(badge, &invalid).err(), Some(Error(3)));
    assert_eq!(app.badge_config(badge).unwrap(), config);
    assert_eq!(app.badge_result(badge).unwrap(), Some(badge_result));
    app.open(panel, false).unwrap();
    app.step(0.016).unwrap();
    assert_eq!(app.icon_result(icon).unwrap(), None);
    assert_eq!(app.badge_result(badge).unwrap(), None);
    assert!(app.badge_indicator_bounds(badge).unwrap().is_none());
    app.put_icon_asset(art, path, SVG).unwrap();
    app.open(panel, true).unwrap();
    app.step(0.016).unwrap();
    let recovered = app.icon_result(icon).unwrap().unwrap();
    assert_eq!(recovered.icon_error, IconError::Ok);
    assert!(recovered.frame_sequence > previous.frame_sequence);
    assert_eq!(
        app.badge_result(badge).unwrap().unwrap().icon_error,
        Some(IconError::Ok)
    );
    let named = BadgeConfig {
        name: "check".into(),
        variant: BadgeVariant::NamedIcon("check".into()),
        ..Default::default()
    };
    app.configure_badge(badge, &named).unwrap();
    assert_eq!(app.badge_config(badge).unwrap(), named);
    assert_eq!(app.icon_info(art).unwrap().references, 1);
    app.step(0.016).unwrap();
    assert_eq!(app.badge_result(badge).unwrap().unwrap().icon_error, None);
    let dot = BadgeConfig {
        variant: BadgeVariant::Dot,
        color_rgba: Some(0),
        ..Default::default()
    };
    app.configure_badge(badge, &dot).unwrap();
    assert_eq!(app.badge_config(badge).unwrap(), dot);
    app.step(0.016).unwrap();
    assert!(app.badge_indicator_bounds(badge).unwrap().is_some());
    app.remove(panel).unwrap();
    assert_eq!(app.badge_result(badge), Err(Error(4)));
    assert_eq!(app.icon_info(art).unwrap().references, 0);
    app.release_icon_resource(art).unwrap();
}

#[test]
fn handles_and_creation_rollback() {
    let _lock = NATIVE.lock().unwrap_or_else(|err| err.into_inner());
    let mut app = App::headless(320., 240.).unwrap();
    let mut foreign = App::headless(320., 240.).unwrap();
    let root = app.root(Kind::Column, "root").unwrap();
    let foreign_root = foreign.root(Kind::Column, "foreign").unwrap();
    let art = artwork(&mut app);
    assert_eq!(
        foreign.configured_icon(foreign_root, "foreign", &IconConfig::new(art)),
        Err(Error(3))
    );
    assert_eq!(
        foreign.configured_badge(foreign_root, "foreign", &badge_artwork(art)),
        Err(Error(3))
    );
    assert_eq!(
        app.configured_badge(foreign_root, "foreign", &BadgeConfig::count(2)),
        Err(Error(3))
    );
    assert_eq!(app.icon_info(art).unwrap().references, 0);
    app.release_icon_resource(art).unwrap();
    assert_eq!(
        app.configured_icon(root, "icon", &IconConfig::new(art)),
        Err(Error(4))
    );
    assert_eq!(
        app.configured_badge(root, "badge", &badge_artwork(art)),
        Err(Error(4))
    );
    let art = artwork(&mut app);
    app.configured_icon(root, "icon", &IconConfig::new(art))
        .unwrap();
    app.configured_badge(root, "badge", &badge_artwork(art))
        .unwrap();
    assert_eq!(app.icon_info(art).unwrap().references, 2);
    app.close().unwrap();
    assert_eq!(app.icon_info(art), Err(Error(2)));
}

#[test]
fn optional_provider_preserves_legacy() {
    let _lock = NATIVE.lock().unwrap_or_else(|err| err.into_inner());
    let mut app = App::headless(320., 240.).unwrap();
    let mode = std::env::var("CUI_TEST_ICON_BADGE_PROVIDER").unwrap_or_default();
    let wants_icon = mode != "alpha4" && mode != "legacy";
    let wants_badge = wants_icon && mode != "partial_badge";
    assert_eq!(app.supports_icon_presentation(), wants_icon);
    assert_eq!(app.supports_configured_badge(), wants_badge);
    let root = app.root(Kind::Column, "root").unwrap();
    app.build_child(root, Kind::Icon, "legacy")
        .unwrap()
        .text("check")
        .unwrap();
    app.build_child(root, Kind::Badge, "old-badge")
        .unwrap()
        .text("New")
        .unwrap();
    if mode != "legacy" {
        let art = artwork(&mut app);
        let icon = app.configured_icon(root, "new-icon", &IconConfig::new(art));
        assert_eq!(icon.is_ok(), wants_icon);
        if !wants_icon {
            assert_eq!(icon, Err(Error(9)));
        }
        assert_eq!(
            app.icon_info(art).unwrap().references,
            u32::from(wants_icon)
        );
    }
    let badge = app.configured_badge(root, "badge", &BadgeConfig::count(2));
    assert_eq!(badge.is_ok(), wants_badge);
    if !wants_badge {
        assert_eq!(badge, Err(Error(9)));
    }
    app.step(0.016).unwrap();
}
