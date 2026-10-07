"""Training talk for what Lucy's engine and skills work out: the answer, the
"why?" after it, the "are you sure?", and the questions she asks back when
something is missing.  Every episode is marked "% skill": its steps are kept
exactly as written (the engine or a skill did them, not the sense core), so
the brain learns to say what was found in its own words.

The action ("! ...") and result ("= ...") lines are written the way the
engine logs them while it runs, so what the brain sees here is what it sees
in a real chat.  gen_sense.py mixes these episodes in with its own."""

import random

NONCE_A = ["bloop", "zarn", "flim", "gorp", "trell", "wum", "snib", "plox", "drub", "kest"]
NONCE_B = ["razzie", "tolp", "vremp", "quab", "nirk", "sloon", "brell", "mipp", "yorv", "clabe"]
NONCE_C = ["lazzie", "fendle", "grop", "hoob", "jinx", "kalb", "morp", "pindle", "rusk", "tweb"]
KINDS = [("dog", "mammal", "animal"), ("cat", "mammal", "animal"), ("rose", "flower", "plant"),
         ("sparrow", "bird", "animal"), ("oak", "tree", "plant"), ("salmon", "fish", "animal"),
         ("square", "rectangle", "shape"), ("violin", "instrument", "object")]
NAMES = ["Tom", "Ann", "Bob", "Mia", "Leo", "Kai", "Eva", "Sam", "Zoe", "Max", "Ola", "Kuba"]
ITEMS = ["apples", "books", "stickers", "marbles", "coins", "cookies", "pens", "cards"]
THINGS_LEGS = [("spider", 8), ("dog", 4), ("cat", 4), ("bird", 2), ("ant", 6), ("horse", 4), ("chicken", 2)]
OPPOSITES = [("hot", "cold"), ("big", "small"), ("up", "down"), ("tall", "short"), ("fast", "slow"),
             ("happy", "sad"), ("open", "closed"), ("light", "dark")]
PACKAGES = [("france", "France", "What is the capital of France?", "places.mem"),
            ("japan", "Japan", "What is the capital of Japan?", "places.mem"),
            ("paris", "Paris", "Which country is Paris in?", "places.mem"),
            ("everest", "Everest", "How tall is Mount Everest?", "places.mem")]
UNITS = [("hour", "hours", "minute", "minutes", 60), ("minute", "minutes", "second", "seconds", 60),
         ("day", "days", "hour", "hours", 24), ("week", "weeks", "day", "days", 7),
         ("kilometer", "kilometers", "meter", "meters", 1000), ("meter", "meters", "centimeter", "centimeters", 100),
         ("kilogram", "kilograms", "gram", "grams", 1000), ("foot", "feet", "inch", "inches", 12),
         ("dozen", "dozens", "egg", "eggs", 12)]
WORDS = ["strawberry", "banana", "letter", "mississippi", "coffee", "apple", "committee", "balloon", "bookkeeper",
         "pepper", "assessment", "giraffe", "tomorrow", "parallel"]

WHY_ASKS = ["why?", "why", "how did you get that?", "show your work", "explain", "how do you know?",
            "explain your answer", "can you explain?", "how did you figure that out?", "walk me through it"]
SURE_ASKS = ["are you sure?", "really?", "are you certain?", "is that right?", "double check", "you sure?"]


def num(v):
    if abs(v - round(v)) < 1e-9:
        return str(int(round(v)))
    t = ("%.6f" % v).rstrip("0").rstrip(".")
    return t


def steps_of(a, ops):
    """[(op, b), ...] worked left to right with * and / first, as the engine
    writes it: "17 - 5 = 12, and then 12 + 2 = 14" """
    vals = [a] + [b for _, b in ops]
    signs = [o for o, _ in ops]
    out = []
    # products first
    i = 0
    while i < len(signs):
        if signs[i] in "*/":
            x, y = vals[i], vals[i + 1]
            z = x * y if signs[i] == "*" else x / y
            out.append("%s %s %s = %s" % (num(x), signs[i], num(y), num(z)))
            vals[i:i + 2] = [z]
            signs.pop(i)
        else:
            i += 1
    while signs:
        x, y = vals[0], vals[1]
        z = x + y if signs[0] == "+" else x - y
        out.append("%s %s %s = %s" % (num(x), signs[0], num(y), num(z)))
        vals[0:2] = [z]
        signs.pop(0)
    joined = ""
    for k, step in enumerate(out):
        joined += ("" if k == 0 else ", and then " if k == len(out) - 1 else ", then ") + step
    return joined, vals[0], len(out)


def follow_why(r, core, why_text, sure_text):
    if r.random() < 0.5:
        return ["> " + r.choice(WHY_ASKS), "! explain my answer", "= " + core, "< " + why_text]
    return ["> " + r.choice(SURE_ASKS), "! check my answer", "= " + core, "< " + sure_text]


