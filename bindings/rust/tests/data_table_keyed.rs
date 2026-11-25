use cui_builders::{
    App, DataTableSelectionMode, Error, Key, Kind, Node, Phase, Rect, ValueNotificationV1,
};
use std::{cell::RefCell, rc::Rc};

fn queue_click(app: &mut App, bounds: Rect) {
    let x = bounds.x + bounds.width / 2.;
    let y = bounds.y + bounds.height / 2.;
    app.pointer(Phase::Down, x, y).unwrap();
    app.pointer(Phase::Up, x, y).unwrap();
}

#[test]
fn keyed_replacement_preserves_viewport_selection_and_keyboard_cursor() {
    let mut app = App::headless(480., 320.).unwrap();
    assert!(app.supports_keyed_data_table());
    let root = app.root(Kind::Column, "root").unwrap();
    let headers = ["Subject", "Sender"];
    let mut subject = String::from("Message 2");
    let mut keys = [11, 22, u32::MAX, 44, 55, 66, 77, 88];
    let rows: [&[&str]; 8] = [
        &["Message 0", "Team"],
        &["Message 1", "Team"],
        &["Message 2", "Team"],
        &["Message 3", "Team"],
        &["Message 4", "Team"],
        &["Message 5", "Team"],
        &["Message 6", "Team"],
        &["Message 7", "Team"],
    ];
    let mut initial_rows = rows;
    let subject_row = [subject.as_str(), "Team"];
    initial_rows[2] = &subject_row;
    let table = app
        .build_child(root, Kind::DataTable, "inbox")
        .unwrap()
        .data_table_keyed(&headers, &initial_rows, &keys)
        .unwrap()
        .visual_size(320., 160.)
        .unwrap()
        .data_table_selection_mode(DataTableSelectionMode::Cell)
        .unwrap()
        .finish();
    keys[2] = 999;
    subject.clear();
    assert_eq!(app.data_table_cell(table, 2, 0).unwrap(), "Message 2");

    let changes = Rc::new(RefCell::new(Vec::new()));
    let captured = Rc::clone(&changes);
    app.on_change_event(table, move |change| {
        captured.borrow_mut().push(change.clone())
    })
    .unwrap();
    app.step(0.016).unwrap();
    let cell = |app: &App, row, column| app.data_table_cell_bounds(table, row, column).unwrap();
    let first = cell(&app, 1, 0);
    app.scroll(first.x + 10., first.y + 10., 0., -1.).unwrap();
    app.step(0.016).unwrap();
    let offset = cell(&app, 1, 0).y;
    assert_eq!(offset, first.y - 48.);
    let selected = cell(&app, 2, 1);
    queue_click(&mut app, selected);
    app.frame(0.016).unwrap();
    let expect_selection = |app: &App, key: u32| {
        let state = app.data_table_state(table).unwrap();
        assert_eq!((state.row, state.column), (key, 2));
    };
    expect_selection(&app, u32::MAX);
    let event = changes.borrow_mut().pop().unwrap();
    assert_eq!(event.value, f64::from(u32::MAX));
    assert!(event.text.contains("\"row\":4294967295"));
    assert!(changes.borrow().is_empty());

    for keys in [&[][..], &[0, 22], &[11, 11], &[11, 22, 33]] {
        assert_eq!(
            app.data_table_keyed(table, &headers, &[&["A", "B"], &["C", "D"]], keys)
                .err(),
            Some(Error(3))
        );
    }
    for invalid in ["two\0parts".into(), "é".repeat(65)] {
        assert_eq!(
            app.data_table_keyed(table, &headers, &[&[&invalid, "Team"]], &[44])
                .err(),
            Some(Error(3))
        );
    }
    assert_eq!(app.data_table_cell(table, 2, 0).unwrap(), "Message 2");
    expect_selection(&app, u32::MAX);

    let mut next_rows: Vec<&[&str]> = vec![&["New message", "Owner"]];
    next_rows.extend(rows);
    keys[2] = u32::MAX;
    let next_keys: Vec<_> = [9].into_iter().chain(keys).collect();
    app.data_table_keyed(table, &headers, &next_rows, &next_keys)
        .unwrap();
    app.step(0.016).unwrap();
    assert_eq!(cell(&app, 2, 0).y, offset);
    assert_eq!(cell(&app, 3, 1).y, selected.y);
    expect_selection(&app, u32::MAX);
    assert!(app.poll_value_notification_v1().unwrap().is_none());

    app.key(true, Key::Down, 0, false).unwrap();
    app.step(0.016).unwrap();
    expect_selection(&app, 44);
    let notification = app.poll_value_notification_v1().unwrap();
    let Some(ValueNotificationV1::TableChange(meta, change)) = notification else {
        panic!("expected one typed selection notification, got {notification:?}");
    };
    assert_eq!(meta.node, Some(table));
    assert_eq!((change.reason, change.row, change.column), (1, 44, 2));
    assert!(app.poll_value_notification_v1().unwrap().is_none());
    assert!(changes.borrow().is_empty());

    app.data_table_keyed(table, &headers, &[], &[]).unwrap();
    app.step(0.016).unwrap();
    assert_eq!(app.data_table_state(table).unwrap().row, 0);
    app.data_table(table, &headers, &next_rows[1..]).unwrap();
    app.step(0.016).unwrap();
    assert_eq!(cell(&app, 1, 0).y, first.y);
    let legacy_cell = cell(&app, 1, 1);
    queue_click(&mut app, legacy_cell);
    app.frame(0.016).unwrap();
    expect_selection(&app, 2);
    assert_eq!(changes.borrow().len(), 1);
    assert!(changes.borrow()[0].text.contains("\"row\":2"));
}

#[test]
fn legacy_provider_keeps_positional_tables_usable() {
    if std::env::var("CUI_RUST_KEYED_DATA_TABLE_LEGACY").as_deref() != Ok("1") {
        return;
    }
    let mut app = App::headless(480., 320.).unwrap();
    assert!(!app.supports_keyed_data_table());
    let root = app.root(Kind::Column, "root").unwrap();
    let headers = ["Subject"];
    let table: Node = app
        .build_child(root, Kind::DataTable, "inbox")
        .unwrap()
        .data_table(&headers, &[&["Old provider"], &["Working row"]])
        .unwrap()
        .visual_size(300., 128.)
        .unwrap()
        .finish();
    app.step(0.016).unwrap();
    let bounds = app.data_table_cell_bounds(table, 1, 0).unwrap();
    queue_click(&mut app, bounds);
    app.frame(0.016).unwrap();
    assert_eq!(
        app.data_table_keyed(table, &headers, &[&["Rejected"]], &[999])
            .err(),
        Some(Error(9))
    );
    assert_eq!(app.data_table_cell(table, 1, 0).unwrap(), "Working row");
    assert_eq!(app.data_table_state(table).unwrap().row, 2);
    app.data_table(table, &headers, &[&["Positional replacement"]])
        .unwrap();
    app.step(0.016).unwrap();
    assert_eq!(
        app.data_table_cell(table, 0, 0).unwrap(),
        "Positional replacement"
    );
    assert_eq!(app.data_table_state(table).unwrap().row, 0);
}
