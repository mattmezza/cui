// SPDX-License-Identifier: MIT
use cui_builders::{
    App, Error, IconAvailability, IconError, IconLimits, IconResource, IconSource, IconSourceKind,
    IconTransform, Kind,
};
use std::sync::Mutex;

static UI_THREAD: Mutex<()> = Mutex::new(());
const SQUARE: &str = r#"<svg width="12" height="12"><path d="M1 1h10v10H1z"/></svg>"#;
const CORNER: &str = r#"<svg width="12" height="12"><path d="M1 1h4v4H1z"/></svg>"#;

fn prepare(app: &mut App, resource: IconResource) -> IconError {
    app.prepare_icon(resource, 12., 12., IconTransform::IDENTITY)
        .unwrap()
}

#[test]
fn optional_provider_preserves_legacy() {
    let _serial = UI_THREAD.lock().unwrap();
    let mut app = App::headless(320., 240.).unwrap();
    let supported = std::env::var("CUI_TEST_ICON_RESOURCE_UNSUPPORTED").as_deref() != Ok("1");
    assert_eq!(app.supports_icon_resources(), supported);
    if !supported {
        assert_eq!(app.default_icon_limits(), Err(Error(9)));
        assert_eq!(
            app.create_icon_resource(&IconSource::Empty, IconLimits::default()),
            Err(Error(9))
        );
    }
    let root = app.root(Kind::Column, "root").unwrap();
    let icon = app.child(root, Kind::Icon, "legacy-icon").unwrap();
    app.text(icon, "check").unwrap();
    app.visual_size(icon, 22., 18.).unwrap();
    let badge = app.child(root, Kind::Badge, "legacy-badge").unwrap();
    app.text(badge, "8").unwrap();
    app.step(0.016).unwrap();
    let bounds = app.bounds(icon).unwrap();
    assert_eq!((bounds.width, bounds.height), (22., 18.));
    assert!(app.bounds(badge).unwrap().width > 0.);
}

#[test]
fn catalog_defaults_and_copied_source() {
    let _serial = UI_THREAD.lock().unwrap();
    let mut app = App::headless(320., 240.).unwrap();
    assert!(app.supports_icon_resources());
    let defaults = app.default_icon_limits().unwrap();
    assert_eq!(
        defaults,
        IconLimits {
            max_svg_bytes: 256 << 10,
            max_pixels: 1 << 20,
            max_cache_bytes: 8 << 20,
            max_versions: 16,
        }
    );
    let count = app.icon_catalog_count().unwrap();
    assert!(count >= 123);
    let entry = app.icon_catalog_entry(0).unwrap();
    assert!(!entry.name.is_empty() && !entry.path.is_empty());
    assert!(app.icon_catalog_entry(count).is_err());
    assert_eq!(app.query_icon("check").unwrap(), IconAvailability::Embedded);
    assert_eq!(
        app.query_icon("outside-catalog-in-cui-tests").unwrap(),
        IconAvailability::Unknown
    );
    assert!(app
        .copy_icon_asset("check")
        .unwrap()
        .unwrap()
        .contains("<svg"));
    assert_eq!(
        app.copy_icon_asset("outside-catalog-in-cui-tests").unwrap(),
        None
    );
    let limits = IconLimits {
        max_versions: 2,
        ..Default::default()
    };
    let named = IconSource::named("check");
    let resource = app.create_icon_resource(&named, limits).unwrap();
    assert_eq!(app.icon_source(resource).unwrap(), named);
    assert_eq!(
        app.icon_limits(resource).unwrap(),
        IconLimits {
            max_versions: 2,
            ..defaults
        }
    );
    assert_eq!(prepare(&mut app, resource), IconError::Ok);
    let info = app.icon_info(resource).unwrap();
    assert_eq!(
        (info.parse_count, info.raster_count, info.references),
        (1, 1, 0)
    );
    assert_eq!(info.source_kind, IconSourceKind::Name);
    app.release_icon_resource(resource).unwrap();
}

