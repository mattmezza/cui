//! Optional typed retained keyboard shortcut presentation.
use crate::{check, App, Builder, Error, Kind, Node, RawNode, Result};
use std::{
    ffi::{c_char, c_void},
    sync::OnceLock,
};

type Set = unsafe extern "C" fn(
    *mut c_void,
    RawNode,
    u32,
    u32,
    *const c_char,
    usize,
    *const c_char,
    usize,
) -> u32;
fn setter() -> Option<Set> {
    static SET: OnceLock<Option<Set>> = OnceLock::new();
    *SET.get_or_init(resolve)
}
#[cfg(cui_bridge_static)]
fn resolve() -> Option<Set> {
    extern "C" {
        fn cui_bridge_set_kbd_stroke_v1(
            app: *mut c_void,
            node: RawNode,
            modifiers: u32,
            flags: u32,
            key: *const c_char,
            key_bytes: usize,
            name: *const c_char,
            name_bytes: usize,
        ) -> u32;
    }
    Some(cui_bridge_set_kbd_stroke_v1)
}
#[cfg(all(not(cui_bridge_static), any(unix, windows)))]
fn resolve() -> Option<Set> {
    let p = unsafe { crate::notification_symbol_v1(c"cui_bridge_set_kbd_stroke_v1".as_ptr()) };
    if p.is_null() {
        None
    } else {
        Some(unsafe { std::mem::transmute::<*mut c_void, Set>(p) })
    }
}
#[cfg(not(any(cui_bridge_static, unix, windows)))]
fn resolve() -> Option<Set> {
    None
}

/// Modifier keys; combine values with the bitwise OR operator.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct KbdModifiers(pub u32);
impl KbdModifiers {
    pub const SHIFT: Self = Self(1);
    pub const CTRL: Self = Self(2);
    pub const ALT: Self = Self(4);
    pub const META: Self = Self(8);
}
impl std::ops::BitOr for KbdModifiers {
    type Output = Self;
    fn bitor(self, rhs: Self) -> Self {
        Self(self.0 | rhs.0)
    }
}
/// Owned formatting options. Empty accessible_name uses the generated label.
#[derive(Clone, Debug, Default, PartialEq, Eq)]
pub struct KbdStrokeOptions {
    pub modifiers: KbdModifiers,
    pub plain: bool,
    pub outline: bool,
    pub mac_symbols: bool,
    pub accessible_name: String,
}
impl KbdStrokeOptions {
    fn validate(&self, key: &str) -> Result<()> {
        if key.is_empty()
            || !valid_line(key, 128)
            || self.modifiers.0 & !15 != 0
            || !valid_line(&self.accessible_name, 1024)
        {
            return Err(Error(3));
        }
        Ok(())
    }
}
fn valid_line(value: &str, limit: usize) -> bool {
    value.len() <= limit
        && !value.contains(|c| {
            matches!(
                c,
                '\0'..='\x1f' | '\x7f' | '\u{85}' | '\u{2028}' | '\u{2029}'
            )
        })
}
impl Error {
    /// The selected provider lacks the optional typed Kbd export.
    pub const KBD_STROKE_UNAVAILABLE: Self = Self(9);
}
impl App {
    /// Whether typed shortcut presentation is supported by this provider.
    pub fn supports_kbd_stroke(&self) -> bool {
        setter().is_some()
    }
    /// Replace the shortcut with a copied UTF-8 key (1–128 bytes) and options.
    pub fn kbd_stroke(
        &mut self,
        node: Node,
        key: &str,
        options: &KbdStrokeOptions,
    ) -> Result<&mut Self> {
        let node = self.valid(node)?;
        options.validate(key)?;
        let set = setter().ok_or(Error::KBD_STROKE_UNAVAILABLE)?;
        let flags = u32::from(options.plain)
            | (u32::from(options.outline) << 1)
            | (u32::from(options.mac_symbols) << 2);
        check(unsafe {
            set(
                self.raw,
                node,
                options.modifiers.0,
                flags,
                key.as_ptr().cast(),
                key.len(),
                options.accessible_name.as_ptr().cast(),
                options.accessible_name.len(),
            )
        })?;
        Ok(self)
    }
    /// Create a typed Kbd child, preflighting support before creating the node.
    pub fn child_kbd_stroke(
        &mut self,
        parent: Node,
        key: &str,
        stroke_key: &str,
        options: &KbdStrokeOptions,
    ) -> Result<Node> {
        options.validate(stroke_key)?;
        setter().ok_or(Error::KBD_STROKE_UNAVAILABLE)?;
        let node = self.child(parent, Kind::Kbd, key)?;
        if let Err(error) = self.kbd_stroke(node, stroke_key, options) {
            self.remove(node)?;
            return Err(error);
        }
        Ok(node)
    }
}
impl Builder<'_> {
    /// Configure this Kbd with a typed shortcut.
    pub fn kbd_stroke(self, key: &str, options: &KbdStrokeOptions) -> Result<Self> {
        self.app.kbd_stroke(self.node, key, options)?;
        Ok(self)
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn validates_owned_shortcuts() {
        let mut options = KbdStrokeOptions::default();
        assert!(options.validate("é").is_ok());
        for key in ["", "bad\0key", "a\tb", "\u{2028}", &"é".repeat(65)] {
            assert_eq!(options.validate(key), Err(Error(3)));
        }
        options.modifiers = KbdModifiers::CTRL | KbdModifiers::SHIFT;
        assert!(options.validate("S").is_ok());
        options.modifiers = KbdModifiers(16);
        assert_eq!(options.validate("S"), Err(Error(3)));
        options.modifiers = KbdModifiers::default();
        options.accessible_name = "x".repeat(1025);
        assert_eq!(options.validate("S"), Err(Error(3)));
    }
}
