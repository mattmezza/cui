//! An authored loading profile. Add --font PATH to open the native UI.
use cui_builders::{App, Kind, Phase, Result, SpinnerConfig, Variant, WindowOptions};
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
            "Loading a workspace profile",
            path,
            WindowOptions {
                initially_hidden: smoke,
            },
        )?
    } else {
        App::headless(480., 320.)?
    };
    let root = app
        .build_root(Kind::Column, "root")?
        .gap_units(2.)?
        .finish();
    let title = app
        .build_child(root, Kind::Label, "title")?
        .text("Workspace profile")?
        .finish();
    let spinner = app.rotating_spinner(root, "loading", SpinnerConfig::default())?;
    let status = app
        .build_child(root, Kind::Label, "status")?
        .text("Loading profile…")?
        .finish();
    let finish = app
        .build_child(root, Kind::Button, "finish")?
        .variant(Variant::Primary)?
        .text("Finish loading")?
        .finish();
    let ready = Rc::new(Cell::new(false));
    let click = Rc::clone(&ready);
    app.on_click(finish, move || click.set(true))?;
    let finish_loading = |app: &mut App| -> Result<()> {
        if ready.replace(false) {
            app.remove(spinner)?;
            app.text(title, "Alex Morgan")?;
            app.text(status, "Workspace admin · alex@example.com")?;
            app.disabled(finish, true)?.text(finish, "Profile loaded")?;
        }
        Ok(())
    };
    if font.is_some() && !smoke {
        return app.run_with(finish_loading);
    }

    app.step(0.25)?;
    assert!((app.bounds(spinner)?.width - 16.).abs() < 0.001);
    let sample = app.spinner_presentation(spinner)?.expect("active sample");
    assert!(sample.next_wake_seconds > sample.core_time);
    let before = app.bounds(status)?.y;
    let button = app.bounds(finish)?;
    let (x, y) = (button.x + button.width / 2., button.y + button.height / 2.);
    app.pointer(Phase::Down, x, y)?;
    app.pointer(Phase::Up, x, y)?;
    app.frame(0.016)?;
    finish_loading(&mut app)?;
    app.step(0.016)?;
    assert_eq!(app.editable_text(title)?, "Alex Morgan");
    assert!(app.bounds(status)?.y < before);
    println!("Spinner loading profile: loaded and spinner removed");
    Ok(())
}