#[test]
fn raster_cache_copies_and_error_recovery() {
    let _serial = UI_THREAD.lock().unwrap();
    let mut app = App::headless(320., 240.).unwrap();
    let resource = app
        .create_icon_resource(&IconSource::Empty, IconLimits::default())
        .unwrap();
    assert_eq!(app.icon_source(resource).unwrap(), IconSource::Empty);
    let empty = app.icon_bitmap(resource).unwrap();
    assert_eq!(
        (empty.width, empty.height, empty.revision, empty.rgba.len()),
        (0, 0, 0, 0)
    );

    let path = "profile/status.svg";
    app.set_icon_source(resource, &IconSource::path(path))
        .unwrap();
    assert_eq!(prepare(&mut app, resource), IconError::NotFound);
    let mut svg = SQUARE.to_owned();
    app.put_icon_asset(resource, path, &svg).unwrap();
    svg.clear();
    assert_eq!(prepare(&mut app, resource), IconError::Ok);
    let mut square = app.icon_bitmap(resource).unwrap();
    assert_eq!(
        (
            square.width,
            square.height,
            square.stride,
            square.rgba.len()
        ),
        (12, 12, 48, 576)
    );
    let alpha = |x: usize, y: usize| y * 48 + x * 4 + 3;
    let pixels = square.rgba.clone();
    square.rgba.fill(0);
    let same = app.icon_bitmap(resource).unwrap();
    assert_eq!(same.rgba, pixels);
    assert!(same.rgba[alpha(8, 8)] > 0);
    let warm = app.icon_info(resource).unwrap();
    assert_eq!(prepare(&mut app, resource), IconError::Ok);
    assert_eq!(app.icon_info(resource).unwrap(), warm);

    let moved = IconTransform([1., 0., 0., 1., 2., 1.]);
    assert_eq!(
        app.prepare_icon(resource, 12., 12., moved).unwrap(),
        IconError::Ok
    );
    let moved_bitmap = app.icon_bitmap(resource).unwrap();
    assert_eq!((moved_bitmap.paint.x, moved_bitmap.paint.y), (2., 1.));
    let rotated = IconTransform::rotation(0.5);
    assert_eq!(
        app.prepare_icon(resource, 12., 12., rotated).unwrap(),
        IconError::Ok
    );
    assert!(app.icon_bitmap(resource).unwrap().width > 12);

    app.put_icon_asset(resource, path, CORNER).unwrap();
    app.reload_icon(resource).unwrap();
    assert_eq!(prepare(&mut app, resource), IconError::Ok);
    let corner = app.icon_bitmap(resource).unwrap();
    assert_eq!(corner.rgba[alpha(8, 8)], 0);
    assert!(corner.rgba[alpha(3, 3)] > 0 && corner.revision != square.revision);
    let before_collect = app.icon_info(resource).unwrap();
    assert!(
        before_collect.source_revision > warm.source_revision && before_collect.cache_versions > 1
    );
    app.collect_icon(resource).unwrap();
    let collected = app.icon_info(resource).unwrap();
    assert_eq!(collected.cache_versions, 1);
    assert!(collected.cache_bytes < before_collect.cache_bytes);

    app.put_icon_asset(resource, path, "<broken").unwrap();
    assert_eq!(prepare(&mut app, resource), IconError::Parse);
    assert_eq!(app.icon_bitmap(resource).unwrap().rgba, corner.rgba);
    app.remove_icon_asset(resource, path).unwrap();
    assert_eq!(prepare(&mut app, resource), IconError::NotFound);
    app.put_icon_asset(resource, path, SQUARE).unwrap();
    app.clear_icon_assets(resource).unwrap();
    assert_eq!(prepare(&mut app, resource), IconError::NotFound);
    app.set_icon_source(resource, &IconSource::svg(SQUARE))
        .unwrap();
    app.set_icon_limits(
        resource,
        IconLimits {
            max_pixels: 64,
            ..Default::default()
        },
    )
    .unwrap();
    assert_eq!(prepare(&mut app, resource), IconError::Limit);
    app.set_icon_limits(resource, IconLimits::default())
        .unwrap();
    assert_eq!(prepare(&mut app, resource), IconError::Ok);

    drop(app);
    assert_eq!(corner.rgba[alpha(8, 8)], 0);
    assert!(pixels[alpha(8, 8)] > 0);
}

#[test]
fn rejects_foreign_stale_and_invalid_calls_without_mutating() {
    let _serial = UI_THREAD.lock().unwrap();
    let mut first = App::headless(320., 240.).unwrap();
    let mut second = App::headless(320., 240.).unwrap();
    let source = IconSource::svg(SQUARE);
    let resource = first
        .create_icon_resource(&source, IconLimits::default())
        .unwrap();
    let other = second
        .create_icon_resource(&source, IconLimits::default())
        .unwrap();
    assert_eq!(second.icon_info(resource), Err(Error(3)));
    assert_eq!(second.release_icon_resource(resource), Err(Error(3)));
    assert_eq!(first.icon_info(other), Err(Error(3)));
    for bad in [
        IconSource::path("embedded\0nul"),
        IconSource::named("é".repeat(2049)),
    ] {
        assert_eq!(first.set_icon_source(resource, &bad), Err(Error(3)));
    }
    assert_eq!(
        first.set_icon_limits(
            resource,
            IconLimits {
                max_versions: 129,
                ..Default::default()
            }
        ),
        Err(Error(3))
    );
    assert_eq!(
        first.prepare_icon(resource, 12., 12., IconTransform([f32::NAN; 6])),
        Err(Error(3))
    );
    assert_eq!(first.icon_source(resource).unwrap(), source);
    let tiny = IconLimits {
        max_svg_bytes: 1,
        ..Default::default()
    };
    for _ in 0..130 {
        assert!(first.create_icon_resource(&source, tiny).is_err());
    }
    first.release_icon_resource(resource).unwrap();
    let replacement = first
        .create_icon_resource(&source, IconLimits::default())
        .unwrap();
    assert_ne!(resource, replacement);
    assert_eq!(first.icon_info(resource), Err(Error(4)));
    assert_eq!(first.release_icon_resource(resource), Err(Error(4)));
    assert_eq!(prepare(&mut first, replacement), IconError::Ok);
    first.close().unwrap();
    assert_eq!(first.icon_info(replacement), Err(Error(2)));
    assert_eq!(prepare(&mut second, other), IconError::Ok);
}
