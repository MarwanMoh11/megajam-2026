// Command-line solver for The Tight Edit: Final Cut.
// Runs the exact rule code from index.html (the <script id="rules"> block), so game and solver can't drift.
//   node solve.mjs             solve every scene: best with all tools, best without the scene's new tool, survive %,
//                              one-clip-at-a-time tuning, ways to get close; checks the stored best scores
//   node solve.mjs --check     also compare the solver's fast scoring with the frame-by-frame simulate() on random edits
//   node solve.mjs --tools     the tool experiments behind NOTES.md (undercrank, razor, jump cut)
//   node solve.mjs --json      machine-readable output
import { readFileSync } from 'node:fs';
import { fileURLToPath } from 'node:url';
import { dirname, join } from 'node:path';

const here = dirname(fileURLToPath(import.meta.url));
const html = readFileSync(join(here, 'index.html'), 'utf8');
const m = html.match(/<script id="rules">([\s\S]*?)<\/script>/);
if (!m) throw new Error('rules block not found in index.html');
const FC = new Function('module', m[1] + '\nreturn FinalCut;')(undefined);
const { CONFIG, LEVELS, solve, withoutTool, simulate, scoreOf, validEdit } = FC;
const args = new Set(process.argv.slice(2));
const samples = 150;

function sceneReport(L, i) {
  const t0 = Date.now();
  const r = solve(L, CONFIG, { samples });
  const without = {};
  const tools = L.tool === 'both' ? ['slow', 'layers'] : L.tool ? [L.tool] : [];
  for (const t of tools) {
    const q = solve(withoutTool(L, t), CONFIG, { tuners: false });
    without[t] = { best: q.best, edit: q.bestEdit };
  }
  return { i, id: L.id, ...r, without, ms: Date.now() - t0, storedBest: L.best, storedEdit: L.bestEdit };
}

const fmtEdit = (e) => `starts [${e.starts}]` + (e.tracks.some(t => t) ? ` tracks [${e.tracks.map(t => t ? 'TOP' : 'CAM')}]` : '') + (e.crank.length ? ` slow at ${e.crank}` : '');

function printScene(r) {
  const L = LEVELS[r.i];
  const lines = [
    `Scene ${r.i + 1}: ${L.name}  (${L.tool ? 'new tool: ' + L.tool : 'no new tool'})`,
    `  edits on the grid: ${r.valid.toLocaleString()}   survive (Studio Cut): ${r.survive.toLocaleString()} (${r.survivePct.toFixed(1)}%)   star timings: ${r.starTimings}`,
    `  best with all tools: ${r.best}  (${r.bestCount} edits)  e.g. ${fmtEdit(r.bestEdit)}`,
  ];
  for (const [t, w] of Object.entries(r.without)) lines.push(`  best without ${t}: ${w.best}${w.edit ? '  e.g. ' + fmtEdit(w.edit) : ''}   (the tool adds ${r.best - w.best})`);
  lines.push(
    `  Director's Cut threshold: ${r.dcThreshold}   edits at/above it: ${r.editsAtDC}   patterns there: ${r.patternsAtDC.join(' ')}`,
    `  distinct patterns within 80% of best (ways to get close): ${r.patternsClose}`,
    `  opening edit scores ${r.openingScore < 0 ? 'a failed take' : r.openingScore}; one-clip-at-a-time from it: ${r.greedyFromStart}; sliding from it: ${r.slideFromStart}`,
    `  from ${r.tunerSamples} random Studio Cuts: one-clip-at-a-time reaches DC ${r.greedyReachDCPct.toFixed(0)}% (best ${r.greedyReachBestPct.toFixed(0)}%, mean ${r.greedyMean.toFixed(0)}); sliding reaches DC ${r.slideReachDCPct.toFixed(0)}%`,
    `  stored best in index.html: ${r.storedBest} ${r.storedBest === r.best ? 'OK' : 'MISMATCH'}   (${r.ms} ms)`,
  );
  console.log(lines.join('\n'));
}

function check(n = 20000) {
  let bad = 0, tested = 0;
  let seed = 99;
  const rnd = () => (seed = (seed * 1103515245 + 12345) & 0x7fffffff) / 0x7fffffff;
  for (const L of LEVELS) {
    for (let k = 0; k < n; k++) {
      const e = {
        starts: L.clips.map(c => Math.floor(rnd() * (L.frames - c.len + 1))),
        tracks: L.clips.map(() => (L.layers && rnd() < 0.5 ? 1 : 0)),
        crank: (L.crank || []).map(c => Math.floor(rnd() * (L.frames - c.len + 1))),
      };
      if (!validEdit(L, e)) continue;
      tested++;
      const a = scoreOf(L, e, CONFIG), s = simulate(L, e, CONFIG, false), b = s.ok ? s.score : -1;
      if (a !== b) { bad++; if (bad < 5) console.log('MISMATCH', L.id, JSON.stringify(e), 'fast', a, 'simulate', b); }
    }
  }
  console.log(`check: ${tested.toLocaleString()} random valid edits, fast scoring vs simulate(): ${bad} mismatches`);
  return bad;
}

