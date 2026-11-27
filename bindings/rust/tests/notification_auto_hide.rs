use cui_builders::{App, Error, Kind, Phase};
use std::{cell::Cell, rc::Rc};

#[test]
fn notification_auto_hide_optional_expiry() {
    let mut app = App::headless(640., 480.).unwrap();
    let root = app.root(Kind::Column, "root").unwrap();
    let note = app.child(root, Kind::Notification, "note").unwrap();
    let supported = app.supports_notification_auto_hide();
    if let Ok(expected) = std::env::var("CUI_EXPECT_NOTIFICATION_AUTO_HIDE") {
        assert_eq!(supported, expected == "1");
    }
    if supported {
        assert!(app.notification_auto_hide(root, true).is_err());
        app.notification_auto_hide(note, true).unwrap();
    } else {
        for enabled in [false, true] {
            assert_eq!(
                app.notification_auto_hide(note, enabled).err(),
                Some(Error::NOTIFICATION_AUTO_HIDE_UNAVAILABLE)
            );
        }
    }
    let closes = Rc::new(Cell::new(0));
    let clicks = Rc::new(Cell::new(0));
    app.on_close_event(note, {
        let closes = closes.clone();
        move |_| closes.set(closes.get() + 1)
    })
    .unwrap();
    app.on_click(note, {
        let clicks = clicks.clone();
        move || clicks.set(clicks.get() + 1)
    })
    .unwrap();
    app.open(note, true).unwrap();
    app.pointer(Phase::Move, -100., -100.).unwrap();
    for _ in 0..90 {
        app.step(0.1).unwrap();
        app.dispatch().unwrap();
    }
    assert_eq!(app.is_open(note).unwrap(), !supported);
    if !supported {
        return;
    }
    assert_eq!(closes.get(), 1);
    assert_eq!(clicks.get(), 0);
    app.notification_auto_hide(note, false).unwrap();
    app.open(note, true).unwrap();
    for _ in 0..90 {
        app.step(0.1).unwrap();
        app.dispatch().unwrap();
    }
    assert!(app.is_open(note).unwrap());
    assert_eq!(closes.get(), 1);
    assert_eq!(clicks.get(), 0);
}
