//! An account settings confirmation. Pass a TTF font path to open an SDL window.
use cui_builders::{
    App, DialogAction, DialogOptions, DialogSlot, Error, Kind, Node, Phase, Result, SurfaceResult,
    Variant,
};
use std::{cell::Cell, rc::Rc};

fn main() -> Result<()> {
    let font = std::env::args().nth(1);
    let mut app = if let Some(font) = &font {
        App::sdl(560., 360., "Account settings", font)?
    } else {
        App::headless(560., 360.)?
    };
    if !app.supports_dialog_composition() {
        return Err(Error::DIALOG_COMPOSITION_UNAVAILABLE);
    }
    let root = app
        .build_root(Kind::Column, "account")?
        .gap_units(3.)?
        .finish();
    app.build_child(root, Kind::Label, "heading")?
        .text("Account settings")?;
    let status = app
        .build_child(root, Kind::Label, "status")?
        .text("Local account preferences are active.")?
        .finish();
    let open_pending = Rc::new(Cell::new(false));
    let trigger = app
        .build_child(root, Kind::Button, "open")?
        .text("Manage account")?
        .on_click({
            let open = Rc::clone(&open_pending);
            move || open.set(true)
        })?
        .finish();
    let accepted = Rc::new(Cell::new(false));
    let alert = app
        .build_child(root, Kind::AlertDialog, "remove")?
        .text("Remove this account?")?
        .detail_text("Only this device is affected.")?
        .dialog_options(Some(DialogOptions {
            centered: true,
            ..Default::default()
        }))?
        .on_close_event({
            let accepted = Rc::clone(&accepted);
            move |event| accepted.set(event.surface_result() == Some(SurfaceResult::Accepted))
        })?
        .finish();
    let body = app.create_dialog_slot(alert, DialogSlot::Body, "details")?;
    app.build_child(body, Kind::Label, "copy")?
        .text("Remove saved preferences and account information.")?;
    let footer = app.create_dialog_slot(alert, DialogSlot::Footer, "actions")?;
    app.build_child(footer, Kind::Button, "cancel")?
        .text("Keep account")?
        .variant(Variant::Outline)?
        .dialog_action(DialogAction::Cancel)?;
    let confirm = app
        .build_child(footer, Kind::Button, "confirm")?
        .text("Remove account")?
        .variant(Variant::Primary)?
        .dialog_action(DialogAction::Confirm)?
        .finish();
    let removed = "Local account preferences removed.";
    let update = |app: &mut App| -> Result<()> {
        if open_pending.replace(false) {
            app.open(alert, true)?;
        }
        if accepted.replace(false) {
            app.text(status, removed)?;
        }
        Ok(())
    };
    if font.is_some() {
        return app.run_with(update);
    }
    app.step(0.016)?;
    click(&mut app, trigger)?;
    update(&mut app)?;
    app.step(0.016)?;
    click(&mut app, confirm)?;
    update(&mut app)?;
    assert_eq!(app.editable_text(status)?, removed);
    println!("{removed}");
    Ok(())
}

fn click(app: &mut App, button: Node) -> Result<()> {
    let rect = app.bounds(button)?;
    for phase in [Phase::Down, Phase::Up] {
        app.pointer(phase, rect.x + rect.width / 2., rect.y + rect.height / 2.)?;
    }
    app.step(0.016)?;
    app.dispatch()?;
    Ok(())
}
