use cui_builders::{App, Error, Kind, Phase, SurfaceResult};
use std::{cell::Cell, rc::Rc};

#[test]
fn responsive_notifications() {
    let mut app = App::headless(220., 320.).unwrap();
    let root = app.root(Kind::Column, "root").unwrap();
    if std::env::var("CUI_TEST_UNSUPPORTED_RESPONSIVE_NOTIFICATIONS").as_deref() == Ok("1") {
        assert!(!app.supports_responsive_notifications());
        for enabled in [false, true] {
            assert_eq!(
                app.set_responsive_notifications(enabled).err(),
                Some(Error(9))
            );
        }
        app.build_child(root, Kind::Notification, "legacy")
            .unwrap()
            .text("Saved")
            .unwrap()
            .detail_text("Body")
            .unwrap()
            .open(true)
            .unwrap()
            .finish();
        app.step(0.4).unwrap();
        return;
    }
    assert!(app.supports_responsive_notifications());
    let notice = app
        .build_child(root, Kind::Notification, "notice")
        .unwrap()
        .text("Saved")
        .unwrap()
        .detail_text(&"Your changes are safely stored. ".repeat(8))
        .unwrap()
        .open(true)
        .unwrap()
        .finish();
    let clicks = Rc::new(Cell::new(0));
    let captured = clicks.clone();
    app.on_click(notice, move || captured.set(captured.get() + 1))
        .unwrap();
    let closes = Rc::new(Cell::new(0));
    let captured = closes.clone();
    app.on_close_event(notice, move |event| {
        assert_eq!(event.surface_result(), Some(SurfaceResult::Closed));
        captured.set(captured.get() + 1);
    })
    .unwrap();
    app.step(0.4).unwrap();
    app.step(0.3).unwrap();
    let legacy = app.bounds(notice).unwrap();
    app.set_responsive_notifications(true).unwrap();
    app.step(0.4).unwrap();
    app.step(0.3).unwrap();
    let responsive = app.bounds(notice).unwrap();
    assert!(
        responsive.height > legacy.height,
        "legacy={legacy:?} responsive={responsive:?}"
    );
    let (x, y) = (responsive.x + responsive.width - 18., responsive.y + 18.);
    app.pointer(Phase::Down, x, y).unwrap();
    app.pointer(Phase::Up, x, y).unwrap();
    app.frame(0.016).unwrap();
    app.frame(0.4).unwrap();
    assert_eq!(clicks.get(), 0);
    assert_eq!(closes.get(), 1);
    assert!(!app.is_open(notice).unwrap());
    app.set_responsive_notifications(false).unwrap();
}
