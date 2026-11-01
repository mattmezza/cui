use cui_builders::*;
use std::{
    cell::{Cell, RefCell},
    rc::Rc,
};
fn pair(single: bool, end: f64) -> SliderValue {
    if single {
        SliderValue::Single { end }
    } else {
        SliderValue::Range { start: 20., end }
    }
}
fn setup(single: bool, vertical: bool) -> (App, Node, Node) {
    let mut a = App::headless(600., 500.).unwrap();
    assert!(a.supports_slider_model());
    a.enable_slider_requests(64).unwrap();
    let root = a.root(Kind::Column, "root").unwrap();
    let row = a.child(root, Kind::Row, "row").unwrap();
    let c = SliderConfig {
        source_generation: 0xfedcba9876543210,
        orientation: if vertical {
            SliderOrientation::Vertical
        } else {
            SliderOrientation::Horizontal
        },
        ..Default::default()
    };
    let n = a.slider_model(row, "typed", c, pair(single, 60.)).unwrap();
    a.step(0.016).unwrap();
    (a, n, row)
}
fn focus(a: &mut App, n: Node) {
    let b = a
        .slider_part_bounds(n, SliderPart::EndThumb)
        .unwrap()
        .unwrap();
    assert!(b.width > 0. && b.height > 0.);
    a.pointer(Phase::Down, b.x + b.width / 2., b.y + b.height / 2.)
        .unwrap();
    a.pointer(Phase::Up, b.x + b.width / 2., b.y + b.height / 2.)
        .unwrap();
    a.step(0.016).unwrap();
    while a.poll_slider_request().unwrap().is_some() {}
}
fn keys(a: &mut App, vertical: bool) {
    let k = if vertical { Key::Down } else { Key::Left };
    a.key(true, k, 15, true).unwrap();
    a.key(true, k, 15, true).unwrap();
    a.step(0.016).unwrap();
}
#[test]
fn typed_pairs_axes_order_and_explicit_acceptance() {
    for single in [false, true] {
        for vertical in [false, true] {
            let (mut a, n, _) = setup(single, vertical);
            focus(&mut a, n);
            let before = a.slider_value(n).unwrap();
            let seen = Rc::new(RefCell::new(Vec::new()));
            let out = seen.clone();
            a.on_slider_request(n, move |ctx, r| {
                if r.phase == SliderPhase::Update {
                    ctx.accept(
                        &r.receipt,
                        if single {
                            SliderValue::Single { end: 55. }
                        } else {
                            SliderValue::Range {
                                start: 25.,
                                end: 55.,
                            }
                        },
                    )?;
                }
                out.borrow_mut().push(r);
                Ok(())
            })
            .unwrap();
            keys(&mut a, vertical);
            assert!(seen.borrow().is_empty());
            assert_eq!(a.dispatch().unwrap(), 6);
            let r = seen.borrow();
            assert_eq!(r.len(), 6);
            assert_eq!(
                r.iter().map(|r| r.phase).collect::<Vec<_>>(),
                vec![
                    SliderPhase::Begin,
                    SliderPhase::Update,
                    SliderPhase::Commit,
                    SliderPhase::Begin,
                    SliderPhase::Update,
                    SliderPhase::Commit
                ]
            );
            for item in r.iter() {
                assert_eq!(item.current, before);
                assert_eq!(item.receipt.info().source_generation, 0xfedcba9876543210);
                assert_eq!(item.input.modifiers, 15);
                assert_eq!(item.input.repeat, 1);
            }
            assert_ne!(r[1].requested, r[4].requested);
            assert_eq!(
                a.slider_value(n).unwrap(),
                if single {
                    SliderValue::Single { end: 55. }
                } else {
                    SliderValue::Range {
                        start: 25.,
                        end: 55.,
                    }
                }
            );
            let copy = r[1].clone();
            drop(r);
            a.step(0.016).unwrap();
            assert!(a.accept_slider_request(&copy.receipt, before).is_err());
            drop(a);
            assert_eq!(copy.current, before);
            assert_eq!(copy.receipt.info().source_generation, 0xfedcba9876543210);
        }
    }
}
#[test]
fn rejecting_polling_precision_and_revision() {
    let (mut a, n, _) = setup(false, false);
    focus(&mut a, n);
    let before = a.slider_value(n).unwrap();
    keys(&mut a, false);
    let mut records = Vec::new();
    while let Some(r) = a.poll_slider_request().unwrap() {
        records.push(r)
    }
    assert_eq!(records.len(), 6);
    assert_eq!(a.slider_value(n).unwrap(), before);
    let exact = SliderValue::Range {
        start: 20.,
        end: f64::from_bits(60f64.to_bits() + 1),
    };
    a.accept_slider_request(&records[1].receipt, exact).unwrap();
    assert_eq!(a.slider_value(n).unwrap(), exact);
    assert!(a
        .accept_slider_request(&records[1].receipt, before)
        .is_err());
    let c = a.slider_config(n).unwrap();
    a.configure_slider(n, c, before).unwrap();
    assert!(a
        .accept_slider_request(&records[4].receipt, before)
        .is_err());
    let mut other = App::headless(100., 100.).unwrap();
    other.enable_slider_requests(8).unwrap();
    assert!(other
        .accept_slider_request(&records[4].receipt, before)
        .is_err());
}
#[test]
fn callback_error_panic_replacement_and_removal() {
    let (mut a, n, row) = setup(false, false);
    focus(&mut a, n);
    let before = a.slider_value(n).unwrap();
    a.on_slider_request(n, |ctx, r| {
        if r.phase == SliderPhase::Update {
            ctx.accept(&r.receipt, r.requested)?;
            return Err(Error(3));
        }
        Ok(())
    })
    .unwrap();
    keys(&mut a, false);
    assert_eq!(a.dispatch(), Err(Error(3)));
    assert_ne!(a.slider_value(n).unwrap(), before); // Accepted work is not rolled back.
    a.remove_slider_handler(n).unwrap();
    a.dispatch_slider_requests().unwrap();
    a.on_slider_request(n, |_, _| panic!("typed callback panic"))
        .unwrap();
    keys(&mut a, false);
    assert!(std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| a.dispatch())).is_err());
    a.remove_slider_handler(n).unwrap();
    a.dispatch_slider_requests().unwrap();
    a.step(0.016).unwrap();
    let replacement = Rc::new(Cell::new(0));
    let received = replacement.clone();
    a.on_slider_request(n, move |ctx, _| {
        let r = received.clone();
        ctx.on_slider_request(n, move |_, _| {
            r.set(r.get() + 1);
            Ok(())
        })
    })
    .unwrap();
    keys(&mut a, false);
    assert_eq!(a.dispatch().unwrap(), 6);
    assert_eq!(replacement.get(), 5);
    let retained = Rc::new(());
    let weak = Rc::downgrade(&retained);
    let captured = retained.clone();
    a.on_slider_request(n, move |_, _| {
        let _ = &captured;
        Ok(())
    })
    .unwrap();
    drop(retained);
    assert!(weak.upgrade().is_some());
    a.remove(row).unwrap();
    assert!(weak.upgrade().is_none());
    assert!(a.slider_value(n).is_err());
}
#[test]
fn request_close_stops_polling_and_releases_handlers() {
    let (mut a, n, _) = setup(false, false);
    focus(&mut a, n);
    let seen = Rc::new(RefCell::new(Vec::new()));
    let out = seen.clone();
    let owner = Rc::new(Cell::new(pair(false, 60.)));
    let accepted = owner.clone();
    let lifetime = Rc::new(());
    let weak = Rc::downgrade(&lifetime);
    let capture = lifetime.clone();
    a.on_slider_request(n, move |ctx, r| {
        let _ = &capture;
        out.borrow_mut().push(r.clone());
        if r.phase == SliderPhase::Update {
            ctx.accept(&r.receipt, r.requested)?;
            accepted.set(r.requested);
            ctx.request_close()?;
        }
        Ok(())
    })
    .unwrap();
    drop(lifetime);
    keys(&mut a, false);
    assert_eq!(a.dispatch().unwrap(), 2);
    assert_eq!(seen.borrow().len(), 2);
    assert_ne!(owner.get(), pair(false, 60.));
    assert!(weak.upgrade().is_none());
    // Request-only close leaves App storage alive; owned records outlive Drop.
    let copy = seen.borrow()[1].clone();
    drop(a);
    assert_eq!(copy.current, pair(false, 60.));
}
#[test]
fn configuration_disabled_bounds_and_atomic_validation() {
    let (mut a, n, _) = setup(false, true);
    let before = a.slider_value(n).unwrap();
    let c = a.slider_config(n).unwrap();
    assert!(a
        .configure_slider(
            n,
            SliderConfig {
                domain: SliderDomain {
                    minimum: 10.,
                    maximum: 1.,
                    step: 1.,
                    scale: SliderScale::Linear
                },
                ..c
            },
            before
        )
        .is_err());
    assert_eq!(a.slider_config(n).unwrap(), c);
    assert_eq!(a.slider_value(n).unwrap(), before);
    assert!(a
        .set_slider_value(n, SliderValue::Single { end: f64::NAN })
        .is_err());
    a.component_disabled(n, true).unwrap();
    a.step(0.016).unwrap();
    for p in [SliderPart::Track, SliderPart::Fill] {
        let b = a.slider_part_bounds(n, p).unwrap().unwrap();
        assert!(b.width > 0. && b.height > 0.)
    }
    assert!(a
        .slider_part_bounds(n, SliderPart::StartThumb)
        .unwrap()
        .is_none());
    assert!(a
        .slider_part_bounds(n, SliderPart::EndThumb)
        .unwrap()
        .is_none());
    let presentation = a.slider_presentation(n).unwrap().unwrap();
    assert_eq!(presentation.render_status, 0);
    assert!(presentation.core_time > 0.);
    a.configure_slider(
        n,
        SliderConfig {
            layout: SliderLayout::Parent,
            length: f32::NAN,
            ..c
        },
        before,
    )
    .unwrap();
    assert_eq!(a.slider_config(n).unwrap().length, 0.);
}
#[test]
fn invalid_ancestry_and_foreign_nodes() {
    let mut a = App::headless(400., 200.).unwrap();
    a.enable_slider_requests(64).unwrap();
    let root = a.root(Kind::Column, "root").unwrap();
    let b = a.child(root, Kind::Button, "button").unwrap();
    assert!(a
        .slider_model(b, "bad", Default::default(), pair(false, 60.))
        .is_err());
    let mut other = App::headless(100., 100.).unwrap();
    assert!(other
        .slider_model(root, "foreign", Default::default(), pair(false, 60.))
        .is_err());
    a.text(b, "Still live").unwrap();
    a.step(0.016).unwrap();
    assert!(a.bounds(b).unwrap().width > 0.);
}

