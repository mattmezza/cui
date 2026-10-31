use cui_builders::{App, Error, Kind, WindowOptions};
#[test]
fn native_window_visibility() {
    if std::env::var("CUI_TEST_VIRTUAL_DISPLAY").as_deref() != Ok("1") {
        return;
    }
    assert!(std::env::var("DISPLAY").is_ok_and(|d| d.split('.').next() != Some(":0")));
    assert!(std::env::var("WAYLAND_DISPLAY").is_err());
    let font = std::env::var("CUI_TEST_FONT").unwrap();
    for hidden in [true, false] {
        let mut app = App::sdl_with_window_options(
            240.,
            160.,
            "Rust hidden",
            &font,
            WindowOptions {
                initially_hidden: hidden,
            },
        )
        .unwrap();
        assert_eq!(app.window_visible().unwrap(), !hidden);
        app.root(Kind::Column, "root").unwrap();
        app.step(0.016).unwrap();
        assert_eq!(app.window_visible().unwrap(), !hidden);
        let mut calls = 0;
        app.run_with(|app| {
            assert_eq!(app.window_visible().unwrap(), !hidden);
            app.set_window_visible(true).unwrap();
            assert!(app.window_visible().unwrap());
            app.set_window_visible(false).unwrap();
            assert!(!app.window_visible().unwrap());
            calls += 1;
            app.close()
        })
        .unwrap();
        assert_eq!(calls, 1);
        assert_eq!(app.window_visible(), Err(Error(2)));
    }
    let app = App::headless(240., 160.).unwrap();
    assert_eq!(app.window_visible(), Err(Error(9)));
}
