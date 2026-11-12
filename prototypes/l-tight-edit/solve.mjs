// Command-line solver for The Tight Edit grey box.
// Runs the exact rule code from index.html (the <script id="rules"> block), so game and solver can't drift.
//   node solve.mjs              solve every level with the game's CONFIG
//   node solve.mjs --compare    same levels under each candidate coupling, plus random level geometries
import { readFileSync } from 'node:fs';
import { fileURLToPath } from 'node:url';
import { dirname, join } from 'node:path';

const here = dirname(fileURLToPath(import.meta.url));
const html = readFileSync(join(here, 'index.html'), 'utf8');
const m = html.match(/<script id="rules">([\s\S]*?)<\/script>/);
if (!m) throw new Error('rules block not found in index.html');
const TE = new Function(m[1] + '\nreturn TightEdit;')();
const { CONFIG, LEVELS, solve } = TE;

const fmt = (r) => [
  `${r.name}`,
  `  edits on the grid: ${r.valid.toLocaleString()}   survive (Studio Cut): ${r.survive.toLocaleString()} (${r.survivePct.toFixed(1)}%)`,
  `  best score: ${r.best} / 300   edits at best: ${r.bestCount}   best edit (clip start frames): [${r.bestEdit}]`,
  `  Director's Cut threshold: ${r.dcThreshold}   edits at/above it: ${r.editsAtDC}   patterns there: ${r.patternsAtDC.join(' ')}`,
  `  distinct patterns within 80% of best: ${r.patternsClose}`,
  `  one-clip-at-a-time tuning from the opening edit: ${r.greedyFromStart} [${r.greedyFromStartEdit}]`,
  `  one-clip-at-a-time from ${r.greedySamples} random Studio Cuts: reaches best ${r.greedyReachBestPct.toFixed(0)}%, reaches DC ${r.greedyReachDCPct.toFixed(0)}%, mean ${r.greedyMean.toFixed(0)}`,
  `  slide-toward-the-star tuning: from the opening edit ${r.slideFromStart} [${r.slideFromStartEdit}], reaches DC from random Studio Cuts ${r.slideReachDCPct.toFixed(0)}%`,
].join('\n');

const VARIANTS = {
  'independent (3 tracks)': (L) => [L, { oneTrack: false, starWaits: false }],
  'one track, no overlap': (L) => [L, { oneTrack: true, starWaits: false }],
  'one clip drives two (2 clips)': (L) => [{ ...L, clips: [{ ...L.clips[0], drives: [0, 2] }, L.clips[1]] }, { oneTrack: false, starWaits: false }],
  'star waits at closed crusher': (L) => [L, { oneTrack: false, starWaits: true }],
  'one track + star waits': (L) => [L, { oneTrack: true, starWaits: true }],
};

function randomLevel(seed) {
  let s = seed;
  const rnd = () => (s = (s * 1103515245 + 12345) & 0x7fffffff) / 0x7fffffff;
  const xs = [3 + rnd() * 3];
  xs.push(xs[0] + 3 + rnd() * 4);
  xs.push(xs[1] + 3 + rnd() * 4);
  const exitX = xs[2] + 4;
  const frames = Math.ceil(exitX / CONFIG.starSpeed * CONFIG.fps) + 18;
  const lens = [0, 1, 2].map(() => 18 + Math.floor(rnd() * 5) * 6);
  return { name: `random ${seed}`, frames, startX: 0, exitX, crushers: xs.map(x => ({ x })),
           clips: lens.map((len) => ({ len, start: 0 })) };
}

