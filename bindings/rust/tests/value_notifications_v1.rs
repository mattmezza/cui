use cui_builders::*;
fn press(a: &mut App, n: Node) {
    let b = a.bounds(n).unwrap();
    a.pointer(Phase::Down, b.x + b.width / 2., b.y + b.height / 2.)
        .unwrap();
    a.pointer(Phase::Up, b.x + b.width / 2., b.y + b.height / 2.)
        .unwrap();
    a.step(0.016).unwrap();
}
fn real() -> bool {
    std::env::var_os("CUI_VALUE_REAL").is_some()
}
#[test]
fn real_scalar_precision_silence_and_lifetime() {
    if !real() {
        return;
    }
    let saved;
    {
        let mut a = App::headless(800., 800.).unwrap();
        let root = a.root(Kind::Column, "root").unwrap();
        let n = a.child(root, Kind::NumberInput, "number").unwrap();
        a.range(n, 0., 100000000., 0.25)
            .unwrap()
            .value(n, 16777216.25)
            .unwrap();
        let calls = std::rc::Rc::new(std::cell::Cell::new(0));
        let counter = calls.clone();
        a.on_change(n, move || counter.set(counter.get() + 1))
            .unwrap();
        a.step(0.016).unwrap();
        press(&mut a, n);
        a.key(true, Key::Up, 0, false).unwrap();
        a.step(0.016).unwrap();
        saved = a.poll_value_notification_v1().unwrap().unwrap();
        match &saved {
            ValueNotificationV1::ScalarChange(m, v) => {
                assert_eq!(*v, 16777216.5);
                assert_eq!(m.legacy.value, *v);
                assert_eq!(m.node, Some(n));
            }
            x => panic!("{x:?}"),
        }
        assert_eq!(calls.get(), 0);
        a.value(n, 42.).unwrap();
        a.step(0.016).unwrap();
        assert!(a.poll_value_notification_v1().unwrap().is_none());
        a.component_disabled(n, true).unwrap();
        press(&mut a, n);
        a.key(true, Key::Up, 0, false).unwrap();
        a.step(0.016).unwrap();
        assert!(a.poll_value_notification_v1().unwrap().is_none());
    }
    assert!(matches!(
        saved,
        ValueNotificationV1::ScalarChange(_, 16777216.5)
    ));
}
#[test]
fn real_text_fifo_or_older_library_no_drain() {
    if !real() && std::env::var_os("CUI_VALUE_OLD").is_none() {
        return;
    }
    let saved;
    {
        let mut a = App::headless(600., 400.).unwrap();
        let root = a.root(Kind::Column, "root").unwrap();
        let n = a.child(root, Kind::Input, "text").unwrap();
        a.step(0.016).unwrap();
        press(&mut a, n);
        a.queue_text("héllo ✓").unwrap();
        a.step(0.016).unwrap();
        if std::env::var_os("CUI_VALUE_OLD").is_some() {
            assert!(a.poll_value_notification_v1().is_err());
            match a.poll_notification_v1().unwrap().unwrap() {
                NotificationV1::Legacy(m) => assert_eq!(m.legacy.text, "héllo ✓"),
                x => panic!("{x:?}"),
            };
            return;
        }
        assert!(matches!(
            a.poll_notification_v1().unwrap(),
            Some(NotificationV1::Legacy(_))
        ));
        a.queue_text("!").unwrap();
        a.step(0.016).unwrap();
        a.queue_text("?").unwrap();
        a.step(0.016).unwrap();
        saved = a.poll_value_notification_v1().unwrap().unwrap();
        match &saved {
            ValueNotificationV1::TextChange(m, text) => {
                assert_eq!(text, "héllo ✓!");
                assert_eq!(m.legacy.text, *text);
            }
            x => panic!("{x:?}"),
        }
        match a.poll_notification_v1().unwrap().unwrap() {
            NotificationV1::Legacy(m) => assert_eq!(m.legacy.text, "héllo ✓!?"),
            x => panic!("{x:?}"),
        }
    }
    assert!(matches!(saved,ValueNotificationV1::TextChange(_,ref s) if s=="héllo ✓!"));
}
#[test]
fn real_selection_remove_reuse_full_generation() {
    if !real() {
        return;
    }
    let mut a = App::headless(800., 800.).unwrap();
    let root = a.root(Kind::Column, "root").unwrap();
    let group = a.child(root, Kind::RadioGroup, "group").unwrap();
    let first = a.child(group, Kind::Item, "first").unwrap();
    let second = a.child(group, Kind::Item, "second").unwrap();
    a.text(first, "First")
        .unwrap()
        .text(second, "Second")
        .unwrap()
        .select(group, first)
        .unwrap();
    a.step(0.016).unwrap();
    press(&mut a, second);
    let old_handle = match a.poll_value_notification_v1().unwrap().unwrap() {
        ValueNotificationV1::SelectionChange(_, v) => {
            assert_eq!(v.node, Some(second));
            v.handle
        }
        x => panic!("{x:?}"),
    };
    press(&mut a, first);
    assert!(matches!(
        a.poll_notification_v1().unwrap(),
        Some(NotificationV1::Legacy(_))
    ));
    press(&mut a, second);
    a.remove(second).unwrap();
    let replacement = a.child(group, Kind::Item, "replacement").unwrap();
    a.text(replacement, "Replacement")
        .unwrap()
        .select(group, first)
        .unwrap();
    a.step(0.016).unwrap();
    press(&mut a, replacement);
    match a.poll_value_notification_v1().unwrap().unwrap() {
        ValueNotificationV1::SelectionChange(_, v) => {
            assert_eq!(v.handle, old_handle);
            assert_eq!(v.node, None);
        }
        x => panic!("{x:?}"),
    }
    match a.poll_value_notification_v1().unwrap().unwrap() {
        ValueNotificationV1::SelectionChange(_, v) => {
            assert_eq!(v.node, Some(replacement));
            assert_eq!(v.handle.slot, old_handle.slot);
            assert_ne!(v.handle.generation, old_handle.generation);
        }
        x => panic!("{x:?}"),
    }
}
#[test]
fn synthetic_protocol() {
    let Ok(mode) = std::env::var("CUI_VALUE_FIXTURE") else {
        return;
    };
    let mut a = App::headless(100., 100.).unwrap();
    if mode != "edges" && mode != "capacity" {
        assert!(a.poll_value_notification_v1().is_err());
        return;
    }
    for (i, frame) in [
        9007199254740991,
        9007199254740992,
        9007199254740993,
        u64::MAX,
    ]
    .into_iter()
    .enumerate()
    {
        let event = a.poll_value_notification_v1().unwrap().unwrap();
        let meta = match event {
            ValueNotificationV1::ScalarChange(m, v) => {
                assert_eq!(i, 0);
                assert_eq!(v, 16777216.5);
                m
            }
            ValueNotificationV1::TextChange(m, s) => {
                assert_eq!(i, 1);
                assert_eq!(s.len(), if mode == "capacity" { 8192 } else { 3 });
                m
            }
            ValueNotificationV1::SelectionChange(m, v) => {
                assert!(i >= 2);
                assert_eq!(
                    v.handle,
                    RawHandle {
                        slot: u32::MAX,
                        generation: u32::MAX
                    }
                );
                assert_eq!(v.node, None);
                m
            }
            x => panic!("{x:?}"),
        };
        assert_eq!(meta.frame, frame);
        assert_eq!(meta.handle.slot, u32::MAX);
        assert_eq!(meta.node, None);
    }
}
// Deliberately exhaustive: additive value API does not extend the original enum.
fn old_exhaustive(n: NotificationV1) {
    match n {
        NotificationV1::Activation(_)
        | NotificationV1::BooleanChange(_, _)
        | NotificationV1::TableChange(_, _)
        | NotificationV1::SurfaceClose(_, _)
        | NotificationV1::Legacy(_) => {}
    }
}
#[test]
fn old_enum_remains_exhaustive() {
    let _ = old_exhaustive as fn(NotificationV1);
}

