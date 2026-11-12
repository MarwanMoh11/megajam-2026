// Headless bots for the K 3D prototype. Runs the exact <script id="rules"> block from index.html.
// Usage: node bots.js [runs=100] [json overrides for CONFIG]
//   node bots.js 200 '{"bruteHp":20}'
const fs = require('fs'), vm = require('vm'), path = require('path');
const html = fs.readFileSync(process.env.KHTML || path.join(__dirname, 'index.html'), 'utf8');
const src = html.match(/<script id="rules">([\s\S]*?)<\/script>/)[1];
const ctx = {};
vm.createContext(ctx);
vm.runInContext(src + '\n;this.K = { CONFIG, newGame, step, makeBot, HEROES };', ctx);
const K = ctx.K;

const RUNS = +(process.argv[2] || 100);
const cfg = Object.assign({}, K.CONFIG, process.argv[3] ? JSON.parse(process.argv[3]) : {});
const DT = 1 / 60;

function runOne(kind, seed) {
  const s = K.newGame(cfg, seed), bot = K.makeBot(kind, seed * 7919 + 1);
  let steps = 0;
  const cutHp = [], actEnds = [];
  while (s.phase !== 'end' && steps < 60 * 900) {
    for (const e of K.step(s, bot(s, DT), DT)) {
      if (e.type === 'cut') cutHp.push(e.hp);
      if (e.type === 'actEnd') actEnds.push(+s.t.toFixed(0));
    }
    steps++;
  }
  return { s, wall: steps * DT, cutHp, actEnds };
}
const mean = a => a.reduce((x, y) => x + y, 0) / (a.length || 1);
function row(kind) {
  const res = [];
  for (let i = 0; i < RUNS; i++) res.push(runOne(kind, 1000 + i));
  const ended = res.filter(r => r.s.phase === 'end');
  const surv = [0, 0, 0, 0];
  for (const r of ended) surv[r.s.ending.survivors.length]++;
  const deaths = { VERA: 0, MOSS: 0, JUNO: 0 };
  const deathAct = [0, 0, 0];
  for (const r of res) for (const k of r.s.killedOff) { deaths[K.HEROES[k.id].name]++; deathAct[k.act]++; }
  const sc = res.map(r => r.s.score).sort((a, b) => a - b);
  const stars = [0, 0, 0, 0, 0, 0]; for (const r of ended) stars[r.s.ending.stars]++;
  return {
    kind, ended: ended.length / RUNS, wall: mean(res.map(r => r.wall)), film: mean(res.map(r => r.s.t)),
    surv, deaths, deathAct, score: mean(sc), p10: sc[Math.floor(RUNS * 0.1)], med: sc[Math.floor(RUNS / 2)], p90: sc[Math.floor(RUNS * 0.9)],
    style: mean(res.map(r => r.s.styleScore)), bonus: mean(res.map(r => r.s.bonusTotal)), cuts: mean(res.map(r => r.s.cuts)),
    jit: mean(res.map(r => r.s.tiers.jit)), close: mean(res.map(r => r.s.tiers.close)), tense: mean(res.map(r => r.s.tiers.tense)),
    cutHp: mean([].concat(...res.map(r => r.cutHp))), stars, off: mean(res.map(r => r.s.offT)), heads: mean(res.map(r => r.s.heads)), act1: mean(res.map(r => r.actEnds[0] || 0)), act2: mean(res.map(r => r.actEnds[1] || 0)),
  };
}
console.log(`${RUNS} runs per policy${process.argv[3] ? ' with ' + process.argv[3] : ''}`);
console.log('policy     ended  wall(s) film(s) act1@ act2@  survivors 0/1/2/3     deaths V/M/J    by act 1/2/3   mean   p10    med    p90   style  bonus cuts  JIT close tense cut@hp  stars 0..5');
for (const k of ['director', 'greedy', 'script', 'idle']) {
  const r = row(k), f = (v, w, d) => (d == null ? String(Math.round(v)) : v.toFixed(d)).padStart(w);
  console.log(k.padEnd(9) + f(r.ended * 100, 5) + '%' + f(r.wall, 8) + f(r.film, 8) + f(r.act1, 6) + f(r.act2, 6) + '  ' +
    r.surv.map(v => String(v).padStart(4)).join('') + '   ' + Object.values(r.deaths).map(v => String(v).padStart(4)).join('') + '   ' +
    r.deathAct.map(v => String(v).padStart(4)).join('') + f(r.score, 7) + f(r.p10, 7) + f(r.med, 7) + f(r.p90, 7) + f(r.style, 7) + f(r.bonus, 7) +
    f(r.cuts, 5, 1) + f(r.jit, 5, 1) + f(r.close, 5, 1) + f(r.tense, 5, 1) + f(r.cutHp, 7, 2) + f(r.off, 7) + f(r.heads, 6) + '   ' + r.stars.join('/'));
}