def ep_why_calc(g):
    r = g.r
    a = r.randint(5, 60)
    ops = []
    for _ in range(r.choice([1, 2, 2, 3])):
        o = r.choice("+-*")
        b = r.randint(2, 12) if o == "*" else r.randint(1, 40)
        ops.append((o, b))
    expr = num(a) + "".join(" %s %s" % (o, num(b)) for o, b in ops)
    steps, value, count = steps_of(a, ops)
    res = num(value)
    lines = ["% skill", "> " + r.choice(["what is %s?" % expr, "%s = ?" % expr, "calculate %s" % expr,
                                         "how much is %s?" % expr]),
             "! calc " + expr, "= " + res,
             "< " + r.choice(["It is %s." % res, "%s = %s." % (expr, res), "%s." % res])]
    if count > 1:
        lines += follow_why(r, steps, "I worked it out step by step: %s." % steps,
                            "Yes. I checked it again, step by step: %s." % steps)
    else:
        lines += follow_why(r, "%s = %s" % (expr, res),
                            "Because %s = %s. I work numbers out exactly, so I don't guess them." % (expr, res),
                            "Yes. I checked it again: %s = %s." % (expr, res))
    return lines


def ep_word_problem(g):
    r = g.r
    who = r.choice(NAMES)
    item = r.choice(ITEMS)
    a = r.randint(8, 40)
    b = r.randint(1, a - 2)
    c = r.randint(1, 9)
    k = r.randint(0, 2)
    if k == 0:
        text = "I have %d %s. I give %d to %s and then %s gives me %d back. How many %s do I have?" % (
            a, item, b, who, who, c, item)
        ops = [("-", b), ("+", c)]
    elif k == 1:
        text = "I had %d %s. I lost %d and found %d. How many %s do I have now?" % (a, item, b, c, item)
        ops = [("-", b), ("+", c)]
    else:
        text = "%s has %d %s and buys %d more, then gives %d away. How many %s does %s have?" % (
            who, a, item, c, b, item, who)
        ops = [("+", c), ("-", b)]
    expr = num(a) + "".join(" %s %s" % (o, num(x)) for o, x in ops)
    steps, value, _ = steps_of(a, ops)
    res = num(value)
    you = "You have" if k < 2 else "%s has" % who
    lines = ["% skill", "> " + text, "! calc " + expr, "= " + res, "< %s %s %s." % (you, res, item)]
    lines += follow_why(r, steps, "I took the numbers from your message and worked it out step by step: %s." % steps,
                        "Yes. I checked it again, step by step: %s." % steps)
    return lines


def ep_why_seq(g):
    r = g.r
    k = r.randint(0, 3)
    if k == 0:
        start, d = r.randint(1, 20), r.choice([2, 3, 4, 5, 7, 10, -2, -3])
        vals = [start + d * i for i in range(r.randint(4, 5))]
        nxt = vals[-1] + d
        rule = "each number is %d %s than the one before" % (abs(d), "more" if d > 0 else "less")
        pairs = ["%s %s %d = %s" % (num(vals[i - 1]), "+" if d > 0 else "-", abs(d), num(vals[i]))
                 for i in range(1, len(vals))] + ["%s %s %d = %s" % (num(vals[-1]), "+" if d > 0 else "-", abs(d), num(nxt))]
        work = ", ".join(pairs[-4:-1]) + ", and " + pairs[-1]
        why = "%s%s: %s." % (rule[0].upper(), rule[1:], work)
    elif k == 1:
        start, f = r.randint(1, 5), r.choice([2, 3])
        vals = [start * f ** i for i in range(4)]
        nxt = vals[-1] * f
        rule = "each number is %d times the one before" % f
        pairs = ["%s * %d = %s" % (num(vals[i - 1]), f, num(vals[i])) for i in range(1, len(vals))] + [
            "%s * %d = %s" % (num(vals[-1]), f, num(nxt))]
        work = ", ".join(pairs[:-1]) + ", and " + pairs[-1]
        why = "%s%s: %s." % (rule[0].upper(), rule[1:], work)
    elif k == 2:
        n = r.randint(1, 3)
        vals = [(n + i) ** 2 for i in range(5)]
        nxt = (n + 5) ** 2
        rule = "they are the squares of %d, %d, %d and so on" % (n, n + 1, n + 2)
        why = "%s%s, so the next one is %d * %d = %d." % (rule[0].upper(), rule[1:], n + 5, n + 5, nxt)
    else:
        a, b = r.choice([(1, 1), (2, 3), (1, 2), (3, 4)])
        vals = [a, b]
        while len(vals) < 6:
            vals.append(vals[-1] + vals[-2])
        nxt = vals[-1] + vals[-2]
        rule = "each number is the sum of the two before it"
        why = "Each number is the sum of the two before it, so the next one is %d + %d = %d." % (vals[-2], vals[-1], nxt)
    seq = ", ".join(num(v) for v in vals)
    ask = r.choice(["what comes next: %s?" % seq, "what is the next number: %s?" % seq, "%s, ?" % seq,
                    "next: %s" % seq, "continue the sequence: %s" % seq])
    reply = "The next number is %s: %s." % (num(nxt), rule)
    lines = ["% skill", "> " + ask, "! sequence", "= " + num(nxt), "< " + reply]
    if r.random() < 0.8:
        lines += follow_why(r, why.rstrip("."), why, "Yes. " + why)
    return lines


