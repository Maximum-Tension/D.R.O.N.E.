#include "SAYINGS.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <sys/stat.h>

/* when the file was last changed and how big it is: a new value means it
 * has to be read again */
static long long
	SAYINGS_STAMP(const char *PATH)
{
	struct stat	INFO;

	if (stat(PATH, &INFO))
		return (0);

	return (
		(long long)INFO.st_mtime * (long long)1000003 + (long long)INFO.st_size
	);
}

static void
	SAYINGS_FORGET(SAYINGS *SET)
{
	int	INDEX;

	for (INDEX = 0; INDEX < SET->COUNT; INDEX++)
		free(SET->LINES[INDEX].TEXT);

	SET->COUNT = 0;
}

/* "Reminder Set " -> "reminder set" */
static void
	SAYINGS_KEY(const char *TEXT, int LENGTH, char *OUTPUT, int OUTPUT_SIZE)
{
	int	POSITION = 0;
	int	INDEX;
	int	SPACE = 0;

	for (INDEX = 0; INDEX < LENGTH && POSITION < OUTPUT_SIZE - 1; INDEX++)
	{
		uint8_t	CHARACTER = (uint8_t)TEXT[INDEX];

		if (CHARACTER == ' ' || CHARACTER == '\t' || CHARACTER == '_')
		{
			SPACE = POSITION > 0;
			continue ;
		}

		if (SPACE && POSITION < OUTPUT_SIZE - 2)
			OUTPUT[POSITION++] = ' ';

		SPACE = 0;
		OUTPUT[POSITION++] = (char)tolower(CHARACTER);
	}

	OUTPUT[POSITION] = 0;
}

static int
	SAYINGS_READ(SAYINGS *SET)
{
	FILE	*STREAM;
	char	LINE[4096];
	int		IS_FIRST = 1;

	SAYINGS_FORGET(SET);
	SET->STAMP = SAYINGS_STAMP(SET->PATH);
	STREAM = fopen(SET->PATH, "rb");

	if (!STREAM)
		return (0);

	while (fgets(LINE, sizeof LINE, STREAM))
	{
		char	*START = LINE;
		char	*COLON;
		char	*TEXT;
		size_t	LENGTH = strlen(LINE);

		while (LENGTH && strchr("\r\n", LINE[LENGTH - 1]))
			LINE[--LENGTH] = 0;

		/* a byte order mark at the start of the file */
		if (
			IS_FIRST &&
			(uint8_t)START[0] == 0XEF &&
			(uint8_t)START[1] == 0XBB &&
			(uint8_t)START[2] == 0XBF
		)
			START += 3;

		IS_FIRST = 0;

		while (*START == ' ' || *START == '\t')
			START++;

		if (!*START || *START == '#')
			continue ;

		COLON = strchr(START, ':');

		if (!COLON || COLON == START)
			continue ;

		TEXT = COLON + 1;

		while (*TEXT == ' ' || *TEXT == '\t')
			TEXT++;

		LENGTH = strlen(TEXT);

		while (LENGTH && (TEXT[LENGTH - 1] == ' ' || TEXT[LENGTH - 1] == '\t'))
			TEXT[--LENGTH] = 0;

		if (!*TEXT)
			continue ;

		if (SET->COUNT == SET->CAPACITY)
		{
			int	CAPACITY;

			if (SET->CAPACITY)
				CAPACITY = SET->CAPACITY * 2;
			else
				CAPACITY = 128;

			SAYING_LINE	*LINES =
				realloc(SET->LINES, (size_t)CAPACITY * sizeof *LINES);

			if (!LINES)
				break ;

			SET->LINES = LINES;
			SET->CAPACITY = CAPACITY;
		}

		SAYINGS_KEY(
			START, (int)(COLON - START), SET->LINES[SET->COUNT].KEY,
			sizeof SET->LINES[0].KEY
		);
		SET->LINES[SET->COUNT].TEXT = malloc(LENGTH + 1);

		if (!SET->LINES[SET->COUNT].TEXT)
			break ;

		memcpy(SET->LINES[SET->COUNT].TEXT, TEXT, LENGTH + 1);
		SET->LINES[SET->COUNT].USED = 0;
		SET->COUNT++;
	}

	fclose(STREAM);

	return (SET->COUNT);
}

