#ifndef WORK_H
#define WORK_H

/* Lucy's work: what she was asked to do, how each task went, and the
 * problem solver that keeps at a task until it is done.
 *
 * A skill says, for each of its actions, what the action makes true and
 * what to do when it fails (its know-how, in plain lines):
 *
 *     trash: makes exists {1} = no
 *     trash: if trash has a file with that name: the trash already has
 *         a {name 1}; try move {1}<tab>trash/{free trash/{name 1}} = put
 *         it in the trash as "{free}"
 *
 * With that, the solver works the same way for every skill:
 *
 *   1. it checks whether the task is done (and trusts the action when there
 *      is no way to check),
 *   2. when it isn't, the cause is what the last step answered,
 *   3. it picks the first fix for that cause it hasn't tried (a fix that
 *      could lose something waits for the user's yes) and tries it,
 *   4. and goes back to 1, until the task is done or no fix is left; then
 *      it says what it tried.
 *
 * When the user says a task isn't done while the check says it is, that is
 * a problem too, with its own cause ("user disagrees"): the skill's fixes
 * for it look at the place ("look list trash"), find another thing with the
 * same name, or ask, one more each time the user says so, before Lucy asks
 * whether they look at the same place.
 *
 * She never offers the same thing twice: a fix the user said no to stays
 * out (unless they say yes to it after all), and when the skill's fixes
 * are used up and the user says "try something else", she goes on with
 * what works for any skill, one step further each time (WORK_PUSH): the
 * action once more, a wider look for the thing, asking what she lacks, and
 * at last saying plainly that she knows no other way.
 *
 * A task can also wait for its time ("in 5 seconds"), and the task worked
 * on last is what "it", "that folder" and "the one we talked about" mean.
 * How Lucy reads "it is still there" or "try something else" is in
 * mind/language/meanings.txt, so she can be taught more ways to say it. */

#define WORK_TASK_LIMIT 16
#define WORK_STEP_LIMIT 16
#define WORK_RULE_LIMIT 400
#define WORK_TRIED_LIMIT 12
#define WORK_MEANING_LIMIT 900
#define WORK_DECLINED_LIMIT 8
#define WORK_ATTEMPT_LIMIT 10

enum
{
	WORK_NONE = 0,
	WORK_WAITING,
	WORK_DOING,
	WORK_DONE,
	WORK_STUCK,
	WORK_ASKING,
	WORK_DROPPED
};

enum
{
	WORK_STEP_MAIN = 1,
	WORK_STEP_FIX,
	WORK_STEP_RETRY,
	WORK_STEP_FOUND,
	WORK_STEP_LOOK
};

/* what the user's message about a task was, for how the story is told */
enum
{
	WORK_TELL_DONE = 0,
	WORK_TELL_COMPLAINT,
	WORK_TELL_QUESTION,
	WORK_TELL_AGAIN
};

enum
{
	WORK_RULE_MAKES = 1,
	WORK_RULE_MEANS,
	WORK_RULE_IF,
	WORK_RULE_UNDO,
	WORK_RULE_KIND
};

typedef struct
{
	int		KIND;
	char	ACTION[48];
	char	ARGUMENT[600];
	char	RESULT[300];
	int		WORKED;
	char	SAID[300];
	int		TURN;
} WORK_STEP;

typedef struct
{
	int			ID;
	int			STATE;
	char		ASKED[600];
	char		ACTION[48];
	char		ARGUMENT[600];
	char		THING[300];
	char		THING_KIND[24];
	char		CAUSE[300];
	char		CAUSE_SAID[300];
	char		ASKING_STEP[700];
	char		ASKING_SAID[300];
	int			ASKING_KIND;
	int			ASKING_DOUBT;
	int			ASKING_WIDER;
	char		DECLINED[WORK_DECLINED_LIMIT][700];
	int			DECLINED_COUNT;
	char		LAST_DECLINED_STEP[700];
	char		LAST_DECLINED_SAID[300];
	int			LAST_DECLINED_KIND;
	int			LAST_DECLINED_TURN;
	char		ATTEMPTS[WORK_ATTEMPT_LIMIT][200];
	int			ATTEMPT_COUNT;
	int			LADDER;
	int			LADDER_TURN;
	char		LADDER_SAID[900];
	int			DOUBT_ROUNDS;
	int			AGAIN_ASKS;
	char		TRIED[WORK_TRIED_LIMIT][300];
	int			TRIED_COUNT;
	WORK_STEP	STEPS[WORK_STEP_LIMIT];
	int			STEP_COUNT;
	int			CHECKED;
	char		SEEN[300];
	char		FREE_NAME[300];
	double		DUE;
	char		LATER[700];
	char		LATER_SAID[300];
	int			TURN;
	int			TOUCHED;
	int			PUSHBACKS;
	double		MADE_AT;
	double		DONE_AT;
} WORK_TASK;

typedef struct
{
	char	ACTION[48];
	int		KIND;
	char	RESULT[160];
	char	TEXT[600];
	char	SOURCE[64];
} WORK_RULE;

