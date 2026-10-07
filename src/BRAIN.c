#include "BRAIN.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#ifdef _WIN32
#include <direct.h>
#else
#include <unistd.h>
#endif

int
	MAKE_DIRECTORY(const char *PATH)
{
#ifdef _WIN32
	return (_mkdir(PATH));
#else
	return (mkdir(PATH, 0755));
#endif
}

int
	MAKE_DIRECTORY_PATH(const char *PATH)
{
	char	PARTIAL_PATH[512];

	snprintf(PARTIAL_PATH, sizeof PARTIAL_PATH, "%s", PATH);

	char	*PATH_CURSOR;

	for (PATH_CURSOR = PARTIAL_PATH + 1; *PATH_CURSOR; PATH_CURSOR++)
		if (*PATH_CURSOR == '/' || *PATH_CURSOR == '\\')
		{
			char	SAVED_CHARACTER = *PATH_CURSOR;

			*PATH_CURSOR = 0;
			MAKE_DIRECTORY(PARTIAL_PATH);
			*PATH_CURSOR = SAVED_CHARACTER;
		}

	MAKE_DIRECTORY(PARTIAL_PATH);

	return (0);
}

int
	FILE_EXISTS(const char *PATH)
{
	FILE	*STREAM = fopen(PATH, "rb");

	if (STREAM)
		fclose(STREAM);

	return (STREAM != NULL);
}

int
	COPY_FILE_CONTENTS(const char *SOURCE_PATH, const char *DESTINATION_PATH)
{
	FILE	*SOURCE_STREAM = fopen(SOURCE_PATH, "rb");

	if (!SOURCE_STREAM)
		return (-1);

	FILE	*DESTINATION_STREAM = fopen(DESTINATION_PATH, "wb");

	if (!DESTINATION_STREAM)
	{
		fclose(SOURCE_STREAM);
		return (-1);
	}

	char	COPY_BUFFER[1 << 16];
	size_t	BYTES_READ;

	while (
		(BYTES_READ =
			fread(COPY_BUFFER, 1, sizeof COPY_BUFFER, SOURCE_STREAM)) > 0
	)
		fwrite(COPY_BUFFER, 1, BYTES_READ, DESTINATION_STREAM);

	fclose(SOURCE_STREAM);
	fclose(DESTINATION_STREAM);

	return (0);
}

static int
	PATH_IS_THERE(const char *PATH)
{
	struct stat	INFORMATION;

	return (stat(PATH, &INFORMATION) == 0);
}

static int
	IS_FOLDER(const char *PATH)
{
	struct stat	INFORMATION;

	return (stat(PATH, &INFORMATION) == 0 && S_ISDIR(INFORMATION.st_mode));
}

/* FROM's things go into TO, which is there already (a kit brought newer
 * word lists): what TO doesn't have yet moves over, what it has stays as
 * it is; FROM goes when nothing is left in it.  How many moved. */
static int
	MERGE_OVER(const char *FROM, const char *TO)
{
	DIR				*FOLDER = opendir(FROM);
	struct dirent	*ENTRY;
	int				MOVED = 0;

	if (!FOLDER)
		return (0);

	while ((ENTRY = readdir(FOLDER)))
	{
		char	FROM_PATH[700];
		char	TO_PATH[700];

		if (!strcmp(ENTRY->d_name, ".") || !strcmp(ENTRY->d_name, ".."))
			continue ;

		snprintf(FROM_PATH, sizeof FROM_PATH, "%s/%s", FROM, ENTRY->d_name);
		snprintf(TO_PATH, sizeof TO_PATH, "%s/%s", TO, ENTRY->d_name);

		if (!PATH_IS_THERE(TO_PATH))
			MOVED += rename(FROM_PATH, TO_PATH) == 0;
		else if (IS_FOLDER(FROM_PATH) && IS_FOLDER(TO_PATH))
			MOVED += MERGE_OVER(FROM_PATH, TO_PATH);
	}

	closedir(FOLDER);
#ifdef _WIN32
	_rmdir(FROM);
#else
	rmdir(FROM);
#endif
	return (MOVED);
}

/* FROM becomes TO when FROM is there; when TO is there already, what it
 * doesn't have yet moves into it.  1 when anything moved. */
