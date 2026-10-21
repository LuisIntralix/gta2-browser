// The Controls screen (issue #2): change the game's twelve keys. The front end's Options item opens
// it (the original launched the separate GTA2 Manager program), and so does the toolbar's
// Controls button. It writes the same registry values GTA2 Manager wrote; the game reads them
// again at once, and the shell keeps them in this browser with the other settings.

// The game's control slots, in its order (hklm\...\gta2\control\0-11).
const ACTIONS = [
  'Forward', 'Back', 'Turn left', 'Turn right', 'Shoot / punch', 'Get in or out of a car', 'Jump / handbrake',
  'Previous weapon', 'Next weapon', 'Special 1 (horn, car extras)', 'Special 2', 'Status',
];

// Keys the browser version uses itself while playing, so the game never sees them.
const RESERVED = {
  KeyM: 'M opens the map', Tab: 'Tab shows the co-op scoreboard', KeyR: 'R turns the radio on and off',
  KeyT: 'T changes radio station (and opens the chat in co-op)', Minus: '− and = change the view distance',
  Equal: '− and = change the view distance', F5: 'F5 opens the Saves screen', F9: 'F9 opens the Saves screen',
  Escape: 'Esc opens the game menu',
};

const NAMES = {
  Space: 'Space', Enter: 'Enter', NumpadEnter: 'Keypad Enter', Backspace: 'Backspace', Tab: 'Tab', CapsLock: 'Caps Lock',
  ShiftLeft: 'Left Shift', ShiftRight: 'Right Shift', ControlLeft: 'Left Ctrl', ControlRight: 'Right Ctrl',
  AltLeft: 'Left Alt', AltRight: 'Right Alt', ArrowUp: '↑', ArrowDown: '↓', ArrowLeft: '←', ArrowRight: '→',
  Minus: '−', Equal: '=', BracketLeft: '[', BracketRight: ']', Semicolon: ';', Quote: "'", Backquote: '`',
  Backslash: '\\', Comma: ',', Period: '.', Slash: '/', PageUp: 'Page Up', PageDown: 'Page Down',
  NumpadMultiply: 'Keypad *', NumpadSubtract: 'Keypad −', NumpadAdd: 'Keypad +', NumpadDecimal: 'Keypad .',
  NumpadDivide: 'Keypad /', NumLock: 'Num Lock', ScrollLock: 'Scroll Lock',
};

function keyName(code) {
  if (NAMES[code]) return NAMES[code];
  return code.replace(/^Key/, '').replace(/^Digit/, '').replace(/^Numpad(\d)$/, 'Keypad $1');
}

const el = (tag, attrs = {}, ...children) => {
  const e = document.createElement(tag);
  for (const [k, v] of Object.entries(attrs)) {
    if (k === 'class') e.className = v;
    else if (k.startsWith('on')) e.addEventListener(k.slice(2), v);
    else e.setAttribute(k, v);
  }
  for (const c of children) if (c !== null && c !== undefined && c !== false) e.append(c);
  return e;
};

export class ControlsScreen {
  /**
   * @param {object} opts
   * @param {object} opts.Module the game module
   * @param {Record<string, [number, number]>} opts.keys KeyboardEvent.code -> [scan code, virtual key]
   * @param {number[]} opts.defaults the browser version's default scan code for each slot
   * @param {string} opts.regPath the game's control registry key
   * @param {() => void} opts.onChange keeps the settings in this browser
   * @param {(paused: boolean) => void} opts.pause pauses the game while the screen is open
   * @param {() => void} opts.onClose gives the keyboard back to the game
   */
  constructor({ Module, keys, defaults, regPath, onChange, pause, onClose }) {
    this.Module = Module;
    this.keys = keys;
    this.defaults = defaults;
    this.regPath = regPath;
    this.onChange = onChange;
    this.pause = pause;
    this.onClose = onClose;
    this.open = false;
    this.waiting = -1; // the slot waiting for a key
    this.message = '';
    this.setDword = Module.cwrap('gta2_port_reg_set_dword', null, ['string', 'string', 'number']);
    this.codeOf = new Map(Object.entries(keys).map(([code, [scan]]) => [scan, code]));
    this.root = el('div', { id: 'controls-screen', role: 'dialog', 'aria-modal': 'true', 'aria-label': 'Controls', hidden: '' });
    this.root.addEventListener('click', (e) => { if (e.target === this.root) this.close(); });
    document.body.append(this.root);
  }

