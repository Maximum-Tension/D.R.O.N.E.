#include "PACKAGES.h"
#include "FILE_SYSTEM.h"
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/* size and time of the file: a different stamp means a new version */
static long long
	PACKAGE_STAMP(const char *PATH)
{
	struct stat	INFORMATION;

	if (stat(PATH, &INFORMATION))
		return (-1);

	return (
		(long long)INFORMATION.st_mtime *(long long)1000003 +
			(long long)INFORMATION.st_size
	);
}

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

/* "Cats" -> "cat", "the dog" -> "dog", "puppies" -> "puppy": how a thing
 * is filed */
void
	PACKAGES_THING_NAME(const char *WORD, char *OUTPUT, int OUTPUT_SIZE)
{
	char	COPY[96];
	char	*START = COPY;

	snprintf(COPY, sizeof COPY, "%s", WORD);
	TRIM(COPY);
	LOWER(COPY);

	if (!strncmp(START, "the ", 4))
		START += 4;
	else if (!strncmp(START, "a ", 2))
		START += 2;
	else if (!strncmp(START, "an ", 3))
		START += 3;

	snprintf(OUTPUT, OUTPUT_SIZE, "%s", START);
}

/* the singular of a filed name, for a second look: "cats" -> "cat" */
static int
	SINGULAR_NAME(const char *NAME, char *OUTPUT, int OUTPUT_SIZE)
{
	static const char	*IRREGULAR[15][2] = {
		{ "people", "person" }, { "men", "man" }, { "women", "woman" },
		{ "children", "child" }, { "mice", "mouse" }, { "geese", "goose" },
		{ "feet", "foot" }, { "teeth", "tooth" }, { "wolves", "wolf" },
		{ "leaves", "leaf" }, { "knives", "knife" }, { "sheep", "sheep" },
		{ "fish", "fish" }, { "deer", "deer" }, { NULL, NULL }
	};
	size_t				LENGTH = strlen(NAME);
	int					INDEX;

	for (INDEX = 0; IRREGULAR[INDEX][0]; INDEX++)
		if (!strcmp(NAME, IRREGULAR[INDEX][0]))
		{
			snprintf(OUTPUT, OUTPUT_SIZE, "%s", IRREGULAR[INDEX][1]);
			return (1);
		}

	if (LENGTH > 4 && !strcmp(NAME + LENGTH - 3, "ies"))
	{
		snprintf(OUTPUT, OUTPUT_SIZE, "%.*sy", (int)LENGTH - 3, NAME);
		return (1);
	}

	if (
		LENGTH > 4 &&
		(
			!strcmp(NAME + LENGTH - 4, "ches") ||
			!strcmp(NAME + LENGTH - 4, "shes") ||
			!strcmp(NAME + LENGTH - 3, "xes") ||
			!strcmp(NAME + LENGTH - 4, "sses")
		)
	)
	{
		snprintf(OUTPUT, OUTPUT_SIZE, "%.*s", (int)LENGTH - 2, NAME);
		return (1);
	}

	if (LENGTH > 2 && NAME[LENGTH - 1] == 's' && NAME[LENGTH - 2] != 's')
	{
		snprintf(OUTPUT, OUTPUT_SIZE, "%.*s", (int)LENGTH - 1, NAME);
		return (1);
	}

	return (0);
}

static int
	ADD_FACT(
		PACKAGE_SET *SET, const char *THING, const char *PROPERTY,
		const char *VALUE, int PACKAGE_INDEX, int IS_DERIVED
	)
{
	PACKAGE_FACT	*FACT;

	if (SET->FACT_COUNT == SET->FACT_CAPACITY)
	{
		int	CAPACITY;

		if (SET->FACT_CAPACITY)
			CAPACITY = SET->FACT_CAPACITY * 2;
		else
			CAPACITY = 1024;

		PACKAGE_FACT	*GROWN =
			realloc(SET->FACTS, (size_t)CAPACITY * sizeof *GROWN);

		if (!GROWN)
			return (-1);

		SET->FACTS = GROWN;
		SET->FACT_CAPACITY = CAPACITY;
	}

	FACT = &SET->FACTS[SET->FACT_COUNT++];
	memset(FACT, 0, sizeof *FACT);
	snprintf(FACT->THING, sizeof FACT->THING, "%s", THING);
	snprintf(FACT->PROPERTY, sizeof FACT->PROPERTY, "%s", PROPERTY);
	snprintf(FACT->VALUE, sizeof FACT->VALUE, "%s", VALUE);
	FACT->PACKAGE_INDEX = (short)PACKAGE_INDEX;
	FACT->IS_DERIVED = (short)IS_DERIVED;

	if (IS_DERIVED)
		FACT->ENTRY = -1;
	else
		FACT->ENTRY = SET->NEXT_ENTRY;

	return (0);
}

