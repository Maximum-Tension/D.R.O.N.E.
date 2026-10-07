#include "SOLVE.h"
#include <ctype.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Equations with unknowns, solved exactly.  A message is read two ways: as
 * math ("[Bat] + [Ball] = 1.10 and [Ball] + 1 = [Bat]", "2x + 3 = 7") and,
 * when it asks for something, as a word problem ("A bat and a ball cost $1.10
 * together. The bat costs $1 more than the ball.").  Every equation becomes
 * a row of numbers (coefficients and the constant), and asking for an
 * unknown runs elimination on the rows: an answer is only given when the
 * rows decide it. */

enum
{
	TOKEN_NUMBER = 0,
	TOKEN_UNKNOWN,
	TOKEN_OPERATOR,
	TOKEN_WORD,
	TOKEN_STOP
};

typedef struct
{
	int		KIND;
	double	VALUE;
	char	TEXT[48];
	char	SHOWN[56];
	int		IS_MONEY;
	int		IS_JOINED;
} SOLVE_TOKEN;

typedef struct
{
	double	COEFFICIENTS[SOLVE_MAX_NAMES];
	double	CONSTANT;
} LINEAR;

typedef struct
{
	SOLVE_TOKEN	*TOKENS;
	int			COUNT;
	int			POSITION;
	SOLVE_STORE	*STORE;
	int			FAILED;
} SOLVE_PARSER;

#define SOLVE_MAX_TOKENS 400

/* strstr that ignores case (strcasestr is not everywhere) */
char
	*FIND_NO_CASE(const char *TEXT, const char *WORD)
{
	size_t	LENGTH = strlen(WORD);

	for (; *TEXT; TEXT++)
		if (!strncasecmp(TEXT, WORD, LENGTH))
			return ((char *)TEXT);

	if (LENGTH)
		return (NULL);

	return ((char *)TEXT);
}

static int	ELIMINATE(SOLVE_STORE *STORE, int *KNOWN, double *VALUES);
static void	SAY_NUMBER(
	double VALUE, int IS_MONEY, char *OUTPUT, int OUTPUT_SIZE
);

/* math said in words, as signs: "x + 1 equals to 43 if x is 42" ->
 * "x + 1 = 43 if x = 42" */
static void
	MATH_WORDS_TO_SIGNS(const char *TEXT, char *OUTPUT, int OUTPUT_SIZE)
{
	static const char	*SWAPS[9][2] = {
		{ " is equal to ", " = " }, { " equals to ", " = " },
		{ " equal to ", " = " }, { " equals ", " = " },
		{ " multiplied by ", " * " }, { " divided by ", " / " },
		{ " plus ", " + " }, { " minus ", " - " }, { NULL, NULL }
	};
	char				WORK[2000];
	int					INDEX;

	snprintf(WORK, sizeof WORK, " %s ", TEXT);

	for (INDEX = 0; SWAPS[INDEX][0]; INDEX++)
	{
		char	*AT;

		while ((AT = FIND_NO_CASE(WORK, SWAPS[INDEX][0])))
		{
			char	REST[2000];

			snprintf(REST, sizeof REST, "%s", AT + strlen(SWAPS[INDEX][0]));
			snprintf(
				AT, sizeof WORK - (size_t)(AT - WORK), "%s%s", SWAPS[INDEX][1],
				REST
			);
		}
	}

	/* "x is 42", "let x be 5": a single letter given a number */
	for (INDEX = 1; WORK[INDEX]; INDEX++)
	{
		char	LETTER = (char)tolower((uint8_t)WORK[INDEX]);
		int		SKIP;

		if (
			!isalpha((uint8_t)LETTER) ||
			LETTER == 'a' ||
			LETTER == 'i' ||
			isalnum((uint8_t)WORK[INDEX - 1]) ||
			WORK[INDEX + 1] != ' '
		)
			continue ;

		if (!strncasecmp(WORK + INDEX + 2, "is ", 3))
			SKIP = 3;
		else if (!strncasecmp(WORK + INDEX + 2, "be ", 3))
			SKIP = 3;
		else
			continue ;

		{
			const char	*VALUE = WORK + INDEX + 2 + SKIP;

			if (*VALUE == '-')
				VALUE++;

			if (!isdigit((uint8_t)*VALUE))
				continue ;

			WORK[INDEX + 2] = '=';
			WORK[INDEX + 3] = ' ';
			WORK[INDEX + 4] = ' ';
		}
	}

	snprintf(OUTPUT, OUTPUT_SIZE, "%s", WORK);
}

/* the text of tokens FROM..TO as it reads: "x + 23" */
static void
	TOKENS_TEXT(
		const SOLVE_TOKEN *TOKENS, int FROM, int TO, char *OUTPUT,
		int OUTPUT_SIZE
	)
{
	int	POSITION = 0;
	int	INDEX;

	OUTPUT[0] = 0;

	for (INDEX = FROM; INDEX < TO && POSITION < OUTPUT_SIZE - 1; INDEX++)
	{
		const SOLVE_TOKEN	*TOKEN = &TOKENS[INDEX];
		const char			*PIECE =
			TOKEN->KIND == TOKEN_UNKNOWN && TOKEN->SHOWN[0] ? TOKEN->SHOWN
															: TOKEN->TEXT;
		int					GLUED = INDEX > FROM &&
			((TOKEN->IS_JOINED && TOKEN->KIND != TOKEN_OPERATOR &&
				TOKENS[INDEX - 1].KIND != TOKEN_OPERATOR) ||
				(TOKEN->KIND == TOKEN_OPERATOR && TOKEN->TEXT[0] == ')') ||
				(TOKENS[INDEX - 1].KIND == TOKEN_OPERATOR &&
					TOKENS[INDEX - 1].TEXT[0] == '('));

		POSITION += snprintf(
			OUTPUT + POSITION, OUTPUT_SIZE - POSITION, "%s%s",
			INDEX > FROM && !GLUED ? " " : "", PIECE
		);
	}

	if (POSITION >= OUTPUT_SIZE)
		OUTPUT[OUTPUT_SIZE - 1] = 0;
}

/* a side of an equation when every unknown in it is known */
static int
	VALUE_OF(
		const LINEAR *SIDE, const SOLVE_STORE *FROM, const SOLVE_STORE *KNOWN,
		const int *IS_KNOWN, const double *VALUES, double *VALUE
	)
{
	int	INDEX;

	*VALUE = SIDE->CONSTANT;

	for (INDEX = 0; INDEX < FROM->NAME_COUNT; INDEX++)
	{
		int	AT;

		if (fabs(SIDE->COEFFICIENTS[INDEX]) < 1e-12)
			continue ;

		for (AT = 0; AT < KNOWN->NAME_COUNT; AT++)
			if (!strcmp(KNOWN->NAMES[AT], FROM->NAMES[INDEX]))
				break ;

		if (AT >= KNOWN->NAME_COUNT || !IS_KNOWN[AT])
			return (0);

		*VALUE += SIDE->COEFFICIENTS[INDEX] * VALUES[AT];
	}

	return (1);
}

/* "x = 42" in words for a check: the values the answer rests on */
static void
	SAY_KNOWN_VALUES(
		const SOLVE_STORE *FROM, const LINEAR *LEFT, const LINEAR *RIGHT,
		const SOLVE_STORE *KNOWN, const double *VALUES, char *OUTPUT,
		int OUTPUT_SIZE
	)
{
	int	POSITION = 0;
	int	INDEX;

	OUTPUT[0] = 0;

	for (INDEX = 0; INDEX < FROM->NAME_COUNT; INDEX++)
	{
		int		AT;
		char	NUMBER[64];

		if (
			fabs(LEFT->COEFFICIENTS[INDEX]) < 1e-12 &&
			fabs(RIGHT->COEFFICIENTS[INDEX]) < 1e-12
		)
			continue ;

		for (AT = 0; AT < KNOWN->NAME_COUNT; AT++)
			if (!strcmp(KNOWN->NAMES[AT], FROM->NAMES[INDEX]))
				break ;

		if (AT >= KNOWN->NAME_COUNT)
			continue ;

		SAY_NUMBER(VALUES[AT], KNOWN->IS_MONEY, NUMBER, sizeof NUMBER);
		POSITION += snprintf(
			OUTPUT + POSITION, OUTPUT_SIZE - POSITION, "%s%s = %s",
			POSITION ? " and " : "", KNOWN->SHOWN[AT], NUMBER
		);

		if (POSITION >= OUTPUT_SIZE)
			break ;
	}
}

static const char	*NOT_NAMES[39] = {
	"it", "they", "them", "this", "that", "these", "those", "i", "you", "we",
	"he", "she", "me", "one", "together", "total", "all", "each", "both",
	"what", "which", "who", "there", "here", "and", "or", "a", "an", "the",
	"is", "are", "of", "how", "much", "many", "if", "so", "then", NULL
};

static int
	IS_IN_LIST(const char *WORD, const char **LIST)
{
	int	INDEX;

	for (INDEX = 0; LIST[INDEX]; INDEX++)
		if (!strcmp(WORD, LIST[INDEX]))
			return (1);

	return (0);
}

void
	SOLVE_CLEAR(SOLVE_STORE *STORE)
{
	memset(STORE, 0, sizeof *STORE);
}

void
	SOLVE_TICK(SOLVE_STORE *STORE)
{
	STORE->AGE++;

	if (STORE->AGE > 8)
		SOLVE_CLEAR(STORE);
}

static int
	FIND_NAME(SOLVE_STORE *STORE, const char *NAME)
{
	int	INDEX;

	for (INDEX = 0; INDEX < STORE->NAME_COUNT; INDEX++)
		if (!strcmp(STORE->NAMES[INDEX], NAME))
			return (INDEX);

	return (-1);
}

static int
	ADD_NAME(SOLVE_STORE *STORE, const char *NAME, const char *SHOWN)
{
	int	INDEX = FIND_NAME(STORE, NAME);

	if (INDEX >= 0)
		return (INDEX);

	if (STORE->NAME_COUNT >= SOLVE_MAX_NAMES)
		return (-1);

	INDEX = STORE->NAME_COUNT++;
	snprintf(STORE->NAMES[INDEX], sizeof STORE->NAMES[INDEX], "%s", NAME);
	snprintf(STORE->SHOWN[INDEX], sizeof STORE->SHOWN[INDEX], "%s", SHOWN);

	return (INDEX);
}

/* "$1.10" -> 1.1 (money), "1,000" -> 1000, "[Ball]" -> the unknown "ball",
 * words, the math signs and the stops between sentences */
static int
	TOKENIZE(const char *TEXT, SOLVE_TOKEN *TOKENS, int MAX_TOKENS)
{
	const unsigned char	*CURSOR = (const unsigned char *)TEXT;
	int					COUNT = 0;
	int					JOINED = 0;

	while (*CURSOR && COUNT < MAX_TOKENS)
	{
		SOLVE_TOKEN	*TOKEN = &TOKENS[COUNT];
		int			IS_MONEY = 0;

		memset(TOKEN, 0, sizeof *TOKEN);

		if (*CURSOR == ' ' || *CURSOR == '\t')
		{
			CURSOR++;
			JOINED = 0;
			continue ;
		}

		TOKEN->IS_JOINED = JOINED;
		JOINED = 1;

		if (*CURSOR == '$' && isdigit(CURSOR[1]))
		{
			IS_MONEY = 1;
			CURSOR++;
		}

		if (isdigit(*CURSOR) || (*CURSOR == '.' && isdigit(CURSOR[1])))
		{
			char	NUMBER[48];
			int		LENGTH = 0;

			while (
				LENGTH < 40 &&
				(
					isdigit(*CURSOR) ||
					(*CURSOR == '.' && isdigit(CURSOR[1])) ||
					(
						*CURSOR == ',' &&
						isdigit(CURSOR[1]) &&
						isdigit(CURSOR[2]) &&
						isdigit(CURSOR[3]) &&
						!isdigit(CURSOR[4])
					)
				)
			)
			{
				if (*CURSOR != ',')
					NUMBER[LENGTH++] = (char)*CURSOR;

				CURSOR++;
			}

			NUMBER[LENGTH] = 0;
			TOKEN->KIND = TOKEN_NUMBER;
			TOKEN->VALUE = atof(NUMBER);
			TOKEN->IS_MONEY = IS_MONEY;
			snprintf(TOKEN->TEXT, sizeof TOKEN->TEXT, "%s", NUMBER);
			COUNT++;
			continue ;
		}

		if (*CURSOR == '[')
		{
			const unsigned char	*END =
				(const unsigned char *)strchr((const char *)CURSOR, ']');
			int					LENGTH;
			int					INDEX;
			int					HAS_LETTER = 0;

			if (END && END - CURSOR > 1 && END - CURSOR < 40)
			{
				LENGTH = (int)(END - CURSOR - 1);

				for (INDEX = 0; INDEX < LENGTH; INDEX++)
				{
					TOKEN->TEXT[INDEX] = (char)tolower(CURSOR[1 + INDEX]);

					if (isalpha(CURSOR[1 + INDEX]))
						HAS_LETTER = 1;
				}

				TOKEN->TEXT[LENGTH] = 0;

				if (HAS_LETTER)
				{
					snprintf(
						TOKEN->SHOWN, sizeof TOKEN->SHOWN, "%.*s", LENGTH,
						(const char *)CURSOR + 1
					);
					TOKEN->KIND = TOKEN_UNKNOWN;
					CURSOR = END + 1;
					COUNT++;
					continue ;
				}
			}
		}

		if (isalpha(*CURSOR))
		{
			int	LENGTH = 0;

			while (
				(isalpha(*CURSOR) || *CURSOR == '\'') &&
				LENGTH < (int)sizeof TOKEN->TEXT - 1
			)
			{
				TOKEN->SHOWN[LENGTH] = (char)*CURSOR;
				TOKEN->TEXT[LENGTH++] = (char)tolower(*CURSOR);
				CURSOR++;
			}

			TOKEN->TEXT[LENGTH] = TOKEN->SHOWN[LENGTH] = 0;
			TOKEN->KIND = TOKEN_WORD;
			COUNT++;
			continue ;
		}

		/* x and the division sign written the long way (UTF-8) */
		if (CURSOR[0] == 0XC3 && (CURSOR[1] == 0X97 || CURSOR[1] == 0XB7))
		{
			TOKEN->KIND = TOKEN_OPERATOR;

			if (CURSOR[1] == 0X97)
				TOKEN->TEXT[0] = '*';
			else
				TOKEN->TEXT[0] = '/';

			CURSOR += 2;
			COUNT++;
			continue ;
		}

		if (CURSOR[0] == 0XE2 && CURSOR[1] == 0X88 && CURSOR[2] == 0X92)
		{
			TOKEN->KIND = TOKEN_OPERATOR;
			TOKEN->TEXT[0] = '-';
			CURSOR += 3;
			COUNT++;
			continue ;
		}

		if (strchr("+-*/()=", *CURSOR))
		{
			TOKEN->KIND = TOKEN_OPERATOR;
			TOKEN->TEXT[0] = (char)*CURSOR;
			CURSOR++;
			COUNT++;
			continue ;
		}

		if (strchr(".,;:?!\n", *CURSOR))
		{
			TOKEN->KIND = TOKEN_STOP;
			TOKEN->TEXT[0] = (char)*CURSOR;
			CURSOR++;
			COUNT++;
			continue ;
		}

		CURSOR++;
	}

	return (COUNT);
}

static int
	IS_SIGN(const SOLVE_TOKEN *TOKEN, const char *SIGNS)
{
	return (
		TOKEN->KIND == TOKEN_OPERATOR &&
		strchr(SIGNS, TOKEN->TEXT[0]) != NULL
	);
}

/* a word next to a sign is an unknown when it is a single letter ("x + 3 =
 * 7", "2x"), or a name already used as an unknown */
static void
	MARK_WORD_UNKNOWNS(SOLVE_TOKEN *TOKENS, int COUNT, SOLVE_STORE *KNOWN)
{
	int	INDEX;

	for (INDEX = 0; INDEX < COUNT; INDEX++)
	{
		SOLVE_TOKEN	*TOKEN = &TOKENS[INDEX];
		int			NEXT_TO_SIGN = 0;
		int			IS_LETTER;
		int			OTHER;

		if (TOKEN->KIND != TOKEN_WORD)
			continue ;

		if (INDEX > 0 && IS_SIGN(&TOKENS[INDEX - 1], "+-*/=("))
			NEXT_TO_SIGN = 1;

		if (INDEX + 1 < COUNT && IS_SIGN(&TOKENS[INDEX + 1], "+-*/=)"))
			NEXT_TO_SIGN = 1;

		IS_LETTER = strlen(TOKEN->TEXT) == 1 && strcmp(TOKEN->TEXT, "a") &&
			strcmp(TOKEN->TEXT, "i");

		/* "2x" */
		if (
			IS_LETTER &&
			INDEX > 0 &&
			TOKEN->IS_JOINED &&
			TOKENS[INDEX - 1].KIND == TOKEN_NUMBER
		)
			NEXT_TO_SIGN = 1;

		if (!NEXT_TO_SIGN)
			continue ;

		if (IS_LETTER || FIND_NAME(KNOWN, TOKEN->TEXT) >= 0)
		{
			TOKEN->KIND = TOKEN_UNKNOWN;
			continue ;
		}

		for (OTHER = 0; OTHER < COUNT; OTHER++)
			if (
				TOKENS[OTHER].KIND == TOKEN_UNKNOWN &&
				!strcmp(TOKENS[OTHER].TEXT, TOKEN->TEXT)
			)
				TOKEN->KIND = TOKEN_UNKNOWN;
	}
}

static int	PARSE_SUM(SOLVE_PARSER *PARSER, LINEAR *RESULT);

static int
	IS_CONSTANT(const LINEAR *VALUE)
{
	int	INDEX;

	for (INDEX = 0; INDEX < SOLVE_MAX_NAMES; INDEX++)
		if (fabs(VALUE->COEFFICIENTS[INDEX]) > 1e-12)
			return (0);

	return (1);
}

static void
	SCALE_LINEAR(LINEAR *VALUE, double FACTOR)
{
	int	INDEX;

	for (INDEX = 0; INDEX < SOLVE_MAX_NAMES; INDEX++)
		VALUE->COEFFICIENTS[INDEX] *= FACTOR;

	VALUE->CONSTANT *= FACTOR;
}

static int
	PARSE_FACTOR(SOLVE_PARSER *PARSER, LINEAR *RESULT)
{
	SOLVE_TOKEN	*TOKEN;

	memset(RESULT, 0, sizeof *RESULT);

	if (PARSER->POSITION >= PARSER->COUNT)
		return (0);

	TOKEN = &PARSER->TOKENS[PARSER->POSITION];

	if (IS_SIGN(TOKEN, "-"))
	{
		PARSER->POSITION++;

		if (!PARSE_FACTOR(PARSER, RESULT))
			return (0);

		SCALE_LINEAR(RESULT, -1);
		return (1);
	}

	if (IS_SIGN(TOKEN, "+"))
	{
		PARSER->POSITION++;
		return (PARSE_FACTOR(PARSER, RESULT));
	}

	if (TOKEN->KIND == TOKEN_NUMBER)
	{
		RESULT->CONSTANT = TOKEN->VALUE;
		PARSER->POSITION++;
		return (1);
	}

	if (TOKEN->KIND == TOKEN_UNKNOWN)
	{
		int	INDEX = ADD_NAME(
			PARSER->STORE, TOKEN->TEXT,
			TOKEN->SHOWN[0] ? TOKEN->SHOWN : TOKEN->TEXT
		);

		if (INDEX < 0)
			return (0);

		RESULT->COEFFICIENTS[INDEX] = 1;
		PARSER->POSITION++;
		return (1);
	}

	if (IS_SIGN(TOKEN, "("))
	{
		PARSER->POSITION++;

		if (!PARSE_SUM(PARSER, RESULT))
			return (0);

		if (
			PARSER->POSITION >= PARSER->COUNT ||
			!IS_SIGN(&PARSER->TOKENS[PARSER->POSITION], ")")
		)
			return (0);

		PARSER->POSITION++;
		return (1);
	}

	return (0);
}

/* a product stays linear while one side of it is a plain number */
static int
	PARSE_PRODUCT(SOLVE_PARSER *PARSER, LINEAR *RESULT)
{
	if (!PARSE_FACTOR(PARSER, RESULT))
		return (0);

	while (PARSER->POSITION < PARSER->COUNT)
	{
		SOLVE_TOKEN	*TOKEN = &PARSER->TOKENS[PARSER->POSITION];
		LINEAR		RIGHT;
		int			IS_DIVISION = 0;

		if (IS_SIGN(TOKEN, "*/"))
		{
			IS_DIVISION = TOKEN->TEXT[0] == '/';
			PARSER->POSITION++;
		}
		/* "2x", "2(x + 1)", "[Ball][Ball]" is not allowed */
		else if (
			!(TOKEN->KIND == TOKEN_UNKNOWN || IS_SIGN(TOKEN, "(")) ||
			!IS_CONSTANT(RESULT)
		)
			break ;

		if (!PARSE_FACTOR(PARSER, &RIGHT))
			return (0);

		if (IS_DIVISION)
		{
			if (!IS_CONSTANT(&RIGHT) || fabs(RIGHT.CONSTANT) < 1e-12)
				return (0);

			SCALE_LINEAR(RESULT, 1 / RIGHT.CONSTANT);
		}
		else if (IS_CONSTANT(RESULT))
		{
			double	FACTOR = RESULT->CONSTANT;

			*RESULT = RIGHT;
			SCALE_LINEAR(RESULT, FACTOR);
		}
		else if (IS_CONSTANT(&RIGHT))
			SCALE_LINEAR(RESULT, RIGHT.CONSTANT);
		else
			return (0);
	}

	return (1);
}

static int
	PARSE_SUM(SOLVE_PARSER *PARSER, LINEAR *RESULT)
{
	if (!PARSE_PRODUCT(PARSER, RESULT))
		return (0);

	while (
		PARSER->POSITION < PARSER->COUNT &&
		IS_SIGN(&PARSER->TOKENS[PARSER->POSITION], "+-")
	)
	{
		double	SIGN;

		if (PARSER->TOKENS[PARSER->POSITION].TEXT[0] == '-')
			SIGN = -1;
		else
			SIGN = 1;

		LINEAR	RIGHT;
		int		INDEX;

		PARSER->POSITION++;

		if (!PARSE_PRODUCT(PARSER, &RIGHT))
			return (0);

		for (INDEX = 0; INDEX < SOLVE_MAX_NAMES; INDEX++)
			RESULT->COEFFICIENTS[INDEX] += SIGN * RIGHT.COEFFICIENTS[INDEX];

		RESULT->CONSTANT += SIGN * RIGHT.CONSTANT;
	}

	return (1);
}

/* left - right = 0 as a row: the coefficients, then the constant on the
 * other side */
static int
	ADD_ROW(SOLVE_STORE *STORE, const LINEAR *LEFT, const LINEAR *RIGHT)
{
	double	ROW[SOLVE_MAX_NAMES + 1];
	int		HAS_UNKNOWN = 0;
	int		INDEX;
	int		OTHER;

	for (INDEX = 0; INDEX < SOLVE_MAX_NAMES; INDEX++)
	{
		ROW[INDEX] = LEFT->COEFFICIENTS[INDEX] - RIGHT->COEFFICIENTS[INDEX];

		if (fabs(ROW[INDEX]) > 1e-12)
			HAS_UNKNOWN = 1;
	}

	ROW[SOLVE_MAX_NAMES] = RIGHT->CONSTANT - LEFT->CONSTANT;

	if (!HAS_UNKNOWN || STORE->ROW_COUNT >= SOLVE_MAX_ROWS)
		return (0);

	/* the same equation said twice is one equation */
	for (OTHER = 0; OTHER < STORE->ROW_COUNT; OTHER++)
	{
		double	RATIO = 0;
		int		SAME = 1;

		for (INDEX = 0; INDEX <= SOLVE_MAX_NAMES; INDEX++)
		{
			double	MINE = ROW[INDEX];
			double	THEIRS = STORE->ROWS[OTHER][INDEX];

			if (fabs(MINE) < 1e-12 && fabs(THEIRS) < 1e-12)
				continue ;

			if (fabs(MINE) < 1e-12 || fabs(THEIRS) < 1e-12)
			{
				SAME = 0;
				break ;
			}

			if (RATIO == 0)
				RATIO = THEIRS / MINE;
			else if (fabs(THEIRS / MINE - RATIO) > 1e-9 * fabs(RATIO))
			{
				SAME = 0;
				break ;
			}
		}

		if (SAME)
			return (0);
	}

	memcpy(STORE->ROWS[STORE->ROW_COUNT++], ROW, sizeof ROW);

	return (1);
}

/* the math stretches between words: "[Bat] + [Ball] = 1.10 and [Ball] + 1 =
 * [Bat]" holds two */
static int
	READ_MATH(
		SOLVE_STORE *STORE, SOLVE_TOKEN *TOKENS, int COUNT, int *IS_MONEY,
		SOLVE_STORE *KNOWN
	)
{
	int		ADDED = 0;
	int		START = 0;
	int		IS_KNOWN[SOLVE_MAX_NAMES];
	double	VALUES[SOLVE_MAX_NAMES];
	int		CAN_CHECK = 0;

	/* an equation about unknowns that are already known is a claim to
	 * check, not one more equation */
	if (KNOWN && KNOWN->ROW_COUNT && KNOWN->ROW_COUNT <= SOLVE_MAX_ROWS)
	{
		SOLVE_STORE	COPY = *KNOWN;

		CAN_CHECK = ELIMINATE(&COPY, IS_KNOWN, VALUES);
	}

	while (START < COUNT)
	{
		int	END = START;
		int	EQUALS = -1;
		int	EQUALS_COUNT = 0;
		int	HAS_UNKNOWN = 0;
		int	INDEX;

		while (
			END < COUNT &&
			TOKENS[END].KIND != TOKEN_WORD &&
			TOKENS[END].KIND != TOKEN_STOP
		)
			END++;

		for (INDEX = START; INDEX < END; INDEX++)
		{
			if (IS_SIGN(&TOKENS[INDEX], "="))
			{
				EQUALS = INDEX;
				EQUALS_COUNT++;
			}

			if (TOKENS[INDEX].KIND == TOKEN_UNKNOWN)
				HAS_UNKNOWN = 1;
		}

		if (EQUALS_COUNT == 1 && HAS_UNKNOWN)
		{
			SOLVE_STORE		TRIAL = *STORE;
			SOLVE_PARSER	PARSER = { TOKENS, EQUALS, START, &TRIAL, 0 };
			LINEAR			LEFT;
			LINEAR			RIGHT;

			if (PARSE_SUM(&PARSER, &LEFT) && PARSER.POSITION == EQUALS)
			{
				PARSER.COUNT = END;
				PARSER.POSITION = EQUALS + 1;

				double	LEFT_VALUE;
				double	RIGHT_VALUE;
				int		HAS_RIGHT =
					PARSE_SUM(&PARSER, &RIGHT) && PARSER.POSITION == END;

				if (!HAS_RIGHT)
					;
				else if (
					CAN_CHECK &&
					VALUE_OF(
						&LEFT, &TRIAL, KNOWN, IS_KNOWN, VALUES, &LEFT_VALUE
					) &&
					VALUE_OF(
						&RIGHT, &TRIAL, KNOWN, IS_KNOWN, VALUES, &RIGHT_VALUE
					)
				)
				{
					char	SIDE[200];
					char	WHEN[200];
					char	SHOWN_VALUE[64];
					char	CLAIMED[64];
					char	VERDICT[500];
					size_t	USED = strlen(KNOWN->CHECKED);

					TOKENS_TEXT(TOKENS, START, EQUALS, SIDE, sizeof SIDE);
					SAY_KNOWN_VALUES(
						&TRIAL, &LEFT, &RIGHT, KNOWN, VALUES, WHEN, sizeof WHEN
					);
					SAY_NUMBER(LEFT_VALUE, KNOWN->IS_MONEY, SHOWN_VALUE, 64);
					SAY_NUMBER(RIGHT_VALUE, KNOWN->IS_MONEY, CLAIMED, 64);

					/* "x = 42" said again */
					if (
						fabs(LEFT_VALUE - RIGHT_VALUE) < 1e-9 &&
						EQUALS - START == 1 &&
						TOKENS[START].KIND == TOKEN_UNKNOWN
					)
						snprintf(
							VERDICT, sizeof VERDICT, "Yes, %s = %s.", SIDE,
							SHOWN_VALUE
						);
					else if (fabs(LEFT_VALUE - RIGHT_VALUE) < 1e-9)
						snprintf(
							VERDICT, sizeof VERDICT,
							"Yes, that's right: when %s, %s = %s.", WHEN, SIDE,
							SHOWN_VALUE
						);
					else
						snprintf(
							VERDICT, sizeof VERDICT,
							"That's not right: when %s, %s is %s, not %s.",
							WHEN, SIDE, SHOWN_VALUE, CLAIMED
						);

					if (!strstr(KNOWN->CHECKED, VERDICT))
						snprintf(
							KNOWN->CHECKED + USED, sizeof KNOWN->CHECKED - USED,
							"%s%s", USED ? " " : "", VERDICT
						);

					ADDED++;
				}
				else if (ADD_ROW(&TRIAL, &LEFT, &RIGHT))
				{
					*STORE = TRIAL;
					ADDED++;

					for (INDEX = START; INDEX < END; INDEX++)
						if (TOKENS[INDEX].IS_MONEY)
							*IS_MONEY = 1;
				}
			}
		}

		START = END + 1;
	}

	return (ADDED);
}

/* "the bat" -> "bat", "balls" -> "ball", "the price of the bat" -> "bat" */
static int
	HEAD_NOUN(
		char (*WORDS)[48], char (*SHOWN)[48], int FROM, int TO, char *OUTPUT,
		char *OUTPUT_SHOWN
	)
{
	static const char	*TRAILING[14] = {
		"does", "do", "did", "is", "are", "was", "were", "costs", "cost",
		"weighs", "weigh", "together", "now", NULL
	};
	char				WORD[48];
	size_t				LENGTH;

	while (TO > FROM && IS_IN_LIST(WORDS[TO - 1], TRAILING))
		TO--;

	if (TO <= FROM)
		return (0);

	snprintf(WORD, sizeof WORD, "%s", WORDS[TO - 1]);
	LENGTH = strlen(WORD);

	if (LENGTH > 2 && !strcmp(WORD + LENGTH - 2, "'s"))
		WORD[LENGTH -= 2] = 0;

	if (LENGTH > 3 && WORD[LENGTH - 1] == 's' && WORD[LENGTH - 2] != 's')
		WORD[--LENGTH] = 0;

	if (!LENGTH || IS_IN_LIST(WORD, NOT_NAMES) || !isalpha((uint8_t)WORD[0]))
		return (0);

	snprintf(OUTPUT, 48, "%s", WORD);
	snprintf(OUTPUT_SHOWN, 48, "%.*s", (int)LENGTH, SHOWN[TO - 1]);

	return (1);
}

static int
	NUMBER_WORD(const char *WORD, double *VALUE)
{
	static const char	*WORDS[14] = {
		"zero", "one", "two", "three", "four", "five", "six", "seven", "eight",
		"nine", "ten", "eleven", "twelve", NULL
	};
	int					INDEX;

	for (INDEX = 0; WORDS[INDEX]; INDEX++)
		if (!strcmp(WORD, WORDS[INDEX]))
		{
			*VALUE = INDEX;
			return (1);
		}

	return (0);
}

static int
	ADD_WORD_ROW(
		SOLVE_STORE *STORE, const char *FIRST, const char *FIRST_SHOWN,
		double FIRST_FACTOR, const char *SECOND, const char *SECOND_SHOWN,
		double SECOND_FACTOR, double CONSTANT
	)
{
	LINEAR	LEFT;
	LINEAR	RIGHT;
	int		FIRST_INDEX = ADD_NAME(STORE, FIRST, FIRST_SHOWN);
	int		SECOND_INDEX;

	if (SECOND)
		SECOND_INDEX = ADD_NAME(STORE, SECOND, SECOND_SHOWN);
	else
		SECOND_INDEX = -1;

	if (FIRST_INDEX < 0 || (SECOND && SECOND_INDEX < 0))
		return (0);

	memset(&LEFT, 0, sizeof LEFT);
	memset(&RIGHT, 0, sizeof RIGHT);
	LEFT.COEFFICIENTS[FIRST_INDEX] += FIRST_FACTOR;

	if (SECOND)
		LEFT.COEFFICIENTS[SECOND_INDEX] += SECOND_FACTOR;

	RIGHT.CONSTANT = CONSTANT;

	return (ADD_ROW(STORE, &LEFT, &RIGHT));
}

/* how the answer is said: "costs", "weighs", "has" ... 5 "apples" */
static void
	NOTE_VERB(
		SOLVE_STORE *STORE, const char *VERB, const char *UNIT, int *IS_MONEY
	)
{
	static const char	*FORMS[14][2] = {
		{ "cost", "costs" }, { "costs", "costs" }, { "weigh", "weighs" },
		{ "weighs", "weighs" }, { "has", "has" }, { "have", "has" },
		{ "had", "has" }, { "earn", "earns" }, { "earns", "earns" },
		{ "make", "makes" }, { "makes", "makes" }, { "get", "gets" },
		{ "gets", "gets" }, { NULL, NULL }
	};
	static const char	*MONEY[6] = {
		"dollar", "dollars", "buck", "bucks", "usd", NULL
	};
	static const char	*NOT_UNITS[13] = {
		"together", "altogether", "in", "total", "combined", "all", "more",
		"less", "fewer", "times", "than", "as", NULL
	};
	int					INDEX;

	if (UNIT && IS_IN_LIST(UNIT, MONEY))
	{
		*IS_MONEY = 1;
		UNIT = NULL;
	}

	if (STORE->VERB[0])
		return ;

	snprintf(STORE->VERB, sizeof STORE->VERB, "is");

	for (INDEX = 0; FORMS[INDEX][0]; INDEX++)
		if (!strcmp(VERB, FORMS[INDEX][0]))
			snprintf(STORE->VERB, sizeof STORE->VERB, "%s", FORMS[INDEX][1]);

	if (UNIT && isalpha((uint8_t)UNIT[0]) && !IS_IN_LIST(UNIT, NOT_UNITS))
		snprintf(STORE->UNIT, sizeof STORE->UNIT, "%s", UNIT);
}

/* one sentence of a word problem:
 *   "a bat and a ball cost $1.10 together"     bat + ball = 1.10
 *   "the bat costs $1 more than the ball"      bat - ball = 1
 *   "the bat costs $1 less than the ball"      bat - ball = -1
 *   "the bat costs 3 times as much as the ball" bat - 3 ball = 0
 *   "the bat costs twice as much as the ball"  bat - 2 ball = 0
 *   "the ball costs $2"                        ball = 2 */
static int
	READ_WORD_SENTENCE(
		SOLVE_STORE *STORE, SOLVE_TOKEN *TOKENS, int COUNT, int *IS_MONEY
	)
{
	static const char	*VERBS[18] = {
		"cost", "costs", "is", "are", "weighs", "weigh", "was", "were", "has",
		"have", "had", "earns", "earn", "makes", "make", "gets", "get", NULL
	};
	static const char	*FILLERS[7] = {
		"together", "altogether", "in", "total", "combined", "all", NULL
	};
	static const char	*LEADS[8] = {
		"and", "but", "so", "if", "now", "also", "then", NULL
	};
	char				WORDS[64][48];
	char				ORIGINAL[64][48];
	double				VALUES[64];
	int					IS_NUMBER[64];
	int					WORD_COUNT = 0;
	int					VERB = -1;
	int					START = 0;
	int					AND = -1;
	int					INDEX;
	int					MONEY = 0;
	char				FIRST[48];
	char				SECOND[48];
	char				FIRST_SHOWN[48];
	char				SECOND_SHOWN[48];

	for (INDEX = 0; INDEX < COUNT && WORD_COUNT < 64; INDEX++)
	{
		if (TOKENS[INDEX].KIND == TOKEN_OPERATOR)
			return (0);

		IS_NUMBER[WORD_COUNT] = TOKENS[INDEX].KIND == TOKEN_NUMBER;
		VALUES[WORD_COUNT] = TOKENS[INDEX].VALUE;

		if (!IS_NUMBER[WORD_COUNT])
			IS_NUMBER[WORD_COUNT] =
				NUMBER_WORD(TOKENS[INDEX].TEXT, &VALUES[WORD_COUNT]);

		if (TOKENS[INDEX].IS_MONEY)
			MONEY = 1;

		snprintf(WORDS[WORD_COUNT], 48, "%s", TOKENS[INDEX].TEXT);
		snprintf(
			ORIGINAL[WORD_COUNT], 48, "%s",
			TOKENS[INDEX].SHOWN[0] ? TOKENS[INDEX].SHOWN : TOKENS[INDEX].TEXT
		);
		WORD_COUNT++;
	}

	while (START < WORD_COUNT && IS_IN_LIST(WORDS[START], LEADS))
		START++;

	for (INDEX = START; INDEX < WORD_COUNT; INDEX++)
		if (IS_IN_LIST(WORDS[INDEX], VERBS))
		{
			VERB = INDEX;
			break ;
		}

	if (VERB <= START)
		return (0);

	for (INDEX = START; INDEX < VERB; INDEX++)
		if (!strcmp(WORDS[INDEX], "and"))
			AND = INDEX;

	if (AND >= 0)
	{
		if (
			!HEAD_NOUN(WORDS, ORIGINAL, START, AND, FIRST, FIRST_SHOWN) ||
			!HEAD_NOUN(WORDS, ORIGINAL, AND + 1, VERB, SECOND, SECOND_SHOWN)
		)
			return (0);
	}
	else if (!HEAD_NOUN(WORDS, ORIGINAL, START, VERB, FIRST, FIRST_SHOWN))
		return (0);

	INDEX = VERB + 1;

	while (INDEX < WORD_COUNT && IS_IN_LIST(WORDS[INDEX], FILLERS))
		INDEX++;

	if (INDEX >= WORD_COUNT)
		return (0);

	/* "twice as much as", "half as much as", "double", "triple" */
	{
		double	FACTOR = 0;

		if (!strcmp(WORDS[INDEX], "twice") || !strcmp(WORDS[INDEX], "double"))
			FACTOR = 2;
		else if (
			!strcmp(WORDS[INDEX], "thrice") ||
			!strcmp(WORDS[INDEX], "triple")
		)
			FACTOR = 3;
		else if (!strcmp(WORDS[INDEX], "half"))
			FACTOR = 0.5;
		else if (
			IS_NUMBER[INDEX] &&
			INDEX + 1 < WORD_COUNT &&
			!strcmp(WORDS[INDEX + 1], "times")
		)
		{
			FACTOR = VALUES[INDEX];
			INDEX++;
		}

		if (FACTOR > 0)
		{
			int	SKIP = INDEX + 1;

			while (
				SKIP < WORD_COUNT &&
				(
					!strcmp(WORDS[SKIP], "as") ||
					!strcmp(WORDS[SKIP], "much") ||
					!strcmp(WORDS[SKIP], "many") ||
					!strcmp(WORDS[SKIP], "the") ||
					!strcmp(WORDS[SKIP], "of")
				)
			)
				SKIP++;

			if (
				AND >= 0 ||
				!HEAD_NOUN(
					WORDS, ORIGINAL, SKIP, WORD_COUNT, SECOND, SECOND_SHOWN
				)
			)
				return (0);

			if (!strcmp(FIRST, SECOND))
				return (0);

			NOTE_VERB(STORE, WORDS[VERB], NULL, IS_MONEY);
			return (
				ADD_WORD_ROW(
					STORE, FIRST, FIRST_SHOWN, 1, SECOND, SECOND_SHOWN, -FACTOR,
					0
				)
			);
		}
	}

	if (!IS_NUMBER[INDEX])
		return (0);

	{
		double	AMOUNT = VALUES[INDEX];
		int		NEXT = INDEX + 1;
		int		SKIPPED = 0;

		if (MONEY)
			*IS_MONEY = 1;

		/* "$1 more than", "5 apples more than", "2 kg less than" */
		while (
			NEXT < WORD_COUNT &&
			SKIPPED < 2 &&
			strcmp(WORDS[NEXT], "more") &&
			strcmp(WORDS[NEXT], "less") &&
			strcmp(WORDS[NEXT], "fewer") &&
			!IS_IN_LIST(WORDS[NEXT], FILLERS)
		)
		{
			NEXT++;
			SKIPPED++;
		}

		if (
			NEXT + 1 < WORD_COUNT &&
			(
				!strcmp(WORDS[NEXT], "more") ||
				!strcmp(WORDS[NEXT], "less") ||
				!strcmp(WORDS[NEXT], "fewer")
			) &&
			!strcmp(WORDS[NEXT + 1], "than")
		)
		{
			double	SIGN;

			if (strcmp(WORDS[NEXT], "more"))
				SIGN = -1;
			else
				SIGN = 1;

			if (
				AND >= 0 ||
				!HEAD_NOUN(
					WORDS, ORIGINAL, NEXT + 2, WORD_COUNT, SECOND, SECOND_SHOWN
				)
			)
				return (0);

			if (!strcmp(FIRST, SECOND))
				return (0);

			NOTE_VERB(
				STORE, WORDS[VERB], NEXT > INDEX + 1 ? WORDS[INDEX + 1] : NULL,
				IS_MONEY
			);
			return (
				ADD_WORD_ROW(
					STORE, FIRST, FIRST_SHOWN, 1, SECOND, SECOND_SHOWN, -1,
					SIGN * AMOUNT
				)
			);
		}

		/* only units and "together" may follow a plain amount */
		for (NEXT = INDEX + 1; NEXT < WORD_COUNT; NEXT++)
			if (!IS_IN_LIST(WORDS[NEXT], FILLERS) && NEXT > INDEX + 2)
				return (0);

		NOTE_VERB(
			STORE, WORDS[VERB],
			INDEX + 1 < WORD_COUNT ? WORDS[INDEX + 1] : NULL, IS_MONEY
		);

		if (AND >= 0)
			return (
				ADD_WORD_ROW(
					STORE, FIRST, FIRST_SHOWN, 1, SECOND, SECOND_SHOWN, 1,
					AMOUNT
				)
			);

		return (
			ADD_WORD_ROW(STORE, FIRST, FIRST_SHOWN, 1, NULL, NULL, 0, AMOUNT)
		);
	}
}

static int
	HAS_QUESTION_CUE(const char *LOWER)
{
	static const char	*CUES[17] = {
		"how much", "how many", "what is", "what's", "whats", "what does",
		"what do", "find", "calculate", "work out", "figure out", "value of",
		"solve", "price of", "cost of", "tell me", NULL
	};
	int					INDEX;

	for (INDEX = 0; CUES[INDEX]; INDEX++)
		if (strstr(LOWER, CUES[INDEX]))
			return (1);

	return (0);
}

static void
	LOWER_COPY(const char *TEXT, char *OUTPUT, int OUTPUT_SIZE)
{
	int	INDEX;

	for (INDEX = 0; TEXT[INDEX] && INDEX < OUTPUT_SIZE - 1; INDEX++)
		OUTPUT[INDEX] = (char)tolower((uint8_t)TEXT[INDEX]);

	OUTPUT[INDEX] = 0;
}

/* what this message says about unknowns joins the kept equations; a new
 * problem (none of the same unknowns) starts over */
int
	SOLVE_READ(SOLVE_STORE *STORE, const char *TEXT)
{
	static SOLVE_TOKEN	TOKENS[SOLVE_MAX_TOKENS];
	SOLVE_STORE			FRESH;
	char				LOWER[2000];
	int					COUNT;
	int					ADDED;
	int					IS_MONEY = 0;
	int					IS_WORDS = 0;
	int					SHARED = 0;
	int					INDEX;
	char				SIGNS[2000];

	if (!TEXT || !TEXT[0])
		return (0);

	MATH_WORDS_TO_SIGNS(TEXT, SIGNS, sizeof SIGNS);
	TEXT = SIGNS;
	COUNT = TOKENIZE(TEXT, TOKENS, SOLVE_MAX_TOKENS);

	/* "reward(x) = x * 8 + 18" is a formula to keep, not an equation */
	for (INDEX = 0; INDEX + 1 < COUNT; INDEX++)
		if (
			TOKENS[INDEX].KIND == TOKEN_WORD &&
			IS_SIGN(&TOKENS[INDEX + 1], "(") &&
			TOKENS[INDEX + 1].IS_JOINED
		)
			return (0);

	LOWER_COPY(TEXT, LOWER, sizeof LOWER);

	if (strstr(LOWER, "formula"))
		return (0);

	SOLVE_CLEAR(&FRESH);
	MARK_WORD_UNKNOWNS(TOKENS, COUNT, STORE);
	STORE->CHECKED[0] = 0;

	/* "x + 1 = 43 if x is 42", "wrong, x + 1 is 43": a claim about what is
	 * known; "2x + 3 = 11" alone starts a new problem */
	{
		static const char	*CLAIM_WORDS[9] = {
			" if ", " when ", "wrong", "right", "correct", "true", "?",
			" check", NULL
		};
		char				PADDED[2100];
		int					IS_CLAIM = 0;

		snprintf(PADDED, sizeof PADDED, " %s ", TEXT);

		for (INDEX = 0; CLAIM_WORDS[INDEX]; INDEX++)
			if (FIND_NO_CASE(PADDED, CLAIM_WORDS[INDEX]))
				IS_CLAIM = 1;

		ADDED = READ_MATH(
			&FRESH, TOKENS, COUNT, &IS_MONEY, IS_CLAIM ? STORE : NULL
		);
	}

	LOWER_COPY(TEXT, LOWER, sizeof LOWER);

	/* only claims about what is known: the kept equations stay as they are */
	if (STORE->CHECKED[0] && !FRESH.ROW_COUNT)
	{
		STORE->AGE = 0;
		return (ADDED);
	}

	/* a word problem is only read when something is asked */
	if (!ADDED && HAS_QUESTION_CUE(LOWER))
	{
		int	START = 0;

		while (START < COUNT)
		{
			int	END = START;

			while (
				END < COUNT &&
				!(
					TOKENS[END].KIND == TOKEN_STOP &&
					strchr(".;?!\n", TOKENS[END].TEXT[0])
				)
			)
				END++;

			if (END - START >= 3)
				IS_WORDS += READ_WORD_SENTENCE(
					&FRESH, TOKENS + START, END - START, &IS_MONEY
				);

			START = END + 1;
		}

		/* one sentence is not a problem worth solving */
		if (IS_WORDS < 2)
			return (0);

		ADDED = IS_WORDS;
	}

	if (!ADDED)
		return (0);

	for (INDEX = 0; INDEX < FRESH.NAME_COUNT; INDEX++)
		if (FIND_NAME(STORE, FRESH.NAMES[INDEX]) >= 0)
			SHARED = 1;

	/* as many equations as unknowns: a whole problem of its own */
	if (FRESH.ROW_COUNT >= FRESH.NAME_COUNT)
		SHARED = 0;

	if (!SHARED || !STORE->ROW_COUNT)
	{
		*STORE = FRESH;
		STORE->IS_MONEY = IS_MONEY;
		STORE->IS_WORDS = IS_WORDS > 0;
		STORE->AGE = 0;
		return (ADDED);
	}

	/* more about the same unknowns */
	{
		int	ROW;

		ADDED = 0;

		for (ROW = 0; ROW < FRESH.ROW_COUNT; ROW++)
		{
			LINEAR	LEFT;
			LINEAR	RIGHT;
			int		NAME;
			int		FITS = 1;

			memset(&LEFT, 0, sizeof LEFT);
			memset(&RIGHT, 0, sizeof RIGHT);

			for (NAME = 0; NAME < FRESH.NAME_COUNT; NAME++)
			{
				int	MAPPED =
					ADD_NAME(STORE, FRESH.NAMES[NAME], FRESH.SHOWN[NAME]);

				if (MAPPED < 0)
				{
					FITS = 0;
					break ;
				}

				LEFT.COEFFICIENTS[MAPPED] = FRESH.ROWS[ROW][NAME];
			}

			RIGHT.CONSTANT = FRESH.ROWS[ROW][SOLVE_MAX_NAMES];

			if (FITS)
				ADDED += ADD_ROW(STORE, &LEFT, &RIGHT);
		}

		STORE->IS_MONEY |= IS_MONEY;
		STORE->AGE = 0;
	}

	/* said again, nothing new: still a problem being talked about */
	if (ADDED)
		return (ADDED);

	return (1);
}

static int
	MENTIONS_NAME(const char *LOWER, const char *NAME, const char *FROM)
{
	const char	*CURSOR = FROM;
	size_t		LENGTH = strlen(NAME);

	while ((CURSOR = strstr(CURSOR, NAME)))
	{
		int	STARTS = CURSOR == LOWER || !isalpha((uint8_t)CURSOR[-1]);
		int	ENDS = !isalpha((uint8_t)CURSOR[LENGTH]) ||
			(CURSOR[LENGTH] == 's' && !isalpha((uint8_t)CURSOR[LENGTH + 1]));

		if (STARTS && ENDS)
			return (1);

		CURSOR++;
	}

	return (0);
}

/* "can you calculate the value of the ball?", "how much is the ball?",
 * "what is x?", "solve it": which unknown (-1 for all of them) */
int
	SOLVE_ASKED(SOLVE_STORE *STORE, const char *TEXT, int *WHICH)
{
	static const char	*CUES[18] = {
		"how much", "how many", "what is", "what's", "whats", "what does",
		"what do", "find", "calculate", "work out", "figure out", "value of",
		"solve", "price of", "cost of", "tell me", "what are", NULL
	};
	static const char	*ALL_CUES[6] = {
		"solve", "work out", "figure out", "calculate", "find", NULL
	};
	char				LOWER[2000];
	const char			*FIRST_CUE = NULL;
	int					INDEX;

	*WHICH = -1;

	if (!STORE->ROW_COUNT || !TEXT)
		return (0);

	LOWER_COPY(TEXT, LOWER, sizeof LOWER);

	for (INDEX = 0; CUES[INDEX]; INDEX++)
	{
		const char	*AT = strstr(LOWER, CUES[INDEX]);

		if (AT && (!FIRST_CUE || AT < FIRST_CUE))
			FIRST_CUE = AT;
	}

	if (!FIRST_CUE)
		return (0);

	{
		const char	*BEST = NULL;

		for (INDEX = 0; INDEX < STORE->NAME_COUNT; INDEX++)
		{
			const char	*CURSOR = FIRST_CUE;

			/* the first unknown named after the question words */
			while ((CURSOR = strstr(CURSOR, STORE->NAMES[INDEX])))
			{
				if (MENTIONS_NAME(LOWER, STORE->NAMES[INDEX], CURSOR))
					break ;

				CURSOR++;
			}

			if (CURSOR && (!BEST || CURSOR < BEST))
			{
				BEST = CURSOR;
				*WHICH = INDEX;
			}
		}

		if (BEST)
			return (1);
	}

	/* "solve it", "can you work it out?": no numbers of its own */
	if (!strchr(LOWER, '='))
		for (INDEX = 0; LOWER[INDEX]; INDEX++)
			if (isdigit((uint8_t)LOWER[INDEX]) || strchr("+*/", LOWER[INDEX]))
				return (0);

	for (INDEX = 0; ALL_CUES[INDEX]; INDEX++)
		if (strstr(LOWER, ALL_CUES[INDEX]))
			return (1);

	return (0);
}

static void
	SAY_NUMBER(double VALUE, int IS_MONEY, char *OUTPUT, int OUTPUT_SIZE)
{
	double	ROUNDED = round(VALUE * 1e9) / 1e9;
	char	DIGITS[64];

	if (fabs(ROUNDED) < 1e-12)
		ROUNDED = 0;

	if (fabs(ROUNDED - round(ROUNDED)) < 1e-9 && fabs(ROUNDED) < 1e15)
		snprintf(DIGITS, sizeof DIGITS, "%.0f", round(ROUNDED));
	else if (IS_MONEY && fabs(ROUNDED * 100 - round(ROUNDED * 100)) < 1e-6)
		snprintf(DIGITS, sizeof DIGITS, "%.2f", ROUNDED);
	else
		snprintf(DIGITS, sizeof DIGITS, "%.10g", ROUNDED);

	if (IS_MONEY && DIGITS[0] == '-')
		snprintf(OUTPUT, OUTPUT_SIZE, "-$%s", DIGITS + 1);
	else if (IS_MONEY)
		snprintf(OUTPUT, OUTPUT_SIZE, "$%s", DIGITS);
	else
		snprintf(OUTPUT, OUTPUT_SIZE, "%s", DIGITS);
}

/* elimination: each unknown is known (KNOWN 1, VALUE), free (0) or the
 * equations disagree (return 0) */
static int
	ELIMINATE(SOLVE_STORE *STORE, int *KNOWN, double *VALUES)
{
	double	ROWS[SOLVE_MAX_ROWS][SOLVE_MAX_NAMES + 1];
	int		PIVOT_OF_ROW[SOLVE_MAX_ROWS];
	int		COLUMNS = STORE->NAME_COUNT;
	int		RANK = 0;
	int		COLUMN;
	int		ROW;

	memcpy(ROWS, STORE->ROWS, sizeof ROWS);

	for (COLUMN = 0; COLUMN < COLUMNS && RANK < STORE->ROW_COUNT; COLUMN++)
	{
		int		BEST = -1;
		double	BEST_SIZE = 1e-10;
		int		INDEX;

		for (ROW = RANK; ROW < STORE->ROW_COUNT; ROW++)
			if (fabs(ROWS[ROW][COLUMN]) > BEST_SIZE)
			{
				BEST_SIZE = fabs(ROWS[ROW][COLUMN]);
				BEST = ROW;
			}

		if (BEST < 0)
			continue ;

		if (BEST != RANK)
			for (INDEX = 0; INDEX <= SOLVE_MAX_NAMES; INDEX++)
			{
				double	SWAP = ROWS[RANK][INDEX];

				ROWS[RANK][INDEX] = ROWS[BEST][INDEX];
				ROWS[BEST][INDEX] = SWAP;
			}

		{
			double	PIVOT = ROWS[RANK][COLUMN];

			for (INDEX = 0; INDEX <= SOLVE_MAX_NAMES; INDEX++)
				ROWS[RANK][INDEX] /= PIVOT;
		}

		for (ROW = 0; ROW < STORE->ROW_COUNT; ROW++)
		{
			double	FACTOR = ROWS[ROW][COLUMN];

			if (ROW == RANK || fabs(FACTOR) < 1e-15)
				continue ;

			for (INDEX = 0; INDEX <= SOLVE_MAX_NAMES; INDEX++)
				ROWS[ROW][INDEX] -= FACTOR * ROWS[RANK][INDEX];
		}

		PIVOT_OF_ROW[RANK] = COLUMN;
		RANK++;
	}

	/* 0 = something: the equations disagree */
	for (ROW = RANK; ROW < STORE->ROW_COUNT; ROW++)
		if (fabs(ROWS[ROW][SOLVE_MAX_NAMES]) > 1e-9)
			return (0);

	for (COLUMN = 0; COLUMN < COLUMNS; COLUMN++)
		KNOWN[COLUMN] = 0;

	for (ROW = 0; ROW < RANK; ROW++)
	{
		int	PIVOT = PIVOT_OF_ROW[ROW];
		int	ALONE = 1;

		for (COLUMN = 0; COLUMN < COLUMNS; COLUMN++)
			if (COLUMN != PIVOT && fabs(ROWS[ROW][COLUMN]) > 1e-12)
				ALONE = 0;

		if (ALONE)
		{
			KNOWN[PIVOT] = 1;
			VALUES[PIVOT] = ROWS[ROW][SOLVE_MAX_NAMES];
		}
	}

	return (1);
}

static void
	SAY_ONE(
		SOLVE_STORE *STORE, int INDEX, double VALUE, char *OUTPUT,
		int OUTPUT_SIZE, int IS_FIRST
	)
{
	char	NUMBER[80];
	char	UNIT[40] = "";

	SAY_NUMBER(VALUE, STORE->IS_MONEY, NUMBER, sizeof NUMBER);

	if (STORE->UNIT[0])
	{
		size_t	LENGTH;

		snprintf(UNIT, sizeof UNIT, " %s", STORE->UNIT);
		LENGTH = strlen(UNIT);

		/* "1 apple", "5 apples" */
		if (fabs(VALUE - 1) < 1e-9 && LENGTH > 3 && UNIT[LENGTH - 1] == 's')
			UNIT[LENGTH - 1] = 0;
	}

	if (STORE->IS_WORDS)
		snprintf(
			OUTPUT, OUTPUT_SIZE, "%s%s %s %s%s",
			isupper((uint8_t)STORE->SHOWN[INDEX][0]) ? ""
				: IS_FIRST ? "The "
			: "the ",
			STORE->SHOWN[INDEX], STORE->VERB[0] ? STORE->VERB : "is", NUMBER,
			UNIT
		);
	else
		snprintf(OUTPUT, OUTPUT_SIZE, "%s = %s", STORE->SHOWN[INDEX], NUMBER);
}

/* "The ball costs $0.05, and the bat costs $1.05.", "[Ball] = 0.05, and
 * [Bat] = 1.05." */
int
	SOLVE_ANSWER(SOLVE_STORE *STORE, int WHICH, char *OUTPUT, int OUTPUT_SIZE)
{
	int		KNOWN[SOLVE_MAX_NAMES];
	double	VALUES[SOLVE_MAX_NAMES];
	int		ORDER[SOLVE_MAX_NAMES];
	int		ORDER_COUNT = 0;
	int		POSITION = 0;
	int		SAID = 0;
	int		INDEX;

	OUTPUT[0] = 0;

	if (!STORE->ROW_COUNT)
		return (0);

	STORE->AGE = 0;

	if (!ELIMINATE(STORE, KNOWN, VALUES))
	{
		snprintf(
			OUTPUT, OUTPUT_SIZE,
			"Those equations don't fit together, so there is no answer for "
			"%s.",
			WHICH >= 0 ? STORE->SHOWN[WHICH] : "them"
		);
		return (1);
	}

	if (WHICH >= 0 && !KNOWN[WHICH])
	{
		snprintf(
			OUTPUT, OUTPUT_SIZE,
			"I can't work out %s yet. I need one more equation about it.",
			STORE->SHOWN[WHICH]
		);
		return (1);
	}

	if (WHICH >= 0)
		ORDER[ORDER_COUNT++] = WHICH;

	for (INDEX = 0; INDEX < STORE->NAME_COUNT; INDEX++)
		if (INDEX != WHICH && KNOWN[INDEX])
			ORDER[ORDER_COUNT++] = INDEX;

	if (!ORDER_COUNT)
	{
		snprintf(
			OUTPUT, OUTPUT_SIZE,
			"I can't work those out yet. I need one more equation."
		);
		return (1);
	}

	for (INDEX = 0; INDEX < ORDER_COUNT && INDEX < 4; INDEX++)
	{
		char	PIECE[200];

		SAY_ONE(
			STORE, ORDER[INDEX], VALUES[ORDER[INDEX]], PIECE, sizeof PIECE,
			!SAID
		);
		POSITION += snprintf(
			OUTPUT + POSITION, OUTPUT_SIZE - POSITION, "%s%s",
			!SAID ? ""
				: INDEX + 1 == ORDER_COUNT || INDEX == 3 ? ", and "
			: ", ",
			PIECE
		);
		SAID++;

		if (POSITION >= OUTPUT_SIZE - 2)
			break ;
	}

	if (POSITION < OUTPUT_SIZE - 2)
		snprintf(OUTPUT + POSITION, OUTPUT_SIZE - POSITION, ".");

	return (1);
}

/* the answer put back into every equation: "$1.05 + $0.05 = $1.10, and
 * $1.05 - $0.05 = $1" (1 when every unknown is known and every equation
 * holds) */
int
	SOLVE_CHECK(SOLVE_STORE *STORE, char *OUTPUT, int OUTPUT_SIZE)
{
	int		KNOWN[SOLVE_MAX_NAMES];
	double	VALUES[SOLVE_MAX_NAMES];
	int		POSITION = 0;
	int		ROW;
	int		INDEX;

	OUTPUT[0] = 0;

	if (!STORE->ROW_COUNT || !ELIMINATE(STORE, KNOWN, VALUES))
		return (0);

	/* every unknown known; a negative one is said plainly elsewhere */
	for (INDEX = 0; INDEX < STORE->NAME_COUNT; INDEX++)
		if (!KNOWN[INDEX] || VALUES[INDEX] < -1e-12)
			return (0);

	for (ROW = 0; ROW < STORE->ROW_COUNT && ROW < 4; ROW++)
	{
		double	TOTAL = 0;
		int		TERMS = 0;
		char	NUMBER[80];

		POSITION += snprintf(
			OUTPUT + POSITION, OUTPUT_SIZE - POSITION, "%s",
			ROW ? (ROW + 1 == STORE->ROW_COUNT || ROW == 3 ? ", and " : ", ")
				: ""
		);

		for (
			INDEX = 0;
			INDEX < STORE->NAME_COUNT && POSITION < OUTPUT_SIZE - 2;
			INDEX++
		)
		{
			double	FACTOR = STORE->ROWS[ROW][INDEX];

			if (fabs(FACTOR) < 1e-12)
				continue ;

			TOTAL += FACTOR * VALUES[INDEX];
			SAY_NUMBER(VALUES[INDEX], STORE->IS_MONEY, NUMBER, sizeof NUMBER);

			if (TERMS)
				POSITION += snprintf(
					OUTPUT + POSITION, OUTPUT_SIZE - POSITION, " %c ",
					FACTOR < 0 ? '-' : '+'
				);
			else if (FACTOR < 0)
				POSITION +=
					snprintf(OUTPUT + POSITION, OUTPUT_SIZE - POSITION, "-");

			if (fabs(fabs(FACTOR) - 1) > 1e-12)
			{
				char	TIMES[80];

				SAY_NUMBER(fabs(FACTOR), 0, TIMES, sizeof TIMES);
				POSITION += snprintf(
					OUTPUT + POSITION, OUTPUT_SIZE - POSITION, "%s * ", TIMES
				);
			}

			POSITION += snprintf(
				OUTPUT + POSITION, OUTPUT_SIZE - POSITION, "%s", NUMBER
			);
			TERMS++;
		}

		if (!TERMS || POSITION >= OUTPUT_SIZE - 2)
			return (0);

		/* every equation has to hold with the values found */
		if (
			fabs(TOTAL - STORE->ROWS[ROW][SOLVE_MAX_NAMES]) >
				1e-6 * (fabs(TOTAL) > 1 ? fabs(TOTAL) : 1)
		)
			return (0);

		SAY_NUMBER(
			STORE->ROWS[ROW][SOLVE_MAX_NAMES], STORE->IS_MONEY, NUMBER,
			sizeof NUMBER
		);
		POSITION += snprintf(
			OUTPUT + POSITION, OUTPUT_SIZE - POSITION, " = %s", NUMBER
		);

		if (POSITION >= OUTPUT_SIZE - 2)
			return (0);
	}

	return (1);
}

/* "what is x + 23?", "x + 23 = ?", "x + 1": an expression whose unknowns are
 * all known, worked out */
int
	SOLVE_EVALUATE(
		SOLVE_STORE *STORE, const char *TEXT, char *OUTPUT, int OUTPUT_SIZE
	)
{
	static SOLVE_TOKEN	TOKENS[SOLVE_MAX_TOKENS];
	char				SIGNS[2000];
	int					IS_KNOWN[SOLVE_MAX_NAMES];
	double				VALUES[SOLVE_MAX_NAMES];
	int					COUNT;
	int					START = 0;

	OUTPUT[0] = 0;

	if (!STORE->ROW_COUNT || !TEXT)
		return (0);

	{
		SOLVE_STORE	COPY = *STORE;

		if (!ELIMINATE(&COPY, IS_KNOWN, VALUES))
			return (0);
	}

	MATH_WORDS_TO_SIGNS(TEXT, SIGNS, sizeof SIGNS);
	COUNT = TOKENIZE(SIGNS, TOKENS, SOLVE_MAX_TOKENS);
	MARK_WORD_UNKNOWNS(TOKENS, COUNT, STORE);

	while (START < COUNT)
	{
		int	END = START;
		int	EQUALS = -1;
		int	EQUALS_COUNT = 0;
		int	HAS_UNKNOWN = 0;
		int	HAS_SIGN = 0;
		int	INDEX;

		while (
			END < COUNT &&
			TOKENS[END].KIND != TOKEN_WORD &&
			TOKENS[END].KIND != TOKEN_STOP
		)
			END++;

		for (INDEX = START; INDEX < END; INDEX++)
		{
			if (IS_SIGN(&TOKENS[INDEX], "="))
			{
				EQUALS = INDEX;
				EQUALS_COUNT++;
			}
			else if (IS_SIGN(&TOKENS[INDEX], "+-*/("))
				HAS_SIGN = 1;

			if (TOKENS[INDEX].KIND == TOKEN_UNKNOWN)
				HAS_UNKNOWN = 1;
		}

		/* "x + 23" or "x + 23 = ?" (the sign last), never "x = 42" */
		if (
			HAS_UNKNOWN &&
			HAS_SIGN &&
			(EQUALS_COUNT == 0 || (EQUALS_COUNT == 1 && EQUALS == END - 1))
		)
		{
			SOLVE_STORE	TRIAL = *STORE;
			int			LAST;

			if (EQUALS_COUNT)
				LAST = EQUALS;
			else
				LAST = END;

			SOLVE_PARSER	PARSER = { TOKENS, LAST, START, &TRIAL, 0 };
			LINEAR			VALUE_FORM;

			if (
				PARSE_SUM(&PARSER, &VALUE_FORM) &&
				PARSER.POSITION == LAST &&
				TRIAL.NAME_COUNT == STORE->NAME_COUNT
			)
			{
				double	VALUE;

				if (
					VALUE_OF(
						&VALUE_FORM, &TRIAL, STORE, IS_KNOWN, VALUES, &VALUE
					)
				)
				{
					char	EXPRESSION[200];
					char	NUMBER[64];
					char	WHEN[200];
					LINEAR	NONE;

					memset(&NONE, 0, sizeof NONE);
					TOKENS_TEXT(
						TOKENS, START, LAST, EXPRESSION, sizeof EXPRESSION
					);
					SAY_NUMBER(VALUE, STORE->IS_MONEY, NUMBER, sizeof NUMBER);
					SAY_KNOWN_VALUES(
						&TRIAL, &VALUE_FORM, &NONE, STORE, VALUES, WHEN,
						sizeof WHEN
					);
					snprintf(
						OUTPUT, OUTPUT_SIZE, "When %s, %s = %s.", WHEN,
						EXPRESSION, NUMBER
					);
					STORE->AGE = 0;
					return (1);
				}
			}
		}

		START = END + 1;
	}

	return (0);
}

/* ---- whole numbers of any length, exactly ---- */

static int
	BIG_COMPARE(const char *FIRST, const char *SECOND)
{
	size_t	FIRST_LENGTH = strlen(FIRST);
	size_t	SECOND_LENGTH = strlen(SECOND);

	if (FIRST_LENGTH != SECOND_LENGTH)
	{
		if (FIRST_LENGTH < SECOND_LENGTH)
			return (-1);

		return (1);
	}

	return (strcmp(FIRST, SECOND));
}

static void
	BIG_TRIM(char *NUMBER)
{
	size_t	START = 0;

	while (NUMBER[START] == '0' && NUMBER[START + 1])
		START++;

	if (START)
		memmove(NUMBER, NUMBER + START, strlen(NUMBER + START) + 1);
}

/* FIRST + SECOND, both without signs */
static void
	BIG_ADD(const char *FIRST, const char *SECOND, char *OUTPUT, int SIZE)
{
	int		FIRST_AT = (int)strlen(FIRST) - 1;
	int		SECOND_AT = (int)strlen(SECOND) - 1;
	int		CARRY = 0;
	char	REVERSED[1200];
	int		LENGTH = 0;
	int		INDEX;

	while ((FIRST_AT >= 0 || SECOND_AT >= 0 || CARRY) && LENGTH < 1199)
	{
		int	SUM = CARRY;

		if (FIRST_AT >= 0)
			SUM += FIRST[FIRST_AT--] - '0';

		if (SECOND_AT >= 0)
			SUM += SECOND[SECOND_AT--] - '0';

		REVERSED[LENGTH++] = (char)('0' + SUM % 10);
		CARRY = SUM / 10;
	}

	for (INDEX = 0; INDEX < LENGTH && INDEX < SIZE - 1; INDEX++)
		OUTPUT[INDEX] = REVERSED[LENGTH - 1 - INDEX];

	OUTPUT[INDEX] = 0;
	BIG_TRIM(OUTPUT);
}

/* FIRST - SECOND, FIRST at least as big, both without signs */
static void
	BIG_SUBTRACT(const char *FIRST, const char *SECOND, char *OUTPUT, int SIZE)
{
	int		FIRST_AT = (int)strlen(FIRST) - 1;
	int		SECOND_AT = (int)strlen(SECOND) - 1;
	int		BORROW = 0;
	char	REVERSED[1200];
	int		LENGTH = 0;
	int		INDEX;

	while (FIRST_AT >= 0 && LENGTH < 1199)
	{
		int	DIGIT = FIRST[FIRST_AT--] - '0' - BORROW;

		if (SECOND_AT >= 0)
			DIGIT -= SECOND[SECOND_AT--] - '0';

		BORROW = DIGIT < 0;

		if (BORROW)
			DIGIT += 10;

		REVERSED[LENGTH++] = (char)('0' + DIGIT);
	}

	for (INDEX = 0; INDEX < LENGTH && INDEX < SIZE - 1; INDEX++)
		OUTPUT[INDEX] = REVERSED[LENGTH - 1 - INDEX];

	OUTPUT[INDEX] = 0;
	BIG_TRIM(OUTPUT);
}

static void
	BIG_MULTIPLY(const char *FIRST, const char *SECOND, char *OUTPUT, int SIZE)
{
	int	FIRST_LENGTH = (int)strlen(FIRST);
	int	SECOND_LENGTH = (int)strlen(SECOND);
	int	DIGITS[1200];
	int	LENGTH = FIRST_LENGTH + SECOND_LENGTH;
	int	FIRST_INDEX;
	int	SECOND_INDEX;
	int	INDEX;

	if (LENGTH >= 1200)
	{
		snprintf(OUTPUT, SIZE, "0");
		return ;
	}

	memset(DIGITS, 0, sizeof DIGITS);

	for (FIRST_INDEX = FIRST_LENGTH - 1; FIRST_INDEX >= 0; FIRST_INDEX--)
		for (
			SECOND_INDEX = SECOND_LENGTH - 1;
			SECOND_INDEX >= 0;
			SECOND_INDEX--
		)
		{
			int	AT = FIRST_INDEX + SECOND_INDEX + 1;
			int	SUM = DIGITS[AT] +
				(FIRST[FIRST_INDEX] - '0') * (SECOND[SECOND_INDEX] - '0');

			DIGITS[AT] = SUM % 10;
			DIGITS[AT - 1] += SUM / 10;
		}

	for (INDEX = 0; INDEX < LENGTH && INDEX < SIZE - 1; INDEX++)
		OUTPUT[INDEX] = (char)('0' + DIGITS[INDEX]);

	OUTPUT[INDEX] = 0;
	BIG_TRIM(OUTPUT);
}

typedef struct
{
	int		IS_NEGATIVE;
	char	DIGITS[1200];
} BIG;

static void
	BIG_SIGNED_ADD(const BIG *FIRST, const BIG *SECOND, BIG *OUTPUT)
{
	BIG	RESULT;

	if (FIRST->IS_NEGATIVE == SECOND->IS_NEGATIVE)
	{
		BIG_ADD(FIRST->DIGITS, SECOND->DIGITS, RESULT.DIGITS, 1200);
		RESULT.IS_NEGATIVE = FIRST->IS_NEGATIVE;
	}
	else if (BIG_COMPARE(FIRST->DIGITS, SECOND->DIGITS) >= 0)
	{
		BIG_SUBTRACT(FIRST->DIGITS, SECOND->DIGITS, RESULT.DIGITS, 1200);
		RESULT.IS_NEGATIVE = FIRST->IS_NEGATIVE;
	}
	else
	{
		BIG_SUBTRACT(SECOND->DIGITS, FIRST->DIGITS, RESULT.DIGITS, 1200);
		RESULT.IS_NEGATIVE = SECOND->IS_NEGATIVE;
	}

	if (!strcmp(RESULT.DIGITS, "0"))
		RESULT.IS_NEGATIVE = 0;

	*OUTPUT = RESULT;
}

/* "491827461294871928132751946195818946291386198 - 1": a sum of whole
 * numbers with + - * where one has 16 digits or more (too long for the
 * calculator's numbers), worked out digit by digit */
int
	SOLVE_BIG(
		const char *TEXT, char *EXPRESSION, int EXPRESSION_SIZE, char *RESULT,
		int RESULT_SIZE
	)
{
	const char	*CURSOR = TEXT;

	while (*CURSOR)
	{
		BIG			TOTAL;
		BIG			TERM;
		const char	*START;
		const char	*AT;
		int			LONGEST = 0;
		int			NUMBERS = 0;
		char		PENDING = '+';
		int			EXPRESSION_LENGTH = 0;

		while (*CURSOR && !isdigit((uint8_t)*CURSOR))
			CURSOR++;

		if (!*CURSOR)
			break ;

		START = CURSOR;
		AT = CURSOR;
		memset(&TOTAL, 0, sizeof TOTAL);
		snprintf(TOTAL.DIGITS, sizeof TOTAL.DIGITS, "0");
		memset(&TERM, 0, sizeof TERM);
		EXPRESSION[0] = 0;

		for (;;)
		{
			char	NUMBER[1200];
			int		LENGTH = 0;
			char	SIGN;

			while (*AT == ' ')
				AT++;

			if (!isdigit((uint8_t)*AT))
				break ;

			while (isdigit((uint8_t)*AT) && LENGTH < 1100)
				NUMBER[LENGTH++] = *AT++;

			NUMBER[LENGTH] = 0;

			/* "3.5" is not a whole number */
			if (*AT == '.' && isdigit((uint8_t)AT[1]))
			{
				NUMBERS = 0;
				break ;
			}

			BIG_TRIM(NUMBER);

			if (LENGTH > LONGEST)
				LONGEST = LENGTH;

			NUMBERS++;
			EXPRESSION_LENGTH += snprintf(
				EXPRESSION + EXPRESSION_LENGTH,
				EXPRESSION_SIZE - EXPRESSION_LENGTH, "%s%s",
				NUMBERS > 1 ? " " : "", NUMBER
			);

			if (PENDING == '*')
				BIG_MULTIPLY(TERM.DIGITS, NUMBER, TERM.DIGITS, 1200);
			else
			{
				/* the term before goes into the total */
				if (NUMBERS > 1)
					BIG_SIGNED_ADD(&TOTAL, &TERM, &TOTAL);

				snprintf(TERM.DIGITS, sizeof TERM.DIGITS, "%s", NUMBER);
				TERM.IS_NEGATIVE = PENDING == '-';
			}

			while (*AT == ' ')
				AT++;

			SIGN = *AT;

			if ((unsigned char)AT[0] == 0XC3 && ((unsigned char)AT[1] == 0X97))
			{
				SIGN = '*';
				AT++;
			}
			else if (SIGN == 'x' && (AT[1] == ' ' || isdigit((uint8_t)AT[1])))
				SIGN = '*';

			if (SIGN != '+' && SIGN != '-' && SIGN != '*')
				break ;

			/* the next thing has to be a number */
			{
				const char	*NEXT = AT + 1;

				while (*NEXT == ' ')
					NEXT++;

				if (!isdigit((uint8_t)*NEXT))
					break ;
			}

			EXPRESSION_LENGTH += snprintf(
				EXPRESSION + EXPRESSION_LENGTH,
				EXPRESSION_SIZE - EXPRESSION_LENGTH, " %c", SIGN
			);
			PENDING = SIGN;
			AT++;
		}

		if (
			NUMBERS >= 2 &&
			LONGEST >= 16 &&
			EXPRESSION_LENGTH < EXPRESSION_SIZE
		)
		{
			BIG_SIGNED_ADD(&TOTAL, &TERM, &TOTAL);
			snprintf(
				RESULT, RESULT_SIZE, "%s%s", TOTAL.IS_NEGATIVE ? "-" : "",
				TOTAL.DIGITS
			);
			return (1);
		}

		if (AT > START)
			CURSOR = AT;
		else
			CURSOR = START + 1;
	}

	return (0);
}
