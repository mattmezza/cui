/* SPDX-License-Identifier: MIT */
/* Host-owned bridge: callbacks receive UTF-8 Uint8Array and UTF-8 byte offsets.
 * sync() must use the finalized Core focused configuration, never widget guesses.
 * The proxy contains only transient host input, never the committed model value. */
export function createSecureInputBridge({parent, text, composition, key, focusLost,
                                        changed, id = 'cui-secure-input'}) {
  const encoder = new TextEncoder();
  let proxy = null, owner = 0, generation = null, composing = false;
  let ended = null, disposed = false, resetting = false;
  let updateSerial = 0;
  const bytes = value => encoder.encode(value);
  const offset = (value, endpoint) => {
    let n = Math.max(0, Math.min(value.length, endpoint));
    if (n && n < value.length && /[\uD800-\uDBFF]/.test(value[n - 1]) && /[\uDC00-\uDFFF]/.test(value[n])) n--;
    return bytes(value.slice(0, n)).length;
  };
  function clear() { if (proxy) { proxy.value = ''; proxy.setSelectionRange(0, 0); } }
  function emit(kind, value = '', anchor = 0, cursor = anchor) {
    composition(kind, bytes(value), offset(value, anchor), offset(value, cursor), owner);
    changed();
  }
  function reset(notify = true) {
    if (composing && notify) emit('cancel');
    composing = false; ended = null; clear();
    if (proxy) { resetting = true; proxy.remove(); proxy = null; resetting = false; }
    owner = 0; generation = null;
  }
  function create(config) {
    proxy = document.createElement(config.multiline ? 'textarea' : 'input'); proxy.id = id;
    if (!config.multiline) proxy.type = config.type === 1 ? 'password' : 'text';
    proxy.autocomplete = config.purpose === 2 ? 'new-password' : config.purpose === 1 ? 'current-password' : 'off';
    proxy.spellcheck = false; proxy.setAttribute('autocorrect', 'off'); proxy.setAttribute('autocapitalize', 'off');
    proxy.setAttribute('aria-label', config.label || 'Text input');
    Object.assign(proxy.style, {position: 'fixed', width: '1px', height: '1px', opacity: '0.01', padding: '0', border: '0'});
    parent.append(proxy);
    proxy.addEventListener('compositionstart', () => { composing = true; ended = null; emit('start'); });
    proxy.addEventListener('compositionupdate', event => {
      if (!composing) { composing = true; emit('start'); }
      const value = event.data || '', serial = ++updateSerial;
      // Native browser proxy selection is UTF-16. Clamp to the current preedit.
      queueMicrotask(() => {
        if (composing && proxy && serial === updateSerial)
          emit('update', value, proxy.selectionStart ?? value.length, proxy.selectionEnd ?? value.length);
      });
    });
    proxy.addEventListener('compositionend', event => {
      if (!composing) { clear(); return; }
      const value = event.data || ''; composing = false;
      ended = value || null; emit(value ? 'commit' : 'cancel', value); clear();
      // beforeinput/input following compositionend belong to that same commit.
      setTimeout(() => { ended = null; }, 0);
    });
    proxy.addEventListener('beforeinput', event => {
      if (event.inputType === 'insertFromPaste') return; // Browser input event supplies transient text.
      if (!composing && ended !== null) {
        const duplicate = event.data === ended ||
          (event.data == null && /Composition/.test(event.inputType));
        if (duplicate) {
          event.preventDefault();
          if (event.cancelable) ended = null;
        } else if (event.inputType === 'insertText') ended = null;
      }
    });
    proxy.addEventListener('input', event => {
      if (composing || event.isComposing) return;
      const value = event.data ?? proxy.value;
      if (ended !== null && (value === ended || !value)) { ended = null; clear(); return; }
      if (value) { text(bytes(value), owner); changed(); }
      clear();
    });
    proxy.addEventListener('keydown', event => {
      if (composing || event.isComposing || event.keyCode === 229) return;
      if (key(event, owner)) { event.preventDefault(); changed(); }
    });
    for (const name of ['copy', 'cut']) proxy.addEventListener(name, event => {
      // A hidden proxy has no committed content. Deny native clipboard extraction
      // as well as forwarding the shortcut to the checked CUI reducer.
      if (config.type === 1) event.preventDefault();
    });
    proxy.addEventListener('blur', () => {
      if (resetting) return;
      if (composing) emit('cancel');
      composing = false; ended = null; clear(); focusLost(owner); changed();
    });
  }
  return {
    sync(config, rect) {
      if (disposed) return;
      if (!config || !config.owner) { reset(); return; }
      if (![0, 1, 2].includes(config.type) || ![0, 1, 2].includes(config.purpose) ||
          (config.multiline && (config.type !== 0 || config.purpose !== 0))) {
        reset(); throw new TypeError('Invalid finalized CUI text input configuration');
      }
      if (!proxy || owner !== config.owner || generation !== config.generation) {
        reset(); owner = config.owner; generation = config.generation; create(config);
      }
      if (rect) { proxy.style.left = `${rect.x}px`; proxy.style.top = `${rect.y}px`; proxy.style.height = `${Math.max(1, rect.h)}px`; }
      if (document.activeElement !== proxy) proxy.focus({preventScroll: true});
    },
    reset, dispose() { reset(); disposed = true; },
    get element() { return proxy; }
  };
}
