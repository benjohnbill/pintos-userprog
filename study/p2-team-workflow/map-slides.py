#!/usr/bin/env python3
"""Draw the day-1 map into slide 2, and a map slide before each later chapter.

Slide 2 builds the map in four steps. Before slides 3-8, one map slide each
shows the whole map with only that chapter's regions lit. Edit the map here,
then run `python3 map-slides.py` in this folder; it rewrites
p2-team-workflow.html in place and is safe to run again.
"""
import re
from pathlib import Path

PAGE = Path(__file__).with_name('p2-team-workflow.html')
ACC, INK, MUT, PANEL = '#ff3300', '#0a0a0a', '#6b6b6b', '#f7f7f5'
MONO = 'style="font-family:var(--font-mono)"'

# Chapter map slides: (next slide, head date, title, lit regions, note)
CHAPTERS = [
    (3, '금 · 10.16', '<em>첫 날에 할 일</em> → 금요일 쪽지 시험', {'today'},
     '먼저, 첫날에 할 일이 어떻게 이어졌는지입니다.'),
    (4, '토 · 10.17', '<em>분할</em> → 시스템 콜 단위로 다시 나누기', {'split', 'down'},
     '다음은 분할입니다.'),
    (5, '토 – 월 · 10.17 – 10.19', '<em>Convention</em> → <code>make ptest</code>와 보드', {'conv', 'cmd'},
     '다음은 컨벤션에서 나온 테스트 명령입니다.'),
    (6, 'Convention · CONVENTION.md', '<em>Convention</em> → <code>CONVENTION.md</code> 세 구역', {'conv', 'doc'},
     '다음은 컨벤션 문서 자체입니다.'),
    (70, '화 · 수 · 10.20 – 10.21', '<em>분할</em> → 기능 단위로 합치기', {'split', 'later'},
     '다음은 다시 분할입니다.'),
    (8, '마무리', '<em>분할 → 공통</em>: 다 짜 보고 리뷰했나', {'up'},
     '마지막으로, 분할에서 공통으로 돌아오는 화살표입니다.'),
]


