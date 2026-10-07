#include "WORK.h"
#include <ctype.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <time.h>

/* how Lucy reads talk about her work when mind/language/meanings.txt is not
 * there: "meaning: phrase, phrase, ..." */
static const char	*DEFAULT_MEANINGS =
	"still there: still there, still here, still exists, still exist, still "
	"see, can still see, i still see, it is there, it's there, its there, "
	"is still there, it is still, it's still, not deleted, not removed, not "
	"gone, isn't gone, is not gone, not in the trash, isn't in the trash, "
	"wasn't deleted, wasn't removed, didn't delete, didn't remove, did not "
	"delete, did not remove, didn't get removed, still in, i see it, i can "
	"see it\n"
	"not there: not there, isn't there, is not there, can't see it, cannot "
	"see it, don't see it, do not see it, doesn't exist, does not exist, "
	"wasn't made, wasn't created, didn't make, didn't create, did not make, "
	"did not create, not made, not created, there is no, there's no\n"
	"failed: didn't work, did not work, doesn't work, does not work, not "
	"working, isn't working, failed, failing, you failed, you are failing, "
	"you're failing, it is not, it's not, it isn't, nothing happened, "
	"nothing changed, you lied, you are lying, you're lying, not done, it "
	"did not, it didn't, you didn't, you did not, still not, not true, "
	"that's not true, wrong\n"
	"try again: try again, try something else, try another way, try a "
	"different way, try harder, try it again, find a way, find another way, "
	"find some way, fix it, fix this, fix that, do it, do it again, do it "
	"now, do something, make it work, figure it out, figure something out, "
	"solve it, solve this, sort it out, handle it, deal with it, another "
	"way, other way, something else, keep trying\n"
	"is it done: is it done, did it work, did that work, did you do it, "
	"is it gone, is it still there, is it there, is it deleted, is it "
	"removed, is it in the trash, did you delete it, did you remove it, did "
	"you make it, did you create it, is it made, is it still here, does it "
	"exist, is it still\n"
	"yes: yes, yeah, yep, yup, sure, okay, ok, go ahead, do it, please do, "
	"please, fine, alright, all right, of course, why not, yes please, go "
	"for it, sounds good, that's fine, that is fine\n"
	"no: no, nope, don't, do not, no thanks, never mind, nevermind, leave "
	"it, cancel, don't do it, do not do it, not that\n"
	"not now: not instantly, not now, not right away, not immediately, "
	"not yet, don't do it now, do not do it now\n"
	"what did you do: what did you do, what have you done, what did you "
	"just do, what you did, what did you change\n"
	"what are you doing: what are you doing, what are you planning, what's "
	"your plan, what is your plan, what will you do, what are you up to, "
	"what are you working on, what are you going to do, what is next, "
	"what's next, are you doing something, what do you plan\n"
	"what were we talking about: what were we talking about, what we were "
	"talking about, what was our topic, what were we doing, what we were "
	"doing, where were we, what were we saying, what we were saying, what "
	"was i saying, what were we just talking about, what was the topic, "
	"what did we talk about, what did we just talk about, asking the past\n"
	"done wrong: you did it wrong, that's wrong, that is wrong, not like "
	"that, you were supposed to, you supposed to, you're supposed to, you "
	"are supposed to, i told you to, i said to, i asked you to, i wanted "
	"you to, should have, instead of\n";

/* ---------- small text helpers ---------- */

static void
	TRIM(char *TEXT)
{
	size_t	LENGTH = strlen(TEXT);
	size_t	START = 0;

	while (LENGTH && strchr(" \t\r\n", TEXT[LENGTH - 1]))
		TEXT[--LENGTH] = 0;

	while (TEXT[START] && strchr(" \t", TEXT[START]))
		START++;

	if (START)
		memmove(TEXT, TEXT + START, LENGTH - START + 1);
}

static void
	LOWER(char *TEXT)
{
	for (; *TEXT; TEXT++)
		*TEXT = (char)tolower((uint8_t)*TEXT);
}

/* "It's still THERE!" -> " it's still there " */
static void
	PLAIN_WORDS(const char *TEXT, char *OUTPUT, int OUTPUT_SIZE)
{
	int	POSITION = 0;

	if (OUTPUT_SIZE < 3)
		return ;

	OUTPUT[POSITION++] = ' ';

	for (; *TEXT && POSITION < OUTPUT_SIZE - 2; TEXT++)
	{
		if (isalnum((uint8_t)*TEXT) || *TEXT == '\'')
			OUTPUT[POSITION++] = (char)tolower((uint8_t)*TEXT);
		else if (OUTPUT[POSITION - 1] != ' ')
			OUTPUT[POSITION++] = ' ';
	}

	if (OUTPUT[POSITION - 1] != ' ')
		OUTPUT[POSITION++] = ' ';

	OUTPUT[POSITION] = 0;
}

static long long
	FILE_STAMP(const char *PATH)
{
	struct stat	INFORMATION;

	if (stat(PATH, &INFORMATION))
		return (-1);

	return (
		(long long)INFORMATION.st_mtime *(long long)1000003 +
			(long long)INFORMATION.st_size
	);
}

static double
	NOW_SECONDS(void)
{
	return ((double)time(NULL));
}

/* one line of the solver's thinking, shown when the engine wants it */
static void
	SHOW_LINE(const WORK_HANDS *HANDS, const char *FORMAT, ...)
{
	char	LINE[900];
	char	*CURSOR;
	va_list	ARGUMENTS;

	if (!HANDS || !HANDS->SHOW)
		return ;

	va_start(ARGUMENTS, FORMAT);
	vsnprintf(LINE, sizeof LINE, FORMAT, ARGUMENTS);
	va_end(ARGUMENTS);

	/* "a\tb" is shown "a -> b" */
	for (CURSOR = LINE; *CURSOR; CURSOR++)
		if (*CURSOR == '\t')
		{
			char	REST[900];

			snprintf(REST, sizeof REST, "%s", CURSOR + 1);
			snprintf(
				CURSOR, sizeof LINE - (size_t)(CURSOR - LINE), " -> %s", REST
			);
			CURSOR += 3;
		}

	HANDS->SHOW(HANDS->ENGINE, LINE);
}

/* "docs/notes.txt" -> "notes.txt" */
void
	WORK_LAST_NAME(const char *PATH, char *OUTPUT, int OUTPUT_SIZE)
{
	const char	*SLASH = strrchr(PATH, '/');

	snprintf(OUTPUT, OUTPUT_SIZE, "%s", SLASH ? SLASH + 1 : PATH);
}

/* "docs/notes.txt" -> "docs", "notes.txt" -> "" */
static void
	FOLDER_PART(const char *PATH, char *OUTPUT, int OUTPUT_SIZE)
{
	const char	*SLASH = strrchr(PATH, '/');

	if (!SLASH)
	{
		if (OUTPUT_SIZE)
			OUTPUT[0] = 0;

		return ;
	}

	snprintf(OUTPUT, OUTPUT_SIZE, "%.*s", (int)(SLASH - PATH), PATH);
}

/* the first or second part of "a.txt\tdocs" */
static void
	ARGUMENT_PART(
		const char *ARGUMENT, int WHICH, char *OUTPUT, int OUTPUT_SIZE
	)
{
	const char	*TAB = strchr(ARGUMENT, '\t');

	if (WHICH == 1)
		snprintf(
			OUTPUT, OUTPUT_SIZE, "%.*s",
			TAB ? (int)(TAB - ARGUMENT) : (int)strlen(ARGUMENT), ARGUMENT
		);
	else
		snprintf(OUTPUT, OUTPUT_SIZE, "%s", TAB ? TAB + 1 : "");
}

static void
	CAPITALIZE(char *TEXT)
{
	if (TEXT[0])
		TEXT[0] = (char)toupper((uint8_t)TEXT[0]);
}

static void
	LOWER_FIRST(char *TEXT)
{
	/* "I" and names stay as they are: only a plain first word is lowered */
	if (
		TEXT[0] &&
		isupper((uint8_t)TEXT[0]) &&
		TEXT[1] &&
		islower((uint8_t)TEXT[1])
	)
		TEXT[0] = (char)tolower((uint8_t)TEXT[0]);
}

/* the end of a sentence: no full stop twice */
static void
	DROP_END(char *TEXT)
{
	size_t	LENGTH = strlen(TEXT);

	while (LENGTH && strchr(" .!", TEXT[LENGTH - 1]))
		TEXT[--LENGTH] = 0;
}

/* "put it in the trash" -> "put it in the trash", "delete the old one" ->
 * "deleted the old one": what was done, said after it was done */
static void
	PAST_TENSE(char *TEXT, int TEXT_SIZE)
{
	static const char	*IRREGULAR[16][2] = {
		{ "put", "put" }, { "make", "made" }, { "write", "wrote" },
		{ "take", "took" }, { "give", "gave" }, { "get", "got" },
		{ "throw", "threw" }, { "find", "found" }, { "keep", "kept" },
		{ "leave", "left" }, { "send", "sent" }, { "set", "set" },
		{ "read", "read" }, { "bring", "brought" }, { "empty", "emptied" },
		{ NULL, NULL }
	};
	char				FIRST[48];
	char				REST[700];
	char				PAST[64];
	size_t				LENGTH;
	int					INDEX;

	LENGTH = strcspn(TEXT, " ");

	if (!LENGTH || LENGTH >= sizeof FIRST)
		return ;

	snprintf(FIRST, sizeof FIRST, "%.*s", (int)LENGTH, TEXT);
	snprintf(REST, sizeof REST, "%s", TEXT + LENGTH);
	PAST[0] = 0;

	for (INDEX = 0; IRREGULAR[INDEX][0]; INDEX++)
		if (!strcasecmp(FIRST, IRREGULAR[INDEX][0]))
			snprintf(PAST, sizeof PAST, "%s", IRREGULAR[INDEX][1]);

	if (!PAST[0])
	{
		if (LENGTH > 1 && FIRST[LENGTH - 1] == 'e')
			snprintf(PAST, sizeof PAST, "%sd", FIRST);
		else if (
			LENGTH > 2 &&
			FIRST[LENGTH - 1] == 'y' &&
			!strchr("aeiou", FIRST[LENGTH - 2])
		)
			snprintf(PAST, sizeof PAST, "%.*sied", (int)LENGTH - 1, FIRST);
		else
			snprintf(PAST, sizeof PAST, "%sed", FIRST);
	}

	snprintf(TEXT, TEXT_SIZE, "%s%s", PAST, REST);
}

/* ---------- the state ---------- */

void
	WORK_INITIALIZE(WORK_STATE *STATE)
{
	memset(STATE, 0, sizeof *STATE);
	STATE->FOCUS = -1;
	STATE->NEXT_ID = 1;
	STATE->LAST_ASKED_TURN = -10;
}

const char
	*WORK_STATE_NAME(int STATE_VALUE)
{
	switch (STATE_VALUE)
	{
		case WORK_WAITING:
		{
			return ("waiting");
		}
		case WORK_DOING:
		{
			return ("doing");
		}
		case WORK_DONE:
		{
			return ("done");
		}
		case WORK_STUCK:
		{
			return ("stuck");
		}
		case WORK_ASKING:
		{
			return ("asking");
		}
		case WORK_DROPPED:
		{
			return ("dropped");
		}
		default:
		{
			return ("none");
		}
	}
}

/* ---------- know-how: what an action makes true, how to fix it ---------- */

void
	WORK_FORGET_RULES(WORK_STATE *STATE, const char *SOURCE)
{
	int	READ_AT;
	int	WRITE_AT = 0;

	for (READ_AT = 0; READ_AT < STATE->RULE_COUNT; READ_AT++)
		if (strcmp(STATE->RULES[READ_AT].SOURCE, SOURCE))
			STATE->RULES[WRITE_AT++] = STATE->RULES[READ_AT];

	STATE->RULE_COUNT = WRITE_AT;
}

/* one line: "trash: makes exists {1} = no", "mkdir: means make the folder
 * {1}", "trash: if no such file: find", "erase: kind risky"; 1 when it was
 * a rule */
static int
	LEARN_LINE(WORK_STATE *STATE, char *LINE, const char *SOURCE)
{
	char		*COLON;
	char		*REST;
	WORK_RULE	*RULE;

	TRIM(LINE);

	if (!LINE[0] || LINE[0] == '#')
		return (0);

	COLON = strchr(LINE, ':');

	if (!COLON || COLON == LINE || COLON - LINE >= 48)
		return (0);

	if (STATE->RULE_COUNT >= WORK_RULE_LIMIT)
		return (0);

	RULE = &STATE->RULES[STATE->RULE_COUNT];
	memset(RULE, 0, sizeof *RULE);
	snprintf(
		RULE->ACTION, sizeof RULE->ACTION, "%.*s", (int)(COLON - LINE), LINE
	);
	TRIM(RULE->ACTION);
	LOWER(RULE->ACTION);
	snprintf(RULE->SOURCE, sizeof RULE->SOURCE, "%s", SOURCE);
	REST = COLON + 1;

	while (*REST == ' ')
		REST++;

	if (!strncmp(REST, "makes ", 6))
	{
		RULE->KIND = WORK_RULE_MAKES;
		snprintf(RULE->TEXT, sizeof RULE->TEXT, "%s", REST + 6);
	}
	else if (!strncmp(REST, "means ", 6))
	{
		RULE->KIND = WORK_RULE_MEANS;
		snprintf(RULE->TEXT, sizeof RULE->TEXT, "%s", REST + 6);
	}
	else if (!strncmp(REST, "undo ", 5))
	{
		RULE->KIND = WORK_RULE_UNDO;
		snprintf(RULE->TEXT, sizeof RULE->TEXT, "%s", REST + 5);
	}
	else if (!strncmp(REST, "kind ", 5))
	{
		RULE->KIND = WORK_RULE_KIND;
		snprintf(RULE->TEXT, sizeof RULE->TEXT, "%s", REST + 5);
	}
	else if (!strncmp(REST, "if ", 3))
	{
		char	*SECOND_COLON = strchr(REST + 3, ':');

		if (!SECOND_COLON)
			return (0);

		RULE->KIND = WORK_RULE_IF;
		snprintf(
			RULE->RESULT, sizeof RULE->RESULT, "%.*s",
			(int)(SECOND_COLON - (REST + 3)), REST + 3
		);
		TRIM(RULE->RESULT);
		LOWER(RULE->RESULT);
		snprintf(RULE->TEXT, sizeof RULE->TEXT, "%s", SECOND_COLON + 1);
	}
	else
		return (0);

	TRIM(RULE->TEXT);
	STATE->RULE_COUNT++;

	return (1);
}

