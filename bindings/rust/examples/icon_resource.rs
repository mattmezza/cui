// SPDX-License-Identifier: MIT
//! Prepare an owned SVG as a headless coverage bitmap.
use cui_builders::{App, Error, IconError, IconLimits, IconSource, IconTransform, Result};

const MARK: &str = r#"<svg width="16" height="16">
    <path d="M2 8l4 4 8-8" fill="none" stroke="black" stroke-width="2"/>
</svg>"#;

fn main() -> Result<()> {
    let mut app = App::headless(320., 240.)?;
    let path = "profile/verified.svg";
    let mark = app.create_icon_resource(&IconSource::path(path), IconLimits::default())?;
    app.put_icon_asset(mark, path, MARK)?;
    let status = app.prepare_icon(mark, 16., 16., IconTransform::IDENTITY)?;
    if status != IconError::Ok {
        eprintln!("SVG preparation failed: {status:?}");
        return Err(Error(8));
    }
    let mask = app.icon_bitmap(mark)?;
    assert_eq!((mask.width, mask.height), (16, 16));
    assert!(mask.rgba.iter().skip(3).step_by(4).any(|alpha| *alpha != 0));
    app.release_icon_resource(mark)?;
    println!("Prepared {path} as a 16x16 coverage mask");
    Ok(())
}
