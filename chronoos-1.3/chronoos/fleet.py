"""ChronoOS Fleet Manager (1.4, first slice): read-only view of the fleet from the receiver."""
import csv, sys, time
from chronoos import server

def _age(sec):
    if sec is None: return "never"
    for unit, size in (("d", 86400), ("h", 3600), ("m", 60)):
        if sec >= size: return f"{sec // size}{unit}"
    return f"{sec}s"

def _fleet(silent_after):
    registered = server._nodes()
    c = server._sdb()
    stats = {n: (cnt, last) for n, cnt, last in c.execute("SELECT node, COUNT(*), MAX(ts) FROM events GROUP BY node")}
    c.close()
    now = int(time.time()); rows = []
    for nid in sorted(set(registered) | set(stats)):
        cnt, last = stats.get(nid, (0, None))
        age = None if last is None else max(0, now - last)
        if nid not in registered: status = "revoked"
        elif last is None: status = "no-events"
        elif age > silent_after: status = "silent"
        else: status = "active"
        rows.append((nid, status, cnt, last, age))
    return rows

def cmd_list(a):
    rows = [r for r in _fleet(a.silent_after) if not a.status or r[1] == a.status]
    print(f"{'NODE':18} {'STATUS':10} {'EVENTS':>7}  LAST EVENT")
    for nid, status, cnt, last, age in rows[:a.limit]:
        print(f"{nid:18} {status:10} {cnt:>7}  {_age(age)} ago" if age is not None else f"{nid:18} {status:10} {cnt:>7}  never")
    if len(rows) > a.limit: print(f"... {len(rows) - a.limit} more (use --limit)")

def cmd_summary(a):
    rows = _fleet(a.silent_after); counts = {}
    for r in rows: counts[r[1]] = counts.get(r[1], 0) + 1
    print(f"nodes: {len(rows)}  events: {sum(r[2] for r in rows)}")
    for k in ("active", "silent", "no-events", "revoked"): print(f"  {k:10} {counts.get(k, 0)}")
    print("(status is based on the last event received, not on a heartbeat)")

def cmd_show(a):
    c = server._sdb()
    rows = c.execute("SELECT ts, kind, msg FROM events WHERE node=? ORDER BY eid DESC LIMIT ?", (a.node_id, a.n)).fetchall(); c.close()
    if not rows: print("no events for that node"); return
    for ts, kind, msg in rows: print(time.strftime("%F %T", time.localtime(ts)), kind, msg)

def cmd_export(a):
    with open(a.file, "w", newline="") as f:
        w = csv.writer(f); w.writerow(["node", "status", "events", "last_event_ts"])
        for nid, status, cnt, last, age in _fleet(a.silent_after): w.writerow([nid, status, cnt, last or ""])
    print("exported:", a.file)

def register(sp):
    p = sp.add_parser("fleet", help="fleet view from the receiver (1.4 preview)")
    p.add_argument("--silent-after", type=int, default=3600, help="seconds without events before a node is 'silent'")
    s = p.add_subparsers(dest="sub", required=True)
    l = s.add_parser("list"); l.add_argument("--status", choices=["active", "silent", "no-events", "revoked"]); l.add_argument("--limit", type=int, default=50); l.set_defaults(fn=cmd_list)
    s.add_parser("summary").set_defaults(fn=cmd_summary)
    w = s.add_parser("show"); w.add_argument("node_id"); w.add_argument("-n", type=int, default=10); w.set_defaults(fn=cmd_show)
    e = s.add_parser("export"); e.add_argument("file"); e.set_defaults(fn=cmd_export)
