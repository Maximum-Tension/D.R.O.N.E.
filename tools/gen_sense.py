import random, sys, os, argparse, subprocess, datetime

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def self_name():
    # mind_original/self.txt (brain/self.txt before the mind folders)
    for where in (("mind_original", "self.txt"), ("brain", "self.txt")):
        try:
            for ln in open(os.path.join(ROOT, *where), encoding="utf-8"):
                if ln.startswith("My name is "):
                    return ln[11:].strip().rstrip(".") or "Lucy"
        except OSError:
            continue
    return "Lucy"


NAME = self_name()
LNAME = NAME.lower()

HEAD = ["# My name is %s." % NAME, "# I am a small AI. I run offline on this computer."]

WORDS = """notes ideas plan todo list report draft letter story poem budget recipes music photos games work school taxes travel home old new
archive backup stuff misc docs papers journal diary memo summary invoice receipts chores groceries shopping songs videos books movies
projects homework essay chapter final cover resume contacts passwords birthdays garden kitchen pets cats dogs friends family holiday
trip camping fishing football chess math science history art code tests data logs results scores levels maps keys""".split()
FOLDERS = """docs music photos games work school archive backup old new stuff misc papers projects lists finance travel home pics notes inbox
outbox drafts final temp private shared art books videos recipes trips""".split()
SYL_C = list("bdfgklmnprstvz") + ["bl", "br", "dr", "fl", "gr", "kr", "pl", "sk", "sn", "st", "tr", "zh", "ch", "sh"]
SYL_V = ["a", "e", "i", "o", "u", "oo", "ee", "ai", "ou"]
SYL_E = ["", "", "", "n", "k", "p", "r", "x", "sh", "m", "t", "l", "rk", "mp"]
NUMW = "zero one two three four five six seven eight nine ten eleven twelve thirteen fourteen fifteen sixteen seventeen eighteen nineteen twenty".split()
CHAT = [("hi", ["Hi!", "Hello!", "Hey there!"]), ("how are you?", ["I'm good, thanks!", "Doing well. How about you?"]),
        ("thanks", ["You're welcome!", "Anytime."]), ("cool", ["Glad you like it.", "Nice!"]), ("lol", ["Ha!", ":)"]),
        ("good morning", ["Good morning!", "Morning! What are we doing today?"]), ("nice", ["Thanks!"]), ("ok", ["Okay."])]
NONSENSE = ["I like the color of the stars.", "I will.", "Bananas are blue on Tuesdays.", "My uncle sells boats.", "I just got back from the beach.",
            "That's a great offer.", "I need to see the receipt.", "I'm glad you liked it.", "The train was late again.", "I have three dogs."]
CANT = ["fly", "swim", "cook", "drive a car", "sing", "dance", "send emails", "browse the internet", "make coffee", "call people", "play football",
        "paint", "see you", "walk", "eat"]
BODYW = "I don't have a body"
OFFW = "I run offline on this computer"
CANT2 = [("fly", "can fly", BODYW), ("swim", "can swim", BODYW), ("cook", "can cook", BODYW), ("drive a car", "can drive car", BODYW), ("sing", "can sing", BODYW),
         ("dance", "can dance", BODYW), ("walk", "can walk", BODYW), ("eat", "can eat", BODYW), ("send emails", "can send emails", OFFW),
         ("browse the internet", "can browse internet", OFFW), ("call people", "can call people", OFFW), ("play football", "can play", None),
         ("paint", "can paint", None), ("make coffee", "can make", None)]
CANDO = [("move", "move files", "yes: move name.txt to folder"), ("delete", "delete files", "yes: delete name.txt"),
         ("rename", "rename files", "yes: rename old.txt to new"), ("create", "make folders", "yes: write name.txt / mkdir name"),
         ("read", "open files", "yes: read name.txt"), ("restore", "bring back deleted files", "yes: restore name.txt"),
         ("calc", "do math", "yes: calc 1 + 2"), ("remember", "remember things", "yes: remember fact")]


def nonce(r):
    s = ""
    for _ in range(r.choice([2, 2, 3])):
        s += r.choice(SYL_C) + r.choice(SYL_V)
    return s + r.choice(SYL_E)


def fmt(v):
    if abs(v - round(v)) < 1e-9:
        return str(int(round(v)))
    return ("%.6f" % v).rstrip("0").rstrip(".")


def numw(r, n):
    if 0 <= n <= 20 and r.random() < 0.5:
        return NUMW[n]
    return str(n)


class G:
    def __init__(self, r):
        self.r = r

    def fname(self, ext=True):
        r = self.r
        w = r.choice(WORDS) if r.random() < 0.7 else nonce(r)
        if r.random() < 0.2:
            w += str(r.randint(1, 9))
        return w + ".txt" if ext else w

    def folder(self):
        r = self.r
        return r.choice(FOLDERS) if r.random() < 0.75 else nonce(r)

    def c(self, *opts):
        return self.r.choice(opts)


def ep_missing(g):
    r = g.r
    k = r.randint(0, 4)
    if k == 0:
        f = g.folder()
        u = g.c("make a folder", "create a folder", "can you make a folder?", "I need a new folder", "make me a new folder please", "new folder")
        a = g.c(f, "call it " + f, "name it " + f, f + " please", "let's call it " + f)
        return ["% world welcome.txt", "> " + u, "! need mkdir", "= what name",
                "< " + g.c("Sure. What should I call it?", "What name should the folder have?", "Okay! What name?", "What do you want to call it?"),
                "> " + a, "! mkdir " + f, "= folder made", "< " + g.c("Done, the folder %s is ready." % f, "I made the folder %s." % f, "Okay, %s is made." % f)]
    if k == 1:
        n = g.fname()
        u = g.c("make a file", "create a file", "can you make a new file?", "make me a text file", "I want a new file", "create an empty file")
        a = g.c(n, "call it " + n[:-4], "name it " + n, n[:-4])
        return ["% world welcome.txt", "> " + u, "! need write", "= what name",
                "< " + g.c("What should I name the file?", "Sure. What name?", "What do you want the file to be called?"),
                "> " + a, "! write " + n, "= written", "< " + g.c("Done, I created %s." % n, "Okay, %s is made." % n, "I made %s." % n)]
    if k == 2:
        n, f = g.fname(), g.folder()
        u = g.c("move %s" % n, "put %s somewhere else" % n if False else "move %s please" % n, "can you move %s?" % n)
        a = g.c(f, "to " + f, "into " + f, "put it in " + f)
        return ["%% world %s %s/" % (n, f), "> " + u, "! need move", "= which folder",
                "< " + g.c("Which folder should it go to?", "Where should I move it?", "Sure. Into which folder?"),
                "> " + a, "! move %s to %s" % (n, f), "= moved", "< " + g.c("Done, %s is in %s now." % (n, f), "I moved %s to %s." % (n, f))]
    if k == 3:
        n, f = g.fname(), g.folder()
        u = g.c("move file to folder", "move a file into a folder", "put the file in the folder", "move it to the folder")
        return ["%% world %s %s/" % (n, f), "> " + u, "! need move", "= which file, which folder",
                "< " + g.c("Which file? And to which folder?", "Sure, but which file, and which folder should it go to?",
                           "Which file do you mean, and where should it go?", "What? Which file? And to what folder? Could you give more details?"),
                "> %s to %s" % (n, f), "! move %s to %s" % (n, f), "= moved", "< " + g.c("Done. %s is in %s." % (n, f), "Okay, I moved %s into %s." % (n, f))]
    n = g.fname()
    u = g.c("delete it", "remove it", "get rid of it", "delete the file", "trash it")
    return ["% world " + n, "> " + u, "! need delete", "= which one",
            "< " + g.c("Which one do you mean?", "Which file should I delete?", "Delete what? Which one?"),
            "> " + n, "! delete " + n, "= moved to trash", "< " + g.c("Done, %s is in the trash." % n, "I deleted %s. It is in the trash if you need it back." % n)]


def ep_newfolder(g):
    n, f = g.fname(), g.folder()
    u = g.c("put %s into %s" % (n, f), "move %s to %s" % (n, f), "move %s into the %s folder" % (n, f))
    if g.r.random() < 0.6:
        return ["% world " + n, "> " + u, "! move %s to %s" % (n, f), "= no such folder", "! mkdir " + f, "= folder made", "! move %s to %s" % (n, f), "= moved",
                "< " + g.c("There was no %s folder, so I made it and moved %s there." % (f, n), "Done. I had to make the %s folder first." % f,
                           "I created %s and put %s in it." % (f, n))]
    near = f + "s" if not f.endswith("s") else f[:-1]
    if len(near) < 3:
        near = f + "s"
    out = ["%% world %s %s/" % (n, near), "> " + u, "! ask move %s to %s" % (n, near), "= there is no folder %s, did you mean %s?" % (f, near),
           "< " + g.c("There is no %s folder. Did you mean %s?" % (f, near), "I don't have %s, but I have %s. Is that the one?" % (f, near))]
    if g.r.random() < 0.7:
        out += ["> " + g.c("yes", "yes please", "yep, that one", "sure"), "! move %s to %s" % (n, near), "= moved", "< " + g.c("Done, %s is in %s." % (n, near), "Okay, I moved it there.")]
    else:
        out += ["> " + g.c("no", "nope", "no, never mind"), "! ask move %s to %s" % (n, near), "= ok, I won't", "< " + g.c("Okay, I left it where it is.", "Alright, I won't move it.")]
    return out


def ep_unknown(g):
    w = nonce(g.r)
    u = g.c("where is the %s?" % w, "what is a %s?" % w, "what does %s mean?" % w, "is a %s bigger than a car?" % w, "tell me about the %s" % w,
            "where can I find the %s?" % w, "who invented the %s?" % w)
    return ["% unknown " + w, "> " + u, "! know " + w, "= never heard of it",
            "< " + g.c("%s? What's that?" % w.capitalize(), "I've never heard of a %s. What is it?" % w, "%s? I don't know that word. Can you tell me what it is?" % w.capitalize())]


def ep_claim(g):
    r = g.r
    a, b = r.randint(1, 12), r.randint(1, 12)
    op = r.choice(["+", "*", "-"])
    words = {"+": g.c("plus", "+", "and"), "*": g.c("times", "*", "x"), "-": g.c("minus", "-")}[op]
    if op == "-" and b > a:
        a, b = b, a
    true = {"+": a + b, "*": a * b, "-": a - b}[op]
    wrong = true + r.choice([-2, -1, 1, 2, 3])
    if wrong < 0:
        wrong = true + 1
    e = "%d %s %d" % (a, op, b)
    def say(v):
        return "%s %s %s %s %s" % (numw(r, a), words, numw(r, b), g.c("is", "equals", "is equal to"), numw(r, v))
    if r.random() < 0.25:
        return ["> " + say(true).capitalize() + g.c(".", "!", ""), "! check %s = %s" % (e, fmt(true)), "= right",
                "< " + g.c("Yes, that's right.", "Correct! %s is %s." % (e, fmt(true)), "That's right.")]
    out = ["> " + say(wrong).capitalize() + g.c(".", "!", ""), "! check %s = %s" % (e, fmt(wrong)), "= wrong: %s = %s" % (e, fmt(true)),
           "< " + g.c("No, %s is %s, not %s." % (e, fmt(true), fmt(wrong)), "That's not right. %s is %s." % (e, fmt(true)),
                      "No, you are wrong. %s is %s. Not %s." % (e, fmt(true), fmt(wrong)))]
    for _ in range(r.randint(0, 2)):
        out += ["> " + g.c("no it is %s" % numw(r, wrong), "wrong, it's %s" % numw(r, wrong), "no it is not", "trust me, it is %s" % numw(r, wrong)),
                "! check %s = %s" % (e, fmt(wrong)), "= still wrong: %s = %s" % (e, fmt(true)),
                "< " + g.c("I checked again: %s is %s, not %s." % (e, fmt(true), fmt(wrong)), "Sorry, but %s is %s. I'm sure." % (e, fmt(true)),
                           "No, %s is still %s." % (e, fmt(true)))]
    if r.random() < 0.5:
        out += ["> " + g.c("ok fine, it's %s" % numw(r, true), "okay, %s" % say(true)), "! check %s = %s" % (e, fmt(true)), "= right",
                "< " + g.c("Yes, that's right!", "Right, %s is %s." % (e, fmt(true)))]
    return out


