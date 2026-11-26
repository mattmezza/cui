//! Run headless by default, or pass --font /path/to/font.ttf for a native window.
use cui_builders::*;
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
            320.,
            240.,
            "Delivery options",
            path,
            WindowOptions {
                initially_hidden: smoke,
            },
        )?
    } else {
        App::headless(320., 240.)?
    };
    let root = app.build_root(Kind::Column, "root")?.gap_px(12.)?.finish();
    app.build_child(root, Kind::Label, "title")?
        .text("Delivery service")?
        .finish();
    let tab_index = app.supports_radio_tab_index().then_some(-2);
    let reset = app
        .build_child(root, Kind::Button, "reset")?
        .text("Reset selection")?
        .finish();
    let motion = app.supports_radio_motion();
    let decorate = app.supports_radio_item_content();
    let hint = app
        .supports_radio_tooltip()
        .then_some("Track your parcel online");
    let group = app
        .build_child(root, Kind::RadioGroup, "delivery")?
        .animate_selection(motion)?
        .tab_index(tab_index)?
        .finish();
    let standard = app
        .build_child(group, Kind::Item, "standard")?
        .text("Standard")?
        .finish();
    let priority = app
        .build_child(group, Kind::Item, "priority")?
        .text("Priority")?
        .detail_text("Tracked delivery. Recommended.")?
        .decorate_content(decorate)?
        .radio_tooltip(hint, 350)?
        .finish();
    let recommendation = app
        .build_child(priority, Kind::Tag, "recommended")?
        .text("Recommended")?
        .finish();
    app.select(group, standard)?;
    let summary = app
        .build_child(root, Kind::Label, "summary")?
        .text("Estimated arrival: Friday")?
        .finish();
    let selected = Rc::new(Cell::new(None));
    let captured = selected.clone();
    let changes = Rc::new(Cell::new(0));
    let count = changes.clone();
    app.on_change_event(group, move |event| {
        captured.set(event.selected);
        count.set(count.get() + 1);
    })?;
    let reset_selection = selected.clone();
    app.on_click(reset, move || reset_selection.set(Some(standard)))?;
    let apply_selection = |app: &mut App| -> Result<()> {
        if let Some(item) = selected.take() {
            if app.selected(group)? != Some(item) {
                app.select(group, item)?;
            }
            let arrival = if item == priority {
                "Wednesday"
            } else {
                "Friday"
            };
            app.text(summary, &format!("Estimated arrival: {arrival}"))?;
        }
        Ok(())
    };
    if font.is_none() || smoke {
        app.step(0.016)?;
        let b = app.bounds(if decorate { recommendation } else { priority })?;
        app.pointer(Phase::Down, b.x + b.width / 2., b.y + b.height / 2.)?;
        app.pointer(Phase::Up, b.x + b.width / 2., b.y + b.height / 2.)?;
        assert_eq!(app.frame(0.016)?, 1);
        assert_eq!(changes.get(), 1);
        assert_eq!(app.selected(group)?, Some(priority));
        apply_selection(&mut app)?;
        assert_eq!(app.editable_text(summary)?, "Estimated arrival: Wednesday");
        if tab_index.is_some() {
            let mut actions = 0;
            for key in [Key::Tab, Key::Enter] {
                app.key(true, key, 0, false)?;
                app.key(false, key, 0, false)?;
                actions += app.frame(0.016)?;
            }
            assert_eq!(actions, 1);
            apply_selection(&mut app)?;
            assert_eq!(app.selected(group)?, Some(standard));
            assert_eq!(app.editable_text(summary)?, "Estimated arrival: Friday");
            assert_eq!(changes.get(), 1);
        }
    } else {
        app.run_with(apply_selection)?;
    }
    Ok(())
}