/* every line of TEXT that is know-how; how many were */
int
	WORK_LEARN(WORK_STATE *STATE, const char *TEXT, const char *SOURCE)
{
	const char	*LINE = TEXT;
	int			COUNT = 0;

	if (!TEXT)
		return (0);

	while (*LINE)
	{
		const char	*END = strchr(LINE, '\n');
		char		ONE[1200];

		if (!END)
			END = LINE + strlen(LINE);

		snprintf(ONE, sizeof ONE, "%.*s", (int)(END - LINE), LINE);
		COUNT += LEARN_LINE(STATE, ONE, SOURCE);

		if (*END)
			LINE = END + 1;
		else
			LINE = END;
	}

	return (COUNT);
}

static const WORK_RULE
	*FIND_RULE(const WORK_STATE *STATE, const char *ACTION, int KIND)
{
	int	INDEX;

	for (INDEX = 0; INDEX < STATE->RULE_COUNT; INDEX++)
		if (
			STATE->RULES[INDEX].KIND == KIND &&
			!strcmp(STATE->RULES[INDEX].ACTION, ACTION)
		)
			return (&STATE->RULES[INDEX]);

	return (NULL);
}

int
	WORK_HAS_CHECK(const WORK_STATE *STATE, const char *ACTION)
{
	return (FIND_RULE(STATE, ACTION, WORK_RULE_MAKES) != NULL);
}

/* the action takes something away ("makes exists {1} = no") */
int
	WORK_GOAL_IS_GONE(const WORK_STATE *STATE, const char *ACTION)
{
	const WORK_RULE	*RULE = FIND_RULE(STATE, ACTION, WORK_RULE_MAKES);

	return (RULE && strstr(RULE->TEXT, "{1} = no") != NULL);
}

static int
	IS_RISKY(const WORK_STATE *STATE, const char *ACTION)
{
	const WORK_RULE	*RULE = FIND_RULE(STATE, ACTION, WORK_RULE_KIND);

	return (RULE && strstr(RULE->TEXT, "risky") != NULL);
}

static int
	IS_READ_ONLY(const WORK_STATE *STATE, const char *ACTION)
{
	const WORK_RULE	*RULE = FIND_RULE(STATE, ACTION, WORK_RULE_KIND);

	return (RULE && strstr(RULE->TEXT, "looks") != NULL);
}

/* ---------- meanings: how Lucy reads talk about her work ---------- */

static void
	ADD_MEANING(WORK_STATE *STATE, const char *MEANING, const char *PHRASE)
{
	char	PLAIN[160];

	if (STATE->MEANING_COUNT >= WORK_MEANING_LIMIT)
		return ;

	PLAIN_WORDS(PHRASE, PLAIN, sizeof PLAIN);

	if (strlen(PLAIN) < 3)
		return ;

	snprintf(STATE->MEANINGS[STATE->MEANING_COUNT][0], 64, "%s", MEANING);
	snprintf(STATE->MEANINGS[STATE->MEANING_COUNT][1], 64, "%s", PLAIN);
	STATE->MEANING_COUNT++;
}

static void
	READ_MEANINGS(WORK_STATE *STATE, const char *TEXT)
{
	const char	*LINE = TEXT;

	while (*LINE)
	{
		const char	*END = strchr(LINE, '\n');
		char		ONE[4000];
		char		*COLON;
		char		*PHRASE;
		char		*SAVED;
		char		MEANING[64];

		if (!END)
			END = LINE + strlen(LINE);

		snprintf(ONE, sizeof ONE, "%.*s", (int)(END - LINE), LINE);

		if (*END)
			LINE = END + 1;
		else
			LINE = END;

		TRIM(ONE);

		if (!ONE[0] || ONE[0] == '#')
			continue ;

		COLON = strchr(ONE, ':');

		if (!COLON || COLON == ONE)
			continue ;

		snprintf(MEANING, sizeof MEANING, "%.*s", (int)(COLON - ONE), ONE);
		TRIM(MEANING);
		LOWER(MEANING);

		for (
			PHRASE = strtok_r(COLON + 1, ",", &SAVED);
			PHRASE;
			PHRASE = strtok_r(NULL, ",", &SAVED)
		)
			ADD_MEANING(STATE, MEANING, PHRASE);
	}
}

int
	WORK_LOAD_MEANINGS(WORK_STATE *STATE, const char *PATH)
{
	FILE	*STREAM;
	char	*TEXT;
	long	LENGTH;

	STATE->MEANING_COUNT = 0;

	if (PATH)
		snprintf(STATE->MEANINGS_PATH, sizeof STATE->MEANINGS_PATH, "%s", PATH);

	STATE->MEANINGS_STAMP = FILE_STAMP(STATE->MEANINGS_PATH);

	if (STATE->MEANINGS_PATH[0])
		STREAM = fopen(STATE->MEANINGS_PATH, "rb");
	else
		STREAM = NULL;

	if (!STREAM)
	{
		READ_MEANINGS(STATE, DEFAULT_MEANINGS);
		return (STATE->MEANING_COUNT);
	}

	fseek(STREAM, 0, SEEK_END);
	LENGTH = ftell(STREAM);
	fseek(STREAM, 0, SEEK_SET);
	TEXT = malloc((size_t)(LENGTH > 0 ? LENGTH : 0) + 1);

	if (!TEXT)
	{
		fclose(STREAM);
		READ_MEANINGS(STATE, DEFAULT_MEANINGS);
		return (STATE->MEANING_COUNT);
	}

	LENGTH = (long)fread(TEXT, 1, (size_t)(LENGTH > 0 ? LENGTH : 0), STREAM);
	TEXT[LENGTH] = 0;
	fclose(STREAM);
	READ_MEANINGS(STATE, TEXT);
	free(TEXT);

	/* a file with no meanings in it: the built-in ones */
	if (!STATE->MEANING_COUNT)
		READ_MEANINGS(STATE, DEFAULT_MEANINGS);

	return (STATE->MEANING_COUNT);
}

/* meanings.txt changed: read again; 1 when it did */
int
	WORK_REFRESH_MEANINGS(WORK_STATE *STATE)
{
	if (FILE_STAMP(STATE->MEANINGS_PATH) == STATE->MEANINGS_STAMP)
		return (0);

	WORK_LOAD_MEANINGS(STATE, NULL);

	return (1);
}

/* PLAIN (" it's still there ") says MEANING ("still there") */
int
	WORK_MEANS(const WORK_STATE *STATE, const char *MEANING, const char *PLAIN)
{
	int	INDEX;

	for (INDEX = 0; INDEX < STATE->MEANING_COUNT; INDEX++)
		if (
			!strcmp(STATE->MEANINGS[INDEX][0], MEANING) &&
			strstr(PLAIN, STATE->MEANINGS[INDEX][1])
		)
			return (1);

	return (0);
}

/* PLAIN starts with a phrase of MEANING (" yes please ", " no "), with at
 * most "oh", "well", "ok" before it */
int
	WORK_MEANS_AT_START(
		const WORK_STATE *STATE, const char *MEANING, const char *PLAIN
	)
{
	int	INDEX;

	for (INDEX = 0; INDEX < STATE->MEANING_COUNT; INDEX++)
	{
		const char	*PHRASE = STATE->MEANINGS[INDEX][1];

		if (strcmp(STATE->MEANINGS[INDEX][0], MEANING))
			continue ;

		if (!strncmp(PLAIN, PHRASE, strlen(PHRASE)))
			return (1);

		{
			static const char	*LEADS[7] = {
				" oh", " well", " ok", " okay", " um", " so", NULL
			};
			int					LEAD;

			for (LEAD = 0; LEADS[LEAD]; LEAD++)
				if (
					!strncmp(PLAIN, LEADS[LEAD], strlen(LEADS[LEAD])) &&
					!strncmp(
						PLAIN + strlen(LEADS[LEAD]), PHRASE, strlen(PHRASE)
					)
				)
					return (1);
		}
	}

	return (0);
}

/* mind/language/verbs.txt: "delete: delete, remove, get rid of | ..." */
int
	WORK_LOAD_VERBS(WORK_STATE *STATE, const char *PATH)
{
	FILE	*STREAM = fopen(PATH, "rb");
	char	LINE[2000];

	STATE->VERB_WORD_COUNT = 0;

	if (!STREAM)
		return (0);

	while (fgets(LINE, sizeof LINE, STREAM))
	{
		char	*COLON;
		char	*BAR;
		char	*WORD;
		char	*SAVED;
		char	CLASS[40];

		TRIM(LINE);

		if (!LINE[0] || LINE[0] == '#')
			continue ;

		COLON = strchr(LINE, ':');

		if (!COLON || COLON == LINE)
			continue ;

		snprintf(CLASS, sizeof CLASS, "%.*s", (int)(COLON - LINE), LINE);
		TRIM(CLASS);

		if (!strcmp(CLASS, "past") || !strcmp(CLASS, "net"))
			continue ;

		BAR = strchr(COLON, '|');

		if (BAR)
			*BAR = 0;

		for (
			WORD = strtok_r(COLON + 1, ",", &SAVED);
			WORD && STATE->VERB_WORD_COUNT < 160;
			WORD = strtok_r(NULL, ",", &SAVED)
		)
		{
			char	PLAIN[64];

			PLAIN_WORDS(WORD, PLAIN, sizeof PLAIN);

			if (strlen(PLAIN) < 3)
				continue ;

			snprintf(
				STATE->VERB_WORDS[STATE->VERB_WORD_COUNT][0], 40, "%s", CLASS
			);
			snprintf(
				STATE->VERB_WORDS[STATE->VERB_WORD_COUNT][1], 40, "%s", PLAIN
			);
			STATE->VERB_WORD_COUNT++;
		}
	}

	fclose(STREAM);

	return (STATE->VERB_WORD_COUNT);
}

/* the word at FOUND in PLAIN comes right after "the", "in", "my" ...:
 * "in the trash" is a place there, not "trash it" */
static int
	USED_AS_NOUN(const char *PLAIN, const char *FOUND)
{
	static const char	*BEFORE_NOUN[11] = {
		" the ", " a ", " an ", " in ", " into ", " from ", " of ", " my ",
		" your ", " this ", NULL
	};
	int					INDEX;

	for (INDEX = 0; BEFORE_NOUN[INDEX]; INDEX++)
	{
		size_t	LENGTH = strlen(BEFORE_NOUN[INDEX]);

		/* " the " ends where " trash " starts (they share the space) */
		if (
			FOUND - PLAIN >= (long)LENGTH - 1 &&
			!strncmp(FOUND - (LENGTH - 1), BEFORE_NOUN[INDEX], LENGTH)
		)
			return (1);
	}

	return (0);
}

/* where WORD stands in PLAIN (" list the files in trash ") as a verb: a
 * whole word, not used as a place; -1 when it doesn't */
static int
	VERB_AT(const char *PLAIN, const char *WORD)
{
	const char	*FOUND;

	/* WORD is " trash ", spaces around, like PLAIN's words */
	for (FOUND = strstr(PLAIN, WORD); FOUND; FOUND = strstr(FOUND + 1, WORD))
		if (!USED_AS_NOUN(PLAIN, FOUND))
			return ((int)(FOUND - PLAIN));

	return (-1);
}

/* the action the words of PLAIN ask for ("delete" for " remove it "): the
 * longest phrase, so "get rid of" beats "get", and of two as long the one
 * said first; a word used as a place ("list the files in trash") is no verb
 * there; NULL when none */
const char
	*WORK_VERB_CLASS(const WORK_STATE *STATE, const char *PLAIN)
{
	int	BEST = -1;
	int	BEST_AT = 0;
	int	BEST_WORDS = 0;
	int	INDEX;

	for (INDEX = 0; INDEX < STATE->VERB_WORD_COUNT; INDEX++)
	{
		const char	*WORD = STATE->VERB_WORDS[INDEX][1];
		int			AT = VERB_AT(PLAIN, WORD);
		int			WORDS = 0;
		const char	*CURSOR;

		if (AT < 0)
			continue ;

		for (CURSOR = WORD + 1; *CURSOR; CURSOR++)
			if (*CURSOR == ' ')
				WORDS++;

		/* more words win ("get rid of" over "get"), then the longer word,
		 * then the one said first */
		if (
			BEST < 0 ||
			WORDS > BEST_WORDS ||
			(
				WORDS == BEST_WORDS &&
				strlen(WORD) > strlen(STATE->VERB_WORDS[BEST][1])
			) ||
			(
				WORDS == BEST_WORDS &&
				strlen(WORD) == strlen(STATE->VERB_WORDS[BEST][1]) &&
				AT < BEST_AT
			)
		)
		{
			BEST = INDEX;
			BEST_AT = AT;
			BEST_WORDS = WORDS;
		}
	}

	if (BEST < 0)
		return (NULL);

	return (STATE->VERB_WORDS[BEST][0]);
}

/* does PLAIN have one of CLASS's words, however it is used ("the time")? */
int
	WORK_HAS_CLASS_WORD(
		const WORK_STATE *STATE, const char *PLAIN, const char *CLASS
	)
{
	int	INDEX;

	for (INDEX = 0; INDEX < STATE->VERB_WORD_COUNT; INDEX++)
		if (
			!strcmp(STATE->VERB_WORDS[INDEX][0], CLASS) &&
			strstr(PLAIN, STATE->VERB_WORDS[INDEX][1])
		)
			return (1);

	return (0);
}

/* the same, from the classes in ALLOWED only ("remove" in "don't
 * remember it, I told you to remove it" when only file work counts) */
const char
	*WORK_VERB_CLASS_AMONG(
		const WORK_STATE *STATE, const char *PLAIN, const char *const *ALLOWED
	)
{
	int	BEST = -1;
	int	BEST_AT = 0;
	int	INDEX;

	for (INDEX = 0; INDEX < STATE->VERB_WORD_COUNT; INDEX++)
	{
		const char	*WORD = STATE->VERB_WORDS[INDEX][1];
		int			AT;
		int			KNOWN = 0;
		int			CLASS_INDEX;

		for (CLASS_INDEX = 0; ALLOWED[CLASS_INDEX] && !KNOWN; CLASS_INDEX++)
			if (!strcmp(ALLOWED[CLASS_INDEX], STATE->VERB_WORDS[INDEX][0]))
				KNOWN = 1;

		if (!KNOWN || (AT = VERB_AT(PLAIN, WORD)) < 0)
			continue ;

		if (
			BEST < 0 ||
			strlen(WORD) > strlen(STATE->VERB_WORDS[BEST][1]) ||
			(strlen(WORD) == strlen(STATE->VERB_WORDS[BEST][1]) && AT < BEST_AT)
		)
		{
			BEST = INDEX;
			BEST_AT = AT;
		}
	}

	if (BEST < 0)
		return (NULL);

	return (STATE->VERB_WORDS[BEST][0]);
}

