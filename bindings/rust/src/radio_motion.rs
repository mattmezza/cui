//! Optional retained Radio and RadioGroup presentation.
use super::{check, App, Builder, Error, Kind, Node, RawNode, Result};
use std::ffi::{c_char, c_void};
type Set = unsafe extern "C" fn(*mut c_void, RawNode, u32) -> u32;
type SetTooltip = unsafe extern "C" fn(*mut c_void, RawNode, *const c_char, u32) -> u32;

#[derive(Clone, Copy)]
enum Setting {
    Motion,
    ItemContent,
}

#[derive(Clone, Copy, PartialEq, Eq)]
pub(super) enum LegacyRadioNode {
    Standalone,
    Group,
    Item,
}

#[cfg(cui_bridge_static)]
fn setter(setting: Setting) -> Option<Set> {
    extern "C" {
        fn cui_bridge_radio_set_motion_v1(a: *mut c_void, n: RawNode, enabled: u32) -> u32;
        fn cui_bridge_radio_item_set_content_v1(a: *mut c_void, n: RawNode, enabled: u32) -> u32;
    }
    Some(match setting {
        Setting::Motion => cui_bridge_radio_set_motion_v1,
        Setting::ItemContent => cui_bridge_radio_item_set_content_v1,
    })
}
#[cfg(all(not(cui_bridge_static), any(unix, windows)))]
fn setter(setting: Setting) -> Option<Set> {
    let name = match setting {
        Setting::Motion => b"cui_bridge_radio_set_motion_v1\0".as_slice(),
        Setting::ItemContent => b"cui_bridge_radio_item_set_content_v1\0".as_slice(),
    };
    unsafe {
        let p = super::notification_symbol_v1(name.as_ptr().cast());
        if p.is_null() {
            None
        } else {
            Some(std::mem::transmute::<*mut c_void, Set>(p))
        }
    }
}
#[cfg(all(not(cui_bridge_static), not(any(unix, windows))))]
fn setter(_: Setting) -> Option<Set> {
    None
}

#[cfg(cui_bridge_static)]
fn tooltip_setter() -> Option<SetTooltip> {
    extern "C" {
        fn cui_bridge_radio_set_tooltip_v1(
            app: *mut c_void,
            node: RawNode,
            text: *const c_char,
            delay_ms: u32,
        ) -> u32;
    }
    Some(cui_bridge_radio_set_tooltip_v1)
}

#[cfg(all(not(cui_bridge_static), any(unix, windows)))]
fn tooltip_setter() -> Option<SetTooltip> {
    let name = b"cui_bridge_radio_set_tooltip_v1\0";
    unsafe {
        let p = super::notification_symbol_v1(name.as_ptr().cast());
        if p.is_null() {
            None
        } else {
            Some(std::mem::transmute::<*mut c_void, SetTooltip>(p))
        }
    }
}

#[cfg(all(not(cui_bridge_static), not(any(unix, windows))))]
fn tooltip_setter() -> Option<SetTooltip> {
    None
}

type SetTabIndex = unsafe extern "C" fn(*mut c_void, RawNode, u32, i32) -> u32;
#[cfg(cui_bridge_static)]
fn tab_index_setter() -> Option<SetTabIndex> {
    extern "C" {
        fn cui_bridge_radio_set_tab_index_v1(
            a: *mut c_void,
            n: RawNode,
            enabled: u32,
            index: i32,
        ) -> u32;
    }
    Some(cui_bridge_radio_set_tab_index_v1)
}
#[cfg(all(not(cui_bridge_static), any(unix, windows)))]
fn tab_index_setter() -> Option<SetTabIndex> {
    unsafe {
        let p =
            super::notification_symbol_v1(b"cui_bridge_radio_set_tab_index_v1\0".as_ptr().cast());
        if p.is_null() {
            None
        } else {
            Some(std::mem::transmute::<*mut c_void, SetTabIndex>(p))
        }
    }
}
#[cfg(all(not(cui_bridge_static), not(any(unix, windows))))]
fn tab_index_setter() -> Option<SetTabIndex> {
    None
}

impl App {
    /// Whether the provider supports signed Radio/RadioGroup traversal order.
    pub fn supports_radio_tab_index(&self) -> bool {
        tab_index_setter().is_some()
    }

    /// Set Radio or RadioGroup traversal order. None restores composition order;
    /// Some(0) is explicit, negative values come earlier. Groups keep one roving stop.
    /// Older providers accept clearing and return Error(9) for explicit indices.
    pub fn radio_tab_index(&mut self, node: Node, index: Option<i32>) -> Result<&mut Self> {
        let raw = self.valid(node)?;
        if let Some(set) = tab_index_setter() {
            check(unsafe { set(self.raw, raw, index.is_some() as u32, index.unwrap_or(0)) })?;
        } else {
            if !matches!(
                self.legacy_radio_node(raw)?,
                Some(LegacyRadioNode::Standalone | LegacyRadioNode::Group)
            ) {
                return Err(Error(8));
            }
            if index.is_some() {
                return Err(Error(9));
            }
        }
        Ok(self)
    }