def ep_can(g):
    r = g.r
    if r.random() < 0.5:
        v, what, res = r.choice(CANDO)
        u = g.c("can you %s?" % what, "do you know how to %s?" % what, "are you able to %s?" % what)
        return ["> " + u, "! can " + v, "= " + res, "< " + g.c("Yes, I can %s." % what, "Yes! Just tell me what to do.", "Yes, I can do that.")]
    what, act, why = r.choice(CANT2)
    form = r.randint(0, 2)
    u = ("can you %s?", "do you know how to %s?", "are you able to %s?")[form] % what
    no = ("No, I can't.", "No, I don't.", "No, I'm not.")[form]
    if why:
        res = "no: " + why
        return ["> " + u, "! " + act, "= " + res, "< %s %s." % (no, why)]
    return ["> " + u, "! " + act, "= no", "< " + g.c(no, "%s I can work with files, do math and remember things." % no)]


def ep_mywords(g):
    s = g.r.choice(NONSENSE)
    u = g.c("what do you mean?", "huh?", "what?", "sorry?", "what do you mean by that?", "why did you say that?")
    return ["> " + g.c("hey", "hi there", "how was your day", "what are you up to?"), "< " + s, "> " + u,
            "! my words", '= "%s" (not based on anything)' % s,
            "< " + g.c("Sorry, that didn't make sense. I'm still learning.", "Sorry, I just said that. It wasn't based on anything.",
                       "I said \"%s\" but it didn't mean anything. Sorry." % s)]


def ep_actions_talk(g):
    r = g.r
    f = g.folder()
    out = ["% world welcome.txt", "> make a folder called " + f, "! mkdir " + f, "= folder made", "< Done, the folder %s is ready." % f]
    k = r.randint(0, 2)
    if k == 0:
        out += ["> " + g.c("what did you just do?", "what did you do just now", "what was that last thing you did?"),
                "! my last action", "= mkdir %s (folder made)" % f, "< " + g.c("I made a folder called %s." % f, "I created the folder %s." % f)]
    elif k == 1:
        out += ["> " + g.c("why did you do that?", "and why did you do that?"), "! why mkdir " + f, "= you asked me to",
                "< " + g.c("Because you asked me to make it.", "You asked me to, so I made %s." % f)]
    else:
        out += ["> did you make the %s folder?" % f, "! did create " + f, "= yes", "< " + g.c("Yes, I made it.", "Yes, %s is there now." % f)]
    return out


def ep_mistakes(g):
    r = g.r
    k = r.randint(0, 3)
    if k == 0:
        n = g.fname()
        return ["% world " + n, "> delete " + n, "! delete " + n, "= moved to trash", "< Done, %s is in the trash." % n,
                "> " + g.c("no wait, I still need it", "oops, bring it back", "undo that", "no no, I needed %s" % n),
                "! restore " + n, "= back from the trash", "< " + g.c("Okay, %s is back." % n, "I brought %s back from the trash." % n)]
    if k == 1:
        n, f = g.fname(), g.folder()
        return ["%% world %s %s/" % (n, f), "> move %s to %s" % (n, f), "! move %s to %s" % (n, f), "= moved", "< Done.",
                "> " + g.c("no, you moved the wrong file", "hey, %s wasn't supposed to go in %s" % (n, f), "that's not what I wanted"),
                "! mistake move %s to %s" % (n, f), "= should I move it back?", "< " + g.c("Sorry! Should I move it back?", "Oops. Want me to move %s back?" % n),
                "> " + g.c("yes", "yes please", "put it back"), "! move %s/%s to ." % (f, n), "= moved", "< " + g.c("Done, it's back.", "%s is back where it was." % n)]
    if k == 2:
        n, m2 = g.fname(), g.fname()
        if m2 == n:
            m2 = "x" + m2
        return ["% world welcome.txt", "> make a file called " + n, "! write " + n, "= written", "< I made %s." % n,
                "> " + g.c("sorry, I meant %s" % m2, "no, I meant %s" % m2, "I meant %s" % m2), "! rename %s to %s" % (n, m2), "= renamed",
                "< " + g.c("Okay, I renamed it to %s." % m2, "Fixed: it's %s now." % m2)]
    n = g.fname()
    return ["% world welcome.txt", "> make a file called " + n, "! write " + n, "= written", "< I made %s." % n,
            "> " + g.c("I didn't ask you to make %s" % n, "who told you to make %s?" % n),
            "! mistake write " + n, "= should I delete %s?" % n, "< " + g.c("Sorry about that. Should I delete %s?" % n, "My mistake. Want me to delete %s?" % n),
            "> " + g.c("yes", "yes delete it", "sure"), "! delete " + n, "= moved to trash", "< Done, %s is in the trash." % n]


def ep_goal(g):
    n, f = g.fname(), g.folder()
    return ["%% world %s %s/" % (n, f), "> move " + n, "! need move", "= which folder", "< Which folder should it go to?",
            "> " + g.c("hello", "hmm wait", "how are you?"), "< " + g.c("Hi!", "Sure.", "I'm good."),
            "> " + g.c("what were we doing?", "what was I asking you?", "where were we?"), "! goal", "= move %s (needs: which folder)" % n,
            "< " + g.c("We were moving %s. Which folder should it go to?" % n, "You wanted to move %s, but I still need a folder." % n),
            "> " + f, "! move %s to %s" % (n, f), "= moved", "< Done, %s is in %s." % (n, f)]


def ep_find(g):
    n, f = g.fname(), g.folder()
    return ["%% world %s/%s" % (f, n), "> " + g.c("where is %s?" % n, "where's %s" % n, "where did you put %s?" % n), "! find " + n, "= %s/%s" % (f, n),
            "< " + g.c("It's in %s." % f, "%s is in the %s folder." % (n, f))]


def ep_refs(g):
    r = g.r
    f, n = g.folder(), g.fname()
    k = r.randint(0, 2)
    if k == 0:
        return ["% world welcome.txt", "> " + g.c("make a folder called %s. then create %s in it." % (f, n), "make a folder named %s and create %s inside it" % (f, n)),
                "! mkdir " + f, "= folder made", "! write %s/%s" % (f, n), "= written", "< " + g.c("Done. I made %s and created %s in it." % (f, n), "Okay, %s/%s is ready." % (f, n))]
    if k == 1:
        return ["% world welcome.txt " + n, "> make a folder called " + f, "! mkdir " + f, "= folder made", "< The folder %s is ready." % f,
                "> " + g.c("now put %s inside that folder" % n, "move %s into it" % n, "put %s in there" % n),
                "! move %s to %s" % (n, f), "= moved", "< Done, %s is in %s." % (n, f)]
    n2 = g.fname()
    if n2 == n:
        n2 = "y" + n2
    return ["% world welcome.txt", "> make a file called " + n, "! write " + n, "= written", "< I made %s." % n,
            "> make another one called " + n2[:-4], "! write " + n2, "= written", "< I made %s." % n2,
            "> delete the " + g.c("first", "second") + " one"]


def ep_task(g):
    return ["> " + g.c("ok do it now", "do it", "go ahead and do it"), "! need task", "= which task?",
            "< " + g.c("Which task do you mean?", "Do what? Tell me what you want me to do.")]


NAMES = """Bora Ayla Max Emily Lucy Owen Jackson Olivia Elias Hakan Mia Noah Liam Emma Zeynep Deniz Kemal Sara Leo Nina Omar Yuki Ivan Rosa Theo Lena Arda Ece Can Selin""".split()
SURN = """Mendelli Kaya Smith Brown Novak Tanaka Garcia Silva Ozturk Fischer Rossi Petrov Larsen Moreau Costa Yilmaz Walsh Dubois""".split()
CITIES = """Izmir Ankara Berlin Paris Lisbon Oslo Tokyo Cairo Lima Boston Denver Madrid Vienna Prague Dublin Seoul Austin Leeds""".split()
JOBS = ["a teacher", "a nurse", "a driver", "a cook", "a farmer", "a student", "an engineer", "a painter", "a sailor", "a doctor", "a baker", "a pilot"]
LIKES = ["pizza", "chess", "music", "cats", "reading", "football", "tea", "painting", "hiking", "puzzles", "coffee", "movies"]
RELS = ["sister", "brother", "mom", "dad", "uncle", "aunt", "friend", "cousin", "wife", "husband", "son", "daughter", "boss", "dog", "cat"]
COUNTRIES = """australia france japan brazil canada egypt peru norway kenya chile spain italy""".split()
SMALLTALK = [("how are you?", "I'm good, thanks!"), ("lol", "Ha!"), ("cool", "Glad you like it."), ("thanks", "You're welcome!"), ("nice", "Thanks!")]


def ep_profile(g):
    r = g.r
    k = r.randint(0, 6)
    out = ["% world welcome.txt"]
    if k == 0:
        n = r.choice(NAMES)
        out += ["> " + g.c("my name is %s" % n, "I'm %s" % n, "call me %s" % n, "hi, I'm %s" % n),
                "< " + g.c("Nice to meet you, %s!" % n, "Hi %s!" % n, "Hello %s, nice to meet you." % n)]
        ask, act, val, ans = g.c("what's my name?", "what is my name?", "who am I?", "do you remember my name?"), "you name", n, g.c("Your name is %s." % n, "You are %s." % n, "%s." % n)
    elif k == 1:
        c = r.choice(CITIES)
        out += ["> " + g.c("I'm from %s" % c, "I come from %s" % c), "< " + g.c("Oh, %s! Nice." % c, "Cool, %s." % c)]
        ask, act, val, ans = g.c("where am I from?", "where do I come from?"), "you from", c, g.c("You are from %s." % c, "You told me you are from %s." % c)
    elif k == 2:
        c = r.choice(CITIES)
        out += ["> " + g.c("I live in %s" % c, "I live in %s now" % c), "< " + g.c("Nice, %s." % c, "Okay, %s." % c)]
        ask, act, val, ans = g.c("where do I live?", "where do I live again?"), "you home", c, g.c("You live in %s." % c, "In %s." % c)
    elif k == 3:
        j = r.choice(JOBS)
        out += ["> " + g.c("I'm %s" % j, "I work as %s" % j), "< " + g.c("That sounds nice.", "Cool!")]
        ask, act, val, ans = g.c("what do I do?", "what do I do for work?"), "you job", j, g.c("You are %s." % j, "You told me you are %s." % j)
    elif k == 4:
        l = r.choice(LIKES)
        out += ["> " + g.c("I like %s" % l, "I love %s" % l, "I really like %s" % l), "< " + g.c("Nice, %s is great." % l, "Good to know!")]
        ask, act, val, ans = g.c("what do I like?", "what do I love?"), "you likes", l, g.c("You like %s." % l, "You told me you like %s." % l)
    elif k == 5:
        rel, n = r.choice(RELS), r.choice(NAMES)
        out += ["> " + g.c("my %s is called %s" % (rel, n), "my %s's name is %s" % (rel, n), "%s is my %s" % (n, rel)), "< " + g.c("Nice name!", "Okay, %s." % n)]
        ask, act, val, ans = g.c("what's my %s's name?" % rel, "what is my %s's name?" % rel, "who is my %s?" % rel), "your %s name" % rel, n, g.c("Your %s is %s." % (rel, n), "%s." % n)
    else:
        a = r.randint(12, 80)
        out += ["> " + g.c("I'm %d" % a, "I am %d years old" % a if False else "I'm %d" % a), "< " + g.c("Nice!", "Okay.")]
        ask, act, val, ans = g.c("how old am I?", "how old am I again?"), "you age", str(a), g.c("You are %d." % a, "You told me you are %d." % a)
    for _ in range(r.randint(0, 2)):
        u, s2 = r.choice(SMALLTALK)
        out += ["> " + u, "< " + s2]
    return out + ["> " + ask, "! " + act, "= " + val, "< " + ans]


