// SPDX-License-Identifier: MIT
use regex::{Regex, RegexBuilder};
use regex_automata::{
    nfa::thompson::{
        self,
        pikevm::{Cache, PikeVM},
        WhichCaptures,
    },
    util::syntax,
};
use std::alloc::{GlobalAlloc, Layout, System};
use std::sync::atomic::{AtomicBool, AtomicUsize, Ordering::SeqCst};
static TRACK: AtomicBool = AtomicBool::new(false);
static CALLS: AtomicUsize = AtomicUsize::new(0);
static FAIL: AtomicUsize = AtomicUsize::new(0);
static LIVE: AtomicUsize = AtomicUsize::new(0);
struct Spy;
unsafe impl GlobalAlloc for Spy {
    unsafe fn alloc(&self, l: Layout) -> *mut u8 {
        if TRACK.load(SeqCst) {
            let n = CALLS.fetch_add(1, SeqCst) + 1;
            if FAIL.load(SeqCst) == n {
                return std::ptr::null_mut();
            }
        }
        let p = System.alloc(l);
        if !p.is_null() {
            LIVE.fetch_add(l.size(), SeqCst);
        }
        p
    }
    unsafe fn alloc_zeroed(&self, l: Layout) -> *mut u8 {
        if TRACK.load(SeqCst) {
            let n = CALLS.fetch_add(1, SeqCst) + 1;
            if FAIL.load(SeqCst) == n {
                return std::ptr::null_mut();
            }
        }
        let p = System.alloc_zeroed(l);
        if !p.is_null() {
            LIVE.fetch_add(l.size(), SeqCst);
        }
        p
    }
    unsafe fn realloc(&self, p: *mut u8, l: Layout, n: usize) -> *mut u8 {
        if TRACK.load(SeqCst) {
            let c = CALLS.fetch_add(1, SeqCst) + 1;
            if FAIL.load(SeqCst) == c {
                return std::ptr::null_mut();
            }
        }
        let q = System.realloc(p, l, n);
        if !q.is_null() {
            if n >= l.size() {
                LIVE.fetch_add(n - l.size(), SeqCst);
            } else {
                LIVE.fetch_sub(l.size() - n, SeqCst);
            }
        }
        q
    }
    unsafe fn dealloc(&self, p: *mut u8, l: Layout) {
        LIVE.fetch_sub(l.size(), SeqCst);
        System.dealloc(p, l)
    }
}
#[global_allocator]
static ALLOC: Spy = Spy;
fn start(fail: usize) {
    CALLS.store(0, SeqCst);
    FAIL.store(fail, SeqCst);
    TRACK.store(true, SeqCst)
}
fn finish() -> usize {
    TRACK.store(false, SeqCst);
    FAIL.store(0, SeqCst);
    CALLS.load(SeqCst)
}
#[derive(Clone, Copy)]
struct Flags {
    case: bool,
    multi: bool,
    dot: bool,
    crlf: bool,
    greed: bool,
    space: bool,
    unicode: bool,
    octal: bool,
    line: u8,
}
fn build(
    p: &str,
    f: Flags,
) -> (
    Result<Regex, regex::Error>,
    Result<PikeVM, regex_automata::nfa::thompson::BuildError>,
) {
    let a = RegexBuilder::new(p)
        .case_insensitive(f.case)
        .multi_line(f.multi)
        .dot_matches_new_line(f.dot)
        .crlf(f.crlf)
        .swap_greed(f.greed)
        .ignore_whitespace(f.space)
        .unicode(f.unicode)
        .octal(f.octal)
        .line_terminator(f.line)
        .build();
    let mut look = regex_automata::util::look::LookMatcher::new();
    look.set_line_terminator(f.line);
    let b = PikeVM::builder()
        .syntax(
            syntax::Config::new()
                .case_insensitive(f.case)
                .multi_line(f.multi)
                .dot_matches_new_line(f.dot)
                .crlf(f.crlf)
                .swap_greed(f.greed)
                .ignore_whitespace(f.space)
                .unicode(f.unicode)
                .octal(f.octal)
                .line_terminator(f.line),
        )
        .thompson(
            thompson::Config::new()
                .which_captures(WhichCaptures::None)
                .look_matcher(look),
        )
        .build(p);
    (a, b)
}
#[test]
fn oracle() {
    let default = Flags {
        case: false,
        multi: false,
        dot: false,
        crlf: false,
        greed: false,
        space: false,
        unicode: true,
        octal: false,
        line: b'\n',
    };
    let variants = [
        default,
        Flags {
            case: true,
            ..default
        },
        Flags {
            multi: true,
            crlf: true,
            ..default
        },
        Flags {
            dot: true,
            greed: true,
            ..default
        },
        Flags {
            space: true,
            ..default
        },
        Flags {
            unicode: false,
            octal: true,
            ..default
        },
        Flags {
            multi: true,
            line: b'@',
            ..default
        },
    ];
    let mut patterns: Vec<String> = [
        "abc",
        "^abc$",
        r"\Aabc\z",
        r"\b\w+\b",
        r"\d+",
        r"\s+",
        r"\p{Greek}+",
        r"\p{Cyrillic}+",
        r"\p{Script=Han}",
        r"\p{Emoji}",
        r"\p{Letter}",
        r"\P{Number}+",
        r"[\pL&&[^a-z]]+",
        r"(?i:straße|Σ|K)",
        r"(?m:^abc$)",
        r"(?s:.)*",
        r"(?U:a+)",
        r"(?x:a b c)",
        r"(?-u:\b)word(?-u:\b)",
        "",
        r"(?:a?|b?|c?)*(?:z|x)",
        r"(?:a|aa|aaa|aaaa|b|bb|bbb)*z",
        r"((a?)*)*",
        r"(?P<name>\w+)",
        r"(ab){0,5}?",
        r"a{5,}",
        r"[^a-z]",
        r"\x{1F600}",
        r"\u{00E9}",
        r"a\x00b",
        r"^$",
        r"\B",
        r"(?R:^a$)",
        r"(?-u:.)",
        r"\141",
        r"(a)\1",
        r"(?=a)",
        r"(?<=a)",
        "[",
        "(",
        r"\p{NotAProperty}",
    ]
    .iter()
    .map(|p| p.to_string())
    .collect();
    for n in 1..24 {
        patterns.push(format!("(?:a?|b?|c?){{0,{n}}}(?:z|x)"));
    }
    let mut hays: Vec<String> = [
        "",
        "abc",
        "xabcx",
        "a",
        "aa",
        "z",
        "abc\n",
        "a\r\nb",
        "a@b",
        "word",
        " word ",
        "ΔΣσς",
        "rök Straße STRASSE KK",
        "中文😀",
        "١२９",
        "e\u{301}",
        "\u{a0}\u{2009}",
        "a\0b",
        "!word!",
        "a\n\nz",
    ]
    .iter()
    .map(|x| x.to_string())
    .collect();
    hays.push("abc".repeat(500));
    hays.push("a".repeat(3000));
    hays.push(format!("{}z", "a".repeat(3000)));
    hays.push("Δ中😀 e\u{301}\r\n".repeat(200));
    let alphabet = [
        'a', 'b', 'z', ' ', '\n', '\r', 'Δ', '中', '😀', 'é', '\u{301}', '١',
    ];
    let mut seed = 17u64;
    for _ in 0..128 {
        let mut s = String::new();
        for _ in 0..48 {
            seed = seed.wrapping_mul(6364136223846793005).wrapping_add(1);
            s.push(alphabet[((seed >> 32) as usize) % alphabet.len()]);
        }
        hays.push(s)
    }
    let mut compared = 0;
    let mut compile_errors = 0;
    let mut compiled = 0;
    for f in variants {
        for p in &patterns {
            let (oracle, _) = build(p, f);
            let options = super::Options {
                flags: (f.case as u32)
                    | ((f.multi as u32) << 1)
                    | ((f.dot as u32) << 2)
                    | ((f.crlf as u32) << 3)
                    | ((f.greed as u32) << 4)
                    | ((f.space as u32) << 5)
                    | (((!f.unicode) as u32) << 6)
                    | ((f.octal as u32) << 7)
                    | 256,
                line: f.line as u32,
                size: 0,
                dfa: 0,
                nest: 0,
                pattern: 0,
                text: 0,
            };
            let mut compiled_handle = super::Handle {
                state: std::ptr::null_mut(),
                generation: 0,
            };
            let code = unsafe {
                super::cui_regex_compile(
                    super::Text {
                        data: p.as_ptr(),
                        length: p.len(),
                    },
                    options,
                    &mut compiled_handle,
                )
            };
            assert_eq!(
                oracle.is_ok(),
                code == super::OK,
                "compile disagreement {p}"
            );
            let oracle = match oracle {
                Ok(a) => a,
                _ => {
                    compile_errors += 1;
                    continue;
                }
            };
            compiled += 1;
            let mut session = super::Handle {
                state: std::ptr::null_mut(),
                generation: 0,
            };
            unsafe {
                assert_eq!(
                    super::cui_regex_session_create(
                        &compiled_handle,
                        64 * 1024 * 1024,
                        &mut session
                    ),
                    0
                );
                assert_eq!(super::cui_regex_destroy(&mut compiled_handle), 0);
            }
            let mut lease = std::ptr::null_mut();
            unsafe {
                assert_eq!(
                    super::begin(
                        &mut session as *mut _ as *mut _,
                        session.generation,
                        &mut lease
                    ),
                    0
                );
            }
            for h in &hays {
                let expected = oracle.is_match(h);
                let mut actual = false;
                start(0);
                let code = unsafe {
                    super::matching(
                        lease,
                        super::Text {
                            data: h.as_ptr(),
                            length: h.len(),
                        },
                        super::Buffer {
                            data: std::ptr::null_mut(),
                            bytes: 0,
                        },
                        &mut actual,
                    )
                };
                let allocations = finish();
                assert_eq!(code, 0);
                assert_eq!(allocations, 0, "match allocated {p}");
                assert_eq!(actual, expected, "semantic mismatch {p} {h:?}");
                compared += 1;
            }
            unsafe {
                assert_eq!(super::end(&mut session as *mut _ as *mut _, lease), 0);
                assert_eq!(super::cui_regex_session_destroy(&mut session), 0);
            }
        }
    }
    let (_, vm) = build("(?:a?|b?|c?)*(?:z|x)", default);
    let vm = vm.unwrap();
    let mut old = vm.create_cache();
    start(0);
    let _ = vm.is_match(&mut old, "aaaaaz");
    let baseline = finish();
    assert!(baseline > 0);
    let zero = Cache::cui_try_new(&vm, 0);
    assert!(zero.is_err());
    drop(zero);
    let live_before = LIVE.load(SeqCst);
    start(0);
    let cache = Cache::cui_try_new(&vm, 64 * 1024 * 1024).unwrap();
    let prep_allocations = finish();
    let cache_bytes = LIVE.load(SeqCst) - live_before;
    drop(cache);
    start(0);
    let short = Cache::cui_try_new(&vm, cache_bytes - 1);
    let attempts = finish();
    assert!(short.is_err() && attempts == 0);
    drop(short);
    let exact = Cache::cui_try_new(&vm, cache_bytes).unwrap();
    drop(exact);
    for fail in 1..=prep_allocations {
        let before = LIVE.load(SeqCst);
        start(fail);
        let result = Cache::cui_try_new(&vm, 64 * 1024 * 1024);
        finish();
        assert!(result.is_err(), "allocation failure ignored");
        drop(result);
        assert_eq!(LIVE.load(SeqCst), before, "failed preparation leaked");
    }
    let options = super::Options {
        flags: 0,
        line: 0,
        size: 0,
        dfa: 0,
        nest: 0,
        pattern: 0,
        text: 0,
    };
    let mut handle = super::Handle {
        state: std::ptr::null_mut(),
        generation: 0,
    };
    unsafe {
        assert_eq!(
            super::cui_regex_compile(
                super::Text {
                    data: b"(?:a?|b?|c?)*(?:z|x)".as_ptr(),
                    length: b"(?:a?|b?|c?)*(?:z|x)".len()
                },
                options,
                &mut handle
            ),
            0
        );
    }
    let before = LIVE.load(SeqCst);
    let mut session = super::Handle {
        state: std::ptr::null_mut(),
        generation: 0,
    };
    start(0);
    let status =
        unsafe { super::cui_regex_session_create(&handle, 64 * 1024 * 1024, &mut session) };
    let reserves = finish();
    assert_eq!(status, 0);
    assert_eq!(reserves, 8);
    unsafe {
        assert_eq!(super::cui_regex_session_destroy(&mut session), 0);
    }
    assert_eq!(LIVE.load(SeqCst), before);
    for fail in 1..=reserves {
        let before = LIVE.load(SeqCst);
        start(fail);
        let status =
            unsafe { super::cui_regex_session_create(&handle, 64 * 1024 * 1024, &mut session) };
        finish();
        assert_eq!(status, super::CAPACITY);
        assert!(session.state.is_null());
        assert_eq!(LIVE.load(SeqCst), before);
    }
    unsafe {
        assert_eq!(super::cui_regex_destroy(&mut handle), 0);
    }
    let captured = "(a)".repeat(40);
    let mut limit_difference = false;
    let mut difference_limit = 0;
    for limit in (256..8192).step_by(64) {
        let source = RegexBuilder::new(&captured).size_limit(limit).build();
        let reduced = PikeVM::builder()
            .thompson(
                thompson::Config::new()
                    .which_captures(WhichCaptures::None)
                    .nfa_size_limit(Some(limit)),
            )
            .build(&captured);
        if source.is_err() && reduced.is_ok() {
            limit_difference = true;
            difference_limit = limit;
            break;
        }
    }
    let mut limited = super::Handle {
        state: std::ptr::null_mut(),
        generation: 0,
    };
    let options = super::Options {
        flags: 512,
        line: 0,
        size: difference_limit,
        dfa: 2 * 1024 * 1024,
        nest: 250,
        pattern: 0,
        text: 0,
    };
    unsafe {
        assert_eq!(
            super::cui_regex_compile(
                super::Text {
                    data: captured.as_ptr(),
                    length: captured.len()
                },
                options,
                &mut limited
            ),
            super::PATTERN
        );
    }
    assert!(limited.state.is_null() && limited.generation == 0);
    assert!(
        limit_difference,
        "expected source captured-NFA compile-limit distinction"
    );
    println!("compiled={compiled} compile_rejections={compile_errors} comparisons={compared} prepared_match_allocations=0 unprepared_first_match_allocations={baseline} cache_prepare_allocations={prep_allocations} injected_prepare_failures={prep_allocations} exact_cache_heap_budget={cache_bytes} capture_free_compile_limit_difference={limit_difference} differing_compile_limit={difference_limit} provider_session_failure_positions={reserves}");
}
