"""Build slide s4 (team git graph) and splice it into the deck.

Usage: python3 gitgraph.py <deck.html>
Data: research/C-git-timeline.md §1-2 (PR merge commits, branch bases).
Omitted on purpose (student, 2026-10-22): day-1 docs commits on main, #109, #117,
#115 (main) and the Copilot back-merge; remove is drawn into develop instead.
integrate/* branches are not a lane: a merge that went through one is a square on develop.
"""
import re
import sys

INK, MUT, ACC, PANEL = "#0a0a0a", "#6b6b6b", "#ff3300", "#f7f7f5"
# People colours come from the deck's :root (--p-ben/--p-nuri/--p-jsy, slides s5 and s70).
PC = {"bj": "var(--p-ben,#ff3300)", "rewrite": "var(--p-ben,#ff3300)",
      "nuri": "var(--p-nuri,#0e7c6b)", "jsy": "var(--p-jsy,#2f5bd3)"}
TINT = {"bj": "#ffece5", "nuri": "#e2f2ef", "jsy": "#e6ecfb"}
MK = {"bj": "gg-ben", "nuri": "gg-nuri", "jsy": "gg-jsy"}
MONO = 'style="font-family:var(--font-mono)"'

# develop sits in the middle: benjohnbill above, nuri and JSY below.
Y = {"main": 58, "rewrite": 148, "bj": 238, "develop": 340, "nuri": 442, "jsy": 540}
DAYS = [(150, "목 10.15"), (300, "금"), (390, "토 10.17"), (640, "일"),
        (690, "월 10.19"), (950, "화 10.20"), (1210, "수 10.21")]
END = 1585

out = []
def add(s): out.append(s)

def text(x, y, s, size=15, fill=INK, anchor="start", weight=None, mono=False):
    w = f' font-weight="{weight}"' if weight else ""
    font = "font-family:var(--font-mono);" if mono else ""
    return (f'<text x="{x}" y="{y}" style="{font}fill:{fill}" stroke="none" font-size="{size}"'
            f' text-anchor="{anchor}"{w}>{s}</text>')

def dot(x, y, r=7, fill=INK):
    return f'<circle cx="{x}" cy="{y}" r="{r}" fill="{fill}" stroke="{fill}" stroke-width="2.5"/>'

def square(x, y, fill=INK):
    return f'<rect x="{x - 8}" y="{y - 8}" width="16" height="16" fill="{fill}" stroke="{fill}" stroke-width="2"/>'

def hollow(x, y):
    return f'<circle cx="{x}" cy="{y}" r="5.5" fill="#ffffff" stroke="{INK}" stroke-width="2"/>'

def edge(x1, y1, x2, y2, color, width, marker, dash="", gap=10):
    """Curve from a lane point to another lane point; stops short of the target dot."""
    sign = 1 if y2 > y1 else -1
    ye = y2 - sign * gap
    mid = (y1 + ye) / 2
    d = f' stroke-dasharray="{dash}"' if dash else ""
    return (f'<path d="M{x1} {y1} C{x1} {mid} {x2} {mid} {x2} {ye}" style="stroke:{color}"'
            f' stroke-width="{width}"{d} marker-end="url(#{marker})"/>')

def merge(x1, lane, x2, width=2.2, own=False, muted=False):
    """own=True: drawn in the owner's colour (the piece that was kept)."""
    if muted:
        return edge(x1, Y[lane], x2, Y["develop"], MUT, 1.6, "gg-mut")
    if own:
        return edge(x1, Y[lane], x2, Y["develop"], PC[lane], 2.8, MK[lane])
    return edge(x1, Y[lane], x2, Y["develop"], INK, width, "gg-ink")

def pull(x1, x2, lane):
    return edge(x1, Y["develop"], x2, Y[lane], MUT, 1.6, "gg-mut", dash="5 5", gap=14)

