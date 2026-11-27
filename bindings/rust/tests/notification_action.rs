use cui_builders::{App, Error, Kind, Phase};
use std::{cell::Cell, rc::Rc};

#[test]
fn notification_action_lifetime_and_click() {
    let mut app = App::headless(640., 480.).unwrap();
    let root = app.root(Kind::Column, "root").unwrap();
    let note = app.child(root, Kind::Notification, "note").unwrap();
    app.text(note, "Saved").unwrap();
    app.open(note, true).unwrap();
    let supported = app.supports_notification_action();
    if let Ok(expected) = std::env::var("CUI_EXPECT_NOTIFICATION_ACTION") {
        assert_eq!(supported, expected == "1");
    }
    if !supported {
        assert_eq!(
            app.notification_action(note, "action", "Undo").err(),
            Some(Error::NOTIFICATION_ACTION_UNAVAILABLE)
        );
        return;
    }
    assert!(app.notification_action(root, "invalid", "Undo").is_err());
    assert!(app
        .notification_action(note, "invalid", "bad\0label")
        .is_err());
    let clicks = Rc::new(Cell::new(0));
    let capture = Rc::new(());
    let weak = Rc::downgrade(&capture);
    let action = app
        .build_notification_action(note, "action", "Undo")
        .unwrap()
        .on_click({
            let clicks = clicks.clone();
            move || {
                let _ = &capture;
                clicks.set(clicks.get() + 1);
            }
        })
        .unwrap()
        .finish();
    assert!(app.notification_action(note, "duplicate", "Again").is_err());
    app.step(0.016).unwrap();
    let b = app.bounds(action).unwrap();
    assert!(b.width > 0. && b.height > 0.);
    for phase in [Phase::Down, Phase::Up] {
        app.pointer(phase, b.x + b.width / 2., b.y + b.height / 2.)
            .unwrap();
    }
    app.step(0.016).unwrap();
    app.dispatch().unwrap();
    assert_eq!(clicks.get(), 1);
    assert!(app.is_open(note).unwrap());
    app.step(0.016).unwrap();
    assert!(app.bounds(action).unwrap().width > 0.);
    app.remove(note).unwrap();
    assert!(app.bounds(action).is_err());
    assert!(weak.upgrade().is_none());
}