    /// Whether this provider offers retained Radio and RadioGroup motion.
    pub fn supports_radio_motion(&self) -> bool {
        setter(Setting::Motion).is_some()
    }
    /// Whether this provider can paint passive children inside a direct RadioGroup Item.
    pub fn supports_radio_item_content(&self) -> bool {
        setter(Setting::ItemContent).is_some()
    }
    /// Whether this provider can attach hints to retained radio options.
    pub fn supports_radio_tooltip(&self) -> bool {
        tooltip_setter().is_some()
    }
    pub(super) fn track_legacy_radio_node(&mut self, raw: RawNode, parent: RawNode, kind: Kind) {
        let legacy = match kind {
            Kind::Radio
                if !self.supports_radio_motion()
                    || !self.supports_radio_tooltip()
                    || !self.supports_radio_tab_index() =>
            {
                LegacyRadioNode::Standalone
            }
            Kind::RadioGroup
                if !self.supports_radio_tab_index()
                    || !self.supports_radio_motion()
                    || !self.supports_radio_item_content()
                    || !self.supports_radio_tooltip() =>
            {
                LegacyRadioNode::Group
            }
            Kind::Item
                if self.legacy_radio_nodes.get(&parent) == Some(&LegacyRadioNode::Group)
                    && (!self.supports_radio_item_content() || !self.supports_radio_tooltip()) =>
            {
                LegacyRadioNode::Item
            }
            _ => return,
        };
        self.legacy_radio_nodes.insert(raw, legacy);
    }
    fn legacy_radio_node(&self, raw: RawNode) -> Result<Option<LegacyRadioNode>> {
        let mut required = 0;
        let status = unsafe {
            super::cui_bridge_get_text(self.raw, raw, std::ptr::null_mut(), 0, &mut required)
        };
        if status != 0 && status != 5 {
            return Err(Error(status));
        }
        Ok(self.legacy_radio_nodes.get(&raw).copied())
    }
    /// Opt into retained mark/fill animation. False preserves immediate presentation
    /// even with an older provider; unsupported opt-in returns Error(9).
    /// Only Radio and RadioGroup nodes are accepted. Events keep their existing timing.
    pub fn animate_selection(&mut self, node: Node, enabled: bool) -> Result<&mut Self> {
        let raw = self.valid(node)?;
        if let Some(set) = setter(Setting::Motion) {
            check(unsafe { set(self.raw, raw, enabled as u32) })?;
        } else {
            if !matches!(
                self.legacy_radio_node(raw)?,
                Some(LegacyRadioNode::Group | LegacyRadioNode::Standalone)
            ) {
                return Err(Error(8));
            }
            if enabled {
                return Err(Error(9));
            }
        }
        Ok(self)
    }

    /// Show or hide a direct RadioGroup Item's passive children. Earlier providers
    /// keep them hidden and return Error(9) when opt-in is requested.
    pub fn decorate_content(&mut self, item: Node, enabled: bool) -> Result<&mut Self> {
        let raw = self.valid(item)?;
        if let Some(set) = setter(Setting::ItemContent) {
            check(unsafe { set(self.raw, raw, enabled as u32) })?;
        } else {
            if self.legacy_radio_node(raw)? != Some(LegacyRadioNode::Item) {
                return Err(Error(8));
            }
            if enabled {
                return Err(Error(9));
            }
        }
        Ok(self)
    }

    /// Set a copied hover/focus hint on a Radio or direct RadioGroup Item.
    /// Use up to 256 UTF-8 bytes with a delay from 0 to 10000 milliseconds.
    /// None or empty clears the hint and ignores the delay. Earlier providers
    /// accept clearing and return Error(9) when a hint is requested.
    pub fn radio_tooltip(
        &mut self,
        option: Node,
        text: Option<&str>,
        delay_ms: u32,
    ) -> Result<&mut Self> {
        let raw = self.valid(option)?;
        let text = text.filter(|s| !s.is_empty());
        if let Some(set) = tooltip_setter() {
            if text.is_some_and(|s| s.len() > 256) {
                return Err(Error(5));
            }
            let text = text.map(super::string).transpose()?;
            let pointer = text.as_ref().map_or(std::ptr::null(), |s| s.as_ptr());
            check(unsafe { set(self.raw, raw, pointer, delay_ms) })?;
        } else {
            if !matches!(
                self.legacy_radio_node(raw)?,
                Some(LegacyRadioNode::Standalone | LegacyRadioNode::Item)
            ) {
                return Err(Error(8));
            }
            if text.is_some() {
                return Err(Error(9));
            }
        }
        Ok(self)
    }
}
impl<'a> Builder<'a> {
    /// Set signed traversal order on a Radio or RadioGroup builder.
    pub fn tab_index(self, index: Option<i32>) -> Result<Self> {
        self.app.radio_tab_index(self.node, index)?;
        Ok(self)
    }

    pub fn animate_selection(self, enabled: bool) -> Result<Self> {
        self.app.animate_selection(self.node, enabled)?;
        Ok(self)
    }

    pub fn decorate_content(self, enabled: bool) -> Result<Self> {
        self.app.decorate_content(self.node, enabled)?;
        Ok(self)
    }

    pub fn radio_tooltip(self, text: Option<&str>, delay_ms: u32) -> Result<Self> {
        self.app.radio_tooltip(self.node, text, delay_ms)?;
        Ok(self)
    }
}
