#include "SKILLS.h"
#include "BRAIN.h"
#include "FILE_SYSTEM.h"
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <windows.h>
#define LIBRARY_ENDING ".dll"

static void
	*OPEN_LIBRARY(const char *PATH)
{
	return ((void *)LoadLibraryA(PATH));
}

static void
	*FIND_SYMBOL(void *LIBRARY, const char *NAME)
{
	return ((void *)GetProcAddress((HMODULE)LIBRARY, NAME));
}

static void
	CLOSE_LIBRARY(void *LIBRARY)
{
	FreeLibrary((HMODULE)LIBRARY);
}

#else
#include <dlfcn.h>
#include <unistd.h>
#define LIBRARY_ENDING ".so"

static void
	*OPEN_LIBRARY(const char *PATH)
{
	return (dlopen(PATH, RTLD_NOW | RTLD_LOCAL));
}

static void
	*FIND_SYMBOL(void *LIBRARY, const char *NAME)
{
	return (dlsym(LIBRARY, NAME));
}

static void
	CLOSE_LIBRARY(void *LIBRARY)
{
	dlclose(LIBRARY);
}

#endif

/* size and time of the file: a different stamp means a new version */
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

static int
	HAS_LIBRARY_ENDING(const char *NAME)
{
	size_t	LENGTH = strlen(NAME);
	size_t	ENDING = strlen(LIBRARY_ENDING);

	return (LENGTH > ENDING && !strcmp(NAME + LENGTH - ENDING, LIBRARY_ENDING));
}

static void
	UNLOAD(LOADED_SKILL *SKILL)
{
	if (SKILL->LIBRARY)
		CLOSE_LIBRARY(SKILL->LIBRARY);

	SKILL->LIBRARY = NULL;
	SKILL->MEMORY = NULL;

	if (SKILL->LOADED_PATH[0])
		remove(SKILL->LOADED_PATH);

	SKILL->LOADED_PATH[0] = 0;
}

/* loads one skill from a copy of its file; 0 when it worked */
static int
	LOAD(SKILL_SET *SET, LOADED_SKILL *SKILL)
{
	char		COPY_FOLDER[600];
	SKILL_ENTRY	ENTRY;

	snprintf(COPY_FOLDER, sizeof COPY_FOLDER, "%s/loaded", SET->FOLDER);
	MAKE_DIRECTORY_PATH(COPY_FOLDER);
	snprintf(
		SKILL->LOADED_PATH, sizeof SKILL->LOADED_PATH, "%s/%d-%s", COPY_FOLDER,
		++SET->COPY_NUMBER, SKILL->NAME
	);

	if (COPY_FILE_CONTENTS(SKILL->PATH, SKILL->LOADED_PATH))
		snprintf(
			SKILL->LOADED_PATH, sizeof SKILL->LOADED_PATH, "%s", SKILL->PATH
		);

	SKILL->LIBRARY = OPEN_LIBRARY(SKILL->LOADED_PATH);

	if (!SKILL->LIBRARY)
	{
		UNLOAD(SKILL);
		return (-1);
	}

	ENTRY = (SKILL_ENTRY)FIND_SYMBOL(SKILL->LIBRARY, "__memory__");

	if (ENTRY)
		SKILL->MEMORY = ENTRY();
	else
		SKILL->MEMORY = NULL;

	/* a skill made for an older Lucy still works; a newer one waits */
	if (
		!SKILL->MEMORY ||
		SKILL->MEMORY->VERSION < 1 ||
		SKILL->MEMORY->VERSION > SKILL_VERSION
	)
	{
		UNLOAD(SKILL);
		return (-1);
	}

	return (0);
}