def plural(w):
    if w.endswith("y") and w[-2:-1] not in "aeiou":
        return w[:-1] + "ies"
    if w.endswith(("s", "x", "sh", "ch")):
        return w + "es"
    return w + "s"


def ep_logic(g):
    r = g.r
    k = r.randint(0, 4)
    if k <= 1:
        a, b, c = r.choice(NONCE_A), r.choice(NONCE_B), r.choice(NONCE_C)
    else:
        a, b, c = r.choice(KINDS)
    A, B, C = plural(a), plural(b), plural(c)
    if k in (0, 2):
        q = r.choice(["If all %s are %s and all %s are %s, are all %s %s?" % (A, B, B, C, A, C),
                      "All %s are %s. All %s are %s. Are all %s %s?" % (A, B, B, C, A, C)])
        reply = "Yes. All %s are %s, and all %s are %s, so all %s are %s." % (A, B, B, C, A, C)
        lines = ["% skill", "> " + q, "! logic %s %s" % (a, C), "= yes", "< " + reply]
        reason = "all %s are %s, and all %s are %s, so all %s are %s" % (A, B, B, C, A, C)
    elif k == 1:
        q = "If all %s are %s and no %s are %s, can a %s be a %s?" % (A, B, B, C, a, c)
        reply = "No. All %s are %s, and no %s are %s, so a %s is not a %s." % (A, B, B, C, a, c)
        lines = ["% skill", "> " + q, "! logic %s %s" % (a, c), "= no", "< " + reply]
        reason = "all %s are %s, and no %s are %s, so a %s is not a %s" % (A, B, B, C, a, c)
    elif k == 3:
        name = r.choice(NAMES)
        q = "All %s are %s. %s is a %s. Is %s a %s?" % (A, B, name, a, name, b)
        reply = "Yes. %s is a %s, and all %s are %s, so %s is a %s." % (name, a, A, B, name, b)
        lines = ["% skill", "> " + q, "! logic %s %s" % (name.lower(), b), "= yes", "< " + reply]
        reason = "%s is a %s, and all %s are %s, so %s is a %s" % (name, a, A, B, name, b)
    else:
        name = r.choice(NAMES)
        adj = r.choice(["black", "small", "fast", "loud"])
        q = "Some %s are %s. %s is a %s. Is %s %s?" % (A, adj, name, a, name, adj)
        reply = "I can't tell. %s is a %s, but only some %s are %s, so %s might or might not." % (name, a, A, adj, name)
        lines = ["% skill", "> " + q, "! logic %s %s" % (name.lower(), adj), "= unknown", "< " + reply]
        reason = "%s is a %s, but only some %s are %s, so %s might or might not be %s" % (name, a, A, adj, name, adj)
    if r.random() < 0.6:
        if k == 4:
            reason = "%s is a %s, but only some %s are %s, so %s might or might not" % (name, a, A, adj, name)
        lines += follow_why(r, "because " + reason, "Because %s." % reason,
                            "Yes. %s%s." % (reason[0].upper(), reason[1:]) if k != 4 else
                            "Yes, I'm sure I can't tell: %s." % reason)
    return lines


def ep_order(g):
    r = g.r
    a, b, c = r.sample(NAMES, 3)
    word, most, least = r.choice([("taller", "tallest", "shortest"), ("older", "oldest", "youngest"),
                                  ("faster", "fastest", "slowest"), ("heavier", "heaviest", "lightest")])
    want_most = r.random() < 0.5
    ans = a if want_most else c
    q = "%s is %s than %s. %s is %s than %s. Who is the %s?" % (a, word, b, b, word, c, most if want_most else least)
    reason = "%s is %s than %s, and %s is %s than %s" % (a, word, b, b, word, c)
    reply = "%s is the %s: %s." % (ans, most if want_most else least, reason)
    lines = ["% skill", "> " + q, "! order %s" % (most if want_most else least), "= " + ans.lower(), "< " + reply]
    if r.random() < 0.6:
        lines += follow_why(r, "because " + reason, "Because %s." % reason,
                            "Yes. %s, so %s is the %s." % (reason, ans, most if want_most else least))
    return lines