static int
	FIND_FACT(PACKAGE_SET *SET, const char *THING, const char *PROPERTY)
{
	int	INDEX;

	for (INDEX = 0; INDEX < SET->FACT_COUNT; INDEX++)
		if (
			!strcmp(SET->FACTS[INDEX].THING, THING) &&
			!strcmp(SET->FACTS[INDEX].PROPERTY, PROPERTY)
		)
			return (INDEX);

	return (-1);
}

/* the facts of one package go; the ones it gave others (an opposite) too */
static void
	FORGET_PACKAGE(PACKAGE_SET *SET, int PACKAGE_INDEX)
{
	int	READ_AT;
	int	WRITE_AT = 0;

	for (READ_AT = 0; READ_AT < SET->FACT_COUNT; READ_AT++)
		if (SET->FACTS[READ_AT].PACKAGE_INDEX != PACKAGE_INDEX)
			SET->FACTS[WRITE_AT++] = SET->FACTS[READ_AT];

	SET->FACT_COUNT = WRITE_AT;
	SET->PACKAGES[PACKAGE_INDEX].IS_LOADED = 0;
	SET->PACKAGES[PACKAGE_INDEX].ENTRY_COUNT = 0;
}

/* "opposite: cold" said about hot is also "opposite: hot" about cold */
static void
	ADD_TURNED_FACTS(PACKAGE_SET *SET, int PACKAGE_INDEX)
{
	int	COUNT = SET->FACT_COUNT;
	int	INDEX;

	for (INDEX = 0; INDEX < COUNT; INDEX++)
	{
		PACKAGE_FACT	FACT = SET->FACTS[INDEX];

		if (
			FACT.PACKAGE_INDEX != PACKAGE_INDEX ||
			FACT.IS_DERIVED ||
			strcmp(FACT.PROPERTY, "opposite") ||
			strchr(FACT.VALUE, ',')
		)
			continue ;

		if (FIND_FACT(SET, FACT.VALUE, "opposite") < 0)
			ADD_FACT(SET, FACT.VALUE, "opposite", FACT.THING, PACKAGE_INDEX, 1);
	}
}

