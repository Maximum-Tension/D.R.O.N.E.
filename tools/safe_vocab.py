"""Build mind/language/safe.txt: the words the AI may say on its own.

The engine lets a reply use a word only if the word is in the conversation
(the user's messages, memory lines, actions, results and thoughts) or in this
list. The list holds the words the assistant uses in its own practice replies
without having seen them in that conversation: its own voice ("okay",
"created", "folder", "sorry"), not topics it would have to make up.

Usage: python tools/safe_vocab.py [--convs 60000] [--out mind/language/safe.txt]
"""
import argparse, os, re, subprocess, sys, collections, tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
WORD = re.compile(r"[a-z]+(?:'[a-z]+)?")


def words(s):
    return WORD.findall(s.lower())


def stem(w):
    if w.endswith("'s"):
        w = w[:-2]
    for suf in ("ing", "ed", "es", "s"):
        if len(w) > len(suf) + 2 and w.endswith(suf):
            return w[:-len(suf)]
    return w


HUMAN = set()


def chunks(s):
    out = []
    for part in re.split(r"[.!?,;:]", s.lower()):
        w = WORD.findall(part)
        if len(w) >= 4:
            out.append(" ".join(w))
    return out


def load_human():
    for name in ["persona_chat_clean.txt", "daily_dialog_clean.txt", "persona_chat.txt", "daily_dialog.txt", "topical_chat_clean.txt", "valid.txt"]:
        p = os.path.join(ROOT, "data", name)
        if not os.path.exists(p):
            continue
        with open(p, encoding="utf-8", errors="replace") as f:
            for line in f:
                if line[:2] in ("< ", "> "):
                    for ch in chunks(line[2:]):
                        HUMAN.add(ch)


def nonsense():
    """The made-up lines the thought practice data puts in the AI's mouth so it can learn to admit them: never safe."""
    try:
        sys.path.insert(0, os.path.join(ROOT, "tools"))
        import gen_sense
        return set(s.lower() for s in gen_sense.NONSENSE)
    except Exception:
        return set()


NONSENSE = nonsense()


def scan(path, cnt, convs_seen):
    ctx = set()
    used = set()
    with open(path, encoding="utf-8", errors="replace") as f:
        for line in f:
            line = line.rstrip("\n")
            if not line.strip():
                for w in used:
                    cnt[w] += 1
                ctx, used = set(), set()
                convs_seen[0] += 1
                continue
            k, rest = line[:1], line[2:]
            if k == "%":
                continue
            if k in "<":
                if rest.strip().lower() in NONSENSE:
                    continue
                for w in words(rest):
                    if w not in ctx and stem(w) not in ctx:
                        used.add(w)
            elif k in "-~":
                continue
            else:
                for w in words(rest):
                    ctx.add(w)
                    ctx.add(stem(w))
    for w in used:
        cnt[w] += 1


# Closed-class English words. They carry no facts, so they are always allowed.
FUNCTION = """a an the this that these those my your his her its our their mine yours me you him us them i we he she it they
myself yourself itself ourselves themselves someone anyone everyone no-one nobody something anything everything nothing
somebody anybody everybody one ones other others another each every either neither both all any some many much more most
few less least several such what which who whom whose when where why how whatever whoever whichever whenever wherever
and or but nor so yet for because since although though while whereas if unless until than as whether
in on at by with without within into onto out of off over under above below up down to from about around through
across along among between behind before after during against toward towards upon via near beside besides beyond
inside outside except like unlike per
be am is are was were been being have has had having do does did done doing will would shall should can could may might
must ought need dare
not no yes never always often sometimes usually again still already also too very really just only even ever quite
rather almost maybe perhaps here there now then today tomorrow yesterday soon later once twice
i'm i've i'll i'd you're you've you'll you'd he's she's it's we're we've we'll they're they've that's there's what's
who's where's how's let's don't doesn't didn't isn't aren't wasn't weren't haven't hasn't hadn't won't wouldn't can't
couldn't shouldn't mustn't
ok okay oh ah hmm yeah yep sure please thanks thank sorry hello hi hey bye goodbye
first second third last next same different new old good bad big small little long short right wrong true false
able sure ready glad happy sad well better best worse worst fine great nice""".split()


def main():

    ap = argparse.ArgumentParser()
    ap.add_argument("--convs", type=int, default=60000)
    ap.add_argument("--min", type=int, default=3)
    ap.add_argument("--out", default=os.path.join(ROOT, "mind", "language", "safe.txt"))
    a = ap.parse_args()
    tmp = tempfile.mkdtemp()
    sk = os.path.join(tmp, "skills.txt")
    se = os.path.join(tmp, "sense.txt")
    subprocess.run([sys.executable, os.path.join(ROOT, "tools", "gen_skills.py"), "--train", sk, "--eval", os.path.join(tmp, "ev.txt"),
                    "--convs", str(a.convs), "--eval-convs", "0", "--seed", "7", "--no-human"], cwd=ROOT, check=True, capture_output=True)
    subprocess.run([sys.executable, os.path.join(ROOT, "tools", "gen_sense.py"), "--out", se, "--convs", str(a.convs // 3), "--seed", "7"],
                   cwd=ROOT, capture_output=True)
    cnt = collections.Counter()
    seen = [0]
    for p in [sk, se + ".raw" if os.path.exists(se + ".raw") else se, os.path.join(ROOT, "data", "tasks_honest.txt")]:
        if os.path.exists(p):
            scan(p, cnt, seen)
    keep = sorted(set(w for w, n in cnt.items() if n >= a.min) | set(FUNCTION))
    with open(a.out, "w", encoding="utf-8") as f:
        f.write("# Words the AI may say without them appearing in the conversation. Made by tools/safe_vocab.py.\n")
        f.write("# Add a word here to allow it; remove one to forbid it unless the user says it first.\n")
        for w in keep:
            f.write(w + "\n")
    print("safe vocabulary: %d words from %d conversations -> %s" % (len(keep), seen[0], a.out))


if __name__ == "__main__":
    main()
