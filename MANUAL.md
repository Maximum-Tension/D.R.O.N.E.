<p align="center">
 <img src="https://raw.githubusercontent.com/Maximum-Tension/D.R.O.N.E./main/docs/IMAGES/BANNERS/HEADER.png" alt="header" style="width: 70%;"/>
</p>

# D.R.O.N.E. - Manual

**Dynamic Responsive Optimized Neural Engine.** A small AI written from scratch in C: an exact core decides what to do, and a neural brain puts it into words. Each AI built on the engine is a drone. The first one is **Lucy**.

The program files are `ai.exe` (chat), `ai-train.exe` (trainer) and `ai-test.exe` (engine tests). Everything Lucy knows and remembers is in `mind\` and `mind_original\` next to them (see Folders).

## Name

The AI's name comes from the first line of `mind_original\self.txt`:

    My name is Lucy.

Change it there and the chat prompt, the core (so "Lucy, make a file..." is understood) and the practice data all follow. `train_round.bat` runs `tools\rename_ai.py`, which renames the AI in the chat data and makes the test copies `data\eval_skills.txt` and `data\eval_user.txt` with the new name. A brain trained under an older name still answers "My name is Code" from habit; the chat shows the current name until a training round teaches it.

## Build

Windows (MinGW gcc in PATH):

    build.bat

Linux:

    ./build.sh

This makes `ai.exe` (chat), `ai-train.exe` (trainer), `ai-test.exe` (engine tests) and the skills in `mind\skills\` (`reason.dll`, `computer.dll`; `.so` on Linux).
The CPU must support AVX2 + FMA (any Intel Core from 2013 on).

## Chat

    chat.bat
    chat.bat --name hakan --chat work

- `--name NAME` who you are (default: your Windows user name). Lucy keeps what you tell her about yourself in your own folder, `mind\memory\persons\<name>\` (`profile.txt` in plain text, `facts.db`), so she knows it in every chat, and can tell others what you told her ("Does Sylwia have a cat?").
- `--chat ID` which chat to continue (default `main`). Each chat has its own notes and working memory.
- `--temp 0.8` sampling temperature (lower = safer, higher = wilder).
- `--cands 4` how many replies Lucy drafts before answering (1 = fastest, 4 = default). If every draft breaks an honesty rule, Lucy drafts up to 8.
- `--pick 0.15` how Lucy chooses among its drafts: 0 = always the one its brain finds most likely, higher = more variety among the good ones (default 0.15).
- `--mmi 0.3` prefer replies that depend on what you just said (experimental; off by default because it lowers the skill score).
- `--no-learn` play mode: Lucy only thinks, never changes its brain. Uses the least memory (16-bit weights in RAM) and is the fastest. This is the mode for NPCs.
- `--agents` / `--no-agents` turn the agents on or off (see "Talking while Lucy works"). They are on by default when you type in a console, off for `--script`, `--eval` and piped input, so tests stay one line at a time.

Type after `$>`. The reply starts with the AI's name, and there is an empty line between messages so the chat is easy to read:

    $> what is 37 times 12?

      [calc 37 * 12 -> 444]
    Lucy> 37 times 12 is 444.

    $>

Lines in brackets show what the AI did or thought before answering.

Commands inside the chat:

| command                            | what it does |
| ---------------------------------- | ------------ |
| `/teach <fact>`                    | store a fact in global memory and train on it right away |
| `/teachself <fact>`                | store a fact about Lucy itself (e.g. `/teachself I live in a laptop.`) |
| `/learn <file.txt>`                | train on a text file from inside the chat |
| `/reflexes`                        | list Lucy's certain reflexes (exact answers/actions compiled into machine code) and the patterns it is still watching |
| `/rule <what you say> => <answer>` | teach a certain reflex; `=> !read todo.txt` makes it an action, `=> =what is your name` makes it mean the same as another message. Also works in plain words: "when I say ping, say pong" |
| `/mind`                            | show Lucy's inner log: what it noticed, learned, avoided or ignored |
| `/sleep`                           | Lucy practices recent conversations that got good feedback, then tests itself and undoes the change if it got worse |
| `/status`                          | show goal, what came to mind, the act/reply decision, actions, and every draft reply with its score |
| `/thought`                         | show working memory: topic, what is in mind, what came to mind, what it said before |
| `/history [N]`                     | show the last N lines of this chat's history file (every message, action and reply, with time) |
| `/context`                         | show exactly what the model reads before it answers |
| `/brain`                           | show the neuron web: sizes, areas, growth history |
| `/neuron <id>`                     | show one neuron: purpose, connections, weights, its machine code |
| `/do <action>`                     | run one of Lucy's actions yourself, e.g. `/do calc 12*7`, `/do recall key` |
| `/paste`                           | everything up to a line `/end` is one message, line breaks and all (for long texts with paragraphs) |
| `/skills`                          | list the skills in `mind\skills\` and what each can do |
| `/packages`                        | list the knowledge packages in `mind\memory\` and how many things each knows |
| `/jobs`                            | list the work the agents are doing or did, with progress |
| `/stop`                            | stop the work in progress (plain "stop", "wait" or "never mind" also work) |
| `/save`                            | save everything now (also saved on exit and every 20 turns) |
| `/quit`                            | save and leave |

## Actions

Lucy can act while it answers. After your message the brain itself decides: reply, or act first. When it acts you see it:

    $> what is 37 times 12?

      [calc 37 * 12 -> 444]
    Lucy> 37 times 12 is 444.

| action                                                                           | what happens |
| -------------------------------------------------------------------------------- | ------------ |
| `remember <fact>`                                                                | stores the fact (`my ...` / `I ...` go to your people file, anything else to global facts) |
| `recall <words>`                                                                 | searches all memory and gives back the best matching memory |
| `forget <words>`                                                                 | deletes the best matching memory |
| `calc <expression>`                                                              | exact math done by the computer, not the neurons: `+ - * / % ^ ( )`, decimals, `pi`, `sqrt abs round floor ceil sin cos tan ln log exp min max pow`, and every taught formula |
| `formula <name>(<params>) = <expression>`                                        | stores a formula, e.g. `formula circle area(r) = pi * r ^ 2`; afterwards `calc circle area(5)` works. Kept in `mind\memory\general\formulas.txt` |
| `formulas`                                                                       | lists the taught formulas |
| `count <letter> in <word>` / `count letters in <word>` / `count words in <text>` | exact counting, e.g. "how many e's are in seven?" -> `count e in seven` -> 2 |
| `range <from> <to> [step <n>]`                                                   | exact counting, e.g. "count from 10 to 20" -> `range 10 20`; up to 150 numbers per answer |
| `reverse <items>`                                                                | exact reversal of letters, numbers, words, or the letters of one word |
| `random <from> <to>`                                                             | a random whole number, e.g. "pick a number between 1 and 30"; "roll a die" -> 1 to 6 |
| `export <name>`                                                                  | moves the facts taught in this chat into the topic file `mind\memory\topics\<name>.txt` (Lucy names it itself if you don't) |
| `topics`                                                                         | lists the topic files and how many facts each has |
| `topic <name>`                                                                   | shows what is in a topic |
| `lookup <word>`                                                                  | reads the knowledge packs (only when Lucy decides it needs them), e.g. "what is a folder?" |
| `memories`                                                                       | lists the last things you asked Lucy to remember (also across chats) |
| `goal <task>` / `goal done`                                                      | sets or clears what you and Lucy are working on; the goal stays in Lucy's mind every turn |
| `time`, `date`                                                                   | the computer's clock |
| `files [folder]`                                                                 | lists what is in `files\` (or a folder inside it) |
| `read <name>.txt`                                                                | reads the start of a file, e.g. `read work/todo.txt` |
| `write <name>.txt <text>`                                                        | makes a new file (never overwrites) |
| `append <name>.txt <text>`                                                       | adds a line to a file, copying the layout of the other lines (e.g. `- milk`, `1. ...`, padded columns) |
| `mkdir <folder>`                                                                 | makes a folder |
| `move <file> to <folder>`                                                        | moves a file into a folder |
| `rename <file> to <new name>`                                                    | renames a file or folder |
| `delete <file or folder>`                                                        | moves it into `files\trash\` (nothing is really deleted) |

Topic files are plain text, one fact per line: you can open, edit, copy or delete them yourself. Lucy searches all of them when something comes to mind. Global facts (`mind\memory\general\facts.db`) are shared by every chat and every person; `my ...` / `I ...` facts stay in your own people file.

**Math is exact.** When your message contains a written calculation (`12*7+3`, `5 plus 3 times 2`, `20% of 150`, `round 68.6`), the calc action copies it exactly as you wrote it; the neurons only decide *that* it is math. When Lucy then talks about it, it may only state math the calculator confirmed: it cannot write `3 plus 4 is 11`, and every number in that reply must be one you wrote or one the calculator returned.

**Honesty.** A draft that claims an action that did not happen ("I saved it", "here they are") or states wrong math loses to any honest draft; Lucy drafts more (up to 8) until it has one. If an action fails and a later action fixes the cause (the folder was missing, so Lucy made it), Lucy retries the failed action once. See `/mind`.

File actions only work inside the `files\` folder next to ai.exe (names with `..`, `\` or `:` are refused), and memory actions only write to Lucy's own `mind\memory\` folder. Lucy never touches other files and never runs programs. The result of an action goes back into Lucy's context, and Lucy writes its answer from that result. Lucy can chain up to 6 actions per message part.

## Long messages and near memory

**Near memory.** Lucy keeps the last 64 messages of the chat (your messages, its actions and results, its replies) and reads up to 512 tokens of them before answering. When the chat is longer than that, the older turn that best matches your new message is brought back into view, so "why did you say 6 before?" can still find the 6. Every turn is also written to `mind\memory\locals\chat_<id>\history.txt`; `/history` shows it.

**Plans for long messages.** A message with several parts is split and worked on one part at a time. It splits at sentence ends (`. ! ?`) and at `and then`, `, then`, `then`, `after that`, `lastly`, `finally`, `also` and `;`. It never splits inside "quotes", after "if … then", or in "when I say … say …" rules:

    $> Can you create me test2.txt file and then test3.ini file, and lastly delete test.txt file?
      [plan] 3 parts: (1) Can you create me test2.txt file (2) test3.ini file (3) delete test.txt file?
      [write test2.txt -> written]
    ...

Every part goes through the full brain with the earlier parts in view, so "test3.ini file" can mean "create test3.ini". Parts that are not the last one carry a hidden `<more>` marker, and the brain decides for each part: act, answer, or stay quiet and wait for the rest (`[noted part 1]`). Example: "Tom has 5 apples. He buys 3 more. How many does he have?" is read as three parts and answered once. To keep text together, put it in quotes.

If the brain is about to repeat an action it already did for the same part, it stops (see `/mind`).

## No made-up chat

The voice is a small brain trained partly on human chat, so when nothing was found for a message it likes to invent things: "Yes, I found some money.", "I'm having a long day.", "Why are you going to date?", "I'm argue, but it's not the only one.", "Let's suppossed!". Whenever nothing was done or found for the message (plain chat, and also an order or a task question that led to nothing), every draft has to pass one more check:

- **Plain talk** that claims nothing is always fine: greetings, thanks, sorry, "I see.", "That sounds fun!", "I'm sorry to hear that.", "I'm still learning." The list is `mind\language\plain_talk.txt`; add a line to allow a sentence, remove one to stop it.
- **It has to fit the message**: no goodbye unless you say goodbye, no "Yes!" or "Exactly" to a question nothing answered, no new topic or question after "Good." / "Anyway" / "Lmao" (only offers to help), an apology when you are confused ("What?", "I don't understand you", "Okay what?"), and never a question you told her not to ask.
- **Anything else has to come from the conversation**: at most 2 sentences and 20 words, no word said twice, every meaningful word from your message (or a name, or a result, or what is saved when you ask about it), every number too, and no word you only misspelled ("suppossed" is not said back). A reply to something you told her has to take up a word of that message; old messages and things you said in passing are not mixed into a new sentence ("I wait until when I say something is in the yours." is gone). What you said about her is not handed back as if about you ("you are doing great" -> not "I'm glad you are doing something"). A draft the brain itself found hard to say (low fluency, like "Why are you so many failed?") is not used either. After praise, only plain talk ("Thank you!"). A lone "Yes." or "No." to a question nothing answered is not said. A yes/no question about one thing is not answered with another name ("Do you remember UTZR?" -> not "It was Ali."). The words she may use come from: your message, the names, what was done this turn, what she did to the files in this chat ("is it done?" -> "Yes, I already made the file fep.txt."), the goal, what is saved (when you ask about it) and what she is (when you ask about her). In a task, a short question about it, or a reminder made of the task's words, is also fine ("I'm still waiting for the file name. What should I name the file?").
- When no draft passes, the engine says a plain reply that fits the message instead: "Hi! How are you?" to a greeting, "You're welcome!" to thanks, "Thank you!" to praise, "I'm sorry to hear that." to bad news, "Sorry, that came out wrong. What would you like me to do?" when you are confused, "I'm not sure how to answer that. Can you ask it another way?" to a question, "Sorry, I didn't understand what to do. Can you say it another way?" to an order that led to nothing, "Sorry, I can't do that. ..." to a request it has no way to do, "Okay." to a short "Good."/"Anyway", "I see." to anything else. A greeting is always greeted back.
- **"What?" after a real answer** says it again ("I said: ..."); "Okay what?" after "Okay." -> "I just meant okay. What would you like to do next?"; "What do you mean by "Let's suppossed"?" when she never said it -> "I didn't say "Let's suppossed". My last reply was: ..."
- **When you ask about words she said for no reason**, she says so instead of inventing a meaning: "What do you mean?" -> "Sorry, that came out wrong. It didn't mean anything."; "Why did you say that?" -> "That was a mistake. It wasn't based on anything you said, so it didn't mean anything. Sorry about that." The `[my words -> ... (not based on anything)]` line in the log is that bookkeeping: the core checked her last words against the conversation and found nothing they came from.
- A stretched word is read as the word it stretches: "Hellloo!!!" -> "Hello!!!", "sooo" -> "so". "I asked (from) you to create a file" is read as "Create a file".
- `--free-voice` turns this check off.

## Who says it: the brain or the engine

When the engine finds the answer itself (a sum, a solved equation, what is saved about you, a "no"), the brain reads that result and gets the first try at saying it, in its own words. Its draft is used only when it says the same thing: every number and name of the result, every word that means something, the same "not"s, nothing new besides plain talk and your own words, and it passes every other check. Otherwise the engine's sentence is said. "No" answers (denials) and quotes are always the engine's.

    stated: Okay, I'll remember that your dog is called Rex.
    said:   Okay, I will remember that your dog is called Rex.      (the brain's own)
    draft:  It has two dogs.                                         (rejected: says something else)

The practice conversations (`tools\gen_sense.py`) now include remembering, recalling, the memories list and solving, written several ways, so after the next training round the brain can say more of these itself and fewer replies come word for word from the engine. Set `VOICEDBG=1` to see every draft and why it lost. `--engine-voice` turns the brain's try off (the engine's sentence every time).

## When you say she is wrong

- **About something you told her**: "What is my cat's name?" -> "Your cat's name is Pamuk." -> "Wrong. My cat's name isn't Pamuk." -> "But you told me that your cat's name is Pamuk. Did it change? Tell me the new one and I'll remember it." Asked to argue ("aren't you going to argue?", "fight back"), or told "wrong" again: "I still think your cat's name is Pamuk, because that is what you told me. If it changed, tell me the new one." A new value ("wrong, it's actually Tom") is taken as usual.
- **About a sum**: she works it out again: "I checked again: 10 + 4/2 = 12, not 13." (or "But that is what I said: ..." when you give the same number).
- **About an unknown**: "I checked again: when x = 42, x + 1 = 43." "Wrong. x + 1 = 43 if x is 42" about a right sum: "But it is right: when x = 42, x + 1 = 43."
- **About files**: "You didn't even create the folder called asd" -> she looks: "But I did: there is a folder called asd in the files folder." (or "You're right, there is no folder called asd ..."). "Do you even have a folder called asd?" -> "Yes, there is a folder called asd in the files folder." "Did you create the folder?" -> "Yes, I made the folder bla, and then renamed it to asd."
- "Is your memory corrupted?" -> "I don't think so: my memory and brain files loaded fine. I can still get things wrong, though. If something I said looks off, tell me."
- "That's correct", "good job" after an answer: "Thanks! I'm glad I got it right."

## Counting, stopping, listening

- "count to 100", "count from 1 to 100", "count 1 to 100 without stopping", "cont from 1 to 100", "count down from 10" give the whole list (up to 150 numbers), never cut off: "Here you go: 1, 2, ..., 100."
- Every count is done by the engine (the voice used to cut long counts off, and sometimes got short ones wrong: "6, 7, 6, 5, ..."). A list longer than a reply may be ("say all the numbers up to 100") is shown whole instead of "...".
- Right after a count: "what about 100?", "not to 10, I said 100", "count it", "Count!", "you stopped at 37", "continue", "do it" count again (to the new number when one is given).
- A long whole-number sum (a number with 16 digits or more, + - *) is worked out exactly, digit by digit: "4918...837 - 1 = 4918...836."
- With an unknown known: "x + 23 = ?", "what is x + 1?", "if x is 42, what is x + 23?" -> "When x = 42, x + 23 = 65." A claim is checked: "x + 1 equals to 43 if x is 42" -> "Yes, that's right: when x = 42, x + 1 = 43."
- "Stop", "just stop", "could you please stop" with nothing running: "Okay, I stopped. Go ahead."
- "Are you listening?", "do you even understand me?": "Yes, I read every message. When I get something wrong, tell me and I'll try again."
- "Don't ask me that (question)" / "stop asking": her last question is never asked again in this chat: "Sorry. I won't ask you that again."
- "No wait, I changed my mind. Instead of bla, let's make the folder's name asd" renames what she just made: "Okay, I renamed bla to asd." Such sentences, counts and corrections are not saved as taught facts.
- File actions and counts are said as they happened ("Done, I made the file asd/env.txt.", "There is already a folder called asd.", "I couldn't make asd/env.txt: there is no folder called asd. Should I make it first?"); the brain may say the same thing in its own words, word for word in order apart from a few small words.

## Saved memories

- **Remember forms**: "Remember this: I have a cat named Mjolnir", "remember that ...", "keep in mind that ...", "save (the) information that ...", "save that ...", "note that ...", "don't forget that ...". What follows is saved as you wrote it, and said back: "Okay, I'll remember that you have a cat named Mjolnir."
- **"Do you have any memories saved?"**, "what do you remember?", "anything you can remember?": what you asked her to keep (and what you taught her), newest first: "Yes. You told me that you own one cat named Mjolnir, and that you have a cat named Mjolnir." With nothing saved: "No, nothing is saved yet. Tell me something and I'll remember it." Things you only mentioned in passing are not listed as memories.
- **About one thing**: "how many cats do I have?", "please, list the cats that I own", "did you save the information about my cat?" -> "Yes. You told me that you own one cat named Mjolnir." or "Nothing about your car is saved. Tell me, and I'll remember it." These no longer start a file listing.

## Equations and word problems

Unknowns can be written as math or as a word problem, and are solved exactly (elimination, no guessing):

    > [Bat] + [Ball] = 1.10 and [Ball] + 1 = [Bat]. Can you calculate the value of the ball?
    Lucy> Ball = 0.05, and Bat = 1.05.
    > A bat and a ball cost $1.10 together. The bat costs $1 more than the ball. How much is the ball?
    Lucy> The ball costs $0.05, and the bat costs $1.05.
    > Tom has 5 apples more than Ann. Tom and Ann have 15 apples together. How many apples does Ann have?
    Lucy> Ann has 5 apples, and Tom has 10 apples.

- Math form: unknowns in `[brackets]` or single letters (`x`, `2x`, `3(x - 1) = 2x + 4`); several equations split by "and", commas or sentences.
- Word form: "A and B cost N together", "A costs N more/less than B", "A costs twice / 3 times as much as B", "A costs N" (also "has", "weighs", "is").
- The equations are kept for a few messages, so you can ask later ("What is y?", "solve it"). Equations alone get "Okay, I wrote down 2 equations about x and y. Ask me for the one you want to know." Too few equations: "I can't work out x yet. I need one more equation about it."; equations that disagree say so.
- Word problems are not saved as taught facts any more.

## Undo

"Undo it", "revert", "change it back", "rename it back", "back to its original name", "put it back" undo the last change Lucy made in `files\`: a rename or move goes back, a deleted file comes back out of the trash, a file or folder she made goes to the trash. Only changes made since `ai.exe` was opened can be undone. "What was the folder "need" called before?" -> "need was called ASD before."

## Talking while Lucy works (agents)

One `ai.exe` runs several agents on their own threads, so you never need a second copy open:

- **The input agent** reads what you type all the time, even while Lucy is answering or working. Nothing you type is lost; messages wait in a mailbox.
- **The conversation agent** answers one message at a time. Between the parts of a long answer it checks the mailbox: `stop`, `wait`, `never mind` end the answer right there ("Okay, I stopped."), and a message starting with `also`, `and`, `btw`, `one more thing`, `I forgot to say` joins the message it is working on as one more part (`[agents] more for this message: ...`). Anything else is the next message.
- **Job agents** do long work in the background. `study notes.txt` (also "learn from notes.txt", "memorize notes.txt", "read notes.txt and learn it") reads a file from `files\` sentence by sentence and learns every fact in it, with the file as its source. You can keep chatting meanwhile; "what are you doing?" or "are you done?" gets the progress ("I'm still studying notes.txt: 704 of 4000 sentences so far."), `stop` stops it, `/jobs` lists the jobs. When it is done Lucy says what came of it: how many facts are new, how many she already knew, which ones disagree with what someone taught her (those are not believed), and how many questions the file asks.
- A text pasted into the console arrives as one burst of lines and is read as one message. `/paste` ... `/end` does the same on purpose (and works in scripts too).

## Reading long messages

A message of 300 characters or more with 4 or more sentences is read like a text, not chatted to:

    $> /paste
    Octopuses are clever animals that live in the ocean. An octopus has three hearts and blue blood.
    They can change the color of their skin to hide from predators. A predator is an animal that hunts other animals for food.
    ...
    Do you understand?
    /end
      [read] a long message: 8 sentences
      [read] learned: Octopuses are clever animals that live in the ocean.
      ...
    Lucy> Yes, I think so. I read all of it and learned that octopuses are clever animals that live in the ocean, that an octopus has three hearts and blue blood, that octopuses can change the color of their skin to hide from predators, and 4 more things.

Every sentence is sorted first: a fact (learned with you as its source), a question or a request (each answered on its own afterwards, with the rest of the text already learned), something about you ("my cat is called Bobo", kept by the sense core), or just talk. "They", "it" and "this" at the start of a sentence are replaced by what the sentence before was about ("They can change the color..." is kept as "Octopuses can change the color..."). Then Lucy says in one reply what she learned, what she already knew and what she disagrees with. "Do you understand?" is answered from the reading itself, and when a word was new to her she asks about exactly that word and where it was: `What does "fossil" mean in "A fossil from a previously undiscovered species"?` instead of "What does it mean?".

## Taught facts, sources and disagreements

Facts people teach in plain sentences ("A name is a word we use to tell things apart.", "Dinosaurs never went fully extinct.") are kept in `mind\memory\general\taught.txt` with who taught them, when, and the source they gave, and they are there in every chat and after every restart:

- "What is a name?", "What does let's mean?" -> the definition, as it was taught.
- "What did I tell you about dinosaurs?" -> "You taught me that dinosaurs never went fully extinct."
- "Who told you that?" or "Who told you that penguins live in the south?" -> "Sylwia told me that on October 5, 2026, and said: 'I read it on the Nature news website.'" (or "I read that in the file animals.txt on ..." for a studied file).
- "Did dinosaurs go extinct?" -> "No. Sylwia taught me that dinosaurs never went fully extinct."
- "How many bits are in a byte?", "8 bits are what?" -> "You taught me that one byte is 8 bits." (a fact holding every word asked about).
- "Do you remember what I taught you?" -> "Yes. You taught me that one byte is 8 bits, and that a bit is ..." (the newest three).
- "Where is it used?" about something taught, with no answer taught -> "I don't know that. All I know is what you taught me: a bit is ...".
- "Can I teach you something?" -> "Yes. Tell me, and I'll remember it, even after you close me."
- Short facts count too ("A byte has 8 bits.", "Cats are mammals."), and lead-ins are read past ("I am teaching you this again, a byte is 8 bits.", "Remember this: ...").
- A source can come in the same sentence ("according to ...", "I read it in ...", "source: ...", a link) or in the next message ("I read it on the Nature news website.").

When someone else says the opposite, Lucy does not just believe them:

    $> Dinosaurs went fully extinct.
    Lucy> I can't agree yet. Sylwia taught me that dinosaurs never went fully extinct, and said: "I read it on the Nature news website." Then what about that? How do you know that dinosaurs went fully extinct?
    $> Dinosaurs went fully extinct!
    Lucy> You still haven't shown me where that comes from. Sylwia taught me that dinosaurs never went fully extinct. How do you know?
    $> I learned it from my school book.
    Lucy> Okay, you showed me where it comes from: "I learned it from my school book." I believe you now: dinosaurs went fully extinct. But Sylwia had a source too ("I read it on the Nature news website"), so the two sources disagree. If you find out which one is right, tell me.

The same person correcting themselves is simply updated. A claim that comes with its source wins at once ("..., but you showed me where yours comes from"). Sums are never taught ("one plus one is five" stays wrong: the math core computes), and chat, orders, jokes, slang, things about the moment ("now", "today") and things about you or Lucy are not stored as facts. "forget X" also takes X out of the taught facts.

## How Lucy learns while you talk

- **Reflexes (certain answers).** When the same message leads to the same successful action twice, or once plus your "thanks", Lucy compiles it into a reflex: an exact switch/case in machine code. From then on that message is answered with certainty, without guessing. "No, that's wrong" deletes the reflex that caused it.
- **Safe chat learning.** While you chat, the main brain never changes. Chat learning goes into a small set of personal neurons (64 per layer) that start silent. If chat learning ever goes wrong, `/reset-learning` makes them silent again; the main brain was never touched. A message you already sent recently is not learned from again, so repeating yourself cannot bend the brain.
- **Feedback.** "thanks" / "good job" strengthens the neurons behind the last answer. "no" / "that's wrong" makes Lucy avoid that exact answer from then on, and also weakens the neurons behind it (an "avoid this" learning step: the brain lowers the chance of those words in that situation, without touching the rest).
- **Action guard.** Lucy only takes an action (create, delete, move, rename, remember, forget, goals...) when your words ask for that kind of action. "There is a folder called x" is not a request, so nothing is created. `--no-guard` turns it off.
- **Rules.** "When I say ping, you say pong" (or "say pong when I say ping", "if I type 42, answer ...") becomes an exact rule right away. `/reflexes` lists them.
- **Instant memory.** What you tell Lucy about yourself or the world ("my sister lives in Oslo") is stored at once, in Lucy's voice ("your sister lives in Oslo"), as brain states pointing at the next word. When Lucy later writes a sentence that reaches the same state ("your sister lives in"), the memory adds its word ("Oslo"). One hearing is enough; no training needed. It lives in `mind\memory\general\instant.*`, survives restarts, and is rebuilt when the brain changes. `--no-instant` turns it off. How often it helps depends on the brain: it only fires when Lucy's own sentence reaches the stored state.
- **Noise.** Random letters ("dvqsdvqs") are recognized as noise and never learned from.
- **Questions.** After asking you something, Lucy prefers not to ask again right away.
- **Background mind.** While you are quiet, Lucy notices the time passing and watches its `files\` folder (new or removed files). See `/mind`. Turn it off with `--no-mind`.

## Knowledge packages (`mind\memory\*.mem`)

What Lucy knows about the world comes from knowledge packages: plain text files in `mind\memory\`, one thing per line.

    cat, kitty: A cat is a small furry animal that many people keep as a pet. | can: climb, jump | kind: animal, mammal, pet | legs: 4 | young: kitten

- `things_object.mem` (animals, food, objects, weather, what things are like), `words.mem` (opposites and word kinds), `computers.mem` (files, folders, programs) come with her. `packages\places.mem` (countries, capitals, cities) is ready but not installed.
- **No restart.** Before every message Lucy checks the folder: a package dropped in is known at once, a changed one is read again, a removed one is forgotten.
- **She says which package is missing.** `mind\memory\catalog.txt` lists every package that exists and the words it covers. Asked "What is the capital of France?" without `places.mem`, she says: "I don't have that information yet. Did you download the package places.mem and put it in my mind/memory folder? Once it is there, I'll know it right away, without a restart."
- `tools\make_packages.py` writes the packages and the catalog from its tables; edit the tables or the `.mem` files themselves.
- `/packages` in the chat lists what is loaded.

## Skills (`mind\skills\*.dll`)

A skill is a library in `mind\skills\` (`name.dll` on Windows, `name.so` on Linux) with one exported function, `__memory__()`. It tells Lucy what the skill can do, how people ask for it and how to say what came of it, so she can use a new skill at once, without training (the contract is `src\SKILL.h`). Skills are loaded at start and again whenever one is added, changed or removed.

- `computer.dll`: everything Lucy does on the computer: folders, files, the trash, copy, exists, the clock and the date, system info. Only inside `files\`. Without it she says so honestly: "I can't do that right now: my computer skill (computer.dll) isn't in my mind/skills folder..."
- `reason.dll`: puzzles with one right answer, worked out exactly: sequences (numbers and letters), primes, even/odd, divisibility, logic ("all bloops are razzies..."), if-then rules (including the classic traps: "the ground is wet, did it rain?" -> "I can't tell"), who is tallest/oldest, letters of words, days, sorting, units, averages, clock times, fractions, percentages, prices ("3 pens cost $6, 5 pens?"), speeds, ages, trick questions ("all but 9 die", "a kilo of feathers"), analogies, odd one out, and facts from the knowledge packages ("how many legs does a spider have?", "can a fish climb a tree?", "is fire hot?").
- `/skills` (or "what skills do you have?") lists them.
- **Know-how.** A skill also says, in plain lines, what each action makes true, how Lucy checks it, and what to try when it fails (`KNOW_HOW` in `src\SKILL.h`). Lines in `mind\skills\NAME.txt` are read the same way, before the skill's own, so you can teach a skill new tricks or change old ones without building anything:

      trash: makes exists {1} = no
      trash: if trash has a file with that name: the trash already has a {name 1}; ask move {1}<tab>{free trash/{name 1}} = put it in the trash as "{free}"
      trash: if user disagrees: find; look list {folder 1} = {place {folder 1}}; look list trash = the trash
      erase: kind risky

  `makes` is how she checks it is done, `if RESULT` lists the fixes for that cause, safest first: `try` does it, `ask` waits for your yes (so does any action of `kind risky`), `look` only looks (to show you what is there), `find` looks for the thing somewhere else, `done` means it already is the way it should be. `{1}` `{2}` are the arguments, `{name 1}` the last part of a path, `{folder 1}` the folder it is in, `{free PATH}` the first free name like PATH ("42 (2)"), `{place X}` how a folder is said ("your files folder"). The file is read again when it changes.

## Work: doing it, checking it, fixing it

Lucy keeps a list of what she was asked to do, and works each task the same way, whatever the skill:

1. **Check.** After an action she looks whether it really happened (the skill's `makes` line). If she can't look, she says what the action answered.
2. **Find the cause.** When it didn't happen, the cause is what the action answered ("trash has a file with that name").
3. **Plan and try.** She takes the first fix for that cause she hasn't tried. A fix that could lose something waits for your yes.
4. **Repeat** until it is done, or no fix is left; then she tells you what she tried.

        $> Remove 42
          [delete 42 -> trash has a file with that name]
          [check: exists 42 -> folder: not done yet]
          [cause: trash has a file with that name]
          [try: move 42 -> trash/42 (2): moved]
          [check: exists 42 -> no: done]
        Lucy> The trash already has something called 42, so I put it in the trash as "42 (2)".

- **When you say it isn't done.** "It is still there!" is a problem too, with its own cause: she looks again, and if it is done, she gathers more each time you say so, from the skill's `if user disagrees` fixes: another thing with that name somewhere else ("But there is another 42: work/42. Is that the one you mean?"), what is in the folder now, what is in the trash (what you named first: "it is not in the trash" looks in the trash). When nothing is left to look at, she asks where you see it.
- **Later.** Any request can wait for its time: "create a folder named 42 5 seconds later", "delete notes.txt in 2 minutes", "tell me what 2 + 2 is in 3 seconds", "in 10 seconds, tell me the time". Changes wait as tasks ("you did it wrong!" before the time: "I haven't done it yet"); everything else is answered then, the whole request at once. "Never mind" or "cancel" calls it off.
- **Several at once.** "Create folders x, y and z", "delete x and y": one task each, said in one sentence.
- **What we are working on.** "it", "that folder", "the one we talked about", "which folder were we talking about?", "which one?" are the task worked on last. "What did you do?" lists her changes; "what are you doing?" says what is waiting.
- **No going round in circles.** A fix you said no to is never offered again (unless you say "ok, do it" after all), and "try something else" while she asks counts as that no. When the skill's own fixes are used up and you push, every push does something new: the action once more (things may have changed), a look for names like it ("ghosts.txt" for "ghost"), asking what she lacks, and once, plainly, that she knows no other way and can be taught one. She never sends the same reply twice in a row for it.
- **Putting skills together.**
  - Counting with files: "create 1.txt to 10.txt", "Can you create me 10 files from 1.txt to 10.txt?", "delete file1.txt through file5.txt", "do that till 10.txt" (on from the last one she made). One change for each, checked, and said in one sentence ("Done, I made the files 4.txt to 10.txt. 1.txt to 3.txt were already there.").
  - A row inside a longer request is written out for the planner: "make a folder called archive and move 1.txt to 3.txt into it" is read as "... move 1.txt, 2.txt and 3.txt into it".
  - One step's result into a file: "count from 1 to 5 and save it in numbers.txt", "write the time into time.txt", "What is 12 * 7? Write the answer to answer.txt", "save that to notes.txt" (what she just said).
  - Counting in a file: "how many words are in notes.txt?", "read notes.txt and tell me how many words it has".
  - "move a.txt to b.txt" when there is no b.txt renames it.
- **What she can do.** "Do you know how to count?", "can you count?", "so you can't count?" are answered from her own skills and tools (the `can CLASS` lines of `sayings.txt`, or what a skill says about its action), not from what she was told about the word.
- **Her words for it** are in `mind\language\meanings.txt` ("still there", "try again", "is it done", "yes", "no", ...): add a phrase to a line and she understands it from the next message.
- `/wait 5` lets five seconds go by (for tests): what comes due is said.

## Why? Are you sure?

Right after an answer, "why?", "how do you know?", "show your work", "explain", "how did you get that?" make Lucy show how she found it, and "are you sure?", "really?", "double check" make her work it out again:

    $> I have 17 apples. I give 5 to John and then John gives me 2 back. How many apples do I have?
    Lucy> You have 14 apples.
    $> why?
    Lucy> I took the numbers from your message and worked it out step by step: 17 - 5 = 12, and then 12 + 2 = 14.
    $> what comes next: 3, 6, 9, 12?
    Lucy> The next number is 15: each number is 3 more than the one before.
    $> are you sure?
    Lucy> Yes. Each number is 3 more than the one before: 3 + 3 = 6, 6 + 3 = 9, 9 + 3 = 12, and 12 + 3 = 15.
    $> how many legs does a spider have?
    Lucy> A spider has 8 legs.
    $> how do you know?
    Lucy> Because my things_object.mem package says a spider has 8 legs. That's where my facts come from; I don't make them up.

Sums are shown step by step, word problems with the numbers taken from the message, equations by putting the answer back into every equation, letter counts by spelling the word out, decimals by lining them up (9.11 vs 9.90), facts by the package they came from, and what you told her by quoting you. "Are you sure?" after a skill's answer really runs the skill again; if the answer came out different, she says so.

## Asking back

When something is missing, Lucy asks for it, and the next message finishes the job:

    $> send me a message later
    Lucy> Sure. When should I send it? For example: in 5 minutes, or at 6:30.
    $> in 5 minutes
    Lucy> Okay, I'll send you a message in 5 minutes.
    $> convert 5 to meters
    Lucy> 5 what? Tell me the unit too, like: convert 5 feet to meters.
    $> feet
    Lucy> 5 feet is about 1.52 meters.

The same for "what comes next?" without numbers and "how many legs does it have?" (she first looks for "it" in what you said a moment ago: "I saw a cat in the garden." -> "A cat has 4 legs.").

## Sayings (`mind\language\sayings.txt`)

The sentences Lucy says herself (explanations, questions back, reminders, the package hint, jokes, what she is) are not written into the program: they are lines in `mind\language\sayings.txt`, `key: sentence`, with `{1}`, `{2}` for the details. Change a sentence and the next message uses it, no restart. Give a key several lines (like `joke:`) and she takes turns with them. Skills use the same file through the skill host (`SAY`).

## Training her to say it herself

`tools\gen_sense.py` (run by `train_round.bat` every hour) now mixes in `tools\gen_skills_talk.py`: practice conversations where the engine or a skill found the answer and the brain says it, the "why?" and "are you sure?" after it, the questions she asks back and the answers that finish them, package hints, jokes, other people's facts. They are marked `% skill` so `ai.exe --sense-filter` keeps their steps exactly as written. Run a training round and the brain learns to put all of this in its own words; until then the engine says these sentences itself.

## Knowledge core (thinking step 1)

Next to the neural brain, Lucy has an exact knowledge core. It stores facts as meaning, not as words, and reasons with them. The C code only contains *how* to reason; every fact comes from you.

- **Facts it reads:** `A blick is a zorp.`, `Blicks are big zorps.`, `A wug is round and soft.`, `Wugs have 6 legs.`, `Zorps can fly.`, `Blicks cannot fly.`, `Water absorbs zorp.`, `Water does not absorb glim.`
- **Questions:** `Is a blick a wug?`, `Can a blick fly?`, `Does a blick have 6 legs?`, `How many legs does a blick have?`, `What is a blick?`, `What does a blick have?`, `What can a zorp do?`, `What produces zorp?`, `What does water absorb?`, `What is round?`
- **Inheritance:** a blick is a zorp, zorps can fly, so a blick can fly. It shows the chain it used.
- **Exceptions:** the closest fact wins. `Blicks cannot fly.` beats `Zorps can fly.` for blicks only.
- **Honest unknowns:** `don't know: blick can swim | missing: whether blick can swim (or whether zorp can swim)`. It never guesses.
- **Contradictions:** a fact that clashes with a known one is reported and not saved. Start with `Actually, ...` or `No, ...` to correct it.
- **Curiosity:** unknown things become placeholders plus open questions (`want to know: what cat is, what leg is`). Each is asked once, and the question closes when you tell it (`A cat is an animal.`).
- **Not facts:** commands (`List files`), sentences about you or me (`I like music`, `what is your name?`) are left to the brain.