/* reads "cat, kitty: A cat is ... | kind: animal | legs: 4" lines */
static int
	READ_PACKAGE(PACKAGE_SET *SET, int PACKAGE_INDEX)
{
	PACKAGE	*ONE = &SET->PACKAGES[PACKAGE_INDEX];
	FILE	*STREAM = fopen(ONE->PATH, "rb");
	char	LINE[4096];

	if (!STREAM)
		return (-1);

	ONE->ENTRY_COUNT = 0;
	ONE->ABOUT[0] = 0;

	while (fgets(LINE, sizeof LINE, STREAM))
	{
		char	*COLON;
		char	NAMES[600];
		char	*REST;
		char	*PIECE;
		char	*SAVED_PIECE;
		int		FIRST_PIECE = 1;

		TRIM(LINE);

		/* "# about: everyday animals and things" */
		if (LINE[0] == '#')
		{
			const char	*ABOUT = strstr(LINE, "about:");

			if (ABOUT && !ONE->ABOUT[0])
			{
				snprintf(ONE->ABOUT, sizeof ONE->ABOUT, "%s", ABOUT + 6);
				TRIM(ONE->ABOUT);
			}

			continue ;
		}

		COLON = strchr(LINE, ':');

		if (!LINE[0] || !COLON || COLON == LINE)
			continue ;

		snprintf(NAMES, sizeof NAMES, "%.*s", (int)(COLON - LINE), LINE);
		REST = COLON + 1;

		for (
			PIECE = strtok_r(REST, "|", &SAVED_PIECE);
			PIECE;
			PIECE = strtok_r(NULL, "|", &SAVED_PIECE), FIRST_PIECE = 0
		)
		{
			char	PROPERTY[32] = "is";
			char	VALUE[240];
			char	*KEY_END = strchr(PIECE, ':');
			char	NAME_LIST[600];
			char	*NAME;
			char	*SAVED_NAME;

			/* "kind: animal" names a property; the first piece without one is
			 * what the thing is */
			if (
				KEY_END &&
				KEY_END - PIECE < 30 &&
				strspn(PIECE, " abcdefghijklmnopqrstuvwxyz_") >=
					(size_t)(KEY_END - PIECE)
			)
			{
				snprintf(
					PROPERTY, sizeof PROPERTY, "%.*s", (int)(KEY_END - PIECE),
					PIECE
				);
				TRIM(PROPERTY);
				snprintf(VALUE, sizeof VALUE, "%s", KEY_END + 1);
			}
			else if (FIRST_PIECE)
				snprintf(VALUE, sizeof VALUE, "%s", PIECE);
			else
				continue ;

			TRIM(VALUE);

			if (!VALUE[0] || !PROPERTY[0])
				continue ;

			snprintf(NAME_LIST, sizeof NAME_LIST, "%s", NAMES);

			for (
				NAME = strtok_r(NAME_LIST, ",", &SAVED_NAME);
				NAME;
				NAME = strtok_r(NULL, ",", &SAVED_NAME)
			)
			{
				char	THING[48];

				PACKAGES_THING_NAME(NAME, THING, sizeof THING);

				if (THING[0])
					ADD_FACT(SET, THING, PROPERTY, VALUE, PACKAGE_INDEX, 0);
			}
		}

		ONE->ENTRY_COUNT++;
		SET->NEXT_ENTRY++;
	}

	fclose(STREAM);
	ADD_TURNED_FACTS(SET, PACKAGE_INDEX);
	ONE->IS_LOADED = 1;

	return (0);
}

/* catalog.txt: "places.mem: countries and capitals | france, paris, ..." */
static void
	READ_CATALOG(PACKAGE_SET *SET)
{
	FILE	*STREAM;
	char	*LINE = malloc(65536);

	SET->CATALOG_COUNT = 0;
	SET->CATALOG_NAME_COUNT = 0;

	if (!SET->CATALOG_PATH[0])
		return ;

	if (LINE)
		STREAM = fopen(SET->CATALOG_PATH, "rb");
	else
		STREAM = NULL;

	if (!STREAM)
	{
		free(LINE);
		return ;
	}

	while (fgets(LINE, 65536, STREAM) && SET->CATALOG_NAME_COUNT < MAX_PACKAGES)
	{
		char	*COLON;
		char	*BAR;
		char	*WORD;
		char	*SAVED;
		int		OWNER;

		TRIM(LINE);
		COLON = strchr(LINE, ':');

		if (LINE[0] == '#' || !COLON)
			continue ;

		*COLON = 0;
		OWNER = SET->CATALOG_NAME_COUNT++;
		snprintf(SET->CATALOG_NAMES[OWNER], 64, "%s", LINE);
		TRIM(SET->CATALOG_NAMES[OWNER]);
		BAR = strchr(COLON + 1, '|');
		SET->CATALOG_ABOUT[OWNER][0] = 0;

		if (BAR)
		{
			*BAR = 0;
			snprintf(SET->CATALOG_ABOUT[OWNER], 160, "%s", COLON + 1);
			TRIM(SET->CATALOG_ABOUT[OWNER]);
		}

		for (
			WORD = strtok_r(BAR ? BAR + 1 : COLON + 1, ",", &SAVED);
			WORD;
			WORD = strtok_r(NULL, ",", &SAVED)
		)
		{
			char	THING[48];

			PACKAGES_THING_NAME(WORD, THING, sizeof THING);

			if (!THING[0])
				continue ;

			if (SET->CATALOG_COUNT == SET->CATALOG_CAPACITY)
			{
				int	CAPACITY;

				if (SET->CATALOG_CAPACITY)
					CAPACITY = SET->CATALOG_CAPACITY * 2;
				else
					CAPACITY = 1024;

				char	(*WORDS)[48] = realloc(
					SET->CATALOG_WORDS, (size_t)CAPACITY * sizeof *WORDS
				);
				short	*OWNERS;

				if (!WORDS)
					break ;

				SET->CATALOG_WORDS = WORDS;
				OWNERS = realloc(
					SET->CATALOG_OWNERS, (size_t)CAPACITY * sizeof *OWNERS
				);

				if (!OWNERS)
					break ;

				SET->CATALOG_OWNERS = OWNERS;
				SET->CATALOG_CAPACITY = CAPACITY;
			}

			snprintf(SET->CATALOG_WORDS[SET->CATALOG_COUNT], 48, "%s", THING);
			SET->CATALOG_OWNERS[SET->CATALOG_COUNT] = (short)OWNER;
			SET->CATALOG_COUNT++;
		}
	}

	fclose(STREAM);
	free(LINE);
}