def bar(x1, x2, lane, label, size=15):
    y = Y[lane]
    return (f'<rect x="{x1}" y="{y - 12}" width="{x2 - x1}" height="24" rx="12"'
            f' style="fill:{PANEL};stroke:{PC[lane]}" stroke-width="2"/>'
            + text((x1 + x2) / 2, y - 20, label, size=size, anchor="middle"))

def kept(x1, x2, lane):
    """Overlay: this piece was taken into develop."""
    y = Y[lane]
    return (f'<rect x="{x1}" y="{y - 12}" width="{x2 - x1}" height="24" rx="12"'
            f' style="fill:{TINT[lane]};stroke:{PC[lane]}" stroke-width="3.5"/>')

def dropped(x1, x2, lane, label_w=34):
    """Overlay: this piece was left out of develop (bar greys out, label is struck)."""
    y, mid = Y[lane], (x1 + x2) / 2
    return (f'<rect x="{x1}" y="{y - 12}" width="{x2 - x1}" height="24" rx="12" fill="#ffffff"'
            f' fill-opacity=".75" stroke="{MUT}" stroke-width="2" stroke-dasharray="4 4"/>'
            f'<line x1="{x1 + 8}" y1="{y}" x2="{x2 - 8}" y2="{y}" stroke="{MUT}" stroke-width="2"/>'
            f'<line x1="{mid - label_w / 2}" y1="{y - 25}" x2="{mid + label_w / 2}" y2="{y - 25}"'
            f' stroke="{INK}" stroke-width="2"/>')

def pr(x, label, sub=None, pos="ul", via_integrate=False):
    """pos: corner of the dot the label sits in (u/l = upper/lower, l/r = left/right).
    Merge edges arrive vertically, so a corner label never sits on an edge."""
    y = Y["develop"]
    s = square(x, y) if via_integrate else dot(x, y)
    upper, left = pos[0] == "u", pos[1] == "l"
    lx, anchor = (x - 14, "end") if left else (x + 14, "start")
    ly, sy = (y - 12, y - 30) if upper else (y + 26, y + 44)
    s += text(lx, ly, label, size=15, anchor=anchor, weight=600, mono=True)
    if sub:
        s += text(lx, sy, sub, size=13, fill=MUT, anchor=anchor)
    return s

# ---- static frame: day bands, lanes, labels --------------------------------
add('<g stroke-width="1" stroke="#ebebeb">')
for x, _ in DAYS:
    add(f'<line x1="{x}" y1="18" x2="{x}" y2="575"/>')
add('</g>')
for x, label in DAYS:
    add(text(x + 8, 26, label, size=14, fill=MUT, mono=True))

LANES = [("main", "main", 2.5), ("bj", "benjohnbill", 2), ("develop", "develop", 5),
         ("nuri", "nuri", 2), ("jsy", "JSY-*", 2)]
SPAN = {"main": (165, END), "develop": (300, 1565), "bj": (300, 1440),
        "nuri": (300, 1500), "jsy": (300, 1532)}
for key, label, width in LANES:
    x1, x2 = SPAN[key]
    color = PC.get(key, INK)
    add(f'<line x1="{x1}" y1="{Y[key]}" x2="{x2}" y2="{Y[key]}" style="stroke:{color}" stroke-width="{width}"/>')
    add(text(16, Y[key] + 5, label, size=16, fill=color, weight=600, mono=True))

# day 1: baseline, convention PR, the common starting point
add(dot(175, Y["main"], r=6))
add(text(175, Y["main"] - 16, "P1 baseline", size=13, fill=MUT, anchor="middle"))
add(f'<path d="M190 {Y["main"]} C200 100 205 100 215 100 L245 100 C255 100 260 100 268 {Y["main"] + 9}"'
    f' stroke="{INK}" stroke-width="2" marker-end="url(#gg-ink)"/>')
add(dot(270, Y["main"]))
add(text(230, 122, "#104 컨벤션", size=14, anchor="middle", weight=600, mono=True))
add(f'<line x1="292" y1="{Y["main"]}" x2="292" y2="{Y["jsy"]}" stroke="{MUT}" stroke-width="1.6"'
    ' stroke-dasharray="5 5"/>')
