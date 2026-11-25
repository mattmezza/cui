//! A retained Inbox with SVG artwork and an unread count.
use cui_builders::{
    App, BadgeConfig, BadgeVariant, IconConfig, IconError, IconLimits, IconOptions, IconSource,
    Kind, Phase, Result, Variant, WindowOptions,
};
use std::{cell::Cell, rc::Rc};

const SYNCED: &str = r#"<svg width="16" height="16">
    <path d="M2 8l4 4 8-8" fill="none" stroke="black" stroke-width="2"/>
</svg>"#;

fn main() -> Result<()> {
    let args: Vec<_> = std::env::args().skip(1).collect();
    let smoke = args.iter().any(|arg| arg == "--smoke");
    let font = args.iter().position(|arg| arg == "--font").map(|index| {
        args.get(index + 1)
            .expect("--font requires a TTF path")
            .as_str()
    });
    if smoke {
        assert!(
            font.is_some() && std::env::var("CUI_TEST_VIRTUAL_DISPLAY").as_deref() == Ok("1"),
            "--smoke requires a font and the isolated display from scripts/test_local.py"
        );
    }
    let mut app = if let Some(font) = font {
        App::sdl_with_window_options(
            520.,
            260.,
            "Inbox",
            font,
            WindowOptions {
                initially_hidden: smoke,
            },
        )?
    } else {
        App::headless(520., 260.)?
    };
    let mark = app.create_icon_resource(&IconSource::svg(SYNCED), IconLimits::default())?;
    let root = app
        .build_root(Kind::Column, "inbox")?
        .gap_units(4.)?
        .finish();
    let heading = app
        .build_child(root, Kind::Row, "heading")?
        .gap_units(2.)?
        .finish();
    let icon = app.configured_icon(
        heading,
        "sync-status",
        &IconConfig {
            name: "Mail synced".into(),
            options: IconOptions {
                width: Some(20.),
                height: Some(20.),
                ..Default::default()
            },
            ..IconConfig::new(mark)
        },
    )?;
    app.build_child(heading, Kind::Label, "title")?
        .text("Workspace inbox")?;
    let status = app
        .build_child(root, Kind::Label, "status")?
        .text("8 unread messages")?
        .finish();
    let actions = app
        .build_child(root, Kind::Row, "actions")?
        .gap_units(3.)?
        .finish();

    let count = app.configured_badge(
        actions,
        "unread",
        &BadgeConfig {
            name: "Unread messages".into(),
            ..BadgeConfig::count(8)
        },
    )?;
    let clear = app
        .build_child(count, Kind::Button, "clear")?
        .text("Mark all as read")?
        .variant(Variant::Primary)?
        .finish();
    let sent = app.configured_badge(
        actions,
        "synced",
        &BadgeConfig {
            variant: BadgeVariant::Artwork {
                resource: mark,
                options: IconOptions {
                    width: Some(12.),
                    height: Some(12.),
                    ..Default::default()
                },
            },
            name: "Synced".into(),
            ..Default::default()
        },
    )?;
    let outbox = app
        .build_child(sent, Kind::Button, "sent")?
        .text("Sent mail")?
        .finish();

    let action = Rc::new(Cell::new(None));
    let click = Rc::clone(&action);
    app.on_click(clear, move || click.set(Some(true)))?;
    let click = Rc::clone(&action);
    app.on_click(outbox, move || click.set(Some(false)))?;
    let update_inbox = |app: &mut App| -> Result<()> {
        match action.take() {
            Some(true) => {
                app.configure_badge(count, &BadgeConfig::count(0))?;
                app.text(status, "Inbox is up to date")?;
            }
            Some(false) => {
                app.text(status, "All outgoing mail is synced")?;
            }
            None => {}
        }
        Ok(())
    };
    if font.is_some() && !smoke {
        return app.run_with(update_inbox);
    }

    app.step(0.016)?;
    assert_eq!(app.icon_info(mark)?.references, 2);
    assert_eq!(app.icon_result(icon)?.unwrap().icon_error, IconError::Ok);
    assert_eq!(
        app.badge_result(sent)?.unwrap().icon_error,
        Some(IconError::Ok)
    );
    assert!(app.badge_indicator_bounds(count)?.is_some());
    let button = app.bounds(clear)?;
    let (x, y) = (button.x + button.width / 2., button.y + button.height / 2.);
    app.pointer(Phase::Down, x, y)?;
    app.pointer(Phase::Up, x, y)?;
    assert_eq!(app.frame(0.016)?, 1);
    update_inbox(&mut app)?;
    app.step(0.016)?;
    assert!(app.badge_indicator_bounds(count)?.is_none());
    assert_eq!(app.editable_text(status)?, "Inbox is up to date");
    app.remove(sent)?;
    app.remove(icon)?;
    app.release_icon_resource(mark)?;
    println!("Inbox: shared SVG, child callback and unread count verified");
    Ok(())
}