static int
	HAS_MEM_ENDING(const char *NAME)
{
	size_t	LENGTH = strlen(NAME);

	return (LENGTH > 4 && !strcmp(NAME + LENGTH - 4, ".mem"));
}

int
	PACKAGES_OPEN(
		PACKAGE_SET *SET, const char *FOLDER, const char *SECOND_FOLDER
	)
{
	char	NEWS[2];

	memset(SET, 0, sizeof *SET);
	snprintf(SET->FOLDERS[0], sizeof SET->FOLDERS[0], "%s", FOLDER);
	SET->FOLDER_COUNT = 1;

	if (SECOND_FOLDER && SECOND_FOLDER[0])
	{
		snprintf(SET->FOLDERS[1], sizeof SET->FOLDERS[1], "%s", SECOND_FOLDER);
		SET->FOLDER_COUNT = 2;
	}

	snprintf(
		SET->CATALOG_PATH, sizeof SET->CATALOG_PATH, "%s/catalog.txt", FOLDER
	);
	PACKAGES_REFRESH(SET, NEWS, sizeof NEWS);

	return (SET->COUNT);
}

/* the packages loaded now, by name, with how many things each knows */
typedef struct
{
	char	NAME[64];
	int		ENTRY_COUNT;
} PACKAGE_SUMMARY;

static int
	SUMMARIZE(PACKAGE_SET *SET, PACKAGE_SUMMARY *SUMMARY)
{
	int	COUNT = 0;
	int	INDEX;

	for (INDEX = 0; INDEX < SET->COUNT; INDEX++)
		if (SET->PACKAGES[INDEX].IS_LOADED)
		{
			snprintf(
				SUMMARY[COUNT].NAME, sizeof SUMMARY[COUNT].NAME, "%s",
				SET->PACKAGES[INDEX].NAME
			);
			SUMMARY[COUNT].ENTRY_COUNT = SET->PACKAGES[INDEX].ENTRY_COUNT;
			COUNT++;
		}

	return (COUNT);
}

/* a package named NAME is loaded from another file than PATH */
static int
	LOADED_ELSEWHERE(PACKAGE_SET *SET, const char *NAME, const char *PATH)
{
	int	INDEX;

	for (INDEX = 0; INDEX < SET->COUNT; INDEX++)
		if (
			SET->PACKAGES[INDEX].IS_LOADED &&
			!strcmp(SET->PACKAGES[INDEX].NAME, NAME) &&
			strcmp(SET->PACKAGES[INDEX].PATH, PATH)
		)
			return (1);

	return (0);
}

/* new, changed and removed packages since the last look; NEWS says what is
 * different now ("package places (196 things)", "package places removed"),
 * and the count of changes comes back.  Two files with the same package
 * name are one package: the one in the first folder counts. */
