//! A live inbox using stable row keys. Add --font PATH to open the native UI.
use cui_builders::{App, Error, Kind, Phase, Result, Variant, WindowOptions};
use std::{cell::Cell, rc::Rc};

fn main() -> Result<()> {
    let args: Vec<_> = std::env::args().skip(1).collect();
    let smoke = args.iter().any(|arg| arg == "--smoke");
    let font = args.iter().position(|arg| arg == "--font").map(|index| {
        args.get(index + 1)
            .expect("--font requires a path")
            .as_str()
    });
    if smoke {
        assert!(
            font.is_some() && std::env::var("CUI_TEST_VIRTUAL_DISPLAY").as_deref() == Ok("1"),
            "--smoke requires a font and the private display from scripts/test_local.py"
        );
    }
    let mut app = if let Some(path) = font {
        App::sdl_with_window_options(
            480.,
            320.,
            "Live inbox",
            path,
            WindowOptions {
                initially_hidden: smoke,
            },
        )?
    } else {
        App::headless(480., 320.)?
    };
    if !app.supports_keyed_data_table() {
        eprintln!("The live inbox requires a bridge with keyed DataTable support.");
        return Err(Error(9));
    }

    let headers = ["Subject", "Sender"];
    let rows: [&[&str]; 8] = [
        &["Project brief", "Alex"],
        &["Component review", "Morgan"],
        &["Starter feedback", "Taylor"],
        &["Updated schedule", "Workspace"],
        &["Performance report", "Build team"],
        &["Accessibility notes", "Sam"],
        &["Asset handoff", "Design"],
        &["Release checklist", "Build team"],
    ];
    let keys = [101, 102, 103, 104, 105, 106, 107, 108];
    let root = app
        .build_root(Kind::RootView, "root")?
        .root_padding(16.)?
        .finish();
    let content = app
        .build_child(root, Kind::Column, "content")?
        .gap_units(2.)?
        .finish();
    app.build_child(content, Kind::Label, "title")?
        .text("Inbox")?
        .finish();
    let load = app
        .build_child(content, Kind::Button, "load")?
        .variant(Variant::Primary)?
        .text("Load latest")?
        .finish();
    let inbox = app
        .build_child(content, Kind::DataTable, "inbox")?
        .data_table_keyed(&headers, &rows, &keys)?
        .visual_size(360., 184.)?
        .finish();
    let status = app
        .build_child(content, Kind::Label, "status")?
        .text("8 messages")?
        .finish();

    let ready = Rc::new(Cell::new(false));
    let click = Rc::clone(&ready);
    app.on_click(load, move || click.set(true))?;
    let receive_latest = |app: &mut App| -> Result<()> {
        if ready.replace(false) {
            let mut next_rows: Vec<&[&str]> = vec![
                &["Release notes", "Build team"],
                &["Design review", "Morgan"],
            ];
            next_rows.extend(rows);
            let next_keys: Vec<_> = [203, 204].into_iter().chain(keys).collect();
            app.data_table_keyed(inbox, &headers, &next_rows, &next_keys)?;
            app.disabled(load, true)?;
            app.text(status, "10 messages · Updated just now")?;
        }
        Ok(())
    };
    if font.is_some() && !smoke {
        return app.run_with(receive_latest);
    }

    app.step(0.016)?;
    let initial = app.data_table_cell_bounds(inbox, 1, 0)?;
    app.scroll(initial.x + 10., initial.y + 10., 0., -1.)?;
    app.step(0.016)?;
    let before = app.data_table_cell_bounds(inbox, 1, 0)?;
    assert!(before.y < initial.y);
    let bounds = app.bounds(load)?;
    let (x, y) = (bounds.x + bounds.width / 2., bounds.y + bounds.height / 2.);
    app.pointer(Phase::Down, x, y)?;
    app.pointer(Phase::Up, x, y)?;
    app.frame(0.016)?;
    receive_latest(&mut app)?;
    app.step(0.016)?;
    assert_eq!(app.data_table_cell(inbox, 0, 0)?, "Release notes");
    assert_eq!(app.data_table_cell_bounds(inbox, 3, 0)?.y, before.y);
    assert_eq!(app.editable_text(status)?, "10 messages · Updated just now");
    println!("Live inbox updated and kept the visible row in place.");
    Ok(())
}
