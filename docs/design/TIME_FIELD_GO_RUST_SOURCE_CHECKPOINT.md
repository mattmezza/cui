# Go/Rust localized TimeField source checkpoint

Base `8a402c2b`; source-only, uncompiled/unexecuted. No bridge, Python, Node, TS,
SDK snapshot or shared coverage metadata changes. Badge branch preserved.

New narrow Go/Rust modules expose C's owned616/80/64 records and complete optional
10-symbol capability group. Declarative Go config is app-resolved at mount;
existing rollback preserves ownership on errors. Rust's app-owned constructor
removes only its newly created node on setup failure. Mounted config writes local
state only after C success. Old scalar time routes and mandatory symbol loads
remain unchanged. Typed callbacks are post-step copied observations, not controlled
proposals. Poll remains explicit after failed steps; failed result getters stay C
failures. No foreign reducer or callback inside C step is added.

Complete authored English/Arabic examples and prepared tests live beside each
facade; `docs/GO_RUST_TIME_FIELD_LOCALIZED.md` documents APIs/ownership/limitations.
Only source formatters ran. Compiler, Go/Cargo tests, examples, optional old/partial
provider runtime, exact raw ABI assertions, installed/package/toolchain and all
canonical generations remain pending the exclusive heavy lease.
