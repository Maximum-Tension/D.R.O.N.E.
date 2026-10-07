#include "LANGUAGE.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static uint32_t
	HASH_STRING(const char *STRING)
{
	uint32_t	HASH = (unsigned int)2166136261;

	while (*STRING)
	{
		HASH ^= (uint8_t)*STRING++;
		HASH *= (unsigned int)16777619;
	}

	return (HASH);
}

static void
	LANGUAGE_REHASH(LANGUAGE *LEXICON)
{
	int32_t	NEW_CAPACITY;

	if (LEXICON->HASH_CAPACITY)
		NEW_CAPACITY = LEXICON->HASH_CAPACITY * 2;
	else
		NEW_CAPACITY = 1 << 16;

	free(LEXICON->HASH_TABLE);
	LEXICON->HASH_TABLE = malloc(NEW_CAPACITY * sizeof(int32_t));

	int32_t	INDEX;

	for (INDEX = 0; INDEX < NEW_CAPACITY; INDEX++)
		LEXICON->HASH_TABLE[INDEX] = -1;

	LEXICON->HASH_CAPACITY = NEW_CAPACITY;

	for (INDEX = 0; INDEX < LEXICON->COUNT; INDEX++)
	{
		uint32_t	SLOT =
			HASH_STRING(LEXICON->STRINGS[INDEX]) & (NEW_CAPACITY - 1);

		while (LEXICON->HASH_TABLE[SLOT] >= 0)
			SLOT = (SLOT + 1) & (NEW_CAPACITY - 1);

		LEXICON->HASH_TABLE[SLOT] = INDEX;
	}
}

LANGUAGE
	*LANGUAGE_NEW(void)
{
	LANGUAGE	*LEXICON = calloc(1, sizeof(LANGUAGE));

	LANGUAGE_REHASH(LEXICON);
	LEXICON->TOKEN_CAPACITY = 1024;
	LEXICON->TOKEN_TO_LEXEME =
		malloc(LEXICON->TOKEN_CAPACITY * sizeof(int32_t));

	int	TOKEN_INDEX;

	for (TOKEN_INDEX = 0; TOKEN_INDEX < TOKEN_FIRST_WORD; TOKEN_INDEX++)
		LEXICON->TOKEN_TO_LEXEME[TOKEN_INDEX] = -1;

	LEXICON->TOKEN_COUNT = TOKEN_FIRST_WORD;

	return (LEXICON);
}

static char
	*ARENA_ALLOCATE(LANGUAGE *LEXICON, size_t BYTE_COUNT)
{
	if (!LEXICON->BLOCK_COUNT || LEXICON->BLOCK_USED + BYTE_COUNT > 65536)
	{
		LEXICON->BLOCKS = realloc(
			LEXICON->BLOCKS, (LEXICON->BLOCK_COUNT + 1) * sizeof(char *)
		);
		LEXICON->BLOCKS[LEXICON->BLOCK_COUNT++] = malloc(65536);
		LEXICON->BLOCK_USED = 0;
	}

	char	*ALLOCATION =
		LEXICON->BLOCKS[LEXICON->BLOCK_COUNT - 1] + LEXICON->BLOCK_USED;

	LEXICON->BLOCK_USED += (int32_t)BYTE_COUNT;

	return (ALLOCATION);
}

void
	LANGUAGE_FREE(LANGUAGE *LEXICON)
{
	if (!LEXICON)
		return ;

	int	BLOCK_INDEX;

	for (BLOCK_INDEX = 0; BLOCK_INDEX < LEXICON->BLOCK_COUNT; BLOCK_INDEX++)
		free(LEXICON->BLOCKS[BLOCK_INDEX]);

	free(LEXICON->BLOCKS);
	free(LEXICON->STRINGS);
	free(LEXICON->USE_COUNTS);
	free(LEXICON->CAPITAL_COUNTS);
	free(LEXICON->HASH_TABLE);
	free(LEXICON->LEXEME_TO_TOKEN);
	free(LEXICON->TOKEN_TO_LEXEME);
	free(LEXICON);
}

int32_t
	LANGUAGE_FIND(LANGUAGE *LEXICON, const char *STRING)
{
	uint32_t	SLOT = HASH_STRING(STRING) & (LEXICON->HASH_CAPACITY - 1);

	while (LEXICON->HASH_TABLE[SLOT] >= 0)
	{
		if (!strcmp(LEXICON->STRINGS[LEXICON->HASH_TABLE[SLOT]], STRING))
			return (LEXICON->HASH_TABLE[SLOT]);

		SLOT = (SLOT + 1) & (LEXICON->HASH_CAPACITY - 1);
	}

	return (-1);
}

