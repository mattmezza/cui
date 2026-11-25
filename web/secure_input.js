/* SPDX-License-Identifier: MIT */
/* Host-owned bridge: callbacks receive UTF-8 Uint8Array and UTF-8 byte offsets.
 * sync() must use the finalized Core focused configuration, never widget guesses.
 * The proxy contains only transient host input, never the committed model value. */
export function createSecureInputBridge({parent, text, composition, key, focusLost,
                                        changed, id = 'cui-secure-input'}) {
  const encoder = new TextEncoder();
  let current = null, disposed = false;
  const bytes = value => encoder.encode(value);
  const offset = (value, endpoint) => {
    let n = Math.max(0, Math.min(value.length, endpoint));
    if (n && n < value.length && /[\uD800-\uDBFF]/.test(value[n - 1]) && /[\uDC00-\uDFFF]/.test(value[n])) n--;
    return bytes(value.slice(0, n)).length;
  };
  function clear(element) { element.value = ''; element.setSelectionRange(0, 0); }
  function emit(session, kind, value = '', anchor = 0, cursor = anchor) {
    composition(kind, bytes(value), offset(value, anchor), offset(value, cursor),
                session.owner, session.generation, session.anchor);
    changed();
  }
  function reset(notify = true) {
    if (!current) return;
    const session = current;
    current = null;
    clear(session.element);
    session.element.remove();
    if (session.composing && notify) emit(session, 'cancel');
  }
  function create(config) {
    const element = document.createElement(config.multiline ? 'textarea' : 'input');
    const session = {element, owner: config.owner, generation: config.generation,
                     anchor: config.anchor ?? null, composing: false, ended: null, serial: 0};
    current = session;
    element.id = id;
    if (session.anchor) element.tabIndex = -1;
    if (!config.multiline) element.type = config.type === 1 ? 'password' : 'text';
    element.autocomplete = config.purpose === 2 ? 'new-password' : config.purpose === 1 ? 'current-password' : 'off';
    element.spellcheck = false; element.setAttribute('autocorrect', 'off'); element.setAttribute('autocapitalize', 'off');
    element.setAttribute('aria-label', config.label || 'Text input');
    Object.assign(element.style, {position: 'fixed', width: '1px', height: '1px', opacity: '0.01', padding: '0', border: '0'});
    if (session.anchor) session.anchor.after(element);
    else parent.append(element);
    const on = (name, callback) => element.addEventListener(name, event => {
      if (current === session) callback(event);
    });
    on('compositionstart', () => {
      session.composing = true; session.ended = null; session.serial++;
      emit(session, 'start');
    });
    on('compositionupdate', event => {
      if (!session.composing) {
        session.composing = true;
        emit(session, 'start');
      }
      const value = event.data || '', serial = ++session.serial;
      // Native browser proxy selection is UTF-16. Clamp to the current preedit.
      queueMicrotask(() => {
        if (current === session && session.composing && serial === session.serial)
          emit(session, 'update', value, element.selectionStart ?? value.length,
               element.selectionEnd ?? value.length);
      });
    });
    on('compositionend', event => {
      if (!session.composing) { clear(element); return; }
      const value = event.data || '';
      session.composing = false;
      const ended = value ? {value} : null;
      session.ended = ended;
      emit(session, value ? 'commit' : 'cancel', value);
      clear(element);
      // beforeinput/input following compositionend belong to that same commit.
      if (ended) setTimeout(() => {
        if (session.ended === ended) session.ended = null;
      }, 0);
    });
    on('beforeinput', event => {
      if (event.inputType === 'insertFromPaste') return; // Browser input event supplies transient text.
      if (!session.composing && session.ended) {
        const duplicate = event.data === session.ended.value ||
          (event.data == null && /Composition/.test(event.inputType));
        if (duplicate) {
          event.preventDefault();
          if (event.cancelable) session.ended = null;
        } else if (event.inputType === 'insertText') session.ended = null;
      }
    });
    on('input', event => {
      if (session.composing || event.isComposing) return;
      const value = event.data ?? element.value;
      if (session.ended && (value === session.ended.value || !value)) {
        session.ended = null; clear(element); return;
      }
      if (value) { text(bytes(value), session.owner, session.generation, session.anchor); changed(); }
      clear(element);
    });
    on('keydown', event => {
      if (session.composing || event.isComposing || event.keyCode === 229) return;
      if (key(event, session.owner)) { event.preventDefault(); changed(); }
    });
    for (const name of ['copy', 'cut']) on(name, event => {
      // A hidden proxy has no committed content. Deny native clipboard extraction
      // as well as forwarding the shortcut to the checked CUI reducer.
      if (config.type === 1) event.preventDefault();
    });
    on('blur', event => {
      if (session.composing) emit(session, 'cancel');
      session.composing = false; session.ended = null; clear(element);
      focusLost(session.owner, session.generation, event); changed();
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
      if (!current || current.owner !== config.owner || current.generation !== config.generation ||
          current.anchor !== (config.anchor ?? null)) {
        reset();
        create(config);
      }
      const element = current.element;
      if (rect) { element.style.left = `${rect.x}px`; element.style.top = `${rect.y}px`; element.style.height = `${Math.max(1, rect.h)}px`; }
      if (document.activeElement !== element) element.focus({preventScroll: true});
    },
    reset, dispose() { reset(); disposed = true; },
    get element() { return current?.element ?? null; }
  };
}