def ep_if_then(g):
    r = g.r
    cause, effect, fact_yes, q_effect, fact_no_effect, q_cause = r.choice([
        ("it rains", "the ground gets wet", "It rains.", "Is the ground wet?", "The ground is not wet.", "Did it rain?"),
        ("the light is red", "cars stop", "The light is red.", "Do cars stop?", "Cars do not stop.", "Is the light red?"),
        ("you heat ice", "it melts", "You heat ice.", "Does it melt?", "It does not melt.", "Did you heat ice?"),
        ("the alarm rings", "Tom wakes up", "The alarm rings.", "Does Tom wake up?", "Tom does not wake up.",
         "Did the alarm ring?")])
    k = r.randint(0, 2)
    rule = "If %s, %s." % (cause, effect)
    rule = rule[0].upper() + rule[1:]
    if k == 0:
        fact = fact_yes[0].lower() + fact_yes[1:-1]
        lines = ["% skill", "> %s %s %s" % (rule, fact_yes, q_effect), "! if then", "= yes",
                 "< Yes. If %s, %s, and %s, so %s." % (cause, effect, fact, effect)]
        why = "The rule is \"if %s, %s\", and you told me %s. When the if part holds, the then part follows, so %s." % (
            cause, effect, fact, effect)
    elif k == 1:
        lines = ["% skill", "> %s %s %s" % (rule, effect[0].upper() + effect[1:] + ".", q_cause), "! if then back",
                 "= can't tell",
                 "< I can't tell. If %s, %s, but that could also happen for another reason." % (cause, effect)]
        why = "The rule only goes one way: from %s to %s. Knowing that %s doesn't tell me that %s." % (
            cause, effect, effect, cause)
    else:
        fact = fact_no_effect[0].lower() + fact_no_effect[1:-1]
        lines = ["% skill", "> %s %s %s" % (rule, fact_no_effect, q_cause), "! if then back", "= no",
                 "< No. If %s, %s. But %s, so it can't be that %s." % (cause, effect, fact, cause)]
        why = "Whenever %s, %s. You said %s, so the if part (%s) can't be true." % (cause, effect, fact, cause)
    if r.random() < 0.6:
        lines += ["> " + r.choice(WHY_ASKS), "! explain my answer", "= " + why.rstrip("."), "< " + why]
    return lines


def ep_units(g):
    r = g.r
    one, many, small_one, small_many, factor = r.choice(UNITS)
    n = r.randint(2, 12)
    res = n * factor
    if r.random() < 0.5:
        q = r.choice(["How many %s are in %d %s?" % (small_many, n, many), "convert %d %s to %s" % (n, many, small_many)])
        action = "convert %d %s to %s" % (n, many, small_many)
        reply = "There are %d %s in %d %s." % (res, small_many, n, many) if q.startswith("How") else \
            "%d %s is %d %s." % (n, many, res, small_many)
        why = "1 %s is %d %s, so %d %s is %d * %d = %d %s." % (one, factor, small_many, n, many, n, factor, res, small_many)
        lines = ["% skill", "> " + q, "! " + action, "= %d" % res, "< " + reply]
    else:
        q = "How many %s are in %d %s?" % (many, res, small_many)
        action = "convert %d %s to %s" % (res, small_many, many)
        reply = "There are %d %s in %d %s." % (n, many, res, small_many)
        why = "1 %s is %d %s, so %d %s is %d / %d = %d %s." % (one, factor, small_many, res, small_many, res, factor, n, many)
        lines = ["% skill", "> " + q, "! " + action, "= %d" % n, "< " + reply]
    if r.random() < 0.6:
        lines += follow_why(r, why.rstrip("."), why, "Yes. " + why)
    return lines


def clock_text(m):
    m %= 1440
    h, mm = divmod(m, 60)
    half = "am" if h < 12 else "pm"
    h12 = h % 12 or 12
    return "%d:%02d %s" % (h12, mm, half)


def ep_clock(g):
    r = g.r
    start = r.randint(6, 20) * 60 + r.choice([0, 15, 30, 45])
    hours, minutes = r.randint(1, 3), r.choice([10, 20, 40, 50])
    shift = hours * 60 + minutes
    a, mid, b = clock_text(start), clock_text(start + hours * 60), clock_text(start + shift)
    q = r.choice(["If a train leaves at %s and the trip takes %d hours %d minutes, when does it arrive?" % (a, hours, minutes),
                  "A movie starts at %s and lasts %d hours and %d minutes. When does it end?" % (a, hours, minutes)])
    why = "%s + %d hour%s = %s, and then %s + %d minutes = %s." % (a, hours, "" if hours == 1 else "s", mid, mid, minutes, b)
    lines = ["% skill", "> " + q, "! clock %s +%d minutes" % (a, shift), "= " + b, "< It will be %s." % b]
    if r.random() < 0.6:
        lines += follow_why(r, why.rstrip("."), why, "Yes. " + why)
    return lines


def ep_letters(g):
    r = g.r
    word = r.choice(WORDS)
    letter = r.choice(sorted(set(word)))
    places = [i + 1 for i, ch in enumerate(word) if ch == letter]
    count = len(places)
    spelled = "-".join(word)
    q = r.choice(["How many %s's are in the word %s?" % (letter, word), "how many %s in %s?" % (letter, word),
                  "count the letter %s in %s" % (letter, word)])
    reply = "There %s %d %s's in %s." % ("is" if count == 1 else "are", count, letter, word) if count != 1 else \
        "There is 1 %s in %s." % (letter, word)
    where = places[0] if count == 1 else ", ".join(str(p) for p in places[:-1]) + " and " + str(places[-1])
    lines = ["% skill", "> " + q, "! count %s in %s" % (letter, word), "= %d" % count, "< " + reply]
    if r.random() < 0.7:
        core = "%s: %s" % (spelled, where)
        if count == 1:
            lines += follow_why(r, core, "I spelled it out: %s. The %s is letter %s, and it is the only one." % (
                spelled, letter, where), "Yes. I spelled it out again: %s. The %s is letter %s, and it is the only one." % (
                spelled, letter, where))
        else:
            lines += follow_why(r, core, "I spelled it out: %s. The %s's are letters %s." % (spelled, letter, where),
                                "Yes. I spelled it out again: %s. The %s's are letters %s." % (spelled, letter, where))
    return lines