for lane in ("bj", "nuri", "jsy"):
    add(hollow(292, Y[lane]))
add(dot(292, Y["develop"], r=5))
add(text(300, Y["develop"] + 28, "같은 main에서 출발", size=13, fill=MUT))

def card(x1, w, lines, lead_x):
    """Criteria card above a merge, with a leader line down toward its square."""
    y1, dy = 70, 24
    h = 16 + dy * len(lines)
    s = (f'<rect x="{x1}" y="{y1}" width="{w}" height="{h}" rx="4" style="fill:#ffffff;stroke:{INK}" stroke-width="2"/>'
         f'<line x1="{lead_x}" y1="{y1 + h}" x2="{lead_x}" y2="{Y["develop"] - 44}" stroke="{MUT}" stroke-width="1.5" stroke-dasharray="3 4"/>')
    for k, (t, kind) in enumerate(lines):
        y = y1 + 26 + k * dy
        if kind == "title":
            s += text(x1 + 14, y, t, size=15, weight=700)
        elif kind == "rule":
            s += text(x1 + 14, y, t, size=15)
        elif kind == "sub":
            s += text(x1 + 34, y, t, size=13, fill=MUT)
        else:
            s += text(x1 + 14, y, t, size=14, weight=600, mono=True)
    return s

NUM = lambda n: f'<tspan style="fill:{ACC}" font-weight="700">{n}</tspan>'

# ---- step 1: Fri-Sat, everyone writes args + write/exit --------------------
add('<g data-step="1">')
add(bar(340, 452, "nuri", "args"))
add(bar(458, 575, "nuri", "write/exit"))
add(bar(470, 572, "bj", "args · write/exit"))
add(bar(470, 572, "jsy", "args"))
add('</g>')

# ---- step 2: Sat 22:21 first merge (#105), picked by AI on four criteria ---
add('<g data-step="2">')
add(card(400, 350, [("#105 선택 기준 · AI 평가", "title"),
                    (f"{NUM('①')} 컨벤션 충실도   {NUM('②')} 안정성", "rule"),
                    (f"{NUM('③')} 복잡도   {NUM('④')} 가독성", "rule"),
                    ("→ args ← JSY · write/exit ← nuri", "pick")], 628))
add(dropped(470, 572, "bj", label_w=118))
add(text(521, Y["bj"] + 32, "t->name 한 줄만 남음", size=13, fill=PC["bj"], anchor="middle"))
add(dropped(340, 452, "nuri", label_w=32))
add(kept(458, 575, "nuri"))
add(kept(470, 572, "jsy"))
add(merge(572, "bj", 628, muted=True))
add(merge(575, "nuri", 624, own=True))
add(merge(572, "jsy", 632, own=True))
add(pr(628, "#105", via_integrate=True))
add(pull(636, 700, "nuri"))
add('</g>')

# ---- step 2: Mon, tools, everyone writes fork/wait/exec --------------------
add('<g data-step="3">')
add(f'<path d="M700 {Y["develop"]} C708 292 712 292 720 292 L735 292 C743 292 746 292 750 {Y["develop"] - 10}"'
    f' stroke="{INK}" stroke-width="2" marker-end="url(#gg-ink)"/>')
add(dot(752, Y["develop"]))
add(text(725, 283, "#106·107", size=15, anchor="middle", weight=600, mono=True))
add(text(725, 266, "make ptest", size=13, fill=MUT, anchor="middle"))
add(bar(700, 780, "bj", "wait"))
add(bar(786, 866, "bj", "exec"))
add(bar(706, 786, "nuri", "wait"))
add(bar(792, 878, "nuri", "fork"))
add(bar(780, 826, "jsy", "wait", size=14))
add(bar(832, 878, "jsy", "exec", size=14))
add(pull(758, 780, "jsy"))
add('</g>')