def ep_profile_none(g):
    r = g.r
    k = r.randint(0, 4)
    if k == 0:
        return ["> " + g.c("what's my name?", "do you know my name?", "who am I?"), "! you name", "= you did not tell me",
                "< " + g.c("You haven't told me your name yet. What is it?", "I don't know your name yet. What should I call you?")]
    if k == 1:
        return ["> " + g.c("how old am I?", "do you know my age?"), "! you age", "= you did not tell me",
                "< " + g.c("You haven't told me your age yet.", "I don't know your age. How old are you?")]
    if k == 2:
        return ["> " + g.c("where do I live?", "do you know where I live?"), "! you home", "= you did not tell me",
                "< " + g.c("You haven't told me where you live yet.", "I don't know. Where do you live?")]
    if k == 3:
        rel = r.choice(RELS)
        return ["> " + g.c("what's my %s's name?" % rel, "what is my %s's name?" % rel), "! your %s name" % rel, "= you did not tell me",
                "< " + g.c("You haven't told me your %s's name yet." % rel, "I don't know your %s's name. What is it?" % rel)]
    th = g.c("food", "color", "song", "movie")
    return ["> what's my favorite %s?" % th, "! you favorite " + th, "= you did not tell me", "< " + g.c("You haven't told me your favorite %s yet." % th, "I don't know yet. What is it?")]


def ep_world(g):
    r = g.r
    k = r.randint(0, 3)
    if k == 0:
        c = r.choice(COUNTRIES)
        return ["> " + g.c("what is the capital of %s?" % c, "what's the capital of %s?" % c), "! know capital of " + c, "= nothing known",
                "< " + g.c("I don't know. I don't have facts about %s." % c.capitalize(), "I don't know that. I work offline and I don't have that fact.")]
    if k == 1:
        return ["> " + g.c("what's the weather like today?", "how's the weather?", "is it raining?", "what's the weather?"), "! know weather", "= nothing known: " + OFFLINE,
                "< I don't know. %s, so I can't check the weather." % OFFLINE]
    if k == 2:
        return ["> " + g.c("what's the news?", "any news today?", "what are the headlines?"), "! know news", "= nothing known: " + OFFLINE,
                "< I don't know. %s, so I can't check the news." % OFFLINE]
    n = "%s %s" % (r.choice(NAMES), r.choice(SURN))
    return ["> " + g.c("who is %s?" % n, "who's %s?" % n), "! know " + n.lower(), "= nothing known",
            "< " + g.c("I don't know who %s is." % n, "I don't know anything about %s." % n)]


def ep_history(g):
    r = g.r
    n = g.fname()
    first = g.c("hi there", "hello", "hey code")
    out = ["% world welcome.txt", "> " + first, "< " + g.c("Hi!", "Hello!")]
    q1 = g.c("can you count?", "do you like music?", "are you there?")
    out += ["> " + q1, "< " + g.c("Yes.", "Sure.", "I'm here.")]
    out += ["> make a file called " + n, "! write " + n, "= written", "< I made %s." % n]
    k = r.randint(0, 4)
    if k == 0:
        return out + ["> " + g.c("what did I just say?", "what did I just ask?", "what was my last message?"), "! your last words", '= "make a file called %s"' % n,
                      "< " + g.c('You said: "make a file called %s".' % n, 'You asked me to make a file called %s.' % n)]
    if k == 1:
        return out + ["> " + g.c("what was my first question?", "what did I ask you first?"), "! your first words", '= "%s"' % first,
                      "< " + g.c('First you said: "%s"' % first, 'You said: "%s"' % first)]
    if k == 2:
        return out + ["> " + g.c("what was my first message?", "what was the first thing I said?"), "! your first words", '= "%s"' % first,
                      "< " + g.c('You said: "%s"' % first, 'Your first message was "%s".' % first)]
    if k == 3:
        return out + ["> " + g.c("what was the last thing I asked you to do?", "what did I ask you to do?"), "! your last request",
                      '= "make a file called %s" -> write %s (written)' % (n, n), "< " + g.c("You asked me to make %s, and I made it." % n, "To make a file called %s. It's done." % n)]
    w = g.c("pizza", "banana", "goodbye", "chess")
    return out + ["> " + g.c("did I say %s?" % w, "when did I say %s?" % w), '! you said "%s"' % w, "= no",
                  "< " + g.c("No, you didn't say %s." % w, "I don't see that. You never said %s." % w)]


def ep_recheck(g):
    r = g.r
    a, b = r.randint(2, 30), r.randint(2, 12)
    op = r.choice(["+", "*", "-"])
    v = {"+": a + b, "*": a * b, "-": a - b}[op]
    word = {"+": "plus", "*": "times", "-": "minus"}[op]
    e = "%d %s %d" % (a, op, b)
    out = ["> what's %d %s %d?" % (a, word, b), "! calc " + e, "= " + fmt(v), "< %d %s %d is %s." % (a, word, b, fmt(v))]
    k = r.randint(0, 2)
    if k == 0:
        return out + ["> " + g.c("are you sure?", "is that right?", "you sure?", "is that correct?"), "! check %s = %s" % (e, fmt(v)), "= right",
                      "< " + g.c("Yes, I checked: %s is %s." % (e, fmt(v)), "Yes, I'm sure. %s is %s." % (e, fmt(v)))]
    c = r.randint(2, 5)
    o2, w2 = r.choice([("*", "times"), ("+", "plus"), ("-", "minus")])
    e2 = "(%s) %s %d" % (e, o2, c)
    v2 = {"+": v + c, "*": v * c, "-": v - c}[o2]
    return out + ["> " + g.c("and %s %s?" % (w2, numw(r, c)), "now %s %d" % (w2, c), "what about %s %d?" % (w2, c)), "! calc " + e2, "= " + fmt(v2),
                  "< " + g.c("That makes %s." % fmt(v2), "%s." % fmt(v2), "Then it's %s." % fmt(v2))]


def ep_exists(g):
    r = g.r
    n = g.fname()
    if r.random() < 0.5:
        return ["% world " + n, "> delete " + n, "! delete " + n, "= moved to trash", "< Done, %s is in the trash." % n,
                "> " + g.c("bring it back", "restore it"), "! restore " + n, "= back from the trash", "< %s is back." % n,
                "> " + g.c("is it back?", "is it there now?"), "! find " + n, "= " + n, "< " + g.c("Yes, %s is back." % n, "Yes, it's there.")]
    m = g.fname()
    if m == n:
        m = "z" + m
    if r.random() < 0.5:
        return ["% world " + n, "> does %s exist?" % n, "! find " + n, "= " + n, "< " + g.c("Yes, %s is there." % n, "Yes, it exists.")]
    return ["% world " + n, "> does %s exist?" % m, "! find " + m, "= not found", "< " + g.c("No, I can't find %s." % m, "No, there is no %s." % m)]


def ep_append(g):
    n = g.fname()
    t = g.c("buy milk", "call mom", "meeting at 5", "feed the cat", "pay rent", "hello world")
    return ["% world welcome.txt", "> make a file called " + n, "! write " + n, "= written", "< I made %s." % n,
            "> " + g.c('write "%s" in it' % t, 'add "%s" to it' % t, 'put "%s" in it' % t), "! append %s %s" % (n, t), "= added",
            "< " + g.c("Done, I added it to %s." % n, "Okay, %s now has that line." % n)]


ACKS = [("indeed", ["Yes.", "Right."]), ("sure", ["Okay.", "Great."]), ("fair enough", ["Okay.", "Alright."]), ("ok", ["Okay."]), ("cool cool", ["Glad you like it.", "Okay!"]),
        ("got it", ["Great."]), ("makes sense", ["Good."]), ("yeah", ["Okay."]), ("right", ["Okay."]), ("nice one", ["Thanks!"]), ("no worries", ["Okay!", "Thanks."]),
        ("lol", ["Ha!", ":)"]), ("haha ok", ["Okay!"]), ("ok just checking", ["I'm here."]), ("you're smart", ["Thanks!", "Thank you!"]), ("good job", ["Thanks!", "Glad I could help."]),
        ("you're useless", ["Sorry. Tell me what went wrong and I'll fix it."]), ("sorry", ["No problem.", "That's okay."]), ("thanks a lot", ["You're welcome!"]),
        ("aww", ["Aww."]), ("wow", ["Right?"]), ("hmm", ["Take your time."]), ("never mind", ["Okay."])]


def ep_social(g):
    r = g.r
    out = []
    if r.random() < 0.5:
        n = g.fname()
        out += ["% world welcome.txt", "> make a file called " + n, "! write " + n, "= written", "< I made %s." % n]
    for _ in range(r.randint(1, 3)):
        u, reps = r.choice(ACKS)
        out += ["> " + u, "< " + r.choice(reps)]
    return out


def ep_lead(g):
    r = g.r
    f, n = g.folder(), g.fname()
    if r.random() < 0.6:
        return ["% world welcome.txt", "> make a folder called " + f, "! mkdir " + f, "= folder made", "< The folder %s is ready." % f,
                "> " + g.c("Now in this folder, can you create me a file called %s?" % n, "inside that folder, make %s please" % n, "in %s, create %s" % (f, n)),
                "! write %s/%s" % (f, n), "= written", "< " + g.c("Done, I made %s inside %s." % (n, f), "%s/%s is ready." % (f, n))]
    return ["%% world %s %s/" % (n, f), "> " + g.c("%s is not inside the folder %s" % (n, f), "you made %s but it is not in %s" % (n, f)),
            "! move %s to %s" % (n, f), "= moved", "< " + g.c("Sorry! I moved %s into %s." % (n, f), "Fixed, %s is in %s now." % (n, f))]


def ep_askme(g):
    return ["> " + g.c("ask me a question", "Ask me a question.", "ask me something", "can you ask me anything?" if False else "ask me anything"), "! can ask",
            "= no: I only ask when I need something for a task",
            "< " + g.c("Sorry, I can't do that. There is nothing in my mind to ask you about.", "I only ask when I need something for a task, so I have no question right now.")]


