#ifndef SKILL_H
#define SKILL_H

/* The contract between Lucy and a skill.  A skill is a library in
 * mind/skills (name.dll on Windows, name.so elsewhere) that exports one
 * function, __memory__().  It returns what the skill knows: the actions it
 * can do, the words people use to ask for them, how to say what came of
 * them, and (if it has one) a reader that answers a whole message by itself.
 * Lucy loads every skill in the folder when she starts, and any new or
 * changed one before each message, so a skill dropped in works without a
 * restart and without training. */

#define SKILL_VERSION 4

#ifdef _WIN32
#define SKILL_EXPORT __declspec(dllexport)
#else
#define SKILL_EXPORT __attribute__((visibility("default")))
#endif

/* what Lucy gives a skill: where it may work, who is talking, a way to tell
 * her about changes she should be able to undo, and (from version 2) what
 * her memory packages know and what was said a moment ago.  A skill checks
 * VERSION before it uses the later parts. */
typedef struct SKILL_HOST
{
	int			VERSION;
	const char	*FILES_FOLDER;
	const char	*MIND_FOLDER;
	const char	*USER_NAME;
	const char	*ASSISTANT_NAME;
	void		*ENGINE;
	void		(*LOG_CHANGE)(
		void *ENGINE, const char *VERB, const char *FROM, const char *TO
	);

	/* version 2: "cat", "legs" -> "4"; 1 when known */
	int	(*KNOW)(
		void *ENGINE, const char *THING, const char *PROPERTY, char *VALUE,
		int VALUE_SIZE
	);
	/* "kind", "color" -> "red, blue, green"; how many */
	int	(*THINGS_WITH)(
		void *ENGINE, const char *PROPERTY, const char *VALUE, char *OUTPUT,
		int OUTPUT_SIZE
	);
	/* the package that would know WORD ("places.mem"): 1 when it is not in
	 * the memory folder yet, 2 when it is, 0 when none would */
	int	(*PACKAGE_FOR)(
		void *ENGINE, const char *WORD, char *OUTPUT, int OUTPUT_SIZE
	);
	/* the user's last few messages before this one, oldest first, one per
	 * line */
	int	(*RECENT)(void *ENGINE, char *OUTPUT, int OUTPUT_SIZE);
	/* 1 when someone taught Lucy something about WORD */
	int	(*HEARD_OF)(void *ENGINE, const char *WORD);

	/* version 3: how Lucy says KEY, from mind/language/sayings.txt, with
	 * {1} and {2} filled in; DEFAULT when the file doesn't have it (then 0
	 * is returned) */
	int	(*SAY)(
		void *ENGINE, const char *KEY, const char *DEFAULT, const char *FIRST,
		const char *SECOND, char *OUTPUT, int OUTPUT_SIZE
	);
} SKILL_HOST;

/* one thing a skill can do.  NAME is the action as Lucy's planner writes it
 * ("mkdir"), USAGE how it is called ("mkdir PATH"), EXAMPLES the ways
 * people ask for it, one per line, with {1} where the argument goes ("make a
 * folder called {1}"), SAYS how to say each result, one per line as
 * "result: sentence" with {1} for the argument and {R} for the result
 * ("folder made: Done, I made the folder {1}.").  RUN does it: ARGUMENT in,
 * RESULT out, 0 when it worked. */
typedef struct
{
	const char	*NAME;
	const char	*USAGE;
	const char	*ABOUT;
	const char	*EXAMPLES;
	const char	*SAYS;
	int			(*RUN)(
		const SKILL_HOST *HOST, const char *ARGUMENT, char *RESULT,
		int RESULT_SIZE
	);
} SKILL_ACTION;

/* everything a skill knows.  ANSWER, when there is one, reads a whole
 * message and returns 1 with the action it took ("sequence 2 4 6 8") and
 * the exact reply ("The next number is 10.") when the message is one for
 * this skill.  The reply may end with SKILL_WHY_MARK and how the answer was
 * found ("8 + 2 = 10, and each number is 2 more than the one before"):
 * that part is not said, but Lucy uses it when she is asked "why?" or "are
 * you sure?" right after. */
#define SKILL_WHY_MARK '\x1e'

typedef struct
{
	int					VERSION;
	const char			*NAME;
	const char			*ABOUT;
	int					ACTION_COUNT;
	const SKILL_ACTION	*ACTIONS;
	int					(*ANSWER)(
		const SKILL_HOST *HOST, const char *MESSAGE, char *ACTION,
		int ACTION_SIZE, char *REPLY, int REPLY_SIZE
	);

	/* version 4: the skill's know-how, one line each, for Lucy's problem
	 * solver: what an action makes true and how she checks it, how doing it
	 * is said, and what to try when it fails, safest first:
	 *
	 *     trash: makes exists {1} = no
	 *     trash: means put {1} in the trash
	 *     trash: if no such file: there is no {name 1}; find
	 *     trash: if trash has a file with that name: the trash already has a
	 *         {name 1}; try move {1}<tab>{free trash/{name 1}} = put it in
	 *         the trash as "{free}"; ask erase trash/{name 1} = delete the
	 *         old one for good
	 *     erase: kind risky
	 *
	 * "try" fixes are done by themselves, "ask" ones (and any action of kind
	 * risky) wait for the user's yes, "find" looks for the thing elsewhere
	 * and "done" means the goal is there already.  {1} {2} are the
	 * arguments, {name 1} the last part of a path, {free PATH} the first
	 * free name like PATH ("42 (2)"), <tab> splits two arguments.  Lines in
	 * mind/skills/NAME.txt are read the same way, before these. */
	const char	*KNOW_HOW;
} SKILL_MEMORY;

typedef const SKILL_MEMORY *(*SKILL_ENTRY)(void);

#endif