def svg(sfx, steps=True, lit=None):
    """The map. steps=True: slide 2's four-step build. lit: regions to light."""
    a, aa = f'b-ah{sfx}', f'b-ah-acc{sfx}'

    def rg(name, body):
        hi = ' hi' if lit and name in lit else ''
        return f'<g class="rg{hi}">{body}</g>'

    def step(n, body):
        return f'<g data-step="{n}">{body}</g>' if steps else body

    unit = (
        '<rect x="50" y="115" width="120" height="52" rx="4"/>'
        '<rect x="50" y="195" width="120" height="52" rx="4"/>'
        '<path d="M182 120 Q205 120 205 140 L205 170 Q205 181 218 181 Q205 181 205 192 L205 222 Q205 242 182 242"/>'
        f'<line x1="222" y1="181" x2="258" y2="181" marker-end="url(#{a})"/>'
        '<ellipse cx="322" cy="181" rx="62" ry="34"/>'
        f'<line x1="370" y1="160" x2="532" y2="112" marker-end="url(#{a})"/>'
        f'<line x1="370" y1="202" x2="532" y2="316" marker-end="url(#{a})"/>'
        f'<g fill="{INK}" stroke="none" font-size="26" font-weight="700" text-anchor="middle">'
        '<text x="110" y="150">회의</text><text x="110" y="230">PR</text><text x="322" y="190">작업</text></g>'
        f'<g fill="{MUT}" stroke="none" font-size="17" text-anchor="middle">'
        '<text x="110" y="95">팀 활동의 단위</text><text x="322" y="245">단위를 정한다</text></g>'
    )
    common = ('<ellipse cx="600" cy="100" rx="66" ry="36"/>'
              f'<text x="600" y="109" fill="{INK}" stroke="none" font-size="26" font-weight="700" text-anchor="middle">공통</text>')
    split = ('<ellipse cx="600" cy="330" rx="66" ry="36"/>'
             f'<text x="600" y="339" fill="{INK}" stroke="none" font-size="26" font-weight="700" text-anchor="middle">분할</text>')
    down = (f'<line x1="636" y1="137" x2="636" y2="290" stroke="{ACC}" marker-end="url(#{aa})"/>'
            f'<g fill="{INK}" stroke="none" font-size="19">'
            f'<text x="686" y="178" font-weight="700" fill="{ACC}">↓ 공통 → 분할</text>'
            '<text x="686" y="204">AI를 통한</text><text x="686" y="228">시스템 콜 단위 일 분배</text></g>')
    up = (f'<line x1="564" y1="293" x2="564" y2="140" stroke="{ACC}" marker-end="url(#{aa})"/>'
          f'<g fill="{INK}" stroke="none" font-size="19">'
          f'<text x="686" y="268" font-weight="700" fill="{ACC}">↑ 분할 → 공통</text>'
          '<text x="686" y="294">로직·변수를 많이 공유하면</text><text x="686" y="318">다 짜 보고 리뷰</text></g>')
    today = (
        f'<line x1="668" y1="100" x2="996" y2="100" marker-end="url(#{a})"/>'
        f'<rect x="1000" y="40" width="560" height="360" rx="4" fill="{PANEL}"/>'
        f'<g fill="{MUT}" stroke="none" font-size="17" text-anchor="middle">'
        '<text x="832" y="84">선행 요건</text>'
        f'<text x="832" y="128" {MONO} font-size="15">args · write/exit/wait 루프</text></g>'
        f'<g fill="{INK}" stroke="none" font-size="22">'
        '<text x="1030" y="88" font-size="26" font-weight="700">첫 날(10/15)에 할 일</text>'
        '<text x="1030" y="140">1  Pintos Gitbook 읽기</text>'
        '<text x="1030" y="188">2  Shell·userprog가 도는 방식</text>'
        f'<text x="1062" y="213" fill="{MUT}" font-size="16">args-passing 준비 · 각자 AI로 조사</text>'
        '<text x="1030" y="260">3  System Call 조사</text></g>'
        f'<line x1="1030" y1="295" x2="1530" y2="295" stroke="{MUT}" stroke-width="1.5"/>'
    )
    core = (f'<g fill="{INK}" stroke="none" font-size="22">'
            f'<text x="1030" y="345" {MONO} font-size="24" font-weight="700">★ fork() wait() exec()</text>'
            f'<text x="1062" y="375" fill="{MUT}" font-size="16">이번 주 핵심 → 공통으로</text></g>')
    later = (f'<line x1="668" y1="345" x2="996" y2="345" stroke-dasharray="9 7" marker-end="url(#{a})"/>'
             f'<text x="832" y="378" fill="{MUT}" stroke="none" font-size="17" text-anchor="middle">프로세스 syscall 이후</text>')
    conv = (
        '<rect x="50" y="470" width="190" height="52" rx="4"/>'
        f'<line x1="145" y1="522" x2="145" y2="566" marker-end="url(#{a})"/>'
        f'<rect x="50" y="570" width="340" height="160" rx="4" fill="{PANEL}"/>'
        f'<line x1="240" y1="496" x2="376" y2="496" marker-end="url(#{a})"/>'
        '<rect x="380" y="455" width="430" height="82" rx="4"/>'
        f'<g fill="{INK}" stroke="none" font-size="20">'
        '<text x="145" y="505" font-size="24" font-weight="700" text-anchor="middle">Convention</text>'
        f'<text x="70" y="606" fill="{MUT}" font-size="17">같이 볼 것</text>'
        '<text x="70" y="643">✓ 이번 주 목표 한 줄</text>'
        '<text x="70" y="679">✓ 날짜별 현황 (test)</text>'
        '<text x="70" y="715">✓ 공통/분할 방법론</text>'
        '<text x="595" y="492" font-weight="700" text-anchor="middle">오픈소스 규약 → formatter + linter</text>'
        f'<text x="595" y="520" fill="{MUT}" font-size="16" text-anchor="middle">함수·모듈·API·변수 이름과 모양 통일</text></g>'
    )
    doc = (f'<line x1="810" y1="480" x2="916" y2="471" marker-end="url(#{a})"/>'
           '<rect x="920" y="445" width="260" height="52" rx="4"/>'
           f'<g stroke="none" font-size="16" {MONO}>'
           f'<text x="1050" y="478" fill="{INK}" font-size="20" font-weight="700" text-anchor="middle">CONVENTION.md</text>'
           f'<text x="1200" y="466" fill="{MUT}">.clang-format · .clang-tidy</text>'
           f'<text x="1200" y="490" fill="{MUT}">PR #104 · 10.15 merge</text></g>')
    cmd = (f'<line x1="810" y1="512" x2="916" y2="597" marker-end="url(#{a})"/>'
           '<rect x="920" y="575" width="260" height="52" rx="4"/>'
           f'<g stroke="none" font-size="16">'
           f'<text x="1050" y="608" fill="{INK}" font-size="20" font-weight="700" text-anchor="middle" {MONO}>make ptest</text>'
           f'<text x="1200" y="596" fill="{MUT}">같은 테스트 명령</text>'
           f'<text x="1200" y="620" fill="{MUT}">결과는 보드로</text></g>')

    body = (
        step(1, rg('unit', unit) + rg('common', common) + rg('split', split))
        + step(2, rg('down', down) + rg('up', up))
        + step(3, rg('today', today) + rg('core', core) + rg('later', later))
        + step(4, rg('conv', conv) + rg('doc', doc) + rg('cmd', cmd))
    )
    label = ('첫날 칠판 지도: 회의와 PR의 단위는 작업이 정하고, 작업은 공통과 분할로 나뉜다. '
             'AI를 통한 시스템 콜 단위 분배로 공통에서 분할로, 로직·변수 공유가 많으면 분할에서 공통으로 옮긴다. '
             '공통인 fork·wait·exec의 선행 요건에서 첫날 할 일이 나왔고, 컨벤션에서 CONVENTION.md와 make ptest가 나왔다.')
    return (
        f'<svg viewBox="0 0 1600 745" role="img" aria-label="{label}" fill="none" stroke="currentColor" stroke-width="2.5">'
        f'<defs><marker id="{a}" viewBox="0 0 10 10" refX="9" refY="5" markerWidth="7" markerHeight="7" orient="auto-start-reverse">'
        f'<path d="M0 0 L10 5 L0 10 z" fill="{INK}" stroke="none"/></marker>'
        f'<marker id="{aa}" viewBox="0 0 10 10" refX="9" refY="5" markerWidth="7" markerHeight="7" orient="auto-start-reverse">'
        f'<path d="M0 0 L10 5 L0 10 z" fill="{ACC}" stroke="none"/></marker></defs>'
        f'{body}</svg>'
    )