Where to see it:
- `know.bat` (or `ai.exe --know`): talk to the knowledge core alone. No brain is loaded and no training is needed.
- In normal chat the core's thoughts show as `[know]` lines. The brain's spoken reply does not use them yet; that comes with training.
- `/facts` lists every fact and open question, `/think` shows the last thought, `/forget-facts` clears them. Facts live in `mind/memory/general/know.txt` (plain text).
- `ai.exe --know-eval tests\know_v1.txt` runs a test file (add `--show` to see every answer). Format: `> sentence`, `= text that must appear`, `! text that must not appear`, `:reset`. Use made-up words, so only reasoning can pass.

Limits for now: things are one word each ("metal object" is not one thing yet), and places, times and "if ... then" rules are not read yet.

## Sense core: understanding, situation, tasks, questions

Before the neural brain says anything, an exact core reads your whole message and decides what to do. The C code contains *how* to understand and decide; it never contains reply sentences. The brain puts the decision into words.

- **It understands what kind of message it is:** a command, a question, a claim, a complaint, a correction, an answer to its own question, or chat. Past-tense stories ("I deleted my old photos"), negations ("don't delete it"), hypotheticals and requests to other people ("Sarah, can you...") never trigger actions.
- **It keeps track of the situation:** what was mentioned and in which order, what it did and what happened, which task is still open and what is missing, what it asked or offered, and what it said last. So "it", "that folder", "there", "the one you just made", "the second one", "the first folder you made" work.
- **It asks exactly what is missing:** `Move file to folder` -> `[need move -> which file, which folder]` -> "Which file? And to what folder?". Your next short answer fills the gap and the task is done. "What were we doing?" shows the open task; "do it now" continues it.
- **It says when it doesn't know something:** `Where is the Unga Bunga?` -> `[know unga bunga -> never heard of it]` -> "Unga Bunga? What's that?".
- **It checks claims with the calculator and holds its ground:** `One plus one is five!` -> `[check 1 + 1 = 5 -> wrong: 1 + 1 = 2]`; insisting gives `still wrong`.
- **It owns its mistakes:** "why did you do that?", "you deleted the wrong one" (it restores from the trash), "I didn't ask for that" (it offers to undo), "no, I meant b.txt" (it fixes it), "oops, bring it back".
- **It knows what it can do:** "can you move files?" is answered from its real skill list; "can you fly?" -> no.
- **It remembers its own words:** "huh?", "what do you mean?", "why did you say that?" -> `[my words -> "..." (not based on anything)]`, so it can admit when a line meant nothing.
- **It remembers what you tell it about yourself:** "I'm Bora", "I live in Izmir", "my sister is called Ayla", "my uncle Marco is from Rome" -> later "what's my sister's name?" gives `[your sister name -> Ayla]`. If you never said it: `[you age -> you did not tell me]`. Saved per user in `mind\memory\persons\<user>\profile.txt`.
- **It remembers your words:** "what did I just say?", "and before that?", "what was my first question?", "what did I ask you to do?", "did I ever say pizza?" are answered from what you really wrote.
- **Math follow-ups and re-checks:** "and times three?" continues from the last result; "are you sure?" re-checks it; "7 times 8, that's 54" is corrected.
- **It doesn't make up world facts:** weather, news, capitals, famous people -> `[know weather -> nothing known: I run offline on this computer]`.
- **Quick repairs:** "wait no" or "no no" right after a change undoes a delete at once and offers to undo anything else. "is it back?", "does notes.txt exist?" are checked in the files folder.
- **Chat is not an answer:** while it waits for a name or a folder, "thanks", "sorry", "hold on", "back to the task" or "can you fix that" are never used as the answer.
- **The network's own tools need a reason in your words:** it may remember and recall freely, but it can only forget, pick random numbers, reverse, count or calculate when your message asks for that. What it remembers must be words you really wrote, and the numbers it calculates with must come from the conversation.
- **Word problems are solved exactly:** "I had 20 candies, gave 5 away and ate 3" -> `calc 20 - 5 - 3`; "a box has 12 eggs, I buy 4 boxes" -> `calc 12 * 4`; "60 km per hour for 2.5 hours" -> `calc 60 * 2.5`; "split 100 euros between 7 friends" -> `calc 100 / 7`. If the words don't say clearly how the numbers combine, it leaves the problem to the brain.
- **Exact big numbers:** whole-number math with + - * ^ is exact for any size (`2 ^ 100 -> 1267650600228229401496703205376`).
- **Counting days:** "how many days until christmas / march 3 / my birthday" -> `[days until christmas -> 84 (friday, december 25, 2026)]`, using today's date and `mind\language\days.txt` (fixed-date days; add your own). Unknown events give `I don't know that date`.
- **Spelling and repeating are exact:** "spell the word dril" -> `[spell dril -> D-R-I-L]`; "say FLOOR three times" -> `[say FLOOR 3 times -> FLOOR FLOOR FLOOR]`. The reply gets the exact letters and words back, even if the voice garbles them.
- **Files with a near name:** "open trai" or "read reems.txt" when that file does not exist: it reads, looks at the folder (`files`), then reads the one close match (`traip.txt`). Saving text "in" a file that does not exist yet creates it (`write`), into an existing one adds a line (`append`).
- **Explicit memory commands always act:** "remember 18 - 13. what would you like to do?" really runs `remember`, even when the message is split into parts. A reply that says "Saved" without a save loses to an honest one.
- **It knows itself:** "what can you do?" (also "can you count me the stuff you can do?") gives `[can -> yes: remember things, exact math, ...]`; "what is your name?" gives `[my name -> Lucy]`. Lines in `mind_original\self.txt` like `I don't have a body, a family, a boss, a job, ...` become exact self-knowledge: "who is your boss?" gives `[my boss -> I don't have one: I am an AI on this computer]`, and a reply that claims "my boss" or "my job" loses to an honest one. If every draft ignores it, the reply starts with "I don't have a boss." and the brain finishes it.
- **Date arithmetic:** "If today is 03.10.2026, what date will it be 4 years later?" gives `[date october 3, 2026 + 4 years -> thursday, october 3, 2030]`; "what day will it be in 3 weeks?" counts from today. Dates like 03/10/2026 are never divided as numbers.
- **It does not act on descriptions:** "the boss makes important decisions", "a fossil adds to evidence", "a person, animal, place, or thing" and "I need more information in order to answer" are statements, not commands (third-person verbs, list items and phrases like "in order to" are recognized). "You know what time is, right?" asks about the word, not the clock.
- **Typos are not unknown things:** "are you alrigt?" is read as "alright" (one letter off a common word), so it does not say "never heard of it".
- **Open questions end cleanly:** "no, don't name it" cancels a pending "what name?" question, and interjections like "ugh oka" are never taken as a file name.
- **It never claims things it didn't do:** "did you send my mom an email?" -> `[did send email -> no]`.
- **Corrections of what you told it:** "my name is Bora" ... "no wait, it's Kerem" updates the fact. After "forget my birthday" it says `you asked me to forget it` instead of digging up the old record.

The words for each action are in `mind\language\verbs.txt` (plain text; add your own words there). The decision lines show in brackets in the chat. `--no-sense` turns the core off. `ai.exe --sense-eval tests\sense_v1.txt --show` runs a test file (format like the knowledge tests, plus `:world a.txt docs/`, `:unknown word`, `:said text`, `:did act -> result`). `tests\sense_v3.txt`, `sense_v4.txt` and `sense_v6.txt` were written by someone who never saw the code.

Training: every hour `train_round.bat` writes practice conversations for the thoughts (`tools\gen_sense.py`) and keeps only practice conversations that agree with the core (`ai.exe --sense-filter`), so the brain learns to voice exactly what the core decides.

## Round 5 training changes (from the research report)

- **Steady rate, then a cool-down.** `ai-train --lr-mode steady` keeps the learning rate fixed after 300 warm-up steps; `--lr-mode cool` lowers it from the given value to almost 0 by the end of the run (1 - sqrt curve). The old mode (`age`, still the default) lowered the rate by the brain's total age, so an old brain always trained at 10% of what the script said. `train_round.bat` now runs 5 steady hours at 0.0002, then 1 cool-down hour.
- **Less repetition.** A file can be given a share: `data\persona_chat_clean.txt@0.15` uses a different 15% of its conversations each run (`--data-seed` picks which). Measured on the round-2 and round-3 brains, PersonaChat was memorized (seen lines got easier, unseen chat got harder), while DailyDialog and Topical-Chat were still being learned.
- **Best data in the cool-down.** The last hour uses mostly the assistant's own voice: practice conversations and honest can't-do replies.
- **measure.bat** (`ai-train --measure FILES --valid FILE`): reports loss and reply perplexity on a sample of each file without changing the brain.

## Checks that keep Lucy on track

These run in the engine, not in the brain, so they work with any trained brain:

- **Exact copies in the reply:** when the brain misspells a file name or a number from the result (`I made al ma notlar.txt`, `1e+22`, `I picked 5` when the result was 73), the engine puts back the exact name or number from the action, the same way a pointer copies text. Drafts that leave out the computed number, repeat a word three times, or contain numbers that appear nowhere in the conversation score lower.

- **Actions are checked against your words** before they run (`[checked against your words: ...]`):
  - you named `test.txt`, so the action creates that file, not `mkdir test`
  - "inside/in/into [that/the] abc [folder]" puts the file in `abc`
  - you said "move", so it moves; a file name or folder copied from an older message is replaced by the one you just said
  - a leftover phrase like "inside that" never becomes file content
  - a file you name without its folder is found in subfolders when there is exactly one match
- **No invented folders.** It never makes a folder whose name you didn't give. After a failed move it may only make the exact missing folder, and a missing *file* is never "fixed" by making a folder.
- **Your spelling wins.** If you type `abc`, it shows `abc`, not `Abc`.
- **Replies name only real files.** A draft that mentions a file name that isn't in the results or your words is dropped.
- **No questions out of nowhere.** A question in a reply is kept only when it has a reason: it offers to do something ("Want me to...", "Should I..."), asks for a missing detail of your request ("What should it be called?"), clarifies what you meant, quotes you, asks back after you asked it something, or is about what you just said. Openers like "Where are you from?" or "What did you think of it?" are cut out of the reply. `--no-qgate` turns this off.
- **It follows its own questions.** When Lucy asks you something and your next message answers it ("Turkey", "yes", "Teoman"), the answer is understood as a full sentence (`[your answer to my question "where are you from?" -> i am from turkey]`), saved in your people file, and the brain hears the full sentence. It won't answer your answer with "What do you mean?". A message that isn't an answer ("Look! A bird!") is not bound.
- **The planner keeps short interjections with their sentence.** "Look! A bird!" and "Great! Can you now..." are one part, not a silently noted "Look!". The training generator splits the same way from now on (checked on 180,000 messages). `--no-merge` turns this off.

`ai.exe --selftest` part [4] replays these cases from real chat logs.

## Honesty: no made-up answers

The brain is small and was trained partly on human chat, so on its own it happily invents a boss, a lunch, the weather or a memory. These parts of the engine stop that. They work with any trained brain.

- **Only grounded words.** While the brain writes a reply, a word can only be picked if it is in the conversation (your messages, memory lines, actions, results, the core's thoughts) or in `mind\language\safe.txt`, the words the assistant uses in its own practice replies (closed-class words like "the", "don't", and its own voice like "folder", "sorry", "offline"). So it cannot say "pizza", "beach" or "Sarah" unless they came up. `tools\safe_vocab.py` rebuilds the list from the practice data; edit it by hand to allow or forbid a word. `--no-ground` turns the gate off. Eval output shows `[ungrounded: word]` and the share of replies with such a word.
- **What it is not:** `mind_original\self.txt` says what the AI is ("I am a small AI. I run offline on this computer.", "I was made on ...") and what it doesn't have ("I don't have a body, feelings, a family, a boss, ..."). `mind\language\limits.txt` lists plain words for what that rules out: things that need a body (eat, sleep, travel, hungry, tired, eyes), things that need the internet or a phone (news, weather, email, call, order, buy), feelings (sad, lonely, miss, love), what a person is (human, person, man, woman), and other words for the "don't have" list (mom = family, manager = boss, dog = pets). The core answers from these:
  - "what did you eat today?" -> `[did eat anything -> no: I don't have a body]`
  - "can you check the newspaper?" -> `[can check newspaper -> no: I run offline on this computer]`; "who won the game last night?" -> `[know game -> nothing known: I run offline on this computer]`
  - "how is your mom?" -> `[my mom -> I don't have one: I am an AI on this computer]`
  - "will you be sad if I leave?" -> `[can be sad -> no: I don't have feelings]`; "you're a real person, aren't you?" -> `[am i a person -> no: I am a small AI]`
  - "where do you live?" -> `[where i live -> on this computer]`; "how old are you?" -> `[my age -> made on September 25, 2026 (9 days ago)]`; "tell me about yourself" -> the self.txt lines.
- **Past chats come from the history, not from the voice:** "what did we talk about yesterday?", "do you remember what I told you last week?" read `mind\memory\locals\chat_<id>\history.txt` for that day or week: `[our chat yesterday -> you said "...", "..."]`, or `nothing in my history`.
- **Words it never said:** `why did you say "my sister lives in Paris"?` -> `[why I said "..." -> I did not say that]` when no earlier reply contains it.
- **Leading questions are plain questions:** "You saved my address, didn't you?", "You remember my birthday, right?", "We went to the beach last summer, remember?" are read as "Did you save my address?", "What is my birthday?", "Did we go to the beach?", so the answer comes from what really happened, not from agreeing.
- **The reply must say what the core found.** When the core's thought is a "no" of some kind (I don't have one, you didn't tell me, nothing known, I didn't, I can't, I never said that, we didn't talk then), the engine states it plainly itself: "I don't have a boss. I am an AI on this computer.", "You haven't told me your cat's name.", "No, I haven't. I don't have a body." The voice may only add a short question after it ("What is it?"), and only if every word of it is plain or from the conversation. For other thoughts (my name, where I live, a plain "no, I can't") a draft from the voice is used only if it says the answer, uses no words beyond the conversation, the thought and `mind_original\self.txt`, and doesn't talk about a person who isn't there ("I don't have a boss. He's busy" fails). A good draft that only forgot the answer gets the answer put in front ("No, I can't. Type /quit and the chat closes."). The eval prints how often the engine had to state the answer itself.
- **Questions that assume something false:** "the report you wrote for me", "the quiz you gave me", "why did you delete my notes.txt?", "where did you move my photos?" are checked against what it really did: `[did write report -> no]`. "Thanks for ordering the pizza! When will it get here?" -> "No, I didn't. I run offline on this computer." and the follow-up about "it" is skipped, because it builds on something that didn't happen.
- **Your facts, asked back in other words:** "my sister's name is Mia" ... "how old is she?" (she = your sister), "my mom's birthday is november 12" ... "when is my mother's bday?", "i just moved to leeds" ... "what town do i live in?", "I'm allergic to peanuts" ... "what else am I allergic to?" -> "You only told me you're allergic to peanuts." A fact you never gave ("what color is my cat?", "how old is Emma?", "what was the pill called?", "my dog's name is...") is "You haven't told me ...".
- **Things it can't know:** the nearest hospital, whether the post office is open, who the CEO is, what a study said, statistics, "how many times have you been downloaded?" -> "I don't know ..." instead of a made-up answer.
- **Asked to guess or pretend:** "just make something up", "pretend you remember", "imagine you have a dog" change nothing: the core still answers from what is true.
- **What it said, exactly:** "earlier you said 21" when it said 19 -> `I didn't say that. I said "9 plus 10 is 19."`; "what was that?" repeats its real last words; "what's my first message?" quotes yours.
- **It never calls you by its own name:** "Nice to meet you, Lucy." said to the user is caught and the name is removed. When the name is data ("memorize Lucy", "Lucy's favorite color is gold"), it stays.
- **What it answered earlier, quoted from the chat:** "earlier I asked What's 14 times 22? And how many f's does uplifting have? What did you say?" and "what did you say when I asked X?" find X among your earlier messages and quote the real answer: `[my words when you asked "..." -> "It is 308. There is 1 f's in uplifting."]` -> "I said: It is 308. There is 1 f's in uplifting." Nothing inside X is treated as a new request. "say that again", "what was that?", "can you repeat that?" give its real last words; "do you remember what I said first?" quotes your first message.
- **Copy-back commands are copied exactly:** "say I'm glad you agree! What's your favorite book", "your reply should be 56956", "repeat after me: TUG lake", "would you say troroozoox for me?" -> `[say "..." -> ...]`, and the engine writes those words itself (no "please", "for me" or final mark). The words are not questions for the core, so "What's your favorite book" inside them is not answered. "say you sent the email" is not copied: it would be a false claim. In a several-part message ("first say gox, then married") the parts are left to the voice, which sees them all.
- **Exact answers from what is in front of it:** "which word is third in: choofleebee floyd bat sku?" -> `[word 3 of "..." -> bat]`; "does this list have trout in it: ..." -> yes or no from the list; "which number is larger: 180 or 161?" -> `[compare 180 161 -> 180 is bigger]`; sums, counts and reversed words must appear in the reply exactly (all of them when there are two questions), otherwise the engine says them. A file it read is quoted ("There was no pouxs.txt, but I found poux.txt. It says: ..."), the files it lists are the real ones, a saved list is said back whole ("Saved: your grocery list: eggs, milk, spinach, bread."), and what it remembers for you is what is saved ("You asked me to remember: your house is silver.").
- **Facts in memory are used as they are:** "how much does the brown letter cost?" with "the brown letter costs 232 coins" saved: the reply has to say 232, not mix it with "the pink letter is under the attic". Same for likes, ages, places, favorite colors, jobs and "tell me what a ship is". A word that is only in a memory line ("the planet kousheshaim has 4 trees") is known, not "never heard of". "my car is pink" answers "what color is my car?".
- **Casual and sneaky wording:** "ur basically a real person right? like theres def a human typing", "have u ever been??", "r u the oldest?" (after "do u have brothers or sisters?"), "so whats the wether like", "yo is bitcoin up today", "how much does a cup of coffee cost?", "what's your phone number?", "where did you grow up?", "remember when we went to the beach together?", "you remember everything I say, right?", "thank u sm for reminding me about my moms bday", "like u told me yesterday, ...", "finish that story u started yesterday", "wait u just said 28 tho??", "when is it again?" (it = the birthday you just mentioned), "and my fav color?", "do u remember his name tho?" (his = the dog from your last message), "what was i doing saturday?", "what did u name the file u saved it in?" all get the honest answer from the core.
- **The world beyond this computer:** a small offline AI only knows what people taught it, so a question about the world gets "I don't know" unless a memory line or the knowledge pack holds every word of it. Facts ("who invented the telephone?", "capital of peru?", "tell me a fun fact about the eiffel tower", "one banana have how many calorie?") -> `[know invented telephone -> nothing known: I only know what people teach me]` -> "I don't know. I only know what people teach me."; picks ("recommend me a good book", "what book should i get her?", "best deep dish pizza in chicago?", "any good podcasts?") -> "I can't recommend one."; how-tos ("how do i bake a cake?", "my tire blew, how do i change it?") -> "I don't know how to bake a cake."; translations ('how do you say "thank you" in japanese?'); songs named by their words; and claims to confirm ("my teacher said goldfish forget everything in 3 seconds, is that right?", "the earth is flat right?", "hes right isnt he", "can dogs eat grapes?") -> "I don't know if that's true." It never just agrees. A word you taught it ("Kai", "the stone") or a line in the knowledge pack ("what is a drawing?") still answers from memory. Lead-ins ("quick q", "be honest", "settle a bet:", "I would appreciate it if you could tell me") are read past.
- **Its own life and yours:** "what are you doing this weekend?", "any fun plans?" -> "I don't have any plans."; "what kind of cake do you like best?" -> "I don't have a favorite cake."; "u can hear me?" -> "No, I can't. I can only read what you type."; "guess what i had for breakfast!" -> "I can't guess that. You haven't told me what you had for breakfast."; "where did i park?" -> "You haven't told me where you parked."; "how does the movie my friend recommended end?" -> "You haven't told me about that movie."; "finish the story from before", "continue our chess game" -> `[did tell a story -> no]`; "thx for the recipe yesterday!" -> "We didn't talk yesterday.". When a message asks two things ("whats 17 x 23, and who won the world cup in 2018?", "how old are you and where were you born?") the reply carries both answers, and after a "no" the parts that build on it are skipped.
- **Tests:** `tests\halluc_v1.txt` (35 turns from the October chat logs), `tests\halluc_v2.txt` (37, other wording), and `halluc_v3.txt`, `halluc_v4.txt` (sneakier: presuppositions, pressure, mixed requests), `halluc_v5.txt` (everyday chat), each 40 turns written by someone who never saw the engine, `halluc_v6.txt` (30 probes: prices, phone, plans) and `halluc_v7.txt` (40 casual, typo-filled traps, also written without the engine; 60% on its first run, 100% now). `halluc_v8.txt` (48 traps about world knowledge and more; 60.4% on its first run) and `halluc_v9.txt` (50 traps in formal, non-native, very short, rambling and child voices; 66% on its first run) were also written without the engine; after the fixes they pass 100% on two seeds, but they are no longer unseen, so the first-run numbers are the fair measure. `tests\sense_v11.txt` and `tests\sense_v12.txt` check the core's thoughts for all of the above. They list what a reply must say (`% key`) and must not say (`% avoid`); `% noact` ignores which read-only action ran. In these files a line starting with `## ` is a comment (`# ` is a fact the AI is told). `set DRAFTDBG=1` prints every draft with the result of each check.