#[test]
fn invalid_domain_factory_does_not_allocate_nodes() {
    // Twin apps expose node identities only through real copied requests. Invalid
    // Rust proposals must consume no slots before the otherwise identical UI.
    let mut identities = Vec::new();
    for invalid_first in [false, true] {
        let mut app = App::headless(400., 300.).unwrap();
        app.enable_slider_requests(64).unwrap();
        let root = app.root(Kind::Column, "root").unwrap();
        if invalid_first {
            for domain in [
                SliderDomain {
                    minimum: -f64::MAX,
                    maximum: f64::MAX,
                    step: 1.,
                    scale: SliderScale::Linear,
                },
                SliderDomain {
                    minimum: 1.,
                    maximum: 1.,
                    step: 0.,
                    scale: SliderScale::Logarithmic,
                },
            ] {
                assert_eq!(
                    app.slider_model(
                        root,
                        "invalid",
                        SliderConfig {
                            domain,
                            ..Default::default()
                        },
                        SliderValue::Single { end: 1. }
                    ),
                    Err(Error(3))
                );
            }
        }
        let n = app
            .slider_model(root, "selection", Default::default(), pair(false, 60.))
            .unwrap();
        app.step(0.016).unwrap();
        focus(&mut app, n);
        keys(&mut app, false);
        let r = app.poll_slider_request().unwrap().unwrap();
        let info = r.receipt.info();
        identities.push((info.slot, info.generation));
    }
    assert_eq!(identities[0], identities[1]);
}

