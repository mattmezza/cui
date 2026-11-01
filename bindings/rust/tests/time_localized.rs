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
    app.step(0.016).unwrap();
    app.localized_time_result(n).expect("ordinary admission");
    let r = app.bounds(n).unwrap();
    app.pointer(Phase::Down, r.x + r.width / 4., r.y + r.height / 2.)
        .unwrap();
    app.pointer(Phase::Up, r.x + r.width / 4., r.y + r.height / 2.)
        .unwrap();
    app.queue_text("10:45").unwrap();
    app.step(0.016).unwrap();
    assert!(seen.borrow().is_empty());
    app.dispatch_time_field_changes().unwrap();
    eprintln!("bounds={r:?} result={:?}", app.localized_time_result(n));
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
    app.step(0.016).unwrap();
    app.localized_time_result(n).expect("ordinary admission");
    let r = app.bounds(n).unwrap();
    app.pointer(Phase::Down, r.x + r.width / 4., r.y + r.height / 2.)
        .unwrap();
    app.pointer(Phase::Up, r.x + r.width / 4., r.y + r.height / 2.)
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

#[test]
fn zero_generation_rejected_before_creation() {
    let mut app = App::headless(600., 240.).unwrap();
    let root = app.root(Kind::Column, "root").unwrap();
    let mut c = TimeFieldConfig::english();
    assert_eq!(c.edit_generation, 1);
    assert_eq!(TimeFieldConfig::arabic().edit_generation, 1);
    c.edit_generation = 0;
    assert!(app
        .localized_time_field(
            root,
            "zero",
            Time {
                hour: 9,
                minute: 0,
                second: 0
            },
            &c
        )
        .is_err());
    assert!(app.child(root, Kind::TimeField, "zero").is_ok());
}
#[test]
fn optional_provider_preserves_scalar_default() {
    if std::env::var_os("CUI_TEST_TIME_PROVIDER").is_none() {
        return;
    }
    let mut app = App::headless(600., 240.).unwrap();
    assert!(!app.supports_localized_time_field());
    let root = app.root(Kind::Column, "root").unwrap();
    let n = app.child(root, Kind::TimeField, "legacy").unwrap();
    app.time(
        n,
        Time {
            hour: 9,
            minute: 30,
            second: 0,
        },
    )
    .unwrap();
    app.step(0.016).unwrap();
    assert_eq!(app.enable_localized_time_fields(2), Err(Error(9)));
    assert!(app
        .configure_localized_time(n, &TimeFieldConfig::english())
        .is_err());
}
fn paste(app: &mut App, n: Node, text: &str) {
    let r = app.bounds(n).unwrap();
    app.pointer(Phase::Down, r.x + r.width / 4., r.y + r.height / 2.)
        .unwrap();
    app.pointer(Phase::Up, r.x + r.width / 4., r.y + r.height / 2.)
        .unwrap();
    app.queue_text(text).unwrap();
    app.step(0.016).unwrap();
}
#[test]
fn arabic_readonly_disabled_and_validation_clear() {
    let mut app = App::headless(600., 240.).unwrap();
    let root = app.root(Kind::Column, "root").unwrap();
    let mut c = TimeFieldConfig::arabic();
    c.minimum = Some(Time {
        hour: 8,
        minute: 0,
        second: 0,
    });
    c.maximum = Some(Time {
        hour: 18,
        minute: 0,
        second: 0,
    });
    let n = app
        .localized_time_field(
            root,
            "arabic",
            Time {
                hour: 14,
                minute: 15,
                second: 0,
            },
            &c,
        )
        .unwrap();
    app.step(0.016).unwrap();
    app.step(0.016).unwrap();
    paste(&mut app, n, "٠٣:٢٠ م");
    assert_eq!(
        app.localized_time_result(n).unwrap().change.value,
        Time {
            hour: 15,
            minute: 20,
            second: 0
        }
    );
    assert_eq!(app.dispatch_time_field_changes().unwrap(), 1);
    assert!(app.poll_time_field_change().unwrap().is_none());
    for disabled in [false, true] {
        let mut next = c.clone();
        next.readonly = !disabled;
        next.disabled = disabled;
        app.configure_localized_time(n, &next).unwrap();
        app.step(0.016).unwrap();
        app.step(0.016).unwrap();
        paste(&mut app, n, "٠٤:٣٠ م");
        assert_eq!(app.localized_time_result(n).unwrap().change.value.hour, 15);
    }
    app.configure_localized_time(n, &c).unwrap();
    app.step(0.016).unwrap();
    app.step(0.016).unwrap();
    paste(&mut app, n, "١١:٣٠ م");
    let result = app.localized_time_result(n).unwrap();
    assert_eq!(result.change.validation, 5);
    assert!(!result.changed);
    app.clear_time_validation(n).unwrap();
    app.step(0.016).unwrap();
    assert_eq!(app.localized_time_result(n).unwrap().change.validation, 0);
}
