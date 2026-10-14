use cui_builders::*;
use std::{cell::Cell, env, rc::Rc};
fn main() -> Result<()> {
    let smoke_frames = env::var("CUI_RUST_SMOKE_FRAMES").ok().map(|s| {
        s.parse::<usize>()
            .expect("CUI_RUST_SMOKE_FRAMES must be an integer")
    });
    if smoke_frames.is_some() && env::var("CUI_TEST_VIRTUAL_DISPLAY").as_deref() != Ok("1") {
        panic!("smoke test requires an invisible display marker");
    }
    let font = env::var("CUI_TEST_FONT").expect("set CUI_TEST_FONT to a TTF path");
    let mut app = App::sdl(480., 240., "CUI Rust builders", &font)?;
    let root = app
        .build_root(Kind::Column, "root")?
        .gap_units(2.)?
        .finish();
    let label = app
        .build_child(root, Kind::Label, "status")?
        .text("Unsaved changes")?
        .finish();
    let row = app
        .build_child(root, Kind::Row, "actions")?
        .gap_px(8.)?
        .finish();
    let saved = Rc::new(Cell::new(false));
    let clicked = saved.clone();
    let _button = app
        .build_child(row, Kind::Button, "save")?
        .text("Save changes")?
        .variant(Variant::Primary)?
        .rounding(Rounding::Large)?
        .on_click(move || clicked.set(true))?
        .finish();
    let mut frames = 0;
    loop {
        match app.frame(1. / 60.) {
            Ok(_) => {}
            Err(Error(2)) => break,
            Err(e) => return Err(e),
        }
        if saved.replace(false) {
            app.text(label, "Saved ✓")?;
        }
        frames += 1;
        if smoke_frames.is_some_and(|limit| frames >= limit) {
            break;
        }
        std::thread::sleep(std::time::Duration::from_millis(16));
    }
    Ok(())
}
