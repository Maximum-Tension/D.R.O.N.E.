<p align="center">
 <img src="https://raw.githubusercontent.com/Maximum-Tension/D.R.O.N.E./main/docs/IMAGES/BANNERS/HEADER.png" alt="header" style="width: 70%;"/>
</p>

# D.R.O.N.E. - Dynamic Responsive Optimized Neural Engine

[![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.23137562.svg)](https://doi.org/10.5281/zenodo.23137562)

## [**[📜 D.R.O.N.E. - V1.2.pdf](https://docs.google.com/viewer?url=https://raw.githubusercontent.com/Maximum-Tension/D.R.O.N.E./main/docs/ARTICLES/EN/D.R.O.N.E.%20-%20V1.2.pdf)**]

**Dynamic Responsive Optimized Neural Engine**: a small conversational AI written from scratch in plain C, with no frameworks and no outside libraries.

An exact core decides what to do: it reads the request, does the work, checks that it really happened and fixes what went wrong. A small neural brain, trained on a normal CPU, then puts the result into words. Each AI built on the engine is a *drone*. The first one is **Lucy**.

```
$> Can you create me multiple files? Like 1.txt to 10.txt
Lucy> Done, I made the files 1.txt to 10.txt.
$> Remove 42
Lucy> The trash already has something called 42, so I put it in the trash as "42 (2)".
$> count from 1 to 5 and save it in numbers.txt
Lucy> Done, I wrote it to numbers.txt: 1, 2, 3, 4, 5
```

## What makes it different

- **She doesn't make things up.** Anything that can be worked out exactly, like math, time, files, counting or what you told her, comes from the core and never from the neural brain's guess.
- **She solves problems.** After every action she checks the result. If it failed, she finds the cause, tries the next untried fix and checks again. She never offers the same fix twice.
- **Skills are taught in plain lines.** A skill is a small plug-in (`.dll` / `.so`) plus a text file of know-how that says what each action makes true and what to try when it fails. You can edit that file while she runs.
- **Her words live in text files.** What she says (`sayings.txt`) and how she reads you (`meanings.txt`) are plain lines you can extend.
- **She remembers.** She keeps knowledge packages, facts about you, and the task she is working on ("it", "that folder").
- **She runs on a CPU.** The engine uses AVX2 + FMA, and there's an optional GPU trainer.

## Build

Windows (MinGW gcc in PATH):

    build.bat

Linux:

    ./build.sh

This makes `ai` (chat), `ai-train` (trainer), `ai-test` (engine tests), plus the skills in `mind/skills/`.

## Run

    chat.bat        (or ./ai --chat)

A trained brain is not part of this repository because it is too large. Train one with `train.bat` / `train_round.bat` (see the full manual), or download a release.

## Tests

    ./ai-test                                    engine tests
    ./ai --sense-eval tests/sense_v12.txt        understanding
    python3 tools/check_chat.py tests/work_v1.txt   whole conversations

## Layout

| Folder           | What is in it                                                             |
| ---------------- | ------------------------------------------------------------------------- |
| `src/`           | the engine: chat, core (SENSE), work solver (WORK), neural brain, trainer |
| `src/skills/`    | built-in skills (computer, reason)                                        |
| `mind/language/` | sayings, meanings, verbs: her words, editable                             |
| `mind/memory/`   | knowledge packages                                                        |
| `tools/`         | data generators, checks, training helpers                                 |
| `tests/`         | understanding and conversation checks                                     |

The full manual, covering every command, file and skill line format, is in [MANUAL.md](MANUAL.md).

## Copyright

 * D.R.O.N.E. is licensed under the GPL-3.0 License
 * © 2026 Maximum Tension™.