// ------------------------------------------------------------------ tool experiments (--tools)
function experiments() {
  const byId = Object.fromEntries(LEVELS.map(L => [L.id, L]));
  const best = (L) => solve(L, CONFIG, { tuners: false }).best;
  const out = [];
  // 1. UNDERCRANK (fast x2) in place of OVERCRANK on the slow scenes, and added to the no-tool scenes
  for (const id of ['overcrank', 'longway', 'final', 'corridor', 'wall']) {
    const L = byId[id];
    const base = best(withoutTool(L, 'slow'));
    const len = (L.crank && L.crank[0] ? L.crank[0].len : 18);
    const F = { ...L, id: id + '-fast', crank: [{ kind: 'fast', len, start: 0 }] };
    const S = { ...L, id: id + '-slow', crank: [{ kind: 'slow', len, start: 0 }] };
    const rf = solve(F, CONFIG, { samples: 60 });
    out.push(`undercrank  ${id.padEnd(11)} no crank ${base}  slow ${best(S)}  fast ${rf.best}  (fast: survive ${rf.survivePct.toFixed(0)}%, one-at-a-time reaches DC ${rf.greedyReachDCPct.toFixed(0)}%)`);
  }
  // 2. RAZOR: one cut splits a clip into two pieces that slide on their own (both arm the same trap)
  for (const id of ['corridor', 'wall', 'overcrank', 'superimpose']) {
    const L0 = withoutTool(withoutTool(byId[id], 'layers'), 'slow');
    const base = solve(L0, CONFIG, { samples: 60 });
    let bestCut = { best: -1 };
    L0.clips.forEach((c, k) => {
      for (const a of [Math.floor(c.len / 3), Math.floor(c.len / 2), Math.ceil(2 * c.len / 3)]) {
        const clips = L0.clips.filter((_, j) => j !== k).concat([{ trap: c.trap, len: a, start: 0 }, { trap: c.trap, len: c.len - a, start: 0 }]);
        const Lr = { ...L0, id: `${id}-razor-${k}-${a}`, clips };
        const r = solve(Lr, CONFIG, { samples: 30 });
        if (r.best > bestCut.best) bestCut = { ...r, k, a };
      }
    });
    out.push(`razor       ${id.padEnd(11)} without ${base.best} (one-at-a-time DC ${base.greedyReachDCPct.toFixed(0)}%)  with one cut ${bestCut.best} (clip ${bestCut.k + 1} cut at ${bestCut.a}; survive ${bestCut.survivePct.toFixed(0)}%, one-at-a-time reaches DC ${bestCut.greedyReachDCPct.toFixed(0)}%, ways to get close ${bestCut.patternsClose})`);
  }
  // 3. JUMP CUT: a one-frame clip on the STAR track that cuts CONFIG.jumpFrames of his run (he skips ahead)
  for (const id of ['wall', 'longway', 'superimpose']) {
    const L0 = withoutTool(withoutTool(byId[id], 'layers'), 'slow');
    const base = best(L0);
    const Lj = { ...L0, id: id + '-jump', crank: [{ kind: 'jump', len: 1, start: 0 }] };
    const r = solve(Lj, CONFIG, { samples: 60 });
    const skips = r.patternsAtDC.filter(p => p.includes('-')).length;
    out.push(`jump cut    ${id.padEnd(11)} without ${base}  with ${r.best}  survive ${r.survivePct.toFixed(0)}% (without: ${solve(L0, CONFIG, { tuners: false }).survivePct.toFixed(0)}%)  DC patterns ${r.patternsAtDC.length} (${skips} skip a trap)  one-at-a-time DC ${r.greedyReachDCPct.toFixed(0)}%`);
  }
  return out;
}

const reports = LEVELS.map(sceneReport);
let fail = reports.some(r => r.storedBest !== r.best);
if (args.has('--json')) {
  console.log(JSON.stringify(reports.map(r => ({ ...r, bestEdit: r.bestEdit })), null, 1));
} else {
  console.log(`CONFIG: fps ${CONFIG.fps}, star ${CONFIG.starSpeed} m/s, star ${CONFIG.starWidth} m, trap ${CONFIG.trapWidth} m, near miss < ${CONFIG.nearMissFrames} frames, DC slack ${CONFIG.dcSlack}, slow x${CONFIG.slowRate}\n`);
  reports.forEach(r => { printScene(r); console.log(''); });
}
if (args.has('--check')) fail = check() > 0 || fail;
if (args.has('--tools')) { console.log('\nTool experiments'); experiments().forEach(l => console.log(l)); }
if (fail) { console.log('\nFAIL: a stored best differs from the solver, or the fast path disagrees with simulate()'); process.exit(1); }