/* how many times words of the ALLOWED classes stand in PLAIN as verbs
 * ("make a folder and move 1.txt into it": 2); with LAST_CLASS, the class
 * of the last one */
int
	WORK_VERB_OCCURRENCES(
		const WORK_STATE *STATE, const char *PLAIN, const char *const *ALLOWED,
		const char **LAST_CLASS
	)
{
	int	SEEN[64];
	int	SEEN_COUNT = 0;
	int	LAST_AT = -1;
	int	INDEX;

	if (LAST_CLASS)
		*LAST_CLASS = NULL;

	for (INDEX = 0; INDEX < STATE->VERB_WORD_COUNT; INDEX++)
	{
		const char	*WORD = STATE->VERB_WORDS[INDEX][1];
		const char	*FOUND;
		int			KNOWN = 0;
		int			CLASS_INDEX;

		for (CLASS_INDEX = 0; ALLOWED[CLASS_INDEX] && !KNOWN; CLASS_INDEX++)
			if (!strcmp(ALLOWED[CLASS_INDEX], STATE->VERB_WORDS[INDEX][0]))
				KNOWN = 1;

		if (!KNOWN)
			continue ;

		for (
			FOUND = strstr(PLAIN, WORD);
			FOUND;
			FOUND = strstr(FOUND + 1, WORD)
		)
		{
			int	AT = (int)(FOUND - PLAIN);
			int	KNOWN_AT = 0;
			int	SEEN_INDEX;

			/* a word used as a place ("in the trash") is no verb */
			if (USED_AS_NOUN(PLAIN, FOUND))
				continue ;

			for (SEEN_INDEX = 0; SEEN_INDEX < SEEN_COUNT; SEEN_INDEX++)
				if (SEEN[SEEN_INDEX] == AT)
					KNOWN_AT = 1;

			if (KNOWN_AT || SEEN_COUNT >= 64)
				continue ;

			SEEN[SEEN_COUNT++] = AT;

			if (AT > LAST_AT)
			{
				LAST_AT = AT;

				if (LAST_CLASS)
					*LAST_CLASS = STATE->VERB_WORDS[INDEX][0];
			}
		}
	}

	return (SEEN_COUNT);
}

/* ---------- tasks ---------- */

int
	WORK_NEW_TASK(
		WORK_STATE *STATE, const char *ASKED, const char *ACTION,
		const char *ARGUMENT, const char *THING
	)
{
	WORK_TASK	*TASK;
	int			INDEX;

	/* the oldest task gives its place */
	if (STATE->TASK_COUNT == WORK_TASK_LIMIT)
	{
		int	OLDEST = 0;

		for (INDEX = 1; INDEX < STATE->TASK_COUNT; INDEX++)
			if (
				STATE->TASKS[INDEX].STATE != WORK_WAITING &&
				STATE->TASKS[INDEX].TOUCHED < STATE->TASKS[OLDEST].TOUCHED
			)
				OLDEST = INDEX;

		INDEX = OLDEST;

		if (STATE->FOCUS == INDEX)
			STATE->FOCUS = -1;
	}
	else
		INDEX = STATE->TASK_COUNT++;

	TASK = &STATE->TASKS[INDEX];
	memset(TASK, 0, sizeof *TASK);
	TASK->ID = STATE->NEXT_ID++;
	TASK->STATE = WORK_DOING;
	snprintf(TASK->ASKED, sizeof TASK->ASKED, "%s", ASKED ? ASKED : "");
	snprintf(TASK->ACTION, sizeof TASK->ACTION, "%s", ACTION);
	snprintf(TASK->ARGUMENT, sizeof TASK->ARGUMENT, "%s", ARGUMENT);

	if (THING && THING[0])
		snprintf(TASK->THING, sizeof TASK->THING, "%s", THING);
	else
		ARGUMENT_PART(ARGUMENT, 1, TASK->THING, sizeof TASK->THING);

	TASK->CHECKED = -1;
	TASK->TURN = STATE->TURN;
	TASK->TOUCHED = STATE->TURN;
	TASK->MADE_AT = NOW_SECONDS();
	STATE->FOCUS = INDEX;

	return (INDEX);
}

void
	WORK_ADD_STEP(
		WORK_STATE *STATE, int TASK_INDEX, int KIND, const char *ACTION,
		const char *ARGUMENT, const char *RESULT, int WORKED
	)
{
	WORK_TASK	*TASK = &STATE->TASKS[TASK_INDEX];
	WORK_STEP	*STEP;

	/* full: the oldest look around goes first (it changed nothing), else
	 * the oldest step */
	if (TASK->STEP_COUNT == WORK_STEP_LIMIT)
	{
		int	DROP = 0;
		int	INDEX;

		for (INDEX = 0; INDEX < TASK->STEP_COUNT; INDEX++)
			if (TASK->STEPS[INDEX].KIND == WORK_STEP_LOOK)
			{
				DROP = INDEX;
				break ;
			}

		memmove(
			TASK->STEPS + DROP, TASK->STEPS + DROP + 1,
			(size_t)(WORK_STEP_LIMIT - DROP - 1) * sizeof TASK->STEPS[0]
		);
		TASK->STEP_COUNT--;
	}

	STEP = &TASK->STEPS[TASK->STEP_COUNT++];
	memset(STEP, 0, sizeof *STEP);
	STEP->KIND = KIND;
	snprintf(STEP->ACTION, sizeof STEP->ACTION, "%s", ACTION);
	snprintf(STEP->ARGUMENT, sizeof STEP->ARGUMENT, "%s", ARGUMENT);
	snprintf(STEP->RESULT, sizeof STEP->RESULT, "%s", RESULT ? RESULT : "");
	STEP->WORKED = WORKED;
	STEP->TURN = STATE->TURN;
	TASK->TOUCHED = STATE->TURN;
}

/* the task "it" means: the one worked on last, not older than MAX_AGE
 * turns; -1 when there is none */
int
	WORK_FOCUS_TASK(const WORK_STATE *STATE, int MAX_AGE)
{
	if (
		STATE->FOCUS< 0 ||
		STATE->FOCUS >= STATE->TASK_COUNT ||
		STATE->TURN - STATE->TASKS[STATE->FOCUS].TOUCHED> MAX_AGE
	)
		return (-1);

	return (STATE->FOCUS);
}

/* the newest task about THING ("42"), -1 when none */
int
	WORK_FIND_TASK(const WORK_STATE *STATE, const char *THING)
{
	int	BEST = -1;
	int	INDEX;

	for (INDEX = 0; INDEX < STATE->TASK_COUNT; INDEX++)
	{
		char	NAME[300];

		WORK_LAST_NAME(STATE->TASKS[INDEX].THING, NAME, sizeof NAME);

		if (
			(
				!strcasecmp(STATE->TASKS[INDEX].THING, THING) ||
				!strcasecmp(NAME, THING)
			) &&
			(
				BEST < 0 ||
				STATE->TASKS[INDEX].TOUCHED >= STATE->TASKS[BEST].TOUCHED
			)
		)
			BEST = INDEX;
	}

	return (BEST);
}

/* ---------- templates: {1} {2} {name 1} {folder 1} {free X} {free}
 * {thing} <tab> ---------- */

static int	EXPAND(
	WORK_STATE *STATE, WORK_TASK *TASK, const WORK_HANDS *HANDS,
	const char *TEMPLATE, int LENGTH, char *OUTPUT, int OUTPUT_SIZE
);

/* PATH when nothing has that name, else "PATH (2)", "PATH (3)" ... */
static void
	FREE_PATH(
		const WORK_HANDS *HANDS, const char *PATH, char *OUTPUT, int OUTPUT_SIZE
	)
{
	char		RESULT[200];
	char		STEM[700];
	const char	*ENDING = "";
	const char	*SLASH = strrchr(PATH, '/');
	const char	*DOT = strrchr(SLASH ? SLASH : PATH, '.');
	int			NUMBER;

	snprintf(OUTPUT, OUTPUT_SIZE, "%s", PATH);

	if (!HANDS || !HANDS->DO)
		return ;

	/* "notes.txt" becomes "notes (2).txt", a folder "42 (2)" */
	if (DOT && DOT > (SLASH ? SLASH + 1 : PATH))
	{
		ENDING = DOT;
		snprintf(STEM, sizeof STEM, "%.*s", (int)(DOT - PATH), PATH);
	}
	else
		snprintf(STEM, sizeof STEM, "%s", PATH);

	for (NUMBER = 2; NUMBER < 100; NUMBER++)
	{
		RESULT[0] = 0;
		HANDS->DO(HANDS->ENGINE, "exists", OUTPUT, RESULT, sizeof RESULT);
		LOWER(RESULT);

		if (!strcmp(RESULT, "no"))
			return ;

		snprintf(OUTPUT, OUTPUT_SIZE, "%s (%d)%s", STEM, NUMBER, ENDING);
	}
}

/* Lucy's words for a place; DEFAULT without her sayings */
static void
	SAY_PLACE(
		const WORK_HANDS *HANDS, const char *KEY, const char *DEFAULT,
		const char *FIRST, char *OUTPUT, int OUTPUT_SIZE
	)
{
	const char	*BRACE;

	if (HANDS && HANDS->SAY)
	{
		HANDS->SAY(
			HANDS->ENGINE, KEY, DEFAULT, FIRST, NULL, NULL, OUTPUT, OUTPUT_SIZE
		);
		return ;
	}

	BRACE = strstr(DEFAULT, "{1}");

	if (BRACE)
		snprintf(
			OUTPUT, OUTPUT_SIZE, "%.*s%s%s", (int)(BRACE - DEFAULT), DEFAULT,
			FIRST ? FIRST : "", BRACE + 3
		);
	else
		snprintf(OUTPUT, OUTPUT_SIZE, "%s", DEFAULT);
}

static void
	DIRECTIVE(
		WORK_STATE *STATE, WORK_TASK *TASK, const WORK_HANDS *HANDS,
		const char *INSIDE, char *OUTPUT, int OUTPUT_SIZE
	)
{
	char	PART[700];

	(void)STATE;

	if (!strcmp(INSIDE, "1") || !strcmp(INSIDE, "2"))
	{
		ARGUMENT_PART(TASK->ARGUMENT, INSIDE[0] - '0', OUTPUT, OUTPUT_SIZE);
		return ;
	}

	if (!strncmp(INSIDE, "name ", 5) || !strncmp(INSIDE, "folder ", 7))
	{
		const char	*WHICH = strchr(INSIDE, ' ') + 1;

		if (!strcmp(WHICH, "1") || !strcmp(WHICH, "2"))
			ARGUMENT_PART(TASK->ARGUMENT, WHICH[0] - '0', PART, sizeof PART);
		else
			snprintf(PART, sizeof PART, "%s", WHICH);

		if (INSIDE[0] == 'n')
			WORK_LAST_NAME(PART, OUTPUT, OUTPUT_SIZE);
		else
			FOLDER_PART(PART, OUTPUT, OUTPUT_SIZE);

		return ;
	}

	/* {free or 42}: the free name used, else 42 */
	if (!strncmp(INSIDE, "free or ", 8))
	{
		snprintf(
			OUTPUT, OUTPUT_SIZE, "%s",
			TASK->FREE_NAME[0] ? TASK->FREE_NAME : INSIDE + 8
		);
		return ;
	}

	if (!strncmp(INSIDE, "free ", 5))
	{
		FREE_PATH(HANDS, INSIDE + 5, OUTPUT, OUTPUT_SIZE);
		WORK_LAST_NAME(OUTPUT, TASK->FREE_NAME, sizeof TASK->FREE_NAME);
		return ;
	}

	if (!strcmp(INSIDE, "free"))
	{
		snprintf(OUTPUT, OUTPUT_SIZE, "%s", TASK->FREE_NAME);
		return ;
	}

	if (!strcmp(INSIDE, "thing"))
	{
		snprintf(OUTPUT, OUTPUT_SIZE, "%s", TASK->THING);
		return ;
	}

	/* {place work} "the folder work", {place } "your files folder" */
	if (!strncmp(INSIDE, "place", 5) && (!INSIDE[5] || INSIDE[5] == ' '))
	{
		const char	*WHICH = INSIDE[5] ? INSIDE + 6 : "";

		while (*WHICH == ' ')
			WHICH++;

		if (WHICH[0])
			SAY_PLACE(
				HANDS, "work place folder", "the folder {1}", WHICH, OUTPUT,
				OUTPUT_SIZE
			);
		else
			SAY_PLACE(
				HANDS, "work place files", "your files folder", NULL, OUTPUT,
				OUTPUT_SIZE
			);

		return ;
	}

	/* not a directive: kept as it was written */
	snprintf(OUTPUT, OUTPUT_SIZE, "{%s}", INSIDE);
}

static int
	EXPAND(
		WORK_STATE *STATE, WORK_TASK *TASK, const WORK_HANDS *HANDS,
		const char *TEMPLATE, int LENGTH, char *OUTPUT, int OUTPUT_SIZE
	)
{
	int	POSITION = 0;
	int	INDEX = 0;

	if (OUTPUT_SIZE < 1)
		return (0);

	while (INDEX < LENGTH && POSITION < OUTPUT_SIZE - 1)
	{
		if (TEMPLATE[INDEX] == '{')
		{
			int		DEPTH = 1;
			int		END = INDEX + 1;
			char	INSIDE[700];
			char	VALUE[700];

			while (END < LENGTH && DEPTH)
			{
				if (TEMPLATE[END] == '{')
					DEPTH++;
				else if (TEMPLATE[END] == '}')
					DEPTH--;

				if (DEPTH)
					END++;
			}

			if (DEPTH)
			{
				OUTPUT[POSITION++] = TEMPLATE[INDEX++];
				continue ;
			}

			/* what is inside is filled in first: {free trash/{name 1}} */
			EXPAND(
				STATE, TASK, HANDS, TEMPLATE + INDEX + 1, END - INDEX - 1,
				INSIDE, sizeof INSIDE
			);
			DIRECTIVE(STATE, TASK, HANDS, INSIDE, VALUE, sizeof VALUE);
			POSITION += snprintf(
				OUTPUT + POSITION, OUTPUT_SIZE - POSITION, "%s", VALUE
			);

			if (POSITION >= OUTPUT_SIZE)
				POSITION = OUTPUT_SIZE - 1;

			INDEX = END + 1;
			continue ;
		}

		if (!strncmp(TEMPLATE + INDEX, "<tab>", 5) && INDEX + 5 <= LENGTH)
		{
			OUTPUT[POSITION++] = '\t';
			INDEX += 5;
			continue ;
		}

		OUTPUT[POSITION++] = TEMPLATE[INDEX++];
	}

	OUTPUT[POSITION] = 0;

	return (POSITION);
}

