//! Actual authored English/Arabic UI; default windowless, --native is manual.
use cui_builders::*;
use std::{cell::RefCell, rc::Rc};
fn main() -> Result<()> {
    let native = std::env::args().any(|s| s == "--native");
    let mut app = if native {
        App::sdl(
            800.,
            400.,
            "Localized time",
            &std::env::var("CUI_TEST_FONT").expect("native requires CUI_TEST_FONT"),
        )?
    } else {
        App::headless(800., 400.)?
    };
    app.enable_localized_time_fields(16)?;
    let root = app.root(Kind::Column, "root")?;
    let title = app.child(root, Kind::Label, "title")?;
    app.text(title, "English 24h / العربية 12h")?;
    let row = app.child(root, Kind::Row, "fields")?;
    let mut english = TimeFieldConfig::english();
    let mut arabic = TimeFieldConfig::arabic();
    for c in [&mut english, &mut arabic] {
        c.minimum = Some(Time {
            hour: 8,
            minute: 0,
            second: 0,
        });
        c.maximum = Some(Time {
            hour: 18,
            minute: 0,
            second: 0,
        });
        c.edit_generation = 1
    }
    let en = app.localized_time_field(
        row,
        "english",
        Time {
            hour: 9,
            minute: 30,
            second: 0,
        },
        &english,
    )?;
    let ar = app.localized_time_field(
        row,
        "arabic",
        Time {
            hour: 14,
            minute: 15,
            second: 0,
        },
        &arabic,
    )?;
    let summary = app.child(root, Kind::Label, "summary")?;
    app.text(summary, "No applied edits")?;
    let changes = Rc::new(RefCell::new(Vec::<TimeFieldChange>::new()));
    for n in [en, ar] {
        let out = changes.clone();
        app.on_time_field_change(n, move |c| out.borrow_mut().push(c.clone()))?;
    }
    let controls = app.child(root, Kind::Row, "controls")?;
    let commands = Rc::new(RefCell::new(Vec::<u8>::new()));
    for (i, label) in [
        "Clear validation",
        "Toggle read-only",
        "Toggle disabled",
        "Toggle bounds",
    ]
    .iter()
    .enumerate()
    {
        let n = app.child(controls, Kind::Button, &format!("control-{i}"))?;
        app.text(n, label)?;
        let out = commands.clone();
        app.on_click(n, move || out.borrow_mut().push(i as u8))?;
    }
    let mut apply = move |app: &mut App| -> Result<()> {
        for command in commands.borrow_mut().drain(..) {
            let mut next = english.clone();
            match command {
                0 => {
                    app.clear_time_validation(en)?;
                    app.clear_time_validation(ar)?;
                    continue;
                }
                1 => next.readonly = !next.readonly,
                2 => next.disabled = !next.disabled,
                _ => {
                    if next.minimum.is_some() {
                        next.minimum = None;
                        next.maximum = None
                    } else {
                        next.minimum = Some(Time {
                            hour: 8,
                            minute: 0,
                            second: 0,
                        });
                        next.maximum = Some(Time {
                            hour: 18,
                            minute: 0,
                            second: 0,
                        })
                    };
                    next.edit_generation += 1
                }
            };
            app.configure_localized_time(en, &next)?;
            english = next;
        }
        for c in changes.borrow_mut().drain(..) {
            app.text(
                summary,
                &format!(
                    "Applied {:02}:{:02} → {:02}:{:02}; frame status {}",
                    c.previous.hour,
                    c.previous.minute,
                    c.value.hour,
                    c.value.minute,
                    c.frame_status
                ),
            )?;
        }
        Ok(())
    };
    if native {
        return app.run_with(apply);
    }
    app.step(0.016)?;
    for (n, text) in [(en, "10:45"), (ar, "٠٣:٢٠ م")] {
        let r = app.bounds(n)?;
        app.pointer(Phase::Down, r.x + r.width / 2., r.y + r.height / 2.)?;
        app.pointer(Phase::Up, r.x + r.width / 2., r.y + r.height / 2.)?;
        app.queue_text(text)?;
        app.step(0.016)?;
        app.dispatch()?;
        apply(&mut app)?;
    }
    println!("English applied: {:?}", app.localized_time_result(en)?);
    Ok(())
}
