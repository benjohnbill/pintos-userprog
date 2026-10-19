#!/usr/bin/env python3
"""Sync one person's checks on the team plan page with the latest test results.
Called by `make pcheck WHO=<name>`.

  --who NAME   whose blocks to sync (benjohnbill, Hannuri-Song, JSZeroDev);
               the shared blocks are synced too
  --dry-run    print the changes without writing them
  --build DIR  build directory (default: pintos/userprog/build)
  --plan SRC   plan URL or local file (default: the published team-plan.json)

A test with a result checks its box on PASS and clears it on FAIL. A test
without a result, or with one older than kernel.bin, is left as it is, so a
partial run never clears the rest. Task items with an "auto" rule are checked
when every result under that directory is present and passes.

The plan is team-plan.json on the study site, the same file the page renders.
Checks live in the page's Firebase database.
"""
import argparse
import json
import os
import re
import subprocess
import sys
import urllib.parse
import urllib.request

PLAN = 'https://benjohnbill.github.io/study/week08/team-plan.json'
DB = ('https://pintos-userprog-default-rtdb.asia-southeast1'
      '.firebasedatabase.app/team-plan/checks')

ROOT = subprocess.run(['git', 'rev-parse', '--show-toplevel'],
                      capture_output=True, text=True, check=True).stdout.strip()


def fetch(url, method='GET', body=None):
    req = urllib.request.Request(url, data=body, method=method,
                                 headers={'Cache-Control': 'no-cache'})
    with urllib.request.urlopen(req, timeout=15) as r:
        return json.load(r)


def db_key(who, block, item):
    """Same key as dbKey() in team-plan.html."""
    return ('all' if who == '*' else who) + '|' + block + '|' + \
        re.sub(r'[.#$\[\]/]', '_', item)


def read_results(build):
    """{relative path without .result: True on PASS}, skipping stale files."""
    kernel = os.path.join(build, 'kernel.bin')
    if not os.path.exists(kernel):
        sys.exit(f'no {kernel}: build and run the tests first')
    fresh = os.path.getmtime(kernel)
    results, stale = {}, 0
    tests = os.path.join(build, 'tests')
    for dirpath, dirnames, files in os.walk(tests):
        dirnames[:] = [d for d in dirnames if d != 'mlfqs']
        for f in files:
            if not f.endswith('.result'):
                continue
            path = os.path.join(dirpath, f)
            if os.path.getmtime(path) < fresh:
                stale += 1
                continue
            with open(path) as fp:
                rel = os.path.relpath(path, tests)[:-len('.result')]
                results[rel] = fp.readline().strip() == 'PASS'
    return results, stale


def want_test(name, results):
    """True / False to set the box, None to leave it."""
    hits = [ok for rel, ok in results.items() if os.path.basename(rel) == name]
    return hits[0] if hits else None


def want_auto(rule, results):
    """Every result under rule['dir'] present (rule['count']) and passing."""
    prefix = rule['dir'] + '/' if rule['dir'] else ''
    hits = [ok for rel, ok in results.items() if rel.startswith(prefix)]
    if not all(hits):
        return False
    return True if len(hits) == rule['count'] else None


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('--who', required=True)
    ap.add_argument('--dry-run', action='store_true')
    ap.add_argument('--build', default=os.path.join(ROOT, 'pintos/userprog/build'))
    ap.add_argument('--plan', default=PLAN, help='plan URL or local file')
    args = ap.parse_args()

    if os.path.exists(args.plan):
        with open(args.plan) as fp:
            plan = json.load(fp)
    else:
        plan = fetch(args.plan)
    people = sorted({p for d in plan['days'] for p in d['who'] if p != '*'})
    if args.who not in people:
        sys.exit(f'unknown WHO={args.who!r}; one of: {", ".join(people)}')

    results, stale = read_results(args.build)
    checks = fetch(DB + '.json') or {}

    changes = []                        # (key, on)
    for d in plan['days']:
        owners = ['*'] if d['who'][0] == '*' else d['who']
        if args.who not in owners and owners != ['*']:
            continue
        who = owners[0] if owners == ['*'] else args.who
        rows = [(t, want_test(t, results)) for t in d.get('t', [])]
        rows += [(k['name'], want_auto(k['auto'], results)) if isinstance(k, dict)
                 else (k, None) for k in d.get('k', [])]
        lines, done = [], 0
        for name, new in rows:
            key = db_key(who, d['id'], name)
            now = checks.get(key) is True
            on = now if new is None else new
            done += on
            if on != now:
                changes.append((key, on))
                lines.append(f'  {"+" if on else "-"} {name}')
        print(f'{d["day"]} · {d["h"]} ({"전원" if who == "*" else who})'
              f'  {done} / {len(rows)}')
        print('\n'.join(lines) if lines else '  (변화 없음)')

    print(f'\nresults: {len(results)} read, {stale} older than kernel.bin skipped')
    if args.dry_run or not changes:
        print('dry run: nothing written' if args.dry_run else 'nothing to write')
        return
    for key, on in changes:
        url = DB + '/' + urllib.parse.quote(key, safe='') + '.json'
        fetch(url, 'PUT', b'true') if on else fetch(url, 'DELETE')
    print(f'written: {len(changes)} → {PLAN.replace(".json", ".html")}')


if __name__ == '__main__':
    main()