def ep_decimals(g):
    r = g.r
    whole = r.randint(1, 9)
    a, b = r.choice([(11, 9), (25, 3), (45, 5), (125, 13), (9, 1), (15, 2)])
    first = "%d.%d" % (whole, a)
    second = "%d.%d" % (whole, b)
    fa, fb = float(first), float(second)
    if fa == fb:
        return None
    small, big = (first, second) if fa < fb else (second, first)
    width = max(len(str(a)), len(str(b)))
    lined_a = first + "0" * (width - len(str(a)))
    lined_b = second + "0" * (width - len(str(b)))
    diff = num(abs(fa - fb))
    bigger_asked = r.random() < 0.5
    holds = (fa > fb) if bigger_asked else (fa < fb)
    word = "bigger" if bigger_asked else "smaller"
    q = "Is %s %s than %s?" % (first, word, second)
    result = "%s: %s is %s than %s" % ("yes" if holds else "no", first, word if holds else
                                     ("smaller" if bigger_asked else "bigger"), second)
    reply = "%s, %s is %s than %s." % ("Yes" if holds else "No", first, word if holds else
                                       ("smaller" if bigger_asked else "bigger"), second)
    why = ("Line them up: %s and %s. The whole parts are the same, so look after the point: %s has less there. "
           "So %s is smaller than %s (%s - %s = %s)." % (lined_a, lined_b, small, small, big, big, small, diff))
    lines = ["% skill", "> " + q, "! is %s %s than %s" % (first, word, second), "= " + result, "< " + reply]
    if r.random() < 0.7:
        lines += ["> " + r.choice(WHY_ASKS), "! explain my answer", "= %s - %s = %s" % (big, small, diff), "< " + why]
    return lines


def ep_ask(g):
    """the question back when something is missing, then the answer"""
    r = g.r
    k = r.randint(0, 3)
    if k == 0:
        start, d = r.randint(1, 9), r.choice([2, 3, 5, 10])
        vals = [start + d * i for i in range(4)]
        seq = ", ".join(str(v) for v in vals)
        return ["% skill", "> " + r.choice(["what comes next?", "what's next?", "what is the next number?",
                                            "continue the sequence"]),
                "! ask which sequence", "= asked",
                "< Next after what? Give me a few numbers or letters in order, like 2, 4, 6, 8, and I'll find the next one.",
                "> " + seq, "! sequence", "= %d" % (vals[-1] + d),
                "< The next number is %d: each number is %d more than the one before." % (vals[-1] + d, d)]
    if k == 1:
        n = r.randint(2, 20)
        unit_one, unit_many, target, size = r.choice([("foot", "feet", "meters", 0.3048), ("mile", "miles", "kilometers", 1.609344),
                                                      ("inch", "inches", "centimeters", 2.54), ("hour", "hours", "minutes", 60)])
        value = n * size
        shown = num(round(value, 2)) if abs(value - round(value)) > 1e-9 else num(value)
        about = "about " if abs(value * 100 - round(value * 100)) > 1e-6 else ""
        return ["% skill", "> convert %d to %s" % (n, target), "! ask which unit", "= asked",
                "< %d what? Tell me the unit too, like: convert %d feet to meters." % (n, n),
                "> " + r.choice([unit_many, "in " + unit_many, "%d %s" % (n, unit_many)]),
                "! convert %d %s to %s" % (n, unit_many, target), "= %s%s" % (about, shown),
                "< %d %s is %s%s %s." % (n, unit_many, about, shown, target)]
    if k == 2:
        thing, legs = r.choice(THINGS_LEGS)
        return ["% skill", "> how many legs does it have?", "! ask what it is", "= asked",
                "< What do you mean by \"it\"? Tell me which animal or thing, like: a spider.",
                "> " + r.choice(["a " + thing, thing, "I mean a " + thing]), "! know %s legs" % thing, "= %d" % legs,
                "< A %s has %d legs." % (thing, legs)]
    minutes = r.choice([1, 2, 5, 10, 15, 30])
    when = "in %d minute%s" % (minutes, "" if minutes == 1 else "s")
    if r.random() < 0.5:
        return ["% skill", "> " + r.choice(["send me a message later", "can you send me a message?", "message me later"]),
                "! ask when to remind", "= asked",
                "< Sure. When should I send it? For example: in 5 minutes, or at 6:30.",
                "> " + r.choice([when, when[3:]]),
                "! remind %s: Here's the message you asked for. It's been %s." % (when, when[3:]), "= set",
                "< Okay, I'll send you a message %s." % when]
    task = r.choice(["stretch", "drink water", "call mom", "check the oven", "take a break"])
    return ["% skill", "> remind me to " + task, "! ask when to remind", "= asked",
            "< Sure. When should I send it? For example: in 5 minutes, or at 6:30.",
            "> " + r.choice([when, when[3:]]),
            "! remind %s: It's time to %s. You asked me to remind you %s ago." % (when, task, when[3:]), "= set",
            "< Okay, I'll remind you to %s %s." % (task, when)]


