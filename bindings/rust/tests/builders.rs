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
#[test]
fn message_scroller_items_and_offset() {
    let _guard = NATIVE.lock().unwrap();
    let mut app = App::headless(500., 300.).unwrap();
    let root = app.root(Kind::Column, "root").unwrap();
    let scroller = app
        .build_child(root, Kind::MessageScroller, "chat")
        .unwrap()
        .visual_size(300., 100.)
        .unwrap()
        .finish();
    for i in 0..12 {
        let _item = app
            .build_child(scroller, Kind::Item, &format!("message-{i}"))
            .unwrap()
            .text(&format!("Message {i}"))
            .unwrap()
            .detail_text("Ada")
            .unwrap()
            .step_status((i % 2) as u32)
            .unwrap()
            .finish();
    }
    app.step(0.016).unwrap();
    assert!(app.bounds(scroller).unwrap().width > 0.);
    assert!(app.current_value(scroller).unwrap() > 0.);
    assert_eq!(app.value(scroller, -1.).err(), Some(Error(3)));
    app.value(scroller, 0.).unwrap();
    app.step(0.016).unwrap();
    assert_eq!(app.current_value(scroller).unwrap(), 0.);
}
#[test]
fn carousel_selection_and_navigation() {
    let _guard = NATIVE.lock().unwrap();
    let mut app = App::headless(400., 250.).unwrap();
    let root = app.root(Kind::Column, "root").unwrap();
    let carousel = app
        .build_child(root, Kind::Carousel, "carousel")
        .unwrap()
        .visual_size(300., 120.)
        .unwrap()
        .finish();
    let one = app
        .build_child(carousel, Kind::Item, "one")
        .unwrap()
        .text("First")
        .unwrap()
        .finish();
    let two = app
        .build_child(carousel, Kind::Item, "two")
        .unwrap()
        .text("Second")
        .unwrap()
        .finish();
    let panel = app
        .build_child(two, Kind::Label, "panel")
        .unwrap()
        .text("Slide content")
        .unwrap()
        .finish();
    app.step(0.016).unwrap();
    assert_eq!(app.selected(carousel).unwrap(), Some(one));
    app.select(carousel, two).unwrap();
    app.step(0.016).unwrap();
    assert_eq!(app.selected(carousel).unwrap(), Some(two));
    assert!(app.bounds(panel).unwrap().width > 0.);
}
#[test]
fn resizable_panel_drag() {
    let _guard = NATIVE.lock().unwrap();
    let mut app = App::headless(500., 300.).unwrap();
    let root = app.root(Kind::Column, "root").unwrap();
    let group = app
        .build_child(root, Kind::Resizable, "split")
        .unwrap()
        .visual_size(420., 160.)
        .unwrap()
        .finish();
    let first = app
        .build_child(group, Kind::Item, "first")
        .unwrap()
        .text("First")
        .unwrap()
        .value(240.)
        .unwrap()
        .finish();
    let _second = app
        .build_child(group, Kind::Item, "second")
        .unwrap()
        .text("Second")
        .unwrap()
        .finish();
    let values = Rc::new(std::cell::RefCell::new(Vec::new()));
    let captured = values.clone();
    app.on_change_event(first, move |e| captured.borrow_mut().push(e.value))
        .unwrap();
    app.step(0.016).unwrap();
    let before = app.current_value(first).unwrap();
    let h = app.resizable_handle_bounds(group, first).unwrap();
    let x = h.x + h.width / 2.;
    let y = h.y + h.height / 2.;
    app.pointer(Phase::Down, x, y).unwrap();
    app.step(0.016).unwrap();
    app.pointer(Phase::Move, x + 30., y).unwrap();
    app.step(0.016).unwrap();
    app.pointer(Phase::Up, x + 30., y).unwrap();
    app.frame(0.016).unwrap();
    assert!(app.current_value(first).unwrap() > before);
    assert!(values.borrow().iter().any(|v| *v > before));
    assert_eq!(app.value(first, 90.).err(), Some(Error(3)));
}
#[test]
fn text_view_formats_selection_and_pointer_snapshot() {
    let _guard = NATIVE.lock().unwrap();
    let mut app = App::headless(640., 480.).unwrap();
    let root = app.root(Kind::Column, "root").unwrap();
    let view = app
        .build_child(root, Kind::TextView, "document")
        .unwrap()
        .visual_size(320., 120.)
        .unwrap()
        .text("**Hello** world")
        .unwrap()
        .finish();
    app.step(0.016).unwrap();
    app.text_view_select(view, 0, 5).unwrap();
    assert_eq!(app.text_view_selection(view).unwrap(), "Hello");
    assert_eq!(app.text_view_select(view, 50, 51).err(), Some(Error(3)));
    app.text_view_format(view, TextViewFormat::Html).unwrap();
    app.text(view, "<b>HTML</b> text").unwrap();
    app.text_view_select(view, 0, 4).unwrap();
    assert_eq!(app.text_view_selection(view).unwrap(), "HTML");
    app.text_view_format(view, TextViewFormat::Plain).unwrap();
    app.text(view, "Plain text").unwrap();
    app.text_view_select(view, 0, 0).unwrap();
    let snapshots = Rc::new(std::cell::RefCell::new(Vec::new()));
    let captured = snapshots.clone();
    app.on_change_event(view, move |e| captured.borrow_mut().push(e.text.clone()))
        .unwrap();
    app.step(0.016).unwrap();
    let b = app.bounds(view).unwrap();
    let x = b.x + 10.;
    let y = b.y + 14.;
    app.pointer(Phase::Down, x, y).unwrap();
    app.pointer(Phase::Move, x + 50., y).unwrap();
    app.pointer(Phase::Up, x + 50., y).unwrap();
    app.frame(0.016).unwrap();
    assert!(!snapshots.borrow().is_empty());
    assert_eq!(
        snapshots.borrow().last().unwrap(),
        &app.text_view_selection(view).unwrap()
    );
    app.component_disabled(view, true).unwrap();
    app.step(0.016).unwrap();
    let count = snapshots.borrow().len();
    app.pointer(Phase::Down, x, y).unwrap();
    app.pointer(Phase::Move, x + 50., y).unwrap();
    app.pointer(Phase::Up, x + 50., y).unwrap();
    app.frame(0.016).unwrap();
    assert_eq!(snapshots.borrow().len(), count);
}
#[test]
fn editor_multiline_owned_text_and_disabled_input() {
    let _guard = NATIVE.lock().unwrap();
    let mut app = App::headless(640., 480.).unwrap();
    let root = app.root(Kind::Column, "root").unwrap();
    let editor = app
        .build_child(root, Kind::Editor, "source")
        .unwrap()
        .text("first\nsecond")
        .unwrap()
        .detail_text("c")
        .unwrap()
        .visual_size(300., 120.)
        .unwrap()
        .finish();
    let other = app
        .build_child(root, Kind::Editor, "other")
        .unwrap()
        .visual_size(300., 120.)
        .unwrap()
        .finish();
    let snapshots = Rc::new(std::cell::RefCell::new(Vec::new()));
    let captured = snapshots.clone();
    app.on_change_event(editor, move |e| captured.borrow_mut().push(e.text.clone()))
        .unwrap();
    app.step(0.016).unwrap();
    let b = app.bounds(editor).unwrap();
    app.pointer(Phase::Down, b.x + 12., b.y + 16.).unwrap();
    app.pointer(Phase::Up, b.x + 12., b.y + 16.).unwrap();
    app.frame(0.016).unwrap();
    app.queue_text("✓").unwrap();
    app.frame(0.016).unwrap();
    assert!(app.editable_text(editor).unwrap().contains('✓'));
    assert_eq!(app.editable_text(other).unwrap(), "");
    assert!(snapshots.borrow().iter().any(|s| s.contains('✓')));
    app.component_disabled(editor, true).unwrap();
    app.step(0.016).unwrap();
    let before = app.editable_text(editor).unwrap();
    let count = snapshots.borrow().len();
    app.queue_text("X").unwrap();
    app.frame(0.016).unwrap();
    assert_eq!(app.editable_text(editor).unwrap(), before);
    assert_eq!(snapshots.borrow().len(), count);
}
#[test]
fn chart_copies_caller_data_and_keeps_instances_separate() {
    let _guard = NATIVE.lock().unwrap();
    let mut app = App::headless(640., 480.).unwrap();
    let root = app.root(Kind::Column, "root").unwrap();
    let chart = app
        .build_child(root, Kind::Chart, "revenue")
        .unwrap()
        .visual_size(300., 180.)
        .unwrap()
        .chart_kind(ChartKind::Bar)
        .unwrap()
        .finish();
    let other = app.child(root, Kind::Chart, "cost").unwrap();
    let mut owned = String::from("North");
    app.chart_data(chart, &[(owned.as_str(), 10.), ("South", 20.)])
        .unwrap();
    owned.replace_range(.., "Changed");
    assert_eq!(app.chart_datum(chart, 0).unwrap(), ("North".into(), 10.));
    assert_eq!(app.chart_datum(chart, 1).unwrap(), ("South".into(), 20.));
    assert_eq!(app.chart_datum(other, 0).err(), Some(Error(3)));
    assert_eq!(
        app.chart_data(chart, &[("bad\0label", 1.)]).err(),
        Some(Error(3))
    );
    assert_eq!(
        app.chart_data(chart, &[("invalid", f64::NAN)]).err(),
        Some(Error(3))
    );
    app.step(0.016).unwrap();
    assert!(app.bounds(chart).unwrap().width > 0.);
}
#[test]
fn compositional_table_geometry_updates_and_removal() {
    let _guard = NATIVE.lock().unwrap();
    let mut app = App::headless(640., 480.).unwrap();
    let root = app.root(Kind::Column, "root").unwrap();
    let table = app
        .build_child(root, Kind::Table, "scores")
        .unwrap()
        .visual_size(400., 0.)
        .unwrap()
        .finish();
    assert_eq!(
        app.child(table, Kind::Button, "invalid").err(),
        Some(Error(8))
    );
    let header = app.child(table, Kind::Item, "header").unwrap();
    let heading_name = app
        .build_child(header, Kind::Item, "name")
        .unwrap()
        .text("Name")
        .unwrap()
        .finish();
    let _heading_score = app
        .build_child(header, Kind::Item, "score")
        .unwrap()
        .text("Score")
        .unwrap()
        .finish();
    let row = app.child(table, Kind::Item, "alice").unwrap();
    let _name = app
        .build_child(row, Kind::Item, "alice-name")
        .unwrap()
        .text("Alice")
        .unwrap()
        .finish();
    let score = app
        .build_child(row, Kind::Item, "alice-score")
        .unwrap()
        .text("95")
        .unwrap()
        .finish();
    app.step(0.016).unwrap();
    assert!(app.bounds(table).unwrap().width > 0.);
    assert!(app.bounds(row).unwrap().y > app.bounds(header).unwrap().y);
    assert!(app.bounds(score).unwrap().x > app.bounds(row).unwrap().x);
    app.text(score, "96").unwrap();
    app.step(0.016).unwrap();
    app.remove(row).unwrap();
    assert_eq!(app.bounds(score).err(), Some(Error(4)));
    assert!(app.bounds(heading_name).is_ok());
}
#[test]
fn data_table_owned_cells_selection_and_sort_request() {
    let _guard = NATIVE.lock().unwrap();
    let mut app = App::headless(640., 480.).unwrap();
    let root = app.root(Kind::Column, "root").unwrap();
    let table = app
        .build_child(root, Kind::DataTable, "results")
        .unwrap()
        .visual_size(320., 150.)
        .unwrap()
        .finish();
    let mut name = String::from("Alice");
    app.data_table(
        table,
        &["Name", "Score"],
        &[&[name.as_str(), "95"], &["Bob", "87"]],
    )
    .unwrap();
    name.replace_range(.., "Changed");
    assert_eq!(app.data_table_cell(table, 0, 0).unwrap(), "Alice");
    assert_eq!(app.data_table_cell(table, 1, 0).unwrap(), "Bob");
    assert_eq!(app.data_table_cell(table, 2, 0).err(), Some(Error(3)));
    assert_eq!(
        app.data_table(table, &["Name", "Score"], &[&["short"]])
            .err(),
        Some(Error(3))
    );
    assert_eq!(app.data_table_cell(table, 1, 0).unwrap(), "Bob");
    let values = Rc::new(std::cell::RefCell::new(Vec::new()));
    let captured = values.clone();
    app.on_change_event(table, move |e| captured.borrow_mut().push(e.clone()))
        .unwrap();
    app.step(0.016).unwrap();
    let cell = app.data_table_cell_bounds(table, 1, 0).unwrap();
    let header = app.data_table_header_bounds(table, 0).unwrap();
    assert!(cell.y > header.y);
    let x = cell.x + cell.width / 2.;
    let y = cell.y + cell.height / 2.;
    app.pointer(Phase::Down, x, y).unwrap();
    app.pointer(Phase::Up, x, y).unwrap();
    app.frame(0.016).unwrap();
    assert_eq!(app.data_table_state(table).unwrap().row, 2);
    assert!(values.borrow().iter().any(|e| e.value == 2.
        && e.text.contains("\"reason\":\"selection\"")
        && e.text.contains("\"row\":2")));
    let x = header.x + header.width / 2.;
    let y = header.y + header.height / 2.;
    app.pointer(Phase::Down, x, y).unwrap();
    app.pointer(Phase::Up, x, y).unwrap();
    app.frame(0.016).unwrap();
    let sort = app.data_table_state(table).unwrap();
    assert_eq!(sort.sort_column, 1);
    assert_ne!(sort.sort_direction, 0);
    assert!(values
        .borrow()
        .iter()
        .any(|e| e.text.contains("\"reason\":\"sort\"") && e.text.contains("\"sortColumn\":1")));
    app.data_table_selection_mode(table, DataTableSelectionMode::Cell)
        .unwrap();
    app.step(0.016).unwrap();
    let second_column = app.data_table_cell_bounds(table, 0, 1).unwrap();
    let cx = second_column.x + second_column.width / 2.;
    let cy = second_column.y + second_column.height / 2.;
    app.pointer(Phase::Down, cx, cy).unwrap();
    app.pointer(Phase::Up, cx, cy).unwrap();
    app.frame(0.016).unwrap();
    assert_eq!(app.data_table_state(table).unwrap().column, 2);
    assert!(values
        .borrow()
        .iter()
        .any(|e| e.text.contains("\"column\":2")));
    app.component_disabled(table, true).unwrap();
    app.step(0.016).unwrap();
    app.pointer(Phase::Down, x, y).unwrap();
    app.pointer(Phase::Up, x, y).unwrap();
    app.frame(0.016).unwrap();
    assert_eq!(
        app.data_table_state(table).unwrap().sort_direction,
        sort.sort_direction
    );
}
#[test]
fn plot_copies_points_and_preserves_model_on_invalid_update() {
    let _guard = NATIVE.lock().unwrap();
    let mut app = App::headless(640., 480.).unwrap();
    let root = app.root(Kind::Column, "root").unwrap();
    let plot = app
        .build_child(root, Kind::Plot, "samples")
        .unwrap()
        .visual_size(240., 120.)
        .unwrap()
        .finish();
    let mut points = [PlotPoint { x: 20., y: 20. }, PlotPoint { x: 60., y: 60. }];
    app.plot_points(plot, &points).unwrap();
    points[0].x = 999.;
    assert_eq!(points[0].x, 999.);
    assert_eq!(
        app.plot_point(plot, 0).unwrap(),
        PlotPoint { x: 20., y: 20. }
    );
    assert_eq!(
        app.plot_points(
            plot,
            &[PlotPoint {
                x: f64::INFINITY,
                y: 1.
            }]
        )
        .err(),
        Some(Error(3))
    );
    assert_eq!(app.plot_point(plot, 1).unwrap().x, 60.);
    app.step(0.016).unwrap();
    assert!(app.bounds(plot).unwrap().width > 0.);
    app.remove(plot).unwrap();
    assert_eq!(app.plot_point(plot, 0).err(), Some(Error(4)));
}
#[test]
fn command_palette_filter_select_and_confirm() {
    let _guard = NATIVE.lock().unwrap();
    let mut app = App::headless(640., 480.).unwrap();
    let root = app.root(Kind::Column, "root").unwrap();
    let command = app
        .build_child(root, Kind::Command, "actions")
        .unwrap()
        .visual_size(300., 150.)
        .unwrap()
        .finish();
    assert_eq!(
        app.child(command, Kind::Button, "invalid").err(),
        Some(Error(8))
    );
    let open = app
        .build_child(command, Kind::Item, "open")
        .unwrap()
        .text("Open")
        .unwrap()
        .finish();
    let close = app
        .build_child(command, Kind::Item, "close")
        .unwrap()
        .text("Close")
        .unwrap()
        .detail_text("exit")
        .unwrap()
        .finish();
    let clicks = Rc::new(Cell::new(0));
    let captured = clicks.clone();
    app.on_click(close, move || captured.set(captured.get() + 1))
        .unwrap();
    app.step(0.016).unwrap();
    assert_eq!(app.selected(command).unwrap(), Some(open));
    let b = app.command_item_bounds(command, close).unwrap();
    let x = b.x + b.width / 2.;
    let y = b.y + b.height / 2.;
    app.pointer(Phase::Down, x, y).unwrap();
    app.pointer(Phase::Up, x, y).unwrap();
    app.frame(0.016).unwrap();
    assert_eq!(app.selected(command).unwrap(), Some(close));
    assert_eq!(clicks.get(), 1);
    app.text(command, "exit").unwrap();
    app.step(0.016).unwrap();
    assert_eq!(app.editable_text(command).unwrap(), "exit");
    assert_eq!(app.selected(command).unwrap(), Some(close));
    app.component_disabled(close, true).unwrap();
    app.step(0.016).unwrap();
    assert_eq!(app.selected(command).unwrap(), None);
    app.remove(command).unwrap();
    assert_eq!(
        app.command_item_bounds(command, close).err(),
        Some(Error(4))
    );
}
#[test]
fn virtual_list_wheel_scroll_and_visible_item_bounds() {
    let _guard = NATIVE.lock().unwrap();
    let mut app = App::headless(640., 480.).unwrap();
    let root = app.root(Kind::Column, "root").unwrap();
    let list = app
        .build_child(root, Kind::VirtualList, "items")
        .unwrap()
        .visual_size(200., 96.)
        .unwrap()
        .finish();
    let mut items = Vec::new();
    for i in 0..20 {
        let key = format!("item-{i}");
        items.push(
            app.build_child(list, Kind::Item, &key)
                .unwrap()
                .text(&key)
                .unwrap()
                .finish(),
        );
    }
    let offsets = Rc::new(std::cell::RefCell::new(Vec::new()));
    let captured = offsets.clone();
    app.on_change_event(list, move |e| captured.borrow_mut().push(e.value))
        .unwrap();
    app.step(0.016).unwrap();
    assert!(app.virtual_list_item_bounds(list, items[0]).is_ok());
    assert_eq!(
        app.virtual_list_item_bounds(list, items[19]).err(),
        Some(Error(8))
    );
    let b = app.bounds(list).unwrap();
    app.scroll(b.x + 20., b.y + 20., 0., -1.).unwrap();
    app.frame(0.016).unwrap();
    let offset = app.current_value(list).unwrap();
    assert!(offset > 0.);
    assert!(offsets.borrow().iter().any(|v| *v > 0.));
    app.value(list, 500.).unwrap();
    app.step(0.016).unwrap();
    assert!(app.virtual_list_item_bounds(list, items[19]).is_ok());
    app.component_disabled(list, true).unwrap();
    app.step(0.016).unwrap();
    let disabled_offset = app.current_value(list).unwrap();
    app.scroll(b.x + 20., b.y + 20., 0., 1.).unwrap();
    app.step(0.016).unwrap();
    assert_eq!(app.current_value(list).unwrap(), disabled_offset);
}
#[test]
fn sidebar_navigation_and_open_state() {
    let _guard = NATIVE.lock().unwrap();
    let mut app = App::headless(640., 480.).unwrap();
    let root = app.root(Kind::Column, "root").unwrap();
    let sidebar = app
        .build_child(root, Kind::Sidebar, "navigation")
        .unwrap()
        .visual_size(240., 180.)
        .unwrap()
        .finish();
    assert_eq!(
        app.child(sidebar, Kind::Button, "invalid").err(),
        Some(Error(8))
    );
    let home = app
        .build_child(sidebar, Kind::Item, "home")
        .unwrap()
        .text("Home")
        .unwrap()
        .finish();
    let settings = app
        .build_child(sidebar, Kind::Item, "settings")
        .unwrap()
        .text("Settings")
        .unwrap()
        .finish();
    let changes = Rc::new(std::cell::RefCell::new(Vec::new()));
    let captured = changes.clone();
    app.on_change_event(sidebar, move |e| captured.borrow_mut().push(e.selected))
        .unwrap();
    let clicks = Rc::new(Cell::new(0));
    let captured = clicks.clone();
    app.on_click(settings, move || captured.set(captured.get() + 1))
        .unwrap();
    app.step(0.016).unwrap();
    let b = app.bounds(settings).unwrap();
    let x = b.x + b.width / 2.;
    let y = b.y + b.height / 2.;
    app.pointer(Phase::Down, x, y).unwrap();
    app.pointer(Phase::Up, x, y).unwrap();
    app.frame(0.016).unwrap();
    assert_eq!(app.selected(sidebar).unwrap(), Some(settings));
    assert!(changes.borrow().contains(&Some(settings)));
    assert_eq!(clicks.get(), 1);
    app.open(sidebar, false).unwrap();
    app.step(0.016).unwrap();
    assert!(!app.is_open(sidebar).unwrap());
    app.open(sidebar, true).unwrap();
    app.component_disabled(home, true).unwrap();
    app.step(0.016).unwrap();
    let b = app.bounds(home).unwrap();
    app.pointer(Phase::Down, b.x + b.width / 2., b.y + b.height / 2.)
        .unwrap();
    app.pointer(Phase::Up, b.x + b.width / 2., b.y + b.height / 2.)
        .unwrap();
    app.frame(0.016).unwrap();
    assert_eq!(app.selected(sidebar).unwrap(), Some(settings));
    app.remove(sidebar).unwrap();
    assert_eq!(app.is_open(sidebar).err(), Some(Error(4)));
}
#[test]
fn theme_presets_are_named_and_app_scoped() {
    let _guard = NATIVE.lock().unwrap();
    let names = App::theme_presets().unwrap();
    assert!(names.len() >= 3);
    assert!(names.iter().all(|name| !name.is_empty()));
    let mut first = App::headless(320., 240.).unwrap();
    let mut second = App::headless(320., 240.).unwrap();
    assert_eq!(first.theme_preset().unwrap(), None);
    assert_eq!(
        first.set_theme_preset(names.len() as u32).err(),
        Some(Error(3))
    );
    first.set_theme_preset(0).unwrap();
    second.set_theme_preset(1).unwrap();
    assert_eq!(first.theme_preset().unwrap(), Some(0));
    assert_eq!(second.theme_preset().unwrap(), Some(1));
    let root = first.root(Kind::Column, "root").unwrap();
    first
        .build_child(root, Kind::Button, "button")
        .unwrap()
        .text("Action")
        .unwrap();
    first.step(0.016).unwrap();
    first.set_theme_preset(1).unwrap();
    first.step(0.016).unwrap();
    assert_eq!(first.theme_preset().unwrap(), Some(1));
}
#[test]
fn root_view_padding_and_child_click() {
    let _guard = NATIVE.lock().unwrap();
    let mut app = App::headless(320., 240.).unwrap();
    let root = app
        .build_root(Kind::RootView, "root")
        .unwrap()
        .root_padding(20.)
        .unwrap()
        .finish();
    assert_eq!(
        app.child(root, Kind::RootView, "nested").err(),
        Some(Error(8))
    );
    let clicks = Rc::new(Cell::new(0));
    let captured = clicks.clone();
    let button = app
        .build_child(root, Kind::Button, "apply")
        .unwrap()
        .text("Apply")
        .unwrap()
        .on_click(move || captured.set(captured.get() + 1))
        .unwrap()
        .finish();
    assert_eq!(app.root_padding(button, 5.).err(), Some(Error(8)));
    assert_eq!(app.root_padding(root, -1.).err(), Some(Error(3)));
    app.step(0.016).unwrap();
    let rb = app.bounds(root).unwrap();
    assert_eq!((rb.width, rb.height), (320., 240.));
    let b = app.bounds(button).unwrap();
    assert!(b.x >= 20.);
    app.pointer(Phase::Down, b.x + b.width / 2., b.y + b.height / 2.)
        .unwrap();
    app.pointer(Phase::Up, b.x + b.width / 2., b.y + b.height / 2.)
        .unwrap();
    app.frame(0.016).unwrap();
    assert_eq!(clicks.get(), 1);
    app.root_padding(root, 40.).unwrap();
    app.step(0.016).unwrap();
    assert!(app.bounds(button).unwrap().x >= 40.);
}
#[test]
fn clipboard_headless_and_input_validation() {
    let _guard = NATIVE.lock().unwrap();
    let mut app = App::headless(100., 100.).unwrap();
    assert_eq!(app.clipboard_set("copy ✓").err(), Some(Error(9)));
    assert_eq!(app.clipboard_get().err(), Some(Error(9)));
    assert_eq!(app.clipboard_set("bad\0text").err(), Some(Error(3)));
    assert_eq!(app.clipboard_set(&"a".repeat(65537)).err(), Some(Error(3)));
}
#[test]
fn speech_host_report_lifecycle_and_copied_events() {
    let _guard = NATIVE.lock().unwrap();
    let mut app = App::headless(320., 240.).unwrap();
    let root = app.root(Kind::Column, "root").unwrap();
    let speech = app.child(root, Kind::Speech, "speech").unwrap();
    let events = Rc::new(std::cell::RefCell::new(Vec::new()));
    let captured = events.clone();
    app.on_change_event(speech, move |e| {
        captured
            .borrow_mut()
            .push((e.value, e.checked, e.text.clone()))
    })
    .unwrap();
    app.step(0.016).unwrap();
    assert!(app.bounds(speech).unwrap().width > 0.);
    assert_eq!(
        app.speech_report(speech, "early", true).err(),
        Some(Error(8))
    );
    app.speech_start(speech).unwrap();
    assert_eq!(app.speech_start(speech).err(), Some(Error(8)));
    app.speech_report(speech, "hel", false).unwrap();
    app.step(0.016).unwrap();
    app.speech_report(speech, "hello", true).unwrap();
    app.frame(0.016).unwrap();
    assert_eq!(app.speech_transcript(speech).unwrap(), "hello");
    assert!(app.is_checked(speech).unwrap());
    assert!(events
        .borrow()
        .iter()
        .any(|(v, _, text)| *v == 1. && text == "hel"));
    assert!(events
        .borrow()
        .iter()
        .any(|(v, _, text)| *v == 1. && text == "hello"));
    app.speech_finish(speech).unwrap();
    app.frame(0.016).unwrap();
    assert!(events
        .borrow()
        .iter()
        .any(|(v, checked, text)| *v == 2. && !checked && text == "hello"));
    app.component_disabled(speech, true).unwrap();
    assert_eq!(app.speech_start(speech).err(), Some(Error(8)));
    app.remove(speech).unwrap();
    assert_eq!(app.speech_start(speech).err(), Some(Error(4)));
}
#[test]
fn questionnaire_choices_validation_navigation_and_snapshots() {
    let _guard = NATIVE.lock().unwrap();
    let mut app = App::headless(480., 400.).unwrap();
    let root = app.root(Kind::Column, "root").unwrap();
    let form = app.child(root, Kind::Questionnaire, "survey").unwrap();
    let color = app
        .build_child(form, Kind::Item, "color")
        .unwrap()
        .text("Choose a color")
        .unwrap()
        .questionnaire_question(true, false)
        .unwrap()
        .finish();
    let red = app
        .build_child(color, Kind::Item, "red")
        .unwrap()
        .text("Red")
        .unwrap()
        .finish();
    let blue = app
        .build_child(color, Kind::Item, "blue")
        .unwrap()
        .text("Blue")
        .unwrap()
        .finish();
    let shape = app
        .build_child(form, Kind::Item, "shape")
        .unwrap()
        .text("Shape")
        .unwrap()
        .questionnaire_question(false, false)
        .unwrap()
        .finish();
    let round = app
        .build_child(shape, Kind::Item, "round")
        .unwrap()
        .text("Round")
        .unwrap()
        .finish();
    let snapshots = Rc::new(std::cell::RefCell::new(Vec::new()));
    let captured = snapshots.clone();
    app.on_change_event(form, move |e| captured.borrow_mut().push(e.text.clone()))
        .unwrap();
    app.step(0.016).unwrap();
    assert_eq!(app.questionnaire_state(form).unwrap().current, Some(color));
    assert_eq!(app.questionnaire_submit(form).err(), Some(Error(8)));
    assert_eq!(app.questionnaire_next(form).err(), Some(Error(8)));
    assert_eq!(app.text(red, "Changed").err(), Some(Error(8)));
    for choice in [red, blue] {
        let b = app.bounds(choice).unwrap();
        app.pointer(Phase::Down, b.x + b.width / 2., b.y + b.height / 2.)
            .unwrap();
        app.pointer(Phase::Up, b.x + b.width / 2., b.y + b.height / 2.)
            .unwrap();
        app.frame(0.016).unwrap();
    }
    assert!(!app.questionnaire_choice_selected(red).unwrap());
    assert!(app.questionnaire_choice_selected(blue).unwrap());
    assert!(snapshots
        .borrow()
        .iter()
        .any(|s| s.contains("\"selected\":[1]")));
    assert!(snapshots
        .borrow()
        .iter()
        .any(|s| s.contains("\"selected\":[2]")));
    app.questionnaire_next(form).unwrap();
    assert_eq!(app.questionnaire_state(form).unwrap().current, Some(shape));
    app.step(0.016).unwrap();
    let b = app.bounds(round).unwrap();
    app.pointer(Phase::Down, b.x + b.width / 2., b.y + b.height / 2.)
        .unwrap();
    app.pointer(Phase::Up, b.x + b.width / 2., b.y + b.height / 2.)
        .unwrap();
    app.frame(0.016).unwrap();
    app.questionnaire_previous(form).unwrap();
    assert_eq!(app.questionnaire_state(form).unwrap().current, Some(color));
    app.questionnaire_submit(form).unwrap();
    app.questionnaire_submit(form).unwrap();
    app.dispatch().unwrap();
    assert!(app.questionnaire_state(form).unwrap().complete);
    assert_eq!(
        snapshots
            .borrow()
            .iter()
            .filter(|s| s.contains("\"event\":3"))
            .count(),
        2
    );
    app.remove(form).unwrap();
    assert_eq!(app.questionnaire_state(form).err(), Some(Error(4)));
}

