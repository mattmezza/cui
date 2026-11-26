//! Workspace preferences. Pass a TrueType font path for the native SDL window.
use cui_builders::{
    App, DesignSize, DisclosureConfig, DisclosureCursor, DisclosureDivider, DisclosureHeading,
    DisclosureHeadingLayer, DisclosureTitleState, DisclosureTitleStyle, DisclosureTriggerStyle,
    DisclosureTypography, DisclosureWeight, Kind, Phase, Result,
};
use std::{cell::Cell, rc::Rc};

fn main() -> Result<()> {
    let font = std::env::args().nth(1);
    let mut app = match font.as_deref() {
        Some(font) => App::sdl(480., 320., "Workspace preferences", font)?,
        None => App::headless(480., 320.)?,
    };
    let root = app
        .build_root(Kind::Column, "workspace")?
        .gap_units(3.)?
        .finish();
    let title = app.child(root, Kind::Label, "title")?;
    app.text(title, "Workspace preferences")?;
    let sections = app.child(root, Kind::Accordion, "sections")?;
    let account = app.child(sections, Kind::Item, "account")?;
    app.text(account, "Account")?;
    let account_body = app.child(account, Kind::Label, "account-body")?;
    app.text(account_body, "Personal details")?;
    let editor = app.child(sections, Kind::Item, "editor")?;
    app.text(editor, "Editor")?;
    let editor_body = app.child(editor, Kind::Label, "editor-body")?;
    app.text(editor_body, "Auto-save is enabled.")?;
    let advanced = app.child(root, Kind::Collapsible, "advanced")?;
    app.text(advanced, "Advanced")?;
    let cache = app.child(advanced, Kind::Label, "cache")?;
    app.text(cache, "Keep local cache")?;
    if app.supports_disclosure_presentation() {
        app.configure_disclosure(
            advanced,
            Some(&DisclosureConfig {
                width: 360.,
                borderless: true,
                ..Default::default()
            }),
        )?;
        app.configure_disclosure(
            sections,
            Some(&DisclosureConfig {
                width: 360.,
                design_size: DesignSize::Lg,
                joined: true,
                bordered: true,
                single: true,
                allow_collapse: true,
                collapsed_icon: "plus",
                expanded_icon: "minus",
                ..Default::default()
            }),
        )?;
        app.set_disclosure_trigger_style(
            editor,
            Some(&DisclosureTriggerStyle {
                hover_rgba: Some(0x4879e02b),
                cursor: Some(DisclosureCursor::Pointer),
                ..Default::default()
            }),
        )?;
    }
    if app.supports_disclosure_heading() {
        for (layer, font_size, line_height, weight) in [
            (
                DisclosureHeadingLayer::Normal,
                17.,
                24.,
                DisclosureWeight::Medium,
            ),
            (
                DisclosureHeadingLayer::Expanded,
                19.,
                26.,
                DisclosureWeight::Bold,
            ),
        ] {
            app.set_disclosure_heading(
                sections,
                layer,
                Some(&DisclosureHeading {
                    typography: Some(DisclosureTypography {
                        font_size,
                        line_height,
                        weight,
                        wrap: true,
                        ..Default::default()
                    }),
                    ..Default::default()
                }),
            )?;
        }
    }
    if app.supports_disclosure_title_state() {
        app.set_disclosure_title_state(
            sections,
            DisclosureTitleState::Hovered,
            Some(&DisclosureTitleStyle {
                typography: Some(DisclosureTypography {
                    font_size: 17.,
                    line_height: 24.,
                    weight: DisclosureWeight::Semibold,
                    wrap: true,
                    ..Default::default()
                }),
                ..Default::default()
            }),
        )?;
        app.set_disclosure_title_state(
            sections,
            DisclosureTitleState::FocusVisible,
            Some(&DisclosureTitleStyle {
                title_rgba: Some(0x275dccff),
                ..Default::default()
            }),
        )?;
    }
    if app.supports_disclosure_divider() {
        app.set_disclosure_divider(
            sections,
            Some(&DisclosureDivider {
                thickness: 2.,
                inset_left: 12.,
                inset_right: 12.,
                color_rgba: 0x4d80c0aa,
            }),
        )?;
    }
    let status = app.child(root, Kind::Label, "status")?;
    app.text(status, "Open a section to edit your preferences.")?;
    let expanded = Rc::new(Cell::new(false));
    let changed = expanded.clone();
    app.on_change_event(editor, move |event| changed.set(event.checked))?;
    let update_status = |app: &mut App| -> Result<()> {
        if expanded.replace(false) {
            app.text(status, "Editing workspace preferences.")?;
        }
        Ok(())
    };
    if font.is_some() {
        return app.run_with(update_status);
    }
    app.step(0.016)?;
    let bounds = app.bounds(editor)?;
    for phase in [Phase::Down, Phase::Up] {
        app.pointer(phase, bounds.x + 20., bounds.y + 12.)?;
    }
    assert_eq!(app.frame(0.016)?, 1);
    update_status(&mut app)?;
    let text = app.editable_text(status)?;
    assert_eq!(text, "Editing workspace preferences.");
    println!("{text}");
    Ok(())
}
