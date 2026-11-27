//! Headless Button focus treatment with optional provider support and live reset.
use cui_builders::{App, ButtonFocusMode, Kind, Result};
fn main() -> Result<()> {
    let mut app = App::headless(320., 200.)?;
    let root = app.root(Kind::Column, "root")?;
    let mode = if app.supports_button_focus() {
        ButtonFocusMode::OuterRing
    } else {
        ButtonFocusMode::Legacy
    };
    let button = app
        .build_child(root, Kind::Button, "save")?
        .text("Save")?
        .button_focus(mode)?
        .finish();
    app.step(1. / 60.)?;
    app.button_focus(button, ButtonFocusMode::Legacy)?;
    app.step(1. / 60.)?;
    Ok(())
}