def ep_know(g):
    """what the knowledge packages say, and where it came from"""
    r = g.r
    k = r.randint(0, 2)
    if k == 0:
        thing, legs = r.choice(THINGS_LEGS)
        lines = ["% skill", "> how many legs does a %s have?" % thing, "! know %s legs" % thing, "= %d" % legs,
                 "< A %s has %d legs." % (thing, legs)]
        said = "a %s has %d legs" % (thing, legs)
        package = "things_object.mem"
    elif k == 1:
        a, b = r.choice(OPPOSITES)
        lines = ["% skill", "> what is the opposite of %s?" % a, "! know %s opposite" % a, "= " + b,
                 "< The opposite of %s is %s." % (a, b)]
        said = "the opposite of %s is %s" % (a, b)
        package = "words.mem"
    else:
        thing, kind, wrong = r.choice([("whale", "mammal", "fish"), ("tomato", "fruit", "vegetable"),
                                       ("penguin", "bird", "fish"), ("bat", "mammal", "bird")])
        lines = ["% skill", "> is a %s a %s?" % (thing, wrong), "! know %s kind %s" % (thing, wrong), "= no",
                 "< No, a %s is a %s, not a %s." % (thing, kind, wrong)]
        said = "a %s is a %s, not a %s" % (thing, kind, wrong)
        package = "things_object.mem"
    if r.random() < 0.6:
        if r.random() < 0.5:
            lines += ["> " + r.choice(["how do you know?", "how do you know that?", "why?", "where did you get that?"]),
                      "! explain my answer", "= " + package,
                      "< Because my %s package says %s. That's where my facts come from; I don't make them up." % (package, said)]
        else:
            lines += ["> " + r.choice(SURE_ASKS), "! check my answer", "= " + package,
                      "< Yes. My %s package says %s." % (package, said)]
    return lines


def ep_package(g):
    r = g.r
    key, shown, q, package = r.choice(PACKAGES)
    return ["% skill", "> " + q, "! package for " + key, "= " + package,
            "< I don't have that information yet. Did you download the package %s and put it in my mind/memory folder? "
            "Once it is there, I'll know it right away, without a restart." % package]


def ep_repeat(g):
    r = g.r
    chars = "ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnpqrstuvwxyz23456789"
    code = "".join(r.choice(chars) for _ in range(4)) + "-" + "".join(r.choice(chars) for _ in range(4))
    if r.random() < 0.5:
        code += "-" + r.choice(["blue", "red", "cactus", "river", "moon"]) + "-" + str(r.randint(100, 9999))
    ask = r.choice(["Repeat this exact string character-for-character: %s", "repeat exactly: %s", "say this exactly: %s",
                    "copy this: %s"]) % code
    return ["% skill", "> " + ask, "! say \"%s\"" % code, "= " + code, "< " + code]


def ep_told(g):
    """facts the user told, asked back, and how Lucy knows"""
    r = g.r
    n = r.randint(2, 12)
    item = r.choice(ITEMS)
    words = ["two", "three", "four", "five", "six", "seven", "eight", "nine", "ten", "eleven", "twelve"]
    said_n = words[n - 2] if r.random() < 0.5 else str(n)
    told = "I have %s %s." % (said_n, item)
    lines = ["% skill", "> " + told, "< " + r.choice(["Okay.", "I see.", "Got it."]),
             "> How many %s do I have?" % item, "! recall " + item, "= you have %s %s" % (said_n, item),
             "< You told me that you have %s %s." % (said_n, item)]
    if r.random() < 0.6:
        lines += ["> " + r.choice(["how do you know?", "how do you know that?", "are you sure?"]),
                  "! explain my answer", "= you said: " + told[:-1],
                  "< Because you told me: \"%s\"" % told]
    return lines


def ep_arch(g):
    r = g.r
    return ["% skill", "> " + r.choice(["Explain your architecture.", "What kind of neural network are you?",
                                        "how do you work?", "how big is your brain?", "what model are you?"]),
            "! about my brain", "= about 11.4 million numbers, 27 MB",
            "< I'm a small language model, not a big LLM. My brain is a transformer: 4 layers, 256 wide, 64 attention "
            "heads, about 11.4 million numbers, 27 MB. Around the brain is an engine written in C: it reads your "
            "message, plans what to do, does exact work like math, files and memory, and checks what the brain says "
            "before I say it."]


def ep_plain_talk(g):
    r = g.r
    said, replies = r.choice([("I see.", ["Okay.", "Mm-hm.", "Got it."]), ("Yay!", ["Yay!", "Nice!", "Glad you're happy!"]),
                              ("ok", ["Okay."]), ("cool", ["Glad you like it.", "Cool!"]),
                              ("thanks!", ["You're welcome!", "Anytime."]), ("lol", ["Ha!", ":)"]),
                              ("hmm", ["Take your time."]), ("nice", ["Thanks!"])])
    return ["% skill", "> " + said, "< " + r.choice(replies)]


