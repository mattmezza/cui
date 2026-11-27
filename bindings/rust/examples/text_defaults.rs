//! Headless by default; --font /path/to/font.ttf opens a native window.
use cui_builders::*;
fn main() -> Result<()> {
    let args: Vec<_> = std::env::args().collect();
    let font = args
        .iter()
        .position(|arg| arg == "--font")
        .map(|i| args.get(i + 1).expect("font path"));
    let native = font.is_some() && !args.iter().any(|arg| arg == "--headless");
    let mut app = if native {
        App::sdl(640., 360., "Scoped text defaults", font.unwrap())?
    } else {
        App::headless(640., 360.)?
    };
    let root = app.root(Kind::Column, "root")?;
    let heading = app.child(root, Kind::Label, "heading")?;
    app.text(heading, "Workspace status")?;
    let nested = app.child(root, Kind::Column, "details")?;
    let row = app.child(nested, Kind::Row, "progress")?;
    let icon = app.child(row, Kind::Icon, "activity")?;
    app.text(icon, "check")?;
    let label = app.child(row, Kind::Label, "status")?;
    app.text(label, "Synchronizing your workspace")?;
    app.child(row, Kind::Spinner, "busy")?;
    let detail = app.child(nested, Kind::Label, "detail")?;
    app.text(detail, "Your changes stay available offline.")?;
    if app.supports_text_defaults() {
        app.text_defaults(
            root,
            TextDefaults {
                font_size: 20.,
                color_rgba: Some(0x407adfff),
            },
        )?
        .text_defaults(
            nested,
            TextDefaults {
                font_size: 16.,
                ..Default::default()
            },
        )?;
    }
    if native {
        app.run()?;
    } else {
        app.step(1. / 60.)?;
    }
    Ok(())
}
