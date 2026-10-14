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