def ep_world_facts(g):
    """what things are like, what they can do, odd ones out and analogies"""
    r = g.r
    k = r.randint(0, 5)
    if k == 0:
        thing, trait, opposite = r.choice([("fire", "hot", "cold"), ("ice", "cold", "hot"), ("snow", "cold", "hot"),
                                           ("sugar", "sweet", "sour"), ("a lemon", "sour", "sweet"),
                                           ("a stone", "hard", "soft"), ("a feather", "light", "heavy"),
                                           ("the sun", "bright", "dark")])
        key = thing.split()[-1]
        if r.random() < 0.6:
            return ["% skill", "> Is %s %s?" % (thing, trait), "! know %s is %s" % (key, trait), "= yes",
                    "< Yes, %s is %s." % (thing, trait)]
        return ["% skill", "> Is %s %s?" % (thing, opposite), "! know %s is %s" % (key, opposite), "= no",
                "< No, %s is %s, not %s." % (thing, trait, opposite)]
    if k == 1:
        thing, verb, obj, can = r.choice([("fish", "climb", "a tree", "swim"), ("dog", "read", "a book", "swim and run"),
                                          ("cat", "fly", "", "climb and jump"), ("penguin", "fly", "", "")])
        if not can:
            return ["% skill", "> Can a %s %s?" % (thing, verb), "! know %s can %s" % (thing, verb), "= no",
                    "< No, a %s can't %s." % (thing, verb)]
        return ["% skill", "> Can a %s %s%s?" % (thing, verb, " " + obj if obj else ""),
                "! know %s can %s" % (thing, verb), "= no",
                "< No, a %s can't %s%s; it can %s." % (thing, verb, " " + obj if obj else "", can)]
    if k == 2:
        items, odd, kind, others = r.choice([(["dog", "cat", "car", "mouse"], "car", "a vehicle", "mammals"),
                                             (["apple", "banana", "carrot", "grape"], "carrot", "a vegetable", "fruits"),
                                             (["red", "blue", "chair", "green"], "chair", "furniture", "colors")])
        r.shuffle(items)
        return ["% skill", "> Which word does not belong: %s?" % ", ".join(items), "! odd one out", "= " + odd,
                "< %s: it is %s, and the others are %s." % (odd.capitalize(), kind, others)]
    if k == 3:
        a, b, c, d, rel = r.choice([("bird", "fly", "fish", "swim", "can"), ("hot", "cold", "up", "down", "opposite"),
                                    ("cat", "kitten", "dog", "puppy", "young")])
        if rel == "can":
            reply = "Swim: a bird can fly, and a fish can swim."
        elif rel == "opposite":
            reply = "Down: cold is the opposite of hot, and down is the opposite of up."
        else:
            reply = "Puppy: a baby cat is a kitten, and a baby dog is a puppy."
        return ["% skill", "> %s is to %s as %s is to what?" % (a.capitalize(), b, c), "! analogy " + rel, "= " + d,
                "< " + reply]
    if k == 4:
        miles, hours = r.choice([(120, 2), (150, 3), (90, 2), (200, 4), (60, 1)])
        speed = miles // hours if miles % hours == 0 else miles / hours
        return ["% skill", "> A train travels %d miles in %d hours. What is its average speed?" % (miles, hours),
                "! calc %d / %d" % (miles, hours), "= %s" % num(speed),
                "< %s miles per hour: %d miles / %d hours = %s." % (num(speed), miles, hours, num(speed))]
    pens, cost, want = r.choice([(3, 6, 5), (4, 8, 10), (2, 5, 6), (5, 10, 3)])
    each = cost / pens
    total = each * want
    return ["% skill", "> If %d pens cost $%d, how much do %d pens cost?" % (pens, cost, want),
            "! calc $%d / %d * %d" % (cost, pens, want), "= $%s" % num(total),
            "< $%s: one pen costs $%d / %d = $%s, so %d pens cost %d x $%s = $%s." % (
                num(total), cost, pens, num(each), want, want, num(each), num(total))]


def ep_self_talk(g):
    """what Lucy is and isn't, jokes, help, things she can't make"""
    r = g.r
    k = r.randint(0, 5)
    if k == 0:
        return ["% skill", "> " + r.choice(["Who made you?", "who created you?", "who built you?"]),
                "! about me", "= September 25, 2026",
                "< I was made on September 25, 2026. My engine, DRONEngine, is written in C, and my small brain was "
                "trained on example conversations and tasks."]
    if k == 1:
        return ["% skill", "> " + r.choice(["tell me a joke", "Tell me a joke.", "make me laugh", "another joke"]),
                "! joke", "= told", "< " + r.choice([
                    "Why was the math book sad? It had too many problems.",
                    "Why did the computer go to the doctor? It had a virus.",
                    "Parallel lines have so much in common. It's a shame they'll never meet.",
                    "What did the zero say to the eight? Nice belt!"])]
    if k == 2:
        return ["% skill", "> " + r.choice(["Write a haiku about the sea.", "write a poem", "tell me a story",
                                            "write a song about cats"]),
                "! can write", "= no: my brain is too small",
                "< I can't write poems or stories: my brain is small and made for exact work like math, logic, memory "
                "and files. I can tell you a joke, though."]
    if k == 3:
        return ["% skill", "> " + r.choice(["Translate hello to Spanish.", "how do you say cat in French?"]),
                "! can translate", "= no: I only know English",
                "< I can't translate: I only know English words. A dictionary package in my mind/memory folder could "
                "teach me some, but I don't have one yet."]
    if k == 4:
        return ["% skill", "> " + r.choice(["help", "what can you help me with?", "what can I ask you?"]),
                "! help", "= what I can do",
                "< I can remember what you tell me, do exact math and word problems, solve sequences and logic "
                "puzzles, convert units, count letters, work out clock times, tell the time and date, send you a "
                "reminder later, and read, write, move, rename and delete files and folders in my files folder. "
                "After an answer, ask me \"why?\" and I'll show you how I got it."]
    age = r.randint(18, 60)
    years = r.randint(2, 10)
    return ["% skill", "@ I am %d years old" % age, "> " + r.choice(["In %d years, how old will I be?" % years,
                                                               "how old will I be in %d years?" % years]),
            "! calc %d + %d" % (age, years), "= %d" % (age + years),
            "< You told me you're %d, so in %d years you'll be %d + %d = %d." % (age, years, age, years, age + years)]


