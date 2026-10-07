#!/usr/bin/env python3
"""Chat checks: whole conversations through the real program, with what each
reply must (and must not) say.

    python3 tools/check_chat.py tests/work_v1.txt [--ai ./ai] [--show]
        [--template FOLDER]

FOLDER holds the brain to talk with (files, mind and mind_original as the kit
has them); without it the repository's own is used.

A test file is a list of conversations. Each starts with "=== name" and runs
in a fresh copy of the files and mind folders, so one can't spoil the next:

    === delete a folder, then say it is still there
    > Create a folder named 42
    ~ made the folder 42
    > Remove 42
    ~ trash
    > It is still there!
    ~ I just checked
    ! Done, I made
    /wait 3

"> text" is a message, "~ words" must be in the reply to the message before
it (case does not matter), "! words" must not be, "/wait N" lets N seconds
go by (what comes due then is the reply checked by the lines after it), and
lines starting with "#" are notes. The exit code is the number of failed
checks, so a script can stop on it.
"""

import os
import shutil
import subprocess
import sys
import tempfile


def read_tests(path):
    tests = []
    current = None

    for raw in open(path, encoding="utf-8"):
        line = raw.rstrip("\n")

        if not line.strip() or line.startswith("#"):
            continue

        if line.startswith("==="):
            current = {"name": line[3:].strip(), "steps": []}
            tests.append(current)
            continue

        if current is None:
            current = {"name": "unnamed", "steps": []}
            tests.append(current)

        if line.startswith("> ") or line.startswith("/wait"):
            text = line[2:] if line.startswith("> ") else line
            current["steps"].append({"say": text, "want": [], "not": []})
        elif line.startswith("~ ") and current["steps"]:
            current["steps"][-1]["want"].append(line[2:].strip())
        elif line.startswith("! ") and current["steps"]:
            current["steps"][-1]["not"].append(line[2:].strip())

    return tests


def fresh_folder(repo, template):
    """a folder to talk in: the brain from TEMPLATE (a folder with files,
    mind and mind_original, as the kit has them) or the repository, and the
    repository's language files, skills and knowledge packages"""
    folder = tempfile.mkdtemp(prefix="lucy_check_")
    base = template or repo

    for name in ("files", "mind_original", "mind"):
        source = os.path.join(base, name)

        if os.path.isdir(source):
            shutil.copytree(
                source, os.path.join(folder, name),
                ignore=shutil.ignore_patterns("locals", "persons", "topics")
            )

    mind = os.path.join(folder, "mind")

    for name in ("language", "skills"):
        source = os.path.join(repo, "mind", name)
        target = os.path.join(mind, name)
        os.makedirs(target, exist_ok=True)

        if os.path.isdir(source):
            for item in os.listdir(source):
                if item.endswith(".txt") or item.endswith(".so") or \
                        item.endswith(".dll") or item.endswith(".lex"):
                    if item.endswith(".lex") and template:
                        continue
                    shutil.copy(os.path.join(source, item), target)

    for item in ("self.txt",):
        source = os.path.join(repo, "mind_original", item)

        if os.path.isfile(source):
            os.makedirs(os.path.join(folder, "mind_original"), exist_ok=True)
            shutil.copy(source, os.path.join(folder, "mind_original"))

    # only the knowledge packages, never anyone's saved memories
    memory = os.path.join(mind, "memory")
    os.makedirs(memory, exist_ok=True)
    source = os.path.join(repo, "mind", "memory")

    for name in os.listdir(source):
        if name.endswith(".mem") or name == "catalog.txt":
            shutil.copy(os.path.join(source, name), memory)

    return folder


def replies(output, count):
    """the reply lines after each prompt, in order"""
    chunks = output.split("$> ")[1:]
    said = []

    for index in range(count):
        chunk = chunks[index] if index < len(chunks) else ""
        lines = [
            line.strip() for line in chunk.split("\n")
            if line.strip().startswith("Lucy>")
        ]
        said.append(" ".join(line[5:].strip() for line in lines))

    return said


def run_test(binary, repo, template, test, show):
    folder = fresh_folder(repo, template)
    messages = [step["say"] for step in test["steps"]]
    feed = "\n".join(messages + ["/quit"]) + "\n"

    try:
        result = subprocess.run(
            [binary, "--seed", "1", "--chat", "check", "--name", "tester"],
            input=feed.encode("utf-8"), cwd=folder, capture_output=True,
            timeout=600
        )
        output = result.stdout.decode("utf-8", "replace")
    finally:
        shutil.rmtree(folder, ignore_errors=True)

    said = replies(output, len(messages))
    failed = 0

    for step, reply in zip(test["steps"], said):
        lower = reply.lower()
        problems = [
            "missing: " + want for want in step["want"]
            if want.lower() not in lower
        ] + [
            "should not say: " + bad for bad in step["not"]
            if bad.lower() in lower
        ]

        if show or problems:
            print("  > %s" % step["say"])
            print("    Lucy: %s" % (reply or "(nothing)"))

        for problem in problems:
            print("    FAIL %s" % problem)

        failed += len(problems)

    return failed


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 1

    repo = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    binary = os.path.join(repo, "ai.exe" if os.name == "nt" else "ai")
    show = "--show" in sys.argv

    if "--ai" in sys.argv:
        binary = os.path.abspath(sys.argv[sys.argv.index("--ai") + 1])

    template = None

    if "--template" in sys.argv:
        template = os.path.abspath(sys.argv[sys.argv.index("--template") + 1])

    tests = read_tests(sys.argv[1])
    total = 0
    checks = sum(
        len(step["want"]) + len(step["not"])
        for test in tests for step in test["steps"]
    )

    for test in tests:
        print("=== %s" % test["name"])
        total += run_test(binary, repo, template, test, show)

    print(
        "chat checks: %d of %d passed%s" % (
            checks - total, checks, " - ALL PASSED" if not total else ""
        )
    )

    return min(total, 125)


if __name__ == "__main__":
    sys.exit(main())
