use cui_builders::{App, Kind, Phase};

#[test]
fn callback_panic_stays_in_rust() {
    let mut app = App::headless(300., 100.).unwrap();
    let root = app.root(Kind::Row, "root").unwrap();
    let button = app.child(root, Kind::Button, "button").unwrap();
    app.text(button, "Click").unwrap();
    app.on_click(button, || panic!("intentional callback panic"))
        .unwrap();
    app.step(0.016).unwrap();
    let bounds = app.bounds(button).unwrap();
    app.pointer(Phase::Down, bounds.x + 1., bounds.y + 1.)
        .unwrap();
    app.pointer(Phase::Up, bounds.x + 1., bounds.y + 1.)
        .unwrap();
    app.step(0.016).unwrap();
    let panicked = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| app.dispatch()));
    assert!(panicked.is_err());
    assert!(app.step(0.016).is_ok());
}