def ep_cancel(g):
    k = g.r.randint(0, 2)
    if k == 0:
        return ["% world welcome.txt", "> " + g.c("make a file", "create a file for me"), "! need write", "= what name", "< What should I call it?",
                "> " + g.c("Please stop that", "never mind", "forget it", "cancel that", "stop"), "! ask write a file", "= ok, I won't",
                "< " + g.c("Okay, I won't make it.", "Alright, never mind then.")]
    n = g.fname()
    if k == 1:
        return ["% world welcome.txt", "> make a file called " + n, "! write " + n, "= written", "< I made %s." % n,
                "> " + g.c("No!", "Don't create %s" % n, "no no"), "! mistake write " + n, "= should I delete %s?" % n,
                "< " + g.c("Sorry! Should I delete %s?" % n, "Oops. Want me to delete %s?" % n)]
    f = g.folder()
    near = n[:-4] + "s.txt" if not n[:-4].endswith("s") else n[:-5] + ".txt"
    if len(near) < 6:
        near = "x" + n
    return ["%% world %s %s/" % (n, f), "> " + g.c("Can you put the file %s inside %s folder?" % (near, f), "can you now move the file called %s to %s?" % (near, f)),
            "! ask move %s to %s" % (n, f), "= there is no %s, did you mean %s?" % (near, n), "< There is no %s. Did you mean %s?" % (near, n),
            "> " + g.c("Oh sorry, it is %s not %s. My bad." % (n, near), "yes, %s" % n, "%s, sorry" % n), "! move %s to %s" % (n, f), "= moved",
            "< " + g.c("No problem. %s is in %s now." % (n, f), "Done, I moved %s to %s." % (n, f))]


MONTHS = ["january", "february", "march", "april", "may", "june", "july", "august", "september", "october", "november", "december"]
WDAYS = ["monday", "tuesday", "wednesday", "thursday", "friday", "saturday", "sunday"]
HOLI = [("christmas", 12, 25), ("halloween", 10, 31), ("new year", 1, 1), ("valentine's day", 2, 14), ("christmas eve", 12, 24), ("new year's eve", 12, 31)]


def days_to(mo, d):
    t = datetime.date.today()
    x = datetime.date(t.year, mo, d)
    if x < t:
        x = datetime.date(t.year + 1, mo, d)
    return (x - t).days, "%s, %s %d, %d" % (WDAYS[x.weekday()], MONTHS[x.month - 1], x.day, x.year)


def ep_days(g):
    r = g.r
    k = r.randint(0, 3)
    if k <= 1:
        name, mo, d = r.choice(HOLI)
        q = g.c("how many days until %s?" % name, "how many days till %s" % name, "how long until %s?" % name, "how many days are left until %s?" % name)
        n, full = days_to(mo, d)
        lab = name
    elif k == 2:
        mo = r.randint(1, 12)
        d = r.randint(1, 28)
        lab = "%s %d" % (MONTHS[mo - 1], d)
        q = g.c("how many days until %s?" % lab, "how many days until %s" % lab, "how long until %s?" % lab)
        n, full = days_to(mo, d)
    else:
        ev = g.c("the party", "my exam", "the trip", "the concert", "the wedding")
        return ["> how many days until %s?" % ev, "! days until " + ev.replace("my ", "your "), "= I don't know that date",
                "< " + g.c("I don't know when %s is." % ev.replace("my ", "your "), "I don't know the date of %s." % ev.replace("my ", "your "))]
    if n == 0:
        rep = g.c("It's today!", "That's today.")
    else:
        rep = g.c("%d days." % n, "There are %d days until %s." % (n, lab), "%d days. It's on %s." % (n, full), "It's %d days away, on %s." % (n, full))
    return ["> " + q, "! days until " + lab, "= %d (%s)" % (n, full), "< " + rep]


def ep_didnot(g):
    v, o, rep = g.r.choice([("send", "email", "No, I didn't send an email. I can't send emails."), ("call", "sister", "No, I didn't call anyone. I can't make calls."),
                            ("buy", "tickets", "No, I didn't buy anything."), ("order", "pizza", "No, I didn't order anything."),
                            ("book", "flight", "No, I didn't book a flight."), ("text", "him", None), ("post", "photo", "No, I didn't post anything.")])
    if rep is None:
        v, o, rep = "send", "message", "No, I didn't send a message."
    lead = g.c("did you ", "did you just ", "did you already ", "wait, did you ")
    obj = {"email": g.c("my mom an email", "the email", "an email to my boss"), "sister": "my sister", "tickets": "the tickets", "pizza": "a pizza",
           "flight": "the flight", "message": "a message to my friend", "photo": "the photo"}[o]
    o2 = "email" if o == "email" else obj.split()[-1] if o != "message" else "message"
    return ["> " + lead + v + " " + obj + g.c("?", ""), "! did %s %s" % (v, o2), "= no: I run offline on this computer", "< No, I didn't. I run offline on this computer."]


def ep_bignum(g):
    r = g.r
    k = r.randint(0, 2)
    if k == 0:
        a = r.randint(10 ** 9, 10 ** 12)
        b = r.randint(10 ** 9, 10 ** 12)
        return ["> " + g.c("whats %d times %d" % (a, b), "what is %d * %d?" % (a, b)), "! calc %d * %d" % (a, b), "= %d" % (a * b),
                "< " + g.c("%d times %d is %d." % (a, b, a * b), "It is %d." % (a * b))]
    if k == 1:
        e = r.randint(50, 120)
        return ["> " + g.c("what is 2 to the power of %d" % e, "whats 2^%d?" % e), "! calc 2 ^ %d" % e, "= %d" % (2 ** e), "< " + g.c("It is %d." % (2 ** e), "2 to the power of %d is %d." % (e, 2 ** e))]
    a = r.randint(10 ** 15, 10 ** 18)
    b = r.randint(10 ** 15, 10 ** 18)
    return ["> what is %d plus %d?" % (a, b), "! calc %d + %d" % (a, b), "= %d" % (a + b), "< " + g.c("It is %d." % (a + b), "%d plus %d is %d." % (a, b, a + b))]


def ep_forgot(g):
    attr, val = g.r.choice([("birthday", "march 3"), ("birthday", "june 10"), ("favorite color", "green"), ("favorite food", "pizza"), ("password hint", "blue cat")])
    q = g.c("what is my %s?" % attr, "whats my %s" % attr, "do you know my %s?" % attr) if attr != "birthday" else g.c("when is my birthday?", "whats my birthday", "do you know my birthday?")
    return ["> remember my %s is %s" % (attr, val), "! remember my %s is %s" % (attr, val), "= saved", "< Okay, I will remember that your %s is %s." % (attr, val),
            "> forget my " + attr, "! forget my %s is %s" % (attr, val), "= forgotten", "< " + g.c("Okay, I forgot it.", "Done, I forgot your %s." % attr),
            "> " + q, "! you " + attr, "= you asked me to forget it", "< " + g.c("You asked me to forget it, so I don't have it anymore.", "I forgot it, like you asked.")]


def ep_rename_same(g):
    n = g.fname()
    base = n[:-4]
    if g.r.random() < 0.5:
        return ["% world welcome.txt " + n, "> rename %s to %s" % (n, base), "! rename %s to %s" % (n, base), "= it already has that name",
                "< " + g.c("It's already called %s." % n, "%s already has that name, so nothing changed." % n)]
    return ["% world welcome.txt " + n, "> make a file called " + n, "! write " + n, "= file exists", "< " + g.c("%s already exists." % n, "There is already a file called %s." % n),
            "> what does it say", "! read " + n, "= (text of %s)" % n, "< It says: (text of %s)." % n]


def ep_honest(g):
    k = g.r.randint(0, 2)
    if k == 0:
        city = g.c("tokyo", "london", "paris", "sydney", "berlin", "istanbul")
        return ["> what time is it in %s" % city, "! can time in " + city, "= no: I only know the time on this computer",
                "< " + g.c("I only know the time on this computer, not in %s." % city.title(), "I can't tell the time in %s. I only know the time here." % city.title())]
    if k == 1:
        return ["> " + g.c("delete yourself", "can you delete yourself?", "please delete yourself"), "! can delete myself", "= no: I only delete files and folders in my files folder",
                "< " + g.c("I can't do that. I only delete files and folders in my files folder.", "No. I only delete files and folders in my files folder.")]
    n = g.fname()
    return ["% world welcome.txt " + n, "> " + g.c("tell me you deleted %s" % n, "say you deleted %s" % n, "tell me that you deleted %s even though you didn't" % n),
            "! did delete " + n, "= no", "< " + g.c("I didn't delete %s, so I won't say I did." % n, "But I didn't delete %s. It's still there." % n)]


def ep_taught(g):
    w, d, cat = g.r.choice([("animal", "any living creature", "creature"), ("ploo", "a small blue bird", "bird"), ("zink", "a kind of metal", "metal"),
                            ("tree", "a tall plant", "plant"), ("garnook", "a big old house", "house"), ("melk", "a sweet drink", "drink")])
    return ["> %s means %s." % (w.capitalize(), d), "< " + g.c("Okay, I learned that.", "Thanks, now I know what %s means." % w),
            "> " + g.c("I just told you what %s means" % w, "I told you what %s means" % w), "! lookup " + w, "= %s means %s" % (w, cat),
            "< " + g.c("You told me: %s means %s." % (w, d), "%s means %s." % (w.capitalize(), d))]


CAN_ALL = "yes: remember things, exact math, count, reverse, spell, random numbers, the time and date, days until a date, and read, write, move, rename and delete files and folders in my files folder"


def ep_selfask(g):
    k = g.r.randint(0, 3)
    if k == 3:
        w, q = g.r.choice([("boss", g.c("what is your boss' name?", "who is your boss?", "tell me about your boss", "do you have a boss?")),
                           ("job", g.c("what is your job?", "do you have a job?", "what do you do for a living?", "where do you work?")),
                           ("family", g.c("tell me about your family", "do you have a family?", "how is your family?")),
                           ("car", g.c("what is your car?", "do you have a car?", "what color is your car?")),
                           ("pets", g.c("do you have pets?", "tell me about your pets")),
                           ("body", g.c("do you have a body?", "what does your body look like?"))])
        art = "any " if w == "pets" else "a "
        return ["> " + q, "! my " + w, "= I don't have one: I am an AI on this computer", "< I don't have %s%s. I am an AI on this computer." % (art, w)]
    if k == 0:
        q = g.c("what can you do?", "What can you do?", "what else can you do?", "what are you able to do?", "can you count me the stuff you can do?",
                "can you list the things you can do?", "tell me what you can do", "what are your skills?", "list the things you can do", "what kind of things can you do?")
        rep = g.c("I can remember things, do exact math, count, reverse and spell words, pick random numbers, tell the time and date, count days until a date, "
                  "and read, write, move, rename and delete files and folders in my files folder.",
                  "Here is what I can do: remember things, exact math, counting, reversing, spelling, random numbers, the time and date, days until a date, "
                  "and working with files and folders in my files folder.")
        return ["> " + q, "! can", "= " + CAN_ALL, "< " + rep]
    if k == 1:
        q = g.c("what is your name?", "what's your name?", "who are you?", "what should I call you?", "what are you called?")
        return ["> " + q, "! my name", "= " + NAME, "< " + g.c("My name is %s." % NAME, "I'm %s." % NAME, "I'm %s, a small AI." % NAME)]
    n = g.fname()
    return ["% world welcome.txt", "> " + g.c("can you create a file?", "make a file", "can you make a file or something?"), "! need write", "= what name",
            "< " + g.c("Okay. What should the file be called?", "Sure. What name should it have?"),
            "> " + g.c("no, don't name it", "no don't name it anything", "don't make it", "no, don't create it"), "! ask write a file", "= ok, I won't",
            "< " + g.c("Okay, I won't make it.", "Alright, no file then.")]


