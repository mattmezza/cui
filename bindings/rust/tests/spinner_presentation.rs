use cui_builders::{App, Kind, SpinnerConfig, SpinnerSize};
#[test]
fn optional_spinner_lifecycle() {
    let mut app = App::headless(320., 240.).unwrap();
    let root = app.root(Kind::Column, "root").unwrap();
    if std::env::var("CUI_TEST_UNSUPPORTED_SPINNER").as_deref() == Ok("1") {
        assert!(!app.supports_spinner_presentation());
        assert!(app
            .rotating_spinner(root, "new", SpinnerConfig::default())
            .is_err());
        let legacy = app.child(root, Kind::Spinner, "old").unwrap();
        app.visual_size(legacy, 20., 20.).unwrap();
        app.step(0.25).unwrap();
        return;
    }
    assert!(app.supports_spinner_presentation());
    for size in [
        SpinnerSize::XSmall,
        SpinnerSize::Small,
        SpinnerSize::Medium,
        SpinnerSize::Large,
        SpinnerSize::Custom(23.),
    ] {
        let config = SpinnerConfig {
            size,
            ..Default::default()
        };
        let node = app.rotating_spinner(root, "size", config.clone()).unwrap();
        assert_eq!(app.spinner_config(node).unwrap(), Some(config));
        app.remove(node).unwrap();
    }
    for config in [
        SpinnerConfig {
            size: SpinnerSize::Large,
            rem_size: 3000.,
            ..Default::default()
        },
        SpinnerConfig {
            icon: "a\0b".into(),
            ..Default::default()
        },
    ] {
        assert!(app.rotating_spinner(root, "bad", config).is_err());
    }
    let mut c = SpinnerConfig::default();
    let original = c.clone();
    let n = app.rotating_spinner(root, "new", c.clone()).unwrap();
    c.icon = "loader".into();
    assert_eq!(app.spinner_config(n).unwrap(), Some(original.clone()));
    assert!(app.spinner_presentation(n).unwrap().is_none());
    app.step(0.25).unwrap();
    let first = app.spinner_presentation(n).unwrap().unwrap();
    assert_eq!(first.phase, 0.);
    app.step(0.2).unwrap();
    let next = app.spinner_presentation(n).unwrap().unwrap();
    assert!(next.phase > 0.);
    assert_eq!(first.phase, 0.);
    c.duration = f32::NAN;
    assert!(app.configure_spinner(n, c.clone()).is_err());
    assert_eq!(app.spinner_presentation(n).unwrap(), Some(next));
    c = original;
    c.reduced_motion = true;
    c.rgba = Some(0);
    app.configure_spinner(n, c.clone()).unwrap();
    assert!(app.spinner_presentation(n).unwrap().is_none());
    app.step(0.2).unwrap();
    let stopped = app.spinner_presentation(n).unwrap().unwrap();
    assert_eq!((stopped.phase, stopped.next_wake_seconds), (0., 0.));
    assert!(app.visual_size(n, 2., 2.).is_err());
    c.size = SpinnerSize::Custom(-1.);
    assert!(app.rotating_spinner(root, "bad", c).is_err());
    app.remove(n).unwrap();
    assert!(app.spinner_presentation(n).is_err());
}