const args = process.argv.slice(2);
const t0 = Date.now();
if (args.includes('--search')) {
  // level search under the game's coupling: keep geometries that are easy to survive, hard to max one clip
  // at a time, and have several near-best patterns. Prints the top candidates as level data.
  const N = Number((args.find(a => a.startsWith('--n=')) || '--n=300').slice(4));
  const out = [];
  for (let i = 0; i < N; i++) {
    let s = 77 + i * 104729;
    const rnd = () => (s = (s * 1103515245 + 12345) & 0x7fffffff) / 0x7fffffff;
    const xs = [3 + Math.round(rnd() * 8) / 2];
    xs.push(xs[0] + 2.5 + Math.round(rnd() * 7) / 2);
    xs.push(xs[1] + 2.5 + Math.round(rnd() * 7) / 2);
    const exitX = xs[2] + 4;
    const frames = Math.ceil(exitX / CONFIG.starSpeed * CONFIG.fps) + 12 + Math.floor(rnd() * 4) * 6;
    const lens = [0, 1, 2].map(() => 18 + Math.floor(rnd() * 6) * 6);
    const L = { name: `cand ${i}`, frames, startX: 0, exitX, crushers: xs.map(x => ({ x })), clips: lens.map(len => ({ len, start: 0 })) };
    const r = solve(L, CONFIG, { samples: 60 });
    const minBest = Number((args.find(a => a.startsWith('--minbest=')) || '--minbest=1').slice(10));
    if (r.survivePct < 8 || r.best < minBest) continue;
    const q = (r.patternsAtDC.length >= 2) * 2 + Math.min(r.patternsClose, 6) / 2 + (r.greedyReachDCPct < 50) * 2 - (r.greedyReachDCPct < 5) * 1.5;
    out.push([q, r, L]);
  }
  out.sort((a, b) => b[0] - a[0]);
  for (const [q, r, L] of out.slice(0, 8)) {
    console.log(`q ${q.toFixed(1)}  x ${L.crushers.map(c => c.x)}  lens ${L.clips.map(c => c.len)}  frames ${L.frames}  ` +
      `survive ${r.survivePct.toFixed(1)}%  best ${r.best}  ways@DC ${r.patternsAtDC.join(' ')}  close ${r.patternsClose}  ` +
      `greedy DC ${r.greedyReachDCPct.toFixed(0)}%  best edit [${r.bestEdit}]`);
  }
} else if (args.includes('--compare')) {
  for (const L of LEVELS) {
    console.log(`\n=== ${L.name} ===`);
    for (const [vn, make] of Object.entries(VARIANTS)) {
      const [lv, rule] = make(L);
      const r = solve(lv, { ...CONFIG, ...rule }, { samples: 150 });
      console.log(`${vn.padEnd(32)} survive ${r.survivePct.toFixed(1).padStart(5)}%  best ${String(r.best).padStart(3)}  ` +
        `DC ${String(r.dcThreshold).padStart(3)}  ways@DC ${String(r.patternsAtDC.length).padStart(2)}  ` +
        `best-response: start ${String(r.greedyFromStart).padStart(3)}, DC ${r.greedyReachDCPct.toFixed(0).padStart(3)}%  slide: start ${String(r.slideFromStart).padStart(3)}, DC ${r.slideReachDCPct.toFixed(0).padStart(3)}%`);
    }
  }
  const N = Number((args.find(a => a.startsWith('--n=')) || '--n=20').slice(4));
  console.log(`\n=== ${N} random geometries (3 crushers 3-7 m apart, clips 18-42 frames) ===`);
  console.log('variant                          mean survive%  mean best  levels where best-response reaches DC <50% of the time  ...slide <50%  mean ways@DC');
  for (const [vn, make] of Object.entries(VARIANTS)) {
    let sv = 0, bs = 0, hard = 0, hardS = 0, ways = 0;
    for (let i = 0; i < N; i++) {
      const [lv, rule] = make(randomLevel(1000 + i * 7919));
      const r = solve(lv, { ...CONFIG, ...rule }, { samples: 60 });
      sv += r.survivePct; bs += r.best; ways += r.patternsAtDC.length;
      if (r.greedyReachDCPct < 50) hard++;
      if (r.slideReachDCPct < 50) hardS++;
    }
    console.log(`${vn.padEnd(32)} ${(sv / N).toFixed(1).padStart(12)}  ${(bs / N).toFixed(0).padStart(9)}  ${String(hard).padStart(3)} / ${N}${' '.repeat(40)}${String(hardS).padStart(3)} / ${N}  ${(ways / N).toFixed(1).padStart(10)}`);
  }
} else {
  for (const L of LEVELS) console.log(fmt(solve(L, CONFIG, { samples: 200 })) + '\n');
}
console.log(`(${((Date.now() - t0) / 1000).toFixed(1)} s, node ${process.version})`);