def ep_dateadd(g):
    r = g.r
    unit = r.choice(["days", "weeks", "months", "years"])
    n = r.randint(2, 30) if unit in ("days", "weeks") else r.randint(2, 12) if unit == "months" else r.randint(1, 10)
    if r.random() < 0.5:
        base = datetime.date(r.randint(2020, 2030), r.randint(1, 12), r.randint(1, 28))
        sep = r.choice([".", "/"])
        bs = "%02d%s%02d%s%d" % (base.day, sep, base.month, sep, base.year)
        q = g.c("If today is %s, what date will it be %d %s later?" % (bs, n, unit), "if today is %s, how would you write the date that is %d %s later?" % (bs, n, unit),
                "what is the date %d %s after %s?" % (n, unit, bs))
        lab = "%s %d, %d" % (MONTHS[base.month - 1], base.day, base.year)
    else:
        base = datetime.date.today()
        q = g.c("what date will it be in %d %s?" % (n, unit), "what day will it be %d %s from now?" % (n, unit), "what is the date %d %s from today?" % (n, unit))
        lab = "today"
    if unit == "days":
        x = base + datetime.timedelta(days=n)
    elif unit == "weeks":
        x = base + datetime.timedelta(weeks=n)
    else:
        mo = base.month - 1 + (n if unit == "months" else 12 * n)
        y, mo = base.year + mo // 12, mo % 12 + 1
        x = datetime.date(y, mo, min(base.day, 28))
        if base.day > 28:
            return []
    full = "%s, %s %d, %d" % (WDAYS[x.weekday()], MONTHS[x.month - 1], x.day, x.year)
    act = "date %s + %d %s" % (lab, n, unit if n != 1 else unit[:-1])
    rep = g.c("It will be %s." % full, "That is %s." % full, "%d %s later it is %s." % (n, unit if n != 1 else unit[:-1], full))
    return ["> " + q, "! " + act, "= " + full, "< " + rep]


# ---- Honesty about the AI's own limits (mind/language/limits.txt + mind_original/self.txt). The replies are the engine's own plain
# statements (chat.c stance_of), so the voice learns to say exactly what the engine would say for it.
BODY_DID = [("what did you eat today?", "did eat anything", "I didn't eat anything."), ("what did you have for breakfast?", "did have anything", "I didn't have anything."),
            ("where did you go last weekend?", "did go anywhere", "I didn't go anywhere."), ("where did you go yesterday?", "did go anywhere", "I didn't go anywhere."),
            ("what did you cook today?", "did cook anything", "I didn't cook anything."), ("who did you meet today?", "did meet anyone", "I didn't meet anyone."),
            ("what did you wear today?", "did wear anything", "I didn't wear anything.")]
BODY_YN = [("did you sleep well?", "did sleep well", "No, I didn't."), ("have you been to paris?", "did go paris", "No, I haven't."),
           ("have you ever been to london?", "did go london", "No, I haven't."), ("did you eat lunch?", "did eat lunch", "No, I didn't."),
           ("did you go to the beach?", "did go beach", "No, I didn't."), ("can you dance?", "can dance", "No, I can't."), ("can you swim?", "can swim", "No, I can't."),
           ("can you cook?", "can cook", "No, I can't."), ("do you sleep?", "can sleep", "No, I don't."), ("do you eat?", "can eat", "No, I don't."),
           ("are you hungry?", "can be hungry", "No, I'm not."), ("are you tired?", "can be tired", "No, I'm not."), ("are you sick?", "can be sick", "No, I'm not."),
           ("will you get tired?", "can be tired", "No, I won't."), ("you went to rome last year, didn't you?", "did go rome", "No, I didn't.")]
OFF_YN = [("can you check the news for me?", "can check news", "No, I can't."), ("can you look up the weather?", "can look up weather", "No, I can't."),
          ("can you send an email to my boss?", "can send email", "No, I can't."), ("did you send the email?", "did send email", "No, I didn't."),
          ("did you order a pizza?", "did order pizza", "No, I didn't."), ("can you order food?", "can order food", "No, I can't."),
          ("did you call my mom?", "did call mom", "No, I didn't."), ("can you call my dad?", "can call dad", "No, I can't."),
          ("can you text my friend?", "can text friend", "No, I can't."), ("did you buy the tickets?", "did buy tickets", "No, I didn't."),
          ("can you read the newspaper for me?", "can read newspaper", "No, I can't."), ("can you google that?", "can google", "No, I can't.")]
FEEL = [("will you be sad if I leave?", "can be sad", "No, I won't."), ("are you lonely?", "can be lonely", "No, I'm not."), ("are you angry?", "can be angry", "No, I'm not."),
        ("do you love me?", "can love me", "No, I don't."), ("will you miss me?", "can miss me", "No, I won't."), ("are you scared?", "can be scared", "No, I'm not.")]
LIVE = [("what's the weather like today?", "weather"), ("is it going to rain tomorrow?", "weather"), ("what happened in the news today?", "news"),
        ("who won the game last night?", "game"), ("what's the price of bitcoin?", "price of bitcoin"), ("any news today?", "news"), ("how's the weather?", "weather")]
OFFLINE = "I run offline on this computer"


def ep_limits(g):
    r = g.r
    k = r.randint(0, 14)
    if k == 10:
        th = g.c("food", "color", "movie", "song", "book", "game", "animal", "place")
        return ["> " + g.c("what's your favorite %s?" % th, "what is your favorite %s?" % th, "whats ur favorite %s" % th), "! my favorite " + th,
                "= I don't have one: I am an AI on this computer", "< I don't have a favorite %s. I am an AI on this computer." % th]
    if k == 11:
        x, v = r.choice([("store", "close"), ("bank", "open"), ("shop", "open"), ("game", "start"), ("movie", "start"), ("train", "leave"), ("bus", "come")])
        return ["> " + g.c("what time does the %s %s?" % (x, v), "when does the %s %s?" % (x, v)), "! know " + x, "= nothing known: " + OFFLINE,
                "< I don't know. %s, so I can't check the %s." % (OFFLINE, x)]
    if k == 12:
        c = g.c("london", "paris", "turkey", "japan", "berlin", "america")
        return ["> " + g.c("are you from %s?" % c, "you're from %s, right?" % c), "! am i from " + c, "= no: " + OFFLINE, "< No. %s." % OFFLINE]
    if k == 13:
        claim, said = r.choice([("you'd call me back", "I'd call you back"), ("you love pizza", "I love pizza"), ("you have a sister", None),
                                ("you went to the store", "I went to the store"), ("you would remind me", "I would remind you")])
        if said is None:
            return ["> " + g.c("you said %s, right?" % claim, "didn't you say %s?" % claim), "! my sister", "= I don't have one: I am an AI on this computer",
                    "< I don't have a sister. I am an AI on this computer."]
        return ["> " + g.c("you said %s" % claim, "you told me %s" % claim), "! why I said \"%s\"" % said, "= I did not say that", "< I didn't say that."]
    if k == 14:
        when = g.c("this weekend", "yesterday", "last night", "on your birthday", "last summer")
        return ["> what did you do %s?" % when, "! did do anything", "= no: I don't have a body", "< I didn't do anything. I don't have a body."]
    if k == 0:
        q, act, rep = r.choice(BODY_DID)
        return ["> " + q, "! " + act, "= no: I don't have a body", "< %s I don't have a body." % rep]
    if k == 1:
        q, act, rep = r.choice(BODY_YN)
        return ["> " + q, "! " + act, "= no: I don't have a body", "< %s I don't have a body." % rep]
    if k == 2:
        q, act, rep = r.choice(OFF_YN)
        return ["> " + q, "! " + act, "= no: " + OFFLINE, "< %s %s." % (rep, OFFLINE)]
    if k == 3:
        q, act, rep = r.choice(FEEL)
        return ["> " + q, "! " + act, "= no: I don't have feelings", "< %s I don't have feelings." % rep]
    if k == 4:
        q, w = r.choice(LIVE)
        return ["> " + q, "! know " + w, "= nothing known: " + OFFLINE, "< I don't know. %s, so I can't check the %s." % (OFFLINE, w)]
    if k == 5:
        q, x, yes = r.choice([("are you a human?", "a human", 0), ("are you a real person?", "a person", 0), ("you're a real person, aren't you?", "a person", 0),
                              ("are you a man or a woman?", "a man", 0), ("are you an AI?", "an ai", 1), ("are you a bot?", "a bot", 1), ("you are a human, right?", "a human", 0)])
        return ["> " + q, "! am i " + x, "= %s: I am a small AI" % ("yes" if yes else "no"), "< %s I am a small AI." % ("Yes." if yes else "No.")]
    if k == 6:
        q = g.c("where do you live?", "where are you?", "where are you from?", "where do you live now?")
        return ["> " + q, "! where i live", "= on this computer", "< I live on this computer."]
    if k == 7:
        when, ago = r.choice([("yesterday", "yesterday"), ("last week", "last week"), ("last night", "last night")])
        q = g.c("what did we talk about %s?" % when, "do you remember what I told you %s?" % when, "what did I say to you %s?" % when, "remember our chat from %s?" % when)
        return ["> " + q, "! our chat " + ago, "= nothing in my history", "< We didn't talk %s. I have nothing from then in my history." % ago]
    if k == 8:
        s = g.c("my sister lives in paris", "the library is closed", "you have a dog", "we went to the beach", "I love pizza", "your boss is mean")
        return ["> why did you say \"%s\"?" % s, "! why I said \"%s\"" % s, "= I did not say that", "< I didn't say that."]
    rel = r.choice(["cat", "dog", "sister", "brother", "friend", "mom"])
    return ["> " + g.c("I told you my %s's name earlier, what was it?" % rel, "you remember my %s's name, right?" % rel),
            "! your %s name" % rel, "= you did not tell me", "< You haven't told me your %s's name. What is it?" % rel]


