"""Build slide s6 (CONVENTION.md in three zones) and splice it into the deck.

Usage: python3 convgraph.py <deck.html>
Data: CONVENTION.md on origin/main (A 모양, B 이름, C 이름 짓는 규칙), the root
Makefile (`make format` = A over the tree, `make lint` = A + B), PR #105 (criterion 1)
(renames 899bb3c).
"""
import re
import sys

INK, MUT, ACC, PANEL, WARN = "#0a0a0a", "#6b6b6b", "#ff3300", "#f7f7f5", "#d9a400"

out = []
def add(s): out.append(s)

def text(x, y, s, size=18, fill=INK, anchor="start", weight=None, mono=False):
    w = f' font-weight="{weight}"' if weight else ""
    font = "font-family:var(--font-mono);" if mono else ""
    return (f'<text x="{x}" y="{y}" style="{font}fill:{fill}" stroke="none" font-size="{size}"'
            f' text-anchor="{anchor}"{w}>{s}</text>')

def wrong(x, y, code, size=17):
    """A wrong line: grey and struck through. Width estimate: 0.6em per mono char."""
    w = len(code) * size * 0.6
    return (text(x, y, "✗", size=size, fill=MUT, mono=True)
            + text(x + 26, y, code, size=size, fill=MUT, mono=True)
            + f'<line x1="{x + 26}" y1="{y - size * 0.32}" x2="{x + 26 + w}" y2="{y - size * 0.32}" stroke="{MUT}" stroke-width="1.6"/>')

def right(x, y, code, size=17):
    return (text(x, y, "✓", size=size, fill=ACC, mono=True, weight=700)
            + text(x + 26, y, code, size=size, mono=True, weight=600))

BOX_Y, BOX_H, BOX_W = 40, 400, 488
XS = [36, 556, 1076]
# The rules themselves are the content; the tool is a small note in the header.
ZONES = [
    ("A", "모양", ".clang-format · 저장하면 고쳐짐", [
        ("A1", "들여쓰기는 스페이스 4칸"),
        ("A2", "중괄호: 함수만 다음 줄, 나머지는 같은 줄"),
        ("A3", "본문이 한 줄이어도 중괄호"),
        ("A4", "<tspan style=\"font-family:var(--font-mono)\">if (</tspan>는 띄우고 <tspan style=\"font-family:var(--font-mono)\">foo(</tspan>는 붙인다"),
        ("A5", "<tspan style=\"font-family:var(--font-mono)\">case</tspan>는 <tspan style=\"font-family:var(--font-mono)\">switch</tspan>와 같은 깊이"),
        ("A6", "한 줄은 80자까지"),
    ]),
    ("B", "이름", ".clang-tidy · 에디터 밑줄", [
        ("B1", "함수·변수·구조체는 snake_case"),
        ("B2", "상수 매크로·enum 값은 대문자"),
    ]),
    ("C", "이름 짓는 규칙", "도구 밖 · 합의하고 merge 때 판단", [
        ("C1", "남이 부르는 함수: 모듈_동사[_목적어]"),
        ("C2", "동사 뜻 고정: init · create · find …"),
        ("C3", "변수는 명사, 범위가 넓으면 길게"),
        ("C4", "같은 개념은 모든 파일에서 같은 이름"),
        ("C5", "구조체에 typedef 안 붙임"),
        ("C6", "숫자 상수에는 이름을 붙인다"),
    ]),
]
RULE_Y, RULE_DY = BOX_Y + 128, 36

for i, (letter, title, tool, rules) in enumerate(ZONES):
    x = XS[i]
    add(f'<g data-step="{i + 1}">')
    add(f'<rect x="{x}" y="{BOX_Y}" width="{BOX_W}" height="{BOX_H}" rx="4"'
        f' style="fill:{PANEL if letter != "C" else "#ffffff"};stroke:{INK}" stroke-width="2.5"/>')
    add(f'<circle cx="{x + 46}" cy="{BOX_Y + 50}" r="26" style="fill:{ACC}" stroke="none"/>')
    add(text(x + 46, BOX_Y + 61, letter, size=30, fill="#ffffff", anchor="middle", weight=700))
    add(text(x + 88, BOX_Y + 48, title, size=28, weight=700))
    add(text(x + 88, BOX_Y + 74, tool, size=15, fill=MUT))
    add(f'<line x1="{x + 24}" y1="{BOX_Y + 96}" x2="{x + BOX_W - 24}" y2="{BOX_Y + 96}" stroke="#ebebeb" stroke-width="2"/>')
    for k, (rid, rule) in enumerate(rules):
        y = RULE_Y + k * RULE_DY
        add(text(x + 24, y, rid, size=17, fill=ACC, weight=700, mono=True))
        add(text(x + 66, y, rule, size=18, weight=500))
    add('</g>')

