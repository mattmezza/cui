// SPDX-License-Identifier: MIT
// Additional independent Panel minimum-versus-maximum oracle. Does not alter
// the frozen28-row driver, CSV or receipt. Uses its exact cached Taffy rlib.
use taffy::prelude::*;
fn main() {
    println!("case,min,max,body,panel");
    for (name, minimum, maximum, body) in [
        ("small_short", 96., 48., 20.),
        ("small_long", 96., 48., 900.),
        ("tiny_empty", 96., 8., 0.),
        ("ordinary_short", 96., 568., 20.),
    ] {
        let mut tree: TaffyTree<()> = TaffyTree::new();
        let child = tree.new_leaf(Style {
            size: Size { width: Dimension::from_length(148.), height: Dimension::from_length(body) },
            flex_shrink: 0.,
            ..Default::default()
        }).unwrap();
        let panel = tree.new_with_children(Style {
            flex_direction: FlexDirection::Column,
            size: Size { width: Dimension::from_length(148.), height: Dimension::AUTO },
            min_size: Size { width: Dimension::AUTO, height: Dimension::from_length(minimum) },
            max_size: Size { width: Dimension::AUTO, height: Dimension::from_length(maximum) },
            ..Default::default()
        }, &[child]).unwrap();
        tree.compute_layout(panel, Size::MAX_CONTENT).unwrap();
        println!("{name},{minimum:.6},{maximum:.6},{body:.6},{:.6}",
                 tree.layout(panel).unwrap().size.height);
    }
}