static void
	EXPAND_TEXT(
		WORK_STATE *STATE, WORK_TASK *TASK, const WORK_HANDS *HANDS,
		const char *TEMPLATE, char *OUTPUT, int OUTPUT_SIZE
	)
{
	EXPAND(
		STATE, TASK, HANDS, TEMPLATE, (int)strlen(TEMPLATE), OUTPUT, OUTPUT_SIZE
	);
}

/* ---------- checking ---------- */

/* one clause: "exists {1} = no", "exists {2} != no", "read {1} has {2}" */
static int
	CHECK_CLAUSE(
		WORK_STATE *STATE, WORK_TASK *TASK, const WORK_HANDS *HANDS,
		const char *CLAUSE
	)
{
	char		EXPANDED[1400];
	char		ACTION[48];
	char		ARGUMENT[1400];
	char		EXPECTED[600];
	char		RESULT[2000];
	const char	*OPERATOR;
	int			NEGATED = 0;
	int			CONTAINS = 0;
	int			CODE;
	const char	*SPACE;

	EXPAND_TEXT(STATE, TASK, HANDS, CLAUSE, EXPANDED, sizeof EXPANDED);

	if ((OPERATOR = strstr(EXPANDED, " != ")))
		NEGATED = 1;
	else if ((OPERATOR = strstr(EXPANDED, " has ")))
		CONTAINS = 1;
	else if (!(OPERATOR = strstr(EXPANDED, " = ")))
		return (-1);

	SPACE = strchr(EXPANDED, ' ');

	if (!SPACE || SPACE > OPERATOR)
		return (-1);

	snprintf(ACTION, sizeof ACTION, "%.*s", (int)(SPACE - EXPANDED), EXPANDED);
	snprintf(
		ARGUMENT, sizeof ARGUMENT, "%.*s", (int)(OPERATOR - SPACE - 1),
		SPACE + 1
	);
	snprintf(
		EXPECTED, sizeof EXPECTED, "%s",
		OPERATOR +
			(NEGATED ? 4
				: CONTAINS ? 5
							: 3)
	);
	TRIM(EXPECTED);
	LOWER(EXPECTED);
	RESULT[0] = 0;
	CODE = HANDS->DO(HANDS->ENGINE, ACTION, ARGUMENT, RESULT, sizeof RESULT);

	/* no skill can look: it can't be told */
	if (CODE == -1000 || !strcmp(RESULT, "no computer skill"))
		return (-1);

	{
		char	SHOWN[300];
		char	FIRST_PART[600];

		ARGUMENT_PART(ARGUMENT, 1, FIRST_PART, sizeof FIRST_PART);
		snprintf(
			SHOWN, sizeof SHOWN, "%s %s -> %.100s", ACTION, FIRST_PART, RESULT
		);
		snprintf(TASK->SEEN, sizeof TASK->SEEN, "%s", SHOWN);
	}

	LOWER(RESULT);
	TRIM(RESULT);

	if (CONTAINS)
	{
		char	WANTED[600];

		snprintf(WANTED, sizeof WANTED, "%s", EXPECTED);

		return (CODE == 0 && (!WANTED[0] || strstr(RESULT, WANTED) != NULL));
	}

	if (NEGATED)
		return (strcmp(RESULT, EXPECTED) != 0);

	return (!strcmp(RESULT, EXPECTED));
}

/* is the task done? 1 yes, 0 no, -1 when it can't be told */
int
	WORK_CHECK(WORK_STATE *STATE, int TASK_INDEX, const WORK_HANDS *HANDS)
{
	WORK_TASK		*TASK = &STATE->TASKS[TASK_INDEX];
	const WORK_RULE	*RULE = FIND_RULE(STATE, TASK->ACTION, WORK_RULE_MAKES);
	char			CLAUSES[600];
	char			*CLAUSE;
	char			*NEXT;
	int				ALL = 1;

	if (!RULE || !HANDS || !HANDS->DO)
	{
		TASK->CHECKED = -1;
		return (-1);
	}

	snprintf(CLAUSES, sizeof CLAUSES, "%s", RULE->TEXT);

	for (CLAUSE = CLAUSES; CLAUSE; CLAUSE = NEXT)
	{
		int	MET;

		NEXT = strstr(CLAUSE, " and ");

		if (NEXT)
		{
			*NEXT = 0;
			NEXT += 5;
		}

		MET = CHECK_CLAUSE(STATE, TASK, HANDS, CLAUSE);

		if (MET < 0)
		{
			TASK->CHECKED = -1;
			return (-1);
		}

		if (!MET)
			ALL = 0;
	}

	TASK->CHECKED = ALL;
	SHOW_LINE(
		HANDS, "check: %s: %s", TASK->SEEN, ALL ? "done" : "not done yet"
	);

	return (ALL);
}

/* ---------- the solver ---------- */

typedef struct
{
	int		KIND;
	char	RAW[300];
	char	STEP[700];
	char	SAID[300];
} WORK_FIX;

enum
{
	FIX_TRY = 1,
	FIX_ASK,
	FIX_DONE,
	FIX_FIND,
	FIX_RETRY,
	FIX_LOOK
};

static int
	WAS_TRIED(const WORK_TASK *TASK, const char *RAW)
{
	int	INDEX;

	for (INDEX = 0; INDEX < TASK->TRIED_COUNT; INDEX++)
		if (!strcmp(TASK->TRIED[INDEX], RAW))
			return (1);

	return (0);
}

static void
	MARK_TRIED(WORK_TASK *TASK, const char *RAW)
{
	if (TASK->TRIED_COUNT < WORK_TRIED_LIMIT && !WAS_TRIED(TASK, RAW))
		snprintf(TASK->TRIED[TASK->TRIED_COUNT++], 300, "%s", RAW);
}

/* a step the user said no to ("mkdir welcome (2).txt"): never offered
 * again, unless they say yes to it after all */
static int
	IS_DECLINED(const WORK_TASK *TASK, const char *STEP)
{
	int	INDEX;

	for (INDEX = 0; INDEX < TASK->DECLINED_COUNT; INDEX++)
		if (!strcmp(TASK->DECLINED[INDEX], STEP))
			return (1);

	return (0);
}

static void
	ADD_DECLINED(WORK_TASK *TASK, const char *STEP)
{
	if (!STEP[0] || IS_DECLINED(TASK, STEP))
		return ;

	if (TASK->DECLINED_COUNT == WORK_DECLINED_LIMIT)
	{
		memmove(
			TASK->DECLINED, TASK->DECLINED + 1,
			(WORK_DECLINED_LIMIT - 1) * sizeof TASK->DECLINED[0]
		);
		TASK->DECLINED_COUNT--;
	}

	snprintf(
		TASK->DECLINED[TASK->DECLINED_COUNT++], sizeof TASK->DECLINED[0], "%s",
		STEP
	);
}

static void
	REMOVE_DECLINED(WORK_TASK *TASK, const char *STEP)
{
	int	INDEX;

	for (INDEX = 0; INDEX < TASK->DECLINED_COUNT; INDEX++)
		if (!strcmp(TASK->DECLINED[INDEX], STEP))
		{
			memmove(
				TASK->DECLINED + INDEX, TASK->DECLINED + INDEX + 1,
				(size_t)(TASK->DECLINED_COUNT - INDEX - 1) *
					sizeof TASK->DECLINED[0]
			);
			TASK->DECLINED_COUNT--;
			return ;
		}
}

static void	SAY(
	const WORK_HANDS *HANDS, const char *KEY, const char *DEFAULT,
	const char *FIRST, const char *SECOND, const char *THIRD, char *OUTPUT,
	int OUTPUT_SIZE
);

/* what was tried for the task, in words, for "I've tried ...": KEY's
 * sentence with DETAIL ("looked for 42 in all your folders") */
static void
	NOTE_ATTEMPT(
		WORK_TASK *TASK, const WORK_HANDS *HANDS, const char *KEY,
		const char *DEFAULT, const char *DETAIL
	)
{
	char	SAID[200];
	int		INDEX;

	SAY(HANDS, KEY, DEFAULT, DETAIL, NULL, NULL, SAID, sizeof SAID);
	DROP_END(SAID);
	LOWER_FIRST(SAID);

	for (INDEX = 0; INDEX < TASK->ATTEMPT_COUNT; INDEX++)
		if (!strcmp(TASK->ATTEMPTS[INDEX], SAID))
			return ;

	if (TASK->ATTEMPT_COUNT < WORK_ATTEMPT_LIMIT)
		snprintf(
			TASK->ATTEMPTS[TASK->ATTEMPT_COUNT++], sizeof TASK->ATTEMPTS[0],
			"%s", SAID
		);
}

/* "tried it again, looked for 42 in all your folders and offered to ..." */
static void
	ATTEMPTS_SAID(const WORK_TASK *TASK, char *OUTPUT, int OUTPUT_SIZE)
{
	int	POSITION = 0;
	int	INDEX;

	if (OUTPUT_SIZE)
		OUTPUT[0] = 0;

	for (INDEX = 0; INDEX < TASK->ATTEMPT_COUNT; INDEX++)
	{
		POSITION += snprintf(
			OUTPUT + POSITION, OUTPUT_SIZE - POSITION, "%s%s",
			!INDEX ? ""
				: INDEX == TASK->ATTEMPT_COUNT - 1 ? " and "
			: ", ",
			TASK->ATTEMPTS[INDEX]
		);

		if (POSITION >= OUTPUT_SIZE)
			break ;
	}
}

/* "trash has a file with that name" matches the rule's "if ..." */
static int
	CAUSE_MATCHES(const char *RULE_RESULT, const char *CAUSE)
{
	char	LOWER_CAUSE[300];

	if (!strcmp(RULE_RESULT, "*"))
		return (1);

	snprintf(LOWER_CAUSE, sizeof LOWER_CAUSE, "%s", CAUSE);
	LOWER(LOWER_CAUSE);
	TRIM(LOWER_CAUSE);

	return (!strcmp(RULE_RESULT, LOWER_CAUSE));
}

/* the rule's words for the cause ("the trash already has a 42"), filled */
static void
	CAUSE_WORDS(
		WORK_STATE *STATE, WORK_TASK *TASK, const WORK_HANDS *HANDS,
		const char *CAUSE, char *OUTPUT, int OUTPUT_SIZE
	)
{
	int	INDEX;

	if (OUTPUT_SIZE)
		OUTPUT[0] = 0;

	for (INDEX = 0; INDEX < STATE->RULE_COUNT; INDEX++)
	{
		const WORK_RULE	*RULE = &STATE->RULES[INDEX];
		char			FIRST[600];
		const char		*SEMICOLON;

		if (
			RULE->KIND != WORK_RULE_IF ||
			strcmp(RULE->ACTION, TASK->ACTION) ||
			!CAUSE_MATCHES(RULE->RESULT, CAUSE)
		)
			continue ;

		SEMICOLON = strchr(RULE->TEXT, ';');
		snprintf(
			FIRST, sizeof FIRST, "%.*s",
			SEMICOLON ? (int)(SEMICOLON - RULE->TEXT) : (int)strlen(RULE->TEXT),
			RULE->TEXT
		);
		TRIM(FIRST);

		if (
			!strncmp(FIRST, "try ", 4) ||
			!strncmp(FIRST, "ask ", 4) ||
			!strncmp(FIRST, "look ", 5) ||
			!strncmp(FIRST, "done", 4) ||
			!strcmp(FIRST, "find") ||
			!strcmp(FIRST, "retry")
		)
			continue ;

		EXPAND_TEXT(STATE, TASK, HANDS, FIRST, OUTPUT, OUTPUT_SIZE);
		return ;
	}
}

/* does the user's message (PLAIN, " words ") name what the step works on?
 * "it is not in the trash" names "list trash" */
static int
	STEP_IS_NAMED(const char *STEP, const char *PLAIN)
{
	const char	*SPACE = strchr(STEP, ' ');
	char		WORDS[700];
	char		*WORD;
	char		*SAVED;

	if (!PLAIN || !SPACE)
		return (0);

	PLAIN_WORDS(SPACE + 1, WORDS, sizeof WORDS);

	for (
		WORD = strtok_r(WORDS, " ", &SAVED);
		WORD;
		WORD = strtok_r(NULL, " ", &SAVED)
	)
	{
		char	WANTED[80];

		if (strlen(WORD) < 4)
			continue ;

		snprintf(WANTED, sizeof WANTED, " %s ", WORD);

		if (strstr(PLAIN, WANTED))
			return (1);
	}

	return (0);
}

/* the next fix for CAUSE that wasn't tried; 0 when none is left.  With
 * PREFER (the user's words), a fix about what they named comes first. */