int
	SAYINGS_OPEN(SAYINGS *SET, const char *PATH)
{
	memset(SET, 0, sizeof *SET);
	snprintf(SET->PATH, sizeof SET->PATH, "%s", PATH);

	return (SAYINGS_READ(SET));
}

/* read again when the file changed; 1 when it did */
int
	SAYINGS_REFRESH(SAYINGS *SET)
{
	if (!SET->PATH[0] || SAYINGS_STAMP(SET->PATH) == SET->STAMP)
		return (0);

	SAYINGS_READ(SET);

	return (1);
}

void
	SAYINGS_CLOSE(SAYINGS *SET)
{
	SAYINGS_FORGET(SET);
	free(SET->LINES);
	SET->LINES = NULL;
	SET->CAPACITY = 0;
}

/* the line for KEY, the least used one when there are several */
const char
	*SAYINGS_FIND(SAYINGS *SET, const char *KEY)
{
	char	WANTED[48];
	int		BEST = -1;
	int		INDEX;

	if (!SET || !KEY)
		return (NULL);

	SAYINGS_KEY(KEY, (int)strlen(KEY), WANTED, sizeof WANTED);

	for (INDEX = 0; INDEX < SET->COUNT; INDEX++)
		if (
			!strcmp(SET->LINES[INDEX].KEY, WANTED) &&
			(BEST < 0 || SET->LINES[INDEX].USED < SET->LINES[BEST].USED)
		)
			BEST = INDEX;

	if (BEST < 0)
		return (NULL);

	SET->LINES[BEST].USED++;

	return (SET->LINES[BEST].TEXT);
}

/* the sentence with {1}, {2} ... filled in; a detail that is missing is
 * left out */
void
	SAYINGS_FILL(
		const char *TEMPLATE, const char *const *DETAILS, int DETAIL_COUNT,
		char *OUTPUT, int OUTPUT_SIZE
	)
{
	int	POSITION = 0;

	if (!OUTPUT_SIZE)
		return ;

	while (*TEMPLATE && POSITION < OUTPUT_SIZE - 1)
	{
		if (
			TEMPLATE[0] == '{' &&
			TEMPLATE[1] >= '1' &&
			TEMPLATE[1] <= '9' &&
			TEMPLATE[2] == '}'
		)
		{
			int			WHICH = TEMPLATE[1] - '1';
			const char	*DETAIL =
				WHICH < DETAIL_COUNT && DETAILS[WHICH] ? DETAILS[WHICH] : "";

			while (*DETAIL && POSITION < OUTPUT_SIZE - 1)
				OUTPUT[POSITION++] = *DETAIL++;

			TEMPLATE += 3;
			continue ;
		}

		/* "\n" written in the file is a new line */
		if (TEMPLATE[0] == '\\' && TEMPLATE[1] == 'n')
		{
			OUTPUT[POSITION++] = '\n';
			TEMPLATE += 2;
			continue ;
		}

		OUTPUT[POSITION++] = *TEMPLATE++;
	}

	OUTPUT[POSITION] = 0;
}

/* what to say for KEY: the file's sentence, or DEFAULT; 1 when it came
 * from the file */
int
	SAYINGS_SAY(
		SAYINGS *SET, const char *KEY, const char *DEFAULT,
		const char *const *DETAILS, int DETAIL_COUNT, char *OUTPUT,
		int OUTPUT_SIZE
	)
{
	const char	*FOUND = SAYINGS_FIND(SET, KEY);

	SAYINGS_FILL(
		FOUND ? FOUND
			: DEFAULT ? DEFAULT
		: "",
		DETAILS, DETAIL_COUNT, OUTPUT, OUTPUT_SIZE
	);

	return (FOUND != NULL);
}