typedef struct
{
	void	*ENGINE;

	/* does ACTION with ARGUMENT (a skill action, two parts split by a tab):
	 * 0 when it worked, what came of it in RESULT */
	int	(*DO)(
		void *ENGINE, const char *ACTION, const char *ARGUMENT, char *RESULT,
		int RESULT_SIZE
	);

	/* Lucy's sentence for KEY, {1} {2} {3} filled in; DEFAULT when her
	 * sayings don't have it */
	void	(*SAY)(
		void *ENGINE, const char *KEY, const char *DEFAULT, const char *FIRST,
		const char *SECOND, const char *THIRD, char *OUTPUT, int OUTPUT_SIZE
	);

	/* how the skill says what came of ACTION ARGUMENT; 0 when it has no
	 * sentence for it */
	int	(*SAY_RESULT)(
		void *ENGINE, const char *ACTION, const char *ARGUMENT,
		const char *RESULT, char *OUTPUT, int OUTPUT_SIZE
	);

	/* a thing called like NAME somewhere else in the files: 1 when one was
	 * found (its path in OUTPUT) */
	int	(*FIND)(void *ENGINE, const char *NAME, char *OUTPUT, int OUTPUT_SIZE);

	/* things with a name like NAME (other letter case, other ending, a
	 * name with NAME in it, a letter off), one path per line; how many.
	 * May be NULL */
	int	(*FIND_LIKE)(
		void *ENGINE, const char *NAME, char *OUTPUT, int OUTPUT_SIZE
	);

	/* shows one line of the solver's thinking ("check: exists 42 -> folder:
	 * not done yet"); may be NULL */
	void	(*SHOW)(void *ENGINE, const char *LINE);
} WORK_HANDS;

typedef struct
{
	WORK_RULE	RULES[WORK_RULE_LIMIT];
	int			RULE_COUNT;
	WORK_TASK	TASKS[WORK_TASK_LIMIT];
	int			TASK_COUNT;
	int			NEXT_ID;
	int			TURN;
	int			FOCUS;
	int			LAST_ASKED_TURN;
	char		MEANINGS[WORK_MEANING_LIMIT][2][64];
	int			MEANING_COUNT;
	char		MEANINGS_PATH[600];
	long long	MEANINGS_STAMP;
	char		VERB_WORDS[160][2][40];
	int			VERB_WORD_COUNT;
} WORK_STATE;

void		WORK_INITIALIZE(WORK_STATE *STATE);
void		WORK_FORGET_RULES(WORK_STATE *STATE, const char *SOURCE);
int			WORK_LEARN(WORK_STATE *STATE, const char *TEXT, const char *SOURCE);
int			WORK_LOAD_MEANINGS(WORK_STATE *STATE, const char *PATH);
int			WORK_REFRESH_MEANINGS(WORK_STATE *STATE);
int			WORK_LOAD_VERBS(WORK_STATE *STATE, const char *PATH);
int			WORK_MEANS(
	const WORK_STATE *STATE, const char *MEANING, const char *PLAIN
);
int			WORK_MEANS_AT_START(
	const WORK_STATE *STATE, const char *MEANING, const char *PLAIN
);
const char	*WORK_VERB_CLASS(const WORK_STATE *STATE, const char *PLAIN);
int			WORK_HAS_CLASS_WORD(
	const WORK_STATE *STATE, const char *PLAIN, const char *CLASS
);
int			WORK_VERB_OCCURRENCES(
	const WORK_STATE *STATE, const char *PLAIN, const char *const *ALLOWED,
	const char **LAST_CLASS
);
const char	*WORK_VERB_CLASS_AMONG(
	const WORK_STATE *STATE, const char *PLAIN, const char *const *ALLOWED
);
int			WORK_NEW_TASK(
	WORK_STATE *STATE, const char *ASKED, const char *ACTION,
	const char *ARGUMENT, const char *THING
);
void		WORK_ADD_STEP(
	WORK_STATE *STATE, int TASK_INDEX, int KIND, const char *ACTION,
	const char *ARGUMENT, const char *RESULT, int WORKED
);
int			WORK_CHECK(
	WORK_STATE *STATE, int TASK_INDEX, const WORK_HANDS *HANDS
);
int			WORK_SOLVE(
	WORK_STATE *STATE, int TASK_INDEX, const WORK_HANDS *HANDS
);
int			WORK_ANSWER_ASKING(
	WORK_STATE *STATE, int TASK_INDEX, int SAYS_YES, const WORK_HANDS *HANDS
);
int			WORK_PUSH(
	WORK_STATE *STATE, int TASK_INDEX, const WORK_HANDS *HANDS
);
int			WORK_YES_AFTER_ALL(
	WORK_STATE *STATE, int TASK_INDEX, const WORK_HANDS *HANDS
);
int			WORK_DOUBT(
	WORK_STATE *STATE, int TASK_INDEX, const char *PLAIN,
	const WORK_HANDS *HANDS
);
void		WORK_DOUBT_STORY(
	WORK_STATE *STATE, int TASK_INDEX, int FROM_STEP, const WORK_HANDS *HANDS,
	char *OUTPUT, int OUTPUT_SIZE
);
void		WORK_STORY(
	WORK_STATE *STATE, int TASK_INDEX, int FROM_STEP, int MODE,
	const WORK_HANDS *HANDS, char *OUTPUT, int OUTPUT_SIZE
);
void		WORK_DOING_SAID(
	const WORK_STATE *STATE, const char *ACTION, const char *ARGUMENT,
	char *OUTPUT, int OUTPUT_SIZE
);
int			WORK_HAS_CHECK(const WORK_STATE *STATE, const char *ACTION);
int			WORK_GOAL_IS_GONE(const WORK_STATE *STATE, const char *ACTION);
int			WORK_FOCUS_TASK(const WORK_STATE *STATE, int MAX_AGE);
int			WORK_FIND_TASK(const WORK_STATE *STATE, const char *THING);
const char	*WORK_STATE_NAME(int STATE_VALUE);
void		WORK_LAST_NAME(const char *PATH, char *OUTPUT, int OUTPUT_SIZE);

#endif