static int
	NEXT_FIX_FOR(
		WORK_STATE *STATE, WORK_TASK *TASK, const WORK_HANDS *HANDS,
		const char *CAUSE, const char *PREFER, WORK_FIX *FIX
	)
{
	int	INDEX;
	int	PASS;

	memset(FIX, 0, sizeof *FIX);

	for (PASS = PREFER ? 0 : 1; PASS < 2; PASS++)
		for (INDEX = 0; INDEX < STATE->RULE_COUNT; INDEX++)
		{
			const WORK_RULE	*RULE = &STATE->RULES[INDEX];
			char			LIST[600];
			char			*PIECE;
			char			*SAVED;

			if (
				RULE->KIND != WORK_RULE_IF ||
				strcmp(RULE->ACTION, TASK->ACTION) ||
				!CAUSE_MATCHES(RULE->RESULT, CAUSE)
			)
				continue ;

			snprintf(LIST, sizeof LIST, "%s", RULE->TEXT);

			for (
				PIECE = strtok_r(LIST, ";", &SAVED);
				PIECE;
				PIECE = strtok_r(NULL, ";", &SAVED)
			)
			{
				char	RAW[300];
				char	*EQUALS;
				int		KIND = 0;
				char	*BODY;

				snprintf(RAW, sizeof RAW, "%s", PIECE);
				TRIM(RAW);

				if (!strncmp(RAW, "try ", 4))
					KIND = FIX_TRY;
				else if (!strncmp(RAW, "ask ", 4))
					KIND = FIX_ASK;
				else if (!strncmp(RAW, "look ", 5))
					KIND = FIX_LOOK;
				else if (!strncmp(RAW, "done", 4))
					KIND = FIX_DONE;
				else if (!strcmp(RAW, "find"))
					KIND = FIX_FIND;
				else if (!strcmp(RAW, "retry"))
					KIND = FIX_RETRY;

				/* looking again at what the user names is never wasted: it
				 * changes nothing and things may have moved */
				if (
					!KIND ||
					(WAS_TRIED(TASK, RAW) && (PASS || KIND != FIX_LOOK))
				)
					continue ;

				FIX->KIND = KIND;
				snprintf(FIX->RAW, sizeof FIX->RAW, "%s", RAW);
				BODY = RAW;

				if (KIND == FIX_TRY || KIND == FIX_ASK)
					BODY += 4;
				else if (KIND == FIX_LOOK)
					BODY += 5;
				else if (KIND == FIX_DONE)
					BODY += 4;
				else
					BODY += strlen(BODY);

				EQUALS = strstr(BODY, " = ");

				if (!EQUALS && KIND == FIX_DONE && !strncmp(BODY, " =", 2))
					EQUALS = BODY;

				if (EQUALS)
				{
					char	SAID[300];

					snprintf(
						SAID, sizeof SAID, "%s",
						EQUALS + (EQUALS == BODY ? 2 : 3)
					);
					TRIM(SAID);
					*EQUALS = 0;
					snprintf(FIX->SAID, sizeof FIX->SAID, "%s", SAID);
				}

				TRIM(BODY);

				if (BODY[0])
					EXPAND_TEXT(
						STATE, TASK, HANDS, BODY, FIX->STEP, sizeof FIX->STEP
					);

				/* the first pass only takes what the user named */
				if (!PASS && !STEP_IS_NAMED(FIX->STEP, PREFER))
				{
					memset(FIX, 0, sizeof *FIX);
					continue ;
				}

				/* what the user said no to is not offered again */
				if (FIX->STEP[0] && IS_DECLINED(TASK, FIX->STEP))
				{
					memset(FIX, 0, sizeof *FIX);
					continue ;
				}

				/* the words are filled after the step: {free} is known now */
				if (FIX->SAID[0])
				{
					char	FILLED[300];

					EXPAND_TEXT(
						STATE, TASK, HANDS, FIX->SAID, FILLED, sizeof FILLED
					);
					snprintf(FIX->SAID, sizeof FIX->SAID, "%s", FILLED);
				}

				return (1);
			}
		}

	/* nothing taught for it: a thing that isn't where it was asked for is
	 * looked for, and so is another one with the name of a thing the user
	 * still sees */
	{
		char	LOWER_CAUSE[300];

		snprintf(LOWER_CAUSE, sizeof LOWER_CAUSE, "%s", CAUSE);
		LOWER(LOWER_CAUSE);

		if (
			(
				strstr(LOWER_CAUSE, "no such") ||
				strstr(LOWER_CAUSE, "not found") ||
				strstr(LOWER_CAUSE, "missing") ||
				(
					!strcmp(LOWER_CAUSE, "user disagrees") &&
					WORK_GOAL_IS_GONE(STATE, TASK->ACTION)
				)
			) &&
			!WAS_TRIED(TASK, "find")
		)
		{
			FIX->KIND = FIX_FIND;
			snprintf(FIX->RAW, sizeof FIX->RAW, "find");
			return (1);
		}
	}

	return (0);
}

static int
	NEXT_FIX(
		WORK_STATE *STATE, WORK_TASK *TASK, const WORK_HANDS *HANDS,
		const char *CAUSE, WORK_FIX *FIX
	)
{
	return (NEXT_FIX_FOR(STATE, TASK, HANDS, CAUSE, NULL, FIX));
}

/* "move 42<tab>trash/42 (2)" -> "move", "42<tab>trash/42 (2)" */
static void
	SPLIT_STEP(
		const char *STEP, char *ACTION, int ACTION_SIZE, char *ARGUMENT,
		int ARGUMENT_SIZE
	)
{
	const char	*SPACE = strchr(STEP, ' ');

	if (!SPACE)
	{
		snprintf(ACTION, ACTION_SIZE, "%s", STEP);
		snprintf(ARGUMENT, ARGUMENT_SIZE, "%s", "");
		return ;
	}

	snprintf(ACTION, ACTION_SIZE, "%.*s", (int)(SPACE - STEP), STEP);
	snprintf(ARGUMENT, ARGUMENT_SIZE, "%s", SPACE + 1);
}

static int
	RUN_STEP(
		WORK_STATE *STATE, int TASK_INDEX, const WORK_HANDS *HANDS, int KIND,
		const char *ACTION, const char *ARGUMENT, const char *SAID
	)
{
	char	RESULT[2000];
	int		CODE;

	RESULT[0] = 0;
	CODE = HANDS->DO(HANDS->ENGINE, ACTION, ARGUMENT, RESULT, sizeof RESULT);
	SHOW_LINE(
		HANDS, "%s: %s %s: %s",
		KIND == WORK_STEP_FIX ? "try"
			: KIND == WORK_STEP_RETRY ? "again"
			: KIND == WORK_STEP_FOUND ? "use"
		: "do",
		ACTION, ARGUMENT, RESULT
	);
	WORK_ADD_STEP(
		STATE, TASK_INDEX, KIND, ACTION, ARGUMENT, RESULT, CODE == 0 ? 1 : 0
	);

	if (SAID && SAID[0])
	{
		WORK_TASK	*TASK = &STATE->TASKS[TASK_INDEX];

		snprintf(
			TASK->STEPS[TASK->STEP_COUNT - 1].SAID, sizeof TASK->STEPS[0].SAID,
			"%s", SAID
		);
	}

	/* the same action done to another thing instead ("make the folder as
	 * welcome (2).txt instead"): the task is about that one now */
	{
		WORK_TASK	*TASK = &STATE->TASKS[TASK_INDEX];

		if (
			KIND == WORK_STEP_FIX &&
			CODE == 0 &&
			!strcmp(ACTION, TASK->ACTION) &&
			strcmp(ARGUMENT, TASK->ARGUMENT)
		)
		{
			snprintf(TASK->ARGUMENT, sizeof TASK->ARGUMENT, "%s", ARGUMENT);
			ARGUMENT_PART(ARGUMENT, 1, TASK->THING, sizeof TASK->THING);
			SHOW_LINE(HANDS, "now about: %s", TASK->THING);
		}
	}

	return (CODE == 0);
}

/* the failure the task is stuck on: the last main step or retry that
 * didn't work (a fix that didn't work is no cause of its own) */
static const char
	*CURRENT_CAUSE(const WORK_TASK *TASK)
{
	int	INDEX;

	for (INDEX = TASK->STEP_COUNT - 1; INDEX >= 0; INDEX--)
	{
		const WORK_STEP	*STEP = &TASK->STEPS[INDEX];

		if (
			(
				STEP->KIND == WORK_STEP_MAIN ||
				STEP->KIND == WORK_STEP_RETRY ||
				STEP->KIND == WORK_STEP_FOUND
			)
		)
			return (STEP->RESULT);
	}

	return ("");
}

/* some step of the task did what it was asked (or the task was done once) */
static int
	TASK_EVER_WORKED(const WORK_TASK *TASK)
{
	int	INDEX;

	if (TASK->DONE_AT > 0)
		return (1);

	for (INDEX = 0; INDEX < TASK->STEP_COUNT; INDEX++)
		if (TASK->STEPS[INDEX].WORKED == 1)
			return (1);

	return (0);
}

/* works TASK_INDEX until it is done, stuck, or waits for a yes */
int
	WORK_SOLVE(WORK_STATE *STATE, int TASK_INDEX, const WORK_HANDS *HANDS)
{
	WORK_TASK	*TASK = &STATE->TASKS[TASK_INDEX];
	int			ROUND;

	STATE->FOCUS = TASK_INDEX;
	TASK->TOUCHED = STATE->TURN;
	TASK->STATE = WORK_DOING;
	TASK->ASKING_DOUBT = 0;

	for (ROUND = 0; ROUND < 10; ROUND++)
	{
		int			MET = WORK_CHECK(STATE, TASK_INDEX, HANDS);
		WORK_STEP	*LAST;
		WORK_FIX	FIX;
		const char	*CAUSE;

		/* something to take away that was never there is not done: it is
		 * missing ("remove notes" when only notes.txt is there) */
		if (
			MET == 1 &&
			WORK_GOAL_IS_GONE(STATE, TASK->ACTION) &&
			!TASK_EVER_WORKED(TASK)
		)
		{
			if (!TASK->STEP_COUNT)
				WORK_ADD_STEP(
					STATE, TASK_INDEX, WORK_STEP_MAIN, TASK->ACTION,
					TASK->ARGUMENT, "no such file", 0
				);

			MET = 0;
		}

		if (MET == 1)
		{
			TASK->STATE = WORK_DONE;
			TASK->DONE_AT = NOW_SECONDS();
			return (TASK->STATE);
		}

		/* nothing done yet: the action itself */
		if (!TASK->STEP_COUNT)
		{
			RUN_STEP(
				STATE, TASK_INDEX, HANDS, WORK_STEP_MAIN, TASK->ACTION,
				TASK->ARGUMENT, NULL
			);
			continue ;
		}

		/* the last step that did something (looking around changes
		 * nothing) */
		{
			int	LAST_INDEX = TASK->STEP_COUNT - 1;

			while (
				LAST_INDEX > 0 &&
				TASK->STEPS[LAST_INDEX].KIND == WORK_STEP_LOOK
			)
				LAST_INDEX--;

			LAST = &TASK->STEPS[LAST_INDEX];
		}

		/* no way to look: the action's word is taken */
		if (MET < 0 && LAST->WORKED == 1)
		{
			TASK->STATE = WORK_DONE;
			TASK->DONE_AT = NOW_SECONDS();
			return (TASK->STATE);
		}

		/* a fix worked but the task isn't done yet: the action again */
		if ((LAST->KIND == WORK_STEP_FIX) && LAST->WORKED == 1)
		{
			RUN_STEP(
				STATE, TASK_INDEX, HANDS, WORK_STEP_RETRY, TASK->ACTION,
				TASK->ARGUMENT, NULL
			);
			continue ;
		}

		CAUSE = CURRENT_CAUSE(TASK);

		/* the action said it worked, yet the check says otherwise */
		if (MET == 0 && LAST->WORKED == 1 && LAST->KIND != WORK_STEP_FIX)
			snprintf(TASK->CAUSE, sizeof TASK->CAUSE, "still not done");
		else
			snprintf(TASK->CAUSE, sizeof TASK->CAUSE, "%s", CAUSE);

		CAUSE_WORDS(
			STATE, TASK, HANDS, TASK->CAUSE, TASK->CAUSE_SAID,
			sizeof TASK->CAUSE_SAID
		);

		SHOW_LINE(HANDS, "cause: %s", TASK->CAUSE);

		if (!NEXT_FIX(STATE, TASK, HANDS, TASK->CAUSE, &FIX))
		{
			SHOW_LINE(HANDS, "no fix left for it");
			TASK->STATE = WORK_STUCK;
			return (TASK->STATE);
		}

		MARK_TRIED(TASK, FIX.RAW);

		switch (FIX.KIND)
		{
			case FIX_DONE:
			{
				/* the goal is there another way ("it was already there"),
				 * but only when the check agrees: a file called welcome.txt
				 * is no folder welcome.txt */
				if (MET == 0)
				{
					SHOW_LINE(
						HANDS, "done? the check says no, so on to the next"
					);
					break ;
				}

				/* the goal is there another way ("it was already there") */
				WORK_ADD_STEP(
					STATE, TASK_INDEX, WORK_STEP_FIX, "done", "", "done", 1
				);
				snprintf(
					TASK->STEPS[TASK->STEP_COUNT - 1].SAID,
					sizeof TASK->STEPS[0].SAID, "%s", FIX.SAID
				);
				TASK->STATE = WORK_DONE;
				TASK->DONE_AT = NOW_SECONDS();
				return (TASK->STATE);
			}
			case FIX_FIND:
			{
				char	FOUND[600];
				char	FIRST[600];

				ARGUMENT_PART(TASK->ARGUMENT, 1, FIRST, sizeof FIRST);

				{
					char	NAME[300];

					WORK_LAST_NAME(FIRST, NAME, sizeof NAME);
					NOTE_ATTEMPT(
						TASK, HANDS, "work attempt find",
						"looked for {1} in all your folders", NAME
					);
				}

				if (
					!HANDS->FIND ||
					!HANDS->FIND(HANDS->ENGINE, FIRST, FOUND, sizeof FOUND) ||
					!strcmp(FOUND, FIRST)
				)
					break ;

				{
					char	NEW_ARGUMENT[1300];
					char	SECOND[600];
					char	FOUND_STEP[1400];

					ARGUMENT_PART(TASK->ARGUMENT, 2, SECOND, sizeof SECOND);

					if (SECOND[0])
						snprintf(
							NEW_ARGUMENT, sizeof NEW_ARGUMENT, "%s\t%s", FOUND,
							SECOND
						);
					else
						snprintf(
							NEW_ARGUMENT, sizeof NEW_ARGUMENT, "%s", FOUND
						);

					/* the one found was offered and the user said no */
					snprintf(
						FOUND_STEP, sizeof FOUND_STEP, "%s %s", TASK->ACTION,
						NEW_ARGUMENT
					);

					if (IS_DECLINED(TASK, FOUND_STEP))
						break ;

					/* looking needs no yes; changing another thing does */
					if (IS_READ_ONLY(STATE, TASK->ACTION))
					{
						snprintf(
							TASK->ARGUMENT, sizeof TASK->ARGUMENT, "%s",
							NEW_ARGUMENT
						);
						RUN_STEP(
							STATE, TASK_INDEX, HANDS, WORK_STEP_FOUND,
							TASK->ACTION, TASK->ARGUMENT, NULL
						);
						break ;
					}

					snprintf(
						TASK->ASKING_STEP, sizeof TASK->ASKING_STEP, "%s %s",
						TASK->ACTION, NEW_ARGUMENT
					);
					TASK->ASKING_KIND = FIX_FIND;
					snprintf(
						TASK->ASKING_SAID, sizeof TASK->ASKING_SAID, "%s", FOUND
					);
					TASK->STATE = WORK_ASKING;
					STATE->LAST_ASKED_TURN = STATE->TURN;
					return (TASK->STATE);
				}
			}
			case FIX_ASK:
			{
				snprintf(
					TASK->ASKING_STEP, sizeof TASK->ASKING_STEP, "%s", FIX.STEP
				);
				snprintf(
					TASK->ASKING_SAID, sizeof TASK->ASKING_SAID, "%s",
					FIX.SAID[0] ? FIX.SAID : FIX.STEP
				);
				TASK->ASKING_KIND = FIX_ASK;
				TASK->STATE = WORK_ASKING;
				STATE->LAST_ASKED_TURN = STATE->TURN;
				SHOW_LINE(HANDS, "needs a yes first: %s", FIX.STEP);
				return (TASK->STATE);
			}
			case FIX_RETRY:
			{
				RUN_STEP(
					STATE, TASK_INDEX, HANDS, WORK_STEP_RETRY, TASK->ACTION,
					TASK->ARGUMENT, NULL
				);
				NOTE_ATTEMPT(
					TASK, HANDS, "work attempt again", "tried it again", NULL
				);
				break ;
			}
			default:
			{
				char	ACTION[48];
				char	ARGUMENT[700];

				SPLIT_STEP(
					FIX.STEP, ACTION, sizeof ACTION, ARGUMENT, sizeof ARGUMENT
				);

				/* a step that could lose something waits for a yes, even
				 * when it was written as a try */
				if (IS_RISKY(STATE, ACTION))
				{
					snprintf(
						TASK->ASKING_STEP, sizeof TASK->ASKING_STEP, "%s",
						FIX.STEP
					);
					snprintf(
						TASK->ASKING_SAID, sizeof TASK->ASKING_SAID, "%s",
						FIX.SAID[0] ? FIX.SAID : FIX.STEP
					);
					TASK->ASKING_KIND = FIX_ASK;
					TASK->STATE = WORK_ASKING;
					STATE->LAST_ASKED_TURN = STATE->TURN;
					return (TASK->STATE);
				}

				RUN_STEP(
					STATE, TASK_INDEX, HANDS, WORK_STEP_FIX, ACTION, ARGUMENT,
					FIX.SAID
				);
				NOTE_ATTEMPT(
					TASK, HANDS, "work attempt fix", "tried to {1}",
					FIX.SAID[0] ? FIX.SAID : FIX.STEP
				);
			}
		}
	}

	TASK->STATE = WORK_STUCK;

	return (TASK->STATE);
}