int
	PACKAGES_REFRESH(PACKAGE_SET *SET, char *NEWS, int NEWS_SIZE)
{
	FILE_ENTRY		*ENTRIES = calloc(256, sizeof(FILE_ENTRY));
	PACKAGE_SUMMARY	BEFORE[MAX_PACKAGES];
	PACKAGE_SUMMARY	AFTER[MAX_PACKAGES];
	int				BEFORE_COUNT;
	int				AFTER_COUNT;
	int				SEEN[MAX_PACKAGES];
	int				CHANGES = 0;
	int				NEWS_LENGTH = 0;
	int				FOLDER;
	int				INDEX;

	if (NEWS && NEWS_SIZE)
		NEWS[0] = 0;

	if (!ENTRIES)
		return (0);

	memset(SEEN, 0, sizeof SEEN);
	BEFORE_COUNT = SUMMARIZE(SET, BEFORE);

	for (FOLDER = 0; FOLDER < SET->FOLDER_COUNT; FOLDER++)
	{
		int	COUNT = FILE_SYSTEM_LIST(SET->FOLDERS[FOLDER], ENTRIES, 256);

		for (INDEX = 0; INDEX < COUNT; INDEX++)
		{
			char		PATH[600];
			char		NAME[64];
			long long	STAMP;
			int			FOUND;

			if (
				ENTRIES[INDEX].IS_DIRECTORY ||
				!HAS_MEM_ENDING(ENTRIES[INDEX].NAME)
			)
				continue ;

			snprintf(
				PATH, sizeof PATH, "%s/%s", SET->FOLDERS[FOLDER],
				ENTRIES[INDEX].NAME
			);
			snprintf(
				NAME, sizeof NAME, "%.*s", (int)strlen(ENTRIES[INDEX].NAME) - 4,
				ENTRIES[INDEX].NAME
			);

			/* the same package a second time (an old copy in another
			 * folder): the first one counts */
			if (LOADED_ELSEWHERE(SET, NAME, PATH))
			{
				int	SEEN_ELSEWHERE = 0;
				int	OTHER;

				for (OTHER = 0; OTHER < SET->COUNT; OTHER++)
					if (
						SEEN[OTHER] &&
						SET->PACKAGES[OTHER].IS_LOADED &&
						!strcmp(SET->PACKAGES[OTHER].NAME, NAME)
					)
						SEEN_ELSEWHERE = 1;

				if (SEEN_ELSEWHERE)
					continue ;
			}

			STAMP = PACKAGE_STAMP(PATH);

			for (FOUND = 0; FOUND < SET->COUNT; FOUND++)
				if (!strcmp(SET->PACKAGES[FOUND].PATH, PATH))
					break ;

			if (FOUND == SET->COUNT)
			{
				if (SET->COUNT == MAX_PACKAGES)
					continue ;

				memset(&SET->PACKAGES[FOUND], 0, sizeof SET->PACKAGES[FOUND]);
				snprintf(SET->PACKAGES[FOUND].NAME, 64, "%s", NAME);
				snprintf(SET->PACKAGES[FOUND].PATH, 600, "%s", PATH);
				SET->COUNT++;
			}
			else if (
				SET->PACKAGES[FOUND].IS_LOADED &&
				SET->PACKAGES[FOUND].STAMP == STAMP
			)
			{
				SEEN[FOUND] = 1;
				continue ;
			}

			SEEN[FOUND] = 1;

			if (SET->PACKAGES[FOUND].IS_LOADED)
				FORGET_PACKAGE(SET, FOUND);

			SET->PACKAGES[FOUND].STAMP = STAMP;

			if (READ_PACKAGE(SET, FOUND) == 0)
				CHANGES++;
		}
	}

	/* a package taken out of the folder (or a second copy of one) is
	 * forgotten */
	for (INDEX = 0; INDEX < SET->COUNT; INDEX++)
		if (!SEEN[INDEX] && SET->PACKAGES[INDEX].IS_LOADED)
		{
			FORGET_PACKAGE(SET, INDEX);
			SET->PACKAGES[INDEX].STAMP = 0;
			CHANGES++;
		}

	{
		long long	STAMP = PACKAGE_STAMP(SET->CATALOG_PATH);

		if (STAMP != SET->CATALOG_STAMP)
		{
			SET->CATALOG_STAMP = STAMP;
			READ_CATALOG(SET);
		}
	}

	free(ENTRIES);

	/* the news is what is different now, by name: a package read again
	 * the same as before is no news */
	AFTER_COUNT = SUMMARIZE(SET, AFTER);

	for (INDEX = 0; INDEX < AFTER_COUNT && NEWS; INDEX++)
	{
		int	WAS = -1;
		int	OTHER;

		for (OTHER = 0; OTHER < BEFORE_COUNT; OTHER++)
			if (!strcmp(BEFORE[OTHER].NAME, AFTER[INDEX].NAME))
				WAS = BEFORE[OTHER].ENTRY_COUNT;

		if (WAS == AFTER[INDEX].ENTRY_COUNT || NEWS_LENGTH >= NEWS_SIZE - 1)
			continue ;

		NEWS_LENGTH += snprintf(
			NEWS + NEWS_LENGTH, NEWS_SIZE - NEWS_LENGTH,
			"%spackage %s (%d things)", NEWS_LENGTH ? ", " : "",
			AFTER[INDEX].NAME, AFTER[INDEX].ENTRY_COUNT
		);
	}

	for (INDEX = 0; INDEX < BEFORE_COUNT && NEWS; INDEX++)
	{
		int	STILL = 0;
		int	OTHER;

		for (OTHER = 0; OTHER < AFTER_COUNT; OTHER++)
			if (!strcmp(BEFORE[INDEX].NAME, AFTER[OTHER].NAME))
				STILL = 1;

		if (STILL || NEWS_LENGTH >= NEWS_SIZE - 1)
			continue ;

		NEWS_LENGTH += snprintf(
			NEWS + NEWS_LENGTH, NEWS_SIZE - NEWS_LENGTH, "%spackage %s removed",
			NEWS_LENGTH ? ", " : "", BEFORE[INDEX].NAME
		);
	}

	if (NEWS && NEWS_LENGTH >= NEWS_SIZE)
		NEWS[NEWS_SIZE - 1] = 0;

	return (CHANGES);
}