#[test]
fn invalid_domain_configure_preserves_live_values_and_receipts() {
    let (mut app, n, _) = setup(false, false);
    focus(&mut app, n);
    keys(&mut app, false);
    let mut requests = Vec::new();
    while let Some(r) = app.poll_slider_request().unwrap() {
        requests.push(r)
    }
    let config = app.slider_config(n).unwrap();
    let value = app.slider_value(n).unwrap();
    for domain in [
        SliderDomain {
            minimum: -f64::MAX,
            maximum: f64::MAX,
            step: 1.,
            scale: SliderScale::Linear,
        },
        SliderDomain {
            minimum: 1.,
            maximum: 1.,
            step: 0.,
            scale: SliderScale::Logarithmic,
        },
    ] {
        assert_eq!(
            app.configure_slider(n, SliderConfig { domain, ..config }, value),
            Err(Error(3))
        );
        assert_eq!(app.slider_config(n).unwrap(), config);
        assert_eq!(app.slider_value(n).unwrap(), value);
    }
    // Invalid Rust proposals did not revise C receipt authority.
    app.accept_slider_request(&requests[4].receipt, requests[4].requested)
        .unwrap();
}
#[test]
fn context_remove_prunes_later_requests_and_handler_storage() {
    let (mut app, n, _) = setup(false, false);
    focus(&mut app, n);
    let captured = Rc::new(());
    let weak = Rc::downgrade(&captured);
    let owner = captured.clone();
    app.on_slider_request(n, move |ctx, _| {
        let _ = &owner;
        ctx.remove(n)
    })
    .unwrap();
    drop(captured);
    keys(&mut app, false);
    assert_eq!(app.dispatch().unwrap(), 1);
    assert!(weak.upgrade().is_none());
    assert!(app.poll_slider_request().unwrap().is_none());
    assert!(app.slider_value(n).is_err());
}

