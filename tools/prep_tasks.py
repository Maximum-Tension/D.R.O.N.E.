import json, os, random, re, sys, urllib.request


def self_name():
    try:
        root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
        path = os.path.join(root, "mind_original", "self.txt")
        if not os.path.exists(path):
            path = os.path.join(root, "brain", "self.txt")
        for ln in open(path, encoding="utf-8"):
            if ln.startswith("My name is "):
                return ln[11:].strip().rstrip(".") or "Lucy"
    except OSError:
        pass
    return "Lucy"


NAME = self_name()
LNAME = NAME.lower()

SGD = "https://raw.githubusercontent.com/google-research-datasets/dstc8-schema-guided-dialogue/master/train/dialogues_%03d.json"
TM1 = "https://raw.githubusercontent.com/google-research-datasets/Taskmaster/master/TM-1-2019/self-dialogs.json"
TM2 = "https://raw.githubusercontent.com/google-research-datasets/Taskmaster/master/TM-2-2020/data/%s.json"
TM2_FILES = ["flights", "food-ordering", "hotels", "movies", "music", "restaurant-search", "sports"]

DOMAINS = [
    (r"\b(games?|scores?|teams?|match|league|players?|sports?|nba|nfl)\b", "look up sports results"),
    (r"\b(restaurants?|food|pizzas?|coffee|table|eat|dinner|lunch|breakfast|menu|order|cuisine|latte)\b", "order food or book restaurants"),
    (r"\b(flights?|fly|airlines?|plane|bus|buses|trains?|tickets?|car rental|rent a car|rideshare|ride|uber|lyft|taxi|cab|trip|travel)\b", "book trips or tickets"),
    (r"\b(hotels?|rooms?|stay|airbnb|apartments?)\b", "book places to stay"),
    (r"\b(movies?|films?|cinema|theaters?|theatres?)\b", "find or book movies"),
    (r"\b(music|songs?|album|artist|playlist|radio|band)\b", "play music"),
    (r"\b(weather|rain|forecast|temperature|sunny|snow)\b", "check the weather"),
    (r"\b(alarms?|wake me)\b", "set alarms"),
    (r"\b(bank|money|pay|payment|transfer|balance|account)\b", "handle money or bank accounts"),
    (r"\b(calendar|events?|meeting|appointment|doctor|dentist|salon|haircut|stylist)\b", "make appointments or use your calendar"),
    (r"\b(repair|mechanic|oil change|tires?|brakes?)\b", "book car repairs"),
]

REPLIES = [
    "I can't {v}. I work offline on this computer, so I have no internet. I can remember the details for you if you want.",
    "Sorry, I can't {v}. I don't have internet access. Should I save a note about it?",
    "That's something I can't do. I can't {v}. I can remember things, do math, count and work with files on this computer.",
    "I'm not able to {v}. I only run here on this computer. Want me to remember it for you?",
    "Sorry, I can't {v}. I'm a small offline AI. I can keep a note of what you need, though.",
]

SKIP = re.compile(r"\b(time|date|remember|note|file|folder|count|calculate|plus|minus|times|divided|what day|your name|who are you)\b", re.I)


def get(url, path):
    if os.path.exists(path) and os.path.getsize(path) > 0:
        return path
    print("downloading", url)
    urllib.request.urlretrieve(url, path)
    return path


def clean(t):
    t = t.strip().replace("’", "'").replace("‘", "'").replace("“", '"').replace("”", '"')
    t = re.sub(r"\s+", " ", t)
    if not re.fullmatch(r"[A-Za-z0-9 ,.'!?$:/&-]+", t):
        return None
    w = t.split(" ")
    if len(w) < 4 or len(w) > 30:
        return None
    if SKIP.search(t):
        return None
    return t


def domain(t):
    for pat, v in DOMAINS:
        if re.search(pat, t, re.I):
            return v
    return None


def main():
    out = sys.argv[1] if len(sys.argv) > 1 else "data/tasks_honest.txt"
    cache = sys.argv[2] if len(sys.argv) > 2 else "data/_downloads"
    nsgd = int(sys.argv[3]) if len(sys.argv) > 3 else 127
    os.makedirs(cache, exist_ok=True)
    reqs = []
    for i in range(1, nsgd + 1):
        try:
            d = json.load(open(get(SGD % i, os.path.join(cache, "sgd_%03d.json" % i)), encoding="utf-8"))
        except Exception as e:
            print("skip", i, e)
            continue
        for dl in d:
            us = [t["utterance"] for t in dl["turns"] if t["speaker"] == "USER"]
            if us:
                reqs.append(us[0])
    for url, name in [(TM1, "tm1.json")] + [(TM2 % f, "tm2_%s.json" % f) for f in TM2_FILES]:
        try:
            d = json.load(open(get(url, os.path.join(cache, name)), encoding="utf-8"))
        except Exception as e:
            print("skip", name, e)
            continue
        for dl in d:
            us = [u.get("text", "") for u in dl.get("utterances", []) if u.get("speaker") == "USER"]
            for u in us[:1]:
                reqs.append(u)
    r = random.Random(17)
    seen = set()
    convs = []
    for t in reqs:
        c = clean(t)
        if not c or c.lower() in seen:
            continue
        v = domain(c)
        if not v:
            continue
        seen.add(c.lower())
        rep = r.choice(REPLIES).format(v=v)
        lines = []
        if r.random() < 0.8:
            lines.append("# My name is %s." % NAME)
        lines += ["> " + c, "< " + rep]
        c2 = re.sub(r"^(hi|hello|hey|hi there|hello there)[,.!]? ", "", c, flags=re.I)
        want = re.match(r"^(i want|i need|i would like|i'd like|i'm looking|i am looking|i wanna)\b", c2, re.I)
        if want and r.random() < 0.35:
            lines += ["> " + r.choice(["yes please", "ok, remember it", "sure, save it", "yes, remember that", "ok do that"]),
                      "! remember " + re.sub(r"[.!?]+$", "", c2),
                      "= saved",
                      "< " + r.choice(["Okay, I saved it.", "Done. I will remember that.", "Saved."])]
        elif r.random() < 0.3:
            lines += ["> " + r.choice(["no thanks", "never mind", "oh ok", "that's fine"]), "< " + r.choice(["Okay! Anything else?", "Alright. I'm here if you need me.", "Okay."])]
        convs.append("\n".join(lines))
    r.shuffle(convs)
    convs = convs[:6000]
    os.makedirs(os.path.dirname(out) or ".", exist_ok=True)
    with open(out, "w", encoding="utf-8") as f:
        f.write("\n\n".join(convs) + "\n")
    print("wrote %s: %d conversations from %d real requests" % (out, len(convs), len(reqs)))


main()
