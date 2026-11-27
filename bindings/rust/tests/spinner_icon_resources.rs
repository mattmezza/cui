use cui_builders::{App, IconLimits, IconSource, Kind, SpinnerConfig};
#[test]
fn shared_spinner_artwork() {
    let mut app = App::headless(320., 240.).unwrap();
    let root = app.root(Kind::Column, "root").unwrap();
    let node = app
        .rotating_spinner(root, "spinner", SpinnerConfig::default())
        .unwrap();
    if std::env::var("CUI_TEST_UNSUPPORTED_SPINNER_ARTWORK").as_deref() == Ok("1") {
        assert!(!app.supports_spinner_icon_resources());
        assert!(app.set_spinner_icon_resource(node, None).is_err());
        assert!(app.spinner_icon_resource(node).is_err());
        app.step(0.1).unwrap();
        assert!(app.spinner_presentation(node).unwrap().is_some());
        return;
    }
    assert!(app.supports_spinner_icon_resources());
    let second = app
        .rotating_spinner(root, "second", SpinnerConfig::default())
        .unwrap();
    let resource = app
        .create_icon_resource(&IconSource::svg(r#"<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24"><path d="M2 2h20v20H2z"/></svg>"#), IconLimits::default())
        .unwrap();
    assert_eq!(app.spinner_icon_resource(node).unwrap(), None);
    app.set_spinner_icon_resource(node, Some(resource)).unwrap();
    assert_eq!(app.spinner_icon_resource(node).unwrap(), Some(resource));
    app.set_spinner_icon_resource(second, Some(resource))
        .unwrap();
    app.step(0.1).unwrap();
    assert!(app.spinner_presentation(node).unwrap().is_some());
    assert!(app.release_icon_resource(resource).is_err());
    let mut other = App::headless(100., 100.).unwrap();
    let foreign = other
        .create_icon_resource(&IconSource::svg(r#"<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24"><path d="M2 2h20v20H2z"/></svg>"#), IconLimits::default())
        .unwrap();
    assert!(app.set_spinner_icon_resource(node, Some(foreign)).is_err());
    assert_eq!(app.spinner_icon_resource(node).unwrap(), Some(resource));
    app.set_spinner_icon_resource(node, None).unwrap();
    assert_eq!(app.spinner_icon_resource(node).unwrap(), None);
    app.remove(second).unwrap();
    app.release_icon_resource(resource).unwrap();
    assert!(app.set_spinner_icon_resource(node, Some(resource)).is_err());
    app.remove(node).unwrap();
    assert!(app.spinner_icon_resource(node).is_err());
}