#[test]
fn legacy_callbacks_run_before_typed_callbacks_without_retiming_step() {
    let (mut app, n, row) = setup(false, false);
    let button = app.child(row, Kind::Button, "policy").unwrap();
    app.text(button, "Reject proposals").unwrap();
    app.step(0.016).unwrap();
    focus(&mut app, n);
    let order = Rc::new(RefCell::new(Vec::new()));
    let old = order.clone();
    let typed = order.clone();
    app.on_click(button, move || old.borrow_mut().push("legacy"))
        .unwrap();
    app.on_slider_request(n, move |_, _| {
        typed.borrow_mut().push("typed");
        Ok(())
    })
    .unwrap();
    let b = app.bounds(button).unwrap();
    app.key(true, Key::Right, 0, false).unwrap();
    app.pointer(Phase::Down, b.x + b.width / 2., b.y + b.height / 2.)
        .unwrap();
    app.pointer(Phase::Up, b.x + b.width / 2., b.y + b.height / 2.)
        .unwrap();
    app.step(0.016).unwrap();
    assert!(order.borrow().is_empty());
    app.dispatch().unwrap();
    let calls = order.borrow();
    assert_eq!(calls.first(), Some(&"legacy"));
    assert!(calls.iter().skip(1).any(|x| *x == "typed"));
}
#[test]
fn queue_capacity_error_does_not_change_owner_or_invoke_handlers() {
    let mut app = App::headless(400., 300.).unwrap();
    app.enable_slider_requests(1).unwrap();
    let root = app.root(Kind::Column, "root").unwrap();
    let before = pair(false, 60.);
    let n = app
        .slider_model(root, "selection", Default::default(), before)
        .unwrap();
    app.step(0.016).unwrap();
    let calls = Rc::new(Cell::new(0));
    let invoked = calls.clone();
    app.on_slider_request(n, move |_, _| {
        invoked.set(invoked.get() + 1);
        Ok(())
    })
    .unwrap();
    app.key(true, Key::Right, 0, false).unwrap();
    assert_eq!(app.step(0.016), Err(Error(5)));
    assert_eq!(calls.get(), 0);
    assert_eq!(app.slider_value(n).unwrap(), before);
    assert!(app.poll_slider_request().unwrap().is_none());
}