int32_t
	LANGUAGE_ADD(LANGUAGE *LEXICON, const char *STRING, int IS_CAPITALIZED)
{
	int32_t	LEXEME_ID = LANGUAGE_FIND(LEXICON, STRING);

	if (LEXEME_ID >= 0)
	{
		LEXICON->USE_COUNTS[LEXEME_ID]++;

		if (IS_CAPITALIZED)
			LEXICON->CAPITAL_COUNTS[LEXEME_ID]++;

		return (LEXEME_ID);
	}

	if (LEXICON->COUNT >= LEXICON->CAPACITY)
	{
		if (LEXICON->CAPACITY)
			LEXICON->CAPACITY = LEXICON->CAPACITY * 2;
		else
			LEXICON->CAPACITY = 4096;

		LEXICON->STRINGS =
			realloc(LEXICON->STRINGS, LEXICON->CAPACITY * sizeof(char *));
		LEXICON->USE_COUNTS =
			realloc(LEXICON->USE_COUNTS, LEXICON->CAPACITY * sizeof(uint32_t));
		LEXICON->CAPITAL_COUNTS = realloc(
			LEXICON->CAPITAL_COUNTS, LEXICON->CAPACITY * sizeof(uint32_t)
		);
		LEXICON->LEXEME_TO_TOKEN = realloc(
			LEXICON->LEXEME_TO_TOKEN, LEXICON->CAPACITY * sizeof(int32_t)
		);
	}

	if ((LEXICON->COUNT + 1) * 2 > LEXICON->HASH_CAPACITY)
		LANGUAGE_REHASH(LEXICON);

	LEXEME_ID = LEXICON->COUNT++;
	LEXICON->STRINGS[LEXEME_ID] = ARENA_ALLOCATE(LEXICON, strlen(STRING) + 1);
	strcpy(LEXICON->STRINGS[LEXEME_ID], STRING);
	LEXICON->USE_COUNTS[LEXEME_ID] = 1;

	if (IS_CAPITALIZED)
		LEXICON->CAPITAL_COUNTS[LEXEME_ID] = 1;
	else
		LEXICON->CAPITAL_COUNTS[LEXEME_ID] = 0;

	LEXICON->LEXEME_TO_TOKEN[LEXEME_ID] = -1;

	uint32_t	SLOT = HASH_STRING(STRING) & (LEXICON->HASH_CAPACITY - 1);

	while (LEXICON->HASH_TABLE[SLOT] >= 0)
		SLOT = (SLOT + 1) & (LEXICON->HASH_CAPACITY - 1);

	LEXICON->HASH_TABLE[SLOT] = LEXEME_ID;
	LEXICON->IS_DIRTY = 1;

	return (LEXEME_ID);
}

int32_t
	LANGUAGE_VOCABULARY_ADD(LANGUAGE *LEXICON, int32_t LEXEME)
{
	if (LEXEME < 0 || LEXEME >= LEXICON->COUNT)
		return (-1);

	if (LEXICON->LEXEME_TO_TOKEN[LEXEME] >= 0)
		return (LEXICON->LEXEME_TO_TOKEN[LEXEME]);

	if (LEXICON->TOKEN_COUNT >= LEXICON->TOKEN_CAPACITY)
	{
		LEXICON->TOKEN_CAPACITY *= 2;
		LEXICON->TOKEN_TO_LEXEME = realloc(
			LEXICON->TOKEN_TO_LEXEME, LEXICON->TOKEN_CAPACITY * sizeof(int32_t)
		);
	}

	LEXICON->TOKEN_TO_LEXEME[LEXICON->TOKEN_COUNT] = LEXEME;
	LEXICON->LEXEME_TO_TOKEN[LEXEME] = LEXICON->TOKEN_COUNT;

	return (LEXICON->TOKEN_COUNT++);
}

