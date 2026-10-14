// SPDX-License-Identifier: MIT
use regex::RegexBuilder;
use regex_automata::{
    nfa::thompson::{
        self,
        pikevm::{Cache, PikeVM},
        WhichCaptures,
    },
    util::{look::LookMatcher, syntax},
};
use std::{
    alloc::{alloc, dealloc, Layout},
    ffi::c_void,
    panic::{catch_unwind, AssertUnwindSafe},
    sync::{
        atomic::{AtomicU64, Ordering},
        Arc,
    },
};
const OK: i32 = 0;
const INVALID: i32 = 1;
const CAPACITY: i32 = 2;
const STALE: i32 = 3;
const BUSY: i32 = 4;
const FAILED: i32 = 5;
const PATTERN: i32 = 6;
#[repr(C)]
#[derive(Clone, Copy)]
pub struct Text {
    data: *const u8,
    length: usize,
}
#[repr(C)]
#[derive(Clone, Copy)]
pub struct Buffer {
    data: *mut c_void,
    bytes: usize,
}
#[repr(C)]
#[derive(Clone, Copy)]
pub struct Handle {
    state: *mut c_void,
    generation: u64,
}
#[repr(C)]
#[derive(Clone, Copy)]
pub struct Options {
    flags: u32,
    line: u32,
    size: usize,
    dfa: usize,
    nest: usize,
    pattern: usize,
    text: usize,
}
#[repr(C)]
pub struct Ops {
    begin: unsafe extern "C" fn(*mut c_void, u64, *mut *mut c_void) -> i32,
    matching: unsafe extern "C" fn(*mut c_void, Text, Buffer, *mut bool) -> i32,
    end: unsafe extern "C" fn(*mut c_void, *mut c_void) -> i32,
}
#[repr(C)]
pub struct Matcher {
    ops: Ops,
    object: *mut c_void,
    bytes: usize,
    generation: u64,
    scratch: Buffer,
}
struct Compiled {
    vm: PikeVM,
    max_text: usize,
}
struct Session {
    compiled: Arc<Compiled>,
    cache: Cache,
    busy: bool,
    owner: *mut c_void,
}
static GENERATION: AtomicU64 = AtomicU64::new(1);
fn generation() -> Result<u64, i32> {
    GENERATION
        .fetch_update(Ordering::Relaxed, Ordering::Relaxed, |n| n.checked_add(1))
        .map_err(|_| CAPACITY)
}
fn span(p: usize, n: usize) -> bool {
    n == 0 || p != 0 && p.checked_add(n).is_some() && n <= isize::MAX as usize
}
fn overlaps(a: usize, n: usize, b: usize, m: usize) -> bool {
    n != 0 && m != 0 && (a <= b && b - a < n || b < a && a - b < m)
}
unsafe fn text<'a>(t: Text, max: usize) -> Result<&'a str, i32> {
    if !span(t.data as usize, t.length) {
        return Err(INVALID);
    }
    if t.length > max {
        return Err(CAPACITY);
    }
    if t.length == 0 {
        return Ok("");
    }
    std::str::from_utf8(std::slice::from_raw_parts(t.data, t.length)).map_err(|_| INVALID)
}
unsafe fn store<T>(value: T) -> Result<*mut T, i32> {
    let p = alloc(Layout::new::<T>()) as *mut T;
    if p.is_null() {
        return Err(CAPACITY);
    }
    p.write(value);
    Ok(p)
}
unsafe fn release<T>(p: *mut T) {
    std::ptr::drop_in_place(p);
    dealloc(p as *mut u8, Layout::new::<T>());
}
fn protect(f: impl FnOnce() -> Result<(), i32>) -> i32 {
    match catch_unwind(AssertUnwindSafe(f)) {
        Ok(Ok(())) => OK,
        Ok(Err(e)) => e,
        Err(_) => FAILED,
    }
}
fn bounds(o: Options) -> Result<(usize, usize), i32> {
    if o.flags & !1023 != 0
        || o.line > 255
        || o.flags & 256 == 0 && o.line != 0
        || o.flags & 512 == 0 && (o.size != 0 || o.dfa != 0 || o.nest != 0)
        || o.nest > u32::MAX as usize
    {
        return Err(INVALID);
    }
    Ok((
        if o.pattern == 0 { 1048576 } else { o.pattern },
        if o.text == 0 { 1048576 } else { o.text },
    ))
}
#[no_mangle]
pub unsafe extern "C" fn cui_regex_compile(t: Text, o: Options, out: *mut Handle) -> i32 {
    protect(|| {
        if out.is_null()
            || !span(t.data as usize, t.length)
            || overlaps(
                out as usize,
                std::mem::size_of::<Handle>(),
                t.data as usize,
                t.length,
            )
        {
            return Err(INVALID);
        }
        if !(*out).state.is_null() {
            return Err(BUSY);
        }
        let (max_pattern, max_text) = bounds(o)?;
        let p = text(t, max_pattern)?;
        let unicode = o.flags & 64 == 0;
        let line = if o.flags & 256 != 0 {
            o.line as u8
        } else {
            b'\n'
        };
        let mut source = RegexBuilder::new(p);
        source
            .unicode(unicode)
            .case_insensitive(o.flags & 1 != 0)
            .multi_line(o.flags & 2 != 0)
            .dot_matches_new_line(o.flags & 4 != 0)
            .crlf(o.flags & 8 != 0)
            .swap_greed(o.flags & 16 != 0)
            .ignore_whitespace(o.flags & 32 != 0)
            .octal(o.flags & 128 != 0)
            .line_terminator(line);
        if o.flags & 512 != 0 {
            source
                .size_limit(o.size)
                .dfa_size_limit(o.dfa)
                .nest_limit(o.nest as u32);
        }
        let admitted = source.build().map_err(|_| PATTERN)?;
        drop(admitted);
        let syn = syntax::Config::new()
            .unicode(unicode)
            .case_insensitive(o.flags & 1 != 0)
            .multi_line(o.flags & 2 != 0)
            .dot_matches_new_line(o.flags & 4 != 0)
            .crlf(o.flags & 8 != 0)
            .swap_greed(o.flags & 16 != 0)
            .ignore_whitespace(o.flags & 32 != 0)
            .octal(o.flags & 128 != 0)
            .line_terminator(line)
            .nest_limit(if o.flags & 512 != 0 {
                o.nest as u32
            } else {
                250
            });
        let mut look = LookMatcher::new();
        look.set_line_terminator(line);
        let cfg = thompson::Config::new()
            .which_captures(WhichCaptures::None)
            .nfa_size_limit(Some(if o.flags & 512 != 0 {
                o.size
            } else {
                10 * 1024 * 1024
            }))
            .look_matcher(look);
        let vm = PikeVM::builder()
            .syntax(syn)
            .thompson(cfg)
            .build(p)
            .map_err(|_| PATTERN)?;
        let g = generation()?;
        let state = store(Arc::new(Compiled { vm, max_text }))?;
        out.write(Handle {
            state: state as *mut c_void,
            generation: g,
        });
        Ok(())
    })
}
#[no_mangle]
pub unsafe extern "C" fn cui_regex_destroy(h: *mut Handle) -> i32 {
    protect(|| {
        if h.is_null() {
            return Err(INVALID);
        }
        if !(*h).state.is_null() {
            release((*h).state as *mut Arc<Compiled>);
            (*h).state = std::ptr::null_mut();
        }
        Ok(())
    })
}
unsafe fn prepare(h: *const Handle, budget: usize) -> Result<Session, i32> {
    if h.is_null() {
        return Err(INVALID);
    }
    if (*h).state.is_null() || (*h).generation == 0 {
        return Err(STALE);
    }
    let compiled = (&*((*h).state as *const Arc<Compiled>)).clone();
    let cache = Cache::cui_try_new(&compiled.vm, budget).map_err(|_| CAPACITY)?;
    Ok(Session {
        compiled,
        cache,
        busy: false,
        owner: std::ptr::null_mut(),
    })
}
#[no_mangle]
pub unsafe extern "C" fn cui_regex_session_create(
    h: *const Handle,
    budget: usize,
    out: *mut Handle,
) -> i32 {
    protect(|| {
        if out.is_null()
            || h.is_null()
            || overlaps(
                h as usize,
                std::mem::size_of::<Handle>(),
                out as usize,
                std::mem::size_of::<Handle>(),
            )
        {
            return Err(INVALID);
        }
        if !(*out).state.is_null() {
            return Err(BUSY);
        }
        let session = prepare(h, budget)?;
        let g = generation()?;
        let state = store(session)?;
        out.write(Handle {
            state: state as *mut c_void,
            generation: g,
        });
        Ok(())
    })
}
#[no_mangle]
pub unsafe extern "C" fn cui_regex_session_rebind(
    out: *mut Handle,
    h: *const Handle,
    budget: usize,
) -> i32 {
    protect(|| {
        if out.is_null()
            || h.is_null()
            || overlaps(
                h as usize,
                std::mem::size_of::<Handle>(),
                out as usize,
                std::mem::size_of::<Handle>(),
            )
        {
            return Err(INVALID);
        }
        if (*out).state.is_null() {
            return Err(STALE);
        }
        let old = (*out).state as *mut Session;
        if (*old).busy {
            return Err(BUSY);
        }
        let prepared = prepare(h, budget)?;
        let g = generation()?;
        old.write({
            let previous = std::ptr::read(old);
            drop(previous);
            prepared
        });
        (*out).generation = g;
        Ok(())
    })
}
#[no_mangle]
pub unsafe extern "C" fn cui_regex_session_destroy(out: *mut Handle) -> i32 {
    protect(|| {
        if out.is_null() {
            return Err(INVALID);
        }
        if (*out).state.is_null() {
            return Ok(());
        }
        let state = (*out).state as *mut Session;
        if (*state).busy {
            return Err(BUSY);
        }
        release(state);
        (*out).state = std::ptr::null_mut();
        Ok(())
    })
}
unsafe extern "C" fn begin(object: *mut c_void, g: u64, out: *mut *mut c_void) -> i32 {
    protect(|| {
        if object.is_null()
            || out.is_null()
            || overlaps(
                object as usize,
                std::mem::size_of::<Handle>(),
                out as usize,
                std::mem::size_of::<*mut c_void>(),
            )
        {
            return Err(INVALID);
        }
        let h = &*(object as *const Handle);
        if h.state.is_null() || h.generation != g {
            return Err(STALE);
        }
        let state = &mut *(h.state as *mut Session);
        if state.busy {
            return Err(BUSY);
        }
        if overlaps(
            h.state as usize,
            std::mem::size_of::<Session>(),
            out as usize,
            std::mem::size_of::<*mut c_void>(),
        ) {
            return Err(INVALID);
        }
        state.busy = true;
        state.owner = object;
        out.write(h.state);
        Ok(())
    })
}
unsafe extern "C" fn matching(lease: *mut c_void, t: Text, scratch: Buffer, out: *mut bool) -> i32 {
    protect(|| {
        if lease.is_null()
            || out.is_null()
            || !scratch.data.is_null()
            || scratch.bytes != 0
            || !span(t.data as usize, t.length)
            || overlaps(
                t.data as usize,
                t.length,
                out as usize,
                std::mem::size_of::<bool>(),
            )
            || overlaps(
                lease as usize,
                std::mem::size_of::<Session>(),
                out as usize,
                std::mem::size_of::<bool>(),
            )
            || overlaps(
                lease as usize,
                std::mem::size_of::<Session>(),
                t.data as usize,
                t.length,
            )
        {
            return Err(INVALID);
        }
        let s = &mut *(lease as *mut Session);
        if !s.busy {
            return Err(STALE);
        }
        if overlaps(
            s.owner as usize,
            std::mem::size_of::<Handle>(),
            out as usize,
            std::mem::size_of::<bool>(),
        ) || overlaps(
            s.owner as usize,
            std::mem::size_of::<Handle>(),
            t.data as usize,
            t.length,
        ) {
            return Err(INVALID);
        }
        let value = text(t, s.compiled.max_text)?;
        let found = s.compiled.vm.is_match(&mut s.cache, value);
        out.write(found);
        Ok(())
    })
}
unsafe extern "C" fn end(object: *mut c_void, lease: *mut c_void) -> i32 {
    protect(|| {
        if object.is_null() || lease.is_null() {
            return Err(INVALID);
        }
        let h = &*(object as *const Handle);
        if h.state != lease {
            return Err(STALE);
        }
        let s = &mut *(lease as *mut Session);
        if !s.busy {
            return Err(STALE);
        }
        s.busy = false;
        s.owner = std::ptr::null_mut();
        Ok(())
    })
}
#[no_mangle]
pub unsafe extern "C" fn cui_regex_matcher(h: *mut Handle) -> Matcher {
    Matcher {
        ops: Ops {
            begin,
            matching,
            end,
        },
        object: h as *mut c_void,
        bytes: std::mem::size_of::<Handle>(),
        generation: if h.is_null() { 0 } else { (*h).generation },
        scratch: Buffer {
            data: std::ptr::null_mut(),
            bytes: 0,
        },
    }
}

#[cfg(test)]
mod tests;