/* the user's yes or no to the fix Lucy asked about; the new state */
int
	WORK_ANSWER_ASKING(
		WORK_STATE *STATE, int TASK_INDEX, int SAYS_YES, const WORK_HANDS *HANDS
	)
{
	WORK_TASK	*TASK = &STATE->TASKS[TASK_INDEX];
	char		ACTION[48];
	char		ARGUMENT[700];

	if (TASK->STATE != WORK_ASKING)
		return (TASK->STATE);

	TASK->TOUCHED = STATE->TURN;

	if (!SAYS_YES)
	{
		/* said no to: not offered again, unless it is wanted after all */
		ADD_DECLINED(TASK, TASK->ASKING_STEP);
		snprintf(
			TASK->LAST_DECLINED_STEP, sizeof TASK->LAST_DECLINED_STEP, "%s",
			TASK->ASKING_STEP
		);
		snprintf(
			TASK->LAST_DECLINED_SAID, sizeof TASK->LAST_DECLINED_SAID, "%s",
			TASK->ASKING_SAID
		);
		TASK->LAST_DECLINED_KIND = TASK->ASKING_KIND;
		TASK->LAST_DECLINED_TURN = STATE->TURN;

		if (TASK->ASKING_KIND == FIX_ASK)
			NOTE_ATTEMPT(
				TASK, HANDS, "work attempt offered", "offered to {1}",
				TASK->ASKING_SAID
			);

		/* "no, not that one" about another thing found: the task itself
		 * stays done */
		if (TASK->ASKING_DOUBT)
		{
			TASK->ASKING_DOUBT = 0;
			TASK->ASKING_STEP[0] = 0;
			TASK->STATE = WORK_DONE;
			return (TASK->STATE);
		}

		/* "no" to a guess at a name like it: still not done, not called off */
		if (TASK->ASKING_WIDER)
		{
			TASK->ASKING_WIDER = 0;
			TASK->ASKING_STEP[0] = 0;
			TASK->STATE = WORK_STUCK;
			return (TASK->STATE);
		}

		TASK->STATE = WORK_DROPPED;
		TASK->ASKING_STEP[0] = 0;
		return (TASK->STATE);
	}

	TASK->ASKING_WIDER = 0;

	SPLIT_STEP(
		TASK->ASKING_STEP, ACTION, sizeof ACTION, ARGUMENT, sizeof ARGUMENT
	);

	if (TASK->ASKING_KIND == FIX_FIND)
	{
		/* yes, that one: the task is about the thing found now */
		snprintf(TASK->ARGUMENT, sizeof TASK->ARGUMENT, "%s", ARGUMENT);
		ARGUMENT_PART(ARGUMENT, 1, TASK->THING, sizeof TASK->THING);
		RUN_STEP(
			STATE, TASK_INDEX, HANDS, WORK_STEP_FOUND, ACTION, ARGUMENT, NULL
		);
	}
	else
		RUN_STEP(
			STATE, TASK_INDEX, HANDS, WORK_STEP_FIX, ACTION, ARGUMENT,
			TASK->ASKING_SAID
		);

	TASK->ASKING_STEP[0] = 0;

	return (WORK_SOLVE(STATE, TASK_INDEX, HANDS));
}

/* ---------- when the skill's fixes are used up ---------- */

static void	STEP_SENTENCE(
	const WORK_HANDS *HANDS, const WORK_STEP *STEP, char *OUTPUT,
	int OUTPUT_SIZE
);

/* adds SENTENCE to the end of OUTPUT, with a space */
static void
	ADD_SENTENCE(char *OUTPUT, int OUTPUT_SIZE, const char *SENTENCE)
{
	size_t	LENGTH = strlen(OUTPUT);

	if (!SENTENCE[0] || LENGTH + strlen(SENTENCE) + 2 >= (size_t)OUTPUT_SIZE)
		return ;

	if (LENGTH)
		strcat(OUTPUT, " ");

	strcat(OUTPUT, SENTENCE);
	CAPITALIZE(OUTPUT + (LENGTH ? LENGTH + 1 : 0));
}

/* "no such file", "not found": the thing itself is missing */
static int
	CAUSE_IS_MISSING(const char *CAUSE)
{
	char	LOWER_CAUSE[300];

	snprintf(LOWER_CAUSE, sizeof LOWER_CAUSE, "%s", CAUSE);
	LOWER(LOWER_CAUSE);

	return (
		strstr(LOWER_CAUSE, "no such") ||
		strstr(LOWER_CAUSE, "not found") ||
		strstr(LOWER_CAUSE, "missing") ||
		strstr(LOWER_CAUSE, "doesn't exist") ||
		strstr(LOWER_CAUSE, "does not exist")
	);
}

/* the user says "try something else" to a task whose fixes are used up (or
 * said no to the one left). Lucy goes on with what works for any skill, a
 * step further each time, and says something new each time:
 *
 *   1. the action once more: things may have changed since;
 *   2. a wider look: things with a name like it ("Ghost.txt", "ghost2");
 *   3. what she lacks, asked of the user, with all she tried;
 *   4. plainly, once, that she knows no other way and can be taught one;
 *   5. after that only a short line, not the whole story again.
 *
 * 1 when the reply is in TASK->LADDER_SAID, 0 when the action worked out
 * differently this time and WORK_STORY tells it. */
int
	WORK_PUSH(WORK_STATE *STATE, int TASK_INDEX, const WORK_HANDS *HANDS)
{
	WORK_TASK	*TASK = &STATE->TASKS[TASK_INDEX];
	char		NAME[300];
	char		DOING[400];
	char		FIRST[600];
	char		SECOND[600];
	char		ONE[1200];
	char		*SAID = TASK->LADDER_SAID;
	int			SAID_SIZE = (int)sizeof TASK->LADDER_SAID;

	SAID[0] = 0;
	TASK->LADDER_TURN = STATE->TURN;
	TASK->TOUCHED = STATE->TURN;
	STATE->FOCUS = TASK_INDEX;
	WORK_LAST_NAME(TASK->THING, NAME, sizeof NAME);
	WORK_DOING_SAID(STATE, TASK->ACTION, TASK->ARGUMENT, DOING, sizeof DOING);
	ARGUMENT_PART(TASK->ARGUMENT, 1, FIRST, sizeof FIRST);
	ARGUMENT_PART(TASK->ARGUMENT, 2, SECOND, sizeof SECOND);

	/* 1. once more */
	if (TASK->LADDER < 1)
	{
		TASK->LADDER = 1;

		if (!IS_RISKY(STATE, TASK->ACTION))
		{
			int	FROM = TASK->STEP_COUNT;

			SHOW_LINE(HANDS, "push: the action once more");
			RUN_STEP(
				STATE, TASK_INDEX, HANDS, WORK_STEP_RETRY, TASK->ACTION,
				TASK->ARGUMENT, NULL
			);
			NOTE_ATTEMPT(
				TASK, HANDS, "work attempt again", "tried it again", NULL
			);
			WORK_SOLVE(STATE, TASK_INDEX, HANDS);

			/* it went another way this time: the story tells it */
			if (TASK->STATE != WORK_STUCK)
				return (0);

			/* the cause in the skill's words, else its sentence for it */
			if (TASK->CAUSE_SAID[0])
				snprintf(ONE, sizeof ONE, "%s", TASK->CAUSE_SAID);
			else
				STEP_SENTENCE(HANDS, &TASK->STEPS[FROM], ONE, sizeof ONE);

			DROP_END(ONE);
			LOWER_FIRST(ONE);
			SAY(HANDS, "work push again", "I tried it again, but {1}.", ONE,
				NULL, NULL, ONE, sizeof ONE);
			ADD_SENTENCE(SAID, SAID_SIZE, ONE);
		}
	}

	/* 2. a wider look for the thing, when it is the thing that is missing */
	if (TASK->LADDER < 2 && !CAUSE_IS_MISSING(TASK->CAUSE))
		TASK->LADDER = 2;

	if (TASK->LADDER < 2)
	{
		char	LIKE[2000] = "";
		char	*LINE;
		char	*SAVED;

		TASK->LADDER = 2;
		SHOW_LINE(HANDS, "push: names like %s", NAME);
		NOTE_ATTEMPT(
			TASK, HANDS, "work attempt like", "looked for names like {1}", NAME
		);

		if (HANDS->FIND_LIKE && NAME[0])
			HANDS->FIND_LIKE(HANDS->ENGINE, NAME, LIKE, sizeof LIKE);

		for (
			LINE = strtok_r(LIKE, "\n", &SAVED);
			LINE;
			LINE = strtok_r(NULL, "\n", &SAVED)
		)
		{
			char	STEP[1400];
			char	FOUND_DOING[400];
			char	ARGUMENT[1300];

			if (!LINE[0] || !strcmp(LINE, FIRST))
				continue ;

			if (SECOND[0])
				snprintf(ARGUMENT, sizeof ARGUMENT, "%s\t%s", LINE, SECOND);
			else
				snprintf(ARGUMENT, sizeof ARGUMENT, "%s", LINE);

			snprintf(STEP, sizeof STEP, "%s %s", TASK->ACTION, ARGUMENT);

			if (IS_DECLINED(TASK, STEP))
				continue ;

			/* maybe that is the one: asked, not done */
			SHOW_LINE(HANDS, "found like it: %s", LINE);
			snprintf(TASK->ASKING_STEP, sizeof TASK->ASKING_STEP, "%s", STEP);
			snprintf(TASK->ASKING_SAID, sizeof TASK->ASKING_SAID, "%s", LINE);
			TASK->ASKING_KIND = FIX_FIND;
			TASK->ASKING_WIDER = 1;
			TASK->ASKING_DOUBT = 0;
			TASK->STATE = WORK_ASKING;
			STATE->LAST_ASKED_TURN = STATE->TURN;
			WORK_DOING_SAID(
				STATE, TASK->ACTION, ARGUMENT, FOUND_DOING, sizeof FOUND_DOING
			);
			SAY(HANDS, "work push like",
				"I looked for names like {1} too, and found {2}. Is that the "
				"one you mean? Should I {3}?",
				NAME, LINE, FOUND_DOING, ONE, sizeof ONE);
			ADD_SENTENCE(SAID, SAID_SIZE, ONE);
			return (1);
		}

		SAY(HANDS, "work push like none",
			"I also looked for anything with a name like {1}, and there is "
			"nothing like it.",
			NAME, NULL, NULL, ONE, sizeof ONE);
		ADD_SENTENCE(SAID, SAID_SIZE, ONE);
	}

	/* 3. ask for what she lacks: where it is, when it is missing */
	if (TASK->LADDER < 3)
	{
		TASK->LADDER = 3;

		if (CAUSE_IS_MISSING(TASK->CAUSE))
			SAY(HANDS, "work push ask",
				"I don't know another way to {1}. Do you know where it is, or "
				"what else I could try?",
				DOING, NULL, NULL, ONE, sizeof ONE);
		else
			SAY(HANDS, "work push ask other",
				"I don't know another way to {1}. Do you know what else I "
				"could "
				"try?",
				DOING, NULL, NULL, ONE, sizeof ONE);

		ADD_SENTENCE(SAID, SAID_SIZE, ONE);
		return (1);
	}

	/* 4. plainly, once */
	if (TASK->LADDER < 4)
	{
		char	TRIED[1200];

		TASK->LADDER = 4;
		ATTEMPTS_SAID(TASK, TRIED, sizeof TRIED);
		SAY(HANDS, "work push none left",
			"I'd only repeat myself now: to {1}, I {2}, and that is every way "
			"I know. Tell me what to try, or teach me a way, and I'll do it.",
			DOING, TRIED[0] ? TRIED : "tried what I could", NULL, ONE,
			sizeof ONE);
		ADD_SENTENCE(SAID, SAID_SIZE, ONE);
		return (1);
	}

	/* 5. after that, short */
	TASK->LADDER++;
	SAY(HANDS, "work push nothing new",
		"Nothing new I can try for that, sorry. Tell me what to try, and I'll "
		"do it.",
		NULL, NULL, NULL, ONE, sizeof ONE);
	ADD_SENTENCE(SAID, SAID_SIZE, ONE);

	return (1);
}

