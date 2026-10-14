use cui_builders::*;
use std::{cell::Cell, env, rc::Rc};
fn main() -> Result<()> {
    let mut app = if env::var("CUI_PACKAGE_SMOKE_BACKEND").as_deref() == Ok("sdl") {
        App::sdl(480.,240.,"CUI Installed Rust Smoke", &env::var("CUI_PACKAGE_SMOKE_FONT").unwrap())?
    } else { App::headless(480.,240.)? };
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
