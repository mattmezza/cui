//! Optional Linux provider/old-module cases. Run through tests/test_slider_protocol.py.
use cui_builders::*;
fn legacy(a: &mut App) -> Node {
    let root = a.root(Kind::Column, "root").unwrap();
    let n = a.child(root, Kind::Slider, "legacy").unwrap();
    a.range(n, 0., 100., 1.).unwrap().value(n, 40.).unwrap();
    a.step(0.016).unwrap();
    assert_eq!(a.current_value(n).unwrap(), 40.);
    n
}
#[test]
fn optional_protocol() {
    let Ok(mode) = std::env::var("CUI_RUST_SLIDER_PROTOCOL") else {
        return;
    };
    let mut a = App::headless(400., 200.).unwrap();
    let n = legacy(&mut a);
    if mode == "old" || mode == "missing" || mode == "unavailable" || mode.ends_with("_size") {
        assert!(!a.supports_slider_model());
        assert_eq!(a.enable_slider_requests(64), Err(Error(9)));
        assert_eq!(a.current_value(n).unwrap(), 40.);
        return;
    }
    assert!(a.supports_slider_model());
    a.enable_slider_requests(64).unwrap();
    if mode == "config_parent" {
        assert_eq!(a.slider_config(n), Err(Error(7)));
        return;
    }
    let result = a.poll_slider_request();
    if mode != "edges" {
        assert!(matches!(result, Err(Error(7))), "{mode}: {result:?}");
        return;
    }
    let r = result.unwrap().unwrap();
    let info = r.receipt.info();
    assert_eq!(info.app_epoch, u64::MAX);
    assert_eq!(info.slot, u32::MAX);
    assert_eq!(info.generation, u32::MAX);
    assert_eq!(info.source_generation, u64::MAX);
    assert_eq!(info.configuration_revision, u64::MAX);
    assert_eq!(info.submission_frame, u64::MAX);
    assert_eq!(info.request_sequence, u64::MAX);
    assert_eq!(r.event_target, u64::MAX);
    assert_eq!(r.event_frame, u64::MAX);
    assert_eq!(r.event_index, u64::MAX);
    assert_eq!(r.input.pointer_id, u64::MAX);
    assert_eq!(r.input.pointer_device, u64::MAX);
    assert_eq!(r.input.modifiers, 15);
    assert_eq!(r.input.repeat, 1);
    assert_eq!(
        r.current,
        SliderValue::Range {
            start: 20.,
            end: 60.
        }
    );
    let copy = r.clone();
    drop(a);
    assert_eq!(copy.receipt.info(), info);
    assert_eq!(copy.requested, r.requested);
}