#[test]
fn real_initial_boolean_and_table_views() {
    if !real() {
        return;
    }
    let mut a = App::headless(1000., 600.).unwrap();
    let root = a.root(Kind::Column, "root").unwrap();
    let check = a.child(root, Kind::Checkbox, "check").unwrap();
    a.text(check, "Check").unwrap();
    let table = a.child(root, Kind::DataTable, "table").unwrap();
    a.data_table_selection_mode(table, DataTableSelectionMode::Cell)
        .unwrap();
    a.data_table(table, &["A", "B", "C"], &[&["a", "b", "c"]])
        .unwrap();
    a.step(0.016).unwrap();
    press(&mut a, check);
    assert!(matches!(
        a.poll_value_notification_v1().unwrap(),
        Some(ValueNotificationV1::BooleanChange(_, true))
    ));
    let b = a.data_table_cell_bounds(table, 0, 2).unwrap();
    a.pointer(Phase::Down, b.x + b.width / 2., b.y + b.height / 2.)
        .unwrap();
    a.pointer(Phase::Up, b.x + b.width / 2., b.y + b.height / 2.)
        .unwrap();
    a.step(0.016).unwrap();
    match a.poll_value_notification_v1().unwrap().unwrap() {
        ValueNotificationV1::TableChange(m, t) => {
            assert_eq!(t.column, 3);
            assert_eq!(m.legacy.checked_raw, 3);
        }
        x => panic!("{x:?}"),
    }
}
