// The front page (the game server's landing page, D21): play, play with
// friends, the controls on a drawn keyboard, options and help. Layout in the
// spirit of browser-game launchers such as Eaglercraft's; our own code and style.

import { LOAD_KEY, SAVE_KEY } from './savestate.js';

// What each key does, by group (colour). Matches PORT_CONTROLS in shell.js
// and the shell's own keys (map, scoreboard, view distance).
const GROUPS = {
  move: { label: 'Move', colour: '#f5c518' },
  fight: { label: 'Fight', colour: '#ff5c5c' },
  car: { label: 'Cars', colour: '#4cd07d' },
  weapon: { label: 'Weapons', colour: '#b38bff' },
  info: { label: 'Info', colour: '#4fc3ff' },
  menu: { label: 'Menus', colour: '#c9c9c9' },
};

export const CONTROLS = [
  { keys: ['W', 'S'], does: 'Forward / back', group: 'move' },
  { keys: ['A', 'D'], does: 'Turn left / right', group: 'move' },
  { keys: ['L'], does: 'Shoot / punch', group: 'fight' },
  { keys: ['Return'], label: 'Enter', does: 'Get in or out of a car', group: 'car' },
  { keys: ['Space'], does: 'Jump / handbrake', group: 'car' },
  { keys: ['J', 'K'], does: 'Previous / next weapon', group: 'weapon' },
  { keys: ['I', 'O'], does: 'Special (horn, car extras)', group: 'weapon' },
  { keys: ['R'], does: 'Radio on / off', group: 'car' },
  { keys: ['T'], does: 'Next radio station (in co-op T is chat)', group: 'car' },
  { keys: ['M'], does: 'Map, waypoints and your current job', group: 'info' },
  { keys: ['Tab'], does: 'Scoreboard (co-op)', group: 'info' },
  { keys: ['P'], does: 'Status', group: 'info' },
  { keys: ['-', '='], does: 'View distance', group: 'info' },
  { keys: ['Esc'], does: 'Game menu', group: 'menu' },
  { keys: [SAVE_KEY, LOAD_KEY], does: 'Saves: save (20% of your cash, at least $10,000) or load (single player)', group: 'menu' },
];

