use cui_builders::*;
use std::{cell::Cell, env, rc::Rc};
fn main() -> Result<()> {
    let mut app = if env::var("CUI_PACKAGE_SMOKE_BACKEND").as_deref() == Ok("sdl") {
        App::sdl(480.,240.,"CUI Installed Rust Smoke", &env::var("CUI_PACKAGE_SMOKE_FONT").unwrap())?
    } else { App::headless(480.,240.)? };
    assert!(app.supports_radio_tab_index(), "installed bridge lacks Radio ordering");
    assert!(app.supports_radio_paint(), "installed bridge lacks Radio paint");
    assert!(app.supports_rating_options(), "installed bridge lacks Rating options");
    assert!(app.supports_stepper_fill(), "installed bridge lacks Stepper Fill");
    assert!(app.supports_virtual_list_rows(), "installed bridge lacks VirtualList live rows");
    assert!(app.supports_disclosure_presentation(), "installed bridge lacks disclosure presentation");
    assert!(app.supports_disclosure_heading(), "installed bridge lacks disclosure headings");
    assert!(app.supports_disclosure_divider(), "installed bridge lacks disclosure dividers");
    let root = app.build_root(Kind::Row,"root")?.finish();
    let clicked = Rc::new(Cell::new(0));
    let callback = clicked.clone();
    let button = app.build_child(root,Kind::Button,"save")?.text("Save ✓")?
        .on_click(move || callback.set(callback.get()+1))?.finish();
    app.step(0.016)?;
    let bounds = app.bounds(button)?;
    assert!(bounds.width > 0. && bounds.height > 0.);
    app.pointer(Phase::Down,bounds.x+bounds.width/2.,bounds.y+bounds.height/2.)?;
    app.pointer(Phase::Up,bounds.x+bounds.width/2.,bounds.y+bounds.height/2.)?;
    app.step(0.016)?;
    assert_eq!(app.dispatch()?,1);
    assert_eq!(clicked.get(),1);
    println!("installed Rust bridge smoke passed");
    Ok(())
}