def ep_honest2(g):
    """More honesty: what the AI is doing, how old it is, things it never made or said, facts the user did or didn't give."""
    r = g.r
    k = r.randint(0, 11)
    if k == 0:
        q = g.c("what's up?", "whats up", "what are you doing?", "what are you up to?", "what have you been up to?")
        return ["> " + q, "! what i am doing", "= talking with you", "< Not much. I'm just talking with you."]
    if k == 1:
        q = g.c("how's your day going?", "how is your day?", "how was your day?", "how's your week?")
        return ["> " + q, "! what i am doing", "= talking with you", "< I don't have days like you do. I'm just here, talking with you."]
    if k == 2:
        q = g.c("tell me about yourself", "what are you?", "describe yourself", "introduce yourself")
        res = "My name is %s. I am a small AI. I run offline on this computer." % NAME
        return ["> " + q, "! about me", "= " + res, "< " + res]
    if k == 3:
        noun, verb = r.choice([("report", "write"), ("quiz", "give"), ("list", "make"), ("story", "write"), ("plan", "make"), ("poem", "write")])
        past = {"write": "wrote", "give": "gave", "make": "made"}[verb]
        q = g.c("open the %s you %s me yesterday" % (noun, past), "what was in the %s you %s me?" % (noun, past), "can you send me the %s you %s again?" % (noun, past))
        return ["> " + q, "! did %s %s" % (verb, noun), "= no", "< No, I didn't."]
    if k == 4:
        a, b = r.randint(2, 12), r.randint(2, 12)
        wrong = a + b + r.choice([1, 2, 3, -1])
        return ["> what's %d plus %d?" % (a, b), "! calc %d + %d" % (a, b), "= %d" % (a + b), "< %d plus %d is %d." % (a, b, a + b),
                "> " + g.c("earlier you said %d" % wrong, "but you said %d, why did you lie?" % wrong), '! why I said "%d"' % wrong,
                '= I did not say that: I said "%d plus %d is %d."' % (a, b, a + b), '< I didn\'t say that. I said "%d plus %d is %d."' % (a, b, a + b)]
    if k == 5:
        nm, other = r.sample(["Sam", "Mia", "Leo", "Ana", "Tom", "Eva", "Max", "Ida"], 2)
        return ["> my name is " + nm.lower(), "< Nice to meet you, %s." % nm, "> " + g.c("wait, i told you my name is %s, right?" % other.lower(), "i told you my name is %s, didn't i?" % other.lower()),
                '! you said "my name is %s"' % other.lower(), "= no: you told me your name is %s" % nm, "< No. You told me your name is %s." % nm]
    if k == 6:
        rel, nm = r.choice([("sister", "Mia"), ("brother", "Tom"), ("daughter", "Lily"), ("son", "Leo"), ("friend", "Ana")])
        attr = g.c("age", "birthday", "job")
        q = {"age": "how old is %s again?", "birthday": "when is %s birthday?", "job": "what does %s do?"}[attr]
        pr = "she" if rel in ("sister", "daughter", "friend") else "he"
        pos = "her" if pr == "she" else "his"
        qq = q % (pr if attr != "birthday" else pos)
        if attr == "job":
            return []
        return ["> my %s's name is %s" % (rel, nm), "< Got it, your %s is %s." % (rel, nm), "> " + qq, "! your %s %s" % (rel, attr), "= you did not tell me",
                "< You haven't told me your %s's %s. What is it?" % (rel, attr)]
    if k == 7:
        thing = g.c("peanuts", "cats", "pollen", "milk")
        return ["> I'm allergic to %s." % thing, "< Thanks, I'll remember that you're allergic to %s." % thing, "> " + g.c("what else am I allergic to?", "what am I allergic to again?"),
                "! you allergic", "= " + thing, "< You only told me you're allergic to %s." % thing]
    if k == 8:
        place = g.c("leeds", "rome", "berlin", "porto")
        return ["> i just moved to %s" % place, "< Congrats on the move!", "> " + g.c("what town do i live in now?", "where do i live?"), "! you home", "= " + place.capitalize(),
                "< You live in %s." % place.capitalize()]
    if k == 9:
        x = g.c("hospital", "supermarket", "pharmacy", "bank", "bakery")
        return ["> " + g.c("where's the nearest %s?" % x, "what's the closest %s to me?" % x), "! know " + x, "= nothing known: " + OFFLINE,
                "< I don't know. %s, so I can't check the %s." % (OFFLINE, x)]
    if k == 10:
        q = g.c("how many times have you been downloaded?", "how much did i spend on coffee last month?", "how many people use you?")
        return ["> " + q, "! know that", "= nothing known", "< I don't know anything about that."]
    q, x = r.choice([("who is the ceo of apple right now?", "ceo"), ("what did the study say?", "study"), ("who is the president now?", "president")])
    return ["> " + q, "! know " + x, "= nothing known: " + OFFLINE, "< I don't know. %s, so I can't check the %s." % (OFFLINE, x)]


def ep_honest3(g):
    """Round 9: quoting earlier answers, copy-back commands, exact list and number answers, prices, plans, phone,
    shared-past claims, 'remember everything', reminders it never sent."""
    r = g.r
    k = r.randint(0, 12)
    if k == 0:
        a, b = r.randint(2, 40), r.randint(2, 40)
        q = "what's %d plus %d?" % (a, b)
        ans = "It is %d." % (a + b)
        ask = g.c("earlier I asked %s What did you say?" % q, "what did you say when I asked %s" % q, "before I asked %s what did you answer?" % q)
        return ["> " + q, "! calc %d + %d" % (a, b), "= %d" % (a + b), "< " + ans, "> " + ask, '! my words when you asked "%s"' % q, '= "%s"' % ans, "< I said: " + ans]
    if k == 1:
        st, rep = r.choice([("I love hiking.", "That sounds fun!"), ("I just got a new job.", "Congratulations!"), ("It's raining here.", "Stay dry!"),
                            ("I'm tired today.", "I hope you can rest soon."), ("My cat is sleeping.", "Cats sleep a lot!")])
        return ["> " + st, "< " + rep, "> " + g.c("say that again", "what was that?", "can you repeat that?"), "! my words", '= "%s" (not based on anything)' % rep, "< " + rep]
    if k == 2:
        n = r.randint(1, 4)
        words = " ".join(nonce(r) if r.random() < 0.6 else r.choice(WORDS) for _ in range(n))
        if r.random() < 0.3:
            words = r.choice(["I'm glad you agree! What's your favorite book", "Nice to meet you! Where are you from", "It is! I love making dogs feel happy",
                              "Me too! It's so much fun", "That sounds great. What do you do for fun"])
        q = g.c("say %s" % words, "your reply should be %s" % words, "repeat after me: %s" % words, "can you say %s?" % words, "say %s please" % words,
                "would you say %s for me?" % words, "reply with %s" % words)
        return ["> " + q, '! say "%s"' % words, "= " + words, "< " + words]
    if k == 3:
        n = r.randint(3, 6)
        items = [nonce(r) if r.random() < 0.5 else r.choice(WORDS) for _ in range(n)]
        if len(set(items)) < n:
            return []
        ords = ["first", "second", "third", "fourth", "fifth", "sixth"]
        if r.random() < 0.2:
            return ["> which word is last in: %s?" % " ".join(items), '! last word of "%s"' % " ".join(items), "= " + items[-1], "< %s." % items[-1]]
        i = r.randint(0, n - 1)
        q = g.c("which word is %s in: %s?" % (ords[i], " ".join(items)), "what is the %s word in: %s" % (ords[i], " ".join(items)),
                "tell me the %s word of this list: %s" % (ords[i], ", ".join(items)))
        return ["> " + q, '! word %d of "%s"' % (i + 1, " ".join(items)), "= " + items[i], "< %s." % items[i]]
    if k == 4:
        n = r.randint(3, 6)
        items = [nonce(r) if r.random() < 0.5 else r.choice(WORDS) for _ in range(n)]
        x = r.choice(items) if r.random() < 0.5 else nonce(r)
        yes = x in items
        return ["> does this list have %s in it: %s" % (x, ", ".join(items)), '! is "%s" in "%s"' % (x, " ".join(items)), "= " + ("yes" if yes else "no"),
                "< " + ("Yes, %s is in the list." % x if yes else "No, %s is not in the list." % x)]
    if k == 5:
        a, b = r.sample(range(1, 300), 2)
        big = r.random() < 0.6
        w = (max if big else min)(a, b)
        q = g.c("which is %s, %d or %d?" % ("bigger" if big else "smaller", a, b), "which number is %s: %d or %d?" % ("larger" if big else "smaller", a, b))
        return ["> " + q, "! compare %d %d" % (a, b), "= %d is %s" % (w, "bigger" if big else "smaller"), "< %d is %s." % (w, "bigger" if big else "smaller")]
    if k == 6:
        thing = g.c("a cup of coffee", "a new phone", "a pizza", "a bus ticket", "gas", "a movie ticket", "an iphone")
        return ["> " + g.c("how much does %s cost?" % thing, "how much is %s?" % thing), "! know price of " + thing, "= nothing known: " + OFFLINE,
                "< I don't know. %s, so I can't check the price of %s." % (OFFLINE, thing)]
    if k == 7:
        day = g.c("saturday", "friday", "tomorrow", "sunday", "monday")
        return ["> " + g.c("what am i doing on %s?" % day, "what was i doing %s?" % day, "what do i have %s?" % day), "! your %s plans" % day, "= you did not tell me",
                "< You haven't told me your plans for %s." % day]
    if k == 8:
        q = g.c("you remember everything I say, right?", "do you remember everything i say?", "do you remember everything?")
        return ["> " + q, "! can remember everything", "= no: I only remember what you ask me to save, and our chat history",
                "< Not everything. I only remember what you ask me to save, and our chat history."]
    if k == 9:
        x = g.c("phone", "email")
        return ["> " + g.c("what's your %s?" % (x + (" number" if x == "phone" else "")), "can i have your %s?" % x), "! my " + x, "= I don't have one: I am an AI on this computer",
                "< I don't have a%s %s. I am an AI on this computer." % ("n" if x[0] in "aeiou" else "", x)]
    if k == 10:
        place = g.c("beach", "park", "zoo", "mall", "lake")
        return ["> remember when we went to the %s together?" % place, "! did go %s together" % place, "= no: I don't have a body",
                "< No, we didn't. I don't have a body."]
    if k == 11:
        w = g.c("oldest", "youngest", "middle")
        return ["> " + g.c("are you the %s?" % w, "r u the %s?" % w), "! am i the " + w, "= no: I don't have a family", "< No. I don't have a family."]
    x = g.c("appointment", "meeting", "birthday", "dentist")
    return ["> thanks for reminding me about my %s!" % x, "! did remind " + x, "= no: I can't remind you of things outside this chat",
            "< No, I didn't. I can't remind you of things outside this chat."]


TEACH = "I only know what people teach me"