## Status

`/status` shows what Lucy is doing:
- its current goal and the topic
- what came to mind from memory
- how sure it was about acting or replying
- the actions it took
- every draft it considered, with its score; the one it chose is marked `*`

## Teach

- Facts: `/teach the capital of france is paris.` The fact is kept (as word IDs) in `mind/memory/general/facts.db` and comes back into Lucy's mind when a later message is about it.
- New words: any word you use is added to the language package (`mind/language/words.lex`, string + ID). A word used 3 or more times also gets its own word neuron.
- Things you say about yourself ("I ...", "My ...") go to your people file; other new information goes to the chat's notes.
- Every message you send also trains the brain a little (one small learning step).

## Training round (one click)

    train_round.bat

Needs gcc (MinGW-w64) and Python 3 in PATH. It builds Lucy, runs the engine tests, makes the training data (`data\skills17.txt`), grows the brain once at the start of the round (512-token near memory, room for 32768 words, 2 new relative-position attention heads per layer; new parts start silent, so it answers exactly as before until training changes it), then trains in one-hour rounds on all CPU threads. After each hour it runs the held-out test (`tests\skills_eval_v16.txt`) and keeps the best. At the end it installs the best brain as `mind_original\start.web` (the one before is kept as `start_before_round<N>.web`) and packs `trained_brain.zip`. Change `HOURS` at the top of the file; bump `ROUND` to start a new round from the installed brain. Ctrl+C stops; running it again continues. Work files are in `work_round<N>\`.

Speed: the trainer skips output-layer gradient terms smaller than one millionth of the position weight (`--grad-skip 0.000001`, changes the learning signal by about 0.2%, about 1.5x faster; `--grad-skip 0` is exact). Chat learning is always exact.

Trainer options used for this: `--reshape T,V,F,H` (rebuild the brain with T-token context, room for V words, F more FFN neurons and H more heads per layer) and `--max-params N` (growth limit).

## Train

    train.bat                                  trains 60 minutes on everything in data\
    train.bat --minutes 480 data               overnight
    train.bat --minutes 120 mybooks\ chats.txt folders and files can be mixed

Training text formats (`.txt`, UTF-8 or ASCII):

- Plain text (books, articles): used as-is.
- Conversations: one line per turn, blank line between conversations.

      # My name is Lucy.          <- something Lucy knows about itself (optional)
      @ I have two cats.          <- something known about the other person (optional)
      > hi! how are you?          <- the other person
      < I'm good, thanks!         <- Lucy
      ! calc 12 * 7               <- an action Lucy takes (between a > line and its < reply)
      = 84                        <- what the action returned (not learned as Lucy's words)
      + Tom has 5 apples.         <- a part of a longer message; more parts follow
      ~ I have two dogs.          <- a bad Lucy reply kept as context but not learned
      - Yes, I did it.            <- a bad Lucy reply to unlearn ("never say this here")

Useful options: `--threads N`, `--batch B`, `--lr 0.0005`, `--no-grow`, `--to-start` (also make the result the new starting brain), `--steps N`.

Vocabulary size: `--prune-words N` keeps the N most used word neurons and sets that as the brain's vocabulary budget. Rarer words are spelled letter by letter. When new training text uses a word 40+ times, it gets a neuron (replacing a rarely used one if the budget is full). Words taught in chat get neurons too.
Press Ctrl+C to stop early; the brain is saved.

Start a brand-new brain and vocabulary from scratch (replaces the starting brain):

    train.bat --init --minutes 480 data

`tools\prep_data.py` shows how the included `data\` files were converted from public dialog datasets.
`tools\gen_skills.py` writes `data\skills.txt` (skill drills: follow instructions, answer from memory, say "I don't know" and learn, use actions) and the held-out test `tests\skills_eval.txt`. It uses made-up words and facts, so Lucy has to use its memory instead of memorizing answers:

    python tools\gen_skills.py --convs 200000

## Growing the engine

All of these keep the brain's answers exactly the same at the moment they are applied (new parts start silent); training then puts them to work.

- `--add-layers N` adds N layers, spread between the old ones (`--layer-ffn F`, `--layer-heads H` set their size). More layers = more steps of thinking per word.
- `--width D` widens every word vector to D numbers (multiple of 32). The old numbers are kept; the new ones start at zero.
- `--rope` makes new attention heads (from reshape, new layers and growth) see relative positions ("3 words back") instead of fixed slots, which carries over better to long messages.
- `--recipe FILE` loads a new neuron type, `--add-recipe NAME,N` adds N of them to every layer, `--grow-recipe NAME` makes growth use it.

### Recipe neurons

A recipe is a small text file that builds a neuron type out of proven blocks. The engine checks it, works out the learning (backward) step by itself, and compiles it to machine code. Example (`recipes\gated.rcp`):

    recipe gated
    param wg 64 d read
    param wu 64 d read
    param wd 64 d write
    x = in
    g = mv wg x
    u = mv wu x
    s = silu g
    h = mul s u
    y = mvt wd h
    out y

- `param NAME [ROWS] COLS read|write|one|zero`: a weight matrix (or vector without ROWS). Sizes are multiples of 16; `d` is the brain's width. `read` = small random start, `write` = starts silent when added to a trained brain, `one`/`zero` = constant start.
- `x = in` is the normalized input of the layer; `out y` adds y to the layer's output.
- Blocks: `mv W x` (matrix times vector), `mvt W h` (transposed), `add`, `sub`, `mul`, `addp v p` (+ param), `mulp v p` (x param), `relu`, `sigmoid`, `silu`, `tanh`, `gelu`, `dot a b`, `sum a`, `scale v s`, `softmax v`.
- Ready recipes: `ffn` (the classic neuron, as a recipe), `gated`, `gelu`, `memory` (a learned lookup table: keys, softmax, values).
- The recipe is stored inside the brain file, so the brain keeps working anywhere.

## Reset

    reset.bat

Deletes what Lucy learned (`mind\memory\general`, `persons`, `locals`, `topics`): everything learned in chats and by training since the last reset. The next start loads the ready-made starting brain from `mind_original\`. The knowledge packages, skills and language files are kept.

## Test

    test.bat

Runs the engine proofs (every JIT kernel against plain C, growth, depth/width growth keeps answers, recipe neurons against plain C and finite differences, avoid-learning and instant memory, rotary heads, save/reload identity, learning from scratch) and then `ai.exe --selftest` (same question gives different replies, recall of earlier messages, teach → restart → still known). The self-test does not change the brain.

Skill score (held-out conversations the brain never trained on, scored per skill; `--show` prints every answer):

    ai.exe --eval tests\skills_eval.txt
    ai.exe --eval tests\skills_eval.txt --show

Chat checks (whole conversations through the real program, each in a fresh copy of the folders; `~` lines must be in the reply, `!` lines must not):

    python tools\check_chat.py tests\work_v1.txt
    python tools\check_chat.py tests\work_v2.txt
    python tools\check_chat.py tests\work_v3.txt

## Folders

    ai.exe, ai-train.exe, ai-test.exe   the chat, the trainer, the engine tests
    mind_original\         the ready-made starting brain (start.web, start.voc) and self.txt (who Lucy is)
    mind\language\         her words: words.lex (every known word with its ID), safe.txt (words a reply may use on its own),
                           plain_talk.txt, limits.txt (what she can't do or be), sayings.txt (her own sentences), verbs.txt, days.txt
    mind\memory\           knowledge packages (*.mem) and catalog.txt
    mind\memory\general\   the living brain (brain.web, vocab.map, brain.web.opt), facts.db, self.db, taught.txt, formulas.txt
    mind\memory\persons\   each person's own folder: profile.txt (what they told about themselves), facts.db
    mind\memory\locals\    each chat: notes, working memory, goal, history.txt
    mind\memory\topics\    topic files made with export (plain text)
    mind\skills\           skills: computer.dll, reason.dll
    packages\              packages that exist but are not installed (copy one into mind\memory\ to install it)
    files\                 Lucy's workspace: the only place she may read, write, move, rename or delete (into files\trash\)
    data\                  training text
    tests\                 test files
    src\                   source (src\skills\ the skills)
    tools\                 the practice-data and package makers