  /** The current binding of each slot, from the game's registry (a value of 0x8000 or more is a game pad). */
  bindings() {
    const text = this.Module.ccall('gta2_port_reg_export', 'string', [], []);
    const out = this.defaults.slice();
    const path = this.regPath.toLowerCase();
    for (const line of text.split('\n')) {
      const [p, name, type, value] = line.split('|');
      if (type === 'd' && p && p.toLowerCase() === path && /^\d+$/.test(name) && Number(name) < out.length) {
        out[Number(name)] = Number(value) >>> 0;
      }
    }
    return out;
  }

  /**
   * A default key, as the controller presses it (web/gamepad.js sends the default controls), turned
   * into the key now bound to that action, so the controller follows the player's choices.
   */
  rebound(code) {
    const scan = this.keys[code] && this.keys[code][0];
    const slot = scan === undefined ? -1 : this.defaults.indexOf(scan);
    if (slot < 0) return code;
    const value = this.bindings()[slot];
    return (value < 0x8000 && this.codeOf.get(value)) || code;
  }

  set(slot, value) {
    this.setDword(this.regPath, String(slot), value);
  }

  apply() {
    this.Module.ccall('gta2_port_reload_controls', null, [], []);
    this.onChange();
  }

  show() {
    if (this.open) return;
    this.open = true;
    this.waiting = -1;
    this.message = '';
    this.pause(true);
    this.root.hidden = false;
    this.render();
  }

  close() {
    if (!this.open) return;
    this.open = false;
    this.waiting = -1;
    this.root.hidden = true;
    this.root.replaceChildren();
    this.pause(false);
    this.onClose();
  }

  /** While open, every key is the screen's: a new binding, Esc to cancel or close. */
  handleKey(e, down) {
    if (!this.open) return false;
    if (!down || e.repeat) return true;
    if (this.waiting < 0) {
      if (e.code === 'Escape') this.close();
      return true;
    }
    if (e.code === 'Escape') {
      this.waiting = -1;
      this.message = '';
    } else if (RESERVED[e.code]) {
      this.message = `${RESERVED[e.code]} in the browser version. Choose another key.`;
    } else if (!this.keys[e.code] || e.metaKey) {
      this.message = 'That key can’t be used. Choose another key.';
    } else {
      this.message = '';
      this.bind(this.waiting, this.keys[e.code][0]); // a swap says which action took the old key
      this.waiting = -1;
    }
    this.render();
    return true;
  }

  /** Gives a slot a key; a slot that had that key takes this slot's old one (a swap). */
  bind(slot, scan) {
    const current = this.bindings();
    const other = current.findIndex((v, i) => i !== slot && v === scan);
    if (other >= 0) {
      this.set(other, current[slot]);
      this.message = `${ACTIONS[other]} is now ${this.label(current[slot])}.`;
    }
    this.set(slot, scan);
    this.apply();
  }

  reset() {
    this.defaults.forEach((scan, slot) => this.set(slot, scan));
    this.apply();
    this.waiting = -1;
    this.message = 'Back to the default keys.';
    this.render();
  }

  label(value) {
    if (value >= 0x8000) return 'Game pad';
    const code = this.codeOf.get(value);
    return code ? keyName(code) : `Key ${value}`;
  }

  render() {
    if (!this.open) return;
    const current = this.bindings();
    const list = el('div', { class: 'controls-list' });
    current.forEach((value, slot) => {
      const waiting = this.waiting === slot;
      list.append(el('div', { class: 'controls-row' },
        el('span', {}, ACTIONS[slot]),
        el('button', {
          type: 'button', class: waiting ? 'waiting' : '', 'aria-label': `${ACTIONS[slot]}: ${this.label(value)}. Change`,
          onclick: () => { this.waiting = slot; this.message = ''; this.render(); },
        }, waiting ? 'Press a key…' : this.label(value))));
    });
    this.root.replaceChildren(el('div', { class: 'saves-panel controls-panel' },
      el('div', { class: 'saves-head' },
        el('h2', {}, 'Controls'),
        el('span', { class: 'saves-keys' }, 'Click an action, then press its new key. Esc cancels.'),
        el('button', { type: 'button', class: 'saves-close', 'aria-label': 'Close', onclick: () => this.close() }, '✕')),
      list,
      el('p', { class: 'saves-note', role: 'status' }, this.message || 'Menus always use the arrow keys, Enter and Esc.'),
      el('div', { class: 'controls-actions' },
        el('button', { type: 'button', onclick: () => this.reset() }, 'Default keys'),
        el('button', { type: 'button', class: 'primary', onclick: () => this.close() }, 'Done'))));
  }
}
