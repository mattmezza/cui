use cui_builders::*;
use std::{cell::Cell,rc::Rc,sync::Mutex};
static NATIVE: Mutex<()> = Mutex::new(());
#[test] fn layout_click_keyboard_and_stale() {
 let _guard=NATIVE.lock().unwrap();
 let mut a=App::headless(400.,200.).unwrap();
 let root=a.root(Kind::Column,"root").unwrap(); a.gap_units(root,2.).unwrap();
 let label=a.child(root,Kind::Label,"label").unwrap();a.text(label,"Save changes ✓").unwrap();
 let button=a.child(root,Kind::Button,"button").unwrap();a.text(button,"Save").unwrap();a.size(button,Size::X9l).unwrap();
 let count=Rc::new(Cell::new(0));let c=count.clone();a.on_click(button,move||c.set(c.get()+1)).unwrap();
 a.step(00.016).unwrap();let b=a.bounds(button).unwrap();assert!(b.width>0. && b.height>0.);
 a.pointer(Phase::Down,b.x+b.width/2.,b.y+b.height/2.).unwrap();a.pointer(Phase::Up,b.x+b.width/2.,b.y+b.height/2.).unwrap();a.frame(00.016).unwrap();assert_eq!(count.get(),1);
 a.disabled(button,true).unwrap();a.pointer(Phase::Down,b.x+1.,b.y+1.).unwrap();a.pointer(Phase::Up,b.x+1.,b.y+1.).unwrap();a.frame(00.016).unwrap();assert_eq!(count.get(),1);
 a.remove(button).unwrap();assert_eq!(a.bounds(button).unwrap_err(),Error(4));
 let mut other=App::headless(100.,100.).unwrap();assert_eq!(other.text(label,"wrong").err(),Some(Error(3)));
 assert_eq!(a.text(label,"nul\0bad").err(),Some(Error(3)));
 assert_eq!(a.text(label,&"x".repeat(4097)).err(),Some(Error(3)));
}
#[test] fn keyboard_click() {
 let _guard=NATIVE.lock().unwrap();
 let mut app=App::headless(300.,100.).unwrap();let root=app.root(Kind::Column,"root").unwrap();let b=app.child(root,Kind::Button,"go").unwrap();app.text(b,"Go").unwrap();
 let n=Rc::new(Cell::new(0));let m=n.clone();app.on_click(b,move||m.set(m.get()+1)).unwrap();app.step(00.016).unwrap();
 app.key(true,Key::Tab,0,false).unwrap();app.key(true,Key::Enter,0,false).unwrap();app.key(false,Key::Enter,0,false).unwrap();app.frame(00.016).unwrap();assert_eq!(n.get(),1);
}
#[test] fn component_changes_and_validation() {
 let _guard=NATIVE.lock().unwrap();
 let mut a=App::headless(600.,400.).unwrap();let root=a.root(Kind::Column,"root").unwrap();
 let checkbox=a.child(root,Kind::Checkbox,"check").unwrap();a.text(checkbox,"Confirm ✓").unwrap();
 let slider=a.child(root,Kind::Slider,"slider").unwrap();a.range(slider,0.,10.,1.) .unwrap().value(slider,5.).unwrap();
 let progress=a.child(root,Kind::Progress,"progress").unwrap();a.value(progress,0.5).unwrap();
 assert_eq!(a.current_value(slider).unwrap(),5.);assert_eq!(a.value(slider,f64::NAN).err(),Some(Error(3)));
 let count=Rc::new(Cell::new(0));let c=count.clone();a.on_change(checkbox,move||c.set(c.get()+1)).unwrap();
 a.step(0.016).unwrap();let b=a.bounds(checkbox).unwrap();assert!(b.width>0.);
 a.pointer(Phase::Down,b.x+b.width/2.,b.y+b.height/2.).unwrap();a.pointer(Phase::Up,b.x+b.width/2.,b.y+b.height/2.).unwrap();a.frame(0.016).unwrap();
 assert!(a.is_checked(checkbox).unwrap());assert_eq!(count.get(),1);
 a.component_disabled(checkbox,true).unwrap();a.step(0.016).unwrap();a.pointer(Phase::Down,b.x+1.,b.y+1.).unwrap();a.pointer(Phase::Up,b.x+1.,b.y+1.).unwrap();a.frame(0.016).unwrap();assert_eq!(count.get(),1);
}
#[test] fn text_event_snapshots() {
 let _guard=NATIVE.lock().unwrap();let mut a=App::headless(600.,300.).unwrap();
 let root=a.root(Kind::Column,"root").unwrap();let input=a.child(root,Kind::Input,"input").unwrap();a.text(input,"hé").unwrap();
 let values=Rc::new(std::cell::RefCell::new(Vec::new()));let captured=values.clone();
 a.on_change_event(input,move|event|captured.borrow_mut().push(event.text.clone())).unwrap();
 a.step(0.016).unwrap();let b=a.bounds(input).unwrap();
 a.pointer(Phase::Down,b.x+b.width/2.,b.y+b.height/2.).unwrap();a.pointer(Phase::Up,b.x+b.width/2.,b.y+b.height/2.).unwrap();a.step(0.016).unwrap();
 a.queue_text("✓").unwrap();a.step(0.016).unwrap();a.queue_text("Z").unwrap();a.step(0.016).unwrap();
 assert_eq!(a.editable_text(input).unwrap(),"hé✓Z");a.dispatch().unwrap();assert_eq!(&*values.borrow(), &["hé✓","hé✓Z"]);
 assert_eq!(a.queue_text("bad\0text"),Err(Error(3)));
}
#[test]
fn grouped_selection_and_disclosure() {
    let _guard = NATIVE.lock().unwrap();
    let mut app = App::headless(500., 300.).unwrap();
    let root = app.root(Kind::Column, "root").unwrap();
    let group = app.child(root, Kind::RadioGroup, "choices").unwrap();
    let one = app.child(group, Kind::Item, "one").unwrap();
    app.text(one, "One").unwrap();
    let two = app.child(group, Kind::Item, "two").unwrap();
    app.text(two, "Two").unwrap();
    assert_eq!(app.selected(group).unwrap(), None);
    let selected = Rc::new(std::cell::RefCell::new(Vec::new()));
    let captured = selected.clone();
    app.on_change_event(group, move |e| captured.borrow_mut().push(e.selected))
        .unwrap();
    app.step(0.016).unwrap();
    for item in [one, two] {
        let b = app.bounds(item).unwrap();
        app.pointer(Phase::Down, b.x + b.width / 2., b.y + b.height / 2.)
            .unwrap();
        app.pointer(Phase::Up, b.x + b.width / 2., b.y + b.height / 2.)
            .unwrap();
        app.step(0.016).unwrap();
    }
    assert_eq!(app.selected(group).unwrap(), Some(two));
    app.dispatch().unwrap();
    assert_eq!(&*selected.borrow(), &[Some(one), Some(two)]);
    app.select(group, one).unwrap();
    assert_eq!(app.selected(group).unwrap(), Some(one));
    let alert = app.child(root, Kind::Alert, "notice").unwrap();
    app.text(alert, "Title")
        .unwrap()
        .detail_text(alert, "Details ✓")
        .unwrap();
    let disclosure = app.child(root, Kind::Collapsible, "more").unwrap();
    app.text(disclosure, "More")
        .unwrap()
        .open(disclosure, true)
        .unwrap();
    assert!(app.is_checked(disclosure).unwrap());
    assert_eq!(app.select(group, disclosure).err(), Some(Error(3)));
    app.step(0.016).unwrap();
    assert!(app.bounds(alert).unwrap().height > 0.);
}
#[test]
fn select_and_combobox_options() {
    let _guard = NATIVE.lock().unwrap();
    let mut app = App::headless(500., 250.).unwrap();
    let root = app.root(Kind::Column, "root").unwrap();
    let select = app.child(root, Kind::Select, "select").unwrap();
    app.text(select, "Fruit")
        .unwrap()
        .detail_text(select, "Choose fruit")
        .unwrap();
    let apple = app.child(select, Kind::Item, "apple").unwrap();
    app.text(apple, "Apple").unwrap();
    let pear = app.child(select, Kind::Item, "pear").unwrap();
    app.text(pear, "Pear").unwrap();
    app.select(select, pear).unwrap();
    assert_eq!(app.selected(select).unwrap(), Some(pear));
    let combo = app.child(root, Kind::ComboBox, "combo").unwrap();
    app.detail_text(combo, "Search")
        .unwrap()
        .text(combo, "App")
        .unwrap();
    let option = app.child(combo, Kind::Item, "option").unwrap();
    app.text(option, "Apple").unwrap();
    assert_eq!(app.editable_text(combo).unwrap(), "App");
    app.select(combo, option).unwrap();
    assert_eq!(app.selected(combo).unwrap(), Some(option));
    app.step(0.016).unwrap();
    assert!(app.bounds(select).unwrap().width > 0.);
}
#[test]
fn capacity_and_invalid_dimensions() {
    let _guard = NATIVE.lock().unwrap();
    assert_eq!(App::headless(f32::NAN, 100.).err(), Some(Error(3)));
    let mut app = App::headless(300., 100.).unwrap();
    let root = app.root(Kind::Column, "root").unwrap();
    for i in 0..255 {
        app.child(root, Kind::Label, &format!("label-{i}")).unwrap();
    }
    assert_eq!(
        app.child(root, Kind::Label, "overflow").err(),
        Some(Error(5))
    );
}
#[test]
fn dropdown_action_and_menu_item() {
    let _guard = NATIVE.lock().unwrap();
    let mut app = App::headless(400., 200.).unwrap();
    let root = app.root(Kind::Column, "root").unwrap();
    let dropdown = app.child(root, Kind::DropdownButton, "dropdown").unwrap();
    app.text(dropdown, "Save")
        .unwrap()
        .detail_text(dropdown, "More")
        .unwrap();
    let item = app.child(dropdown, Kind::Item, "export").unwrap();
    app.text(item, "Export").unwrap();
    let clicks = Rc::new(Cell::new(0));
    let c = clicks.clone();
    app.on_click(dropdown, move || c.set(c.get() + 1)).unwrap();
    let selections = Rc::new(std::cell::RefCell::new(Vec::new()));
    let captured = selections.clone();
    app.on_change_event(dropdown, move |e| captured.borrow_mut().push(e.selected))
        .unwrap();
    app.step(0.016).unwrap();
    let action = app.dropdown_part_bounds(dropdown, false).unwrap();
    let menu = app.dropdown_part_bounds(dropdown, true).unwrap();
    assert!(action.width > 0. && menu.width > 0.);
    app.pointer(
        Phase::Down,
        action.x + action.width / 2.,
        action.y + action.height / 2.,
    )
    .unwrap();
    app.pointer(
        Phase::Up,
        action.x + action.width / 2.,
        action.y + action.height / 2.,
    )
    .unwrap();
    app.frame(0.016).unwrap();
    assert_eq!(clicks.get(), 1);
    app.pointer(
        Phase::Down,
        menu.x + menu.width / 2.,
        menu.y + menu.height / 2.,
    )
    .unwrap();
    app.pointer(
        Phase::Up,
        menu.x + menu.width / 2.,
        menu.y + menu.height / 2.,
    )
    .unwrap();
    app.step(0.016).unwrap();
    let b = app.bounds(item).unwrap();
    app.pointer(Phase::Down, b.x + b.width / 2., b.y + b.height / 2.)
        .unwrap();
    app.pointer(Phase::Up, b.x + b.width / 2., b.y + b.height / 2.)
        .unwrap();
    app.frame(0.016).unwrap();
    assert_eq!(&*selections.borrow(), &[Some(item)]);
}
#[test]
fn surface_close_event_and_popover_anchor() {
    let _guard = NATIVE.lock().unwrap();
    let mut app = App::headless(500., 300.).unwrap();
    let root = app.root(Kind::Column, "root").unwrap();
    let trigger = app.child(root, Kind::Button, "trigger").unwrap();
    app.text(trigger, "Open").unwrap();
    let dialog = app.child(root, Kind::Dialog, "dialog").unwrap();
    app.text(dialog, "Title")
        .unwrap()
        .detail_text(dialog, "Details")
        .unwrap();
    let popover = app.child(root, Kind::Popover, "popover").unwrap();
    assert_eq!(app.open(popover, true).err(), Some(Error(8)));
    app.anchor(popover, trigger).unwrap();
    let results = Rc::new(std::cell::RefCell::new(Vec::new()));
    let captured = results.clone();
    app.on_close_event(dialog, move |e| {
        captured.borrow_mut().push(e.surface_result())
    })
    .unwrap();
    app.open(dialog, true).unwrap();
    app.step(0.016).unwrap();
    assert!(app.is_open(dialog).unwrap());
    app.open(dialog, false).unwrap();
    app.frame(0.016).unwrap();
    assert_eq!(&*results.borrow(), &[Some(SurfaceResult::Closed)]);
    app.open(popover, true).unwrap();
    app.step(0.016).unwrap();
    assert!(app.is_open(popover).unwrap());
}
#[test]
fn hover_surface_lifecycle() {
    let _guard = NATIVE.lock().unwrap();
    let mut app = App::headless(400., 200.).unwrap();
    let root = app.root(Kind::Column, "root").unwrap();
    let trigger = app.child(root, Kind::Button, "trigger").unwrap();
    app.text(trigger, "Hover").unwrap();
    let tip = app.child(root, Kind::Tooltip, "tip").unwrap();
    app.text(tip, "Help").unwrap().anchor(tip, trigger).unwrap();
    let card = app.child(root, Kind::HoverCard, "card").unwrap();
    app.text(card, "Preview")
        .unwrap()
        .anchor(card, trigger)
        .unwrap();
    let body = app.child(card, Kind::Label, "body").unwrap();
    app.text(body, "Rich content").unwrap();
    let transitions = Rc::new(std::cell::RefCell::new(Vec::new()));
    let captured = transitions.clone();
    app.on_change_event(tip, move |e| captured.borrow_mut().push(e.checked))
        .unwrap();
    app.step(0.016).unwrap();
    let b = app.bounds(trigger).unwrap();
    app.pointer(Phase::Move, b.x + b.width / 2., b.y + b.height / 2.)
        .unwrap();
    app.frame(0.016).unwrap();
    assert!(app.is_open(tip).unwrap());
    assert!(app.is_open(card).unwrap());
    app.pointer(Phase::Move, 399., 199.).unwrap();
    app.frame(0.016).unwrap();
    assert!(!app.is_open(tip).unwrap());
    assert!(!app.is_open(card).unwrap());
    assert_eq!(&*transitions.borrow(), &[true, false]);
}
#[test]
fn visual_and_status_components() {
    let _guard = NATIVE.lock().unwrap();
    let mut app = App::headless(600., 500.).unwrap();
    let root = app.root(Kind::Column, "root").unwrap();
    let visuals = [
        Kind::Avatar,
        Kind::Bubble,
        Kind::EmptyView,
        Kind::GroupBox,
        Kind::Kbd,
        Kind::Skeleton,
        Kind::Spinner,
        Kind::Tag,
        Kind::Marker,
        Kind::Shimmer,
    ];
    let mut nodes = Vec::new();
    for (index, kind) in visuals.into_iter().enumerate() {
        let n = app.child(root, kind, &format!("visual-{index}")).unwrap();
        app.text(n, "Sample").unwrap();
        nodes.push(n);
    }
    app.visual_size(nodes[0], 64., 64.).unwrap();
    app.visual_size(nodes[2], 100., 80.).unwrap();
    app.detail_text(nodes[2], "No results").unwrap();
    app.visual_size(nodes[3], 150., 100.).unwrap();
    let child = app.child(nodes[3], Kind::Label, "content").unwrap();
    app.text(child, "Inside").unwrap();
    assert_eq!(app.visual_size(nodes[1], 0., 1.).err(), Some(Error(8)));
    app.step(0.016).unwrap();
    assert!(app.bounds(nodes[0]).unwrap().width > 0.);
    assert!(app.bounds(child).unwrap().height > 0.);
}
#[test]
fn list_and_numeric_navigation() {
    let _guard = NATIVE.lock().unwrap();
    let mut app = App::headless(600., 400.).unwrap();
    let root = app.root(Kind::Column, "root").unwrap();
    let list = app.child(root, Kind::List, "list").unwrap();
    let first = app.child(list, Kind::Item, "first").unwrap();
    app.text(first, "First").unwrap();
    let second = app.child(list, Kind::Item, "second").unwrap();
    app.text(second, "Second").unwrap();
    app.select(list, second).unwrap();
    assert_eq!(app.selected(list).unwrap(), Some(second));
    let menu = app.child(root, Kind::Menu, "menu").unwrap();
    let action = app.child(menu, Kind::Item, "action").unwrap();
    app.text(action, "Action").unwrap();
    app.select(menu, action).unwrap();
    let stepper = app.child(root, Kind::Stepper, "steps").unwrap();
    let step = app.child(stepper, Kind::Item, "step").unwrap();
    app.text(step, "Step")
        .unwrap()
        .detail_text(step, "Description")
        .unwrap()
        .step_status(step, 2)
        .unwrap();
    app.select(stepper, step).unwrap();
    let pages = app.child(root, Kind::Pagination, "pages").unwrap();
    app.total(pages, 5).unwrap().value(pages, 2.).unwrap();
    let rating = app.child(root, Kind::Rating, "rating").unwrap();
    app.total(rating, 5).unwrap().value(rating, 3.).unwrap();
    assert_eq!(app.current_value(pages).unwrap(), 2.);
    assert_eq!(app.step_status(step, 4).err(), Some(Error(3)));
    app.step(0.016).unwrap();
    let events = Rc::new(std::cell::RefCell::new(Vec::new()));
    let captured = events.clone();
    app.on_change_event(pages, move |e| captured.borrow_mut().push(e.value))
        .unwrap();
    let b = app.numeric_part_bounds(pages, 3).unwrap();
    assert!(b.width > 0.);
    app.pointer(Phase::Down, b.x + b.width / 2., b.y + b.height / 2.)
        .unwrap();
    app.pointer(Phase::Up, b.x + b.width / 2., b.y + b.height / 2.)
        .unwrap();
    app.frame(0.016).unwrap();
    assert_eq!(app.current_value(pages).unwrap(), 3.);
    assert_eq!(&*events.borrow(), &[3.]);
}
#[test]
fn description_list_and_scroll() {
    let _guard = NATIVE.lock().unwrap();
    let mut app = App::headless(500., 300.).unwrap();
    let root = app.root(Kind::Column, "root").unwrap();
    let descriptions = app.child(root, Kind::DescriptionList, "details").unwrap();
    let entry = app.child(descriptions, Kind::Item, "entry").unwrap();
    app.text(entry, "Name")
        .unwrap()
        .detail_text(entry, "Alice")
        .unwrap();
    let scroll = app.child(root, Kind::Scrollable, "viewport").unwrap();
    app.visual_size(scroll, 200., 80.)
        .unwrap()
        .scroll_extent(scroll, 200., 500.)
        .unwrap();
    let label = app.child(scroll, Kind::Label, "content").unwrap();
    app.text(label, "Long content").unwrap();
    let offsets = Rc::new(std::cell::RefCell::new(Vec::new()));
    let captured = offsets.clone();
    app.on_change_event(scroll, move |e| captured.borrow_mut().push(e.value))
        .unwrap();
    app.step(0.016).unwrap();
    let b = app.bounds(scroll).unwrap();
    assert!(b.width > 0.);
    app.scroll(b.x + b.width / 2., b.y + b.height / 2., 0., -40.)
        .unwrap();
    app.frame(0.016).unwrap();
    assert!(app.current_value(scroll).unwrap() > 0.);
    assert_eq!(offsets.borrow().len(), 1);
}
#[test]
fn status_toolbar_and_title_bar() {
    let _guard = NATIVE.lock().unwrap();
    let mut app = App::headless(500., 300.).unwrap();
    let root = app.root(Kind::Column, "root").unwrap();
    let status = app.child(root, Kind::StatusBar, "status").unwrap();
    let left = app.child(status, Kind::Item, "left").unwrap();
    app.text(left, "Ready")
        .unwrap()
        .step_status(left, 0)
        .unwrap();
    let center = app.child(status, Kind::Item, "center").unwrap();
    app.text(center, "Line 1")
        .unwrap()
        .step_status(center, 1)
        .unwrap();
    let toolbar = app.child(root, Kind::Toolbar, "toolbar").unwrap();
    let action = app.child(toolbar, Kind::Item, "action").unwrap();
    app.text(action, "Run").unwrap();
    let title = app.child(root, Kind::TitleBar, "title").unwrap();
    let caption = app.child(title, Kind::Item, "caption").unwrap();
    app.text(caption, "Workspace").unwrap();
    app.step(0.016).unwrap();
    assert!(app.bounds(status).unwrap().width > 0.);
    assert!(app.bounds(toolbar).unwrap().height > 0.);
    assert!(app.bounds(title).unwrap().height > 0.);
    app.component_disabled(toolbar, true).unwrap();
    assert_eq!(app.step_status(left, 3).err(), Some(Error(3)));
}
#[test]
fn icon_and_message_rendering() {
    let _guard = NATIVE.lock().unwrap();
    let mut app = App::headless(400., 200.).unwrap();
    let root = app.root(Kind::Column, "root").unwrap();
    let icon = app.child(root, Kind::Icon, "check").unwrap();
    app.text(icon, "check")
        .unwrap()
        .visual_size(icon, 24., 24.)
        .unwrap();
    let message = app.child(root, Kind::Message, "message").unwrap();
    app.text(message, "Saved")
        .unwrap()
        .detail_text(message, "Status")
        .unwrap()
        .visual_size(message, 200., 60.)
        .unwrap();
    app.step(0.016).unwrap();
    assert!(app.bounds(icon).unwrap().width > 0.);
    assert!(app.bounds(message).unwrap().height > 0.);
    app.text(icon, "not-a-real-icon-name").unwrap();
    assert_eq!(app.step(0.016), Err(Error(8)));
}
#[test]
fn attachment_progress() {
    let _guard = NATIVE.lock().unwrap();
    let mut app = App::headless(400., 200.).unwrap();
    let root = app.root(Kind::Column, "root").unwrap();
    let attachment = app
        .build_child(root, Kind::Attachment, "file")
        .unwrap()
        .text("report.pdf")
        .unwrap()
        .detail_text("Uploading")
        .unwrap()
        .attachment_status(AttachmentStatus::Uploading)
        .unwrap()
        .value(0.5)
        .unwrap()
        .visual_size(240., 40.)
        .unwrap()
        .finish();
    assert_eq!(app.current_value(attachment).unwrap(), 0.5);
    app.step(0.016).unwrap();
    assert!(app.bounds(attachment).unwrap().width > 0.);
    assert_eq!(app.value(attachment, 1.5).err(), Some(Error(3)));
}
#[test]
fn owned_rgba_image() {
    let _guard = NATIVE.lock().unwrap();
    let mut app = App::headless(200., 200.).unwrap();
    let root = app.root(Kind::Column, "root").unwrap();
    let image = app.child(root, Kind::Image, "image").unwrap();
    let pixels = vec![
        255u8, 0, 0, 255, 0, 255, 0, 255, 0, 0, 255, 255, 255, 255, 255, 255,
    ];
    app.image_rgba(image, &pixels, 2, 2, 8)
        .unwrap()
        .text(image, "four pixels")
        .unwrap()
        .visual_size(image, 64., 64.)
        .unwrap();
    drop(pixels);
    app.step(0.016).unwrap();
    assert!(app.bounds(image).unwrap().width > 0.);
    assert_eq!(
        app.image_rgba(image, &[0u8; 3], 1, 1, 4).err(),
        Some(Error(3))
    );
}
#[test]
fn tree_selection_and_expansion() {
    let _guard = NATIVE.lock().unwrap();
    let mut app = App::headless(400., 300.).unwrap();
    let root = app.root(Kind::Column, "root").unwrap();
    let tree = app.child(root, Kind::Tree, "tree").unwrap();
    app.visual_size(tree, 250., 160.).unwrap();
    let parent = app.child(tree, Kind::Item, "parent").unwrap();
    app.text(parent, "Parent")
        .unwrap()
        .step_status(parent, 0)
        .unwrap()
        .checked(parent, true)
        .unwrap();
    let child = app.child(tree, Kind::Item, "child").unwrap();
    app.text(child, "Child")
        .unwrap()
        .step_status(child, 1)
        .unwrap();
    app.select(tree, child).unwrap();
    assert_eq!(app.selected(tree).unwrap(), Some(child));
    assert!(app.is_checked(parent).unwrap());
    app.step(0.016).unwrap();
    assert!(app.bounds(tree).unwrap().width > 0.);
    assert!(app.bounds(child).unwrap().height > 0.);
    assert_eq!(app.step_status(child, 64).err(), Some(Error(3)));
}
#[test]
fn time_field_value_and_validation() {
    let _guard = NATIVE.lock().unwrap();
    let mut app = App::headless(300., 100.).unwrap();
    let root = app.root(Kind::Column, "root").unwrap();
    let time = app
        .build_child(root, Kind::TimeField, "time")
        .unwrap()
        .time(Time {
            hour: 14,
            minute: 30,
            second: 5,
        })
        .unwrap()
        .finish();
    assert_eq!(
        app.current_time(time).unwrap(),
        Time {
            hour: 14,
            minute: 30,
            second: 5
        }
    );
    assert_eq!(
        app.time(
            time,
            Time {
                hour: 24,
                minute: 0,
                second: 0
            }
        )
        .err(),
        Some(Error(3))
    );
    app.step(0.016).unwrap();
    assert!(app.bounds(time).unwrap().width > 0.);
}
#[test]
fn color_picker_state_and_hit_targets() {
    let _guard = NATIVE.lock().unwrap();
    let mut app = App::headless(400., 300.).unwrap();
    let root = app.root(Kind::Column, "root").unwrap();
    let color = app
        .build_child(root, Kind::ColorPicker, "color")
        .unwrap()
        .color_rgba(0x336699FF)
        .unwrap()
        .visual_size(220., 160.)
        .unwrap()
        .finish();
    assert_eq!(app.current_color_rgba(color).unwrap(), 0x336699FF);
    app.step(0.016).unwrap();
    for part in 1..=3 {
        assert!(app.color_part_bounds(color, part).unwrap().width > 0.);
    }
    assert_eq!(app.color_part_bounds(color, 0).err(), Some(Error(3)));
    let payloads = Rc::new(std::cell::RefCell::new(Vec::new()));
    let captured = payloads.clone();
    app.on_change_event(color, move |e| {
        captured.borrow_mut().push((e.value, e.text.clone()))
    })
    .unwrap();
    let b = app.color_part_bounds(color, 1).unwrap();
    app.pointer(Phase::Down, b.x + b.width * 0.75, b.y + b.height / 2.)
        .unwrap();
    app.pointer(Phase::Up, b.x + b.width * 0.75, b.y + b.height / 2.)
        .unwrap();
    app.frame(0.016).unwrap();
    let rgba = app.current_color_rgba(color).unwrap();
    assert_ne!(rgba, 0x336699FF);
    assert_eq!(payloads.borrow().len(), 1);
    assert_eq!(payloads.borrow()[0].0, rgba as f64);
    assert_eq!(payloads.borrow()[0].1, format!("#{rgba:08X}"));
}
#[test]
fn calendar_and_date_picker() {
    let _guard = NATIVE.lock().unwrap();
    let mut app = App::headless(500., 350.).unwrap();
    let root = app.root(Kind::Column, "root").unwrap();
    let date = Date {
        year: 2026,
        month: 10,
        day: 14,
    };
    let calendar = app
        .build_child(root, Kind::Calendar, "calendar")
        .unwrap()
        .date(date)
        .unwrap()
        .finish();
    let picker = app
        .build_child(root, Kind::DatePicker, "picker")
        .unwrap()
        .date(date)
        .unwrap()
        .visual_size(180., 40.)
        .unwrap()
        .finish();
    assert_eq!(app.current_date(calendar).unwrap(), date);
    assert_eq!(app.current_date(picker).unwrap(), date);
    assert_eq!(
        app.date(
            calendar,
            Date {
                year: 2026,
                month: 2,
                day: 30
            }
        )
        .err(),
        Some(Error(3))
    );
    app.step(0.016).unwrap();
    assert!(app.calendar_day_bounds(calendar, date).unwrap().width > 0.);
    app.open(picker, true).unwrap();
    app.step(0.016).unwrap();
    assert!(app.is_open(picker).unwrap());
}
#[test]
fn otp_input_queued_text() {
    let _guard = NATIVE.lock().unwrap();
    let mut app = App::headless(400., 100.).unwrap();
    let root = app.root(Kind::Column, "root").unwrap();
    let otp = app
        .build_child(root, Kind::OtpInput, "otp")
        .unwrap()
        .total(4)
        .unwrap()
        .finish();
    let values = Rc::new(std::cell::RefCell::new(Vec::new()));
    let captured = values.clone();
    app.on_change_event(otp, move |e| captured.borrow_mut().push(e.text.clone()))
        .unwrap();
    app.step(0.016).unwrap();
    let b = app.bounds(otp).unwrap();
    app.pointer(Phase::Down, b.x + b.width / 2., b.y + b.height / 2.)
        .unwrap();
    app.pointer(Phase::Up, b.x + b.width / 2., b.y + b.height / 2.)
        .unwrap();
    app.step(0.016).unwrap();
    app.text(otp, "12").unwrap();
    app.queue_text("3").unwrap();
    app.step(0.016).unwrap();
    app.queue_text("4").unwrap();
    app.step(0.016).unwrap();
    assert_eq!(app.editable_text(otp).unwrap(), "1234");
    app.dispatch().unwrap();
    assert_eq!(&*values.borrow(), &["123", "1234"]);
    assert_eq!(app.total(otp, 0).err(), Some(Error(3)));
}
#[test]
fn form_field_and_inherited_disabled() {
    let _guard = NATIVE.lock().unwrap();
    let mut app = App::headless(400., 300.).unwrap();
    let root = app.root(Kind::Column, "root").unwrap();
    let form = app
        .build_child(root, Kind::Form, "form")
        .unwrap()
        .visual_size(300., 200.)
        .unwrap()
        .finish();
    let field = app
        .build_child(form, Kind::Item, "name-field")
        .unwrap()
        .text("Name")
        .unwrap()
        .detail_text("Your name")
        .unwrap()
        .finish();
    let input = app
        .build_child(field, Kind::Input, "name")
        .unwrap()
        .text("Alice")
        .unwrap()
        .finish();
    app.step(0.016).unwrap();
    assert!(app.bounds(input).unwrap().width > 0.);
    app.component_disabled(form, true).unwrap();
    app.step(0.016).unwrap();
    let b = app.bounds(input).unwrap();
    app.pointer(Phase::Down, b.x + b.width / 2., b.y + b.height / 2.)
        .unwrap();
    app.pointer(Phase::Up, b.x + b.width / 2., b.y + b.height / 2.)
        .unwrap();
    app.step(0.016).unwrap();
    app.queue_text("Z").unwrap();
    app.step(0.016).unwrap();
    assert_eq!(app.editable_text(input).unwrap(), "Alice");
}
#[test]
fn input_group_editor_and_addon() {
    let _guard = NATIVE.lock().unwrap();
    let mut app = App::headless(500., 200.).unwrap();
    let root = app.root(Kind::Column, "root").unwrap();
    let group = app
        .build_child(root, Kind::InputGroup, "group")
        .unwrap()
        .text("Alice")
        .unwrap()
        .detail_text("Name")
        .unwrap()
        .visual_size(300., 40.)
        .unwrap()
        .finish();
    let addon = app
        .build_child(group, Kind::Item, "clear")
        .unwrap()
        .text("Clear")
        .unwrap()
        .step_status(1)
        .unwrap()
        .finish();
    assert_eq!(app.editable_text(group).unwrap(), "Alice");
    let clicks = Rc::new(Cell::new(0));
    let c = clicks.clone();
    app.on_click(addon, move || c.set(c.get() + 1)).unwrap();
    app.step(0.016).unwrap();
    let b = app.bounds(addon).unwrap();
    app.pointer(Phase::Down, b.x + b.width / 2., b.y + b.height / 2.)
        .unwrap();
    app.pointer(Phase::Up, b.x + b.width / 2., b.y + b.height / 2.)
        .unwrap();
    app.frame(0.016).unwrap();
    assert_eq!(clicks.get(), 1);
    assert_eq!(app.step_status(addon, 4).err(), Some(Error(3)));
}
#[test]
fn retained_notification_lifecycle() {
    let _guard = NATIVE.lock().unwrap();
    let mut app = App::headless(400., 200.).unwrap();
    let root = app.root(Kind::Column, "root").unwrap();
    let notice = app
        .build_child(root, Kind::Notification, "notice")
        .unwrap()
        .text("Saved")
        .unwrap()
        .detail_text("Your changes are stored")
        .unwrap()
        .open(true)
        .unwrap()
        .finish();
    let results = Rc::new(std::cell::RefCell::new(Vec::new()));
    let captured = results.clone();
    app.on_close_event(notice, move |e| {
        captured.borrow_mut().push(e.surface_result())
    })
    .unwrap();
    app.step(0.016).unwrap();
    assert!(app.is_open(notice).unwrap());
    assert!(app.bounds(notice).unwrap().width > 0.);
    app.open(notice, false).unwrap();
    app.frame(0.3).unwrap();
    assert_eq!(&*results.borrow(), &[Some(SurfaceResult::Closed)]);
}
#[test]
fn focus_trap_blocks_outside_clicks() {
    let _guard = NATIVE.lock().unwrap();
    let mut app = App::headless(400., 250.).unwrap();
    let root = app.root(Kind::Column, "root").unwrap();
    let outside = app
        .build_child(root, Kind::Button, "outside")
        .unwrap()
        .text("Outside")
        .unwrap()
        .finish();
    let trap = app
        .build_child(root, Kind::FocusTrap, "trap")
        .unwrap()
        .open(true)
        .unwrap()
        .finish();
    let inside = app
        .build_child(trap, Kind::Button, "inside")
        .unwrap()
        .text("Inside")
        .unwrap()
        .finish();
    let outside_count = Rc::new(Cell::new(0));
    let captured = outside_count.clone();
    app.on_click(outside, move || captured.set(captured.get() + 1))
        .unwrap();
    let inside_count = Rc::new(Cell::new(0));
    let captured = inside_count.clone();
    app.on_click(inside, move || captured.set(captured.get() + 1))
        .unwrap();
    app.step(0.016).unwrap();
    let b = app.bounds(outside).unwrap();
    app.pointer(Phase::Down, b.x + b.width / 2., b.y + b.height / 2.)
        .unwrap();
    app.pointer(Phase::Up, b.x + b.width / 2., b.y + b.height / 2.)
        .unwrap();
    app.frame(0.016).unwrap();
    assert_eq!(outside_count.get(), 0);
    let b = app.bounds(inside).unwrap();
    app.pointer(Phase::Down, b.x + b.width / 2., b.y + b.height / 2.)
        .unwrap();
    app.pointer(Phase::Up, b.x + b.width / 2., b.y + b.height / 2.)
        .unwrap();
    app.frame(0.016).unwrap();
    assert_eq!(inside_count.get(), 1);
    app.open(trap, false).unwrap();
    app.step(0.016).unwrap();
    let b = app.bounds(outside).unwrap();
    app.pointer(Phase::Down, b.x + b.width / 2., b.y + b.height / 2.)
        .unwrap();
    app.pointer(Phase::Up, b.x + b.width / 2., b.y + b.height / 2.)
        .unwrap();
    app.frame(0.016).unwrap();
    assert_eq!(outside_count.get(), 1);
}
#[test]
fn font_fallback_validation() {
    let _guard = NATIVE.lock().unwrap();
    let mut app = App::headless(100., 100.).unwrap();
    assert_eq!(
        app.font_fallbacks(&["/tmp/fallback.ttf"]).err(),
        Some(Error(9))
    );
    assert_eq!(app.font_fallbacks(&["bad\0path.ttf"]).err(), Some(Error(3)));
}
