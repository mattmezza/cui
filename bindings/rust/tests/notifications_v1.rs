use cui_builders::*;
fn press(a: &mut App, n: Node) {
    let b = a.bounds(n).unwrap();
    a.pointer(Phase::Down, b.x + b.width / 2., b.y + b.height / 2.)
        .unwrap();
    a.pointer(Phase::Up, b.x + b.width / 2., b.y + b.height / 2.)
        .unwrap();
    a.step(0.016).unwrap();
}
#[test]
fn real_queue_fifo_and_stale() {
    if std::env::var_os("CUI_NOTIFICATION_FIXTURE").is_some()
        || std::env::var_os("CUI_NOTIFICATION_UNAVAILABLE").is_some()
    {
        return;
    }
    let mut a = App::headless(500., 300.).unwrap();
    let root = a.root(Kind::Column, "root").unwrap();
    let n = a.child(root, Kind::Checkbox, "check").unwrap();
    a.text(n, "Check").unwrap();
    a.step(0.016).unwrap();
    press(&mut a, n);
    press(&mut a, n);
    match a.poll_notification_v1().unwrap().unwrap() {
        NotificationV1::BooleanChange(m, v) => {
            assert!(v);
            assert!(m.has_frame);
            assert_eq!(m.node, Some(n));
        }
        x => panic!("{x:?}"),
    }
    match a.poll_notification_v1().unwrap().unwrap() {
        NotificationV1::BooleanChange(_, v) => assert!(!v),
        x => panic!("{x:?}"),
    }
    press(&mut a, n);
    a.remove(n).unwrap();
    assert!(a.poll_notification_v1().unwrap().is_none());
}
#[test]
fn injected_protocol() {
    let Ok(mode) = std::env::var("CUI_NOTIFICATION_FIXTURE") else {
        return;
    };
    let mut a = App::headless(100., 100.).unwrap();
    if mode != "edges" && mode != "capacity" {
        assert!(a.poll_notification_v1().is_err());
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
        let n = a.poll_notification_v1().unwrap().unwrap();
        let m = match n {
            NotificationV1::Activation(m) => m,
            NotificationV1::BooleanChange(m, v) => {
                assert!(v);
                m
            }
            NotificationV1::TableChange(m, t) => {
                assert_eq!(t.column, u32::MAX - 1);
                assert_eq!(i, 2);
                m
            }
            NotificationV1::SurfaceClose(m, _) => m,
            x => panic!("{x:?}"),
        };
        assert_eq!(m.frame, frame);
        assert_eq!(m.legacy.frame, frame);
        assert_eq!(m.handle.slot, u32::MAX);
        assert_eq!(m.legacy.checked_raw, u32::MAX);
        if mode == "capacity" {
            assert_eq!(m.legacy.text.len(), 8192);
        }
    }
}

#[test]
fn real_text_and_table() {
    if std::env::var_os("CUI_NOTIFICATION_FIXTURE").is_some()
        || std::env::var_os("CUI_NOTIFICATION_UNAVAILABLE").is_some()
    {
        return;
    }
    let mut a = App::headless(600., 400.).unwrap();
    let root = a.root(Kind::Column, "root").unwrap();
    let input = a.child(root, Kind::Input, "text").unwrap();
    a.text(input, "hé").unwrap();
    let table = a.child(root, Kind::DataTable, "table").unwrap();
    a.data_table_selection_mode(table, DataTableSelectionMode::Cell)
        .unwrap();
    a.data_table(table, &["A", "B", "C"], &[&["a", "b", "c"]])
        .unwrap();
    a.step(0.016).unwrap();
    press(&mut a, input);
    a.queue_text("Z").unwrap();
    a.step(0.016).unwrap();
    let saved = match a.poll_notification_v1().unwrap().unwrap() {
        NotificationV1::Legacy(m) => m.legacy.text,
        x => panic!("{x:?}"),
    };
    assert_eq!(saved, "héZ");
    a.queue_text("Q").unwrap();
    a.step(0.016).unwrap();
    a.poll_notification_v1().unwrap();
    assert_eq!(saved, "héZ");
    let b = a.data_table_cell_bounds(table, 0, 2).unwrap();
    a.pointer(Phase::Down, b.x + b.width / 2., b.y + b.height / 2.)
        .unwrap();
    a.pointer(Phase::Up, b.x + b.width / 2., b.y + b.height / 2.)
        .unwrap();
    a.step(0.016).unwrap();
    match a.poll_notification_v1().unwrap().unwrap() {
        NotificationV1::TableChange(_, t) => assert_eq!(t.column, 3),
        x => panic!("{x:?}"),
    }
}

#[test]
fn unavailable_old_library() {
    if std::env::var_os("CUI_NOTIFICATION_UNAVAILABLE").is_none() {
        return;
    }
    let mut a = App::headless(100., 100.).unwrap();
    assert_eq!(a.poll_notification_v1().unwrap_err(), Error(9));
    a.root(Kind::Column, "root").unwrap();
    a.step(0.016).unwrap();
}
