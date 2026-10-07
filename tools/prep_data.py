import csv, json, os, random, re, sys

NAMES = """james mary john patricia robert jennifer michael linda william elizabeth david barbara richard susan joseph jessica
thomas sarah charles karen daniel nancy matthew lisa anthony betty mark sandra donald ashley steven emily paul kimberly andrew
donna joshua michelle kevin carol brian amanda george melissa timothy deborah ronald stephanie jason rebecca edward laura
jeffrey sharon ryan cynthia jacob kathleen gary amy nicholas angela eric anna jonathan ruth stephen brenda larry pamela justin
nicole scott katherine brandon samantha benjamin christine samuel emma gregory rachel alexander catherine frank carolyn patrick
janet raymond maria jack heather dennis diane jerry olivia tyler julie aaron joyce jose victoria adam kelly nathan christina henry
lauren douglas joan zachary evelyn peter judith kyle megan ethan andrea walter cheryl noah hannah jeremy jacqueline christian
martha keith gloria roger teresa terry ann gerald sara harold madison sean frances austin kathryn carl janice arthur jean lawrence
abigail dylan alice jesse judy jordan sophia bryan grace billy denise joe amber bruce doris gabriel marilyn logan danielle albert
beverly willie isabella alan theresa juan diana wayne natalie elijah brittany randy charlotte roy marie vincent kayla ralph alexis
eugene lori russell bobby mason philip louis hakan emre mehmet ayse zeynep ali fatma can deniz elif kerem selin burak ece""".split()


def clean(s):
    s = s.replace("’", "'").replace("‘", "'").replace("“", '"').replace("”", '"')
    s = s.replace("—", " - ").replace("–", " - ").replace("…", "...")
    s = re.sub(r"\s+", " ", s).strip()
    return s


def dd_detok(s):
    s = s.replace(" ' ", "'").replace(" ’ ", "'")
    s = re.sub(r" ([,.!?;:])", r"\1", s)
    s = re.sub(r"\s+", " ", s)
    return clean(s)


def placeholder_sub(text, speaker, n1, n2):
    def rep(m):
        t = m.group(0).lower()
        if "1" in t:
            return n1
        if "2" in t:
            return n2
        return n2 if speaker == 1 else n1
    return re.sub(r"\[[^\]]*\]|\([^)]*name[^)]*\)|<[^>]*name[^>]*>", rep, text, flags=re.I)


def spc(path, out, rnd):
    rows = list(csv.reader(open(path, encoding="utf-8")))[1:]
    n = 0
    for r in rows:
        p1 = [clean(x) for x in r[0].split("\n") if x.strip()]
        p2 = [clean(x) for x in r[1].split("\n") if x.strip()]
        conv = [l for l in r[2].split("\n") if l.strip()]
        me = rnd.choice((1, 2))
        code_name = "code" if rnd.random() < 0.6 else rnd.choice(NAMES)
        other = rnd.choice(NAMES)
        n1, n2 = (code_name, other) if me == 1 else (other, code_name)
        mem = (p1 if me == 1 else p2)[:]
        you = (p2 if me == 1 else p1)[:]
        rnd.shuffle(mem)
        mem.insert(rnd.randrange(len(mem) + 1), "My name is %s." % code_name.capitalize())
        lines = ["# " + m for m in mem]
        if rnd.random() < 0.4:
            you.insert(0, "My name is %s." % other.capitalize())
            lines += ["@ " + y for y in you]
        for l in conv:
            m = re.match(r"\s*User\s*([12])\s*:\s*(.*)", l)
            if not m:
                continue
            sp = int(m.group(1))
            t = clean(placeholder_sub(m.group(2), sp, n1.capitalize(), n2.capitalize()))
            if t:
                lines.append(("< " if sp == me else "> ") + t)
        out.write("\n".join(lines) + "\n\n")
        n += 1
    return n