# ---- step 3: Mon 21:00 deadline merge (#108): what was kept, what was not --
add('<g data-step="4">')
add(dropped(700, 780, "bj"))
add(dropped(786, 866, "bj"))
add(kept(706, 786, "nuri"))
add(kept(792, 878, "nuri"))
add(dropped(780, 826, "jsy", label_w=30))
add(kept(832, 878, "jsy"))
add(merge(866, "bj", 922, muted=True))
add(merge(878, "nuri", 922, own=True))
add(merge(878, "jsy", 928, own=True))
add(card(780, 356, [("#108 선택 기준 · 마감 + 호환성", "title"),
                    (f"{NUM('①')} 월 21시까지 동작하는 것", "rule"),
                    (f"{NUM('②')} 서로 맞물림: fork·wait가 child_status 공유", "rule"),
                    ("→ fork·wait ← nuri · exec ← JSY", "pick")], 925))
add(pr(925, "#108", via_integrate=True))
add(hollow(950, Y["develop"]))
add('</g>')

# ---- step 4: Tue, fd base (#110), everyone pulls ---------------------------
add('<g data-step="5">')
add(pull(932, 965, "nuri"))
add(pull(952, 980, "bj"))
add(bar(980, 1080, "bj", "page fault"))
add(bar(965, 1085, "nuri", "fd 기반"))
add(merge(1085, "nuri", 1112))
add(pr(1112, "#110", "이름을 컨벤션에", pos="ll", via_integrate=True))
add(hollow(1132, Y["develop"]))
add(pull(1118, 1145, "nuri"))
add(pull(1136, 1152, "bj"))
add(bar(1145, 1258, "nuri", "open·close·read"))
add('</g>')

# ---- step 5: Wed, feature PRs straight from personal branches -------------
add('<g data-step="6">')
add(merge(1258, "nuri", 1268))
add(pr(1268, "#111"))
add(pull(1272, 1290, "bj"))
add(bar(1290, 1360, "bj", "fd 정리·tell", size=14))
add(bar(1285, 1355, "nuri", "seek", size=14))
add(hollow(1318, Y["develop"]))
add(merge(1360, "bj", 1372))
add(pr(1372, "#113"))
add(merge(1355, "nuri", 1406))
add(pr(1406, "#112", pos="ll"))
add(merge(1436, "bj", 1440))
add(pr(1440, "#114", pos="ur"))
add(pull(1444, 1470, "jsy"))
add(bar(1450, 1500, "nuri", "create", size=14))
add(merge(1500, "nuri", 1488))
add(pr(1488, "#116", pos="lr"))
add(bar(1470, 1532, "jsy", "remove", size=14))
add(hollow(1516, Y["develop"]))
add(hollow(1531, Y["develop"]))
add(merge(1532, "jsy", 1550))
add(dot(1550, Y["develop"], r=6))
add(edge(1565, Y["develop"], 1575, Y["main"], INK, 2.6, "gg-ink"))
add(dot(1575, Y["main"], r=8))
add(text(1560, Y["main"] - 16, "#118 → main", size=15, anchor="end", weight=600, mono=True))
add(text(1560, Y["main"] - 34, "수 20:13", size=13, fill=MUT, anchor="end"))
add('</g>')

# ---- step 6: rewrite/fwe, the branch that never came back -----------------
add('<g data-step="7">')
add(text(16, Y["rewrite"] + 5, "rewrite/fwe", size=16, fill=PC["bj"], weight=600, mono=True))
add(f'<path d="M1152 {Y["bj"]} C1152 {Y["rewrite"] + 20} 1160 {Y["rewrite"]} 1172 {Y["rewrite"]}"'
    f' style="stroke:{PC["bj"]}" stroke-width="2.2"/>')
add(f'<rect x="1172" y="{Y["rewrite"] - 12}" width="80" height="24" rx="12"'
    f' style="fill:#ffffff;stroke:{PC["bj"]}" stroke-width="2.2" stroke-dasharray="7 5"/>')
