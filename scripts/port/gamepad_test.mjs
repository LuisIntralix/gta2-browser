// Unit tests for web/gamepad.js: node --test scripts/port/gamepad_test.mjs
import assert from 'node:assert/strict';
import { test } from 'node:test';
import { GamepadInput, PLAY, codesFor, padName, pressedButtons } from '../../web/gamepad.js';
import { PAD_CONTROLS } from '../../web/landing.js';

globalThis.document = { hasFocus: () => true };

function fakePad({ pressed = [], axes = [0, 0, 0, 0] } = {}) {
  const buttons = Array.from({ length: 17 }, (_, i) => ({ pressed: pressed.includes(i) }));
  return { connected: true, mapping: 'standard', buttons, axes };
}

function harness(playing = true) {
  const events = [];
  const state = { pad: fakePad(), playing };
  const input = new GamepadInput({
    send: (code, down) => events.push(`${down ? '+' : '-'}${code}`),
    isPlaying: () => state.playing,
    getPads: () => [state.pad],
  });
  return { input, events, state };
}

test('the left stick counts as the d-pad past the dead zone', () => {
  assert.deepEqual([...pressedButtons(fakePad({ axes: [0.2, -0.9] }))], ['up']);
  assert.deepEqual([...pressedButtons(fakePad({ axes: [0.8, 0.5] }))].sort(), ['down', 'right']);
});

test('the same button maps to a different key in menus', () => {
  assert.deepEqual([...codesFor(['cross'], true)], ['KeyL']);
  assert.deepEqual([...codesFor(['cross'], false)], ['Enter']);
});

test('R2 and stick up together press W once and release it once', () => {
  const { input, events, state } = harness();
  input.poll();
  state.pad = fakePad({ pressed: [7], axes: [0, -1] });
  input.poll();
  state.pad = fakePad({ pressed: [7] });
  input.poll();
  state.pad = fakePad();
  input.poll();
  assert.deepEqual(events, ['+KeyW', '-KeyW']);
});

test('a button held across a context change waits for its release', () => {
  const { input, events, state } = harness(false);
  input.poll();
  state.pad = fakePad({ pressed: [0] });
  input.poll(); // Cross confirms the menu
  state.playing = true;
  input.poll(); // the game starts with Cross still held: no shot
  state.pad = fakePad();
  input.poll();
  state.pad = fakePad({ pressed: [0] });
  input.poll(); // a new press shoots
  assert.deepEqual(events, ['+Enter', '-Enter', '+KeyL']);
});

test('releaseAll lets go of every held key', () => {
  const { input, events, state } = harness();
  state.pad = fakePad({ pressed: [0, 4] });
  input.poll();
  input.releaseAll();
  assert.deepEqual(events, ['+KeyL', '+KeyJ', '-KeyL', '-KeyJ']);
});

test('the controller legend on the front page lists every mapped button', () => {
  const directions = ['up', 'down', 'left', 'right'];
  const shown = new Set(PAD_CONTROLS.flatMap((c) => c.keys).flatMap((k) => (k === 'stick' || k === 'dpad' ? directions : [k])));
  assert.deepEqual([...shown].sort(), Object.keys(PLAY).sort());
});

test('padName drops the ids Chrome and Firefox add', () => {
  assert.equal(padName({ id: 'DualSense Wireless Controller (STANDARD GAMEPAD Vendor: 054c Product: 0ce6)' }), 'DualSense Wireless Controller');
  assert.equal(padName({ id: '054c-0ce6-DualSense Wireless Controller' }), 'DualSense Wireless Controller');
  assert.equal(padName({ id: '' }), 'Controller');
});