int
	LANGUAGE_SAVE(LANGUAGE *LEXICON, const char *PATH)
{
	FILE	*STREAM = fopen(PATH, "wb");

	if (!STREAM)
		return (-1);

	fwrite("CODELEX1", 1, 8, STREAM);
	fwrite(&LEXICON->COUNT, 4, 1, STREAM);

	int32_t	LEXEME_ID;

	for (LEXEME_ID = 0; LEXEME_ID < LEXICON->COUNT; LEXEME_ID++)
	{
		uint8_t	WORD_LENGTH = (uint8_t)strlen(LEXICON->STRINGS[LEXEME_ID]);

		fwrite(&LEXEME_ID, 4, 1, STREAM);
		fwrite(&LEXICON->USE_COUNTS[LEXEME_ID], 4, 1, STREAM);
		fwrite(&LEXICON->CAPITAL_COUNTS[LEXEME_ID], 4, 1, STREAM);
		fwrite(&WORD_LENGTH, 1, 1, STREAM);
		fwrite(LEXICON->STRINGS[LEXEME_ID], 1, WORD_LENGTH, STREAM);
	}

	int	SUCCESS = !ferror(STREAM);

	fclose(STREAM);
	LEXICON->IS_DIRTY = 0;

	if (SUCCESS)
		return (0);

	return (-1);
}

LANGUAGE
	*LANGUAGE_LOAD(const char *PATH)
{
	FILE	*STREAM = fopen(PATH, "rb");

	if (!STREAM)
		return (NULL);

	char		MAGIC[8];
	int32_t		LEXEME_COUNT;
	LANGUAGE	*LEXICON = LANGUAGE_NEW();

	if (
		fread(MAGIC, 1, 8, STREAM) != 8 ||
		memcmp(MAGIC, "CODELEX1", 8) ||
		fread(&LEXEME_COUNT, 4, 1, STREAM) != 1
	)
	{
		fclose(STREAM);
		LANGUAGE_FREE(LEXICON);
		return (NULL);
	}

	int32_t	ENTRY_INDEX;

	for (ENTRY_INDEX = 0; ENTRY_INDEX < LEXEME_COUNT; ENTRY_INDEX++)
	{
		int32_t		SAVED_ID;
		uint32_t	USE_COUNT;
		uint32_t	CAPITAL_COUNT;
		uint8_t		WORD_LENGTH;
		char		WORD_BUFFER[256];

		if (
			fread(&SAVED_ID, 4, 1, STREAM) != 1 ||
			fread(&USE_COUNT, 4, 1, STREAM) != 1 ||
			fread(&CAPITAL_COUNT, 4, 1, STREAM) != 1 ||
			fread(&WORD_LENGTH, 1, 1, STREAM) != 1 ||
			fread(WORD_BUFFER, 1, WORD_LENGTH, STREAM) != WORD_LENGTH
		)
			break ;

		WORD_BUFFER[WORD_LENGTH] = 0;

		int32_t	LEXEME_ID = LANGUAGE_ADD(LEXICON, WORD_BUFFER, 0);

		LEXICON->USE_COUNTS[LEXEME_ID] = USE_COUNT;
		LEXICON->CAPITAL_COUNTS[LEXEME_ID] = CAPITAL_COUNT;
	}

	fclose(STREAM);
	LEXICON->IS_DIRTY = 0;

	return (LEXICON);
}

int
	LANGUAGE_SAVE_VOCABULARY(LANGUAGE *LEXICON, const char *PATH)
{
	FILE	*STREAM = fopen(PATH, "wb");

	if (!STREAM)
		return (-1);

	fwrite("CODEVOC1", 1, 8, STREAM);
	fwrite(&LEXICON->TOKEN_COUNT, 4, 1, STREAM);
	fwrite(LEXICON->TOKEN_TO_LEXEME, 4, LEXICON->TOKEN_COUNT, STREAM);
	fclose(STREAM);

	return (0);
}

int
	LANGUAGE_LOAD_VOCABULARY(LANGUAGE *LEXICON, const char *PATH)
{
	FILE	*STREAM = fopen(PATH, "rb");

	if (!STREAM)
		return (-1);

	char	MAGIC[8];
	int32_t	TOKEN_COUNT;

	if (
		fread(MAGIC, 1, 8, STREAM) != 8 ||
		memcmp(MAGIC, "CODEVOC1", 8) ||
		fread(&TOKEN_COUNT, 4, 1, STREAM) != 1 ||
		TOKEN_COUNT < TOKEN_FIRST_WORD
	)
	{
		fclose(STREAM);
		return (-1);
	}

	int32_t	*TOKEN_LEXEMES = malloc(TOKEN_COUNT * 4);

	if (fread(TOKEN_LEXEMES, 4, TOKEN_COUNT, STREAM) != (size_t)TOKEN_COUNT)
	{
		free(TOKEN_LEXEMES);
		fclose(STREAM);
		return (-1);
	}

	fclose(STREAM);

	int32_t	INDEX;

	for (INDEX = 0; INDEX < LEXICON->COUNT; INDEX++)
		LEXICON->LEXEME_TO_TOKEN[INDEX] = -1;

	LEXICON->TOKEN_COUNT = TOKEN_FIRST_WORD;

	for (INDEX = TOKEN_FIRST_WORD; INDEX < TOKEN_COUNT; INDEX++)
	{
		if (TOKEN_LEXEMES[INDEX] < 0 || TOKEN_LEXEMES[INDEX] >= LEXICON->COUNT)
		{
			free(TOKEN_LEXEMES);
			return (-1);
		}

		LANGUAGE_VOCABULARY_ADD(LEXICON, TOKEN_LEXEMES[INDEX]);
	}

	free(TOKEN_LEXEMES);

	return (0);
}

