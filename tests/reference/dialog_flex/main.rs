// SPDX-License-Identifier: MIT
// Standalone pinned Taffy numerical oracle; never links GPUI or CUI.
use taffy::prelude::*;
use taffy::{Overflow, Point};

#[derive(Clone, Copy)]
struct Case {
    name: &'static str,
    height: Option<f32>,
    max: f32,
    min: f32,
    header: f32,
    title: f32,
    description: f32,
    footer: f32,
    content: Option<f32>,
    body: Option<f32>,
    content_min: Option<f32>,
    content_max: Option<f32>,
    content_fixed: Option<f32>,
    padding_top: f32, padding_bottom: f32, panel_gap: f32, inner_gap: f32,
}
fn col() -> Style {
    Style {
        flex_direction: FlexDirection::Column,
        size: Size { width: Dimension::from_length(240.), height: Dimension::AUTO },
        ..Default::default()
    }
}
fn flex1() -> Style {
    Style {
        flex_grow: 1.,
        flex_shrink: 1.,
        flex_basis: Dimension::from_percent(0.),
        ..col()
    }
}
fn leaf(tree: &mut TaffyTree<()>, height: f32) -> NodeId {
    tree.new_leaf(Style {
        size: Size { width: Dimension::from_length(240.), height: Dimension::from_length(height) },
        flex_shrink: 0.,
        ..col()
    }).unwrap()
}
fn y(tree: &TaffyTree<()>, id: Option<NodeId>) -> f32 {
    id.map(|v| tree.layout(v).unwrap().location.y).unwrap_or(0.)
}
fn h(tree: &TaffyTree<()>, id: Option<NodeId>) -> f32 {
    id.map(|v| tree.layout(v).unwrap().size.height).unwrap_or(0.)
}
fn run(c: Case) {
    let mut tree: TaffyTree<()> = TaffyTree::new();
    tree.disable_rounding();
    let mut children = Vec::new();
    for height in [c.header, c.title, c.description] {
        if height > 0. { children.push(leaf(&mut tree, height)); }
    }
    let content = c.content.map(|height| {
        let child = leaf(&mut tree, height);
        let mut style = flex1();
        if let Some(v) = c.content_min { style.min_size.height = Dimension::from_length(v); }
        if let Some(v) = c.content_max { style.max_size.height = Dimension::from_length(v); }
        if let Some(v) = c.content_fixed {
            style.flex_grow = 0.;
            style.flex_shrink = 0.;
            style.flex_basis = Dimension::AUTO;
            style.size.height = Dimension::from_length(v);
        }
        let node = tree.new_with_children(style, &[child]).unwrap();
        children.push(node);
        node
    });
    let mut viewport = None;
    let body = c.body.map(|height| {
        let child = leaf(&mut tree, height);
        let view = tree.new_with_children(Style {
            size: Size { width: Dimension::from_percent(1.), height: Dimension::from_percent(1.) },
            overflow: Point { x: Overflow::Visible, y: Overflow::Scroll },
            ..col()
        }, &[child]).unwrap();
        viewport = Some(view);
        let node = tree.new_with_children(Style {
            overflow: Point { x: Overflow::Hidden, y: Overflow::Hidden },
            ..flex1()
        }, &[view]).unwrap();
        children.push(node);
        node
    });
    let inner = tree.new_with_children(Style {
        gap: Size { width: LengthPercentage::ZERO, height: LengthPercentage::from_length(c.inner_gap) },
        overflow: Point { x: Overflow::Hidden, y: Overflow::Hidden },
        ..flex1()
    }, &children).unwrap();
    let mut panel_children = vec![inner];
    let footer = if c.footer > 0. {
        let v = leaf(&mut tree, c.footer);
        panel_children.push(v);
        Some(v)
    } else { None };
    let panel = tree.new_with_children(Style {
        padding: Rect { left: LengthPercentage::ZERO, right: LengthPercentage::ZERO,
            top: LengthPercentage::from_length(c.padding_top), bottom: LengthPercentage::from_length(c.padding_bottom) },
        gap: Size { width: LengthPercentage::ZERO, height: LengthPercentage::from_length(c.panel_gap) },
        size: Size {
            width: Dimension::from_length(240.),
            height: c.height.map(Dimension::from_length).unwrap_or(Dimension::AUTO),
        },
        min_size: Size { width: Dimension::AUTO, height: Dimension::from_length(c.min) },
        max_size: Size { width: Dimension::AUTO, height: Dimension::from_length(c.max) },
        ..col()
    }, &panel_children).unwrap();
    tree.compute_layout(panel, Size {
        width: AvailableSpace::Definite(240.), height: AvailableSpace::Definite(c.max),
    }).unwrap();
    println!("{},{:.6},{:.6},{:.6},{:.6},{:.6},{:.6},{:.6},{:.6}", c.name,
        h(&tree, Some(panel)), h(&tree, Some(inner)), h(&tree, content), h(&tree, body),
        content.map(|_| y(&tree, Some(inner)) + y(&tree, content)).unwrap_or(0.),
        body.map(|_| y(&tree, Some(inner)) + y(&tree, body)).unwrap_or(0.),
        y(&tree, footer), h(&tree, viewport));
}
fn main() {
    println!("case,panel,inner,content,body,content_y,body_y,footer_y,viewport");
    let base = Case { name: "base", height: Some(260.), max: 260., min: 0.,
        header: 20., title: 20., description: 20., footer: 40., content: Some(40.), body: Some(800.),
        content_min: None, content_max: None, content_fixed: None,
        padding_top: 0., padding_bottom: 0., panel_gap: 0., inner_gap: 0. };
    // Frozen dialog.rs defaults: padding16, panel gap max(top,8)=16;
    // gap_y_2 at rem16=8. Header40 includes an authored Description;
    // Title20 remains independently submitted. No fictitious Description field.
    let source = Case { name: "source", min: 96., header: 40., description: 0.,
        padding_top: 16., padding_bottom: 16., panel_gap: 16., inner_gap: 8., ..base };
    for c in [
        Case { name: "both_fit_minimum", ..base },
        Case { name: "content_freezes", content: Some(120.), ..base },
        Case { name: "content_minimum_overflow", content: Some(240.), ..base },
        Case { name: "content_minimum_zero", content: Some(240.), content_min: Some(0.), ..base },
        Case { name: "content_max_freezes", content: Some(20.), content_max: Some(30.), ..base },
        Case { name: "content_fixed", content: Some(240.), content_fixed: Some(50.), ..base },
        Case { name: "body_only", content: None, ..base },
        Case { name: "content_only", body: None, ..base },
        Case { name: "neither", content: None, body: None, ..base },
        Case { name: "both_empty", content: Some(0.), body: Some(0.), ..base },
        Case { name: "auto_short", height: None, content: Some(10.), body: Some(20.), ..base },
        Case { name: "auto_long", height: None, ..base },
        Case { name: "auto_content_overflow", height: None, content: Some(240.), ..base },
        Case { name: "auto_source_min96", height: None, min: 96., header: 0., title: 20.,
            description: 0., footer: 0., content: None, body: Some(10.), ..base },
        Case { name: "auto_no_spacer", height: None, header: 0., title: 20.,
            description: 0., footer: 0., content: None, body: Some(10.), ..base },
        Case { name: "fixed_sibling_overflow", header: 300., ..base },
        Case { name: "resize_short", height: Some(160.), max: 160., ..base },
        Case { name: "resize_tall", height: Some(400.), max: 400., ..base },
        Case { name: "source_defaults", ..source },
        Case { name: "source_body_omitted", body: None, ..source },
        Case { name: "source_body_empty", body: Some(0.), ..source },
        Case { name: "source_neither", content: None, body: None, ..source },
        Case { name: "source_content_overflow", content: Some(240.), ..source },
        Case { name: "source_auto_short", height: None, content: Some(10.), body: Some(20.), ..source },
        Case { name: "source_footer_omitted", footer: 0., ..source },
        Case { name: "source_footer_oversize_fixed", footer: 400., ..source },
        Case { name: "source_footer_oversize_auto", height: None, footer: 400., ..source },
        Case { name: "source_footer_auto_fits", height: None, max: 600., footer: 400.,
            content: Some(10.), body: Some(20.), ..source },

    ] { run(c); }
}
