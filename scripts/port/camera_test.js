// Camera acceptance test (decision D17) for the browser port, run in the page.
//
// Load the game with ?devdata=1&fastloop=1&fresh=1&autostart=1, wait for the
// church start, paste this file into the console (or inject it), then:
//
//   await gta2CameraTest({ seconds: 90 })
//
// It finds the longest straight road, drives it with a random mix of
// gas / short brakes / coasting / lane wiggles (reversing out of crashes), and
// records per presented frame the canonical camera height (what Classic draws)
// and the drawn height (Modern, when enabled). Both are scored on the same
// drive: direction reversals per minute (0.1-block hysteresis), peak height
// rate and total height travel, plus each Modern descent with how long the
// canonical height had been lower before it started (D17: never < 1.5 s,
// except the 3-block ceiling after a crash).
window.gta2CameraTest = async ({ seconds = 90, hysteresis = 0.1 } = {}) => {
  const d = window.gta2.dev;
  const R = window.gta2.renderer;
  const rec = [];
  let recording = false;
  const t0 = performance.now();
  if (!R.__origPresent) R.__origPresent = R.present.bind(R);
  R.present = (...a) => {
    R.__origPresent(...a);
    if (!recording) return;
    const c = d.camera();
    const s = d.state();
    rec.push([performance.now() - t0, c.z, c.drawnZ, s.speed]);
  };

  const road = d.findRoad();
  if (!road.length) throw new Error('no straight road found');
  const along = (s) => (road.alongY ? s.y : s.x);
  const startAt = (offset) => (road.alongY ? d.teleport(road.x, road.y + offset) : d.teleport(road.x + offset, road.y));
  const rot = road.alongY ? 0 : 360; // Ang16: direction (sin a, cos a): 0 drives +y, 360 drives +x
  const offsets = [10, 30, 50, 20, 40].filter((o) => o < road.length - 20);
  const deadline = performance.now() + seconds * 1000;
  let run = 0;
  while (performance.now() < deadline) {
    if (d.state().inCar) { recording = false; await d.hold('Enter', 150); await d.sleep(2500); }
    startAt(offsets[run++ % offsets.length]);
    await d.sleep(1000);
    d.spawnCar(10, road.alongY ? 1 : 0, road.alongY ? 0 : 1, rot);
    await d.sleep(300);
    await d.hold('Enter', 150);
    for (let i = 0; i < 40 && !d.state().inCar; i++) await d.sleep(250);
    if (!d.state().inCar) continue;
    recording = true;
    const runEnd = performance.now() + 20000;
    const endOfRoad = (road.alongY ? road.y : road.x) + road.length - 6;
    let still = 0;
    while (performance.now() < Math.min(runEnd, deadline) && along(d.state()) < endOfRoad) {
      const r = Math.random();
      if (r < 0.15) await d.hold('KeyS', 250 + Math.random() * 300);
      else if (r < 0.3) await d.sleep(300 + Math.random() * 400);
      else if (r < 0.45) { d.hold('KeyW', 150); await d.hold(Math.random() < 0.5 ? 'KeyA' : 'KeyD', 100 + Math.random() * 100); }
      else await d.hold('KeyW', 600 + Math.random() * 900);
      if (d.state().speed < 0.02 && ++still > 2) {
        await d.hold('KeyS', 700);
        d.hold('KeyS', 300);
        await d.hold(Math.random() < 0.5 ? 'KeyA' : 'KeyD', 300);
        still = 0;
      } else if (d.state().speed >= 0.02) still = 0;
    }
  }
  recording = false;
  R.present = R.__origPresent;
  return gta2CameraScore(rec, hysteresis);
};

window.gta2CameraScore = (rec, H = 0.1) => {
  let ms = 0;
  for (let i = 1; i < rec.length; i++) if (rec[i][0] - rec[i - 1][0] < 500) ms += rec[i][0] - rec[i - 1][0];
  const mins = ms / 60000;
  const score = (idx) => {
    let rev = 0, dir = 0, ext = rec[0][idx], peak = 0, travel = 0;
    for (let i = 1; i < rec.length; i++) {
      const dt = (rec[i][0] - rec[i - 1][0]) / 1000;
      const z = rec[i][idx];
      if (dt > 0.5) { dir = 0; ext = z; continue; }
      travel += Math.abs(z - rec[i - 1][idx]);
      if (dt > 0) peak = Math.max(peak, Math.abs(z - rec[i - 1][idx]) / dt);
      if (dir === 0) { if (z > ext + H) { dir = 1; ext = z; } else if (z < ext - H) { dir = -1; ext = z; } }
      else if (dir > 0) { if (z > ext) ext = z; else if (z < ext - H) { rev++; dir = -1; ext = z; } }
      else { if (z < ext) ext = z; else if (z > ext + H) { rev++; dir = 1; ext = z; } }
    }
    return { reversalsPerMin: +(rev / mins).toFixed(1), peakRate: +peak.toFixed(2), travelPerMin: +(travel / mins).toFixed(1) };
  };
  // Modern descents: how long the canonical height had been > 0.3 below first.
  const descents = [];
  let down = false;
  for (let i = 2; i < rec.length; i++) {
    if (rec[i][0] - rec[i - 1][0] > 500) { down = false; continue; }
    const dz = rec[i][2] - rec[i - 1][2];
    if (!down && dz < -0.01 && rec[i][2] - rec[i][1] > 0.3) {
      down = true;
      let j = i - 1;
      while (j > 0 && rec[j][1] < rec[j][2] - 0.3 && rec[j][0] - rec[j - 1][0] < 500) j--;
      descents.push({ atMs: Math.round(rec[i][0]), canonicalLowForMs: Math.round(rec[i][0] - rec[j][0]), gap: +(rec[i][2] - rec[i][1]).toFixed(2) });
    }
    if (down && dz >= 0) down = false;
  }
  return { minutes: +mins.toFixed(2), samples: rec.length, classic: score(1), modern: score(2), modernDescents: descents };
};
