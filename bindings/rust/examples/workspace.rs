use cui_builders::*;
use std::{cell::Cell, env, rc::Rc};

fn click(app: &mut App, node: Node) -> Result<()> {
    let r = app.bounds(node)?;
    let x = r.x + r.width / 2.;
    let y = r.y + r.height / 2.;
    app.pointer(Phase::Down, x, y)?;
    app.pointer(Phase::Up, x, y)?;
    app.frame(1. / 60.)?;
    Ok(())
}

struct Workspace {
    app: App,
    account: Node,
    preferences: Node,
    name: Node,
    birthday: Node,
    notifications: Node,
    save: Node,
    dialog: Node,
    status: Node,
    save_requested: Rc<Cell<bool>>,
}
impl Workspace {
    fn new(mut app: App) -> Result<Self> {
        let root = app
            .build_root(Kind::Column, "workspace")?
            .gap_units(2.)?
            .finish();
        let tabs = app.build_child(root, Kind::Tabs, "navigation")?.finish();
        let account = app
            .build_child(tabs, Kind::Item, "account")?
            .text("Account")?
            .finish();
        let name = app
            .build_child(account, Kind::Input, "name")?
            .text("Ada")?
            .finish();
        let birthday = app
            .build_child(account, Kind::DatePicker, "birthday")?
            .date(Date {
                year: 1990,
                month: 5,
                day: 15,
            })?
            .finish();
        let preferences = app
            .build_child(tabs, Kind::Item, "preferences")?
            .text("Preferences")?
            .finish();
        let notifications = app
            .build_child(preferences, Kind::Checkbox, "notifications")?
            .text("Notifications")?
            .finish();
        app.select(tabs, account)?;
        let save_requested = Rc::new(Cell::new(false));
        let requested = Rc::clone(&save_requested);
        let save = app
            .build_child(root, Kind::Button, "save")?
            .text("Save changes")?
            .variant(Variant::Primary)?
            .on_click(move || requested.set(true))?
            .finish();
        let status = app
            .build_child(root, Kind::Label, "status")?
            .text("Unsaved changes")?
            .finish();
        let dialog = app
            .build_child(root, Kind::Dialog, "saved-dialog")?
            .text("Changes saved")?
            .detail_text("Your workspace is up to date")?
            .finish();
        app.step(1. / 60.)?;
        Ok(Self {
            app,
            account,
            preferences,
            name,
            birthday,
            notifications,
            save,
            dialog,
            status,
            save_requested,
        })
    }
    fn after_dispatch(&mut self) -> Result<()> {
        if self.save_requested.replace(false) {
            self.app.text(self.status, "Saved")?;
            self.app.open(self.dialog, true)?;
        }
        Ok(())
    }
    fn verify_headless(mut self) -> Result<()> {
        assert!(self.app.bounds(self.name)?.width > 0.);
        assert!(self.app.bounds(self.notifications).is_err());
        click(&mut self.app, self.name)?;
        self.app.queue_text(" Lovelace")?;
        self.app.frame(1. / 60.)?;
        assert_eq!(self.app.editable_text(self.name)?, "Ada Lovelace");
        assert_eq!(
            self.app.current_date(self.birthday)?,
            Date {
                year: 1990,
                month: 5,
                day: 15
            }
        );
        click(&mut self.app, self.preferences)?;
        assert!(self.app.bounds(self.name).is_err());
        assert!(self.app.bounds(self.notifications)?.width > 0.);
        click(&mut self.app, self.notifications)?;
        assert!(self.app.is_checked(self.notifications)?);
        click(&mut self.app, self.account)?;
        assert!(self.app.bounds(self.name)?.width > 0.);
        click(&mut self.app, self.save)?;
        self.after_dispatch()?;
        self.app.step(1. / 60.)?;
        assert!(self.app.is_open(self.dialog)?);
        println!("workspace headless ok: account edited, preferences toggled, dialog opened");
        Ok(())
    }
    fn run_sdl(mut self, smoke_frames: Option<usize>) -> Result<()> {
        let mut frames = 0;
        loop {
            match self.app.frame(1. / 60.) {
                Ok(_) => self.after_dispatch()?,
                Err(Error(2)) => return Ok(()),
                Err(e) => return Err(e),
            }
            frames += 1;
            if smoke_frames.is_some_and(|limit| frames >= limit) {
                return Ok(());
            }
            std::thread::sleep(std::time::Duration::from_millis(16));
        }
    }
}
fn main() -> Result<()> {
    let headless = env::args().skip(1).any(|arg| arg == "--headless");
    if headless {
        return Workspace::new(App::headless(640., 480.)?)?.verify_headless();
    }
    let smoke_frames = env::var("CUI_RUST_SMOKE_FRAMES").ok().map(|value| {
        value
            .parse::<usize>()
            .expect("CUI_RUST_SMOKE_FRAMES must be an integer")
    });
    if smoke_frames.is_some() && env::var("CUI_TEST_VIRTUAL_DISPLAY").as_deref() != Ok("1") {
        panic!("smoke test requires an invisible display marker");
    }
    let font = env::var("CUI_TEST_FONT").expect("set CUI_TEST_FONT to a TTF path");
    let mut app = App::sdl(640., 480., "CUI Rust workspace", &font)?;
    if let Ok(fallback) = env::var("CUI_FONT_FALLBACK") {
        app.font_fallbacks(&[&fallback])?;
    }
    Workspace::new(app)?.run_sdl(smoke_frames)
}