static int
	MOVE_OVER(const char *FROM, const char *TO, char *NEWS, int NEWS_SIZE)
{
	char	PARENT[512];
	char	*SLASH;

	if (!PATH_IS_THERE(FROM))
		return (0);

	if (PATH_IS_THERE(TO))
	{
		if (!IS_FOLDER(FROM) || !IS_FOLDER(TO) || !MERGE_OVER(FROM, TO))
			return (0);

		if (NEWS && NEWS_SIZE > 0)
		{
			size_t	LENGTH = strlen(NEWS);

			snprintf(
				NEWS + LENGTH, (size_t)NEWS_SIZE - LENGTH, "%s%s -> %s",
				LENGTH ? ", " : "", FROM, TO
			);
		}

		return (1);
	}

	snprintf(PARENT, sizeof PARENT, "%s", TO);
	SLASH = strrchr(PARENT, '/');

	if (SLASH)
	{
		*SLASH = 0;
		MAKE_DIRECTORY_PATH(PARENT);
	}

	if (rename(FROM, TO))
		return (0);

	if (NEWS && NEWS_SIZE > 0)
	{
		size_t	LENGTH = strlen(NEWS);

		snprintf(
			NEWS + LENGTH, (size_t)NEWS_SIZE - LENGTH, "%s%s -> %s",
			LENGTH ? ", " : "", FROM, TO
		);
	}

	return (1);
}

/* An older Lucy kept her things in brain, lang, memory and knowledge next to
 * code.exe.  They move, once, to where this one looks: brain ->
 * mind_original, lang -> mind/language, memory/global ->
 * mind/memory/general, memory/locals and memory/topics into mind/memory, the
 * knowledge folder's word lists into mind/language and the rest of it to
 * mind/memory/old_knowledge.  What moved is listed in NEWS. */
int
	MIND_MIGRATE(char *NEWS, int NEWS_SIZE)
{
	static const char	*WORD_LISTS[4] = {
		"verbs.txt", "days.txt", "actions.txt", NULL
	};
	int					MOVED = 0;
	int					INDEX;

	if (NEWS && NEWS_SIZE > 0)
		NEWS[0] = 0;

	MOVED += MOVE_OVER("brain", ORIGINAL_DIRECTORY, NEWS, NEWS_SIZE);
	MOVED += MOVE_OVER("lang", LANGUAGE_DIRECTORY, NEWS, NEWS_SIZE);
	MOVED += MOVE_OVER("memory/global", GLOBAL_DIRECTORY, NEWS, NEWS_SIZE);
	MOVED += MOVE_OVER("memory/locals", LOCALS_DIRECTORY, NEWS, NEWS_SIZE);
	MOVED += MOVE_OVER("memory/topics", TOPICS_DIRECTORY, NEWS, NEWS_SIZE);

	for (INDEX = 0; WORD_LISTS[INDEX]; INDEX++)
	{
		char	FROM[300];
		char	TO[300];

		snprintf(FROM, sizeof FROM, "knowledge/%s", WORD_LISTS[INDEX]);
		snprintf(TO, sizeof TO, "%s/%s", LANGUAGE_DIRECTORY, WORD_LISTS[INDEX]);

		/* the new word list wins; the old one goes with the rest */
		MOVED += MOVE_OVER(FROM, TO, NEWS, NEWS_SIZE);
	}

	MOVED += MOVE_OVER("knowledge", OLD_KNOWLEDGE_DIRECTORY, NEWS, NEWS_SIZE);

	/* an empty memory folder is left behind by the moves */
	if (PATH_IS_THERE("memory"))

#ifdef _WIN32
		_rmdir("memory");
#else
		rmdir("memory");
#endif

	return (MOVED);
}

