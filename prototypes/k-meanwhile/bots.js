// Headless bots for the K grey box. Runs the exact <script id="rules"> block from index.html.
// Usage: node bots.js [runs=200] [json overrides for CONFIG]
//   node bots.js 300 '{"vStreak":true}'
const fs = require('fs'), vm = require('vm'), path = require('path');
const html = fs.readFileSync(path.join(__dirname, 'index.html'), 'utf8');
const src = html.match(/<script id="rules">([\s\S]*?)<\/script>/)[1];
const ctx = { Math, JSON, console, Date, Set };
vm.createContext(ctx);
vm.runInContext(src + '\n;this.K = { CONFIG, DEFAULTS, newGame, step, makeBot, BOT_PRESETS };', ctx);
const K = ctx.K;

const RUNS = +(process.argv[2] || 200);
const over = process.argv[3] ? JSON.parse(process.argv[3]) : {};
const DT = 1 / 60;

function runOne(threshold, seed, cfg, botOpt) {
  const s = K.newGame(cfg, seed);
  const bot = K.makeBot(threshold, seed * 7919 + 1, botOpt);
  s.cutAt = [];
  while (!s.over) for (const e of K.step(s, bot(s), DT)) if (e.type === 'cut') s.cutAt.push(e.danger);
  return s;
}
function stats(threshold, cfg, botOpt) {
  const res = [];
  for (let i = 0; i < RUNS; i++) res.push(runOne(threshold, 1000 + i, cfg, botOpt));
  const sc = res.map(s => s.score).sort((a, b) => a - b);
  const dead = res.filter(s => s.result === 'dead');
  const mean = a => a.reduce((x, y) => x + y, 0) / (a.length || 1);
  return {
    mean: mean(sc), p10: sc[Math.floor(RUNS * 0.1)], med: sc[Math.floor(RUNS / 2)], p90: sc[Math.floor(RUNS * 0.9)],
    death: dead.length / RUNS, deathT: mean(dead.map(s => s.t)), cuts: mean(res.map(s => s.cuts)),
    bonus: mean(res.map(s => s.bonusTotal)), kills: mean(res.map(s => s.kills)), jit: mean(res.map(s => s.tiers.jit)),
    secPerCut: mean(res.map(s => s.t / Math.max(1, s.cuts))),
    cutD: mean([].concat(...res.map(s => s.cutAt))),
  };
}
function table(cfg, rows, botOpt) {
  const out = ['policy           thresh   mean   p10   med   p90  death%  t_death  cuts  bonus  kills  JIT  s/cut  cut@'];
  for (const [name, th] of rows) {
    const r = stats(th, cfg, botOpt);
    out.push(
      name.padEnd(16) + String(th == null ? '-' : th).padStart(7) +
      [r.mean, r.p10, r.med, r.p90].map(v => Math.round(v).toString().padStart(6)).join('') +
      (Math.round(r.death * 100) + '%').padStart(8) + (r.deathT ? r.deathT.toFixed(0) + 's' : '-').padStart(9) +
      r.cuts.toFixed(1).padStart(6) + Math.round(r.bonus).toString().padStart(7) + Math.round(r.kills).toString().padStart(7) +
      r.jit.toFixed(1).padStart(5) + r.secPerCut.toFixed(1).padStart(7) + (r.cutD ? Math.round(r.cutD * 100) + '%' : '-').padStart(6));
  }
  return out.join('\n');
}

const cfg = Object.assign(JSON.parse(JSON.stringify(K.DEFAULTS)), over);
const rows = [['never', null], ['safe', 0.35], ['t=0.50', 0.5], ['t=0.65', 0.65], ['t=0.75', 0.75], ['t=0.80', 0.8],
  ['t=0.85', 0.85], ['greedy (last)', 0.9], ['t=0.93', 0.93], ['t=0.96', 0.96]];
console.log(`runs ${RUNS}  overrides ${JSON.stringify(over)}`);
console.log(table(cfg, rows, process.env.REACT ? { react: +process.env.REACT } : undefined));