void
	PACKAGES_CLOSE(PACKAGE_SET *SET)
{
	free(SET->FACTS);
	free(SET->CATALOG_WORDS);
	free(SET->CATALOG_OWNERS);
	memset(SET, 0, sizeof *SET);
}

/* what is known about THING: PROPERTY "legs" -> "4"; 1 when known */
int
	PACKAGES_KNOW(
		PACKAGE_SET *SET, const char *THING, const char *PROPERTY, char *VALUE,
		int VALUE_SIZE
	)
{
	char	NAME[48];
	char	SINGLE[48];
	int		FOUND;

	if (!SET || !THING || !PROPERTY)
		return (0);

	PACKAGES_THING_NAME(THING, NAME, sizeof NAME);
	FOUND = FIND_FACT(SET, NAME, PROPERTY);

	if (FOUND < 0 && SINGULAR_NAME(NAME, SINGLE, sizeof SINGLE))
		FOUND = FIND_FACT(SET, SINGLE, PROPERTY);

	if (FOUND < 0)
		return (0);

	if (VALUE && VALUE_SIZE)
		snprintf(VALUE, VALUE_SIZE, "%s", SET->FACTS[FOUND].VALUE);

	/* a word with two meanings ("mouse": an animal, a computer word) has
	 * the kinds of both */
	if (VALUE && VALUE_SIZE && !strcmp(PROPERTY, "kind"))
	{
		int	INDEX;

		for (INDEX = FOUND + 1; INDEX < SET->FACT_COUNT; INDEX++)
			if (
				!strcmp(SET->FACTS[INDEX].THING, SET->FACTS[FOUND].THING) &&
				!strcmp(SET->FACTS[INDEX].PROPERTY, "kind") &&
				strlen(VALUE) + strlen(SET->FACTS[INDEX].VALUE) + 3 <
					(size_t)VALUE_SIZE
			)
			{
				strcat(VALUE, ", ");
				strcat(VALUE, SET->FACTS[INDEX].VALUE);
			}
	}

	return (1);
}

/* 1 when anything at all is known about THING */
int
	PACKAGES_KNOWS_THING(PACKAGE_SET *SET, const char *THING)
{
	char	NAME[48];
	char	SINGLE[48];
	int		INDEX;
	int		HAS_SINGLE;

	if (!SET || !THING)
		return (0);

	PACKAGES_THING_NAME(THING, NAME, sizeof NAME);
	HAS_SINGLE = SINGULAR_NAME(NAME, SINGLE, sizeof SINGLE);

	for (INDEX = 0; INDEX < SET->FACT_COUNT; INDEX++)
		if (
			!strcmp(SET->FACTS[INDEX].THING, NAME) ||
			(HAS_SINGLE && !strcmp(SET->FACTS[INDEX].THING, SINGLE))
		)
			return (1);

	return (0);
}

/* the package that told what is known about THING ("things_object.mem");
 * 1 when one did */