/* "yes, do it after all" right after saying no to a fix: it is done now */
int
	WORK_YES_AFTER_ALL(
		WORK_STATE *STATE, int TASK_INDEX, const WORK_HANDS *HANDS
	)
{
	WORK_TASK	*TASK = &STATE->TASKS[TASK_INDEX];

	if (
		!TASK->LAST_DECLINED_STEP[0] ||
		TASK->LAST_DECLINED_TURN < STATE->TURN - 6
	)
		return (0);

	REMOVE_DECLINED(TASK, TASK->LAST_DECLINED_STEP);
	snprintf(
		TASK->ASKING_STEP, sizeof TASK->ASKING_STEP, "%s",
		TASK->LAST_DECLINED_STEP
	);
	snprintf(
		TASK->ASKING_SAID, sizeof TASK->ASKING_SAID, "%s",
		TASK->LAST_DECLINED_SAID
	);
	TASK->ASKING_KIND = TASK->LAST_DECLINED_KIND;
	TASK->LAST_DECLINED_STEP[0] = 0;
	TASK->STATE = WORK_ASKING;
	WORK_ANSWER_ASKING(STATE, TASK_INDEX, 1, HANDS);

	return (1);
}

/* ---------- when the user says it isn't done ---------- */

static void	SAY(
	const WORK_HANDS *HANDS, const char *KEY, const char *DEFAULT,
	const char *FIRST, const char *SECOND, const char *THIRD, char *OUTPUT,
	int OUTPUT_SIZE
);

/* the user says the task isn't done while the check says it is.  That is a
 * problem like any other, with the cause "user disagrees": the skill's
 * fixes for it are tried one at a time, one more each time the user says
 * so, the ones about what the user named first ("it's not in the trash"
 * looks in the trash).  1 when something new was looked at or found
 * (WORK_DOUBT_STORY tells it), 0 when nothing is left to try. */
int
	WORK_DOUBT(
		WORK_STATE *STATE, int TASK_INDEX, const char *PLAIN,
		const WORK_HANDS *HANDS
	)
{
	WORK_TASK	*TASK = &STATE->TASKS[TASK_INDEX];
	int			ROUND;

	TASK->PUSHBACKS++;
	TASK->TOUCHED = STATE->TURN;
	STATE->FOCUS = TASK_INDEX;

	for (ROUND = 0; ROUND < 8; ROUND++)
	{
		WORK_FIX	FIX;
		char		ACTION[48];
		char		ARGUMENT[700];

		if (!NEXT_FIX_FOR(STATE, TASK, HANDS, "user disagrees", PLAIN, &FIX))
		{
			SHOW_LINE(HANDS, "doubt: nothing left to look at");
			TASK->DOUBT_ROUNDS++;
			return (0);
		}

		MARK_TRIED(TASK, FIX.RAW);
		SHOW_LINE(HANDS, "doubt: %s", FIX.STEP[0] ? FIX.STEP : FIX.RAW);

		switch (FIX.KIND)
		{
			case FIX_LOOK:
			{
				char	RESULT[2000];
				int		CODE;

				SPLIT_STEP(
					FIX.STEP, ACTION, sizeof ACTION, ARGUMENT, sizeof ARGUMENT
				);

				/* only what changes nothing is done without asking */
				if (!IS_READ_ONLY(STATE, ACTION))
					continue ;

				RESULT[0] = 0;
				CODE = HANDS->DO(
					HANDS->ENGINE, ACTION, ARGUMENT, RESULT, sizeof RESULT
				);
				SHOW_LINE(
					HANDS, "look: %s %s: %.120s", ACTION, ARGUMENT, RESULT
				);
				WORK_ADD_STEP(
					STATE, TASK_INDEX, WORK_STEP_LOOK, ACTION, ARGUMENT, RESULT,
					CODE == 0 ? 2 : 0
				);
				snprintf(
					TASK->STEPS[TASK->STEP_COUNT - 1].SAID,
					sizeof TASK->STEPS[0].SAID, "%s", FIX.SAID
				);
				return (1);
			}
			case FIX_FIND:
			{
				char	FIRST[600];
				char	NAME[300];
				char	FOUND[600];
				char	SECOND[600];

				ARGUMENT_PART(TASK->ARGUMENT, 1, FIRST, sizeof FIRST);
				WORK_LAST_NAME(FIRST, NAME, sizeof NAME);

				if (
					!HANDS->FIND ||
					!HANDS->FIND(HANDS->ENGINE, NAME, FOUND, sizeof FOUND) ||
					!strcmp(FOUND, FIRST)
				)
				{
					/* nothing else by that name: worth saying, then on */
					SHOW_LINE(HANDS, "find: nothing else called %s", NAME);
					WORK_ADD_STEP(
						STATE, TASK_INDEX, WORK_STEP_LOOK, "find", NAME, "none",
						2
					);
					continue ;
				}

				/* another one with that name: maybe the one they mean */
				ARGUMENT_PART(TASK->ARGUMENT, 2, SECOND, sizeof SECOND);

				if (SECOND[0])
					snprintf(
						TASK->ASKING_STEP, sizeof TASK->ASKING_STEP,
						"%s %s\t%s", TASK->ACTION, FOUND, SECOND
					);
				else
					snprintf(
						TASK->ASKING_STEP, sizeof TASK->ASKING_STEP, "%s %s",
						TASK->ACTION, FOUND
					);

				/* the user said no to that one already */
				if (IS_DECLINED(TASK, TASK->ASKING_STEP))
				{
					TASK->ASKING_STEP[0] = 0;
					continue ;
				}

				snprintf(
					TASK->ASKING_SAID, sizeof TASK->ASKING_SAID, "%s", FOUND
				);
				TASK->ASKING_KIND = FIX_FIND;
				TASK->ASKING_DOUBT = 1;
				TASK->STATE = WORK_ASKING;
				STATE->LAST_ASKED_TURN = STATE->TURN;
				SHOW_LINE(HANDS, "found: %s", FOUND);
				return (1);
			}
			case FIX_ASK:
			{
				snprintf(
					TASK->ASKING_STEP, sizeof TASK->ASKING_STEP, "%s", FIX.STEP
				);
				snprintf(
					TASK->ASKING_SAID, sizeof TASK->ASKING_SAID, "%s",
					FIX.SAID[0] ? FIX.SAID : FIX.STEP
				);
				TASK->ASKING_KIND = FIX_ASK;
				TASK->ASKING_DOUBT = 1;
				TASK->STATE = WORK_ASKING;
				STATE->LAST_ASKED_TURN = STATE->TURN;
				SHOW_LINE(HANDS, "needs a yes first: %s", FIX.STEP);
				return (1);
			}
			default:
			{
				continue ;
			}
		}
	}

	return (0);
}

/* what looking around showed since FROM_STEP, and the question about
 * another thing found: "I looked in the trash: 42, 42 (2)." */
void
	WORK_DOUBT_STORY(
		WORK_STATE *STATE, int TASK_INDEX, int FROM_STEP,
		const WORK_HANDS *HANDS, char *OUTPUT, int OUTPUT_SIZE
	)
{
	WORK_TASK	*TASK = &STATE->TASKS[TASK_INDEX];
	int			POSITION = 0;
	int			INDEX;
	char		NAME[300];

	if (OUTPUT_SIZE)
		OUTPUT[0] = 0;

	WORK_LAST_NAME(TASK->THING, NAME, sizeof NAME);

	for (
		INDEX = FROM_STEP < 0 ? 0 : FROM_STEP;
		INDEX < TASK->STEP_COUNT;
		INDEX++
	)
	{
		const WORK_STEP	*STEP = &TASK->STEPS[INDEX];
		char			ONE[1400];
		char			LOWER_RESULT[300];

		/* what was looked at for this message */
		if (STEP->KIND != WORK_STEP_LOOK || STEP->TURN != STATE->TURN)
			continue ;

		snprintf(LOWER_RESULT, sizeof LOWER_RESULT, "%s", STEP->RESULT);
		LOWER(LOWER_RESULT);

		if (!strcmp(STEP->ACTION, "find"))
			SAY(HANDS, "work searched none",
				"I searched all your folders, and "
				"there is no other {1}.",
				STEP->ARGUMENT, NULL, NULL, ONE, sizeof ONE);
		else if (STEP->WORKED != 2)
			SAY(HANDS, "work looked failed",
				"I tried to look in {1}, but I "
				"couldn't ({2}).",
				STEP->SAID[0] ? STEP->SAID : STEP->ARGUMENT, STEP->RESULT, NULL,
				ONE, sizeof ONE);
		else if (
			!LOWER_RESULT[0] ||
			!strncmp(LOWER_RESULT, "no ", 3) ||
			!strcmp(LOWER_RESULT, "empty")
		)
			SAY(HANDS, "work looked empty", "I looked in {1}: it's empty.",
				STEP->SAID[0] ? STEP->SAID : STEP->ARGUMENT, NULL, NULL, ONE,
				sizeof ONE);
		else
			SAY(HANDS, "work looked",
				"I looked in {1}, and this is what is "
				"there: {2}.",
				STEP->SAID[0] ? STEP->SAID : STEP->ARGUMENT, STEP->RESULT, NULL,
				ONE, sizeof ONE);

		CAPITALIZE(ONE);
		POSITION += snprintf(
			OUTPUT + POSITION, OUTPUT_SIZE - POSITION, "%s%s",
			POSITION ? " " : "", ONE
		);

		if (POSITION >= OUTPUT_SIZE)
			return ;
	}

	if (TASK->STATE == WORK_ASKING && TASK->ASKING_DOUBT)
	{
		char	ONE[1400];
		char	ACTION[48];
		char	ARGUMENT[700];
		char	DOING[400];

		SPLIT_STEP(
			TASK->ASKING_STEP, ACTION, sizeof ACTION, ARGUMENT, sizeof ARGUMENT
		);
		WORK_DOING_SAID(STATE, ACTION, ARGUMENT, DOING, sizeof DOING);

		if (TASK->ASKING_KIND == FIX_FIND)
			SAY(HANDS, "work doubt found",
				"But there is another {1}: {2}. Is "
				"that the one you mean? Should I {3}?",
				NAME, TASK->ASKING_SAID, DOING, ONE, sizeof ONE);
		else
			SAY(HANDS, "work doubt ask", "Should I {1}?", TASK->ASKING_SAID,
				NULL, NULL, ONE, sizeof ONE);

		CAPITALIZE(ONE);
		snprintf(
			OUTPUT + POSITION, OUTPUT_SIZE - POSITION, "%s%s",
			POSITION ? " " : "", ONE
		);
	}
}

/* ---------- saying what happened ---------- */

static void
	SAY(
		const WORK_HANDS *HANDS, const char *KEY, const char *DEFAULT,
		const char *FIRST, const char *SECOND, const char *THIRD, char *OUTPUT,
		int OUTPUT_SIZE
	)
{
	if (HANDS && HANDS->SAY)
	{
		HANDS->SAY(
			HANDS->ENGINE, KEY, DEFAULT, FIRST, SECOND, THIRD, OUTPUT,
			OUTPUT_SIZE
		);
		return ;
	}

	/* without the sayings: the default, filled */
	{
		const char	*DETAILS[3] = { FIRST, SECOND, THIRD };
		int			POSITION = 0;
		const char	*CURSOR;

		for (CURSOR = DEFAULT; *CURSOR && POSITION < OUTPUT_SIZE - 1; CURSOR++)
		{
			if (
				CURSOR[0] == '{' &&
				CURSOR[1] >= '1' &&
				CURSOR[1] <= '3' &&
				CURSOR[2] == '}'
			)
			{
				const char	*DETAIL = DETAILS[CURSOR[1] - '1'];

				POSITION += snprintf(
					OUTPUT + POSITION, OUTPUT_SIZE - POSITION, "%s",
					DETAIL ? DETAIL : ""
				);

				if (POSITION >= OUTPUT_SIZE)
					POSITION = OUTPUT_SIZE - 1;

				CURSOR += 2;
				continue ;
			}

			OUTPUT[POSITION++] = *CURSOR;
		}

		OUTPUT[POSITION] = 0;
	}
}

/* how doing the action is said: "make the folder 42" */
void
	WORK_DOING_SAID(
		const WORK_STATE *STATE, const char *ACTION, const char *ARGUMENT,
		char *OUTPUT, int OUTPUT_SIZE
	)
{
	const WORK_RULE	*RULE = FIND_RULE(STATE, ACTION, WORK_RULE_MEANS);
	WORK_TASK		SCRATCH;

	if (!RULE)
	{
		char	FIRST[600];

		ARGUMENT_PART(ARGUMENT, 1, FIRST, sizeof FIRST);
		snprintf(OUTPUT, OUTPUT_SIZE, "%s %s", ACTION, FIRST);
		return ;
	}

	memset(&SCRATCH, 0, sizeof SCRATCH);
	snprintf(SCRATCH.ARGUMENT, sizeof SCRATCH.ARGUMENT, "%s", ARGUMENT);
	EXPAND_TEXT(
		(WORK_STATE *)STATE, &SCRATCH, NULL, RULE->TEXT, OUTPUT, OUTPUT_SIZE
	);
}

static void
	STEP_SENTENCE(
		const WORK_HANDS *HANDS, const WORK_STEP *STEP, char *OUTPUT,
		int OUTPUT_SIZE
	)
{
	if (OUTPUT_SIZE)
		OUTPUT[0] = 0;

	if (
		HANDS &&
		HANDS->SAY_RESULT &&
		HANDS->SAY_RESULT(
			HANDS->ENGINE, STEP->ACTION, STEP->ARGUMENT, STEP->RESULT,
			OUTPUT, OUTPUT_SIZE
		) &&
		OUTPUT[0]
	)
		return ;

	snprintf(OUTPUT, OUTPUT_SIZE, "%s.", STEP->RESULT);
	CAPITALIZE(OUTPUT);
}

/* "I tried to put it in the trash as "42 (2)", but that didn't work
 * either." for the fixes that failed since FROM_STEP */
static void
	TRIED_SENTENCE(
		const WORK_TASK *TASK, int FROM_STEP, const WORK_HANDS *HANDS,
		char *OUTPUT, int OUTPUT_SIZE
	)
{
	int	INDEX;
	int	POSITION = 0;

	if (OUTPUT_SIZE)
		OUTPUT[0] = 0;

	for (INDEX = FROM_STEP; INDEX < TASK->STEP_COUNT; INDEX++)
	{
		const WORK_STEP	*STEP = &TASK->STEPS[INDEX];
		char			ONE[600];

		if (STEP->KIND != WORK_STEP_FIX || STEP->WORKED == 1)
			continue ;

		SAY(HANDS, "work tried", "I tried to {1}, but that didn't work ({2}).",
			STEP->SAID[0] ? STEP->SAID : STEP->ACTION, STEP->RESULT, NULL, ONE,
			sizeof ONE);
		POSITION += snprintf(
			OUTPUT + POSITION, OUTPUT_SIZE - POSITION, "%s%s",
			POSITION ? " " : "", ONE
		);

		if (POSITION >= OUTPUT_SIZE)
			break ;
	}
}

