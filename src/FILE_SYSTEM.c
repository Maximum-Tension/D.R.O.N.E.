#include "FILE_SYSTEM.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#endif

int
	FILE_SYSTEM_LIST(const char *PATH, FILE_ENTRY *OUTPUT, int OUTPUT_SIZE)
{
	int	ENTRY_COUNT = 0;
#ifdef _WIN32
	char	SEARCH_PATTERN[1024];

	snprintf(SEARCH_PATTERN, sizeof SEARCH_PATTERN, "%s\\*", PATH);

	WIN32_FIND_DATAA	FIND_DATA;
	HANDLE				FIND_HANDLE =
		FindFirstFileA(SEARCH_PATTERN, &FIND_DATA);

	if (FIND_HANDLE == INVALID_HANDLE_VALUE)
		return (0);

	do
	{
		if (FIND_DATA.cFileName[0] == '.' || ENTRY_COUNT >= OUTPUT_SIZE)
			continue ;

		snprintf(
			OUTPUT[ENTRY_COUNT].NAME, sizeof OUTPUT[ENTRY_COUNT].NAME, "%.127s",
			FIND_DATA.cFileName
		);
		OUTPUT[ENTRY_COUNT].IS_DIRECTORY =
			(FIND_DATA.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
		ENTRY_COUNT++;
	} while (FindNextFileA(FIND_HANDLE, &FIND_DATA));

	FindClose(FIND_HANDLE);
#else
	DIR	*DIRECTORY = opendir(PATH);

	if (!DIRECTORY)
		return (0);

	struct dirent	*DIRECTORY_ENTRY;

	while ((DIRECTORY_ENTRY = readdir(DIRECTORY)) && ENTRY_COUNT < OUTPUT_SIZE)
	{
		if (DIRECTORY_ENTRY->d_name[0] == '.')
			continue ;

		char	FULL_PATH[1200];

		snprintf(
			FULL_PATH, sizeof FULL_PATH, "%s/%s", PATH, DIRECTORY_ENTRY->d_name
		);
		snprintf(
			OUTPUT[ENTRY_COUNT].NAME, sizeof OUTPUT[ENTRY_COUNT].NAME, "%.127s",
			DIRECTORY_ENTRY->d_name
		);
		OUTPUT[ENTRY_COUNT].IS_DIRECTORY = FILE_SYSTEM_IS_DIRECTORY(FULL_PATH);
		ENTRY_COUNT++;
	}

	closedir(DIRECTORY);
#endif
	int	OUTER_INDEX;

	for (OUTER_INDEX = 0; OUTER_INDEX < ENTRY_COUNT; OUTER_INDEX++)
	{
		int	INNER_INDEX;

		for (
			INNER_INDEX = OUTER_INDEX + 1;
			INNER_INDEX < ENTRY_COUNT;
			INNER_INDEX++
		)
			if (strcmp(OUTPUT[OUTER_INDEX].NAME, OUTPUT[INNER_INDEX].NAME) > 0)
			{
				FILE_ENTRY	SWAP_ENTRY = OUTPUT[OUTER_INDEX];

				OUTPUT[OUTER_INDEX] = OUTPUT[INNER_INDEX];
				OUTPUT[INNER_INDEX] = SWAP_ENTRY;
			}
	}

	return (ENTRY_COUNT);
}

int
	FILE_SYSTEM_IS_DIRECTORY(const char *PATH)
{
	struct stat	FILE_STATUS;

	return (
		stat(PATH, &FILE_STATUS) == 0 &&
		(FILE_STATUS.st_mode & S_IFMT) == S_IFDIR
	);
}

int
	FILE_SYSTEM_EXISTS(const char *PATH)
{
	struct stat	FILE_STATUS;

	return (stat(PATH, &FILE_STATUS) == 0);
}

static int
	SAME_IGNORING_CASE(const char *LEFT_STRING, const char *RIGHT_STRING)
{
	while (*LEFT_STRING && *RIGHT_STRING)
	{
		if (
			tolower((unsigned char)*LEFT_STRING) !=
				tolower((unsigned char)*RIGHT_STRING)
		)
			return (0);

		LEFT_STRING++;
		RIGHT_STRING++;
	}

	return (*LEFT_STRING == *RIGHT_STRING);
}

int
	FILE_SYSTEM_RESOLVE(
		const char *ROOT_PATH, const char *RELATIVE_PATH, char *OUTPUT,
		int OUTPUT_SIZE, int ALLOW_NEW
	)
{
	char	CURRENT_PATH[1024];

	snprintf(CURRENT_PATH, sizeof CURRENT_PATH, "%s", ROOT_PATH);

	const char	*CURSOR = RELATIVE_PATH;

	while (*CURSOR == '/' || *CURSOR == ' ')
		CURSOR++;

	if (!*CURSOR)
	{
		snprintf(OUTPUT, OUTPUT_SIZE, "%s", CURRENT_PATH);
		return (0);
	}

	for (;;)
	{
		char	PATH_SEGMENT[256];
		int		SEGMENT_LENGTH = 0;

		while (*CURSOR && *CURSOR != '/' && SEGMENT_LENGTH < 255)
			PATH_SEGMENT[SEGMENT_LENGTH++] = *CURSOR++;

		PATH_SEGMENT[SEGMENT_LENGTH] = 0;

		while (SEGMENT_LENGTH && PATH_SEGMENT[SEGMENT_LENGTH - 1] == ' ')
			PATH_SEGMENT[--SEGMENT_LENGTH] = 0;

		if (
			!SEGMENT_LENGTH ||
			!strcmp(PATH_SEGMENT, ".") ||
			!strcmp(PATH_SEGMENT, "..") ||
			strchr(PATH_SEGMENT, '\\') ||
			strchr(PATH_SEGMENT, ':')
		)
			return (-1);

		int	INDEX;

		for (INDEX = 0; INDEX < SEGMENT_LENGTH; INDEX++)
			if (
				!(
					isalnum((unsigned char)PATH_SEGMENT[INDEX]) ||
					(unsigned char)PATH_SEGMENT[INDEX] >= 0X80 ||
					strchr("._- ", PATH_SEGMENT[INDEX])
				)
			)
				return (-1);

		FILE_ENTRY	ENTRIES[512];
		int			ENTRY_COUNT = FILE_SYSTEM_LIST(CURRENT_PATH, ENTRIES, 512);
		int			MATCH_INDEX = -1;

		for (INDEX = 0; INDEX < ENTRY_COUNT; INDEX++)
			if (SAME_IGNORING_CASE(ENTRIES[INDEX].NAME, PATH_SEGMENT))
				MATCH_INDEX = INDEX;

		int		IS_LAST_SEGMENT = *CURSOR == 0;
		char	NEXT_PATH[1024];

		if (MATCH_INDEX >= 0)
			snprintf(
				NEXT_PATH, sizeof NEXT_PATH, "%.700s/%.127s", CURRENT_PATH,
				ENTRIES[MATCH_INDEX].NAME
			);
		else if (IS_LAST_SEGMENT && ALLOW_NEW)
			snprintf(
				NEXT_PATH, sizeof NEXT_PATH, "%.700s/%.255s", CURRENT_PATH,
				PATH_SEGMENT
			);
		else
			return (-1);

		snprintf(CURRENT_PATH, sizeof CURRENT_PATH, "%s", NEXT_PATH);

		if (IS_LAST_SEGMENT)
			break ;

		CURSOR++;
	}

	snprintf(OUTPUT, OUTPUT_SIZE, "%s", CURRENT_PATH);

	return (0);
}

static int
	IS_TOKEN_CHARACTER(int CHARACTER)
{
	return (isalnum(CHARACTER) || CHARACTER == '.' || CHARACTER == '_');
}

int
	FORMAT_LIKE_LINES(
		const char *SOURCE_TEXT, const char *VALUE_TEXT, char *OUTPUT,
		int OUTPUT_SIZE
	)
{
	const char	*RECENT_LINES[6];
	int			RECENT_LENGTHS[6];
	int			RECENT_COUNT = 0;
	const char	*CURSOR = SOURCE_TEXT;
	const char	*ALL_LINES[4096];
	int			ALL_LENGTHS[4096];
	int			ALL_COUNT = 0;

	while (*CURSOR && ALL_COUNT < 4096)
	{
		const char	*LINE_END = strchr(CURSOR, '\n');
		int			LINE_LENGTH;

		if (LINE_END)
			LINE_LENGTH = (int)(LINE_END - CURSOR);
		else
			LINE_LENGTH = (int)strlen(CURSOR);

		while (
			LINE_LENGTH &&
			(
				CURSOR[LINE_LENGTH - 1] == '\r' ||
				CURSOR[LINE_LENGTH - 1] == ' ' ||
				CURSOR[LINE_LENGTH - 1] == '\t'
			)
		)
			LINE_LENGTH--;

		if (LINE_LENGTH && CURSOR[0] != '#')
		{
			ALL_LINES[ALL_COUNT] = CURSOR;
			ALL_LENGTHS[ALL_COUNT++] = LINE_LENGTH;
		}

		if (!LINE_END)
			break ;

		CURSOR = LINE_END + 1;
	}

	int	LINE_INDEX;

	for (
		LINE_INDEX = ALL_COUNT - 6 < 0 ? 0 : ALL_COUNT - 6;
		LINE_INDEX < ALL_COUNT;
		LINE_INDEX++
	)
	{
		RECENT_LINES[RECENT_COUNT] = ALL_LINES[LINE_INDEX];
		RECENT_LENGTHS[RECENT_COUNT++] = ALL_LENGTHS[LINE_INDEX];
	}

	if (RECENT_COUNT < 2)
		return (0);

	int	MINIMUM_LENGTH = RECENT_LENGTHS[0];

	for (LINE_INDEX = 1; LINE_INDEX < RECENT_COUNT; LINE_INDEX++)
		if (RECENT_LENGTHS[LINE_INDEX] < MINIMUM_LENGTH)
			MINIMUM_LENGTH = RECENT_LENGTHS[LINE_INDEX];

	int	PREFIX_LENGTH = 0;

	while (PREFIX_LENGTH < MINIMUM_LENGTH)
	{
		int	ALL_MATCH = 1;
		int	LINE_INDEX;

		for (LINE_INDEX = 1; LINE_INDEX < RECENT_COUNT; LINE_INDEX++)
			if (
				RECENT_LINES[LINE_INDEX][PREFIX_LENGTH] !=
					RECENT_LINES[0][PREFIX_LENGTH]
			)
				ALL_MATCH = 0;

		if (!ALL_MATCH)
			break ;

		PREFIX_LENGTH++;
	}

	int	SUFFIX_LENGTH = 0;

	while (SUFFIX_LENGTH < MINIMUM_LENGTH - PREFIX_LENGTH)
	{
		int	ALL_MATCH = 1;
		int	LINE_INDEX;

		for (LINE_INDEX = 1; LINE_INDEX < RECENT_COUNT; LINE_INDEX++)
			if (
				RECENT_LINES[LINE_INDEX]
							[RECENT_LENGTHS[LINE_INDEX] - 1 - SUFFIX_LENGTH] !=
					RECENT_LINES[0][RECENT_LENGTHS[0] - 1 - SUFFIX_LENGTH]
			)
				ALL_MATCH = 0;

		if (!ALL_MATCH)
			break ;

		SUFFIX_LENGTH++;
	}

	for (;;)
	{
		if (
			PREFIX_LENGTH == 0 ||
			!IS_TOKEN_CHARACTER(
				(unsigned char)RECENT_LINES[0][PREFIX_LENGTH - 1]
			)
		)
			break ;

		int	INSIDE_TOKEN = 0;
		int	LINE_INDEX;

		for (LINE_INDEX = 0; LINE_INDEX < RECENT_COUNT; LINE_INDEX++)
			if (
				PREFIX_LENGTH < RECENT_LENGTHS[LINE_INDEX] &&
				IS_TOKEN_CHARACTER(
					(unsigned char)RECENT_LINES[LINE_INDEX][PREFIX_LENGTH]
				)
			)
				INSIDE_TOKEN = 1;

		if (!INSIDE_TOKEN)
			break ;

		PREFIX_LENGTH--;
	}

	for (;;)
	{
		if (
			SUFFIX_LENGTH == 0 ||
			!IS_TOKEN_CHARACTER(
				(unsigned char
				)RECENT_LINES[0][RECENT_LENGTHS[0] - SUFFIX_LENGTH]
			)
		)
			break ;

		int	INSIDE_TOKEN = 0;
		int	LINE_INDEX;

		for (LINE_INDEX = 0; LINE_INDEX < RECENT_COUNT; LINE_INDEX++)
			if (
				RECENT_LENGTHS[LINE_INDEX] - SUFFIX_LENGTH - 1 >= 0 &&
				IS_TOKEN_CHARACTER(
					(unsigned char)RECENT_LINES[LINE_INDEX]
					[RECENT_LENGTHS[LINE_INDEX] -
												SUFFIX_LENGTH - 1]
				)
			)
				INSIDE_TOKEN = 1;

		if (!INSIDE_TOKEN)
			break ;

		SUFFIX_LENGTH--;
	}

	while (
		SUFFIX_LENGTH > 0 &&
		RECENT_LINES[0][RECENT_LENGTHS[0] - SUFFIX_LENGTH] == ' '
	)
	{
		int	SAME_MIDDLE = 1;
		int	LINE_INDEX;

		for (LINE_INDEX = 0; LINE_INDEX < RECENT_COUNT; LINE_INDEX++)
			if (
				RECENT_LENGTHS[LINE_INDEX] - SUFFIX_LENGTH - 1 <
					PREFIX_LENGTH ||
				RECENT_LINES[LINE_INDEX]
							[RECENT_LENGTHS[LINE_INDEX] - SUFFIX_LENGTH - 1] !=
					' '
			)
				SAME_MIDDLE = 0;

		if (SAME_MIDDLE)
			break ;

		break ;
	}

	if (PREFIX_LENGTH == 0 && SUFFIX_LENGTH == 0)
		return (0);

	int	MIDDLE_LENGTH = -1;
	int	SAME_MIDDLE = 1;
	int	ALL_IDENTICAL = 1;

	for (LINE_INDEX = 0; LINE_INDEX < RECENT_COUNT; LINE_INDEX++)
	{
		int	LINE_MIDDLE =
			RECENT_LENGTHS[LINE_INDEX] - PREFIX_LENGTH - SUFFIX_LENGTH;

		if (LINE_MIDDLE < 0)
			return (0);

		if (MIDDLE_LENGTH < 0)
			MIDDLE_LENGTH = LINE_MIDDLE;
		else if (LINE_MIDDLE != MIDDLE_LENGTH)
			SAME_MIDDLE = 0;

		if (
			LINE_INDEX &&
			(
				LINE_MIDDLE !=
					RECENT_LENGTHS[0] - PREFIX_LENGTH - SUFFIX_LENGTH ||
				strncmp(
					RECENT_LINES[LINE_INDEX] + PREFIX_LENGTH,
					RECENT_LINES[0] + PREFIX_LENGTH, LINE_MIDDLE
				)
			)
		)
			ALL_IDENTICAL = 0;
	}

	if (ALL_IDENTICAL)
		return (0);

	char	CLEAN_VALUE[512];
	int		VALUE_LENGTH = 0;

	while (*VALUE_TEXT == ' ' || *VALUE_TEXT == '"' || *VALUE_TEXT == '\'')
		VALUE_TEXT++;

	const char	*VALUE_CURSOR;

	for (
		VALUE_CURSOR = VALUE_TEXT;
		*VALUE_CURSOR && VALUE_LENGTH < 500;
		VALUE_CURSOR++
	)
		CLEAN_VALUE[VALUE_LENGTH++] = *VALUE_CURSOR;

	while (
		VALUE_LENGTH &&
		(
			CLEAN_VALUE[VALUE_LENGTH - 1] == ' ' ||
			CLEAN_VALUE[VALUE_LENGTH - 1] == '"' ||
			CLEAN_VALUE[VALUE_LENGTH - 1] == '\''
		)
	)
		VALUE_LENGTH--;

	CLEAN_VALUE[VALUE_LENGTH] = 0;

	if (!VALUE_LENGTH)
		return (0);

	if (PREFIX_LENGTH && !strncmp(CLEAN_VALUE, RECENT_LINES[0], PREFIX_LENGTH))
		return (snprintf(OUTPUT, OUTPUT_SIZE, "%s", CLEAN_VALUE) > 0);

	int	ALL_LOWERCASE = 1;

	for (LINE_INDEX = 0; LINE_INDEX < RECENT_COUNT; LINE_INDEX++)
	{
		char	FIRST_LETTER = RECENT_LINES[LINE_INDEX][PREFIX_LENGTH];

		if (!islower((unsigned char)FIRST_LETTER))
			ALL_LOWERCASE = 0;
	}

	if (
		ALL_LOWERCASE &&
		isupper((unsigned char)CLEAN_VALUE[0]) &&
		!isupper((unsigned char)CLEAN_VALUE[1])
	)
		CLEAN_VALUE[0] = (char)tolower((unsigned char)CLEAN_VALUE[0]);

	int	PADDING;

	if (SAME_MIDDLE && SUFFIX_LENGTH > 0 && VALUE_LENGTH < MIDDLE_LENGTH)
		PADDING = MIDDLE_LENGTH - VALUE_LENGTH;
	else
		PADDING = 0;

	int	WRITTEN_COUNT = snprintf(
		OUTPUT, OUTPUT_SIZE, "%.*s%s%*s%.*s", PREFIX_LENGTH, RECENT_LINES[0],
		CLEAN_VALUE, PADDING, "", SUFFIX_LENGTH,
		RECENT_LINES[0] + RECENT_LENGTHS[0] - SUFFIX_LENGTH
	);

	return (WRITTEN_COUNT > 0);
}
