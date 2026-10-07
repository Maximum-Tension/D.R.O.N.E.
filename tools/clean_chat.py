import re, sys, os

LIFE = re.compile(r"\b(i live|i'm from|i am from|i grew up|i moved|my (wife|husband|girlfriend|boyfriend|kids?|son|daughter|mom|mother|dad|father|brother|sister|family|job|boss|parents|house|apartment|car|dog|cat|husband's|wife's)|"
                  r"i work|i'm working|i am working|i have (a|an|two|three|four|five|[0-9]+) (kids|children|dogs|cats|brothers|sisters|sons|daughters|jobs?)|years old|i was born|"
                  r"i have to go|i gotta go|gotta go|got to go|talk to you later|nice (talking|chatting|meeting) (to|with) you|it was nice talking|i'm married|i am married|i'm single|"
                  r"i'm (a|an) (?!small|little|ai\b)[a-z]+( [a-z]+)? (teacher|student|nurse|doctor|mom|dad|mother|father|lawyer|engineer|chef|cook|driver|waitress|waiter|artist|writer|singer|musician|farmer|vegan|vegetarian)|"
                  r"i am (a|an) (teacher|student|nurse|doctor|mom|dad|mother|father|lawyer|engineer|chef|cook|driver|waitress|waiter|artist|writer|singer|musician|farmer|vegan|vegetarian)|"
                  r"i'm (a|an) (teacher|student|nurse|doctor|mom|dad|mother|father|lawyer|engineer|chef|cook|driver|waitress|waiter|artist|writer|singer|musician|farmer|vegan|vegetarian)|"
                  r"my name is (?!code)|i go to (school|college|work|church)|i went to|i used to (work|live)|i drive|in the midwest|my hometown|i'm retired|i am retired)\b", re.I)


LIFE2 = re.compile(r"\b(my|mine|me too|same here|favou?rite|i (like|love|enjoy|prefer|hate|have|had|went|go to|go|am a|was|work|worked|live|lived|study|studied|play|played|watch|watched|listen|eat|ate|drink|cook|own|grew|moved|visited|travel|travelled|traveled|heard|used to|would love|can't wait|plan|love to|like to|also like|also love|got|bought|made|saw|met|miss|grew up|agree|think so too)|"
                   r"i'm (a|an|from|going|in|at|so|really|very|also|currently|still|just|actually|retired|married|single|busy|excited|planning|working|studying|reading|watching|trying|thinking|learning)|i am (a|an|from|going|in|at|so|really|very|also|retired|married|single|busy|excited)|"
                   r"i've (been|got|had|never|always|seen|heard|read|watched|played|done|tried)|i'd (love|like)|i'll (be|have|go|try|check)|we (went|have|had|are|were|live)|our )\b", re.I)


LIFE3 = re.compile(r"(i'm sure you|you must be|you're a (great|good|wonderful)|you are a (great|good)|i'll pick|see you (at|then|there|tomorrow|soon|next)|\b(hi|hello|hey),? i'm [a-z]+|\bi'm [a-z]+\.$|let's meet|i'll meet|i'll call)", re.I)


def clean(src, dst):
    n = kept = marked = persona = 0
    with open(src, encoding="utf-8", errors="ignore") as f, open(dst, "w", encoding="utf-8") as o:
        for line in f:
            s = line.rstrip("\n")
            if s.startswith("# ") and "persona" in src:
                persona += 1
                continue
            if s.startswith("< "):
                n += 1
                if LIFE.search(s[2:]) or LIFE2.search(s[2:]) or LIFE3.search(s[2:]):
                    o.write("~ " + s[2:] + "\n")
                    marked += 1
                    continue
                kept += 1
            o.write(s + "\n")
    print("%s: %d code lines, %d not learned (invented human life), %d persona lines removed" % (os.path.basename(dst), n, marked, persona))


for name in sys.argv[1:]:
    clean("data/%s.txt" % name, "data/%s_clean.txt" % name)