static int
	IS_PUNCTUATION(int CHARACTER)
{
	return (CHARACTER && strchr(".,!?;:\"()-&%$+=/*#@^", CHARACTER) != NULL);
}

static int
	IS_FILE_EXTENSION(const char *EXTENSION)
{
	static const char	*KNOWN_EXTENSIONS[49] = {
		"txt", "com", "md", "exe", "db", "ini", "json", "csv", "log", "cfg",
		"xml", "html", "htm", "py", "c", "h", "js", "png", "jpg", "jpeg", "gif",
		"bat", "dat", "bin", "zip", "pdf", "doc", "docx", "mp3", "wav", "cpp",
		"ts", "yml", "yaml", "toml", "css", "java", "rtf", "xls", "xlsx", "ppt",
		"pptx", "mp4", "sh", "net", "org", "dll", "mem", "lex"
	};
	size_t				EXTENSION_INDEX;

	for (
		EXTENSION_INDEX = 0;
		EXTENSION_INDEX < sizeof KNOWN_EXTENSIONS / sizeof KNOWN_EXTENSIONS[0];
		EXTENSION_INDEX++
	)
		if (!strcmp(EXTENSION, KNOWN_EXTENSIONS[EXTENSION_INDEX]))
			return (1);

	return (0);
}

int
	SPLIT_WORDS(const char *INPUT_TEXT, SPLIT_WORD *WORDS, int MAXIMUM_WORDS)
{
	int				WORD_COUNT = 0;
	const uint8_t	*CURSOR = (const uint8_t *)INPUT_TEXT;

	while (*CURSOR && WORD_COUNT < MAXIMUM_WORDS)
	{
		int	CHARACTER = *CURSOR;

		if (CHARACTER == 0XE2 && CURSOR[1] == 0X80)
		{
			int		THIRD_BYTE = CURSOR[2];
			char	REPLACEMENT = 0;

			if (THIRD_BYTE == 0X98 || THIRD_BYTE == 0X99)
				REPLACEMENT = '\'';
			else if (THIRD_BYTE == 0X9C || THIRD_BYTE == 0X9D)
				REPLACEMENT = '"';
			else if (THIRD_BYTE == 0X93 || THIRD_BYTE == 0X94)
				REPLACEMENT = '-';
			else if (THIRD_BYTE == 0XA6)
			{
				strcpy(WORDS[WORD_COUNT].WORD_TEXT, "...");
				WORDS[WORD_COUNT++].IS_CAPITALIZED = 0;
				CURSOR += 3;
				continue ;
			}

			CURSOR += 3;

			if (
				REPLACEMENT == '\'' &&
				WORD_COUNT > 0 &&
				isalnum(CURSOR[0]) &&
				isalnum(CURSOR[-4])
			)
			{
				SPLIT_WORD	*PREVIOUS_WORD = &WORDS[WORD_COUNT - 1];
				size_t		WORD_LENGTH = strlen(PREVIOUS_WORD->WORD_TEXT);

				if (WORD_LENGTH < 60)
				{
					PREVIOUS_WORD->WORD_TEXT[WORD_LENGTH] = '\'';
					PREVIOUS_WORD->WORD_TEXT[WORD_LENGTH + 1] = 0;
				}

				while (
					isalnum(*CURSOR) &&
					strlen(PREVIOUS_WORD->WORD_TEXT) < 62
				)
				{
					size_t	END_POSITION = strlen(PREVIOUS_WORD->WORD_TEXT);

					PREVIOUS_WORD->WORD_TEXT[END_POSITION] =
						(char)tolower(*CURSOR);
					PREVIOUS_WORD->WORD_TEXT[END_POSITION + 1] = 0;
					CURSOR++;
				}

				continue ;
			}

			if (REPLACEMENT && REPLACEMENT != '\'')
			{
				WORDS[WORD_COUNT].WORD_TEXT[0] = REPLACEMENT;
				WORDS[WORD_COUNT].WORD_TEXT[1] = 0;
				WORDS[WORD_COUNT++].IS_CAPITALIZED = 0;
			}

			continue ;
		}

		if (CHARACTER >= 0X80)
		{
			CURSOR++;
			continue ;
		}

		if (isalnum(CHARACTER))
		{
			int	WORD_LENGTH = 0;

			if (isupper(CHARACTER))
				WORDS[WORD_COUNT].IS_CAPITALIZED = 1;
			else
				WORDS[WORD_COUNT].IS_CAPITALIZED = 0;

			while (
				*CURSOR &&
				(
					isalnum(*CURSOR) ||
					(*CURSOR == '\'' && isalpha(CURSOR[1]) && WORD_LENGTH > 0)
				)
			)
			{
				if (WORD_LENGTH < 63)
					WORDS[WORD_COUNT].WORD_TEXT[WORD_LENGTH++] =
						(char)tolower(*CURSOR);

				CURSOR++;
			}

			WORDS[WORD_COUNT].WORD_TEXT[WORD_LENGTH] = 0;
			WORD_COUNT++;
			continue ;
		}

		if (CHARACTER == '.' && CURSOR[1] == '.' && CURSOR[2] == '.')
		{
			strcpy(WORDS[WORD_COUNT].WORD_TEXT, "...");
			WORDS[WORD_COUNT++].IS_CAPITALIZED = 0;

			while (*CURSOR == '.')
				CURSOR++;

			continue ;
		}

		if (IS_PUNCTUATION(CHARACTER))
		{
			WORDS[WORD_COUNT].WORD_TEXT[0] = (char)CHARACTER;
			WORDS[WORD_COUNT].WORD_TEXT[1] = 0;
			WORDS[WORD_COUNT++].IS_CAPITALIZED = 0;
		}

		CURSOR++;
	}

	return (WORD_COUNT);
}

