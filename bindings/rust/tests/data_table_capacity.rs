use cui_builders::{App, DataTableLoadMore, Error, Kind, Phase, ValueNotificationV1};

const HEADERS: &[&str] = &["Subject"];

#[test]
fn appending_past_32_preserves_viewport_and_selection() {
    let mut app = App::headless(480., 320.).unwrap();
    let rows: Vec<_> = (0..40).map(|row| [format!("Message {row}")]).collect();
    let cells: Vec<_> = rows.iter().map(|row| [row[0].as_str()]).collect();
    let rows: Vec<_> = cells.iter().map(|row| row.as_slice()).collect();
    let mut keys: Vec<_> = (100..140).collect();
    keys[31] = u32::MAX;
    let root = app.root(Kind::Column, "root").unwrap();
    let table = app
        .build_child(root, Kind::DataTable, "inbox")
        .unwrap()
        .data_table_keyed_capacity(HEADERS, &rows[..32], &keys[..32], 48)
        .unwrap()
        .visual_size(320., 160.)
        .unwrap()
        .data_table_load_more(0)
        .unwrap()
        .finish();
    app.step(0.016).unwrap();
    assert_eq!(app.poll_data_table_load_more(table), Ok(None));
    let first = app.data_table_cell_bounds(table, 0, 0).unwrap();
    app.scroll(first.x + 10., first.y + 10., 0., -40.).unwrap();
    app.step(0.016).unwrap();
    let selected = app.data_table_cell_bounds(table, 30, 0).unwrap();
    let (x, y) = (selected.x + 10., selected.y + 10.);
    app.pointer(Phase::Down, x, y).unwrap();
    app.pointer(Phase::Up, x, y).unwrap();
    app.step(0.016).unwrap();

    assert_eq!(
        app.data_table_keyed_capacity(table, HEADERS, &rows, &keys, 39)
            .err(),
        Some(Error(3))
    );
    let mut duplicate = keys.clone();
    duplicate[39] = duplicate[38];
    assert_eq!(
        app.data_table_keyed_capacity(table, HEADERS, &rows, &duplicate, 64)
            .err(),
        Some(Error(3))
    );
    let request = app.poll_data_table_load_more(table).unwrap().unwrap();
    assert!(request.visible_begin <= 30 && request.frame_sequence != 0);
    assert_eq!(
        (
            request.row_count,
            request.visible_end,
            request.last_row_key,
            request.capacity
        ),
        (32, 32, u32::MAX, 48)
    );
    let Some(ValueNotificationV1::TableChange(_, change)) =
        app.poll_value_notification_v1().unwrap()
    else {
        panic!("expected the independent selection notification");
    };
    assert_eq!(change.row, keys[30]);

    app.data_table_keyed_capacity(table, HEADERS, &rows, &keys, 64)
        .unwrap();
    app.step(0.016).unwrap();
    assert_eq!(app.data_table_state(table).unwrap().row, keys[30]);
    assert_eq!(
        app.data_table_cell_bounds(table, 30, 0).unwrap().y,
        selected.y
    );
    assert_eq!(app.data_table_cell(table, 39, 0).unwrap(), "Message 39");
    assert!(app.poll_value_notification_v1().unwrap().is_none());
    app.scroll(first.x + 10., first.y + 10., 0., -40.).unwrap();
    app.step(0.016).unwrap();
    let after = app.poll_data_table_load_more(table).unwrap().unwrap();
    assert!(after.frame_sequence > request.frame_sequence);
    assert_eq!(
        (
            after.row_count,
            after.visible_end,
            after.last_row_key,
            after.capacity
        ),
        (40, 40, keys[39], 64)
    );

    app.data_table_keyed(table, HEADERS, &[], &[]).unwrap();
    app.step(0.016).unwrap();
    let legacy = app.poll_data_table_load_more(table).unwrap().unwrap();
    assert_eq!(
        (legacy.row_count, legacy.last_row_key, legacy.capacity),
        (0, 0, 32)
    );
    drop(app);
    assert_eq!((request.row_count, request.capacity), (32, 48));
}