# ---- B: two rules leave room for the doc's own examples --------------------
cx, cy = XS[1] + 24, RULE_Y + 2 * RULE_DY + 30
add('<g data-step="2">')
add(text(cx, cy, "int", size=17, mono=True))
add(text(cx + 42, cy, "FdCount;", size=17, mono=True))
squiggle = " ".join(f"{cx + 42 + k * 6} {cy + 8 + (3 if k % 2 else 0)}" for k in range(14))
add(f'<polyline points="{squiggle}" style="stroke:{WARN}" stroke-width="2"/>')
add(text(cx + 140, cy, "⚠ lower_case", size=15, fill=WARN, weight=600))
add(right(cx, cy + 40, "int fd_count;"))
add(wrong(cx, cy + 84, "#define maxFd 128"))
add(right(cx, cy + 124, "#define MAX_FD 128"))
add('</g>')

# ---- C: one real merge that applied C4 and C2 ------------------------------
cx, cy = XS[2] + 24, RULE_Y + 6 * RULE_DY + 2
add('<g data-step="3">')
add(f'<line x1="{cx}" y1="{cy - 26}" x2="{cx + BOX_W - 48}" y2="{cy - 26}" stroke="#ebebeb" stroke-width="2"/>')
add(text(cx, cy, "#110 merge:", size=15, fill=MUT))
add(text(cx + 92, cy, "file_descriptor → fd_entry", size=14, mono=True, weight=600))
add(text(cx + 440, cy, "C4", size=15, fill=ACC, anchor="end", weight=700, mono=True))
add(text(cx + 92, cy + 24, "process_get_file → process_find_file", size=14, mono=True, weight=600))
add(text(cx + 440, cy + 24, "C2", size=15, fill=ACC, anchor="end", weight=700, mono=True))
add('</g>')

# ---- bottom: which command covers which zone -------------------------------
def bracket(x1, x2, y, label, step):
    return (f'<g data-step="{step}"><path d="M{x1} {y} L{x1} {y + 14} L{x2} {y + 14} L{x2} {y}"'
            f' stroke="{INK}" stroke-width="2"/>'
            + text((x1 + x2) / 2, y + 44, label, size=19, anchor="middle", weight=700, mono=True)
            + "</g>")

add(bracket(XS[0] + 10, XS[0] + BOX_W - 10, 456, "make format", 1))
add(bracket(XS[0] + 10, XS[1] + BOX_W - 10, 516, "make lint  (A + B 검사)", 2))
add(bracket(XS[2] + 10, XS[2] + BOX_W - 10, 456, "리뷰 · merge", 3))

LABEL = ("CONVENTION.md의 세 구역. A 모양은 .clang-format이 저장할 때 고친다. "
         "B 이름은 .clang-tidy가 에디터에 밑줄로 알린다. make format은 A를, make lint는 A와 B를 검사한다. "
         "C 이름 짓는 규칙은 도구가 못 잡아서 사람이 합의하고 merge 때 AI가 판단했다. "
         "PR #110에서 file_descriptor는 fd_entry로(C4), process_get_file은 process_find_file로(C2) 바뀌었다.")
SVG = (f'<svg viewBox="0 0 1600 590" role="img" aria-label="{LABEL}" fill="none" stroke="currentColor">'
       + "".join(out) + "</svg>")

SECTION = f'''<section class="slide" id="s6">
    <div class="head"><span>Convention · CONVENTION.md</span><span class="num"></span></div>
    <div class="slide-content">
        <h2>모양은 저장할 때, 이름은 경고로, <em>짓는 법은 사람과 AI가</em> 맞췄다.</h2>
        <figure class="board">{SVG}</figure>
    </div>
    <div class="foot"><span>CONVENTION.md · .clang-format · .clang-tidy · PR #110</span><span>목 – 수</span></div>
    <aside class="notes" data-sec="25">
        <p>CONVENTION.md는 세 구역으로 나뉩니다. 오른쪽으로 갈수록 도구가 해 주는 몫이 줄어듭니다.</p>
        <p><span class="st">▸1</span>A는 모양입니다. 들여쓰기, 중괄호, 공백은 저장하는 순간 clang-format이 고칩니다. make format은 이걸 파일 전체에 적용합니다.</p>
        <p><span class="st">▸2</span>B는 이름입니다. snake_case와 대문자 상수는 clang-tidy가 에디터에 밑줄로 알려 주고, make lint는 A와 B를 함께 검사합니다.</p>
        <p><span class="st">▸3</span>C는 이름 짓는 규칙입니다. 이건 도구가 못 잡아서 사람끼리 합의했고, merge할 때는 AI가 이 규칙으로 판단했습니다. 화요일 merge에서 file_descriptor가 fd_entry로, process_get_file이 process_find_file로 바뀐 게 그 예입니다.</p>
    </aside>
</section>'''

path = sys.argv[1]
html = open(path, encoding="utf-8").read()
pat = re.compile(r'<section class="slide" id="s6">.*?</section>', re.S)
assert len(pat.findall(html)) == 1, "s6 not found exactly once"
html = pat.sub(lambda _: SECTION, html)
html = re.sub(r'(<!-- =+\n\s+)06 · 월: deadline merge', r'\g<1>06 · CONVENTION.md in three zones (generated by convgraph.py; edit the script, not this block)', html)
open(path, "w", encoding="utf-8").write(html)
print("s6 replaced")