#[test]
fn finite_extreme_domains_and_double_values_survive_ffi() {
    for domain in [
        SliderDomain {
            minimum: 1e308,
            maximum: f64::from_bits(1e308f64.to_bits() + 1),
            step: 0.,
            scale: SliderScale::Logarithmic,
        },
        SliderDomain {
            minimum: f64::from_bits(1),
            maximum: 1.,
            step: 0.,
            scale: SliderScale::Logarithmic,
        },
        SliderDomain {
            minimum: -f64::MAX,
            maximum: 0.,
            step: 0.,
            scale: SliderScale::Linear,
        },
    ] {
        let mut app = App::headless(400., 300.).unwrap();
        app.enable_slider_requests(64).unwrap();
        let root = app.root(Kind::Column, "root").unwrap();
        let config = SliderConfig {
            domain,
            ..Default::default()
        };
        let value = SliderValue::Range {
            start: domain.minimum,
            end: domain.maximum,
        };
        let n = app.slider_model(root, "extreme", config, value).unwrap();
        app.step(0.016).unwrap();
        assert_eq!(app.slider_value(n).unwrap(), value);
        assert_eq!(app.slider_config(n).unwrap(), config);
        let b = app
            .slider_part_bounds(n, SliderPart::EndThumb)
            .unwrap()
            .unwrap();
        assert!(b.x.is_finite() && b.y.is_finite() && b.width > 0. && b.height > 0.);
    }
}

#[test]
fn readonly_and_passive_policies_do_not_accept_or_publish_new_input() {
    let (mut app, n, _) = setup(false, false);
    focus(&mut app, n);
    keys(&mut app, false);
    let old = app.poll_slider_request().unwrap().unwrap();
    while app.poll_slider_request().unwrap().is_some() {}
    let before = app.slider_value(n).unwrap();
    let config = app.slider_config(n).unwrap();
    app.configure_slider(
        n,
        SliderConfig {
            readonly: true,
            ..config
        },
        before,
    )
    .unwrap();
    assert!(app.accept_slider_request(&old.receipt, before).is_err());
    app.step(0.016).unwrap();
    keys(&mut app, false);
    assert!(app.poll_slider_request().unwrap().is_none());
    assert_eq!(app.slider_value(n).unwrap(), before);
    app.configure_slider(
        n,
        SliderConfig {
            requests: false,
            ..config
        },
        before,
    )
    .unwrap();
    app.step(0.016).unwrap();
    keys(&mut app, false);
    assert!(app.poll_slider_request().unwrap().is_none());
    assert_eq!(app.slider_value(n).unwrap(), before);
}
#[test]
fn captured_cancel_copies_old_range_and_gesture_domain_without_autoaccept() {
    let (mut app, n, _) = setup(false, false);
    let b = app
        .slider_part_bounds(n, SliderPart::EndThumb)
        .unwrap()
        .unwrap();
    app.pointer(Phase::Down, b.x + b.width / 2., b.y + b.height / 2.)
        .unwrap();
    app.step(0.016).unwrap();
    while app.poll_slider_request().unwrap().is_some() {}
    let old = app.slider_config(n).unwrap();
    let value = SliderValue::Single { end: 5. };
    let domain = SliderDomain {
        minimum: 0.,
        maximum: 10.,
        step: 1.,
        scale: SliderScale::Linear,
    };
    app.configure_slider(
        n,
        SliderConfig {
            domain,
            source_generation: old.source_generation + 1,
            ..old
        },
        value,
    )
    .unwrap();
    app.step(0.016).unwrap();
    let mut cancel = None;
    while let Some(r) = app.poll_slider_request().unwrap() {
        if r.phase == SliderPhase::Cancel {
            cancel = Some(r)
        }
    }
    let r = cancel.expect("real captured cancellation");
    assert_eq!(r.current, value);
    assert_eq!(r.requested, pair(false, 60.));
    assert_eq!(r.domain, domain);
    assert_eq!(r.gesture_domain, old.domain);
    assert!(app.accept_slider_request(&r.receipt, r.requested).is_err());
    assert_eq!(app.slider_value(n).unwrap(), value);
}