int
	SKILLS_OPEN(SKILL_SET *SET, const char *FOLDER)
{
	char		COPY_FOLDER[600];
	FILE_ENTRY	*ENTRIES = calloc(256, sizeof(FILE_ENTRY));
	int			COUNT;
	int			INDEX;

	memset(SET, 0, sizeof *SET);
	snprintf(SET->FOLDER, sizeof SET->FOLDER, "%s", FOLDER);
	SET->HOST.VERSION = SKILL_VERSION;
	MAKE_DIRECTORY_PATH(FOLDER);

	/* copies left from the last run are not needed */
	snprintf(COPY_FOLDER, sizeof COPY_FOLDER, "%s/loaded", FOLDER);

	if (ENTRIES)
	{
		COUNT = FILE_SYSTEM_LIST(COPY_FOLDER, ENTRIES, 256);

		for (INDEX = 0; INDEX < COUNT; INDEX++)
		{
			char	PATH[800];

			snprintf(
				PATH, sizeof PATH, "%s/%s", COPY_FOLDER, ENTRIES[INDEX].NAME
			);
			remove(PATH);
		}

		free(ENTRIES);
	}

	return (SKILLS_REFRESH(SET, NULL, 0));
}

/* loads every new or changed skill, forgets removed ones; NEWS says what
 * changed ("new skill: reason") */
int
	SKILLS_REFRESH(SKILL_SET *SET, char *NEWS, int NEWS_SIZE)
{
	FILE_ENTRY	*ENTRIES = calloc(256, sizeof(FILE_ENTRY));
	int			COUNT;
	int			INDEX;
	int			CHANGES = 0;
	int			POSITION = 0;
	int			SEEN[MAX_SKILLS];

	if (NEWS && NEWS_SIZE)
		NEWS[0] = 0;

	if (!ENTRIES)
		return (0);

	memset(SEEN, 0, sizeof SEEN);
	COUNT = FILE_SYSTEM_LIST(SET->FOLDER, ENTRIES, 256);

	for (INDEX = 0; INDEX < COUNT; INDEX++)
	{
		char		PATH[800];
		long long	STAMP;
		int			FOUND;

		if (
			ENTRIES[INDEX].IS_DIRECTORY ||
			!HAS_LIBRARY_ENDING(ENTRIES[INDEX].NAME)
		)
			continue ;

		snprintf(PATH, sizeof PATH, "%s/%s", SET->FOLDER, ENTRIES[INDEX].NAME);
		STAMP = FILE_STAMP(PATH);

		for (FOUND = 0; FOUND < SET->COUNT; FOUND++)
			if (!strcmp(SET->SKILLS[FOUND].NAME, ENTRIES[INDEX].NAME))
				break ;

		if (FOUND < SET->COUNT)
		{
			SEEN[FOUND] = 1;

			if (SET->SKILLS[FOUND].STAMP == STAMP && SET->SKILLS[FOUND].MEMORY)
				continue ;

			/* a new version of a skill already loaded */
			UNLOAD(&SET->SKILLS[FOUND]);
			SET->SKILLS[FOUND].STAMP = STAMP;

			if (!LOAD(SET, &SET->SKILLS[FOUND]))
			{
				CHANGES++;

				if (NEWS && POSITION < NEWS_SIZE - 80)
					POSITION += snprintf(
						NEWS + POSITION, NEWS_SIZE - POSITION,
						"%supdated skill: %s", POSITION ? ", " : "",
						SET->SKILLS[FOUND].MEMORY->NAME
					);
			}

			continue ;
		}

		if (SET->COUNT == MAX_SKILLS)
			continue ;

		{
			LOADED_SKILL	*SKILL = &SET->SKILLS[SET->COUNT];

			memset(SKILL, 0, sizeof *SKILL);
			snprintf(
				SKILL->NAME, sizeof SKILL->NAME, "%s", ENTRIES[INDEX].NAME
			);
			snprintf(SKILL->PATH, sizeof SKILL->PATH, "%s", PATH);
			SKILL->STAMP = STAMP;

			/* a file that is no skill is remembered, so it is not tried
			 * again until it changes */
			if (!LOAD(SET, SKILL))
			{
				CHANGES++;

				if (NEWS && POSITION < NEWS_SIZE - 80)
					POSITION += snprintf(
						NEWS + POSITION, NEWS_SIZE - POSITION,
						"%snew skill: %s", POSITION ? ", " : "",
						SKILL->MEMORY->NAME
					);
			}

			SEEN[SET->COUNT] = 1;
			SET->COUNT++;
		}
	}

	/* a skill whose file was taken away is forgotten */
	for (INDEX = SET->COUNT - 1; INDEX >= 0; INDEX--)
		if (!SEEN[INDEX])
		{
			if (SET->SKILLS[INDEX].MEMORY && NEWS && POSITION < NEWS_SIZE - 80)
				POSITION += snprintf(
					NEWS + POSITION, NEWS_SIZE - POSITION,
					"%sskill removed: %s", POSITION ? ", " : "",
					SET->SKILLS[INDEX].MEMORY->NAME
				);

			UNLOAD(&SET->SKILLS[INDEX]);
			SET->SKILLS[INDEX] = SET->SKILLS[SET->COUNT - 1];
			SET->COUNT--;
			CHANGES++;
		}

	free(ENTRIES);

	return (CHANGES);
}

