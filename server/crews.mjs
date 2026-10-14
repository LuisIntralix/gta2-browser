// Saved crews (Gang Wars, D21): every crew that has played, its members, stats
// and reputation, kept in one JSON file in the data directory. Writes go to a
// temporary file that replaces the old one, so a crash never leaves half a file.
import { mkdir, readFile, rename, writeFile } from 'node:fs/promises';
import { join } from 'node:path';

// Reputation earned in a Gang Wars match, per crew: a win is worth 50, each kill 5,
// each death -2, and every $10,000 of points 1. It never drops below 0.
export const REPUTATION = { win: 50, kill: 5, death: -2, perPoints: 10000 };

const MAX_CREWS = 500;
const MAX_MATCH_IDS = 500;

const cleanName = (name) => String(name || '').replace(/[^\p{L}\p{N} _.'-]/gu, '').trim().slice(0, 16);
const count = (n) => (Number.isFinite(n) ? Math.max(0, Math.min(1e9, Math.round(n))) : 0);

export class CrewStore {
  constructor(dir) {
    this.dir = dir;
    this.file = join(dir, 'crews.json');
    this.data = { crews: {}, matches: [] };
    this.writing = Promise.resolve();
    this.recent = new Map(); // match id -> its reputation changes, for the other players' reports
  }

  async load() {
    await mkdir(this.dir, { recursive: true });
    try {
      const data = JSON.parse(await readFile(this.file, 'utf8'));
      if (data && data.crews) this.data = { crews: data.crews, matches: data.matches || [] };
    } catch (err) {
      if (err.code !== 'ENOENT') console.error('crews: could not read', this.file, err.message);
    }
    return this;
  }

  save() {
    // one write at a time, each a complete file
    this.writing = this.writing.then(async () => {
      const tmp = `${this.file}.tmp`;
      await writeFile(tmp, JSON.stringify(this.data));
      await rename(tmp, this.file);
    }).catch((err) => console.error('crews: could not save', err.message));
    return this.writing;
  }

  /** The leaderboard: crews by reputation. */
  list() {
    return Object.values(this.data.crews)
      .map((c) => ({
        name: c.name, reputation: c.reputation, matches: c.matches, wins: c.wins, kills: c.kills, deaths: c.deaths,
        points: c.points, lastPlayed: c.lastPlayed, lastGang: c.lastGang || '',
        members: Object.entries(c.members).map(([name, m]) => ({ name, ...m }))
          .sort((a, b) => b.points - a.points).slice(0, 12),
      }))
      .sort((a, b) => b.reputation - a.reputation || b.wins - a.wins || b.points - a.points)
      .slice(0, 100);
  }

  /**
   * Records a finished Gang Wars match (once per match id, whoever reports it).
   * @returns {{ok: boolean, error?: string, changes?: {name: string, reputation: number, delta: number}[]}}
   */
  async record(match) {
    if (!match || typeof match.id !== 'string' || match.id.length > 64 || match.mode !== 'wars') return { ok: false, error: 'not a Gang Wars match' };
    if (this.data.matches.includes(match.id)) return { ok: true, duplicate: true, changes: this.recent.get(match.id) || [] };
    const crews = Array.isArray(match.crews) ? match.crews.slice(0, 3) : [];
    if (crews.length < 2) return { ok: false, error: 'a Gang War needs two crews' };
    const now = new Date().toISOString();
    const changes = [];
    for (const raw of crews) {
      const name = cleanName(raw.name);
      if (!name) continue;
      const key = name.toLowerCase();
      if (!this.data.crews[key]) {
        if (Object.keys(this.data.crews).length >= MAX_CREWS) continue;
        this.data.crews[key] = { name, reputation: 0, matches: 0, wins: 0, kills: 0, deaths: 0, points: 0, members: {}, createdAt: now };
      }
      const c = this.data.crews[key];
      const members = (Array.isArray(raw.members) ? raw.members : []).slice(0, 6);
      const kills = members.reduce((n, m) => n + count(m.kills), 0);
      const deaths = members.reduce((n, m) => n + count(m.deaths), 0);
      const points = members.reduce((n, m) => n + count(m.points), 0);
      const won = !!raw.won;
      const delta = (won ? REPUTATION.win : 0) + kills * REPUTATION.kill + deaths * REPUTATION.death +
        Math.floor(points / REPUTATION.perPoints);
      c.reputation = Math.max(0, c.reputation + delta);
      c.matches += 1;
      c.wins += won ? 1 : 0;
      c.kills += kills;
      c.deaths += deaths;
      c.points += points;
      c.lastPlayed = now;
      c.lastGang = cleanName(raw.gang);
      for (const m of members) {
        const who = cleanName(m.name);
        if (!who) continue;
        const rec = c.members[who] || { matches: 0, kills: 0, deaths: 0, points: 0 };
        rec.matches += 1;
        rec.kills += count(m.kills);
        rec.deaths += count(m.deaths);
        rec.points += count(m.points);
        rec.lastPlayed = now;
        c.members[who] = rec;
      }
      changes.push({ name: c.name, reputation: c.reputation, delta });
    }
    this.data.matches = [...this.data.matches, match.id].slice(-MAX_MATCH_IDS);
    this.recent.set(match.id, changes);
    if (this.recent.size > 100) this.recent.delete(this.recent.keys().next().value);
    await this.save();
    return { ok: true, changes };
  }
}
