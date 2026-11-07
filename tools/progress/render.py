#!/usr/bin/env python3
"""Render a static progress site from an objdiff report."""
import argparse
import datetime
import glob
import json
import os
import shutil
import subprocess

REPO_URL = "https://github.com/frioux/xenogears-decomp"


def num(m, key):
    return float(m.get(key, 0) or 0)


def overall(m):
    return {
        "pct": num(m, "matched_code_percent"),
        "fuzzy": num(m, "fuzzy_match_percent"),
        "total_code": int(num(m, "total_code")),
        "matched_code": int(num(m, "matched_code")),
        "total_functions": int(num(m, "total_functions")),
        "matched_functions": int(num(m, "matched_functions")),
    }


def compact_unit(u):
    cats = u.get("metadata", {}).get("progress_categories", [])
    funcs = []
    for f in u.get("functions", []):
        funcs.append([f["name"], int(f.get("size", 0) or 0), round(float(f.get("fuzzy_match_percent", 0) or 0), 1)])
    funcs.sort(key=lambda f: (f[2] >= 100, f[2], -f[1]))
    return {"name": u["name"], "cat": cats[0] if cats else "", "m": overall(u.get("measures", {})), "f": funcs}


def short_sha(commit):
    return commit[:7]


def git_head():
    try:
        return subprocess.check_output(["git", "rev-parse", "HEAD"], text=True, stderr=subprocess.DEVNULL).strip()
    except (OSError, subprocess.CalledProcessError):
        return "unknown"


def badge_svg(label, value, color):
    def w(s):
        return int(len(s) * 6.6 + 10)
    lw, vw = w(label), w(value)
    tw = lw + vw
    return f'''<svg xmlns="http://www.w3.org/2000/svg" width="{tw}" height="20" role="img" aria-label="{label}: {value}">
<linearGradient id="s" x2="0" y2="100%"><stop offset="0" stop-color="#bbb" stop-opacity=".1"/><stop offset="1" stop-opacity=".1"/></linearGradient>
<clipPath id="r"><rect width="{tw}" height="20" rx="3" fill="#fff"/></clipPath>
<g clip-path="url(#r)"><rect width="{lw}" height="20" fill="#555"/><rect x="{lw}" width="{vw}" height="20" fill="{color}"/><rect width="{tw}" height="20" fill="url(#s)"/></g>
<g fill="#fff" text-anchor="middle" font-family="Verdana,Geneva,DejaVu Sans,sans-serif" font-size="11">
<text x="{lw / 2}" y="15" fill="#010101" fill-opacity=".3">{label}</text><text x="{lw / 2}" y="14">{label}</text>
<text x="{lw + vw / 2}" y="15" fill="#010101" fill-opacity=".3">{value}</text><text x="{lw + vw / 2}" y="14">{value}</text></g></svg>
'''


def badge_color(pct):
    if pct >= 80:
        return "#4c1"
    if pct >= 50:
        return "#dfb317"
    if pct >= 25:
        return "#fe7d37"
    return "#e05d44"