static int
	CHARACTER_INDEX(int CHARACTER)
{
	if (CHARACTER >= 'a' && CHARACTER <= 'z')
		return (CHARACTER - 'a');

	if (CHARACTER >= '0' && CHARACTER <= '9')
		return (26 + CHARACTER - '0');

	if (CHARACTER == '\'')
		return (36);

	return (-1);
}

static const char	TOKEN_CHARACTERS[38] =
	"abcdefghijklmnopqrstuvwxyz0123456789'";

int
	ENCODE_WORDS(
		LANGUAGE *LEXICON, SPLIT_WORD *WORDS, int WORD_COUNT, int32_t *OUTPUT,
		int OUTPUT_SIZE, int LEARN_NEW
	)
{
	int	TOKEN_COUNT = 0;
	int	WORD_INDEX;

	for (
		WORD_INDEX = 0;
		WORD_INDEX < WORD_COUNT && TOKEN_COUNT < OUTPUT_SIZE;
		WORD_INDEX++
	)
	{
		int32_t	LEXEME_ID;

		if (LEARN_NEW)
			LEXEME_ID = LANGUAGE_ADD(
				LEXICON, WORDS[WORD_INDEX].WORD_TEXT,
				WORDS[WORD_INDEX].IS_CAPITALIZED
			);
		else
			LEXEME_ID = LANGUAGE_FIND(LEXICON, WORDS[WORD_INDEX].WORD_TEXT);

		int32_t	TOKEN;

		if (LEXEME_ID >= 0)
			TOKEN = LEXICON->LEXEME_TO_TOKEN[LEXEME_ID];
		else
			TOKEN = -1;

		if (TOKEN >= 0)
		{
			OUTPUT[TOKEN_COUNT++] = TOKEN;
			continue ;
		}

		int			IS_FIRST = 1;
		const char	*CURSOR;

		for (
			CURSOR = WORDS[WORD_INDEX].WORD_TEXT;
			*CURSOR && TOKEN_COUNT < OUTPUT_SIZE;
			CURSOR++
		)
		{
			int	LETTER_CODE = CHARACTER_INDEX(*CURSOR);

			if (LETTER_CODE < 0)
				continue ;

			OUTPUT[TOKEN_COUNT++] = TOKEN_FIRST_CHARACTER +
				(IS_FIRST ? 0 : CHARACTER_COUNT) + LETTER_CODE;
			IS_FIRST = 0;
		}
	}

	return (TOKEN_COUNT);
}

