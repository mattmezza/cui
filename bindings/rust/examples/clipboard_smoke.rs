use cui_builders::*;
use std::env;

fn main() -> Result<()> {
    if env::var("CUI_TEST_VIRTUAL_DISPLAY").as_deref() != Ok("1") {
        panic!("clipboard smoke requires an invisible display marker");
    }
    let font = env::var("CUI_TEST_FONT").expect("set CUI_TEST_FONT to a TTF path");
    let mut app = App::sdl(320., 200., "Rust clipboard smoke", &font)?;
    app.clipboard_set("Rust clipboard ✓")?;
    assert_eq!(app.clipboard_get()?, "Rust clipboard ✓");
    let full = "é".repeat(32768);
    assert_eq!(full.len(), 65536);
    app.clipboard_set(&full)?;
    assert_eq!(app.clipboard_get()?, full);
    app.clipboard_set("")?;
    assert_eq!(app.clipboard_get()?, "");
    Ok(())
}