def write_history(hist_dir, snap):
    os.makedirs(hist_dir, exist_ok=True)
    name = f"{snap['date'][:19].replace(':', '-')}-{short_sha(snap['commit'])}.json"
    with open(os.path.join(hist_dir, name), "w") as fh:
        json.dump(snap, fh, separators=(",", ":"))
    snaps = []
    for p in glob.glob(os.path.join(hist_dir, "*.json")):
        try:
            with open(p) as fh:
                s = json.load(fh)
            snaps.append({"date": s["date"], "commit": s["commit"], "pct": float(s["measures"].get("matched_code_percent", 0) or 0)})
        except (ValueError, KeyError, OSError):
            continue
    snaps.sort(key=lambda s: s["date"])
    return snaps


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--report", required=True)
    ap.add_argument("--out", required=True)
    ap.add_argument("--history")
    ap.add_argument("--commit")
    ap.add_argument("--date")
    args = ap.parse_args()

    with open(args.report) as fh:
        report = json.load(fh)
    commit = args.commit or git_head()
    date = args.date or datetime.datetime.now(datetime.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
    total = overall(report["measures"])

    history = []
    if args.history:
        snap = {
            "date": date,
            "commit": commit,
            "measures": report["measures"],
            "categories": {c["id"]: c["measures"] for c in report.get("categories", [])},
        }
        history = write_history(args.history, snap)

    cat_names = {c["id"]: c["name"] for c in report.get("categories", [])}
    data = {
        "commit": commit,
        "short": short_sha(commit),
        "date": date,
        "repo": REPO_URL,
        "total": total,
        "categories": [{"id": c["id"], "name": c["name"], "m": overall(c["measures"])} for c in report.get("categories", [])],
        "units": [compact_unit(u) for u in report["units"]],
        "history": history,
    }
    blob = json.dumps(data, separators=(",", ":")).replace("</", "<\\/")
    page = TEMPLATE.replace("/*DATA*/", blob)

    os.makedirs(args.out, exist_ok=True)
    with open(os.path.join(args.out, "index.html"), "w") as fh:
        fh.write(page)
    shutil.copyfile(args.report, os.path.join(args.out, "progress.json"))
    pct = f"{total['pct']:.1f}%"
    with open(os.path.join(args.out, "badge.svg"), "w") as fh:
        fh.write(badge_svg("code", pct, badge_color(total["pct"])))
    print(f"rendered {args.out} ({pct}, {short_sha(commit)}, {len(history)} history points)")


TEMPLATE = r"""<!doctype html>
<html lang="en"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Xenogears Decompilation Progress</title>
<style>
:root{--bg:#0e1116;--panel:#161b22;--line:#262d36;--fg:#e6edf3;--dim:#8b949e;--acc:#58a6ff;--good:#3fb950;--mid:#d29922;--bad:#f85149}
*{box-sizing:border-box}
body{margin:0;background:var(--bg);color:var(--fg);font:15px/1.5 -apple-system,BlinkMacSystemFont,"Segoe UI",Roboto,sans-serif}
main{max-width:1000px;margin:0 auto;padding:24px 16px 64px}
a{color:var(--acc);text-decoration:none}a:hover{text-decoration:underline}
h1{font-size:1.4rem;margin:0 0 4px}h2{font-size:1.05rem;margin:32px 0 12px;color:var(--dim);text-transform:uppercase;letter-spacing:.06em;font-weight:600}
.sub{color:var(--dim);font-size:.9rem}
.hero{display:flex;flex-wrap:wrap;align-items:baseline;gap:8px 32px;margin-top:20px;padding:20px;background:var(--panel);border:1px solid var(--line);border-radius:10px}
.big{font-size:3.4rem;font-weight:700;line-height:1;font-variant-numeric:tabular-nums}
.stat{color:var(--dim)}.stat b{color:var(--fg);font-variant-numeric:tabular-nums}
.bar{margin:0 0 14px}.bar .row{display:flex;justify-content:space-between;gap:12px;flex-wrap:wrap;font-size:.9rem}
.bar .meta{color:var(--dim);font-variant-numeric:tabular-nums}
.track{height:10px;background:var(--line);border-radius:5px;overflow:hidden;margin-top:4px}
.fill{height:100%;background:var(--good)}
.panel{background:var(--panel);border:1px solid var(--line);border-radius:10px;padding:12px}
svg.chart{width:100%;height:auto;display:block}
.controls{margin-bottom:10px}
input{width:100%;padding:8px 10px;background:var(--panel);color:var(--fg);border:1px solid var(--line);border-radius:6px;font:inherit}
.scroll{overflow-x:auto}
table{width:100%;border-collapse:collapse;font-size:.88rem;font-variant-numeric:tabular-nums}
th,td{padding:6px 10px;text-align:right;white-space:nowrap}
th:first-child,td:first-child,th:nth-child(2),td:nth-child(2){text-align:left}
th{cursor:pointer;color:var(--dim);font-weight:600;border-bottom:1px solid var(--line);user-select:none;position:sticky;top:0;background:var(--bg)}
th.on{color:var(--fg)}
tr.u{cursor:pointer;border-bottom:1px solid var(--line)}tr.u:hover{filter:brightness(1.3)}
tr.u td:first-child{white-space:normal;word-break:break-all}
.p0{background:rgba(248,81,73,.12)}.p1{background:rgba(210,153,34,.12)}.p2{background:rgba(63,185,80,.10)}.p3{background:rgba(63,185,80,.22)}
tr.fl td{padding:0 10px 10px;white-space:normal;text-align:left}
.fn{display:grid;grid-template-columns:repeat(auto-fill,minmax(260px,1fr));gap:1px 16px;font:.78rem/1.5 ui-monospace,Menlo,Consolas,monospace}
.fn div{display:flex;justify-content:space-between;gap:8px}.fn span:first-child{overflow:hidden;text-overflow:ellipsis}
.fn .m{color:var(--good)}.fn .pa{color:var(--mid)}.fn .z{color:var(--bad)}
.note{color:var(--dim);padding:12px}
@media(max-width:600px){.big{font-size:2.6rem}.hide-m{display:none}}
</style></head><body><main>
<header>
<h1>Xenogears Decompilation Progress</h1>
<div class="sub" id="meta"></div>
<div class="hero"><div class="big" id="pct"></div><div class="stat" id="stats"></div></div>
</header>
<h2>Binaries</h2><div id="bars"></div>
<h2>History</h2><div class="panel" id="hist"></div>
<h2>Units</h2>
<div class="controls"><input id="filter" type="search" placeholder="Filter units or categories..."></div>
<div class="scroll"><table><thead><tr id="head"></tr></thead><tbody id="body"></tbody></table></div>
</main>
<script id="data" type="application/json">/*DATA*/</script>
<script>
const D = JSON.parse(document.getElementById('data').textContent);
const $ = id => document.getElementById(id);
const el = (tag, cls, text) => { const e = document.createElement(tag); if (cls) e.className = cls; if (text != null) e.textContent = text; return e; };
const n = x => x.toLocaleString('en-US');
const pc = x => x.toFixed(1) + '%';
const catName = {}; D.categories.forEach(c => catName[c.id] = c.name);

// header
const meta = $('meta');
meta.append('commit ');
const a = el('a', null, D.short); a.href = D.repo + '/commit/' + D.commit; meta.append(a, ' · ' + D.date.replace('T', ' ').replace('Z', ' UTC'));
$('pct').textContent = pc(D.total.pct);
$('stats').innerHTML = '<div>matched code</div>';
const s = $('stats');
s.innerHTML = '';
[['functions', n(D.total.matched_functions) + ' / ' + n(D.total.total_functions)],
 ['bytes', n(D.total.matched_code) + ' / ' + n(D.total.total_code)],
 ['fuzzy', pc(D.total.fuzzy)]].forEach(([k, v]) => { const d = el('div', 'stat', k + ': '); d.append(el('b', null, v)); s.append(d); });

// bars
D.categories.forEach(c => {
  const b = el('div', 'bar'), r = el('div', 'row');
  const l = el('span', null, c.name); l.append(el('span', 'meta', '  ' + pc(c.m.pct)));
  r.append(l, el('span', 'meta', n(c.m.matched_code) + ' / ' + n(c.m.total_code) + ' B · ' + c.m.matched_functions + ' / ' + c.m.total_functions + ' fn'));
  const t = el('div', 'track'), f = el('div', 'fill'); f.style.width = Math.min(100, c.m.pct) + '%'; t.append(f);
  b.append(r, t); $('bars').append(b);
});

// history chart
(function () {
  const h = D.history, box = $('hist');
  if (h.length < 2) { box.append(el('div', 'note', 'Not enough history yet (need at least 2 snapshots).')); return; }
  const W = 800, H = 280, L = 44, R = 16, T = 16, B = 30, NS = 'http://www.w3.org/2000/svg';
  const t = h.map(p => Date.parse(p.date)), v = h.map(p => p.pct);
  const t0 = Math.min(...t), t1 = Math.max(...t), span = (t1 - t0) || 1;
  const lo = Math.max(0, Math.floor(Math.min(...v) / 5) * 5 - 5), hi = Math.min(100, Math.ceil(Math.max(...v) / 5) * 5 + 5);
  const X = x => L + (x - t0) / span * (W - L - R), Y = y => H - B - (y - lo) / ((hi - lo) || 1) * (H - T - B);
  const mk = (tag, at, text) => { const e = document.createElementNS(NS, tag); for (const k in at) e.setAttribute(k, at[k]); if (text != null) e.textContent = text; return e; };
  const svg = mk('svg', { viewBox: `0 0 ${W} ${H}`, class: 'chart' });
  for (let i = 0; i <= 4; i++) {
    const y = lo + (hi - lo) * i / 4;
    svg.append(mk('line', { x1: L, x2: W - R, y1: Y(y), y2: Y(y), stroke: '#262d36' }));
    svg.append(mk('text', { x: L - 6, y: Y(y) + 4, fill: '#8b949e', 'font-size': 11, 'text-anchor': 'end' }, y.toFixed(1) + '%'));
  }
  [0, 1, 2, 3].forEach(i => {
    const tt = t0 + span * i / 3;
    svg.append(mk('text', { x: X(tt), y: H - 8, fill: '#8b949e', 'font-size': 11, 'text-anchor': i == 0 ? 'start' : i == 3 ? 'end' : 'middle' }, new Date(tt).toISOString().slice(0, 10)));
  });
  svg.append(mk('polyline', { points: h.map((p, i) => X(t[i]) + ',' + Y(v[i])).join(' '), fill: 'none', stroke: '#3fb950', 'stroke-width': 2 }));
  h.forEach((p, i) => {
    const c = mk('circle', { cx: X(t[i]), cy: Y(v[i]), r: 3.5, fill: '#3fb950' });
    c.append(mk('title', {}, p.date.slice(0, 10) + ' ' + p.commit.slice(0, 7) + ': ' + pc(p.pct)));
    svg.append(c);
  });
  box.append(svg);
})();

// units table
const cols = [
  ['unit', u => u.name, ''], ['category', u => catName[u.cat] || u.cat, 'hide-m'],
  ['code %', u => u.m.pct, ''], ['functions', u => u.m.matched_functions / (u.m.total_functions || 1), ''],
  ['bytes', u => u.m.total_code, 'hide-m']];
let sortCol = 2, sortDir = 1, open = new Set();
const head = $('head');
cols.forEach(([name, , cls], i) => {
  const th = el('th', cls, name); th.onclick = () => { if (sortCol === i) sortDir = -sortDir; else { sortCol = i; sortDir = i < 2 ? 1 : -1; } draw(); };
  head.append(th);
});
$('filter').oninput = () => draw();

function rowClass(p) { return p >= 100 ? 'p3' : p >= 60 ? 'p2' : p >= 1 ? 'p1' : 'p0'; }
function fnList(u) {
  const g = el('div', 'fn');
  u.f.forEach(f => {
    const cls = f[2] >= 100 ? 'm' : f[2] > 0 ? 'pa' : 'z';
    const d = el('div', cls); d.append(el('span', null, f[0]), el('span', null, f[1] + 'B ' + (f[2] >= 100 ? '100' : f[2].toFixed(1)) + '%'));
    g.append(d);
  });
  return g;
}
function draw() {
  const q = $('filter').value.trim().toLowerCase();
  const key = cols[sortCol][1];
  const rows = D.units.filter(u => !q || u.name.toLowerCase().includes(q) || (catName[u.cat] || u.cat).toLowerCase().includes(q))
    .sort((x, y) => { const a = key(x), b = key(y); return (a < b ? -1 : a > b ? 1 : 0) * sortDir; });
  [...head.children].forEach((th, i) => th.classList.toggle('on', i === sortCol));
  const body = $('body'); body.textContent = '';
  rows.forEach(u => {
    const tr = el('tr', 'u ' + rowClass(u.m.pct));
    [u.name, catName[u.cat] || u.cat, pc(u.m.pct), u.m.matched_functions + ' / ' + u.m.total_functions, n(u.m.matched_code) + ' / ' + n(u.m.total_code)]
      .forEach((txt, i) => tr.append(el('td', cols[i][2], txt)));
    tr.onclick = () => { open.has(u.name) ? open.delete(u.name) : open.add(u.name); draw(); };
    body.append(tr);
    if (open.has(u.name)) {
      const fr = el('tr', 'fl'), td = el('td'); td.colSpan = 5;
      td.append(u.f.length ? fnList(u) : el('div', 'note', 'No functions.')); fr.append(td); body.append(fr);
    }
  });
  if (!rows.length) { const tr = el('tr'), td = el('td', 'note', 'No matching units.'); td.colSpan = 5; tr.append(td); body.append(tr); }
}
draw();
</script></body></html>
"""

if __name__ == "__main__":
    main()