/* what Lucy says about the task: what she did since FROM_STEP, why, and
 * how it stands; MODE is what the user's message was (WORK_TELL_...) */
void
	WORK_STORY(
		WORK_STATE *STATE, int TASK_INDEX, int FROM_STEP, int MODE,
		const WORK_HANDS *HANDS, char *OUTPUT, int OUTPUT_SIZE
	)
{
	WORK_TASK	*TASK = &STATE->TASKS[TASK_INDEX];
	char		CAUSE[400];
	char		TRIED[1200];
	char		DOING[400];
	char		NAME[300];
	int			INDEX;
	int			FIXES = 0;
	int			LAST_FIX = -1;
	int			LAST_MAIN = -1;
	int			DONE_FIX = -1;
	int			DID = 0;

	if (OUTPUT_SIZE)
		OUTPUT[0] = 0;

	if (FROM_STEP < 0)
		FROM_STEP = 0;

	/* the other thing found, when the user said it wasn't done */
	if (TASK->STATE == WORK_ASKING && TASK->ASKING_DOUBT)
	{
		WORK_DOUBT_STORY(
			STATE, TASK_INDEX, TASK->STEP_COUNT, HANDS, OUTPUT, OUTPUT_SIZE
		);
		return ;
	}

	WORK_LAST_NAME(TASK->THING, NAME, sizeof NAME);
	WORK_DOING_SAID(STATE, TASK->ACTION, TASK->ARGUMENT, DOING, sizeof DOING);

	for (INDEX = FROM_STEP; INDEX < TASK->STEP_COUNT; INDEX++)
	{
		const WORK_STEP	*STEP = &TASK->STEPS[INDEX];

		/* looking around changed nothing: WORK_DOUBT_STORY tells it */
		if (STEP->KIND == WORK_STEP_LOOK)
			continue ;

		DID++;

		if (STEP->KIND == WORK_STEP_FIX && !strcmp(STEP->ACTION, "done"))
			DONE_FIX = INDEX;
		else if (STEP->KIND == WORK_STEP_FIX)
		{
			FIXES++;

			if (STEP->WORKED == 1)
				LAST_FIX = INDEX;
		}
		else
			LAST_MAIN = INDEX;
	}

	/* the cause in words: the rule's, else the skill's sentence for it */
	snprintf(CAUSE, sizeof CAUSE, "%s", TASK->CAUSE_SAID);

	if (!CAUSE[0] && TASK->CAUSE[0])
	{
		WORK_STEP	FAKE;

		memset(&FAKE, 0, sizeof FAKE);
		snprintf(FAKE.ACTION, sizeof FAKE.ACTION, "%s", TASK->ACTION);
		snprintf(FAKE.ARGUMENT, sizeof FAKE.ARGUMENT, "%s", TASK->ARGUMENT);
		snprintf(FAKE.RESULT, sizeof FAKE.RESULT, "%s", TASK->CAUSE);

		if (!strcmp(TASK->CAUSE, "still not done"))
			SAY(HANDS, "work said done",
				"it said it worked, but {1} is still "
				"not done",
				NAME, NULL, NULL, CAUSE, sizeof CAUSE);
		else
			STEP_SENTENCE(HANDS, &FAKE, CAUSE, sizeof CAUSE);
	}

	DROP_END(CAUSE);
	TRIED_SENTENCE(TASK, FROM_STEP, HANDS, TRIED, sizeof TRIED);

	switch (TASK->STATE)
	{
		case WORK_DONE:
		{
			/* nothing was needed: it already was the way it should be */
			if (!DID)
			{
				char	WHERE[400] = "";
				char	THING[400];

				if (!strcmp(TASK->ACTION, "mkdir"))
					snprintf(THING, sizeof THING, "the folder %s", NAME);
				else
					snprintf(THING, sizeof THING, "%s", NAME);

				if (WORK_GOAL_IS_GONE(STATE, TASK->ACTION))
				{
					/* where it went: the trash, maybe under another name,
					 * said only when it is still there */
					char	IN_TRASH[700];
					char	LOOKED[200] = "";

					snprintf(
						IN_TRASH, sizeof IN_TRASH, "trash/%s",
						TASK->FREE_NAME[0] ? TASK->FREE_NAME : NAME
					);

					if (!strcmp(TASK->ACTION, "trash") && HANDS && HANDS->DO)
					{
						HANDS->DO(
							HANDS->ENGINE, "exists", IN_TRASH, LOOKED,
							sizeof LOOKED
						);
						LOWER(LOOKED);
					}

					if (!strcmp(TASK->ACTION, "trash") && !strcmp(LOOKED, "no"))
						SAY(HANDS, "work where gone for good",
							" It's not in the trash either: it's gone for "
							"good.",
							NULL, NULL, NULL, WHERE, sizeof WHERE);
					else if (
						!strcmp(TASK->ACTION, "trash") &&
						TASK->FREE_NAME[0]
					)
						SAY(HANDS, "work where trash as",
							" It's in the trash as \"{1}\".", TASK->FREE_NAME,
							NULL, NULL, WHERE, sizeof WHERE);
					else if (!strcmp(TASK->ACTION, "trash"))
						SAY(HANDS, "work where trash", " It's in the trash.",
							NULL, NULL, NULL, WHERE, sizeof WHERE);

					if (WHERE[0] && WHERE[0] != ' ')
					{
						char	SPACED[400];

						snprintf(SPACED, sizeof SPACED, " %s", WHERE);
						snprintf(WHERE, sizeof WHERE, "%s", SPACED);
					}

					if (MODE == WORK_TELL_QUESTION)
						SAY(HANDS, "work gone answer",
							"No, {1} isn't in your files any more.{2}", NAME,
							WHERE, NULL, OUTPUT, OUTPUT_SIZE);
					else if (MODE == WORK_TELL_AGAIN)
						SAY(HANDS, "work gone already",
							"That's done already: {1} isn't in your files any "
							"more.{2}",
							NAME, WHERE, NULL, OUTPUT, OUTPUT_SIZE);
					else
						SAY(HANDS, "work checked gone",
							"I just checked: {1} isn't in your files any "
							"more.{2}",
							NAME, WHERE, NULL, OUTPUT, OUTPUT_SIZE);
				}
				else if (MODE == WORK_TELL_QUESTION)
					SAY(HANDS, "work there answer", "Yes, {1} is there.", THING,
						NULL, NULL, OUTPUT, OUTPUT_SIZE);
				else if (MODE == WORK_TELL_AGAIN)
					SAY(HANDS, "work there already", "{1} is already there.",
						THING, NULL, NULL, OUTPUT, OUTPUT_SIZE);
				else
					SAY(HANDS, "work checked there",
						"I just checked: {1} is there.", THING, NULL, NULL,
						OUTPUT, OUTPUT_SIZE);

				CAPITALIZE(OUTPUT);
				return ;
			}

			/* "the folder 42 is already there" */
			if (DONE_FIX >= 0)
			{
				char	SAID[400];

				if (TASK->STEPS[DONE_FIX].SAID[0])
				{
					snprintf(
						SAID, sizeof SAID, "%s", TASK->STEPS[DONE_FIX].SAID
					);
					DROP_END(SAID);
					CAPITALIZE(SAID);
					snprintf(OUTPUT, OUTPUT_SIZE, "%s.", SAID);
				}
				else if (LAST_MAIN >= 0)
					STEP_SENTENCE(
						HANDS, &TASK->STEPS[LAST_MAIN], OUTPUT, OUTPUT_SIZE
					);

				return ;
			}

			/* fixed on the way */
			if (LAST_FIX >= 0)
			{
				char	FIX_SAID[400];
				char	AFTER[600] = "";

				snprintf(
					FIX_SAID, sizeof FIX_SAID, "%s",
					TASK->STEPS[LAST_FIX].SAID[0] ? TASK->STEPS[LAST_FIX].SAID
					: TASK->STEPS[LAST_FIX].ACTION
				);
				DROP_END(FIX_SAID);
				LOWER_FIRST(FIX_SAID);
				PAST_TENSE(FIX_SAID, sizeof FIX_SAID);

				/* and then the action itself worked */
				if (
					LAST_MAIN > LAST_FIX &&
					TASK->STEPS[LAST_MAIN].KIND == WORK_STEP_RETRY
				)
					STEP_SENTENCE(
						HANDS, &TASK->STEPS[LAST_MAIN], AFTER, sizeof AFTER
					);

				if (CAUSE[0])
				{
					char	CAUSE_UPPER[400];

					snprintf(CAUSE_UPPER, sizeof CAUSE_UPPER, "%s", CAUSE);
					CAPITALIZE(CAUSE_UPPER);
					SAY(HANDS, "work fixed", "{1}, so I {2}.", CAUSE_UPPER,
						FIX_SAID, NULL, OUTPUT, OUTPUT_SIZE);
				}
				else

					snprintf(OUTPUT, OUTPUT_SIZE, "I %s.", FIX_SAID);

				if (
					AFTER[0] &&
					strlen(OUTPUT) + strlen(AFTER) + 2 < (size_t)OUTPUT_SIZE
				)
				{
					strcat(OUTPUT, " ");
					strcat(OUTPUT, AFTER);
				}

				/* the user had said it wasn't done: how it is now */
				if (
					MODE == WORK_TELL_COMPLAINT &&
					WORK_GOAL_IS_GONE(STATE, TASK->ACTION)
				)
				{
					char	CHECKED[400];

					SAY(HANDS, "work now gone",
						"Now {1} is gone from your files.", NAME, NULL, NULL,
						CHECKED, sizeof CHECKED);
					CAPITALIZE(CHECKED);

					if (
						strlen(OUTPUT) + strlen(CHECKED) + 2 <
							(size_t)OUTPUT_SIZE
					)
					{
						strcat(OUTPUT, " ");
						strcat(OUTPUT, CHECKED);
					}
				}

				return ;
			}

			if (LAST_MAIN >= 0)
				STEP_SENTENCE(
					HANDS, &TASK->STEPS[LAST_MAIN], OUTPUT, OUTPUT_SIZE
				);

			return ;
		}
		case WORK_ASKING:
		{
			char	CAUSE_UPPER[400];
			char	ASK[600];

			snprintf(CAUSE_UPPER, sizeof CAUSE_UPPER, "%s", CAUSE);
			CAPITALIZE(CAUSE_UPPER);

			if (TASK->ASKING_KIND == FIX_FIND)
			{
				char	FOUND_ACTION[48];
				char	FOUND_ARGUMENT[700];
				char	FOUND_DOING[400];

				SPLIT_STEP(
					TASK->ASKING_STEP, FOUND_ACTION, sizeof FOUND_ACTION,
					FOUND_ARGUMENT, sizeof FOUND_ARGUMENT
				);
				WORK_DOING_SAID(
					STATE, FOUND_ACTION, FOUND_ARGUMENT, FOUND_DOING,
					sizeof FOUND_DOING
				);
				SAY(HANDS, "work asking found",
					"There is no {1} in your files, but there is {2}. Should I "
					"{3}?",
					NAME, TASK->ASKING_SAID, FOUND_DOING, ASK, sizeof ASK);
			}
			else if (CAUSE_UPPER[0])
				SAY(HANDS, "work asking", "{1}. Should I {2}?", CAUSE_UPPER,
					TASK->ASKING_SAID, NULL, ASK, sizeof ASK);
			else
				SAY(HANDS, "work asking plain", "Should I {1}?",
					TASK->ASKING_SAID, NULL, NULL, ASK, sizeof ASK);

			/* "The trash has a 42. I tried ... . Should I ...?": what was
			 * tried comes before the question */
			if (TRIED[0])
			{
				char	*SHOULD = strstr(ASK, " Should I ");

				if (SHOULD)
				{
					char	QUESTION[600];

					snprintf(QUESTION, sizeof QUESTION, "%s", SHOULD + 1);
					*SHOULD = 0;
					snprintf(
						OUTPUT, OUTPUT_SIZE, "%s %s %s", ASK, TRIED, QUESTION
					);
					return ;
				}
			}

			snprintf(OUTPUT, OUTPUT_SIZE, "%s", ASK);
			return ;
		}
		case WORK_STUCK:
		{
			char	CAUSE_PART[400];

			snprintf(CAUSE_PART, sizeof CAUSE_PART, "%s", CAUSE);
			LOWER_FIRST(CAUSE_PART);

			if (CAUSE_PART[0])
				SAY(HANDS, "work stuck", "I couldn't {1}: {2}.", DOING,
					CAUSE_PART, NULL, OUTPUT, OUTPUT_SIZE);
			else
				SAY(HANDS, "work stuck plain", "I couldn't {1}.", DOING, NULL,
					NULL, OUTPUT, OUTPUT_SIZE);

			if (
				TRIED[0] &&
				strlen(OUTPUT) + strlen(TRIED) + 2 < (size_t)OUTPUT_SIZE
			)
			{
				strcat(OUTPUT, " ");
				strcat(OUTPUT, TRIED);
			}

			/* out of ideas only after something was tried */
			if (FIXES || TRIED[0])
			{
				char	ASK[300];

				SAY(HANDS, "work stuck ask",
					"I'm out of ideas. What would you like me to do?", NULL,
					NULL, NULL, ASK, sizeof ASK);

				if (strlen(OUTPUT) + strlen(ASK) + 2 < (size_t)OUTPUT_SIZE)
				{
					strcat(OUTPUT, " ");
					strcat(OUTPUT, ASK);
				}
			}

			return ;
		}
		case WORK_DROPPED:
		{
			SAY(HANDS, "work dropped",
				"Okay, I won't. I'll leave {1} as it is.", NAME, NULL, NULL,
				OUTPUT, OUTPUT_SIZE);
			return ;
		}
		case WORK_WAITING:
		{
			SAY(HANDS, "work waiting", "I'll {1} when the time comes.", DOING,
				NULL, NULL, OUTPUT, OUTPUT_SIZE);
			return ;
		}
		default:
		{
			if (LAST_MAIN >= 0)
				STEP_SENTENCE(
					HANDS, &TASK->STEPS[LAST_MAIN], OUTPUT, OUTPUT_SIZE
				);
		}
	}
}
