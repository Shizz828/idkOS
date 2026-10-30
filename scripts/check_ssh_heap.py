#!/usr/bin/env python3
"""Summarize actual ADV [ssh] lifecycle records; no simulated heap claims."""
import argparse
import re
import sys

STAGES = {"before-start", "network-ready", "connected", "after-failed-cleanup",
          "after-disconnect-cleanup", "settled-idle"}


def parse(lines):
    records = []
    for line in lines:
        match = re.search(r"\[ssh\] attempt=(\d+) ([a-z-]+)\s", line)
        if not match or match[2] not in STAGES:
            continue
        values = {key: int(value) for key, value in
                  re.findall(r"([a-z_]+)=(\d+)", line)}
        if {"free", "largest", "sessions", "channels", "sockets", "dns", "https"} <= values.keys():
            records.append(dict(values, stage=match[2]))
    return records


def report(records, warmup=2):
    if not records:
        return ["No SSH lifecycle heap records found."], False
    rows = ["Attempt Stage                       Free B  Largest B  S/C/FD DNS HTTPS"]
    clean = True
    cleanup_seen = False
    for row in records:
        rows.append(f"{row['attempt']:7} {row['stage']:27} {row['free']:7} "
                    f"{row['largest']:10}  {row['sessions']}/{row['channels']}/{row['sockets']}"
                    f"   {row['dns']:3} {row['https']:5}")
        if row["stage"].endswith("cleanup") or row["stage"] == "settled-idle":
            cleanup_seen = True
            if any(row[name] for name in ("sessions", "channels", "sockets")):
                clean = False
        if row["dns"] > 2 or (row["stage"] == "settled-idle" and row["dns"]):
            clean = False
    if not cleanup_seen:
        rows.append("No completed cleanup samples; finish an attempt before checking resources.")
        clean = False
    rows.append("Resource counters: " + ("clear after cleanup." if clean else "REVIEW retained resources."))
    settled = [row for row in records if row["stage"] == "settled-idle"
               and row["attempt"] > warmup and not row["https"] and not row["dns"]]
    if len(settled) < 2:
        rows.append("Need at least two settled-idle samples after warm-up to compare RAM.")
    else:
        first, last = settled[0], settled[-1]
        rows.append(f"Settled free RAM: {first['free']} -> {last['free']} B "
                    f"(delta {last['free'] - first['free']:+d} B).")
        rows.append(f"Settled largest block: {first['largest']} -> {last['largest']} B "
                    f"(delta {last['largest'] - first['largest']:+d} B).")
        rows.append("Compare repeated samples on the same network/workload; a downward "
                    "trend needs review. Wi-Fi/TLS/NVS warm-up can change the baseline.")
    rows.append("The lifetime minimum is not recovered free RAM and does not prove a leak.")
    return rows, clean


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("log", help="Serial log file, or - for stdin")
    parser.add_argument("--warmup", type=int, default=2,
                        help="Exclude this many initial attempts from settled comparison")
    args = parser.parse_args()
    if args.log == "-":
        records = parse(sys.stdin)
    else:
        with open(args.log, encoding="utf-8", errors="replace") as log:
            records = parse(log)
    lines, clean = report(records, max(0, args.warmup))
    print("\n".join(lines))
    return 0 if clean else 1


if __name__ == "__main__":
    sys.exit(main())