int
	BRAIN_OPEN(BRAIN *LOADED_BRAIN, int ALLOW_START, int WANT_OPTIMIZER)
{
	int	PLAY_MODE = WANT_OPTIMIZER < 0;

	LOADED_BRAIN->LEXICON = LANGUAGE_LOAD(LANGUAGE_PATH);

	if (!LOADED_BRAIN->LEXICON)
	{
		fprintf(stderr, "cannot read %s\n", LANGUAGE_PATH);
		return (-1);
	}

	if (!FILE_EXISTS(GLOBAL_WEB_PATH) && ALLOW_START)
	{
		if (!FILE_EXISTS(START_WEB_PATH))
		{
			fprintf(
				stderr, "no brain found (%s or %s)\n", GLOBAL_WEB_PATH,
				START_WEB_PATH
			);
			return (-1);
		}

		MAKE_DIRECTORY_PATH(GLOBAL_DIRECTORY);
		COPY_FILE_CONTENTS(START_WEB_PATH, GLOBAL_WEB_PATH);
		COPY_FILE_CONTENTS(START_VOCABULARY_PATH, GLOBAL_VOCABULARY_PATH);

		if (FILE_EXISTS(START_WEB_PATH ".opt"))
			COPY_FILE_CONTENTS(START_WEB_PATH ".opt", GLOBAL_WEB_PATH ".opt");
	}

	if (PLAY_MODE)
		LOADED_BRAIN->NEURAL_WEB = WEB_LOAD_PLAY(GLOBAL_WEB_PATH);
	else
		LOADED_BRAIN->NEURAL_WEB = WEB_LOAD(GLOBAL_WEB_PATH);

	if (!LOADED_BRAIN->NEURAL_WEB)
		return (-1);

	if (WANT_OPTIMIZER > 0)
		WEB_LOAD_OPTIMIZER(LOADED_BRAIN->NEURAL_WEB, GLOBAL_WEB_PATH);

	if (LANGUAGE_LOAD_VOCABULARY(LOADED_BRAIN->LEXICON, GLOBAL_VOCABULARY_PATH))
	{
		fprintf(stderr, "cannot read %s\n", GLOBAL_VOCABULARY_PATH);
		return (-1);
	}

	if (
		LOADED_BRAIN->LEXICON->TOKEN_COUNT !=
			LOADED_BRAIN->NEURAL_WEB->WORD_COUNT
	)
	{
		fprintf(
			stderr, "vocabulary (%d) does not match brain (%d)\n",
			LOADED_BRAIN->LEXICON->TOKEN_COUNT,
			LOADED_BRAIN->NEURAL_WEB->WORD_COUNT
		);
		return (-1);
	}

	return (0);
}

int
	BRAIN_SAVE(
		BRAIN *LOADED_BRAIN, const char *WEB_PATH, const char *VOCABULARY_PATH,
		int WITH_OPTIMIZER
	)
{
	if (WEB_SAVE(LOADED_BRAIN->NEURAL_WEB, WEB_PATH, WITH_OPTIMIZER))
		return (-1);

	if (LANGUAGE_SAVE_VOCABULARY(LOADED_BRAIN->LEXICON, VOCABULARY_PATH))
		return (-1);

	if (LOADED_BRAIN->LEXICON->IS_DIRTY)
		LANGUAGE_SAVE(LOADED_BRAIN->LEXICON, LANGUAGE_PATH);

	return (0);
}

int
	BRAIN_ADD_WORD(BRAIN *LOADED_BRAIN, int32_t LEXEME)
{
	if (LOADED_BRAIN->LEXICON->LEXEME_TO_TOKEN[LEXEME] >= 0)
		return (LOADED_BRAIN->LEXICON->LEXEME_TO_TOKEN[LEXEME]);

	if (
		LOADED_BRAIN->NEURAL_WEB->WORD_COUNT >=
			LOADED_BRAIN->NEURAL_WEB->CONFIGURATION.MAXIMUM_WORDS
	)
		return (-1);

	int		NEW_WORD = WEB_ADD_WORD(LOADED_BRAIN->NEURAL_WEB);
	int32_t	TOKEN = LANGUAGE_VOCABULARY_ADD(LOADED_BRAIN->LEXICON, LEXEME);

	if (TOKEN != NEW_WORD)
	{
		fprintf(stderr, "vocab mismatch %d %d\n", TOKEN, NEW_WORD);
		exit(1);
	}

	WEB_RELINK(LOADED_BRAIN->NEURAL_WEB);

	return (TOKEN);
}

int32_t
	BRAIN_SPECIAL_TOKEN(BRAIN *LOADED_BRAIN, const char *SPECIAL_NAME)
{
	int32_t	LEXEME_ID = LANGUAGE_FIND(LOADED_BRAIN->LEXICON, SPECIAL_NAME);

	if (LEXEME_ID < 0)
		LEXEME_ID = LANGUAGE_ADD(LOADED_BRAIN->LEXICON, SPECIAL_NAME, 0);

	if (LOADED_BRAIN->LEXICON->LEXEME_TO_TOKEN[LEXEME_ID] >= 0)
		return (LOADED_BRAIN->LEXICON->LEXEME_TO_TOKEN[LEXEME_ID]);

	if (!LOADED_BRAIN->NEURAL_WEB || LOADED_BRAIN->NEURAL_WEB->HALF_PRECISION)
		return (-1);

	return (BRAIN_ADD_WORD(LOADED_BRAIN, LEXEME_ID));
}