#[test]
fn validates_limits_and_copies_the_input() {
    let mut app = App::headless(480., 320.).unwrap();
    assert_eq!(app.data_table_max_row_capacity(), Some(10_000));
    let root = app.root(Kind::Column, "root").unwrap();
    let table = app.child(root, Kind::DataTable, "inbox").unwrap();
    let limit = 10_000;
    let keys: Vec<_> = (1..=limit).collect();
    {
        let text = String::from("Last message");
        let last = [text.as_str()];
        let mut rows = vec![&["Inbox"][..]; limit as usize];
        rows[(limit - 1) as usize] = &last;
        assert_eq!(
            app.data_table_keyed(table, HEADERS, &rows[..33], &keys[..33])
                .err(),
            Some(Error(3))
        );
        let too_long = "é".repeat(65);
        for (rows, keys, capacity) in [
            (&[][..], &[][..], 0),
            (&[][..], &[][..], limit + 1),
            (&rows[..2], &keys[..2], 1),
            (&rows[..1], &[][..], 2),
            (&rows[..1], &[0][..], 2),
            (&[&["A", "B"][..]][..], &keys[..1], 2),
            (&[&[too_long.as_str()][..]][..], &keys[..1], 2),
            (&[&["prefix\0suffix"][..]][..], &keys[..1], 2),
        ] {
            assert_eq!(
                app.data_table_keyed_capacity(table, HEADERS, rows, keys, capacity)
                    .err(),
                Some(Error(3))
            );
        }
        app.data_table_keyed_capacity(table, HEADERS, &rows, &keys, limit)
            .unwrap();
    }
    app.configure_data_table_load_more(table, true, 0).unwrap();
    app.step(0.016).unwrap();
    assert_eq!(
        app.data_table_cell(table, limit - 1, 0).unwrap(),
        "Last message"
    );
    assert_eq!(app.poll_data_table_load_more(table), Ok(None));
    app.data_table_keyed_capacity(table, HEADERS, &[], &[], 1)
        .unwrap();
    app.step(0.016).unwrap();
    let demand = app.poll_data_table_load_more(table).unwrap().unwrap();
    assert!(demand.frame_sequence > 0);
    assert_eq!(
        demand,
        DataTableLoadMore {
            frame_sequence: demand.frame_sequence,
            row_count: 0,
            visible_begin: 0,
            visible_end: 0,
            last_row_key: 0,
            capacity: 1,
        }
    );
}

#[test]
fn optional_provider_preserves_ordinary_tables() {
    let Ok(mode) = std::env::var("CUI_DATA_TABLE_CAPACITY_FIXTURE") else {
        return;
    };
    let mut app = App::headless(480., 320.).unwrap();
    let (maximum, expected_error) = match mode.as_str() {
        "legacy" | "query-only" | "setter-only" | "zero" => (None, Some(Error(9))),
        "limited" => (Some(48), Some(Error(3))),
        "large" => (Some(10_000), None),
        "rejected" => (Some(10_000), Some(Error(10))),
        _ => panic!("unknown fixture {mode}"),
    };
    assert_eq!(app.data_table_max_row_capacity(), maximum);
    let root = app.root(Kind::Column, "root").unwrap();
    let table = app.child(root, Kind::DataTable, "inbox").unwrap();
    app.data_table(table, HEADERS, &[&["Ordinary"]]).unwrap();
    let result = app.data_table_keyed_capacity(table, HEADERS, &[&["Updated"]], &[7], 64);
    assert_eq!(result.err(), expected_error);
    let cell = if expected_error.is_some() {
        "Ordinary"
    } else {
        "Updated"
    };
    app.step(0.016).unwrap();
    assert_eq!(app.data_table_cell(table, 0, 0).unwrap(), cell);
    if mode == "limited" {
        app.data_table_keyed_capacity(table, HEADERS, &[&["At provider limit"]], &[8], 48)
            .unwrap();
        assert_eq!(
            app.data_table_cell(table, 0, 0).unwrap(),
            "At provider limit"
        );
    }
}