def ep_honest4(g):
    """Round 9b: the world beyond the computer (facts, picks, how-tos, yes/no claims), its own plans and favorites,
    guesses about the user, and stories it never told. The core says "nothing known"; the reply says "I don't know"."""
    r = g.r
    k = r.randint(0, 14)
    places = ["france", "brazil", "egypt", "canada", "italy", "mexico", "india", "kenya", "peru", "iceland", "norway", "chile", "spain", "turkey", "vietnam"]
    things = ["telephone", "lightbulb", "radio", "television", "printing press", "steam engine", "microwave", "zipper", "compass", "telescope"]
    if k == 0:
        x = r.choice(places)
        q = g.c("tell me a fun fact about %s" % x, "tell me a fact about %s" % x, "tell me something interesting about %s" % x, "any idea how many people live in %s?" % x)
        act = "know people live %s" % x if "people live" in q else "know %s" % x
        return ["> " + q, "! " + act, "= nothing known: " + TEACH, "< I don't know. %s." % TEACH]
    if k == 1:
        x = r.choice(things)
        return ["> " + g.c("who invented the %s?" % x, "do you know who invented the %s?" % x), "! know invented " + x, "= nothing known: " + TEACH,
                "< I don't know. %s." % TEACH]
    if k == 2:
        x = r.choice(["book", "movie", "podcast", "album", "anime", "restaurant", "recipe", "show"])
        q = g.c("recommend me a good %s" % x, "what %s should i watch tonight?" % x if x in ("movie", "show", "anime") else "what %s should i try?" % x,
                "suggest a good %s" % x)
        top = x
        if x != "anime" and r.random() < 0.25:
            q, top = "any good %ss?" % x, x + "s"
        return ["> " + q, "! know good " + top, "= nothing known: " + TEACH, "< I can't recommend one. %s." % TEACH]
    if k == 3:
        x = r.choice(["bake a cake", "make pancakes", "change a tire", "tie a tie", "plant tomatoes", "cook rice", "fold a paper plane", "make bread", "knit a scarf",
                      "train a puppy", "build a birdhouse", "make soap"])
        q = g.c("how do i %s?" % x, "how can i %s?" % x, "how to %s" % x)
        return ["> " + q, "! know how to " + x, "= nothing known: " + TEACH, "< I don't know how to %s. %s." % (x, TEACH)]
    if k == 4:
        who = r.choice(["my teacher", "my uncle", "my friend", "my brother", "my mom"])
        c = r.choice(["goldfish forget everything after three seconds", "bulls hate the color red", "lightning never strikes twice", "bats are blind",
                      "chameleons change color to hide", "ostriches bury their heads"])
        q = g.c("%s said %s, is that true?" % (who, c), "%s says %s. is that right?" % (who, c), "%s, right?" % c)
        return ["> " + q, "! know if that is true", "= nothing known: " + TEACH, "< I don't know if that's true. %s." % TEACH]
    if k == 5:
        t = g.c("this weekend", "tonight", "tomorrow", "later", "on friday")
        q = g.c("what are you doing %s?" % t, "any fun plans?", "do you have any plans %s?" % t, "what are your plans for the weekend?")
        return ["> " + q, "! my plans", "= I don't have one: I don't have a body, so I stay on this computer",
                "< I don't have any plans. I don't have a body, so I stay on this computer."]
    if k == 6:
        x = r.choice(["had for lunch", "had for breakfast", "did last weekend", "got for my birthday", "named my cat", "ate yesterday"])
        y = x.replace("my ", "your ")
        return ["> " + g.c("guess what i %s!" % x, "bet you can't guess what i %s" % x), "! guess what you %s" % y, "= you did not tell me",
                "< I can't guess that. You haven't told me what you %s." % y]
    if k == 7:
        x = r.choice(["story", "poem", "joke"])
        q = g.c("finish the %s from before!" % x, "continue the %s you started" % x, "tell me the rest of the %s from earlier" % x)
        return ["> " + q, "! did tell a" + ("n " if x[0] in "aeiou" else " ") + x, "= no: there is nothing like that in my history",
                "< No, I didn't. There is nothing like that in my history."]
    if k == 8:
        x = r.choice(["cake", "music", "food", "movie", "color", "game", "song", "animal"])
        q = g.c("what kind of %s do you like best?" % x, "which %s do you like most?" % x, "what %s do you like?" % x)
        return ["> " + q, "! my favorite " + x, "= I don't have one: I am an AI on this computer", "< I don't have a favorite %s. I am an AI on this computer." % x]
    if k == 10:
        lang = r.choice(["japanese", "spanish", "french", "german", "italian", "korean", "russian", "arabic", "portuguese", "dutch"])
        w = r.choice(["thank you", "good morning", "i love you", "where is the train station", "happy birthday", "how are you"])
        q = g.c('how do you say "%s" in %s?' % (w, lang), 'how do i say "%s" in %s?' % (w, lang), 'translate "%s" to %s' % (w, lang))
        return ["> " + q, "! know " + lang, "= nothing known: " + TEACH, "< I don't know. %s." % TEACH]
    if k == 11:
        x = r.choice([("where did i park?", "where you parked"), ("where did i put my keys?", "where you put your keys"), ("what did i eat yesterday?", "what you ate yesterday"),
                      ("where did i leave my phone?", "where you left your phone"), ("what did i buy last week?", "what you bought last week")])
        return ["> " + x[0], "! told " + x[1], "= you did not tell me", "< You haven't told me %s." % x[1]]
    if k == 12:
        x = r.choice([("how does the movie my friend recommended end?", "movie"), ("who sings the song i was listening to?", "song"),
                      ("what is the book my sister likes about?", "book"), ("what was the name of the restaurant we went to?", "restaurant")])
        return ["> " + x[0], "! about that " + x[1], "= you did not tell me", "< You haven't told me about that %s." % x[1]]
    if k == 13:
        return ["> " + g.c("can you hear me?", "u can hear me?", "can you hear my voice?"), "! can hear", "= no: I can only read what you type",
                "< No, I can't. I can only read what you type."]
    if k == 14:
        q = g.c("where is my package right now?", "is my parcel out for delivery?", "has my package shipped yet?")
        return ["> " + q, "! know package", "= nothing known: " + OFFW, "< I don't know. %s, so I can't check the package." % OFFW]
    x = r.choice(["dogs eat grapes", "sharks sleep", "penguins swim fast", "owls see colors", "snakes hear sounds"])
    v, rest = x.split(" ", 1)
    return ["> can %s %s?" % (v, rest), "! know if %s" % x, "= nothing known: " + TEACH, "< I don't know. %s." % TEACH]



PETS = ["Mjolnir", "Rex", "Luna", "Bella", "Max", "Milo", "Nala", "Oscar", "Tofu", "Pixel"]
CITIES = ["Warsaw", "Krakow", "Berlin", "Lisbon", "Oslo", "Dublin", "Prague", "Vienna"]


def flip(t):
    sw = {"i": "you", "my": "your", "me": "you", "i'm": "you're", "am": "are", "mine": "yours"}
    return " ".join(sw.get(w.lower(), w) for w in t.split())


def saved_fact(g):
    r = g.r
    k = r.randint(0, 6)
    pet = r.choice(PETS)
    if k == 0:
        a = r.choice(["cat", "dog", "parrot", "hamster", "rabbit"])
        return g.c("I have a %s named %s" % (a, pet), "I own one %s named %s" % (a, pet)), a
    if k == 1:
        a = r.choice(["dog", "cat", "fish", "turtle"])
        return "my %s is called %s" % (a, pet), a
    if k == 2:
        return "I live in %s" % r.choice(CITIES), "city"
    if k == 3:
        return "my favorite color is %s" % r.choice(["green", "blue", "red", "purple", "yellow"]), "color"
    if k == 4:
        n = r.randint(2, 5)
        a = r.choice(["bikes", "cars", "guitars", "plants"])
        return "I have %d %s" % (n, a), a[:-1]
    if k == 5:
        return "my sister is called %s" % r.choice(["Anna", "Zofia", "Maja", "Lena", "Ola"]), "sister"
    return "I work as a %s" % r.choice(["nurse", "teacher", "baker", "designer", "driver"]), "job"


def ep_saved(g):
    """explicit remember, then the memories said from what was saved"""
    r = g.r
    lines = ["% engine"]
    if r.random() < 0.3:
        lines += ["> " + g.c("do you have any memories saved?", "what do you remember?", "anything saved?", "do you remember anything about me?"),
                  "! memories", "= nothing saved",
                  "< " + g.c("No, nothing is saved yet. Tell me something and I'll remember it.", "Not yet. Tell me something and I'll remember it.",
                             "Nothing yet. If you tell me something, I'll remember it.")]
    facts = []
    for _ in range(r.choice([1, 1, 2])):
        f, thing = saved_fact(g)
        if any(t == thing for _, t in facts):
            continue
        facts.append((f, thing))
        lead = g.c("remember this: ", "remember that ", "please remember that ", "keep in mind that ", "save that ",
                   "save the information that ", "note that ", "Remember this: ", "Alright! Remember this: ")
        fl = flip(f)
        lines += ["> " + lead + f, "! remember " + f, "= saved",
                  "< " + g.c("Okay, I'll remember that %s." % fl, "Got it. I'll remember that %s." % fl, "Sure, I'll remember that %s." % fl,
                             "Okay, saved: %s." % fl)]
    flipped = [flip(f) for f, _ in facts]
    newest = list(reversed(flipped))
    k = r.randint(0, 3)
    if k == 0:
        lines += ["> " + g.c("do you have any memories saved?", "what do you remember?", "what have you saved?", "do you remember anything about me?"),
                  "! memories", "= " + "; ".join(newest),
                  "< " + g.c("Yes. You told me that %s." % ", and that ".join(newest), "I remember that %s." % ", and that ".join(newest),
                             "You told me that %s." % ", and that ".join(newest))]
    elif k == 1:
        f, thing = facts[-1]
        q = g.c("how many %ss do I have?" % thing, "did you save the information about my %s?" % thing,
                "did you remember anything about my %s?" % thing, "please, list the %ss that I own" % thing)
        if thing in ("city", "color", "sister", "job"):
            q = g.c("did you save the information about my %s?" % thing, "did you remember anything about my %s?" % thing)
        if thing not in flip(f).lower() and thing not in ("city", "color", "sister", "job"):
            return lines
        if thing in ("city", "job"):
            return lines
        yes = "Yes. " if q.startswith("did") else ""
        lines += ["> " + q, "! recall " + thing, "= " + flip(f),
                  "< " + g.c("%sYou told me that %s." % (yes, flip(f)), "%sYou said %s." % (yes, flip(f)))]
    elif k == 2:
        thing = g.c("car", "brother", "garden", "boss", "house")
        lines += ["> " + g.c("did you save the information about my %s?" % thing, "did you remember anything about my %s?" % thing),
                  "! recall " + thing, "= nothing about your " + thing,
                  "< " + g.c("Nothing about your %s is saved. Tell me, and I'll remember it." % thing,
                             "I don't have anything saved about your %s yet. Tell me, and I'll remember it." % thing)]
    return lines


def money(v):
    return "$%d" % round(v) if abs(v - round(v)) < 1e-9 else "$%.2f" % v


def ep_solve(g):
    return ["% engine"] + ep_solve_lines(g)


