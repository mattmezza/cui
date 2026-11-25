//! A paged inbox with stable row keys. Add --font PATH to open the native UI.
use cui_builders::{App, Error, Kind, Node, Phase, Rect, Result, WindowOptions};
use std::{
    cell::RefCell,
    rc::Rc,
    time::{Duration, Instant},
};

const HEADERS: &[&str] = &["Subject", "Sender"];
const ROWS: &[&[&str]] = &[
    &["Project brief", "Alex"],
    &["Component review", "Morgan"],
    &["Starter feedback", "Taylor"],
    &["Updated schedule", "Workspace"],
    &["Performance report", "Build team"],
    &["Accessibility notes", "Sam"],
    &["Asset handoff", "Design"],
    &["Release checklist", "Build team"],
    &["Test status", "Morgan"],
    &["Product notes", "Taylor"],
    &["Review requested", "Alex"],
    &["Release notes", "Build team"],
];
const KEYS: &[u32] = &[101, 102, 103, 104, 105, 106, 107, 108, 201, 202, 203, 204];
const INITIAL: usize = 8;

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
struct Ticket {
    generation: u64,
    frame: u64,
}

struct Inbox {
    table: Node,
    status: Node,
    generation: u64,
    pending: Option<Ticket>,
}

impl Inbox {
    fn new(app: &mut App) -> Result<Self> {
        let root = app
            .build_root(Kind::RootView, "root")?
            .root_padding(16.)?
            .finish();
        let content = app
            .build_child(root, Kind::Column, "content")?
            .gap_units(2.)?
            .finish();
        app.build_child(content, Kind::Label, "title")?
            .text("Inbox")?
            .finish();
        let table = app
            .build_child(content, Kind::DataTable, "inbox")?
            .data_table_keyed(HEADERS, &ROWS[..INITIAL], &KEYS[..INITIAL])?
            .visual_size(360., 160.)?
            .data_table_load_more(0)?
            .finish();
        let status = app
            .build_child(content, Kind::Label, "status")?
            .text("8 messages")?
            .finish();
        Ok(Self {
            table,
            status,
            generation: 1,
            pending: None,
        })
    }

    fn start_page(&mut self, app: &mut App) -> Result<Option<Ticket>> {
        if self.pending.is_some() {
            return Ok(None);
        }
        let Some(demand) = app.poll_data_table_load_more(self.table)? else {
            return Ok(None);
        };
        app.configure_data_table_load_more(self.table, false, 0)?;
        let ticket = Ticket {
            generation: self.generation,
            frame: demand.frame_sequence,
        };
        self.pending = Some(ticket);
        app.text(self.status, "Loading the next page…")?;
        Ok(Some(ticket))
    }

    fn retry(&mut self, app: &mut App) -> Result<()> {
        // Cancel the logical request so a late completion cannot be applied.
        self.generation += 1;
        self.pending = None;
        app.configure_data_table_load_more(self.table, true, 0)?;
        Ok(())
    }

    fn complete(&mut self, app: &mut App, ticket: Ticket) -> Result<bool> {
        if self.pending != Some(ticket) {
            return Ok(false);
        }
        app.data_table_keyed(self.table, HEADERS, ROWS, KEYS)?;
        self.pending = None;
        app.text(self.status, "12 messages · End of inbox")?;
        Ok(true)
    }

    fn run(&mut self, app: &mut App) -> Result<()> {
        let mut ready = None;
        app.run_with(|app| {
            // A local timer simulates transport latency without blocking paint.
            if let Some((ticket, when)) = ready {
                if Instant::now() >= when {
                    self.complete(app, ticket)?;
                    ready = None;
                }
            } else if let Some(ticket) = self.start_page(app)? {
                ready = Some((ticket, Instant::now() + Duration::from_millis(500)));
            }
            Ok(())
        })
    }

    fn verify(&mut self, app: &mut App, smoke: bool) -> Result<()> {
        let changes = Rc::new(RefCell::new(Vec::new()));
        let recorded = Rc::clone(&changes);
        app.on_change_event(self.table, move |e| recorded.borrow_mut().push(e.value))?;
        app.step(0.016)?;
        assert!(self.start_page(app)?.is_none());
        let first = app.data_table_cell_bounds(self.table, 1, 0)?;
        app.scroll(first.x + 10., first.y + 10., 0., -20.)?;
        app.step(0.016)?;
        let cancelled = self.start_page(app)?.expect("near-end demand");
        assert!(self.start_page(app)?.is_none());

        let selected = app.data_table_cell_bounds(self.table, 6, 0)?;
        click(app, selected)?;
        app.frame(0.016)?;
        assert_eq!(*changes.borrow(), [f64::from(KEYS[6])]);
        self.retry(app)?;
        app.step(0.016)?;
        let next = self.start_page(app)?.expect("retry demand");
        assert_ne!(cancelled, next);
        assert!(!self.complete(app, cancelled)?);
        assert!(self.complete(app, next)?);
        assert_eq!(app.frame(0.016)?, 0);
        assert_eq!(app.data_table_state(self.table)?.row, KEYS[6]);
        assert_eq!(app.data_table_cell_bounds(self.table, 6, 0)?.y, selected.y);
        assert_eq!(app.data_table_cell(self.table, 11, 0)?, ROWS[11][0]);
        app.scroll(first.x + 10., first.y + 10., 0., -20.)?;
        app.step(0.016)?;
        assert!(self.start_page(app)?.is_none());
        if smoke {
            let cell = app.data_table_cell_bounds(self.table, 10, 0)?;
            click(app, cell)?;
            let mut ran = false;
            app.run_with(|active| {
                assert_eq!(*changes.borrow(), [f64::from(KEYS[6]), f64::from(KEYS[10])]);
                assert!(self.start_page(active)?.is_none());
                ran = true;
                active.close()
            })?;
            assert!(ran);
        }
        println!("Appended the final page, kept the selected row, and discarded a late response.");
        Ok(())
    }
}

fn click(app: &mut App, rect: Rect) -> Result<()> {
    let (x, y) = (rect.x + rect.width / 2., rect.y + rect.height / 2.);
    app.pointer(Phase::Down, x, y)?;
    app.pointer(Phase::Up, x, y)
}

fn main() -> Result<()> {
    let args: Vec<_> = std::env::args().skip(1).collect();
    let smoke = args.iter().any(|arg| arg == "--smoke");
    let font = args.iter().position(|arg| arg == "--font").map(|index| {
        args.get(index + 1)
            .expect("--font requires a path")
            .as_str()
    });
    if smoke {
        assert!(
            font.is_some() && std::env::var("CUI_TEST_VIRTUAL_DISPLAY").as_deref() == Ok("1"),
            "--smoke requires a font and the private display from scripts/test_local.py"
        );
    }
    let mut app = if let Some(path) = font {
        App::sdl_with_window_options(
            480.,
            320.,
            "Paged inbox",
            path,
            WindowOptions {
                initially_hidden: smoke,
            },
        )?
    } else {
        App::headless(480., 320.)?
    };
    if !app.supports_data_table_paging() || !app.supports_keyed_data_table() {
        eprintln!("The paged inbox requires a bridge with paging and keyed replacement.");
        return Err(Error(9));
    }
    let mut inbox = Inbox::new(&mut app)?;
    if font.is_some() && !smoke {
        inbox.run(&mut app)
    } else {
        inbox.verify(&mut app, smoke)
    }
}