void
	SKILLS_CLOSE(SKILL_SET *SET)
{
	int	INDEX;

	for (INDEX = 0; INDEX < SET->COUNT; INDEX++)
		UNLOAD(&SET->SKILLS[INDEX]);

	SET->COUNT = 0;
}

const SKILL_ACTION
	*SKILLS_FIND_ACTION(
		SKILL_SET *SET, const char *NAME, const SKILL_MEMORY **OWNER
	)
{
	int	SKILL;
	int	ACTION;

	for (SKILL = 0; SKILL < SET->COUNT; SKILL++)
	{
		const SKILL_MEMORY	*MEMORY = SET->SKILLS[SKILL].MEMORY;

		if (!MEMORY)
			continue ;

		for (ACTION = 0; ACTION < MEMORY->ACTION_COUNT; ACTION++)
			if (!strcmp(MEMORY->ACTIONS[ACTION].NAME, NAME))
			{
				if (OWNER)
					*OWNER = MEMORY;

				return (&MEMORY->ACTIONS[ACTION]);
			}
	}

	return (NULL);
}

/* runs an action of whichever skill has it: its return code, -1000 when no
 * skill has it */
int
	SKILLS_RUN(
		SKILL_SET *SET, const char *NAME, const char *ARGUMENT, char *RESULT,
		int RESULT_SIZE
	)
{
	const SKILL_ACTION	*ACTION = SKILLS_FIND_ACTION(SET, NAME, NULL);

	if (!ACTION || !ACTION->RUN)
		return (-1000);

	if (RESULT_SIZE)
		RESULT[0] = 0;

	return (
		ACTION->RUN(&SET->HOST, ARGUMENT ? ARGUMENT : "", RESULT, RESULT_SIZE)
	);
}

/* the first skill that answers the whole message by itself */
int
	SKILLS_ANSWER(
		SKILL_SET *SET, const char *MESSAGE, char *ACTION, int ACTION_SIZE,
		char *REPLY, int REPLY_SIZE
	)
{
	int	SKILL;

	for (SKILL = 0; SKILL < SET->COUNT; SKILL++)
	{
		const SKILL_MEMORY	*MEMORY = SET->SKILLS[SKILL].MEMORY;

		if (!MEMORY || !MEMORY->ANSWER)
			continue ;

		ACTION[0] = REPLY[0] = 0;

		if (
			MEMORY->ANSWER(
				&SET->HOST, MESSAGE, ACTION, ACTION_SIZE, REPLY, REPLY_SIZE
			) &&
			REPLY[0]
		)
			return (1);
	}

	return (0);
}