int
	ENCODE_TEXT(
		LANGUAGE *LEXICON, const char *INPUT_TEXT, int32_t *OUTPUT,
		int OUTPUT_SIZE, int LEARN_NEW
	)
{
	SPLIT_WORD	WORDS[512];
	int			WORD_COUNT = SPLIT_WORDS(INPUT_TEXT, WORDS, 512);

	return (
		ENCODE_WORDS(LEXICON, WORDS, WORD_COUNT, OUTPUT, OUTPUT_SIZE, LEARN_NEW)
	);
}

int32_t
	LANGUAGE_TOKEN(LANGUAGE *LEXICON, const char *STRING)
{
	int32_t	LEXEME_ID = LANGUAGE_FIND(LEXICON, STRING);

	if (LEXEME_ID >= 0)
		return (LEXICON->LEXEME_TO_TOKEN[LEXEME_ID]);

	return (-1);
}

int
	IS_SPECIAL_WORD(LANGUAGE *LEXICON, int32_t LEXEME)
{
	return (
		LEXEME >= 0 &&
		LEXEME < LEXICON->COUNT &&
		LEXICON->STRINGS[LEXEME][0] == '<'
	);
}

int
	IS_PROPER_NOUN(LANGUAGE *LEXICON, int32_t LEXEME)
{
	return (
		LEXEME >= 0 &&
		LEXICON->USE_COUNTS[LEXEME] >= 2 &&
		LEXICON->CAPITAL_COUNTS[LEXEME] * 10 >= LEXICON->USE_COUNTS[LEXEME] * 7
	);
}

const char
	*TOKEN_NAME(LANGUAGE *LEXICON, int32_t TOKEN, char *BUFFER)
{
	static const char	*SPECIAL_NAMES[5] = {
		"<text>", "<u>", "<c>", "<mem>", "<you>"
	};

	if (TOKEN < TOKEN_FIRST_CHARACTER)
		return (SPECIAL_NAMES[TOKEN]);

	if (TOKEN < TOKEN_FIRST_CHARACTER + CHARACTER_COUNT)
	{
		sprintf(BUFFER, "^%c", TOKEN_CHARACTERS[TOKEN - TOKEN_FIRST_CHARACTER]);
		return (BUFFER);
	}

	if (TOKEN < TOKEN_FIRST_WORD)
	{
		sprintf(
			BUFFER, "%c",
			TOKEN_CHARACTERS[TOKEN - TOKEN_FIRST_CHARACTER - CHARACTER_COUNT]
		);
		return (BUFFER);
	}

	if (TOKEN < LEXICON->TOKEN_COUNT)
		return (LEXICON->STRINGS[LEXICON->TOKEN_TO_LEXEME[TOKEN]]);

	return ("?");
}

static int
	ATTACHES_LEFT(const char *WORD_TEXT)
{
	return (
		!strcmp(WORD_TEXT, ".") ||
		!strcmp(WORD_TEXT, ",") ||
		!strcmp(WORD_TEXT, "!") ||
		!strcmp(WORD_TEXT, "?") ||
		!strcmp(WORD_TEXT, ";") ||
		!strcmp(WORD_TEXT, ":") ||
		!strcmp(WORD_TEXT, ")") ||
		!strcmp(WORD_TEXT, "...") ||
		!strcmp(WORD_TEXT, "%")
	);
}