def map_slide(n, date, title, lit, note):
    return (
        '<!-- map-slide:start (generated by map-slides.py; edit the script, not this block) -->\n'
        f'<section class="slide mapslide" id="m{n}">\n'
        f'    <div class="head"><span>지도 · 다음</span><span class="num"></span></div>\n'
        '    <div class="slide-content">\n'
        f'        <h2>{title}</h2>\n'
        f'        <figure class="board">{svg(f"-m{n}", steps=False, lit=lit)}</figure>\n'
        '    </div>\n'
        f'    <div class="foot"><span>첫날 칠판 지도</span><span>{date}</span></div>\n'
        f'    <aside class="notes" data-sec="3"><p>{note}</p></aside>\n'
        '</section>\n'
        '<!-- map-slide:end -->\n\n'
    )


html = PAGE.read_text(encoding='utf-8')
html = re.sub(r'<!-- map-slide:start.*?<!-- map-slide:end -->\n\n', '', html, flags=re.S)
html, k = re.subn(r'(<section class="slide" id="s2">.*?<figure class="board">)\s*<svg.*?</svg>',
                  lambda m: m.group(1) + '\n            ' + svg('-s2'), html, count=1, flags=re.S)
assert k == 1, 'slide 2 board not found'
for n, *rest in CHAPTERS:
    at = re.search(rf'<section class="slide[^"]*" id="s{n}">', html).start()
    at = html.rindex('<!-- ====', 0, at)
    html = html[:at] + map_slide(n, *rest) + html[at:]
PAGE.write_text(html, encoding='utf-8')
print(f'slide 2 map + {len(CHAPTERS)} map slides written')