/* "{1}" -> the argument, "{R}" -> the result */
static void
	FILL_TEMPLATE(
		const char *TEMPLATE, int LENGTH, const char *ARGUMENT,
		const char *RESULT, char *OUTPUT, int OUTPUT_SIZE
	)
{
	int			POSITION = 0;
	int			INDEX;
	const char	*TAB = strchr(ARGUMENT, '\t');

	for (INDEX = 0; INDEX < LENGTH && POSITION < OUTPUT_SIZE - 1; INDEX++)
	{
		/* "{1}" is the argument (its first part when it has two, split by a
		 * tab), "{2}" the second part */
		if (!strncmp(TEMPLATE + INDEX, "{1}", 3))
		{
			POSITION += snprintf(
				OUTPUT + POSITION, OUTPUT_SIZE - POSITION, "%.*s",
				TAB ? (int)(TAB - ARGUMENT) : (int)strlen(ARGUMENT), ARGUMENT
			);
			INDEX += 2;
			continue ;
		}

		if (!strncmp(TEMPLATE + INDEX, "{2}", 3))
		{
			POSITION += snprintf(
				OUTPUT + POSITION, OUTPUT_SIZE - POSITION, "%s",
				TAB ? TAB + 1 : ""
			);
			INDEX += 2;
			continue ;
		}

		if (!strncmp(TEMPLATE + INDEX, "{R}", 3))
		{
			POSITION += snprintf(
				OUTPUT + POSITION, OUTPUT_SIZE - POSITION, "%s", RESULT
			);
			INDEX += 2;
			continue ;
		}

		OUTPUT[POSITION++] = TEMPLATE[INDEX];
	}

	if (POSITION >= OUTPUT_SIZE)
		POSITION = OUTPUT_SIZE - 1;

	OUTPUT[POSITION] = 0;
}

/* how the skill says a result: the line of SAYS whose key is the result,
 * else the "*" line, else 0 */
int
	SKILLS_SAY(
		const SKILL_ACTION *ACTION, const char *ARGUMENT, const char *RESULT,
		char *OUTPUT, int OUTPUT_SIZE
	)
{
	const char	*LINE;
	const char	*FALLBACK = NULL;
	int			FALLBACK_LENGTH = 0;

	if (!ACTION || !ACTION->SAYS)
		return (0);

	for (LINE = ACTION->SAYS; *LINE;)
	{
		const char	*END = strchr(LINE, '\n');
		const char	*COLON;
		int			LENGTH;

		if (!END)
			END = LINE + strlen(LINE);

		COLON = memchr(LINE, ':', (size_t)(END - LINE));
		LENGTH = (int)(END - LINE);

		if (COLON)
		{
			int			KEY_LENGTH = (int)(COLON - LINE);
			const char	*SENTENCE = COLON + 1;

			while (*SENTENCE == ' ')
				SENTENCE++;

			if (
				(int)strlen(RESULT) == KEY_LENGTH &&
				!strncasecmp(LINE, RESULT, (size_t)KEY_LENGTH)
			)
			{
				FILL_TEMPLATE(
					SENTENCE, (int)(END - SENTENCE), ARGUMENT, RESULT, OUTPUT,
					OUTPUT_SIZE
				);
				return (1);
			}

			if (KEY_LENGTH == 1 && LINE[0] == '*')
			{
				FALLBACK = SENTENCE;
				FALLBACK_LENGTH = (int)(END - SENTENCE);
			}
		}

		(void)LENGTH;

		if (*END)
			LINE = END + 1;
		else
			LINE = END;
	}

	if (FALLBACK)
	{
		FILL_TEMPLATE(
			FALLBACK, FALLBACK_LENGTH, ARGUMENT, RESULT, OUTPUT, OUTPUT_SIZE
		);
		return (1);
	}

	return (0);
}

/* the words of TEXT, lower case and as written, without the punctuation at
 * their ends */
static int
	SKILL_WORDS(
		const char *TEXT, char (*LOWER)[64], char (*SHOWN)[64], int LIMIT
	)
{
	int	COUNT = 0;

	while (*TEXT && COUNT < LIMIT)
	{
		int	LENGTH = 0;
		int	INDEX;

		while (*TEXT == ' ' || *TEXT == '\t' || *TEXT == ',')
			TEXT++;

		while (TEXT[LENGTH] && TEXT[LENGTH] != ' ' && TEXT[LENGTH] != '\t')
			LENGTH++;

		if (!LENGTH)
			break ;

		{
			int	START = 0;
			int	END = LENGTH;

			while (START < END && strchr("\"'(", TEXT[START]))
				START++;

			while (END > START && strchr(".,!?;:\"')", TEXT[END - 1]))
				END--;

			if (END > START)
			{
				int	SIZE;

				if (END - START < 63)
					SIZE = END - START;
				else
					SIZE = 63;

				memcpy(SHOWN[COUNT], TEXT + START, (size_t)SIZE);
				SHOWN[COUNT][SIZE] = 0;

				for (INDEX = 0; INDEX <= SIZE; INDEX++)
					LOWER[COUNT][INDEX] =
						(char)tolower((uint8_t)SHOWN[COUNT][INDEX]);

				COUNT++;
			}
		}

		TEXT += LENGTH;
	}

	return (COUNT);
}