const ROWS = [
  ['Esc', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 'Delete'],
  ['Tab', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '[', ']', '\\'],
  ['Caps', 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', "'", 'Return'],
  ['Shift', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', ',', '.', '/', 'Shift'],
  ['Ctrl', 'Alt', 'Cmd', 'Space', 'Cmd', 'Alt', 'Ctrl'],
];
const WIDE = { Delete: 1.6, Tab: 1.5, '\\': 1.1, Caps: 1.8, Return: 1.9, Shift: 2.4, Space: 6.2, Cmd: 1.3, Ctrl: 1.3, Alt: 1.2 };

const el = (tag, attrs = {}, ...children) => {
  const e = document.createElement(tag);
  for (const [k, v] of Object.entries(attrs)) {
    if (k === 'class') e.className = v;
    else if (k === 'style') e.style.cssText = v;
    else e.setAttribute(k, v);
  }
  for (const c of children) e.append(c);
  return e;
};

function keyGroup(key) {
  const c = CONTROLS.find((ctl) => ctl.keys.includes(key));
  return c ? c.group : null;
}

/** A keyboard with the game's keys lit in their group's colour. */
function buildKeyboard(root) {
  const board = el('div', { class: 'kb', 'aria-hidden': 'true' });
  for (const row of ROWS) {
    const r = el('div', { class: 'kb-row' });
    for (const key of row) {
      const group = keyGroup(key);
      const k = el('span', { class: `kb-key${group ? ' lit' : ''}`, style: `flex:${WIDE[key] || 1}` }, key === 'Return' ? 'Enter' : key);
      if (group) k.style.setProperty('--c', GROUPS[group].colour);
      r.append(k);
    }
    board.append(r);
  }
  root.append(board);
}

/** The legend: each control with its keys as keycaps. */
function buildLegend(root) {
  const list = el('ul', { class: 'legend' });
  for (const c of CONTROLS) {
    const item = el('li');
    item.style.setProperty('--c', GROUPS[c.group].colour);
    item.append(el('span', { class: 'dot' }));
    const caps = el('span', { class: 'caps' });
    c.keys.forEach((k, i) => {
      if (i) caps.append(el('span', { class: 'sep' }, '/'));
      caps.append(el('kbd', {}, k === 'Return' ? 'Enter' : k));
    });
    item.append(caps, el('span', { class: 'does' }, c.does));
    list.append(item);
  }
  root.append(list);
}

// ---- options: saved in this browser, used when the game starts (the in-game buttons still work) ----

const OPTIONS = [
  { key: 'gta2-port-camera', label: 'Camera', hint: 'Modern stops the zoom pumping on every bump; Classic is the original.',
    choices: [['modern', 'Modern'], ['classic', 'Classic']], fallback: 'modern' },
  { key: 'gta2-port-view-distance', label: 'View distance', hint: 'How much of the city you see (Modern camera).',
    choices: [['0', 'Normal'], ['1', 'Far'], ['2', 'Farther'], ['3', 'Farthest']], fallback: '0' },
  { key: 'gta2-port-aspect', label: 'Screen shape', hint: 'Widescreen shows more of the city to the sides.',
    choices: [['16:9', '16:9'], ['auto', 'Fill window'], ['4:3', '4:3 original']], fallback: '16:9' },
  { key: 'gta2-port-smooth', label: 'Motion', hint: 'Smooth draws extra frames to match your screen (60–165 Hz).',
    choices: [['1', 'Smooth'], ['0', 'Original 30 fps']], fallback: '1' },
  { key: 'gta2-port-hud', label: 'HUD', hint: 'Where score, wanted level and messages sit on a wide screen.',
    choices: [['edges', 'Screen edges'], ['centre', 'Centred']], fallback: 'edges' },
  { key: 'gta2-port-minimap', label: 'Radar', hint: 'The round minimap in the corner.',
    choices: [['true', 'On'], ['false', 'Off']], fallback: 'true' },
  { key: 'gta2-port-show-fps', label: 'FPS counter', hint: 'Frames per second, bottom left.',
    choices: [['1', 'On'], ['0', 'Off']], fallback: '0' },
  { key: 'gta2-port-muted', label: 'Sound', hint: '',
    choices: [['0', 'On'], ['1', 'Muted']], fallback: '0' },
];

const read = (key) => { try { return localStorage.getItem(key); } catch { return null; } };
const write = (key, v) => { try { localStorage.setItem(key, v); } catch { /* storage unavailable */ } };

function buildOptions(root) {
  for (const o of OPTIONS) {
    const current = read(o.key) ?? o.fallback;
    const group = el('div', { class: 'seg', role: 'radiogroup', 'aria-label': o.label });
    for (const [value, text] of o.choices) {
      const b = el('button', { type: 'button', role: 'radio', 'aria-checked': String(value === current) }, text);
      b.addEventListener('click', () => {
        write(o.key, value);
        for (const other of group.children) other.setAttribute('aria-checked', String(other === b));
      });
      group.append(b);
    }
    root.append(el('div', { class: 'opt' }, el('div', { class: 'opt-label' }, el('strong', {}, o.label),
      o.hint ? el('small', {}, o.hint) : ''), group));
  }
}

/**
 * Shows the landing page.
 * @param {object} opts
 * @param {() => void} opts.onPlay single player
 * @param {() => void} opts.onCoop the co-op lobby
 * @param {{promise: Promise, fromCache: boolean}|null} opts.files the game files download, when it runs here
 */
export function showLanding({ onPlay, onCoop, files }) {
  document.body.classList.add('landing-on');
  document.getElementById('landing').hidden = false;
  document.getElementById('topnav').hidden = false;
  buildKeyboard(document.getElementById('kb-root'));
  buildLegend(document.getElementById('legend-root'));
  buildOptions(document.getElementById('options-root'));
  for (const b of document.querySelectorAll('[data-play]')) b.addEventListener('click', () => onPlay());
  for (const b of document.querySelectorAll('[data-coop]')) b.addEventListener('click', () => onCoop());
  const state = document.getElementById('files-state');
  const bar = document.getElementById('files-bar');
  if (files) {
    files.promise.then(() => {
      state.textContent = files.fromCache ? 'Ready: GTA2 is saved in this browser.' : 'Ready: downloaded, and kept for next time.';
      bar.value = 1;
      bar.hidden = true;
    }, () => { /* serverGameFiles() shows the error, with a retry */ });
  } else {
    // a static site, GTA2 not yet in this browser: Play gets it (once) from the Internet Archive
    state.textContent = 'Play downloads GTA2 once from the Internet Archive (free, 346 MB) and keeps it in this browser.';
    bar.hidden = true;
  }
}

/**
 * Continue: the newest save in this browser, as a card under Play.
 * @param {{where: string, how: string, thumb: Blob|null}} save
 * @param {() => void} onContinue
 */
export function showContinue(save, onContinue) {
  const root = document.getElementById('continue-root');
  if (!root) return;
  const button = el('button', { type: 'button', class: 'continue' });
  if (save.thumb) button.append(el('img', { src: URL.createObjectURL(save.thumb), alt: '' }));
  button.append(el('span', {}, el('b', {}, 'Continue'), el('strong', {}, save.where), el('small', {}, save.how)));
  button.addEventListener('click', () => { button.disabled = true; onContinue(); });
  root.replaceChildren(button);
  root.hidden = false;
}

/** Download progress for the landing page's status card. */
export function landingProgress(text, fraction, isError = false) {
  const state = document.getElementById('files-state');
  const bar = document.getElementById('files-bar');
  if (!state) return;
  state.textContent = text;
  state.style.color = isError ? '#ff8a8a' : '';
  bar.hidden = isError;
  bar.value = fraction;
}

/** Hides the landing page (the game is starting). */
export function hideLanding() {
  document.body.classList.remove('landing-on');
  document.getElementById('landing').hidden = true;
  document.getElementById('topnav').hidden = true;
}

/** The crew leaderboard (saved Gang Wars crews on this server). */
export function showLeaderboard(crews) {
  const root = document.getElementById('leaderboard-root');
  if (!root || !crews.length) return;
  const table = el('table', { class: 'board' });
  const head = table.insertRow();
  for (const h of ['#', 'Crew', 'Reputation', 'Wins', 'Kills', 'Deaths', 'Points']) head.append(el('th', {}, h));
  crews.slice(0, 20).forEach((c, i) => {
    const tr = table.insertRow();
    tr.insertCell().append(el('span', { class: 'rank' }, String(i + 1)));
    const name = tr.insertCell();
    name.append(el('div', { class: 'crew-name' }, c.name),
      el('div', { class: 'members' }, c.members.map((m) => m.name).join(', ') + (c.lastGang ? ` · last sided with ${c.lastGang}` : '')));
    tr.insertCell().append(el('span', { class: 'rep' }, String(c.reputation)));
    tr.insertCell().append(`${c.wins} / ${c.matches}`);
    tr.insertCell().append(String(c.kills));
    tr.insertCell().append(String(c.deaths));
    tr.insertCell().append(`$${c.points.toLocaleString()}`);
  });
  root.replaceChildren(table);
}