def daily(path, out, rnd):
    n = 0
    for line in open(path, encoding="utf-8"):
        turns = [dd_detok(t) for t in line.split("__eou__") if t.strip()]
        if len(turns) < 2:
            continue
        me = rnd.choice((0, 1))
        lines = []
        for i, t in enumerate(turns):
            lines.append(("< " if i % 2 == me else "> ") + t)
        out.write("\n".join(lines) + "\n\n")
        n += 1
    return n


def topical(path, out, rnd):
    d = json.load(open(path, encoding="utf-8"))
    n = 0
    for v in d.values():
        me = rnd.choice(("agent_1", "agent_2"))
        lines = []
        for m in v["content"]:
            t = clean(m["message"])
            if t:
                lines.append(("< " if m["agent"] == me else "> ") + t)
        if len(lines) >= 2:
            out.write("\n".join(lines) + "\n\n")
            n += 1
    return n


def sentences(text):
    parts = re.split(r'(?<=[.!?])\s+(?=[A-Z0-9"])', text)
    out, pos = [], 0
    for s in parts:
        i = text.find(s, pos)
        out.append((i, i + len(s), s))
        pos = i + len(s)
    return out


def squad(path, out, rnd):
    d = json.load(open(path, encoding="utf-8"))
    n = 0
    for art in d["data"]:
        for para in art["paragraphs"]:
            ctx = para["context"]
            sents = sentences(ctx)
            qas = para["qas"][:]
            rnd.shuffle(qas)
            for g in range(0, len(qas), 3):
                group = qas[g:g + 3]
                mem, turns = [], []
                for qa in group:
                    a = qa["answers"][0]
                    st = a["answer_start"]
                    idx = [k for k, (b, e, s) in enumerate(sents) if b <= st < e]
                    if not idx:
                        continue
                    s = clean(sents[idx[0]][2])
                    if len(s) > 400:
                        continue
                    if s not in mem:
                        mem.append(s)
                    ans = clean(a["text"]).rstrip(".")
                    if not ans:
                        continue
                    turns.append("> " + clean(qa["question"]))
                    turns.append("< " + ans[0].upper() + ans[1:] + ".")
                if not turns:
                    continue
                others = [clean(s) for (_, _, s) in sents if clean(s) not in mem and len(s) < 300]
                rnd.shuffle(others)
                for s in others:
                    if len(mem) >= 4 or sum(len(m) for m in mem) + len(s) > 450:
                        break
                    mem.append(s)
                if sum(len(m) for m in mem) > 600:
                    continue
                rnd.shuffle(mem)
                tag = "# " if rnd.random() < 0.75 else "@ "
                out.write("\n".join([tag + m for m in mem] + turns) + "\n\n")
                n += 1
    return n


def main():
    src, dst = sys.argv[1], sys.argv[2]
    os.makedirs(dst, exist_ok=True)
    rnd = random.Random(1234)
    files = os.listdir(src)
    def find(s):
        for f in files:
            if s in f:
                return os.path.join(src, f)
        return None
    with open(os.path.join(dst, "persona_chat.txt"), "w", encoding="utf-8") as o:
        print("persona chat", spc(find("Persona-Chat_train"), o, rnd))
    with open(os.path.join(dst, "valid.txt"), "w", encoding="utf-8") as o:
        print("valid", spc(find("Persona-Chat_valid"), o, rnd))
    if find("dialogues.txt"):
        with open(os.path.join(dst, "daily_dialog.txt"), "w", encoding="utf-8") as o:
            print("daily dialog", daily(find("dialogues.txt"), o, rnd))
    if find("train-v1.1.json"):
        with open(os.path.join(dst, "squad_qa.txt"), "w", encoding="utf-8") as o:
            print("squad qa", squad(find("train-v1.1.json"), o, random.Random(77)))
    if find("topical"):
        with open(os.path.join(dst, "topical_chat.txt"), "w", encoding="utf-8") as o:
            print("topical chat", topical(find("topical"), o, rnd))


main()
