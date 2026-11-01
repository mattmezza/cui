use cui_builders::*;
use std::{cell::RefCell, rc::Rc};
#[test]
fn applied_owned_changes_and_atomic_configuration() {
    let mut app = App::headless(600., 240.).unwrap();
    assert!(app.supports_localized_time_field());
    app.enable_localized_time_fields(4).unwrap();
    let root = app.root(Kind::Column, "root").unwrap();
    let c = TimeFieldConfig::english();
    let n = app
        .localized_time_field(
            root,
            "local",
            Time {
                hour: 9,
                minute: 30,
                second: 0,
            },
            &c,
        )
        .unwrap();
    let seen = Rc::new(RefCell::new(Vec::new()));
    let out = seen.clone();
    app.on_time_field_change(n, move |v| out.borrow_mut().push(v.clone()))
        .unwrap();
    app.step(0.016).unwrap();
    let r = app.bounds(n).unwrap();
    app.pointer(Phase::Down, r.x + r.width / 2., r.y + r.height / 2.)
        .unwrap();
    app.pointer(Phase::Up, r.x + r.width / 2., r.y + r.height / 2.)
        .unwrap();
    app.queue_text("10:45").unwrap();
    app.step(0.016).unwrap();
    assert!(seen.borrow().is_empty());
    app.dispatch_time_field_changes().unwrap();
    assert_eq!(seen.borrow().len(), 1);
    assert_eq!(seen.borrow()[0].previous.hour, 9);
    assert_eq!(seen.borrow()[0].value.hour, 10);
    assert!(app.poll_time_field_change().unwrap().is_none());
    let before = app.localized_time_config(n).unwrap();
    let mut bad = before.clone();
    bad.tokens[0] = String::new();
    assert!(app.configure_localized_time(n, &bad).is_err());
    assert_eq!(app.localized_time_config(n).unwrap(), before);
    app.remove(n).unwrap();
    drop(app);
    assert_eq!(seen.borrow()[0].value.minute, 45);
}
#[test]
fn wrong_kind_invalid_constructor_and_capacity_are_atomic() {
    let mut app = App::headless(600., 240.).unwrap();
    app.enable_localized_time_fields(1).unwrap();
    let root = app.root(Kind::Column, "root").unwrap();
    assert!(app
        .configure_localized_time(root, &TimeFieldConfig::english())
        .is_err());
    let one = app
        .localized_time_field(
            root,
            "one",
            Time {
                hour: 9,
                minute: 0,
                second: 0,
            },
            &TimeFieldConfig::english(),
        )
        .unwrap();
    assert!(app
        .localized_time_field(
            root,
            "two",
            Time {
                hour: 9,
                minute: 0,
                second: 0
            },
            &TimeFieldConfig::arabic()
        )
        .is_err());
    app.remove(one).unwrap();
    assert!(app
        .localized_time_field(
            root,
            "two",
            Time {
                hour: 9,
                minute: 0,
                second: 0
            },
            &TimeFieldConfig::arabic()
        )
        .is_ok());
}
#[test]
fn panic_consumes_current_record_and_resets_dispatch_guard() {
    let mut app = App::headless(600., 240.).unwrap();
    let root = app.root(Kind::Column, "root").unwrap();
    let n = app
        .localized_time_field(
            root,
            "local",
            Time {
                hour: 9,
                minute: 0,
                second: 0,
            },
            &TimeFieldConfig::english(),
        )
        .unwrap();
    app.on_time_field_change(n, |_| panic!("application callback"))
        .unwrap();
    app.step(0.016).unwrap();
    let r = app.bounds(n).unwrap();
    app.pointer(Phase::Down, r.x + r.width / 2., r.y + r.height / 2.)
        .unwrap();
    app.pointer(Phase::Up, r.x + r.width / 2., r.y + r.height / 2.)
        .unwrap();
    app.queue_text("10:30").unwrap();
    app.step(0.016).unwrap();
    assert!(std::panic::catch_unwind(std::panic::AssertUnwindSafe(
        || app.dispatch_time_field_changes()
    ))
    .is_err());
    assert_eq!(app.dispatch_time_field_changes().unwrap(), 0);
    app.step(0.016).unwrap();
}