/* the action a message asks for, by the skill's own example phrasings,
 * for actions Lucy's planner does not know (KNOWN_ACTIONS): the literal
 * words of an example in order, the {1} taking the words between them */
int
	SKILLS_MATCH(
		SKILL_SET *SET, const char *MESSAGE, const char *const *KNOWN_ACTIONS,
		const SKILL_ACTION **ACTION, char *ARGUMENT, int ARGUMENT_SIZE
	)
{
	static char	WORDS[80][64];
	static char	SHOWN[80][64];
	static char	PATTERN[40][64];
	static char	PATTERN_SHOWN[40][64];
	int			WORD_COUNT = SKILL_WORDS(MESSAGE, WORDS, SHOWN, 80);
	int			BEST_SCORE = 0;
	int			SKILL;

	for (SKILL = 0; SKILL < SET->COUNT; SKILL++)
	{
		const SKILL_MEMORY	*MEMORY = SET->SKILLS[SKILL].MEMORY;
		int					ACTION_INDEX;

		if (!MEMORY)
			continue ;

		for (
			ACTION_INDEX = 0;
			ACTION_INDEX < MEMORY->ACTION_COUNT;
			ACTION_INDEX++
		)
		{
			const SKILL_ACTION	*ACTION_HERE = &MEMORY->ACTIONS[ACTION_INDEX];
			const char			*LINE;
			int					KNOWN = 0;
			int					INDEX;

			for (INDEX = 0; KNOWN_ACTIONS && KNOWN_ACTIONS[INDEX]; INDEX++)
				if (!strcmp(KNOWN_ACTIONS[INDEX], ACTION_HERE->NAME))
					KNOWN = 1;

			if (KNOWN || !ACTION_HERE->EXAMPLES)
				continue ;

			for (LINE = ACTION_HERE->EXAMPLES; *LINE;)
			{
				const char	*END = strchr(LINE, '\n');
				char		EXAMPLE[400];
				int			PATTERN_COUNT;
				int			SLOT_AT[2] = { -1, -1 };
				int			SLOT_FROM[2] = { -1, -1 };
				int			SLOT_TO[2] = { -1, -1 };
				int			LITERALS = 0;
				int			POSITION = 0;
				int			MATCHED = 1;
				int			EXTRA = 0;
				int			FILLING = -1;
				int			BAD = 0;

				if (!END)
					END = LINE + strlen(LINE);

				snprintf(
					EXAMPLE, sizeof EXAMPLE, "%.*s", (int)(END - LINE), LINE
				);

				if (*END)
					LINE = END + 1;
				else
					LINE = END;

				PATTERN_COUNT =
					SKILL_WORDS(EXAMPLE, PATTERN, PATTERN_SHOWN, 40);

				for (INDEX = 0; INDEX < PATTERN_COUNT; INDEX++)
					if (!strcmp(PATTERN[INDEX], "{1}"))
						SLOT_AT[0] = INDEX;
					else if (!strcmp(PATTERN[INDEX], "{2}"))
						SLOT_AT[1] = INDEX;
					else
						LITERALS++;

				/* two slots side by side cannot be told apart */
				if (
					SLOT_AT[0] >= 0 &&
					SLOT_AT[1] >= 0 &&
					abs(SLOT_AT[0] - SLOT_AT[1]) == 1
				)
					BAD = 1;

				if (!LITERALS || BAD || (SLOT_AT[1] >= 0 && SLOT_AT[0] < 0))
					continue ;

				/* each literal word in order; a slot takes the words between
				 * the literals around it */
				for (INDEX = 0; INDEX < PATTERN_COUNT && MATCHED; INDEX++)
				{
					int	SLOT;

					if (INDEX == SLOT_AT[0])
						SLOT = 0;
					else if (INDEX == SLOT_AT[1])
						SLOT = 1;
					else
						SLOT = -1;

					int	SCAN;

					if (SLOT >= 0)
					{
						SLOT_FROM[SLOT] = POSITION;
						FILLING = SLOT;

						if (INDEX + 1 >= PATTERN_COUNT)
						{
							SLOT_TO[SLOT] = WORD_COUNT;
							POSITION = WORD_COUNT;
						}

						continue ;
					}

					/* the literal after a slot: at least one word later */
					for (
						SCAN = POSITION + (FILLING >= 0 ? 1 : 0);
						SCAN < WORD_COUNT;
						SCAN++
					)
						if (!strcmp(WORDS[SCAN], PATTERN[INDEX]))
							break ;

					if (SCAN >= WORD_COUNT)
					{
						MATCHED = 0;
						break ;
					}

					if (FILLING >= 0)
					{
						SLOT_TO[FILLING] = SCAN;
						FILLING = -1;
					}
					else
						EXTRA += SCAN - POSITION;

					POSITION = SCAN + 1;
				}

				/* a few extra words ("please", "lucy", "can you") are fine */
				if (
					!MATCHED ||
					EXTRA +
							(FILLING < 0 && SLOT_AT[0] < 0
								? WORD_COUNT - POSITION
								: 0) >
						3
				)
					continue ;

				if (SLOT_AT[0] >= 0 && SLOT_TO[0] <= SLOT_FROM[0])
					continue ;

				if (SLOT_AT[1] >= 0 && SLOT_TO[1] <= SLOT_FROM[1])
					continue ;

				/* words after the last literal, with no slot to take them */
				if (
					SLOT_AT[0] != PATTERN_COUNT - 1 &&
					SLOT_AT[1] != PATTERN_COUNT - 1 &&
					WORD_COUNT - POSITION > 3
				)
					continue ;

				if (LITERALS * 10 - EXTRA > BEST_SCORE)
				{
					int	ARGUMENT_POSITION = 0;
					int	SLOT;

					BEST_SCORE = LITERALS * 10 - EXTRA;
					*ACTION = ACTION_HERE;
					ARGUMENT[0] = 0;

					for (SLOT = 0; SLOT < 2; SLOT++)
					{
						int	STARTED = 0;

						if (SLOT_AT[SLOT] < 0)
							continue ;

						if (SLOT == 1 && ARGUMENT_POSITION < ARGUMENT_SIZE - 1)
							ARGUMENT[ARGUMENT_POSITION++] = '\t';

						for (
							INDEX = SLOT_FROM[SLOT];
							INDEX < SLOT_TO[SLOT] &&
								ARGUMENT_POSITION < ARGUMENT_SIZE - 1;
							INDEX++
						)
						{
							ARGUMENT_POSITION += snprintf(
								ARGUMENT + ARGUMENT_POSITION,
								ARGUMENT_SIZE - ARGUMENT_POSITION, "%s%s",
								STARTED ? " " : "", SHOWN[INDEX]
							);
							STARTED = 1;
						}

						ARGUMENT[ARGUMENT_POSITION] = 0;
					}
				}
			}
		}
	}

	return (BEST_SCORE > 0);
}

void
	SKILLS_LIST(SKILL_SET *SET, char *OUTPUT, int OUTPUT_SIZE)
{
	int	POSITION = 0;
	int	INDEX;

	OUTPUT[0] = 0;

	for (INDEX = 0; INDEX < SET->COUNT && POSITION < OUTPUT_SIZE - 1; INDEX++)
	{
		const SKILL_MEMORY	*MEMORY = SET->SKILLS[INDEX].MEMORY;

		if (!MEMORY)
			continue ;

		POSITION += snprintf(
			OUTPUT + POSITION, OUTPUT_SIZE - POSITION, "%s%s (%s)",
			POSITION ? "; " : "", MEMORY->NAME,
			MEMORY->ABOUT ? MEMORY->ABOUT : ""
		);
	}
}
