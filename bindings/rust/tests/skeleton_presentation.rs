use cui_builders::{App, Error, Kind, SkeletonConfig, SkeletonWidth};
use std::sync::Mutex;

static UI_THREAD: Mutex<()> = Mutex::new(());

#[test]
fn responsive_geometry_and_legacy_coexist() {
    let _serial = UI_THREAD.lock().unwrap();
    for width in [320., 480.] {
        let mut app = App::headless(width, 220.).unwrap();
        assert!(app.supports_skeleton_presentation());
        let root = app.root(Kind::Column, "root").unwrap();
        let mut style = SkeletonConfig {
            width: SkeletonWidth::Fraction(0.65),
            radius: 6.,
            secondary: true,
            background_rgba: Some(0),
            ..Default::default()
        };
        let copy = style;
        let line = app.responsive_skeleton(root, "line", style).unwrap();
        style.width = SkeletonWidth::Fraction(0.4);
        assert_ne!(style, copy);
        let capped = app
            .build_child(root, Kind::Skeleton, "capped")
            .unwrap()
            .skeleton_presentation(SkeletonConfig {
                width: SkeletonWidth::Grow,
                max_width: 128.,
                height: 12.,
                ..Default::default()
            })
            .unwrap()
            .finish();
        let circle = app
            .responsive_skeleton(
                root,
                "circle",
                SkeletonConfig {
                    width: SkeletonWidth::Fixed(40.),
                    height: 40.,
                    radius: 20.,
                    ..Default::default()
                },
            )
            .unwrap();
        let legacy = app.child(root, Kind::Skeleton, "legacy").unwrap();
        app.visual_size(legacy, 72., 12.).unwrap();
        assert_eq!(app.skeleton_config(line).unwrap(), Some(copy));
        assert!(app.skeleton_config(legacy).unwrap().is_none());
        assert!(app.skeleton_presentation(line).unwrap().is_none());
        app.step(0.25).unwrap();
        for (node, expected_width, expected_height) in [
            (line, (width - 32.) * 0.65, 16.),
            (capped, 128., 12.),
            (circle, 40., 40.),
            (legacy, 72., 12.),
        ] {
            let bounds = app.bounds(node).unwrap();
            assert!((bounds.width - expected_width).abs() < 0.001, "{bounds:?}");
            assert_eq!(bounds.height, expected_height);
        }
        assert!(app.visual_size(line, 1., 1.).is_err());
    }
}

#[test]
fn clock_and_owned_samples_follow_node_lifecycle() {
    let _serial = UI_THREAD.lock().unwrap();
    let mut app = App::headless(320., 240.).unwrap();
    let root = app.root(Kind::Column, "root").unwrap();
    let panel = app.child(root, Kind::Collapsible, "panel").unwrap();
    app.text(panel, "Profile")
        .unwrap()
        .open(panel, true)
        .unwrap();
    let mut config = SkeletonConfig::default();
    let line = app.responsive_skeleton(panel, "line", config).unwrap();
    app.step(0.25).unwrap();
    let first = app.skeleton_presentation(line).unwrap().unwrap();
    assert_eq!(first.frame_sequence, 1);
    assert_eq!(first.core_time, 0.25);
    assert_eq!(first.phase, 0.);
    assert!((first.next_wake_seconds - first.core_time - 1. / 60.).abs() < 1e-10);

    let bad = SkeletonConfig {
        max_width: 20.,
        ..config
    };
    assert!(app.configure_skeleton(line, bad).is_err());
    assert_eq!(app.skeleton_config(line).unwrap(), Some(config));
    assert_eq!(app.skeleton_presentation(line).unwrap(), Some(first));

    config.secondary = true;
    app.configure_skeleton(line, config).unwrap();
    assert!(app.skeleton_presentation(line).unwrap().is_none());
    app.step(0.5).unwrap();
    let next = app.skeleton_presentation(line).unwrap().unwrap();
    assert_eq!(
        (next.frame_sequence, next.phase, next.opacity),
        (2, 0.5, 0.75)
    );
    app.open(panel, false).unwrap();
    app.step(0.25).unwrap();
    assert!(app.skeleton_presentation(line).unwrap().is_none());
    app.open(panel, true).unwrap();
    app.step(0.25).unwrap();
    assert_eq!(app.skeleton_presentation(line).unwrap().unwrap().phase, 0.);
    config.reduced_motion = true;
    app.configure_skeleton(line, config).unwrap();
    app.step(0.25).unwrap();
    let stopped = app.skeleton_presentation(line).unwrap().unwrap();
    assert_eq!((stopped.next_wake_seconds, stopped.opacity), (0., 1.));

    app.remove(panel).unwrap();
    assert!(app.skeleton_presentation(line).is_err());
    app.step(0.25).unwrap();
    drop(app);
    assert_eq!((first.frame_sequence, first.phase), (1, 0.));
}

#[test]
fn invalid_constructor_and_foreign_handles_are_rejected() {
    let _serial = UI_THREAD.lock().unwrap();
    let mut first = App::headless(320., 240.).unwrap();
    let mut second = App::headless(320., 240.).unwrap();
    let root = first.root(Kind::Column, "root").unwrap();
    let foreign = second.root(Kind::Column, "foreign").unwrap();
    let config = SkeletonConfig::default();
    let bad = SkeletonConfig {
        height: f32::NAN,
        ..config
    };
    assert!(first.responsive_skeleton(root, "line", bad).is_err());
    assert!(first.responsive_skeleton(foreign, "line", config).is_err());
    let line = first.responsive_skeleton(root, "line", config).unwrap();
    assert!(first.configure_skeleton(root, config).is_err());
    assert!(second.configure_skeleton(line, config).is_err());
    assert!(second.skeleton_config(line).is_err());
    assert!(second.skeleton_presentation(line).is_err());
    assert_eq!(first.skeleton_config(line).unwrap(), Some(config));
}

#[test]
fn optional_provider_preserves_legacy() {
    if std::env::var("CUI_TEST_UNSUPPORTED_SKELETON").as_deref() != Ok("1") {
        return;
    }
    let _serial = UI_THREAD.lock().unwrap();
    let mut app = App::headless(320., 240.).unwrap();
    assert!(!app.supports_skeleton_presentation());
    let root = app.root(Kind::Column, "root").unwrap();
    let config = SkeletonConfig::default();
    assert!(matches!(
        app.responsive_skeleton(root, "line", config),
        Err(Error(9))
    ));
    let line = app.child(root, Kind::Skeleton, "line").unwrap();
    assert!(matches!(
        app.configure_skeleton(line, config),
        Err(Error(9))
    ));
    app.visual_size(line, 72., 12.).unwrap();
    app.step(0.25).unwrap();
    let bounds = app.bounds(line).unwrap();
    assert_eq!((bounds.width, bounds.height), (72., 12.));
}