int
	DECODE_TOKENS(
		LANGUAGE *LEXICON, const int32_t *TOKENS, int TOKEN_COUNT, char *OUTPUT,
		int OUTPUT_SIZE, const char **KNOWN_NAMES, int NAME_COUNT
	)
{
	char	WORDS[256][64];
	int32_t	LEXEMES[256];
	int		WORD_COUNT = 0;
	int		INDEX;

	for (INDEX = 0; INDEX < TOKEN_COUNT && WORD_COUNT < 256; INDEX++)
	{
		int32_t	TOKEN = TOKENS[INDEX];

		if (
			TOKEN >= TOKEN_FIRST_CHARACTER &&
			TOKEN < TOKEN_FIRST_CHARACTER + CHARACTER_COUNT
		)
		{
			WORDS[WORD_COUNT][0] =
				TOKEN_CHARACTERS[TOKEN - TOKEN_FIRST_CHARACTER];
			WORDS[WORD_COUNT][1] = 0;
			LEXEMES[WORD_COUNT++] = -2;
			continue ;
		}

		if (
			TOKEN >= TOKEN_FIRST_CHARACTER + CHARACTER_COUNT &&
			TOKEN < TOKEN_FIRST_WORD
		)
		{
			if (WORD_COUNT > 0 && LEXEMES[WORD_COUNT - 1] == -2)
			{
				size_t	WORD_LENGTH = strlen(WORDS[WORD_COUNT - 1]);

				if (WORD_LENGTH < 62)
				{
					WORDS[WORD_COUNT - 1][WORD_LENGTH] = TOKEN_CHARACTERS
						[TOKEN - TOKEN_FIRST_CHARACTER - CHARACTER_COUNT];
					WORDS[WORD_COUNT - 1][WORD_LENGTH + 1] = 0;
				}
			}
			else
			{
				WORDS[WORD_COUNT][0] = TOKEN_CHARACTERS
					[TOKEN - TOKEN_FIRST_CHARACTER - CHARACTER_COUNT];
				WORDS[WORD_COUNT][1] = 0;
				LEXEMES[WORD_COUNT++] = -2;
			}

			continue ;
		}

		if (
			TOKEN >= TOKEN_FIRST_WORD &&
			TOKEN < LEXICON->TOKEN_COUNT &&
			IS_SPECIAL_WORD(LEXICON, LEXICON->TOKEN_TO_LEXEME[TOKEN])
		)
			continue ;

		if (TOKEN >= TOKEN_FIRST_WORD && TOKEN < LEXICON->TOKEN_COUNT)
		{
			snprintf(
				WORDS[WORD_COUNT], 64, "%s",
				LEXICON->STRINGS[LEXICON->TOKEN_TO_LEXEME[TOKEN]]
			);
			LEXEMES[WORD_COUNT++] = LEXICON->TOKEN_TO_LEXEME[TOKEN];
		}
	}

	int	OUTPUT_LENGTH = 0;
	int	SENTENCE_START = 1;
	int	IN_QUOTE = 0;
	int	NO_SPACE = 1;

	OUTPUT[0] = 0;

	for (INDEX = 0; INDEX < WORD_COUNT; INDEX++)
	{
		char	WORD_TEXT[64];

		strcpy(WORD_TEXT, WORDS[INDEX]);

		int	IS_NAME = 0;
		int	NAME_INDEX;

		for (NAME_INDEX = 0; NAME_INDEX < NAME_COUNT; NAME_INDEX++)
			if (
				KNOWN_NAMES[NAME_INDEX] &&
				!strcmp(KNOWN_NAMES[NAME_INDEX], WORD_TEXT)
			)
				IS_NAME = 1;

		if (
			!strcmp(WORD_TEXT, "i") ||
			!strncmp(WORD_TEXT, "i'", 2) ||
			IS_NAME ||
			(LEXEMES[INDEX] >= 0 && IS_PROPER_NOUN(LEXICON, LEXEMES[INDEX]))
		)
			WORD_TEXT[0] = (char)toupper(WORD_TEXT[0]);

		if (SENTENCE_START && isalnum((uint8_t)WORD_TEXT[0]))
		{
			WORD_TEXT[0] = (char)toupper(WORD_TEXT[0]);
			SENTENCE_START = 0;
		}

		int	ADD_SPACE = !NO_SPACE && !ATTACHES_LEFT(WORD_TEXT);

		if (!strcmp(WORD_TEXT, "\""))
		{
			if (IN_QUOTE)
				ADD_SPACE = 0;

			IN_QUOTE = !IN_QUOTE;
		}

		if (!strcmp(WORD_TEXT, "-") && INDEX > 0 && INDEX + 1 < WORD_COUNT)
			ADD_SPACE = 0;

		if (
			!strcmp(WORD_TEXT, "/") &&
			INDEX > 0 &&
			INDEX + 1 < WORD_COUNT &&
			isalnum((uint8_t)WORDS[INDEX - 1][0]) &&
			isalnum((uint8_t)WORDS[INDEX + 1][0])
		)
		{
			static const char	*COMMAND_WORDS[11] = {
				"type", "use", "enter", "press", "try", "run", "write", "with",
				"or", "and", NULL
			};
			char				PREVIOUS_WORD[64];

			snprintf(
				PREVIOUS_WORD, sizeof PREVIOUS_WORD, "%s", WORDS[INDEX - 1]
			);

			char	*CURSOR;

			for (CURSOR = PREVIOUS_WORD; *CURSOR; CURSOR++)
				*CURSOR = (char)tolower((uint8_t)*CURSOR);

			int	IS_COMMAND = 0;
			int	COMMAND_INDEX;

			for (
				COMMAND_INDEX = 0;
				COMMAND_WORDS[COMMAND_INDEX];
				COMMAND_INDEX++
			)
				if (!strcmp(PREVIOUS_WORD, COMMAND_WORDS[COMMAND_INDEX]))
					IS_COMMAND = 1;

			if (!IS_COMMAND)
				ADD_SPACE = 0;
		}

		if (INDEX > 0 && !strcmp(WORDS[INDEX - 1], "-") && INDEX > 1)
			ADD_SPACE = 0;

		int	WORD_LENGTH = (int)strlen(WORD_TEXT);

		if (OUTPUT_LENGTH + WORD_LENGTH + 2 >= OUTPUT_SIZE)
			break ;

		if (ADD_SPACE)
			OUTPUT[OUTPUT_LENGTH++] = ' ';

		memcpy(OUTPUT + OUTPUT_LENGTH, WORD_TEXT, WORD_LENGTH);
		OUTPUT_LENGTH += WORD_LENGTH;
		OUTPUT[OUTPUT_LENGTH] = 0;
		NO_SPACE =
			(!strcmp(WORD_TEXT, "\"") && IN_QUOTE) || !strcmp(WORD_TEXT, "(");

		/* "$0.05" */
		if (
			!strcmp(WORD_TEXT, "$") &&
			INDEX + 1 < WORD_COUNT &&
			isdigit((uint8_t)WORDS[INDEX + 1][0])
		)
			NO_SPACE = 1;

		if (
			(!strcmp(WORD_TEXT, ".") || !strcmp(WORD_TEXT, ":")) &&
			INDEX > 0 &&
			INDEX + 1 < WORD_COUNT &&
			isdigit((uint8_t)WORDS[INDEX - 1][0]) &&
			isdigit((uint8_t)WORDS[INDEX + 1][0])
		)
		{
			NO_SPACE = 1;
			SENTENCE_START = 0;
			continue ;
		}

		if (
			!strcmp(WORD_TEXT, ".") &&
			INDEX > 0 &&
			INDEX + 1 < WORD_COUNT &&
			isalnum((uint8_t)WORDS[INDEX - 1][0]) &&
			IS_FILE_EXTENSION(WORDS[INDEX + 1])
		)
		{
			NO_SPACE = 1;
			SENTENCE_START = 0;
			continue ;
		}

		if (
			!strcmp(WORD_TEXT, "/") &&
			INDEX > 0 &&
			INDEX + 1 < WORD_COUNT &&
			isalnum((uint8_t)WORDS[INDEX - 1][0]) &&
			isalnum((uint8_t)WORDS[INDEX + 1][0])
		)
		{
			static const char	*COMMAND_VERBS[11] = {
				"type", "use", "enter", "press", "try", "run", "write", "with",
				"or", "and", NULL
			};
			char				PREVIOUS_WORD[64];

			snprintf(
				PREVIOUS_WORD, sizeof PREVIOUS_WORD, "%s", WORDS[INDEX - 1]
			);

			char	*CURSOR;

			for (CURSOR = PREVIOUS_WORD; *CURSOR; CURSOR++)
				*CURSOR = (char)tolower((uint8_t)*CURSOR);

			int	IS_COMMAND = 0;
			int	COMMAND_INDEX;

			for (
				COMMAND_INDEX = 0;
				COMMAND_VERBS[COMMAND_INDEX];
				COMMAND_INDEX++
			)
				if (!strcmp(PREVIOUS_WORD, COMMAND_VERBS[COMMAND_INDEX]))
					IS_COMMAND = 1;

			(void)IS_COMMAND;
			NO_SPACE =
				1; /* "a/b" and "type /quit": never a space after the slash */
			continue ;
		}

		if (
			!strcmp(WORD_TEXT, ".") ||
			!strcmp(WORD_TEXT, "!") ||
			!strcmp(WORD_TEXT, "?") ||
			!strcmp(WORD_TEXT, "...")
		)
			SENTENCE_START = 1;
	}

	return (OUTPUT_LENGTH);
}
