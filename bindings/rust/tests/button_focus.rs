use cui_builders::{App, ButtonFocusMode, Error, Kind};
#[test]
fn optional_button_focus_lifecycle() {
    let mut app = App::headless(320., 240.).unwrap();
    let root = app.root(Kind::Column, "root").unwrap();
    let button = app
        .build_child(root, Kind::Button, "save")
        .unwrap()
        .button_focus(ButtonFocusMode::Legacy)
        .unwrap()
        .finish();
    if std::env::var("CUI_TEST_UNSUPPORTED_BUTTON_FOCUS").as_deref() == Ok("1") {
        assert!(!app.supports_button_focus());
        app.button_focus(button, ButtonFocusMode::Legacy).unwrap();
        assert_eq!(
            app.button_focus(button, ButtonFocusMode::OuterRing).err(),
            Some(Error::BUTTON_FOCUS_UNAVAILABLE)
        );
        app.step(0.01).unwrap();
        return;
    }
    assert!(app.supports_button_focus());
    for mode in [
        ButtonFocusMode::Variant,
        ButtonFocusMode::OuterRing,
        ButtonFocusMode::Legacy,
    ] {
        app.button_focus(button, mode).unwrap();
        app.step(0.01).unwrap();
    }
    assert_eq!(
        app.button_focus(root, ButtonFocusMode::Variant).err(),
        Some(Error(8))
    );
    app.remove(button).unwrap();
    assert!(app.button_focus(button, ButtonFocusMode::Legacy).is_err());
}
