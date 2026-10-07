import os, re, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def self_name():
    try:
        path = os.path.join(ROOT, "mind_original", "self.txt")
        if not os.path.exists(path):
            path = os.path.join(ROOT, "brain", "self.txt")
        for ln in open(path, encoding="utf-8"):
            if ln.startswith("My name is "):
                return ln[11:].strip().rstrip(".") or "Lucy"
    except OSError:
        pass
    return "Lucy"


def swap(text, old, new):
    text = re.sub(r"(?<!Vinci )\b%s\b" % re.escape(old), new, text)
    return re.sub(r"\b%s\b" % re.escape(old.lower()), new.lower(), text)


def main():
    name = self_name()
    data = os.path.join(ROOT, "data")
    mark = os.path.join(data, "ai_name.txt")
    old = "Code"
    if os.path.exists(mark):
        old = open(mark, encoding="utf-8").read().strip() or "Code"
    if old != name:
        for f in ["persona_chat_clean.txt", "tasks_honest.txt", "valid.txt"]:
            p = os.path.join(data, f)
            if not os.path.exists(p):
                continue
            t = open(p, encoding="utf-8").read()
            t = re.sub(r"(?<!Vinci )\b%s\b" % re.escape(old), name, t)
            open(p, "w", encoding="utf-8", newline="\n").write(t)
            print("renamed %s -> %s in data/%s" % (old, name, f))
        open(mark, "w", encoding="utf-8").write(name + "\n")
    for src, dst in [("skills_eval_v17.txt", "eval_skills.txt"), ("user_v1.txt", "eval_user.txt")]:
        p = os.path.join(ROOT, "tests", src)
        if os.path.exists(p):
            t = open(p, encoding="utf-8").read()
            if name != "Code":
                t = swap(t, "Code", name)
            open(os.path.join(data, dst), "w", encoding="utf-8", newline="\n").write(t)
    print("the AI is called %s" % name)
    return 0


if __name__ == "__main__":
    sys.exit(main())