int
	PACKAGES_SOURCE(
		PACKAGE_SET *SET, const char *THING, char *OUTPUT, int OUTPUT_SIZE
	)
{
	char	NAME[48];
	char	SINGLE[48];
	int		INDEX;
	int		HAS_SINGLE;

	if (OUTPUT && OUTPUT_SIZE)
		OUTPUT[0] = 0;

	if (!SET || !THING)
		return (0);

	PACKAGES_THING_NAME(THING, NAME, sizeof NAME);
	HAS_SINGLE = SINGULAR_NAME(NAME, SINGLE, sizeof SINGLE);

	for (INDEX = 0; INDEX < SET->FACT_COUNT; INDEX++)
		if (
			(
				!strcmp(SET->FACTS[INDEX].THING, NAME) ||
				(HAS_SINGLE && !strcmp(SET->FACTS[INDEX].THING, SINGLE))
			) &&
			SET->FACTS[INDEX].PACKAGE_INDEX >= 0 &&
			SET->FACTS[INDEX].PACKAGE_INDEX < SET->COUNT
		)
		{
			if (OUTPUT && OUTPUT_SIZE)
				snprintf(
					OUTPUT, (size_t)OUTPUT_SIZE, "%s.mem",
					SET->PACKAGES[SET->FACTS[INDEX].PACKAGE_INDEX].NAME
				);

			return (1);
		}

	return (0);
}

/* the things whose PROPERTY includes VALUE: kind "color" -> "red, blue,
 * green"; how many come back */
int
	PACKAGES_THINGS_WITH(
		PACKAGE_SET *SET, const char *PROPERTY, const char *VALUE, char *OUTPUT,
		int OUTPUT_SIZE
	)
{
	char	WANTED[64];
	char	SINGLE[64];
	int		HAS_SINGLE;
	int		COUNT = 0;
	int		LENGTH = 0;
	int		INDEX;

	if (OUTPUT && OUTPUT_SIZE)
		OUTPUT[0] = 0;

	if (!SET)
		return (0);

	PACKAGES_THING_NAME(VALUE, WANTED, sizeof WANTED);
	HAS_SINGLE = SINGULAR_NAME(WANTED, SINGLE, sizeof SINGLE);

	for (INDEX = 0; INDEX < SET->FACT_COUNT; INDEX++)
	{
		char	LIST[240];
		char	*ITEM;
		char	*SAVED;
		int		MATCH = 0;

		if (strcmp(SET->FACTS[INDEX].PROPERTY, PROPERTY))
			continue ;

		snprintf(LIST, sizeof LIST, "%s", SET->FACTS[INDEX].VALUE);

		for (
			ITEM = strtok_r(LIST, ",", &SAVED);
			ITEM && !MATCH;
			ITEM = strtok_r(NULL, ",", &SAVED)
		)
		{
			char	NAME[64];

			PACKAGES_THING_NAME(ITEM, NAME, sizeof NAME);
			MATCH =
				!strcmp(NAME, WANTED) || (HAS_SINGLE && !strcmp(NAME, SINGLE));
		}

		if (!MATCH)
			continue ;

		/* each thing once, by its first name ("cat", not also "kitty") */
		{
			int	BEFORE;
			int	REPEATED = 0;

			for (BEFORE = 0; BEFORE < INDEX && !REPEATED; BEFORE++)
				if (
					!strcmp(SET->FACTS[BEFORE].PROPERTY, PROPERTY) &&
					(
						!strcmp(
							SET->FACTS[BEFORE].THING, SET->FACTS[INDEX].THING
						) ||
						(
							SET->FACTS[INDEX].ENTRY >= 0 &&
							SET->FACTS[BEFORE].ENTRY == SET->FACTS[INDEX].ENTRY
						)
					)
				)
					REPEATED = 1;

			if (REPEATED)
				continue ;
		}

		if (OUTPUT && LENGTH < OUTPUT_SIZE - 1)
			LENGTH += snprintf(
				OUTPUT + LENGTH, OUTPUT_SIZE - LENGTH, "%s%s",
				COUNT ? ", " : "", SET->FACTS[INDEX].THING
			);

		COUNT++;
	}

	return (COUNT);
}

/* the package that would know WORD, from the catalog ("places.mem"); 1 when
 * one is named and it is not in the folder yet, 2 when it is there */
