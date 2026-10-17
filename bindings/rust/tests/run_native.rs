use cui_builders::{App, Error, Kind, Phase};
#[test]
fn native_runner_lifecycle() {
    if std::env::var("CUI_TEST_VIRTUAL_DISPLAY").as_deref() != Ok("1") {
        return;
    }
    assert!(std::env::var("DISPLAY").is_ok_and(|d| d.split('.').next() != Some(":0")));
    assert!(std::env::var("WAYLAND_DISPLAY").is_err());
    let font = std::env::var("CUI_TEST_FONT").unwrap();
    for mode in ["close", "error", "panic", "nested"] {
        let mut app = App::sdl(400., 200., "Rust callback runner", &font).unwrap();
        app.root(Kind::Column, "root").unwrap();
        let mut calls = 0;
        let result = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| {
            app.run_with(|app| {
                calls += 1;
                match mode {
                    "error" => return Err(Error(3)),
                    "panic" => panic!("runner callback marker"),
                    "nested" => {
                        assert_eq!(app.run(), Err(Error(8)));
                        assert_eq!(app.step(0.016), Err(Error(8)));
                    }
                    _ => {}
                }
                app.close()
            })
        }));
        assert_eq!(calls, 1);
        match mode {
            "panic" => assert!(result.is_err()),
            "error" => assert_eq!(result.unwrap(), Err(Error(3))),
            _ => assert!(result.unwrap().is_ok()),
        }
        assert_eq!(app.step(0.016), Err(Error(2)));
    }
    // A regular widget handler also executes inside the foreign callback.
    let mut app = App::sdl(400., 200., "Rust handler panic", &font).unwrap();
    let root = app.root(Kind::Column, "root").unwrap();
    let button = app.child(root, Kind::Button, "button").unwrap();
    app.text(button, "Go").unwrap();
    app.on_click(button, || panic!("widget handler marker"))
        .unwrap();
    app.step(0.016).unwrap();
    let bounds = app.bounds(button).unwrap();
    app.pointer(Phase::Down, bounds.x + 2., bounds.y + 2.)
        .unwrap();
    app.pointer(Phase::Up, bounds.x + 2., bounds.y + 2.)
        .unwrap();
    let outcome = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| app.run()));
    assert!(outcome.is_err());
    assert_eq!(app.step(0.016), Err(Error(2)));
}
#[test]
fn portable_runner_reports_unavailable() {
    let mut app = App::headless(300., 100.).unwrap();
    app.root(Kind::Column, "root").unwrap();
    assert_eq!(app.run(), Err(Error(9)));
    assert!(app.step(0.016).is_ok());
}