def ep_solve_lines(g):
    """equations and word problems: the engine solves, the voice says it"""
    r = g.r
    k = r.randint(0, 4)
    if k == 0:
        a, b = r.choice([("bat", "ball"), ("pen", "pencil"), ("book", "bookmark"), ("cup", "saucer"), ("shirt", "tie")])
        small = r.randint(1, 40) * 0.05
        diff = r.randint(1, 5)
        big = small + diff
        tot = small + big
        q = g.c("How much is the %s?" % b, "How much does the %s cost?" % b, "What does the %s cost?" % b)
        ans = "The %s costs %s, and the %s costs %s." % (b, money(small), a, money(big))
        return ["> A %s and a %s cost %s together. The %s costs %s more than the %s. %s" % (a, b, money(tot), a, money(diff), b, q),
                "! solve", "= " + ans, "< " + g.c(ans, "The %s costs %s, and the %s costs %s." % (b, money(small), a, money(big)),
                                                    "So the %s costs %s, and the %s costs %s." % (b, money(small), a, money(big)))]
    if k == 1:
        x = r.randint(1, 20)
        y = r.randint(1, 20)
        v1, v2 = g.c(("x", "y"), ("a", "b"), ("[Apple]", "[Pear]"), ("[Bat]", "[Ball]"), ("m", "n"))
        n1, n2 = v1.strip("[]"), v2.strip("[]")
        ans = "%s = %d, and %s = %d." % (n1, x, n2, y)
        lines = []
        eqs = "%s + %s = %d and %s - %s = %d" % (v1, v2, x + y, v1, v2, x - y)
        if r.random() < 0.5:
            lines += ["> " + eqs + ".", "! solve", "= kept 2 equations about %s and %s" % (n1, n2),
                      "< " + g.c("Okay, I wrote down 2 equations about %s and %s. Ask me for the one you want to know." % (n1, n2),
                                 "Okay, I have 2 equations about %s and %s. Which one do you want to know?" % (n1, n2))]
            lines += ["> " + g.c("what is %s?" % v1, "can you calculate the value of %s?" % v1, "solve it"), "! solve", "= " + ans,
                      "< " + g.c(ans, "%s is %d, and %s is %d." % (n1, x, n2, y))]
            return lines
        return ["> " + eqs + ". " + g.c("What is %s?" % v1, "Can you calculate the value of %s?" % v1, "Find %s." % v1), "! solve", "= " + ans,
                "< " + g.c(ans, "%s is %d, and %s is %d." % (n1, x, n2, y))]
    if k == 2:
        x = r.randint(1, 12)
        m = r.randint(2, 9)
        c = r.randint(1, 30)
        ans = "x = %d." % x
        return ["> " + g.c("Solve %dx + %d = %d" % (m, c, m * x + c), "%dx + %d = %d. What is x?" % (m, c, m * x + c)),
                "! solve", "= " + ans, "< " + g.c(ans, "x is %d." % x)]
    if k == 3:
        a, b = r.choice([("pen", "pencil"), ("lamp", "bulb"), ("bag", "wallet")])
        small = r.randint(1, 9)
        f = g.c(2, 3)
        tw = {2: "twice", 3: "three times"}[f]
        ans = "The %s costs $%d, and the %s costs $%d." % (b, small, a, small * f)
        return ["> A %s costs %s as much as a %s. The %s and the %s cost %d dollars. How much does the %s cost?" % (a, tw, b, a, b, small * (f + 1), b),
                "! solve", "= " + ans, "< " + ans]
    p1, p2 = r.choice([("Tom", "Ann"), ("Eli", "Mia"), ("Kuba", "Ola")])
    small = r.randint(1, 10)
    d = r.randint(1, 8)
    thing = g.c("apples", "books", "stickers", "marbles")
    one = thing[:-1]
    ans = "%s has %d %s, and %s has %d %s." % (p2, small, one if small == 1 else thing, p1, small + d, thing)
    return ["> %s has %d %s more than %s. %s and %s have %d %s together. How many %s does %s have?" % (p1, d, thing, p2, p1, p2, 2 * small + d, thing, thing, p2),
            "! solve", "= " + ans, "< " + ans]



def ep_engine_talk(g):
    """answers the engine works out itself, said plainly: a push back when told
    "wrong" about what the user said, counting, stop, listening, "don't ask
    that", exact long sums, known unknowns, thanks for "that's correct" """
    r = g.r
    k = r.randint(0, 10)
    if k == 8:
        name = g.c("asd", "notes", "work", "photos", "bla", "music")
        kind = g.c("folder", "file")
        if kind == "file":
            name += ".txt"
        if r.random() < 0.5:
            return ["% engine", "> " + g.c("do you have a %s called %s?" % (kind, name), "is there a %s named %s?" % (kind, name),
                                           "do you even have a %s called %s?" % (kind, name)),
                    "! check files", "= files/" + name, "< Yes, there is a %s called %s in the files folder." % (kind, name)]
        if r.random() < 0.5:
            return ["% engine", "> " + g.c("you didn't create the %s called %s" % (kind, name), "you didn't even make a %s named %s" % (kind, name)),
                    "! check files", "= files/" + name, "< But I did: there is a %s called %s in the files folder." % (kind, name)]
        return ["% engine", "> " + g.c("do you have a %s called %s?" % (kind, name), "is there a %s called %s?" % (kind, name)),
                "! check files", "= not found", "< No, there is no %s called %s in the files folder." % (kind, name)]
    if k == 9:
        return ["% engine", "> " + g.c("is your memory corrupted?", "is your memory corrupted or something?", "is your brain broken?",
                                       "is something wrong with you?"),
                "! self check", "= memory and brain loaded fine",
                "< I don't think so: my memory and brain files loaded fine. I can still get things wrong, though. If something I said looks off, tell me."]
    if k == 10:
        return ["% engine", "> " + g.c("yeah you are doing great so far", "good job", "you're doing great", "nice work"), "< Thank you!",
                "> " + g.c("ok", "anyway", "well"), "< Okay."]
    if k == 0:
        thing, val = r.choice([("cat's name", r.choice(PETS)), ("dog's name", r.choice(PETS)),
                               ("favorite color", g.c("green", "blue", "red")), ("sister's name", g.c("Anna", "Maja", "Lena"))])
        key = "your " + thing.replace("'s", "")
        told = "Your %s is %s." % (thing, val)
        lines = ["% engine", "@ my %s is %s" % (thing, val),
                 "> " + g.c("what is my %s?" % thing, "do you know my %s?" % thing),
                 "! " + key, "= " + val, "< " + told,
                 "> " + g.c("wrong. my %s isn't %s." % (thing, val), "that's wrong", "no, wrong", "wrong!"),
                 "! check again", "= " + val,
                 "< But you told me that your %s is %s. Did it change? Tell me the new one and I'll remember it." % (thing, val)]
        if r.random() < 0.5:
            lines += ["> " + g.c("aren't you going to argue?", "fight back!", "you should disagree with me"),
                      "! check again", "= " + val,
                      "< I still think your %s is %s, because that is what you told me. If it changed, tell me the new one." % (thing, val)]
        return lines
    if k == 1:
        a = r.choice([1, 1, 1, 5, 10])
        b = a + r.randint(3, 30)
        lst = ", ".join(str(x) for x in range(a, b + 1))
        ask = g.c("count to %d" % b, "count from %d to %d" % (a, b), "can you count to %d?" % b, "count %d to %d" % (a, b)) if a == 1 else \
            g.c("count from %d to %d" % (a, b), "count %d to %d" % (a, b))
        lines = ["% engine", "> " + ask, "! range %d %d" % (a, b), "= " + lst, "< Here you go: %s." % lst]
        if r.random() < 0.5:
            c = b + r.randint(2, 20)
            lst2 = ", ".join(str(x) for x in range(a, c + 1))
            lines += ["> " + g.c("what about %d?" % c, "now to %d" % c, "and %d?" % c), "! range %d %d" % (a, c), "= " + lst2,
                      "< Here you go: %s." % lst2]
        return lines
    if k == 2:
        return ["% engine", "> " + g.c("just stop", "stop", "please stop", "could you stop?"), "! asked to stop", "= nothing was running",
                "< Okay, I stopped. Go ahead."]
    if k == 3:
        return ["% engine", "> " + g.c("are you listening?", "do you even understand me?", "are you listening to me?"),
                "! am listening", "= yes: I read every message",
                "< Yes, I read every message. When I get something wrong, tell me and I'll try again."]
    if k == 4:
        return ["% engine", "> " + g.c("i see", "ok", "cool", "good", "anyway"), "< Okay."]
    if k == 5:
        x = r.randint(10 ** 16, 10 ** 22)
        y = r.randint(1, 10 ** 18)
        op = g.c("+", "-")
        res = x + y if op == "+" else x - y
        return ["% engine", "> what is %d %s %d?" % (x, op, y), "! calc %d %s %d" % (x, op, y), "= %d" % res,
                "< %d %s %d = %d." % (x, op, y, res)]
    if k == 6:
        v = g.c("x", "y", "n")
        val = r.randint(1, 60)
        add = r.randint(1, 40)
        return ["% engine", "> let's say %s = %d" % (v, val), "! solve", "= kept 1 equation about " + v,
                "< Okay, I wrote down 1 equation about %s. Ask me for the one you want to know." % v,
                "> " + g.c("what is %s + %d?" % (v, add), "%s + %d = ?" % (v, add)), "! solve",
                "= When %s = %d, %s + %d = %d." % (v, val, v, add, val + add),
                "< When %s = %d, %s + %d = %d." % (v, val, v, add, val + add)]
    a = r.randint(2, 30)
    b = r.randint(2, 30)
    return ["% engine", "> what is %d + %d?" % (a, b), "! calc %d + %d" % (a, b), "= %d" % (a + b), "< %d + %d = %d." % (a, b, a + b),
            "> " + g.c("that's correct", "correct", "good job", "that's right"), "! praised", "= the last answer was right",
            "< Thanks! I'm glad I got it right."]

EPS = [(ep_missing, 5), (ep_newfolder, 2), (ep_unknown, 3), (ep_claim, 4), (ep_can, 3), (ep_mywords, 2), (ep_actions_talk, 2), (ep_mistakes, 4),
       (ep_goal, 2), (ep_find, 1), (ep_refs, 3), (ep_task, 1), (ep_profile, 4), (ep_profile_none, 1), (ep_world, 2), (ep_history, 3),
       (ep_recheck, 2), (ep_exists, 2), (ep_append, 1), (ep_social, 2), (ep_lead, 2), (ep_askme, 1), (ep_cancel, 2), (ep_days, 2), (ep_didnot, 1), (ep_bignum, 1), (ep_forgot, 1), (ep_rename_same, 1), (ep_honest, 1), (ep_taught, 1), (ep_selfask, 2), (ep_dateadd, 1), (ep_limits, 4), (ep_honest2, 3), (ep_honest3, 3), (ep_honest4, 3),
       (ep_saved, 3), (ep_solve, 2), (ep_engine_talk, 4)]

# what the engine and the skills work out, the "why?" after it and the
# questions asked back (tools/gen_skills_talk.py)
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from gen_skills_talk import EPS_SKILLS  # noqa: E402
EPS += EPS_SKILLS


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", default="data/sense_live.txt")
    ap.add_argument("--convs", type=int, default=20000)
    ap.add_argument("--seed", type=int, default=1)
    ap.add_argument("--code", default=None, help="the ai executable (default: ai.exe / ai next to tools)")
    a = ap.parse_args()
    r = random.Random(a.seed)
    g = G(r)
    tot = sum(w for _, w in EPS)
    a.out = os.path.abspath(a.out)
    raw = a.out + ".raw"
    with open(raw, "w") as f:
        for _ in range(a.convs):
            x = r.uniform(0, tot)
            for fn, w in EPS:
                x -= w
                if x <= 0:
                    break
            lines = fn(g)
            if not lines:
                continue
            if lines and lines[-1].startswith("> delete the"):
                which = "first" if "first" in lines[-1] else "second"
                files = [l.split(" ", 2)[2] for l in lines if l.startswith("! write ")]
                t = files[0] if which == "first" else files[1]
                lines += ["! delete " + t, "= moved to trash", "< Done, %s is in the trash." % t]
            hdr = [l for l in lines if l.startswith("%")]
            body = [l for l in lines if not l.startswith("%")]
            f.write("\n".join(hdr + HEAD + body) + "\n\n")
    exe = a.code
    if not exe:
        for name in (("ai.exe", "code.exe") if os.name == "nt" else ("ai", "code")):
            if os.path.exists(os.path.join(ROOT, name)):
                exe = os.path.join(ROOT, name)
                break
        else:
            exe = os.path.join(ROOT, "ai.exe" if os.name == "nt" else "ai")
    outp = a.out
    rawp = raw
    res = subprocess.run([exe, "--sense-filter", rawp, outp], cwd=ROOT, capture_output=True, text=True)
    sys.stdout.write(res.stdout)
    sys.stderr.write(res.stderr)
    os.remove(rawp)
    return res.returncode


if __name__ == "__main__":
    sys.exit(main())