def ep_people(g):
    """what other people told about themselves, asked by someone else"""
    r = g.r
    name = r.choice(["Sylwia", "Alex", "Maja", "Tom", "Ola", "Kuba"])
    key = name.lower()
    pet, pet_name = r.choice([("cat", "Mjolnir"), ("dog", "Rex"), ("parrot", "Kiwi"), ("hamster", "Bean")])
    k = r.randint(0, 2)
    if k == 0:
        return ["% skill", "> Does %s have a %s?" % (name, pet), "! person " + key,
                "= %s has a %s named %s" % (name, pet, pet_name),
                "< Yes. %s has a %s named %s. That's what %s told me." % (name, pet, pet_name, name)]
    if k == 1:
        return ["% skill", "> What is %s's %s's name?" % (name, pet), "! person " + key, "= " + pet_name,
                "< %s's %s is called %s. That's what %s told me." % (name, pet, pet_name, name)]
    other = r.choice(["dog", "snake", "horse"]) if pet != "dog" else "snake"
    return ["% skill", "> Does %s have a %s?" % (name, other), "! person " + key, "= not told",
            "< I don't know. %s never told me about that." % name]


FOLDER_NAMES = ["42", "work", "photos", "box", "music", "notes", "old", "projects", "games", "x"]
LATER_TIMES = [(5, "5 seconds"), (10, "10 seconds"), (3, "3 seconds"), (2, "2 minutes"), (1, "1 minute")]


def ep_work_later(g):
    """a change asked for later: it waits, and is done when its time comes"""
    r = g.r
    name = r.choice(FOLDER_NAMES)
    _, when = r.choice(LATER_TIMES)
    ask = r.choice(["Create a folder named %s %s later.", "In %s, make a folder called %s.",
                    "Make a folder called %s in %s.", "can you create me a folder named \"%s\" %s later?"])
    if ask.startswith("In "):
        ask = ask % (when, name)
    else:
        ask = ask % (name, when)
    return ["% skill", "> " + ask, "! mkdir " + name, "= later, in " + when,
            "< Okay, I'll make the folder %s in %s." % (name, when)]


def ep_work_fixed(g):
    """the trash had one with that name: put under a free name, and said"""
    r = g.r
    name = r.choice(FOLDER_NAMES)
    return ["% skill", "> " + r.choice(["Remove %s", "delete %s", "put %s in the trash", "Remove the folder %s"]) % name,
            "! delete " + name, "= moved to trash",
            "< The trash already has something called %s, so I put it in the trash as \"%s (2)\"." % (name, name)]


def ep_work_checked(g):
    """told it isn't done while it is: she looks again and says what she saw"""
    r = g.r
    name = r.choice(FOLDER_NAMES)
    files = r.choice(["welcome.txt", "notes.txt, welcome.txt", "photos (empty folder), welcome.txt"])
    return ["% skill", "> " + r.choice(["%s is still there", "Folder %s is still there!", "It is still there.",
                                        "you didn't delete %s"]).replace("%s", name),
            "! exists " + name, "= no",
            "< I just checked: %s isn't in your files any more. It's in the trash. I looked in your files folder, "
            "and this is what is there: %s." % (name, files)]


EPS_SKILLS = [(ep_why_calc, 4), (ep_word_problem, 3), (ep_why_seq, 3), (ep_logic, 3), (ep_order, 2),
              (ep_if_then, 2), (ep_units, 2), (ep_clock, 1), (ep_letters, 2), (ep_decimals, 1), (ep_ask, 3),
              (ep_know, 2), (ep_package, 1), (ep_repeat, 1), (ep_told, 2), (ep_arch, 1), (ep_plain_talk, 2),
              (ep_world_facts, 3), (ep_self_talk, 2), (ep_people, 1), (ep_work_later, 1), (ep_work_fixed, 1),
              (ep_work_checked, 1)]


if __name__ == "__main__":
    class G:
        def __init__(self, r):
            self.r = r
    g = G(random.Random(1))
    for fn, _ in EPS_SKILLS:
        for _ in range(2):
            lines = fn(g)
            if lines:
                print("\n".join(lines))
                print()