add(text(1264, Y["rewrite"] + 5, "빈칸 다시 짜기 → fork에서 멈춤", size=14, fill=PC["bj"]))
add(text(1264, Y["rewrite"] + 24, "로컬 전용 · develop에 합치지 않음", size=13, fill=MUT))
add('</g>')

# ---- pass counts: a strip under the lanes; each merge step adds its number ---
# Sources: PR bodies and research C §3. #111/#112 recorded no numbers, so none here.
SY = 582
add(f'<line x1="160" y1="{SY - 5}" x2="{END}" y2="{SY - 5}" stroke="#ebebeb" stroke-width="22" stroke-linecap="round"/>')
add(text(16, SY, "통과 수", size=16, weight=600, mono=True))
add(text(270, SY, "18/95", size=14, anchor="middle", weight=600, mono=True))
COUNTS = {2: [(628, "args 6/6")], 4: [(925, "46/95")], 5: [(1112, "50/95")],
          6: [(1318, "69/95"), (1372, "73/96"), (1488, "94/96")]}
for step, items in COUNTS.items():
    add(f'<g data-step="{step}">')
    for x, v in items:
        add(text(x, SY, v, size=14, anchor="middle", weight=600, mono=True))
    if step == 6:
        add(text(1575, SY, "96/96", size=15, fill=ACC, anchor="middle", weight=700, mono=True))
        add(text(1575, SY + 22, "make ptest", size=13, fill=MUT, anchor="end"))
    add('</g>')

# ---- legend ----------------------------------------------------------------
lx, ly = 160, 640
add(f'<path d="M{lx} {ly + 8} L{lx} {ly - 10}" stroke="{INK}" stroke-width="2.2" marker-end="url(#gg-ink)"/>')
add(text(lx + 12, ly + 5, "합침", size=14))
add(f'<path d="M{lx + 70} {ly - 10} L{lx + 70} {ly + 6}" stroke="{MUT}" stroke-width="1.6"'
    ' stroke-dasharray="5 5" marker-end="url(#gg-mut)"/>')
add(text(lx + 82, ly + 5, "develop 받아오기", size=14))
add(square(lx + 230, ly))
add(text(lx + 244, ly + 5, "integrate 브랜치에서 골라 합침", size=14))
add(dot(lx + 470, ly))
add(text(lx + 484, ly + 5, "개인 브랜치 PR", size=14))
add(hollow(lx + 610, ly))
add(text(lx + 624, ly + 5, "develop 직접 커밋", size=14))
add(f'<rect x="{lx + 760}" y="{ly - 10}" width="40" height="20" rx="10" style="fill:{TINT["nuri"]};stroke:{PC["nuri"]}" stroke-width="3.5"/>')
add(text(lx + 810, ly + 5, "합칠 때 채택", size=14))
add(f'<rect x="{lx + 920}" y="{ly - 10}" width="40" height="20" rx="10" fill="#ffffff" stroke="{MUT}" stroke-width="2" stroke-dasharray="4 4"/>'
    f'<line x1="{lx + 926}" y1="{ly}" x2="{lx + 954}" y2="{ly}" stroke="{MUT}" stroke-width="2"/>')
add(text(lx + 970, ly + 5, "빠짐", size=14))

def marker(mid, color):
    return (f'<marker id="{mid}" viewBox="0 0 10 10" refX="9" refY="5" markerWidth="6" markerHeight="6"'
            f' orient="auto-start-reverse"><path d="M0 0 L10 5 L0 10 z" style="fill:{color}" stroke="none"/></marker>')

DEFS = ("<defs>" + marker("gg-ink", INK) + marker("gg-mut", MUT)
        + "".join(marker(MK[k], PC[k]) for k in MK) + "</defs>")