int
	PACKAGES_PACKAGE_FOR(
		PACKAGE_SET *SET, const char *WORD, char *OUTPUT, int OUTPUT_SIZE
	)
{
	char	NAME[48];
	char	SINGLE[48];
	int		HAS_SINGLE;
	int		INDEX;

	if (!SET || !WORD)
		return (0);

	PACKAGES_THING_NAME(WORD, NAME, sizeof NAME);
	HAS_SINGLE = SINGULAR_NAME(NAME, SINGLE, sizeof SINGLE);

	for (INDEX = 0; INDEX < SET->CATALOG_COUNT; INDEX++)
		if (
			!strcmp(SET->CATALOG_WORDS[INDEX], NAME) ||
			(HAS_SINGLE && !strcmp(SET->CATALOG_WORDS[INDEX], SINGLE))
		)
		{
			const char	*PACKAGE_NAME =
				SET->CATALOG_NAMES[SET->CATALOG_OWNERS[INDEX]];
			char		BARE[64];
			int			LOADED;

			snprintf(BARE, sizeof BARE, "%s", PACKAGE_NAME);

			if (HAS_MEM_ENDING(BARE))
				BARE[strlen(BARE) - 4] = 0;

			if (OUTPUT && OUTPUT_SIZE)
				snprintf(OUTPUT, OUTPUT_SIZE, "%s.mem", BARE);

			for (LOADED = 0; LOADED < SET->COUNT; LOADED++)
				if (
					SET->PACKAGES[LOADED].IS_LOADED &&
					!strcmp(SET->PACKAGES[LOADED].NAME, BARE)
				)
					return (2);

			return (1);
		}

	return (0);
}

/* "A cat is a small furry animal ..." lines of one package, one per line,
 * for the topic Lucy looks things up in */
int
	PACKAGES_DEFINITIONS(
		PACKAGE_SET *SET, int PACKAGE_INDEX, char *OUTPUT, int OUTPUT_SIZE
	)
{
	int	LENGTH = 0;
	int	COUNT = 0;
	int	INDEX;

	if (OUTPUT_SIZE)
		OUTPUT[0] = 0;

	for (
		INDEX = 0;
		INDEX < SET->FACT_COUNT && LENGTH < OUTPUT_SIZE - 1;
		INDEX++
	)
	{
		const PACKAGE_FACT	*FACT = &SET->FACTS[INDEX];
		int					BEFORE;
		int					REPEATED = 0;

		if (
			FACT->PACKAGE_INDEX != PACKAGE_INDEX ||
			strcmp(FACT->PROPERTY, "is")
		)
			continue ;

		/* "bike, bicycle: ..." is one sentence */
		for (BEFORE = INDEX - 1; BEFORE >= 0 && BEFORE >= INDEX - 4; BEFORE--)
			if (
				!strcmp(SET->FACTS[BEFORE].PROPERTY, "is") &&
				!strcmp(SET->FACTS[BEFORE].VALUE, FACT->VALUE)
			)
				REPEATED = 1;

		if (REPEATED)
			continue ;

		LENGTH += snprintf(
			OUTPUT + LENGTH, OUTPUT_SIZE - LENGTH, "%s\n", FACT->VALUE
		);
		COUNT++;
	}

	return (COUNT);
}

/* "things_object (152 things): everyday animals ..." for each package */
void
	PACKAGES_LIST(PACKAGE_SET *SET, char *OUTPUT, int OUTPUT_SIZE)
{
	int	LENGTH = 0;
	int	INDEX;

	if (OUTPUT_SIZE)
		OUTPUT[0] = 0;

	for (INDEX = 0; INDEX < SET->COUNT && LENGTH < OUTPUT_SIZE - 1; INDEX++)
		if (SET->PACKAGES[INDEX].IS_LOADED)
			LENGTH += snprintf(
				OUTPUT + LENGTH, OUTPUT_SIZE - LENGTH,
				"%s%s.mem (%d things%s%s)", LENGTH ? ", " : "",
				SET->PACKAGES[INDEX].NAME, SET->PACKAGES[INDEX].ENTRY_COUNT,
				SET->PACKAGES[INDEX].ABOUT[0] ? ": " : "",
				SET->PACKAGES[INDEX].ABOUT
			);
}