#[test]
fn dock_split_activation_serialization_and_frozen_panels() {
    let _guard = NATIVE.lock().unwrap();
    let mut app = App::headless(640., 480.).unwrap();
    let root = app.root(Kind::Column, "root").unwrap();
    let dock = app
        .build_child(root, Kind::Dock, "dock")
        .unwrap()
        .visual_size(480., 300.)
        .unwrap()
        .finish();
    let first = app
        .build_child(dock, Kind::Item, "one")
        .unwrap()
        .text("First")
        .unwrap()
        .finish();
    let second = app
        .build_child(dock, Kind::Item, "two")
        .unwrap()
        .text("Second")
        .unwrap()
        .finish();
    app.build_child(first, Kind::Label, "content")
        .unwrap()
        .text("Panel body")
        .unwrap();
    let selected = Rc::new(std::cell::RefCell::new(Vec::new()));
    let captured = selected.clone();
    app.on_change_event(dock, move |e| captured.borrow_mut().push(e.selected))
        .unwrap();
    app.step(0.016).unwrap();
    assert_eq!(app.dock_active(dock).unwrap(), Some(first));
    assert!(app.dock_panel_bounds(dock, second).unwrap().width > 0.);
    assert_eq!(app.text(second, "Later").err(), Some(Error(8)));
    assert_eq!(app.remove(second).err(), Some(Error(8)));
    let before = app.dock_serialize(dock).unwrap();
    app.dock_move_split(dock, second, first, DockAxis::Horizontal, 0.4, false)
        .unwrap();
    assert_ne!(app.dock_serialize(dock).unwrap(), before);
    app.step(0.016).unwrap();
    app.dock_activate(dock, second).unwrap();
    app.dispatch().unwrap();
    assert_eq!(app.dock_active(dock).unwrap(), Some(second));
    assert!(selected.borrow().contains(&Some(second)));
    app.component_disabled(dock, true).unwrap();
    assert_eq!(app.dock_activate(dock, first).err(), Some(Error(8)));
    app.remove(dock).unwrap();
    assert_eq!(app.dock_active(dock).err(), Some(Error(4)));
}
#[test]
fn settings_pages_fields_filter_and_reset() {
    let _guard = NATIVE.lock().unwrap();
    let mut app = App::headless(900., 650.).unwrap();
    let root = app.root(Kind::Column, "root").unwrap();
    let settings = app
        .build_child(root, Kind::Settings, "settings")
        .unwrap()
        .visual_size(720., 500.)
        .unwrap()
        .finish();
    let general = app
        .build_child(settings, Kind::Item, "general")
        .unwrap()
        .text("General")
        .unwrap()
        .finish();
    let group = app
        .build_child(general, Kind::Item, "prefs")
        .unwrap()
        .text("Preferences")
        .unwrap()
        .finish();
    let toggle = app
        .build_child(group, Kind::Switch, "enable")
        .unwrap()
        .text("Enable")
        .unwrap()
        .finish();
    let checkbox = app
        .build_child(group, Kind::Checkbox, "confirm")
        .unwrap()
        .text("Confirm")
        .unwrap()
        .finish();
    let input = app
        .build_child(group, Kind::Input, "name")
        .unwrap()
        .detail_text("Name")
        .unwrap()
        .finish();
    let number = app
        .build_child(group, Kind::NumberInput, "count")
        .unwrap()
        .detail_text("Count")
        .unwrap()
        .finish();
    let advanced = app
        .build_child(settings, Kind::Item, "advanced")
        .unwrap()
        .text("Advanced")
        .unwrap()
        .finish();
    app.child(advanced, Kind::Item, "other").unwrap();
    let pages = Rc::new(std::cell::RefCell::new(Vec::new()));
    let captured = pages.clone();
    app.on_change_event(settings, move |e| captured.borrow_mut().push(e.selected))
        .unwrap();
    let inputs = Rc::new(std::cell::RefCell::new(Vec::new()));
    let captured = inputs.clone();
    app.on_change_event(input, move |e| captured.borrow_mut().push(e.text.clone()))
        .unwrap();
    app.step(0.016).unwrap();
    assert_eq!(app.settings_page(settings).unwrap(), Some(general));
    assert_eq!(app.text(general, "Changed").err(), Some(Error(8)));
    let b = app.settings_field_bounds(settings, toggle).unwrap();
    app.pointer(Phase::Down, b.x + b.width / 2., b.y + b.height / 2.)
        .unwrap();
    app.pointer(Phase::Up, b.x + b.width / 2., b.y + b.height / 2.)
        .unwrap();
    app.frame(0.016).unwrap();
    assert!(app.is_checked(toggle).unwrap());
    let b = app.settings_field_bounds(settings, input).unwrap();
    app.pointer(Phase::Down, b.x + b.width / 2., b.y + b.height / 2.)
        .unwrap();
    app.pointer(Phase::Up, b.x + b.width / 2., b.y + b.height / 2.)
        .unwrap();
    app.frame(0.016).unwrap();
    app.queue_text("Zoë").unwrap();
    app.frame(0.016).unwrap();
    assert_eq!(app.editable_text(input).unwrap(), "Zoë");
    assert!(inputs.borrow().contains(&"Zoë".into()));
    app.component_disabled(checkbox, true).unwrap();
    app.step(0.016).unwrap();
    let b = app.settings_field_bounds(settings, checkbox).unwrap();
    app.pointer(Phase::Down, b.x + b.width / 2., b.y + b.height / 2.)
        .unwrap();
    app.pointer(Phase::Up, b.x + b.width / 2., b.y + b.height / 2.)
        .unwrap();
    app.frame(0.016).unwrap();
    assert!(!app.is_checked(checkbox).unwrap());
    app.settings_set_page(settings, advanced).unwrap();
    assert_eq!(app.settings_page(settings).unwrap(), Some(advanced));
    app.settings_set_page(settings, general).unwrap();
    app.dispatch().unwrap();
    assert!(pages.borrow().contains(&Some(advanced)));
    assert!(pages.borrow().contains(&Some(general)));
    app.settings_reset_page(settings, general).unwrap();
    app.dispatch().unwrap();
    assert!(!app.is_checked(toggle).unwrap());
    assert_eq!(app.editable_text(input).unwrap(), "");
    app.value(number, 7.).unwrap();
    app.step(0.016).unwrap();
    app.settings_reset_page(settings, general).unwrap();
    assert_eq!(app.current_value(number).unwrap(), 0.);
    assert_eq!(
        app.settings_query(settings, &"x".repeat(257)).err(),
        Some(Error(5))
    );
    app.settings_query(settings, "Advanced").unwrap();
    app.step(0.016).unwrap();
    assert_eq!(
        app.settings_field_bounds(settings, toggle).err(),
        Some(Error(8))
    );
    app.remove(settings).unwrap();
    assert_eq!(app.settings_page(settings).err(), Some(Error(4)));
}
#[test]
fn avatar_style() {
    let _guard = NATIVE.lock().unwrap();
    let mut app = App::headless(600., 400.).unwrap();
    let root = app.root(Kind::Column, "root").unwrap();
    let person = app.build_child(root, Kind::Avatar, "person").unwrap()
        .text("Ada Lovelace").unwrap().design_size(DesignSize::Md).unwrap()
        .identity_colors(true, Some(4)).unwrap().anonymous_icon(true, "user").unwrap().finish();
    let legacy = app.child(root, Kind::Avatar, "legacy").unwrap();
    app.visual_size(legacy, 60., 60.).unwrap();
    app.step(0.016).unwrap();
    assert_eq!(app.bounds(person).unwrap().width, 48.);
    assert_eq!(app.bounds(legacy).unwrap().width, 60.);
    app.design_size(person, DesignSize::Lg).unwrap().identity_colors(person, false, None).unwrap();
    app.step(0.016).unwrap();
    assert_eq!(app.bounds(person).unwrap().width, 80.);
    app.anonymous_icon(person, false, "").unwrap().design_size(person, DesignSize::Inherit).unwrap();
    app.step(0.016).unwrap();
    assert_eq!(app.bounds(person).unwrap().width, 40.);
    assert_eq!(app.identity_colors(person, true, Some(12)).err(), Some(Error(3)));
    assert_eq!(app.anonymous_icon(person, true, "bad\0").err(), Some(Error(3)));
    assert_eq!(app.anonymous_icon(person, true, &"x".repeat(4097)).err(), Some(Error(3)));
    assert!(app.design_size(root, DesignSize::Md).is_err());
    let button = app.child(root, Kind::Button, "button").unwrap();
    assert!(app.identity_colors(button, true, None).is_err());
    assert!(app.size(person, Size::Md).is_err());
    app.design_size(button, DesignSize::Lg).unwrap().size(button, Size::Sm).unwrap();
    app.remove(person).unwrap();
    assert!(app.design_size(person, DesignSize::Md).is_err());
}