LABEL = ("팀 저장소의 일주일 git graph. develop이 가운데 있고 benjohnbill은 위, nuri와 JSY는 아래에 있다. "
         "세 사람이 같은 main에서 출발해 각자 짜고, 토·월·화에는 integrate 브랜치에서 골라 develop에 합쳤으며 "
         "수요일에는 기능마다 develop으로 바로 PR을 올렸다. 합칠 때마다 develop을 각자 받아갔다. "
         "월요일 마감 merge에서는 nuri의 wait·fork와 JSY의 exec가 채택되고 benjohnbill의 wait·exec와 JSY의 wait는 빠졌다. "
         "통과 수는 18/95에서 96/96까지 올라갔다. 수요일 저녁 develop을 main에 합쳤다. benjohnbill의 rewrite/fwe 브랜치는 빈칸 다시 짜기 중 fork에서 멈췄고 develop에 합치지 않았다.")
SVG = (f'<svg viewBox="0 0 1600 655" role="img" aria-label="{LABEL}" fill="none" stroke="currentColor">'
       + DEFS + "".join(out) + "</svg>")

SECTION = f'''<!-- ============================================================
     04 · 목~수: the week as one git graph (generated by gitgraph.py; edit the script, not this block)
     ============================================================ -->
<section class="slide" id="s4">
    <div class="head"><span>목 – 수 · 10.15 – 10.21</span><span class="num"></span></div>
    <div class="slide-content">
        <h2>각자 짜고, <em>develop</em>에서 합치고, 다시 나눠 받았다.</h2>
        <figure class="board">{SVG}</figure>
    </div>
    <div class="foot"><span>github.com/benjohnbill/pintos-userprog · PR #104 – #118</span><span>목 – 수</span></div>
    <aside class="notes" data-sec="55">
        <p>저희 팀 저장소의 일주일을 git graph 하나로 그렸습니다. 가운데가 develop이고, 셋 다 같은 main에서 출발해서 각자 브랜치에서 짰습니다.</p>
        <p>맨 아래 줄은 합칠 때마다 잰 테스트 통과 수입니다.</p>
        <p><span class="st">▸1</span>토요일까지 셋이 각자 args와 write, exit을 짰습니다.</p>
        <p><span class="st">▸2</span>토요일 밤 첫 merge에서는 AI에게 컨벤션 충실도, 안정성, 복잡도, 가독성으로 평가하게 해서 골랐습니다. args는 승영님 것, write와 exit은 누리님 것이 들어갔습니다.</p>
        <p><span class="st">▸3</span>월요일에는 테스트 도구를 먼저 넣고, fork, wait, exec는 셋이 각자 짰습니다.</p>
        <p><span class="st">▸4</span>월요일 merge는 기준이 달랐습니다. 저녁 9시까지 동작하는 것, 그리고 서로 맞물리는 것이었습니다. wait와 fork는 nuri님 것, exec는 승영님 것이 들어갔고, 제 wait와 exec는 빠졌습니다.</p>
        <p><span class="st">▸5</span>합칠 때마다 각자 develop을 받아가서 그 위에서 다음 작업을 시작했습니다.</p>
        <p><span class="st">▸6</span>수요일에는 integrate 없이 기능마다 develop으로 바로 PR을 올렸고, 저녁에 96개를 모두 통과해 main으로 합쳤습니다. 이 숫자는 전부 같은 명령 하나, make ptest로 쟀습니다.</p>
        <p><span class="st">▸7</span>그리고 이 빨간 가지가 제가 혼자 판 빈칸 다시 짜기 브랜치입니다. 마지막에 다시 말씀드리겠습니다.</p>
    </aside>
</section>'''

path = sys.argv[1]
html = open(path, encoding="utf-8").read()
pat = re.compile(r"<!-- =+\n\s+04 · .*?</section>", re.S)
assert len(pat.findall(html)) == 1, "s4 block not found exactly once"
html = pat.sub(lambda _: SECTION, html)
open(path, "w", encoding="utf-8").write(html)
print("s4 replaced")
