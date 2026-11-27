use cui_builders::*;
use std::{cell::RefCell, rc::Rc};

#[test]
fn retained_alert_queues_click_then_close() {
    let mut app = App::headless(480., 320.).unwrap();
    let root = app.root(Kind::Column, "root").unwrap();
    let alert = app.child(root, Kind::AlertDialog, "alert").unwrap();
    let options = DialogOptions {
        width: 300.,
        centered: true,
        ..Default::default()
    };
    if !app.supports_dialog_composition() {
        assert_eq!(
            app.dialog_options(alert, Some(options)).err(),
            Some(Error::DIALOG_COMPOSITION_UNAVAILABLE)
        );
        app.open(alert, true).unwrap();
        app.step(0.016).unwrap();
        return;
    }
    app.text(alert, "Remove account?").unwrap();
    app.dialog_options(alert, Some(options)).unwrap();
    assert!(app
        .dialog_options(
            alert,
            Some(DialogOptions {
                width: f32::INFINITY,
                ..options
            })
        )
        .is_err());
    let body = app
        .create_dialog_slot(alert, DialogSlot::Body, "body")
        .unwrap();
    app.build_child(body, Kind::Label, "copy")
        .unwrap()
        .text("Local preferences will be removed.")
        .unwrap();
    assert!(app
        .create_dialog_slot(alert, DialogSlot::Body, "duplicate")
        .is_err());
    let footer = app
        .build_dialog_slot(alert, DialogSlot::Footer, "footer")
        .unwrap()
        .gap_px(12.)
        .unwrap()
        .finish();
    let accept = app
        .build_child(footer, Kind::Button, "remove")
        .unwrap()
        .text("Remove account")
        .unwrap()
        .dialog_action(DialogAction::Confirm)
        .unwrap()
        .finish();
    let events = Rc::new(RefCell::new(Vec::new()));
    let click = Rc::clone(&events);
    app.on_click(accept, move || click.borrow_mut().push("click"))
        .unwrap();
    let close = Rc::clone(&events);
    app.on_close_event(alert, move |event| {
        assert_eq!(event.surface_result(), Some(SurfaceResult::Accepted));
        close.borrow_mut().push("close");
    })
    .unwrap();
    app.open(alert, true).unwrap();
    app.step(0.016).unwrap();
    assert_eq!(app.bounds(alert).unwrap().width, 300.);
    let rect = app.bounds(accept).unwrap();
    let (x, y) = (rect.x + rect.width / 2., rect.y + rect.height / 2.);
    app.pointer(Phase::Down, x, y).unwrap();
    app.pointer(Phase::Up, x, y).unwrap();
    app.step(0.016).unwrap();
    app.dispatch().unwrap();
    assert_eq!(*events.borrow(), ["click", "close"]);
    assert!(!app.is_open(alert).unwrap());
}
