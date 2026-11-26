//! Supply a TrueType font path for an SDL window; omit it for headless checks.
use cui_builders::{App, Kind, Phase, Result, VirtualListRow};
use std::{cell::RefCell, rc::Rc};

#[derive(Clone, Copy)]
enum Action {
    Prepend,
    Resize,
}

fn main() -> Result<()> {
    let font = std::env::args().nth(1);
    let mut app = match font.as_deref() {
        Some(font) => App::sdl(480., 320., "Live inbox", font)?,
        None => App::headless(480., 320.)?,
    };
    let supported = app.supports_virtual_list_rows();
    let root = app
        .build_root(Kind::Column, "inbox")?
        .gap_units(2.)?
        .finish();
    let title = app.child(root, Kind::Label, "title")?;
    app.text(title, "Live inbox")?;
    let actions = app
        .build_child(root, Kind::Row, "actions")?
        .gap_units(2.)?
        .finish();
    let prepend = app.child(actions, Kind::Button, "prepend")?;
    let resize = app.child(actions, Kind::Button, "resize")?;
    app.text(prepend, "Prepend message")?
        .text(resize, "Resize first row")?
        .disabled(prepend, !supported)?
        .disabled(resize, !supported)?;
    let list = app
        .build_child(root, Kind::VirtualList, "messages")?
        .visual_size(380., 176.)?
        .finish();
    if supported {
        app.preserve_virtual_list_anchor(list, true)?;
    }
    let mut rows = Vec::new();
    for i in 1..=12 {
        let label = format!("Inbox message {i}");
        let item = app.child(list, Kind::Item, &label)?;
        app.text(item, &label)?;
        rows.push(VirtualListRow { item, extent: 0. });
    }
    app.value(list, 105.5)?;
    let first_visible = rows[3].item;
    let pending = Rc::new(RefCell::new(Vec::new()));
    for (button, action) in [(prepend, Action::Prepend), (resize, Action::Resize)] {
        let pending = pending.clone();
        app.on_click(button, move || pending.borrow_mut().push(action))?;
    }
    let mut apply_edits = |app: &mut App| -> Result<()> {
        for action in pending.borrow_mut().drain(..) {
            match action {
                Action::Prepend if rows.len() == 64 => continue,
                Action::Prepend => {
                    let label = format!("New message {}", rows.len() - 11);
                    let item = app.child(list, Kind::Item, &label)?;
                    app.text(item, &label)?;
                    rows.insert(0, VirtualListRow { item, extent: 60. });
                    if rows.len() == 64 {
                        app.disabled(prepend, true)?;
                    }
                }
                Action::Resize => {
                    rows[0].extent = if rows[0].extent == 86. { 60. } else { 86. };
                }
            }
            app.set_virtual_list_rows(list, &rows)?;
        }
        Ok(())
    };
    if font.is_some() {
        return app.run_with(apply_edits);
    }

    app.step(0.016)?;
    let expected = if supported {
        for control in [prepend, resize] {
            let bounds = app.bounds(control)?;
            for phase in [Phase::Down, Phase::Up] {
                app.pointer(
                    phase,
                    bounds.x + bounds.width / 2.,
                    bounds.y + bounds.height / 2.,
                )?;
            }
            assert_eq!(app.frame(0.016)?, 1);
            apply_edits(&mut app)?;
        }
        app.step(0.016)?;
        191.5
    } else {
        105.5
    };
    assert_eq!(app.current_value(list)?, expected);
    let bounds = app.bounds(list)?;
    let row = app.virtual_list_item_bounds(list, first_visible)?;
    assert_eq!(row.y - bounds.y, -9.5);
    println!("Inbox message 4 is still first, 9.5 pixels into the row");
    Ok(())
}
