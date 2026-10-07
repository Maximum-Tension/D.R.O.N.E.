#include "../SKILL.h"
#include <ctype.h>
#include <math.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

/* The reasoning skill: puzzles that have one right answer, worked out
 * exactly instead of guessed.  Sequences ("2, 4, 6, 8, ?"), primes and
 * even numbers, "all cats are animals, Tom is a cat", "Alice is taller than
 * Bob ... who is the tallest?", letters of a word, days of the week,
 * sorting, units, the largest or the average of some numbers, clock times,
 * fractions, rates and ages.  Lucy asks it about every message; it answers
 * only the ones it is sure about. */

#define MAX_TOKENS 200

enum
{
	TOKEN_WORD = 1,
	TOKEN_NUMBER,
	TOKEN_SYMBOL
};

typedef struct
{
	char	TEXT[64];
	char	ORIGINAL[64];
	int		KIND;
	double	VALUE;
	int		HOUR;
	int		MINUTE;
	int		IS_FRACTION;
	long	NUMERATOR;
	long	DENOMINATOR;
	int		IS_NUMBER_WORD;
	int		START;
	int		END;
} TOKEN;

typedef struct
{
	const char	*MESSAGE;
	TOKEN		TOKENS[MAX_TOKENS];
	int			COUNT;
	char		LOWER[2400];
} READING;

/* ---------- reading the message ---------- */

static const char	*NUMBER_NAMES[21] = {
	"zero", "one", "two", "three", "four", "five", "six", "seven", "eight",
	"nine", "ten", "eleven", "twelve", "thirteen", "fourteen", "fifteen",
	"sixteen", "seventeen", "eighteen", "nineteen", NULL
};
static const char	*TENS_NAMES[9] = {
	"twenty", "thirty", "forty", "fifty", "sixty", "seventy", "eighty",
	"ninety", NULL
};

static int
	FIND_WORD(const char *const *LIST, const char *WORD)
{
	int	INDEX;

	for (INDEX = 0; LIST[INDEX]; INDEX++)
		if (!strcmp(LIST[INDEX], WORD))
			return (INDEX);

	return (-1);
}

static void
	ADD_TOKEN(
		READING *READ, const char *START, int LENGTH, int KIND, int OFFSET
	)
{
	TOKEN	*TOKEN_ITEM;
	int		INDEX;

	if (READ->COUNT >= MAX_TOKENS || LENGTH <= 0)
		return ;

	TOKEN_ITEM = &READ->TOKENS[READ->COUNT++];
	memset(TOKEN_ITEM, 0, sizeof *TOKEN_ITEM);

	if (LENGTH > 63)
		LENGTH = 63;

	memcpy(TOKEN_ITEM->ORIGINAL, START, (size_t)LENGTH);
	TOKEN_ITEM->ORIGINAL[LENGTH] = 0;

	for (INDEX = 0; INDEX < LENGTH; INDEX++)
		TOKEN_ITEM->TEXT[INDEX] = (char)tolower((uint8_t)START[INDEX]);

	TOKEN_ITEM->TEXT[LENGTH] = 0;
	TOKEN_ITEM->KIND = KIND;
	TOKEN_ITEM->HOUR = -1;
	TOKEN_ITEM->START = OFFSET;
	TOKEN_ITEM->END = OFFSET + LENGTH;
}

/* words, numbers ("9.11", "-3", "1/2", "10:30", "1,000") and symbols */
static void
	READ_MESSAGE(const char *MESSAGE, READING *READ)
{
	const char	*CURSOR = MESSAGE;
	int			POSITION = 0;
	int			INDEX;

	memset(READ, 0, sizeof *READ);
	READ->MESSAGE = MESSAGE;

	while (*CURSOR)
	{
		const char	*START = CURSOR;

		if (isspace((uint8_t)*CURSOR))
		{
			CURSOR++;
			continue ;
		}

		if (isalpha((uint8_t)*CURSOR))
		{
			while (
				isalpha((uint8_t)*CURSOR) ||
				(*CURSOR == '\'' && isalpha((uint8_t)CURSOR[1]))
			)
				CURSOR++;

			ADD_TOKEN(
				READ, START, (int)(CURSOR - START), TOKEN_WORD,
				(int)(START - MESSAGE)
			);
			continue ;
		}

		if (
			isdigit((uint8_t)*CURSOR) ||
			(*CURSOR == '.' && isdigit((uint8_t)CURSOR[1])) ||
			(
				*CURSOR == '-' &&
				isdigit((uint8_t)CURSOR[1]) &&
				(
					READ->COUNT == 0 ||
					READ->TOKENS[READ->COUNT - 1].KIND == TOKEN_SYMBOL ||
					READ->TOKENS[READ->COUNT - 1].KIND == TOKEN_WORD
				)
			)
		)
		{
			TOKEN	*NUMBER_TOKEN;
			char	TEXT[64];
			int		LENGTH;

			if (*CURSOR == '-')
				CURSOR++;

			while (
				isdigit((uint8_t)*CURSOR) ||
				(*CURSOR == '.' && isdigit((uint8_t)CURSOR[1])) ||
				(
					*CURSOR == ',' &&
					isdigit((uint8_t)CURSOR[1]) &&
					isdigit((uint8_t)CURSOR[2]) &&
					isdigit((uint8_t)CURSOR[3]) &&
					!isdigit((uint8_t)CURSOR[4])
				)
			)
				CURSOR++;

			LENGTH = (int)(CURSOR - START);

			/* "10:30" is a time */
			if (
				*CURSOR == ':' &&
				isdigit((uint8_t)CURSOR[1]) &&
				isdigit((uint8_t)CURSOR[2]) &&
				!isdigit((uint8_t)CURSOR[3])
			)
			{
				ADD_TOKEN(
					READ, START, (int)(CURSOR + 3 - START), TOKEN_NUMBER,
					(int)(START - MESSAGE)
				);
				NUMBER_TOKEN = &READ->TOKENS[READ->COUNT - 1];
				NUMBER_TOKEN->HOUR = atoi(START);
				NUMBER_TOKEN->MINUTE = atoi(CURSOR + 1);
				NUMBER_TOKEN->VALUE = NUMBER_TOKEN->HOUR;
				CURSOR += 3;
				continue ;
			}

			/* "1/2" is a fraction ("03/10/2026" is a date) */
			if (
				*CURSOR == '/' &&
				isdigit((uint8_t)CURSOR[1]) &&
				!memchr(START, '.', (size_t)(CURSOR - START))
			)
			{
				const char	*AFTER = CURSOR + 1;

				while (isdigit((uint8_t)*AFTER))
					AFTER++;

				if (*AFTER == '/' || *AFTER == '-')
				{
					while (*AFTER && !isspace((uint8_t)*AFTER))
						AFTER++;

					ADD_TOKEN(
						READ, START, (int)(AFTER - START), TOKEN_SYMBOL,
						(int)(START - MESSAGE)
					);
					CURSOR = AFTER;
					continue ;
				}

				ADD_TOKEN(
					READ, START, (int)(AFTER - START), TOKEN_NUMBER,
					(int)(START - MESSAGE)
				);
				NUMBER_TOKEN = &READ->TOKENS[READ->COUNT - 1];
				NUMBER_TOKEN->IS_FRACTION = 1;
				NUMBER_TOKEN->NUMERATOR = strtol(START, NULL, 10);
				NUMBER_TOKEN->DENOMINATOR = strtol(CURSOR + 1, NULL, 10);

				if (NUMBER_TOKEN->DENOMINATOR)
					NUMBER_TOKEN->VALUE = (double)NUMBER_TOKEN->NUMERATOR /
						(double)NUMBER_TOKEN->DENOMINATOR;
				else
					NUMBER_TOKEN->VALUE = 0;

				CURSOR = AFTER;
				continue ;
			}

			ADD_TOKEN(
				READ, START, LENGTH, TOKEN_NUMBER, (int)(START - MESSAGE)
			);
			NUMBER_TOKEN = &READ->TOKENS[READ->COUNT - 1];

			{
				int	OUT = 0;

				for (INDEX = 0; INDEX < LENGTH && OUT < 63; INDEX++)
					if (START[INDEX] != ',')
						TEXT[OUT++] = START[INDEX];

				TEXT[OUT] = 0;
			}

			NUMBER_TOKEN->VALUE = strtod(TEXT, NULL);
			continue ;
		}

		ADD_TOKEN(READ, START, 1, TOKEN_SYMBOL, (int)(START - MESSAGE));
		CURSOR++;
	}

	/* number words: "three", "twenty five", "a hundred" */
	for (INDEX = 0; INDEX < READ->COUNT; INDEX++)
	{
		TOKEN	*ITEM = &READ->TOKENS[INDEX];
		int		FOUND;

		if (ITEM->KIND != TOKEN_WORD)
			continue ;

		if ((FOUND = FIND_WORD(NUMBER_NAMES, ITEM->TEXT)) >= 0)
		{
			ITEM->KIND = TOKEN_NUMBER;
			ITEM->VALUE = FOUND;
			ITEM->IS_NUMBER_WORD = 1;
		}
		else if ((FOUND = FIND_WORD(TENS_NAMES, ITEM->TEXT)) >= 0)
		{
			ITEM->KIND = TOKEN_NUMBER;
			ITEM->VALUE = 20 + 10 * FOUND;
			ITEM->IS_NUMBER_WORD = 1;

			/* "twenty five", "twenty-five" */
			{
				int	NEXT = INDEX + 1;
				int	UNIT;

				if (NEXT < READ->COUNT && !strcmp(READ->TOKENS[NEXT].TEXT, "-"))
					NEXT++;

				if (
					NEXT < READ->COUNT &&
					READ->TOKENS[NEXT].KIND == TOKEN_WORD &&
					(UNIT = FIND_WORD(NUMBER_NAMES, READ->TOKENS[NEXT].TEXT)
					) > 0 &&
					UNIT < 10
				)
				{
					ITEM->VALUE += UNIT;
					ITEM->END = READ->TOKENS[NEXT].END;
					memmove(
						&READ->TOKENS[INDEX + 1], &READ->TOKENS[NEXT + 1],
						(size_t)(READ->COUNT - NEXT - 1) * sizeof(TOKEN)
					);
					READ->COUNT -= NEXT - INDEX;
				}
			}
		}
		else if (
			!strcmp(ITEM->TEXT, "hundred") ||
			!strcmp(ITEM->TEXT, "thousand")
		)
		{
			double	SCALE;

			if (ITEM->TEXT[0] == 'h')
				SCALE = 100;
			else
				SCALE = 1000;

			if (
				INDEX > 0 &&
				READ->TOKENS[INDEX - 1].KIND == TOKEN_NUMBER &&
				READ->TOKENS[INDEX - 1].IS_NUMBER_WORD
			)
			{
				READ->TOKENS[INDEX - 1].VALUE *= SCALE;
				memmove(
					&READ->TOKENS[INDEX], &READ->TOKENS[INDEX + 1],
					(size_t)(READ->COUNT - INDEX - 1) * sizeof(TOKEN)
				);
				READ->COUNT--;
				INDEX--;
			}
			else if (
				INDEX > 0 &&
				(
					!strcmp(READ->TOKENS[INDEX - 1].TEXT, "a") ||
					!strcmp(READ->TOKENS[INDEX - 1].TEXT, "one")
				)
			)
			{
				READ->TOKENS[INDEX - 1].KIND = TOKEN_NUMBER;
				READ->TOKENS[INDEX - 1].VALUE = SCALE;
				READ->TOKENS[INDEX - 1].IS_NUMBER_WORD = 1;
				memmove(
					&READ->TOKENS[INDEX], &READ->TOKENS[INDEX + 1],
					(size_t)(READ->COUNT - INDEX - 1) * sizeof(TOKEN)
				);
				READ->COUNT--;
				INDEX--;
			}
		}
	}

	/* the plain lower case words, a space at both ends */
	READ->LOWER[POSITION++] = ' ';

	for (INDEX = 0; INDEX < READ->COUNT && POSITION < 2300; INDEX++)
	{
		const TOKEN	*ITEM = &READ->TOKENS[INDEX];

		if (ITEM->KIND == TOKEN_SYMBOL && strchr(",.!?;:\"()", ITEM->TEXT[0]))
			continue ;

		POSITION += snprintf(
			READ->LOWER + POSITION, sizeof READ->LOWER - POSITION, "%s ",
			ITEM->TEXT
		);
	}

	READ->LOWER[POSITION] = 0;
}

static int
	HAS(const READING *READ, const char *PHRASE)
{
	return (strstr(READ->LOWER, PHRASE) != NULL);
}

static int
	IS_NUMBER(const TOKEN *ITEM)
{
	return (ITEM->KIND == TOKEN_NUMBER && ITEM->HOUR < 0);
}

/* "10", "2.5", "1/3": as people write them */
static void
	SAY_NUMBER(double VALUE, char *OUTPUT, int OUTPUT_SIZE)
{
	if (fabs(VALUE - floor(VALUE + 0.5)) < 1e-9 && fabs(VALUE) < 1e15)
		snprintf(OUTPUT, OUTPUT_SIZE, "%.0f", floor(VALUE + 0.5));
	else
	{
		int	LENGTH;

		snprintf(OUTPUT, OUTPUT_SIZE, "%.6f", VALUE);
		LENGTH = (int)strlen(OUTPUT);

		while (LENGTH > 1 && OUTPUT[LENGTH - 1] == '0')
			OUTPUT[--LENGTH] = 0;

		if (LENGTH && OUTPUT[LENGTH - 1] == '.')
			OUTPUT[--LENGTH] = 0;
	}

	if (!strcmp(OUTPUT, "-0"))
		snprintf(OUTPUT, OUTPUT_SIZE, "0");
}

/* how the answer was found, after the why mark: not said, but there when
 * Lucy is asked "why?" */
static void
	ADD_WHY(char *REPLY, int REPLY_SIZE, const char *FORMAT, ...)
{
	size_t	LENGTH = strlen(REPLY);
	va_list	ARGUMENTS;

	if (strchr(REPLY, SKILL_WHY_MARK) || LENGTH + 4 >= (size_t)REPLY_SIZE)
		return ;

	REPLY[LENGTH++] = SKILL_WHY_MARK;
	REPLY[LENGTH] = 0;
	va_start(ARGUMENTS, FORMAT);
	vsnprintf(REPLY + LENGTH, (size_t)REPLY_SIZE - LENGTH, FORMAT, ARGUMENTS);
	va_end(ARGUMENTS);
}

static const char
	*ARTICLE(const char *WORD)
{
	return (strchr("aeiouAEIOU", WORD[0]) ? "an" : "a");
}

/* the numbers of a list ("2, 4, 6, 8", "4, 8 and 12"): the longest run of
 * numbers with only commas, "and", "then" between them */
static int
	NUMBER_LIST(
		const READING *READ, int FROM, double *VALUES, int LIMIT, int *FIRST,
		int *LAST
	)
{
	int	BEST_START = -1;
	int	BEST_COUNT = 0;
	int	INDEX;

	for (INDEX = FROM; INDEX < READ->COUNT; INDEX++)
	{
		int	COUNT = 0;
		int	SCAN = INDEX;
		int	LAST_NUMBER = INDEX;

		if (!IS_NUMBER(&READ->TOKENS[INDEX]) || READ->TOKENS[INDEX].IS_FRACTION)
			continue ;

		while (SCAN < READ->COUNT)
		{
			const TOKEN	*ITEM = &READ->TOKENS[SCAN];

			if (IS_NUMBER(ITEM) && !ITEM->IS_FRACTION)
			{
				COUNT++;
				LAST_NUMBER = SCAN;
				SCAN++;
				continue ;
			}

			if (
				!strcmp(ITEM->TEXT, ",") ||
				!strcmp(ITEM->TEXT, "and") ||
				!strcmp(ITEM->TEXT, "then") ||
				!strcmp(ITEM->TEXT, ";") ||
				!strcmp(ITEM->TEXT, "or")
			)
			{
				SCAN++;
				continue ;
			}

			break ;
		}

		if (COUNT > BEST_COUNT)
		{
			BEST_COUNT = COUNT;
			BEST_START = INDEX;

			if (LAST)
				*LAST = LAST_NUMBER;
		}

		INDEX = SCAN - 1;
	}

	if (BEST_START < 0)
		return (0);

	if (FIRST)
		*FIRST = BEST_START;

	{
		int	COUNT = 0;

		for (
			INDEX = BEST_START;
			INDEX < READ->COUNT && COUNT < BEST_COUNT && COUNT < LIMIT;
			INDEX++
		)
			if (IS_NUMBER(&READ->TOKENS[INDEX]))
				VALUES[COUNT++] = READ->TOKENS[INDEX].VALUE;

		return (COUNT);
	}
}

/* ---------- sequences ---------- */

static int
	IS_PRIME_NUMBER(long long VALUE)
{
	long long	DIVISOR;

	if (VALUE < 2)
		return (0);

	if (VALUE < 4)
		return (1);

	if (VALUE % 2 == 0)
		return (0);

	for (DIVISOR = 3; DIVISOR * DIVISOR <= VALUE; DIVISOR += 2)
		if (VALUE % DIVISOR == 0)
			return (0);

	return (1);
}

static int
	SAME(double FIRST, double SECOND)
{
	return (fabs(FIRST - SECOND) <= 1e-9 * (1 + fabs(FIRST) + fabs(SECOND)));
}

/* the next value and why, or 0 */
static int
	NEXT_IN_SEQUENCE(
		const double *VALUES, int COUNT, double *NEXT, char *WHY, int WHY_SIZE,
		const char *ITEM
	)
{
	double	DIFFERENCES[64];
	int		INDEX;
	int		ALL_SAME;
	char	NUMBER_TEXT[64];

	if (COUNT < 3 || COUNT > 60)
		return (0);

	for (INDEX = 0; INDEX + 1 < COUNT; INDEX++)
		DIFFERENCES[INDEX] = VALUES[INDEX + 1] - VALUES[INDEX];

	/* the same step every time */
	for (ALL_SAME = 1, INDEX = 1; INDEX + 1 < COUNT; INDEX++)
		if (!SAME(DIFFERENCES[INDEX], DIFFERENCES[0]))
			ALL_SAME = 0;

	if (ALL_SAME)
	{
		*NEXT = VALUES[COUNT - 1] + DIFFERENCES[0];
		SAY_NUMBER(fabs(DIFFERENCES[0]), NUMBER_TEXT, sizeof NUMBER_TEXT);

		if (SAME(DIFFERENCES[0], 0))
			snprintf(WHY, WHY_SIZE, "it stays the same");
		else
			snprintf(
				WHY, WHY_SIZE, "each %s is %s %s the one before", ITEM,
				NUMBER_TEXT, DIFFERENCES[0] > 0 ? "more than" : "less than"
			);

		return (1);
	}

	/* the same factor every time */
	{
		int		OK = 1;
		double	RATIO = 0;

		for (INDEX = 0; INDEX + 1 < COUNT && OK; INDEX++)
		{
			if (SAME(VALUES[INDEX], 0))
			{
				OK = 0;
				break ;
			}

			if (INDEX == 0)
				RATIO = VALUES[1] / VALUES[0];
			else if (!SAME(VALUES[INDEX + 1] / VALUES[INDEX], RATIO))
				OK = 0;
		}

		if (OK && !SAME(RATIO, 1))
		{
			*NEXT = VALUES[COUNT - 1] * RATIO;

			if (fabs(RATIO) >= 1)
			{
				SAY_NUMBER(RATIO, NUMBER_TEXT, sizeof NUMBER_TEXT);
				snprintf(
					WHY, WHY_SIZE, "each %s is %s times the one before", ITEM,
					NUMBER_TEXT
				);
			}
			else
			{
				SAY_NUMBER(1 / RATIO, NUMBER_TEXT, sizeof NUMBER_TEXT);
				snprintf(
					WHY, WHY_SIZE, "each %s is the one before divided by %s",
					ITEM, NUMBER_TEXT
				);
			}

			return (1);
		}
	}

	/* each the sum of the two before it */
	{
		int	OK = COUNT >= 4;

		for (INDEX = 2; INDEX < COUNT && OK; INDEX++)
			if (!SAME(VALUES[INDEX], VALUES[INDEX - 1] + VALUES[INDEX - 2]))
				OK = 0;

		if (OK)
		{
			*NEXT = VALUES[COUNT - 1] + VALUES[COUNT - 2];
			snprintf(
				WHY, WHY_SIZE, "each %s is the sum of the two before it", ITEM
			);
			return (1);
		}
	}

	/* squares and cubes of whole numbers in a row */
	{
		int	POWER;

		for (POWER = 2; POWER <= 3; POWER++)
		{
			int		OK = 1;
			double	BASE = pow(fabs(VALUES[0]), 1.0 / POWER);

			BASE = floor(BASE + 0.5);

			for (INDEX = 0; INDEX < COUNT && OK; INDEX++)
				if (!SAME(VALUES[INDEX], pow(BASE + INDEX, POWER)))
					OK = 0;

			if (OK)
			{
				*NEXT = pow(BASE + COUNT, POWER);
				snprintf(
					WHY, WHY_SIZE, "they are the %s of %s, %s, %s and so on",
					POWER == 2 ? "squares" : "cubes",
					BASE == 0 ? "0"
						: BASE == 1 ? "1"
									: "the numbers in a row",
					BASE + 1 == 2 ? "2" : "the next", "the next"
				);

				/* "the squares of 1, 2, 3 and so on" */
				{
					char	FIRST_TEXT[32];
					char	SECOND_TEXT[32];
					char	THIRD_TEXT[32];

					SAY_NUMBER(BASE, FIRST_TEXT, sizeof FIRST_TEXT);
					SAY_NUMBER(BASE + 1, SECOND_TEXT, sizeof SECOND_TEXT);
					SAY_NUMBER(BASE + 2, THIRD_TEXT, sizeof THIRD_TEXT);
					snprintf(
						WHY, WHY_SIZE,
						"they are the %s of %s, %s, %s and so on",
						POWER == 2 ? "squares" : "cubes", FIRST_TEXT,
						SECOND_TEXT, THIRD_TEXT
					);
				}

				return (1);
			}
		}
	}

	/* the steps grow by the same amount: 1, 2, 4, 7, 11 */
	{
		int	OK = COUNT >= 4;

		for (INDEX = 2; INDEX + 1 < COUNT && OK; INDEX++)
			if (
				!SAME(
					DIFFERENCES[INDEX] - DIFFERENCES[INDEX - 1],
					DIFFERENCES[1] - DIFFERENCES[0]
				)
			)
				OK = 0;

		if (OK && !SAME(DIFFERENCES[1] - DIFFERENCES[0], 0))
		{
			double	GROWTH = DIFFERENCES[1] - DIFFERENCES[0];
			double	STEP = DIFFERENCES[COUNT - 2] + GROWTH;

			*NEXT = VALUES[COUNT - 1] + STEP;
			SAY_NUMBER(fabs(GROWTH), NUMBER_TEXT, sizeof NUMBER_TEXT);
			snprintf(
				WHY, WHY_SIZE, "the step %s by %s each time",
				GROWTH > 0 ? "grows" : "shrinks", NUMBER_TEXT
			);
			return (1);
		}
	}

	/* times 2, times 3, times 4: 1, 2, 6, 24 */
	{
		int		OK = COUNT >= 4;
		double	FACTOR = 0;

		for (INDEX = 0; INDEX + 1 < COUNT && OK; INDEX++)
		{
			double	THIS;

			if (SAME(VALUES[INDEX], 0))
			{
				OK = 0;
				break ;
			}

			THIS = VALUES[INDEX + 1] / VALUES[INDEX];

			if (INDEX == 0)
				FACTOR = THIS;
			else if (!SAME(THIS, FACTOR + INDEX))
				OK = 0;
		}

		if (OK && fabs(FACTOR - floor(FACTOR + 0.5)) < 1e-9)
		{
			*NEXT = VALUES[COUNT - 1] * (FACTOR + COUNT - 1);
			snprintf(
				WHY, WHY_SIZE,
				"each %s is the one before times the next whole number", ITEM
			);
			return (1);
		}
	}

	/* two steps taking turns: +1, +2, +1, +2 */
	{
		int	OK = COUNT >= 5;

		for (INDEX = 2; INDEX + 1 < COUNT && OK; INDEX++)
			if (!SAME(DIFFERENCES[INDEX], DIFFERENCES[INDEX - 2]))
				OK = 0;

		if (OK && !SAME(DIFFERENCES[0], DIFFERENCES[1]))
		{
			char	FIRST_TEXT[32];
			char	SECOND_TEXT[32];

			*NEXT = VALUES[COUNT - 1] + DIFFERENCES[COUNT - 3];
			SAY_NUMBER(DIFFERENCES[0], FIRST_TEXT, sizeof FIRST_TEXT);
			SAY_NUMBER(DIFFERENCES[1], SECOND_TEXT, sizeof SECOND_TEXT);
			snprintf(
				WHY, WHY_SIZE, "the steps take turns: %s%s, then %s%s",
				DIFFERENCES[0] >= 0 ? "+" : "", FIRST_TEXT,
				DIFFERENCES[1] >= 0 ? "+" : "", SECOND_TEXT
			);
			return (1);
		}
	}

	/* prime numbers in a row */
	{
		int			OK = 1;
		long long	VALUE;

		for (INDEX = 0; INDEX < COUNT && OK; INDEX++)
		{
			if (
				VALUES[INDEX] != floor(VALUES[INDEX]) ||
				!IS_PRIME_NUMBER((long long)VALUES[INDEX])
			)
				OK = 0;

			if (OK && INDEX)
				for (
					VALUE = (long long)VALUES[INDEX - 1] + 1;
					VALUE < (long long)VALUES[INDEX];
					VALUE++
				)
					if (IS_PRIME_NUMBER(VALUE))
						OK = 0;
		}

		if (OK)
		{
			for (
				VALUE = (long long)VALUES[COUNT - 1] + 1;
				!IS_PRIME_NUMBER(VALUE);
				VALUE++
			)
				;

			*NEXT = (double)VALUE;
			snprintf(WHY, WHY_SIZE, "they are the prime numbers in order");
			return (1);
		}
	}

	return (0);
}

/* how the next one comes from the ones before, step by step, added to the
 * reply after the why mark: "Each number is 3 more than the one before: 3 +
 * 3 = 6, 6 + 3 = 9, and 9 + 3 = 12." */
static void
	ADD_SEQUENCE_WORK(
		const double *VALUES, int COUNT, double NEXT, const char *RULE,
		char *REPLY, int REPLY_SIZE
	)
{
	char	WORK[700] = "";
	int		POSITION = 0;
	int		SAME_STEP = COUNT >= 2;
	int		SAME_FACTOR = COUNT >= 2;
	int		FROM;

	if (COUNT > 4)
		FROM = COUNT - 4;
	else
		FROM = 0;

	int		INDEX;
	double	STEP;

	if (COUNT >= 2)
		STEP = VALUES[1] - VALUES[0];
	else
		STEP = 0;

	double	FACTOR;

	if (COUNT >= 2 && VALUES[0] != 0)
		FACTOR = VALUES[1] / VALUES[0];
	else
		FACTOR = 0;

	size_t	LENGTH = strlen(REPLY);

	for (INDEX = 1; INDEX < COUNT; INDEX++)
	{
		if (fabs(VALUES[INDEX] - VALUES[INDEX - 1] - STEP) > 1e-9)
			SAME_STEP = 0;

		if (
			VALUES[INDEX - 1] == 0 ||
			fabs(VALUES[INDEX] / VALUES[INDEX - 1] - FACTOR) > 1e-9
		)
			SAME_FACTOR = 0;
	}

	if (SAME_STEP && fabs(NEXT - VALUES[COUNT - 1] - STEP) > 1e-9)
		SAME_STEP = 0;

	if (SAME_FACTOR && fabs(NEXT - VALUES[COUNT - 1] * FACTOR) > 1e-9)
		SAME_FACTOR = 0;

	if (SAME_STEP || (SAME_FACTOR && fabs(FACTOR) != 1))
	{
		char	SIZE_TEXT[64];

		SAY_NUMBER(
			SAME_STEP ? fabs(STEP) : FACTOR, SIZE_TEXT, sizeof SIZE_TEXT
		);

		for (INDEX = FROM + 1; INDEX <= COUNT && POSITION < 600; INDEX++)
		{
			char	LEFT[64];
			char	RIGHT[64];

			SAY_NUMBER(VALUES[INDEX - 1], LEFT, sizeof LEFT);
			SAY_NUMBER(
				INDEX < COUNT ? VALUES[INDEX] : NEXT, RIGHT, sizeof RIGHT
			);
			POSITION += snprintf(
				WORK + POSITION, sizeof WORK - POSITION, "%s%s %s %s = %s",
				INDEX == FROM + 1 ? ""
					: INDEX == COUNT ? ", and "
				: ", ",
				LEFT, SAME_STEP ? (STEP < 0 ? "-" : "+") : "*", SIZE_TEXT, RIGHT
			);
		}
	}
	else
	{
		char	LAST[64];
		char	BEFORE_LAST[64];
		char	NEXT_TEXT[64];
		double	ROOT = sqrt(fabs(NEXT));

		SAY_NUMBER(VALUES[COUNT - 1], LAST, sizeof LAST);
		SAY_NUMBER(
			COUNT >= 2 ? VALUES[COUNT - 2] : 0, BEFORE_LAST, sizeof BEFORE_LAST
		);
		SAY_NUMBER(NEXT, NEXT_TEXT, sizeof NEXT_TEXT);

		/* the sum of the two before: "5 + 8 = 13" */
		if (
			COUNT >= 3 &&
			fabs(VALUES[COUNT - 1] + VALUES[COUNT - 2] - NEXT) < 1e-9 &&
			strstr(RULE, "sum")
		)
			snprintf(
				WORK, sizeof WORK, "so the next one is %s + %s = %s",
				BEFORE_LAST, LAST, NEXT_TEXT
			);
		/* squares: "6 * 6 = 36" */
		else if (
			strstr(RULE, "square") &&
			fabs(ROOT - floor(ROOT + 0.5)) < 1e-9
		)
		{
			char	ROOT_TEXT[64];

			SAY_NUMBER(floor(ROOT + 0.5), ROOT_TEXT, sizeof ROOT_TEXT);
			snprintf(
				WORK, sizeof WORK, "so the next one is %s * %s = %s", ROOT_TEXT,
				ROOT_TEXT, NEXT_TEXT
			);
		}
		else
			snprintf(
				WORK, sizeof WORK, "so after %s comes %s", LAST, NEXT_TEXT
			);
	}

	if (LENGTH + strlen(RULE) + strlen(WORK) + 8 < (size_t)REPLY_SIZE)
		snprintf(
			REPLY + LENGTH, REPLY_SIZE - LENGTH, "%c%c%s%s %s.", SKILL_WHY_MARK,
			toupper((uint8_t)RULE[0]), RULE + 1,
			SAME_STEP || SAME_FACTOR ? ":" : ",", WORK
		);
}

static int
	ANSWER_SEQUENCE(
		const READING *READ, char *ACTION, int ACTION_SIZE, char *REPLY,
		int REPLY_SIZE
	)
{
	static const char	*ASKS[38] = {
		" what comes next ", " what's next ", " whats next ", " next number ",
		" next term ", " next in the sequence ", " next in this sequence ",
		" next in the series ", " next in this series ",
		" continue the sequence ", " complete the sequence ",
		" continue the pattern ", " complete the pattern ",
		" what number comes next ", " next letter ", " what letter comes next ",
		" comes next in ", " the next one ", " next in the pattern ",
		" next item ", " what follows ", " complete the series ",
		" continue the series ", " finish the series ", " finish the sequence ",
		" finish the pattern ", " what goes next ", " what should come next ",
		" next value ", " next one ", " missing number ", " what comes after ",
		" next in line ", " extend the sequence ", " extend the pattern ",
		" next element ", " next two ", NULL
	};
	double				VALUES[64];
	int					COUNT;
	int					INDEX;
	int					ASKED = 0;
	double				NEXT;
	char				WHY[300];
	char				NEXT_TEXT[64];

	for (INDEX = 0; ASKS[INDEX]; INDEX++)
		if (HAS(READ, ASKS[INDEX]))
			ASKED = 1;

	/* "next: 1, 1, 2, 3, 5, 8" */
	if (!ASKED && HAS(READ, " next "))
	{
		double	LISTED[64];

		if (NUMBER_LIST(READ, 0, LISTED, 64, NULL, NULL) >= 3)
			ASKED = 1;
	}

	/* "2, 4, 6, 8, ?" with nothing else: the list asks by itself */
	if (!ASKED && READ->COUNT >= 6)
	{
		int	WORDS = 0;

		for (INDEX = 0; INDEX < READ->COUNT; INDEX++)
			if (
				READ->TOKENS[INDEX].KIND == TOKEN_WORD &&
				strlen(READ->TOKENS[INDEX].TEXT) > 1
			)
				WORDS++;

		if (
			!WORDS &&
			(
				READ->TOKENS[READ->COUNT - 1].TEXT[0] == '?' ||
				READ->TOKENS[READ->COUNT - 1].TEXT[0] == '_'
			)
		)
			ASKED = 1;
	}

	if (!ASKED)
		return (0);

	COUNT = NUMBER_LIST(READ, 0, VALUES, 64, NULL, NULL);

	if (COUNT >= 3)
	{
		if (!NEXT_IN_SEQUENCE(VALUES, COUNT, &NEXT, WHY, sizeof WHY, "number"))
		{
			snprintf(
				REPLY, REPLY_SIZE,
				"I can't find the rule in that sequence. Can you give me a few "
				"more numbers?"
			);
			snprintf(ACTION, ACTION_SIZE, "sequence: no rule found");
			return (1);
		}

		SAY_NUMBER(NEXT, NEXT_TEXT, sizeof NEXT_TEXT);
		snprintf(
			REPLY, REPLY_SIZE, "The next number is %s: %s.", NEXT_TEXT, WHY
		);
		ADD_SEQUENCE_WORK(VALUES, COUNT, NEXT, WHY, REPLY, REPLY_SIZE);
		snprintf(ACTION, ACTION_SIZE, "sequence -> %s", NEXT_TEXT);
		return (1);
	}

	/* letters: "A, C, E, ?" */
	{
		int		UPPER = 0;
		int		LETTERS = 0;
		int		BEST = 0;
		double	BEST_VALUES[64];
		int		BEST_UPPER = 0;

		for (INDEX = 0; INDEX < READ->COUNT; INDEX++)
		{
			const TOKEN	*ITEM = &READ->TOKENS[INDEX];

			if (
				ITEM->KIND == TOKEN_WORD &&
				strlen(ITEM->TEXT) == 1 &&
				strcmp(ITEM->ORIGINAL, "a") &&
				strcmp(ITEM->ORIGINAL, "I")
			)
			{
				if (LETTERS < 64)
					VALUES[LETTERS++] = ITEM->TEXT[0] - 'a' + 1;

				UPPER = isupper((uint8_t)ITEM->ORIGINAL[0]);
				continue ;
			}

			if (
				ITEM->KIND == TOKEN_SYMBOL &&
				(ITEM->TEXT[0] == ',' || ITEM->TEXT[0] == '-')
			)
				continue ;

			if (LETTERS > BEST)
			{
				BEST = LETTERS;
				memcpy(BEST_VALUES, VALUES, sizeof VALUES);
				BEST_UPPER = UPPER;
			}

			LETTERS = 0;
		}

		if (LETTERS > BEST)
		{
			BEST = LETTERS;
			memcpy(BEST_VALUES, VALUES, sizeof VALUES);
			BEST_UPPER = UPPER;
		}

		if (
			BEST >= 3 &&
			NEXT_IN_SEQUENCE(
				BEST_VALUES, BEST, &NEXT, WHY, sizeof WHY, "letter"
			)
		)
		{
			int	POSITION = (int)floor(NEXT + 0.5);

			if (POSITION < 1 || POSITION > 26)
			{
				snprintf(
					REPLY, REPLY_SIZE,
					"That sequence runs past the end of the alphabet."
				);
				snprintf(ACTION, ACTION_SIZE, "letter sequence: past z");
				return (1);
			}

			snprintf(
				REPLY, REPLY_SIZE, "The next letter is %c: %s in the alphabet.",
				(BEST_UPPER ? 'A' : 'a') + POSITION - 1, WHY
			);
			snprintf(
				ACTION, ACTION_SIZE, "letter sequence -> %c",
				(BEST_UPPER ? 'A' : 'a') + POSITION - 1
			);
			return (1);
		}
	}

	return (0);
}

/* ---------- numbers: prime, even, odd, divisible, compare ---------- */

static int
	ANSWER_NUMBER_FACTS(
		const READING *READ, char *ACTION, int ACTION_SIZE, char *REPLY,
		int REPLY_SIZE
	)
{
	int			INDEX;
	const TOKEN	*FIRST = NULL;
	const TOKEN	*SECOND = NULL;
	char		FIRST_TEXT[64];
	char		SECOND_TEXT[64];

	if (
		strncmp(READ->LOWER, " is ", 4) &&
		!HAS(READ, " tell me if ") &&
		!HAS(READ, " is the number ")
	)
	{
		/* "Answer with only yes or no: is 10 greater than 5?" */
		if (!HAS(READ, " is "))
			return (0);
	}

	for (INDEX = 0; INDEX < READ->COUNT; INDEX++)
		if (IS_NUMBER(&READ->TOKENS[INDEX]))
		{
			if (!FIRST)
				FIRST = &READ->TOKENS[INDEX];
			else if (!SECOND)
				SECOND = &READ->TOKENS[INDEX];
		}

	if (!FIRST || FIRST->IS_FRACTION)
		return (0);

	SAY_NUMBER(FIRST->VALUE, FIRST_TEXT, sizeof FIRST_TEXT);

	if (SECOND)
		SAY_NUMBER(SECOND->VALUE, SECOND_TEXT, sizeof SECOND_TEXT);

	/* "is 17 a prime number?" */
	if (HAS(READ, " prime ") && !SECOND && HAS(READ, " is "))
	{
		long long	VALUE = (long long)FIRST->VALUE;
		long long	DIVISOR;

		if (FIRST->VALUE != floor(FIRST->VALUE) || FIRST->VALUE > 1e12)
			return (0);

		if (IS_PRIME_NUMBER(VALUE))
		{
			long long	LIMIT = (long long)floor(sqrt((double)VALUE));

			snprintf(
				REPLY, REPLY_SIZE,
				"Yes, %s is a prime number: it can only be divided by 1 and "
				"itself.",
				FIRST_TEXT
			);

			if (LIMIT >= 2)
				ADD_WHY(
					REPLY, REPLY_SIZE,
					"I tried dividing %s by every whole "
					"number from 2 to %lld, and none of them goes in evenly. I "
					"can stop at %lld, because %lld * %lld = %lld is already "
					"more than %s.",
					FIRST_TEXT, LIMIT, LIMIT, LIMIT + 1, LIMIT + 1,
					(LIMIT + 1) * (LIMIT + 1), FIRST_TEXT
				);
			else
				ADD_WHY(
					REPLY, REPLY_SIZE,
					"No whole number between 1 and %s "
					"divides it.",
					FIRST_TEXT
				);
		}
		else if (VALUE < 2)
			snprintf(
				REPLY, REPLY_SIZE,
				"No, %s is not a prime number: a prime is a whole number above "
				"1 that only 1 and itself divide.",
				FIRST_TEXT
			);
		else
		{
			for (DIVISOR = 2; VALUE % DIVISOR; DIVISOR++)
				;

			snprintf(
				REPLY, REPLY_SIZE,
				"No, %s is not a prime number: %s = %lld x %lld.", FIRST_TEXT,
				FIRST_TEXT, DIVISOR, VALUE / DIVISOR
			);
			ADD_WHY(
				REPLY, REPLY_SIZE,
				"A prime can only be divided by 1 and "
				"itself, but %s can also be divided by %lld: %s / %lld = %lld.",
				FIRST_TEXT, DIVISOR, FIRST_TEXT, DIVISOR, VALUE / DIVISOR
			);
		}

		snprintf(
			ACTION, ACTION_SIZE, "prime %s -> %s", FIRST_TEXT,
			IS_PRIME_NUMBER(VALUE) ? "yes" : "no"
		);
		return (1);
	}

	/* "is 27 even or odd?" */
	if (
		(HAS(READ, " even ") || HAS(READ, " odd ")) &&
		!SECOND &&
		FIRST->VALUE == floor(FIRST->VALUE)
	)
	{
		long long	VALUE = (long long)FIRST->VALUE;
		int			IS_EVEN = VALUE % 2 == 0;

		if (HAS(READ, " even ") && HAS(READ, " odd "))
			snprintf(
				REPLY, REPLY_SIZE, "%s is %s.", FIRST_TEXT,
				IS_EVEN ? "even" : "odd"
			);
		else if (HAS(READ, " even "))
			snprintf(
				REPLY, REPLY_SIZE, "%s, %s is %s.", IS_EVEN ? "Yes" : "No",
				FIRST_TEXT, IS_EVEN ? "even" : "odd"
			);
		else
			snprintf(
				REPLY, REPLY_SIZE, "%s, %s is %s.", IS_EVEN ? "No" : "Yes",
				FIRST_TEXT, IS_EVEN ? "even" : "odd"
			);

		snprintf(
			ACTION, ACTION_SIZE, "parity %s -> %s", FIRST_TEXT,
			IS_EVEN ? "even" : "odd"
		);
		return (1);
	}

	/* "is 51 divisible by 3?", "is 12 a multiple of 4?" */
	if (
		SECOND &&
		(
			HAS(READ, " divisible by ") ||
			HAS(READ, " a multiple of ") ||
			HAS(READ, " a factor of ")
		)
	)
	{
		double	BIG = FIRST->VALUE;
		double	SMALL = SECOND->VALUE;
		int		YES;

		if (HAS(READ, " a factor of "))
		{
			BIG = SECOND->VALUE;
			SMALL = FIRST->VALUE;
		}

		if (SMALL == 0 || BIG != floor(BIG) || SMALL != floor(SMALL))
			return (0);

		YES = fmod(BIG, SMALL) == 0;

		if (HAS(READ, " a factor of "))
			snprintf(
				REPLY, REPLY_SIZE, "%s, %s %s a factor of %s%s",
				YES ? "Yes" : "No", FIRST_TEXT, YES ? "is" : "is not",
				SECOND_TEXT, YES ? "." : ""
			);
		else
			snprintf(
				REPLY, REPLY_SIZE, "%s, %s %s divisible by %s",
				YES ? "Yes" : "No", FIRST_TEXT, YES ? "is" : "is not",
				SECOND_TEXT
			);

		if (YES && !HAS(READ, " a factor of "))
		{
			char	QUOTIENT[64];

			SAY_NUMBER(BIG / SMALL, QUOTIENT, sizeof QUOTIENT);
			snprintf(
				REPLY + strlen(REPLY), REPLY_SIZE - strlen(REPLY),
				": %s / %s = %s.", FIRST_TEXT, SECOND_TEXT, QUOTIENT
			);
		}
		else if (!YES)
		{
			char	LEFT_OVER[64];

			SAY_NUMBER(fmod(BIG, SMALL), LEFT_OVER, sizeof LEFT_OVER);
			snprintf(
				REPLY + strlen(REPLY), REPLY_SIZE - strlen(REPLY),
				": dividing leaves %s over.", LEFT_OVER
			);
		}

		snprintf(
			ACTION, ACTION_SIZE, "divisible %s %s -> %s", FIRST_TEXT,
			SECOND_TEXT, YES ? "yes" : "no"
		);
		return (1);
	}

	/* "Answer with only yes or no: is 10 greater than 5?" (a plain "is 10
	 * greater than 5?" is the calculator's compare) */
	if (
		SECOND &&
		(HAS(READ, " yes or no ") || HAS(READ, " true or false ")) &&
		!HAS(READ, " times ") &&
		!HAS(READ, " plus ") &&
		!HAS(READ, " minus ") &&
		!HAS(READ, " divided ")
	)
	{
		for (
			INDEX = (int)(FIRST - READ->TOKENS) + 1;
			INDEX < (int)(SECOND - READ->TOKENS);
			INDEX++
		)
			if (
				READ->TOKENS[INDEX].KIND == TOKEN_SYMBOL &&
				strchr("+-*/x^", READ->TOKENS[INDEX].TEXT[0])
			)
				return (0);

		static const char	*MORE[6] = {
			" greater than ", " bigger than ", " larger than ", " more than ",
			" higher than ", NULL
		};
		static const char	*LESS[5] = {
			" less than ", " smaller than ", " lower than ", " fewer than ",
			NULL
		};
		int					WANT = 0;
		int					YES;

		for (INDEX = 0; MORE[INDEX]; INDEX++)
			if (HAS(READ, MORE[INDEX]))
				WANT = 1;

		for (INDEX = 0; LESS[INDEX]; INDEX++)
			if (HAS(READ, LESS[INDEX]))
				WANT = -1;

		if (HAS(READ, " equal to ") || HAS(READ, " the same as "))
			WANT = 2;

		if (!WANT)
			return (0);

		if (WANT == 1)
			YES = FIRST->VALUE > SECOND->VALUE;
		else if (WANT == -1)
			YES = FIRST->VALUE < SECOND->VALUE;
		else
			YES = SAME(FIRST->VALUE, SECOND->VALUE);

		snprintf(
			REPLY, REPLY_SIZE, "%s, %s is %s%s %s.", YES ? "Yes" : "No",
			FIRST->ORIGINAL, YES ? "" : "not ",
			WANT == 1 ? "greater than"
				: WANT == -1 ? "less than"
			: "equal to",
			SECOND->ORIGINAL
		);
		snprintf(
			ACTION, ACTION_SIZE, "compare %s %s -> %s", FIRST_TEXT, SECOND_TEXT,
			YES ? "yes" : "no"
		);
		return (1);
	}

	return (0);
}

/* ---------- "all cats are animals, Tom is a cat, is Tom an animal?" ---- */

/* "cats" -> "cat", "boxes" -> "box", "puppies" -> "puppy" */
static void
	SINGULAR(const char *WORD, char *OUTPUT, int OUTPUT_SIZE)
{
	static const char	*IRREGULAR[16][2] = {
		{ "people", "person" }, { "men", "man" }, { "women", "woman" },
		{ "children", "child" }, { "mice", "mouse" }, { "geese", "goose" },
		{ "feet", "foot" }, { "teeth", "tooth" }, { "wolves", "wolf" },
		{ "leaves", "leaf" }, { "knives", "knife" }, { "wives", "wife" },
		{ "lives", "life" }, { "calves", "calf" }, { "halves", "half" },
		{ NULL, NULL }
	};
	size_t				LENGTH = strlen(WORD);
	int					INDEX;

	for (INDEX = 0; IRREGULAR[INDEX][0]; INDEX++)
		if (!strcmp(WORD, IRREGULAR[INDEX][0]))
		{
			snprintf(OUTPUT, OUTPUT_SIZE, "%s", IRREGULAR[INDEX][1]);
			return ;
		}

	snprintf(OUTPUT, OUTPUT_SIZE, "%s", WORD);

	if (LENGTH > 4 && !strcmp(WORD + LENGTH - 3, "ies"))
		snprintf(OUTPUT + LENGTH - 3, OUTPUT_SIZE - (LENGTH - 3), "y");
	else if (
		LENGTH > 4 &&
		(
			!strcmp(WORD + LENGTH - 4, "ches") ||
			!strcmp(WORD + LENGTH - 4, "shes") ||
			!strcmp(WORD + LENGTH - 3, "xes") ||
			!strcmp(WORD + LENGTH - 4, "sses")
		)
	)
		OUTPUT[LENGTH - 2] = 0;
	else if (
		LENGTH > 3 &&
		WORD[LENGTH - 1] == 's' &&
		WORD[LENGTH - 2] != 's' &&
		WORD[LENGTH - 2] != 'u' &&
		WORD[LENGTH - 2] != 'i'
	)
		OUTPUT[LENGTH - 1] = 0;
}

/* "barks" -> "bark", "flies" -> "fly", "catches" -> "catch" */
static void
	VERB_STEM(const char *WORD, char *OUTPUT, int OUTPUT_SIZE)
{
	size_t	LENGTH = strlen(WORD);

	snprintf(OUTPUT, OUTPUT_SIZE, "%s", WORD);

	if (!strcmp(WORD, "has"))
		snprintf(OUTPUT, OUTPUT_SIZE, "have");
	else if (!strcmp(WORD, "does"))
		snprintf(OUTPUT, OUTPUT_SIZE, "do");
	else if (!strcmp(WORD, "goes"))
		snprintf(OUTPUT, OUTPUT_SIZE, "go");
	else if (LENGTH > 4 && !strcmp(WORD + LENGTH - 3, "ies"))
		snprintf(OUTPUT + LENGTH - 3, OUTPUT_SIZE - (LENGTH - 3), "y");
	else if (
		LENGTH > 4 &&
		(
			!strcmp(WORD + LENGTH - 4, "ches") ||
			!strcmp(WORD + LENGTH - 4, "shes") ||
			!strcmp(WORD + LENGTH - 3, "xes") ||
			!strcmp(WORD + LENGTH - 4, "sses") ||
			!strcmp(WORD + LENGTH - 3, "zes")
		)
	)
		OUTPUT[LENGTH - 2] = 0;
	else if (
		LENGTH > 3 &&
		WORD[LENGTH - 1] == 's' &&
		WORD[LENGTH - 2] != 's' &&
		WORD[LENGTH - 2] != 'u'
	)
		OUTPUT[LENGTH - 1] = 0;
	else if (LENGTH > 4 && !strcmp(WORD + LENGTH - 3, "ied"))
		snprintf(OUTPUT + LENGTH - 3, OUTPUT_SIZE - (LENGTH - 3), "y");
	else if (LENGTH > 4 && !strcmp(WORD + LENGTH - 2, "ed"))
	{
		/* "passed" -> "pass", "stopped" -> "stop" */
		OUTPUT[LENGTH - 2] = 0;

		if (
			LENGTH - 2 >= 3 &&
			OUTPUT[LENGTH - 3] == OUTPUT[LENGTH - 4] &&
			!strchr("aeiouls", OUTPUT[LENGTH - 3])
		)
			OUTPUT[LENGTH - 3] = 0;
	}
}

/* "pass" -> "passed", "like" -> "liked", "study" -> "studied" */
static void
	VERB_PAST(const char *WORD, char *OUTPUT, int OUTPUT_SIZE)
{
	size_t	LENGTH = strlen(WORD);

	if (!strcmp(WORD, "have"))
		snprintf(OUTPUT, OUTPUT_SIZE, "had");
	else if (!strcmp(WORD, "go"))
		snprintf(OUTPUT, OUTPUT_SIZE, "went");
	else if (!strcmp(WORD, "do"))
		snprintf(OUTPUT, OUTPUT_SIZE, "did");
	else if (LENGTH > 1 && WORD[LENGTH - 1] == 'e')
		snprintf(OUTPUT, OUTPUT_SIZE, "%sd", WORD);
	else if (
		LENGTH > 1 &&
		WORD[LENGTH - 1] == 'y' &&
		!strchr("aeiou", WORD[LENGTH - 2])
	)
		snprintf(OUTPUT, OUTPUT_SIZE, "%.*sied", (int)LENGTH - 1, WORD);
	else
		snprintf(OUTPUT, OUTPUT_SIZE, "%sed", WORD);
}

/* "bark" -> "barks", "fly" -> "flies", "catch" -> "catches" */
static void
	VERB_THIRD(const char *WORD, char *OUTPUT, int OUTPUT_SIZE)
{
	size_t	LENGTH = strlen(WORD);

	if (!strcmp(WORD, "have"))
		snprintf(OUTPUT, OUTPUT_SIZE, "has");
	else if (!strcmp(WORD, "do") || !strcmp(WORD, "go"))
		snprintf(OUTPUT, OUTPUT_SIZE, "%ses", WORD);
	else if (
		LENGTH > 1 &&
		WORD[LENGTH - 1] == 'y' &&
		!strchr("aeiou", WORD[LENGTH - 2])
	)
		snprintf(OUTPUT, OUTPUT_SIZE, "%.*sies", (int)LENGTH - 1, WORD);
	else if (
		(
			LENGTH > 1 &&
			(
				WORD[LENGTH - 1] == 's' ||
				WORD[LENGTH - 1] == 'x' ||
				WORD[LENGTH - 1] == 'z' ||
				WORD[LENGTH - 1] == 'o'
			)
		) ||
		(
			LENGTH > 2 &&
			(
				!strcmp(WORD + LENGTH - 2, "ch") ||
				!strcmp(WORD + LENGTH - 2, "sh")
			)
		)
	)
		snprintf(OUTPUT, OUTPUT_SIZE, "%ses", WORD);
	else
		snprintf(OUTPUT, OUTPUT_SIZE, "%ss", WORD);
}

#define LOGIC_TEXT 96
#define MAX_LOGIC 24

enum
{
	PREDICATE_IS = 1,
	PREDICATE_CAN,
	PREDICATE_HAVE,
	PREDICATE_DO
};

/* what is said about a thing or a kind: "is an animal", "can fly", "has
 * wings", "barks" */
typedef struct
{
	int		TYPE;
	char	KEY[LOGIC_TEXT];
	char	SHOWN[LOGIC_TEXT];
	char	SHOWN_PLURAL[LOGIC_TEXT];
	int		IS_NOUN;
	int		IS_NOT;
	int		PAST;
} PREDICATE;

/* "all cats are animals": what every one of a kind is */
typedef struct
{
	char		FROM[48];
	char		FROM_SHOWN[48];
	PREDICATE	WHAT;
	int			SOME;
	char		TEXT[200];
} LOGIC_RULE;

/* "Tom is a cat": what one thing is */
typedef struct
{
	char		NAME[48];
	PREDICATE	WHAT;
	char		TEXT[200];
} LOGIC_FACT;

typedef struct
{
	LOGIC_RULE	RULES[MAX_LOGIC];
	int			RULE_COUNT;
	LOGIC_FACT	FACTS[MAX_LOGIC];
	int			FACT_COUNT;
} LOGIC_BOOK;

static int	IS_LETTER_NAME(const READING *READ, int INDEX);
static int	IS_ARTICLE_AT(const READING *READ, int INDEX);

static int
	IS_WORD_TOKEN(const READING *READ, int INDEX)
{
	return (
		INDEX >= 0 &&
		INDEX < READ->COUNT &&
		READ->TOKENS[INDEX].KIND == TOKEN_WORD
	);
}

static const char
	*TEXT_AT(const READING *READ, int INDEX)
{
	if (INDEX < 0 || INDEX >= READ->COUNT)
		return ("");

	return (READ->TOKENS[INDEX].TEXT);
}

static int
	IN_LIST(const char *WORD, const char *const *LIST)
{
	return (FIND_WORD(LIST, WORD) >= 0);
}

static const char	*LOGIC_STOPS[21] = {
	"and", "but", "so", "then", "if", "because", "while", "or", "is", "are",
	"can", "does", "do", "therefore", "thus", "hence", "who", "which", "that",
	"when", NULL
};
static const char	*LOGIC_ADVERBS[13] = {
	"definitely", "necessarily", "certainly", "also", "really", "always",
	"still", "surely", "too", "then", "therefore", "actually", NULL
};
static const char	*NOT_A_NAME[73] = {
	"all", "every", "each", "no", "some", "any", "what", "who", "which",
	"where", "when", "why", "how", "it", "he", "she", "they", "we", "you", "i",
	"this", "that", "these", "those", "there", "here", "a", "an", "the", "if",
	"and", "so", "then", "is", "are", "can", "does", "do", "not", "one",
	"nobody", "everyone", "everybody", "someone", "somebody", "thing", "things",
	"yes", "ok", "okay", "but", "or", "my", "your", "his", "her", "our",
	"their", "its", "me", "him", "them", "us", "let", "lets", "let's", "please",
	"thanks", "today", "tomorrow", "yesterday", "now", NULL
};

static const char	*LOGIC_PLACES[8] = {
	"in", "of", "at", "from", "on", "with", "near", NULL
};
static const char	*LOGIC_DETERMINERS[12] = {
	"the", "a", "an", "my", "our", "your", "this", "that", "his", "her",
	"their", NULL
};

/* the words of a predicate, from START up to a stop: "climb trees" */
static int
	PREDICATE_WORDS(
		const READING *READ, int START, char *KEY, int KEY_SIZE, char *SHOWN,
		int SHOWN_SIZE, int STEM_FIRST, int SINGULAR_LAST
	)
{
	int	INDEX = START;
	int	COUNT = 0;
	int	KEY_LENGTH = 0;
	int	SHOWN_LENGTH = 0;
	int	LAST_KEY_AT = 0;

	KEY[0] = SHOWN[0] = 0;

	while (
		(
			IS_WORD_TOKEN(READ, INDEX) ||
			(
				COUNT &&
				INDEX < READ->COUNT &&
				READ->TOKENS[INDEX].KIND == TOKEN_NUMBER
			) ||
			(
				!COUNT &&
				STEM_FIRST == 0 &&
				INDEX < READ->COUNT &&
				READ->TOKENS[INDEX].KIND == TOKEN_NUMBER &&
				IS_WORD_TOKEN(READ, INDEX + 1)
			)
		) &&
		COUNT < 4
	)
	{
		const char	*WORD = READ->TOKENS[INDEX].TEXT;
		char		FORM[64];

		if (COUNT && IN_LIST(WORD, LOGIC_STOPS))
			break ;

		if (IN_LIST(WORD, LOGIC_ADVERBS))
		{
			INDEX++;
			continue ;
		}

		if (!COUNT && STEM_FIRST)
		{
			size_t	FORM_LENGTH;

			VERB_STEM(WORD, FORM, sizeof FORM);
			FORM_LENGTH = strlen(FORM);

			/* "like" and "liked" are the same verb */
			if (FORM_LENGTH > 3 && FORM[FORM_LENGTH - 1] == 'e')
				FORM[FORM_LENGTH - 1] = 0;
		}
		else
			snprintf(FORM, sizeof FORM, "%s", WORD);

		LAST_KEY_AT = KEY_LENGTH + (KEY_LENGTH ? 1 : 0);
		KEY_LENGTH += snprintf(
			KEY + KEY_LENGTH, KEY_SIZE - KEY_LENGTH, "%s%s",
			KEY_LENGTH ? " " : "", FORM
		);
		SHOWN_LENGTH += snprintf(
			SHOWN + SHOWN_LENGTH, SHOWN_SIZE - SHOWN_LENGTH, "%s%s",
			SHOWN_LENGTH ? " " : "", READ->TOKENS[INDEX].ORIGINAL
		);
		COUNT++;
		INDEX++;

		if (KEY_LENGTH >= KEY_SIZE - 1 || SHOWN_LENGTH >= SHOWN_SIZE - 1)
			break ;
	}

	/* "or not?" ends the question */
	if (
		COUNT >= 2 &&
		!strcmp(TEXT_AT(READ, INDEX - 1), "not") &&
		!strcmp(TEXT_AT(READ, INDEX - 2), "or")
	)
		COUNT -= 2;

	if (!COUNT)
		return (-1);

	if (SINGULAR_LAST)
	{
		char	LAST[64];

		SINGULAR(KEY + LAST_KEY_AT, LAST, sizeof LAST);
		snprintf(KEY + LAST_KEY_AT, KEY_SIZE - LAST_KEY_AT, "%s", LAST);
	}

	return (INDEX);
}

/* the predicate that starts with the verb at INDEX ("are animals", "can
 * fly", "has wings", "barks"); the token after it, or -1 */
static int
	READ_PREDICATE(
		const READING *READ, int INDEX, int PLURAL_SUBJECT, PREDICATE *WHAT
	)
{
	const char	*VERB = TEXT_AT(READ, INDEX);
	char		WORDS_KEY[LOGIC_TEXT];
	int			AFTER;
	int			IS_NOT = 0;
	int			PAST = 0;

	memset(WHAT, 0, sizeof *WHAT);

	if (
		!strcmp(VERB, "is") ||
		!strcmp(VERB, "are") ||
		!strcmp(VERB, "isn't") ||
		!strcmp(VERB, "aren't")
	)
	{
		int	HAD_ARTICLE = 0;

		IS_NOT = strchr(VERB, '\'') != NULL;
		INDEX++;

		while (IN_LIST(TEXT_AT(READ, INDEX), LOGIC_ADVERBS))
			INDEX++;

		if (!strcmp(TEXT_AT(READ, INDEX), "not"))
		{
			IS_NOT = !IS_NOT;
			INDEX++;
		}

		if (IS_ARTICLE_AT(READ, INDEX))
		{
			HAD_ARTICLE = 1;
			INDEX++;
		}

		AFTER = PREDICATE_WORDS(
			READ, INDEX, WORDS_KEY, sizeof WORDS_KEY, WHAT->SHOWN,
			sizeof WHAT->SHOWN, 0, 1
		);

		if (AFTER < 0)
			return (-1);

		WHAT->TYPE = PREDICATE_IS;
		snprintf(WHAT->KEY, sizeof WHAT->KEY, "is:%s", WORDS_KEY);

		/* "an animal", "animals" are kinds; "black", "happy" are not */
		{
			const char	*LAST = strrchr(WHAT->SHOWN, ' ');
			size_t		LENGTH;

			if (LAST)
				LAST = LAST + 1;
			else
				LAST = WHAT->SHOWN;

			LENGTH = strlen(LAST);
			WHAT->IS_NOUN = HAD_ARTICLE ||
				(!strcmp(VERB, "are") && LENGTH > 3 &&
					LAST[LENGTH - 1] == 's' && LAST[LENGTH - 2] != 's' &&
					LAST[LENGTH - 2] != 'u');
		}

		WHAT->IS_NOT = IS_NOT;
		return (AFTER);
	}

	if (
		!strcmp(VERB, "can") ||
		!strcmp(VERB, "cannot") ||
		!strcmp(VERB, "can't")
	)
	{
		IS_NOT = strcmp(VERB, "can") != 0;
		INDEX++;

		if (!strcmp(TEXT_AT(READ, INDEX), "not"))
		{
			IS_NOT = !IS_NOT;
			INDEX++;
		}

		AFTER = PREDICATE_WORDS(
			READ, INDEX, WORDS_KEY, sizeof WORDS_KEY, WHAT->SHOWN,
			sizeof WHAT->SHOWN, 1, 0
		);

		if (AFTER < 0)
			return (-1);

		WHAT->TYPE = PREDICATE_CAN;
		snprintf(WHAT->KEY, sizeof WHAT->KEY, "can:%s", WORDS_KEY);
		WHAT->IS_NOT = IS_NOT;
		return (AFTER);
	}

	if (!strcmp(VERB, "have") || !strcmp(VERB, "has"))
	{
		INDEX++;

		if (!strcmp(TEXT_AT(READ, INDEX), "no"))
		{
			IS_NOT = 1;
			INDEX++;
		}

		AFTER = PREDICATE_WORDS(
			READ, INDEX, WORDS_KEY, sizeof WORDS_KEY, WHAT->SHOWN,
			sizeof WHAT->SHOWN, 0, 1
		);

		if (AFTER < 0)
			return (-1);

		WHAT->TYPE = PREDICATE_HAVE;
		snprintf(WHAT->KEY, sizeof WHAT->KEY, "have:%s", WORDS_KEY);
		WHAT->IS_NOT = IS_NOT;
		return (AFTER);
	}

	if (
		!strcmp(VERB, "do") ||
		!strcmp(VERB, "does") ||
		!strcmp(VERB, "don't") ||
		!strcmp(VERB, "doesn't") ||
		!strcmp(VERB, "did") ||
		!strcmp(VERB, "didn't")
	)
	{
		IS_NOT = strchr(VERB, '\'') != NULL;
		PAST = VERB[2] == 'd';
		INDEX++;

		if (!strcmp(TEXT_AT(READ, INDEX), "not"))
		{
			IS_NOT = !IS_NOT;
			INDEX++;
		}

		if (!strcmp(TEXT_AT(READ, INDEX), "have"))
		{
			AFTER = READ_PREDICATE(READ, INDEX, PLURAL_SUBJECT, WHAT);

			if (AFTER >= 0 && IS_NOT)
				WHAT->IS_NOT = !WHAT->IS_NOT;

			if (AFTER >= 0)
				WHAT->PAST = PAST;

			return (AFTER);
		}

		VERB = TEXT_AT(READ, INDEX);
	}

	/* "dogs bark", "Rex barks loudly", "some flowers fade quickly" */
	if (
		!IS_WORD_TOKEN(READ, INDEX) ||
		IN_LIST(VERB, NOT_A_NAME) ||
		IN_LIST(VERB, LOGIC_STOPS) ||
		!strcmp(VERB, "was") ||
		!strcmp(VERB, "were") ||
		!strcmp(VERB, "will") ||
		!strcmp(VERB, "would")
	)
		return (-1);

	AFTER = PREDICATE_WORDS(
		READ, INDEX, WORDS_KEY, sizeof WORDS_KEY, WHAT->SHOWN,
		sizeof WHAT->SHOWN, 1, 0
	);

	if (AFTER < 0)
		return (-1);

	WHAT->TYPE = PREDICATE_DO;
	snprintf(WHAT->KEY, sizeof WHAT->KEY, "do:%s", WORDS_KEY);
	WHAT->IS_NOT = IS_NOT;
	WHAT->PAST =
		PAST || (strlen(VERB) > 3 && !strcmp(VERB + strlen(VERB) - 2, "ed"));

	/* shown as the plain verb: "bark loudly" */
	{
		char	*SPACE = strchr(WHAT->SHOWN, ' ');
		char	FIRST[64];
		char	REST[LOGIC_TEXT];

		snprintf(REST, sizeof REST, "%s", SPACE ? SPACE : "");

		if (SPACE)
			*SPACE = 0;

		VERB_STEM(WHAT->SHOWN, FIRST, sizeof FIRST);
		snprintf(WHAT->SHOWN, sizeof WHAT->SHOWN, "%s%s", FIRST, REST);
	}

	(void)PLURAL_SUBJECT;

	return (AFTER);
}

/* the words from FROM to TO as written: "all cats are animals" */
static void
	SPAN_TEXT(const READING *READ, int FROM, int TO, char *OUTPUT, int SIZE)
{
	int	LENGTH = 0;
	int	INDEX;

	OUTPUT[0] = 0;

	for (INDEX = FROM; INDEX < TO && INDEX < READ->COUNT; INDEX++)
	{
		const TOKEN	*ITEM = &READ->TOKENS[INDEX];

		if (ITEM->KIND == TOKEN_SYMBOL && strchr(",.;!?", ITEM->TEXT[0]))
			continue ;

		LENGTH += snprintf(
			OUTPUT + LENGTH, SIZE - LENGTH, "%s%s", LENGTH ? " " : "",
			ITEM->ORIGINAL
		);

		if (LENGTH >= SIZE - 1)
			break ;
	}

	/* "All cats" said in the middle of a sentence: "all cats" */
	if (
		OUTPUT[0] &&
		isupper((uint8_t)OUTPUT[0]) &&
		OUTPUT[1] &&
		islower((uint8_t)OUTPUT[1]) &&
		FROM < READ->COUNT &&
		IN_LIST(READ->TOKENS[FROM].TEXT, NOT_A_NAME)
	)
		OUTPUT[0] = (char)tolower((uint8_t)OUTPUT[0]);
}

static int
	IS_PLURAL_WORD(const char *WORD)
{
	static const char	*PLURALS[14] = {
		"people", "men", "women", "children", "mice", "geese", "fish", "sheep",
		"deer", "feet", "teeth", "cattle", "police", NULL
	};
	size_t				LENGTH = strlen(WORD);

	if (IN_LIST(WORD, PLURALS))
		return (1);

	return (
		LENGTH > 3 &&
		WORD[LENGTH - 1] == 's' &&
		WORD[LENGTH - 2] != 's' &&
		WORD[LENGTH - 2] != 'u' &&
		WORD[LENGTH - 2] != 'i'
	);
}

/* "All A are B": a capital letter standing for a kind or a thing */
static int
	IS_LETTER_NAME(const READING *READ, int INDEX)
{
	const char	*NEXT;

	if (
		INDEX < 0 ||
		INDEX >= READ->COUNT ||
		READ->TOKENS[INDEX].KIND != TOKEN_WORD ||
		strlen(READ->TOKENS[INDEX].ORIGINAL) != 1 ||
		!isupper((uint8_t)READ->TOKENS[INDEX].ORIGINAL[0])
	)
		return (0);

	/* "A snake": the article ("A C": two letters) */
	NEXT = TEXT_AT(READ, INDEX + 1);

	if (
		IS_WORD_TOKEN(READ, INDEX + 1) &&
		strlen(READ->TOKENS[INDEX + 1].ORIGINAL) == 1 &&
		isupper((uint8_t)READ->TOKENS[INDEX + 1].ORIGINAL[0])
	)
		return (1);

	return (
		!(
			READ->TOKENS[INDEX].ORIGINAL[0] == 'A' &&
			IS_WORD_TOKEN(READ, INDEX + 1) &&
			strcmp(NEXT, "are") &&
			strcmp(NEXT, "is") &&
			strcmp(NEXT, "can") &&
			strcmp(NEXT, "have") &&
			strcmp(NEXT, "has") &&
			strcmp(NEXT, "and") &&
			strcmp(NEXT, "or")
		)
	);
}

/* "a" or "an" in front of a word */
static int
	IS_ARTICLE_AT(const READING *READ, int INDEX)
{
	return (
		(
			!strcmp(TEXT_AT(READ, INDEX), "a") ||
			!strcmp(TEXT_AT(READ, INDEX), "an")
		) &&
		!IS_LETTER_NAME(READ, INDEX) &&
		IS_WORD_TOKEN(READ, INDEX + 1)
	);
}

/* a clause starts here: the message start, after a mark, "and", "if" */
static int
	CLAUSE_START(const READING *READ, int INDEX)
{
	const char	*BEFORE;

	if (INDEX == 0)
		return (1);

	BEFORE = TEXT_AT(READ, INDEX - 1);

	return (
		READ->TOKENS[INDEX - 1].KIND == TOKEN_SYMBOL ||
		!strcmp(BEFORE, "and") ||
		!strcmp(BEFORE, "if") ||
		!strcmp(BEFORE, "that") ||
		!strcmp(BEFORE, "also") ||
		!strcmp(BEFORE, "but") ||
		!strcmp(BEFORE, "since") ||
		!strcmp(BEFORE, "because")
	);
}

/* the rules and facts the message states */
static void
	READ_LOGIC(const READING *READ, LOGIC_BOOK *BOOK, int QUESTION_AT)
{
	int	INDEX;

	memset(BOOK, 0, sizeof *BOOK);

	for (INDEX = 0; INDEX < READ->COUNT; INDEX++)
	{
		const char	*WORD = READ->TOKENS[INDEX].TEXT;
		int			QUANTIFIER = 0;
		int			KIND_AT;
		int			VERB_AT;
		int			AFTER;
		PREDICATE	WHAT;

		(void)QUESTION_AT;

		if (
			READ->TOKENS[INDEX].KIND != TOKEN_WORD ||
			!CLAUSE_START(READ, INDEX)
		)
			continue ;

		/* "all cats ...", "no fish ...", "some flowers ...", "every dog ..." */
		if (
			!strcmp(WORD, "all") ||
			!strcmp(WORD, "every") ||
			!strcmp(WORD, "each") ||
			!strcmp(WORD, "no") ||
			!strcmp(WORD, "some")
		)
		{
			QUANTIFIER = WORD[0];
			KIND_AT = INDEX + 1;

			/* "all the kids": the kids */
			if (!strcmp(TEXT_AT(READ, KIND_AT), "the") && QUANTIFIER == 'a')
				KIND_AT++;

			if (
				!strcmp(TEXT_AT(READ, KIND_AT), "the") ||
				!strcmp(TEXT_AT(READ, KIND_AT), "of")
			)
				continue ;
		}
		else
			KIND_AT = INDEX;

		/* "A snake is a reptile": the thing is "snake" ("All A are B": A is
		 * a name) */
		if (!QUANTIFIER && IS_ARTICLE_AT(READ, KIND_AT))
			KIND_AT++;

		if (
			!IS_WORD_TOKEN(READ, KIND_AT) ||
			(
				IN_LIST(TEXT_AT(READ, KIND_AT), NOT_A_NAME) &&
				!IS_LETTER_NAME(READ, KIND_AT)
			)
		)
			continue ;

		VERB_AT = KIND_AT + 1;

		/* "every student in the class passed": the kind is "student" */
		if (QUANTIFIER && IN_LIST(TEXT_AT(READ, VERB_AT), LOGIC_PLACES))
		{
			int	SKIP = VERB_AT + 1;

			if (IN_LIST(TEXT_AT(READ, SKIP), LOGIC_DETERMINERS))
				SKIP++;

			if (IS_WORD_TOKEN(READ, SKIP))
				VERB_AT = SKIP + 1;
		}

		/* "Mice are ...", "all cats ...": a kind; "Tom is ...": a thing */
		if (
			QUANTIFIER ||
			(
				(
					IS_PLURAL_WORD(TEXT_AT(READ, KIND_AT)) ||
					IS_LETTER_NAME(READ, KIND_AT)
				) &&
				(
					!strcmp(TEXT_AT(READ, VERB_AT), "are") ||
					!strcmp(TEXT_AT(READ, VERB_AT), "can") ||
					!strcmp(TEXT_AT(READ, VERB_AT), "cannot") ||
					!strcmp(TEXT_AT(READ, VERB_AT), "can't") ||
					!strcmp(TEXT_AT(READ, VERB_AT), "have") ||
					!strcmp(TEXT_AT(READ, VERB_AT), "don't")
				)
			)
		)
		{
			int			SINGLE = QUANTIFIER == 'e';
			LOGIC_RULE	*RULE;

			if (
				!IS_PLURAL_WORD(TEXT_AT(READ, KIND_AT)) &&
				!SINGLE &&
				!IS_LETTER_NAME(READ, KIND_AT)
			)
				continue ;

			AFTER = READ_PREDICATE(READ, VERB_AT, !SINGLE, &WHAT);

			if (AFTER < 0 || BOOK->RULE_COUNT >= MAX_LOGIC)
				continue ;

			if (
				SINGLE &&
				WHAT.TYPE == PREDICATE_DO &&
				TEXT_AT(
					READ, VERB_AT
				)[strlen(TEXT_AT(READ, VERB_AT)) - 1] != 's' &&
				!WHAT.PAST
			)
				continue ;

			RULE = &BOOK->RULES[BOOK->RULE_COUNT++];
			SINGULAR(TEXT_AT(READ, KIND_AT), RULE->FROM, sizeof RULE->FROM);
			snprintf(
				RULE->FROM_SHOWN, sizeof RULE->FROM_SHOWN, "%s",
				READ->TOKENS[KIND_AT].ORIGINAL
			);
			RULE->WHAT = WHAT;
			RULE->SOME = QUANTIFIER == 's';

			if (QUANTIFIER == 'n')
				RULE->WHAT.IS_NOT = !RULE->WHAT.IS_NOT;

			SPAN_TEXT(READ, INDEX, AFTER, RULE->TEXT, sizeof RULE->TEXT);
			INDEX = AFTER - 1;
			continue ;
		}

		/* "Tom is a cat", "Tweety can fly", "Rex has a tail", "Rex barks" */
		{
			LOGIC_FACT	*FACT;

			/* one thing takes "is", "has", "can", "does" or a verb with its
			 * "-s": "Tom is", "Rex barks" */
			{
				const char	*VERB = TEXT_AT(READ, VERB_AT);
				size_t		LENGTH = strlen(VERB);

				if (
					strcmp(VERB, "is") &&
					strcmp(VERB, "isn't") &&
					strcmp(VERB, "has") &&
					strcmp(VERB, "can") &&
					strcmp(VERB, "cannot") &&
					strcmp(VERB, "can't") &&
					strcmp(VERB, "does") &&
					strcmp(VERB, "doesn't") &&
					(LENGTH < 3 || VERB[LENGTH - 1] != 's')
				)
					continue ;
			}

			AFTER = READ_PREDICATE(READ, VERB_AT, 0, &WHAT);

			if (AFTER < 0 || BOOK->FACT_COUNT >= MAX_LOGIC)
				continue ;

			FACT = &BOOK->FACTS[BOOK->FACT_COUNT++];
			snprintf(
				FACT->NAME, sizeof FACT->NAME, "%s", TEXT_AT(READ, KIND_AT)
			);
			FACT->WHAT = WHAT;
			SPAN_TEXT(READ, INDEX, AFTER, FACT->TEXT, sizeof FACT->TEXT);
			INDEX = AFTER - 1;
		}
	}
}

/* what is asked: "is Tom an animal?", "can penguins fly?", "does Rex have
 * fur?", "are all roses flowers?" */
static int
	READ_LOGIC_QUESTION(
		const READING *READ, char *SUBJECT, int SUBJECT_SIZE, char *SHOWN,
		int SHOWN_SIZE, int *IS_KIND, int *HAS_ARTICLE, PREDICATE *WHAT
	)
{
	int	INDEX;

	for (INDEX = READ->COUNT - 1; INDEX >= 0; INDEX--)
	{
		const char	*WORD = READ->TOKENS[INDEX].TEXT;
		int			SUBJECT_AT;
		int			AFTER;
		int			ALL = 0;
		char		VERB[16];

		if (READ->TOKENS[INDEX].KIND != TOKEN_WORD)
			continue ;

		if (
			!(
				INDEX == 0 ||
				READ->TOKENS[INDEX - 1].KIND == TOKEN_SYMBOL ||
				!strcmp(TEXT_AT(READ, INDEX - 1), "so") ||
				!strcmp(TEXT_AT(READ, INDEX - 1), "then") ||
				!strcmp(TEXT_AT(READ, INDEX - 1), "therefore") ||
				!strcmp(TEXT_AT(READ, INDEX - 1), "but")
			)
		)
			continue ;

		if (
			strcmp(WORD, "is") &&
			strcmp(WORD, "are") &&
			strcmp(WORD, "can") &&
			strcmp(WORD, "does") &&
			strcmp(WORD, "do") &&
			strcmp(WORD, "did")
		)
			continue ;

		SUBJECT_AT = INDEX + 1;

		if (
			!strcmp(TEXT_AT(READ, SUBJECT_AT), "all") ||
			!strcmp(TEXT_AT(READ, SUBJECT_AT), "every")
		)
		{
			ALL = 1;
			SUBJECT_AT++;
		}

		/* "does a snake have fur?": a snake, any snake */
		*HAS_ARTICLE = 0;

		if (!ALL && IS_ARTICLE_AT(READ, SUBJECT_AT))
		{
			*HAS_ARTICLE = 1;
			SUBJECT_AT++;
		}

		if (
			!IS_WORD_TOKEN(READ, SUBJECT_AT) ||
			(
				IN_LIST(TEXT_AT(READ, SUBJECT_AT), NOT_A_NAME) &&
				!IS_LETTER_NAME(READ, SUBJECT_AT)
			)
		)
			continue ;

		snprintf(VERB, sizeof VERB, "%s", WORD);

		/* "is Tom an animal" is read as "Tom is an animal" */
		{
			READING	*SHIFTED = malloc(sizeof *SHIFTED);

			if (!SHIFTED)
				return (0);

			memcpy(SHIFTED, READ, sizeof *SHIFTED);

			/* the verb moved after the subject */
			{
				TOKEN	VERB_TOKEN = SHIFTED->TOKENS[INDEX];
				int		MOVE;

				for (MOVE = INDEX; MOVE < SUBJECT_AT; MOVE++)
					SHIFTED->TOKENS[MOVE] = SHIFTED->TOKENS[MOVE + 1];

				SHIFTED->TOKENS[SUBJECT_AT] = VERB_TOKEN;
			}

			AFTER = READ_PREDICATE(
				SHIFTED, SUBJECT_AT, IS_PLURAL_WORD(TEXT_AT(READ, SUBJECT_AT)),
				WHAT
			);
			free(SHIFTED);
		}

		if (AFTER < 0)
			continue ;

		/* the question ends there */
		if (
			AFTER < READ->COUNT &&
			strcmp(TEXT_AT(READ, AFTER), "?") &&
			strcmp(TEXT_AT(READ, AFTER), ".") &&
			strcmp(TEXT_AT(READ, AFTER), "or")
		)
			continue ;

		*IS_KIND = ALL ||
			(IS_PLURAL_WORD(TEXT_AT(READ, SUBJECT_AT)) &&
				(!strcmp(VERB, "are") || !strcmp(VERB, "do") ||
					!strcmp(VERB, "can")));

		if (*IS_KIND)
			SINGULAR(TEXT_AT(READ, SUBJECT_AT), SUBJECT, SUBJECT_SIZE);
		else
			snprintf(SUBJECT, SUBJECT_SIZE, "%s", TEXT_AT(READ, SUBJECT_AT));

		snprintf(
			SHOWN, SHOWN_SIZE, "%s%s%s", ALL ? "all " : "",
			*HAS_ARTICLE ? READ->TOKENS[SUBJECT_AT - 1].TEXT : "",
			*HAS_ARTICLE ? " " : ""
		);
		snprintf(
			SHOWN + strlen(SHOWN), SHOWN_SIZE - strlen(SHOWN), "%s",
			READ->TOKENS[SUBJECT_AT].ORIGINAL
		);
		return (INDEX);
	}

	return (-1);
}

/* "is an animal", "can fly", "has wings", "barks"; for a kind: "are
 * animals", "can fly", "have wings", "bark" */
static void
	SAY_PREDICATE(
		const PREDICATE *WHAT, int PLURAL, int NEGATE, char *OUTPUT, int SIZE
	)
{
	int		IS_NOT = WHAT->IS_NOT ^ NEGATE;
	char	SHOWN[LOGIC_TEXT];

	snprintf(SHOWN, sizeof SHOWN, "%s", WHAT->SHOWN);

	if (WHAT->TYPE == PREDICATE_IS)
	{
		char	THING[LOGIC_TEXT];

		/* "animals" -> "an animal" for one thing, "animal" -> "animals" */
		if (WHAT->IS_NOUN && !PLURAL)
		{
			char	*LAST = strrchr(SHOWN, ' ');
			char	SINGLE[64];

			if (LAST)
				LAST = LAST + 1;
			else
				LAST = SHOWN;

			SINGULAR(LAST, SINGLE, sizeof SINGLE);
			snprintf(LAST, sizeof SHOWN - (size_t)(LAST - SHOWN), "%s", SINGLE);
			snprintf(THING, sizeof THING, "%s %s", ARTICLE(SHOWN), SHOWN);
		}
		else if (WHAT->IS_NOUN && PLURAL && !IS_PLURAL_WORD(SHOWN))
			snprintf(THING, sizeof THING, "%ss", SHOWN);
		else
			snprintf(THING, sizeof THING, "%s", SHOWN);

		snprintf(
			OUTPUT, SIZE, "%s%s %s", PLURAL ? "are" : "is",
			IS_NOT ? " not" : "", THING
		);
		return ;
	}

	if (WHAT->TYPE == PREDICATE_CAN)
	{
		snprintf(OUTPUT, SIZE, "%s %s", IS_NOT ? "cannot" : "can", SHOWN);
		return ;
	}

	if (WHAT->TYPE == PREDICATE_HAVE)
	{
		if (IS_NOT)
			snprintf(
				OUTPUT, SIZE, "%s not have %s", PLURAL ? "do" : "does", SHOWN
			);
		else
			snprintf(OUTPUT, SIZE, "%s %s", PLURAL ? "have" : "has", SHOWN);

		return ;
	}

	if (IS_NOT)
		snprintf(
			OUTPUT, SIZE, "%s not %s",
			WHAT->PAST ? "did"
				: PLURAL ? "do"
			: "does",
			SHOWN
		);
	else if (WHAT->PAST)
	{
		char	*SPACE = strchr(SHOWN, ' ');
		char	FIRST[64];
		char	PAST_FORM[80];

		snprintf(
			FIRST, sizeof FIRST, "%.*s",
			SPACE ? (int)(SPACE - SHOWN) : (int)strlen(SHOWN), SHOWN
		);
		VERB_PAST(FIRST, PAST_FORM, sizeof PAST_FORM);
		snprintf(OUTPUT, SIZE, "%s%s", PAST_FORM, SPACE ? SPACE : "");
	}
	else if (PLURAL)
		snprintf(OUTPUT, SIZE, "%s", SHOWN);
	else
	{
		char	*SPACE = strchr(SHOWN, ' ');
		char	FIRST[64];
		char	THIRD[80];

		snprintf(
			FIRST, sizeof FIRST, "%.*s",
			SPACE ? (int)(SPACE - SHOWN) : (int)strlen(SHOWN), SHOWN
		);
		VERB_THIRD(FIRST, THIRD, sizeof THIRD);
		snprintf(OUTPUT, SIZE, "%s%s", THIRD, SPACE ? SPACE : "");
	}
}

/* the base of a predicate after "can still": "be animals", "fly", "have
 * wings", "bark" */
static void
	SAY_BASE(const PREDICATE *WHAT, char *OUTPUT, int SIZE)
{
	if (WHAT->TYPE == PREDICATE_IS)
	{
		if (WHAT->IS_NOUN && !IS_PLURAL_WORD(WHAT->SHOWN))
			snprintf(OUTPUT, SIZE, "be %ss", WHAT->SHOWN);
		else
			snprintf(OUTPUT, SIZE, "be %s", WHAT->SHOWN);
	}
	else if (WHAT->TYPE == PREDICATE_HAVE)
		snprintf(OUTPUT, SIZE, "have %s", WHAT->SHOWN);
	else
		snprintf(OUTPUT, SIZE, "%s", WHAT->SHOWN);
}

typedef struct
{
	char	KEYS[MAX_LOGIC * 3][LOGIC_TEXT];
	int		IS_NOT[MAX_LOGIC * 3];
	int		PARENT[MAX_LOGIC * 3];
	int		RULE[MAX_LOGIC * 3];
	int		FACT[MAX_LOGIC * 3];
	int		COUNT;
} LOGIC_KNOWN;

/* "Tom is a cat, and all cats are animals": how KNOWN->KEYS[AT] follows */
static void
	SAY_CHAIN(
		const LOGIC_BOOK *BOOK, const LOGIC_KNOWN *KNOWN, int AT, char *OUTPUT,
		int SIZE
	)
{
	int	PATH[MAX_LOGIC * 3];
	int	PATH_LENGTH = 0;
	int	LENGTH = 0;

	OUTPUT[0] = 0;

	for (; AT >= 0 && PATH_LENGTH < MAX_LOGIC * 3; AT = KNOWN->PARENT[AT])
		PATH[PATH_LENGTH++] = AT;

	while (PATH_LENGTH--)
	{
		const char	*STEP_TEXT;
		char		STEP[220];

		AT = PATH[PATH_LENGTH];

		if (KNOWN->FACT[AT] >= 0)
			STEP_TEXT = BOOK->FACTS[KNOWN->FACT[AT]].TEXT;
		else if (KNOWN->RULE[AT] >= 0)
			STEP_TEXT = BOOK->RULES[KNOWN->RULE[AT]].TEXT;
		else
			continue ;

		snprintf(STEP, sizeof STEP, "%s", STEP_TEXT);

		if (
			LENGTH &&
			KNOWN->RULE[AT] >= 0 &&
			isupper((uint8_t)STEP[0]) &&
			islower((uint8_t)STEP[1])
		)
			STEP[0] = (char)tolower((uint8_t)STEP[0]);

		LENGTH += snprintf(
			OUTPUT + LENGTH, SIZE - LENGTH, "%s%s", LENGTH ? ", and " : "", STEP
		);

		if (LENGTH >= SIZE - 1)
			break ;
	}

	if (OUTPUT[0])
		OUTPUT[0] = (char)toupper((uint8_t)OUTPUT[0]);
}

static int
	ADD_KNOWN(
		LOGIC_KNOWN *KNOWN, const char *KEY, int IS_NOT, int PARENT, int RULE,
		int FACT
	)
{
	int	INDEX;

	if (KNOWN->COUNT >= MAX_LOGIC * 3)
		return (-1);

	for (INDEX = 0; INDEX < KNOWN->COUNT; INDEX++)
		if (!strcmp(KNOWN->KEYS[INDEX], KEY) && KNOWN->IS_NOT[INDEX] == IS_NOT)
			return (-1);

	snprintf(KNOWN->KEYS[KNOWN->COUNT], LOGIC_TEXT, "%s", KEY);
	KNOWN->IS_NOT[KNOWN->COUNT] = IS_NOT;
	KNOWN->PARENT[KNOWN->COUNT] = PARENT;
	KNOWN->RULE[KNOWN->COUNT] = RULE;
	KNOWN->FACT[KNOWN->COUNT] = FACT;

	return (KNOWN->COUNT++);
}

/* "Tom is a cat, and all cats are animals, so Tom is an animal" */
/* "can a dog be a fish?" asks "is a dog a fish?"; "are any dogs fish?"
 * asks "are all dogs fish?" (what holds for all holds for any); 1 when the
 * question was said another way */
static int
	PLAIN_LOGIC_QUESTION(const char *MESSAGE, char *OUTPUT, int OUTPUT_SIZE)
{
	const char	*START = MESSAGE;
	const char	*CURSOR;
	char		LOWER[1600];
	size_t		OFFSET;
	int			INDEX;

	if (strlen(MESSAGE) >= sizeof LOWER - 1)
		return (0);

	/* the question is the last sentence or clause */
	for (CURSOR = MESSAGE; *CURSOR; CURSOR++)
		if (
			(*CURSOR == '.' || *CURSOR == ',' || *CURSOR == ';') &&
			CURSOR[1] == ' ' &&
			strchr(CURSOR + 1, '?')
		)
			START = CURSOR + 2;

	OFFSET = (size_t)(START - MESSAGE);

	for (INDEX = 0; MESSAGE[INDEX]; INDEX++)
		LOWER[INDEX] = (char)tolower((uint8_t)MESSAGE[INDEX]);

	LOWER[INDEX] = 0;

	if (
		(
			!strncmp(LOWER + OFFSET, "can ", 4) ||
			!strncmp(LOWER + OFFSET, "could ", 6)
		) &&
		strstr(LOWER + OFFSET, " be ")
	)
	{
		const char	*BE = strstr(LOWER + OFFSET, " be ");
		size_t		VERB;

		if (!strncmp(LOWER + OFFSET, "can ", 4))
			VERB = 4;
		else
			VERB = 6;

		snprintf(
			OUTPUT, OUTPUT_SIZE, "%.*sis %.*s%s", (int)OFFSET, MESSAGE,
			(int)(BE - (LOWER + OFFSET) - VERB), MESSAGE + OFFSET + VERB,
			MESSAGE + (BE - LOWER) + 3
		);
		return (1);
	}

	if (!strncmp(LOWER + OFFSET, "are any ", 8))
	{
		snprintf(
			OUTPUT, OUTPUT_SIZE, "%.*sare all %s", (int)OFFSET, MESSAGE,
			MESSAGE + OFFSET + 8
		);
		return (1);
	}

	return (0);
}

static int
	ANSWER_SYLLOGISM(
		const READING *READ, char *ACTION, int ACTION_SIZE, char *REPLY,
		int REPLY_SIZE
	)
{
	static LOGIC_BOOK	BOOK;
	static LOGIC_KNOWN	KNOWN;
	char				SUBJECT[48];
	char				SUBJECT_SHOWN[64];
	char				SUBJECT_WORD[64];
	int					IS_KIND = 0;
	int					HAS_ARTICLE = 0;
	PREDICATE			ASKED;
	int					QUESTION_AT;
	int					FOUND = -1;
	int					INDEX;
	int					SUBJECT_FACTS = 0;
	char				CHAIN[900];
	char				SAID[LOGIC_TEXT * 2];
	char				WHY[1200] = "";
	int					RULE;

	if (
		!HAS(READ, " all ") &&
		!HAS(READ, " every ") &&
		!HAS(READ, " no ") &&
		!HAS(READ, " some ") &&
		!HAS(READ, " each ") &&
		!HAS(READ, " are ")
	)
		return (0);

	{
		static READING	PLAIN_READ;
		char			PLAIN_TEXT[1600];

		if (PLAIN_LOGIC_QUESTION(READ->MESSAGE, PLAIN_TEXT, sizeof PLAIN_TEXT))
		{
			READ_MESSAGE(PLAIN_TEXT, &PLAIN_READ);
			READ = &PLAIN_READ;
		}
	}

	QUESTION_AT = READ_LOGIC_QUESTION(
		READ, SUBJECT, sizeof SUBJECT, SUBJECT_SHOWN, sizeof SUBJECT_SHOWN,
		&IS_KIND, &HAS_ARTICLE, &ASKED
	);

	if (QUESTION_AT < 0)
		return (0);

	READ_LOGIC(READ, &BOOK, QUESTION_AT);

	if (!BOOK.RULE_COUNT)
		return (0);

	snprintf(
		SUBJECT_WORD, sizeof SUBJECT_WORD, "%s",
		strncmp(SUBJECT_SHOWN, "all ", 4) ? SUBJECT_SHOWN : SUBJECT_SHOWN + 4
	);

	/* where the reasoning starts: what the thing is, or the kind itself */
	memset(&KNOWN, 0, sizeof KNOWN);

	if (IS_KIND)
	{
		char	KEY[LOGIC_TEXT];

		snprintf(KEY, sizeof KEY, "is:%s", SUBJECT);
		ADD_KNOWN(&KNOWN, KEY, 0, -1, -1, -1);
	}
	else
	{
		for (INDEX = 0; INDEX < BOOK.FACT_COUNT; INDEX++)
			if (!strcmp(BOOK.FACTS[INDEX].NAME, SUBJECT))
			{
				ADD_KNOWN(
					&KNOWN, BOOK.FACTS[INDEX].WHAT.KEY,
					BOOK.FACTS[INDEX].WHAT.IS_NOT, -1, -1, INDEX
				);
				SUBJECT_FACTS++;
			}

		/* "does a square have four sides?": a square is a square */
		if (HAS_ARTICLE)
		{
			char	KEY[LOGIC_TEXT];
			char	SINGLE[48];

			SINGULAR(SUBJECT, SINGLE, sizeof SINGLE);
			snprintf(KEY, sizeof KEY, "is:%s", SINGLE);

			for (RULE = 0; RULE < BOOK.RULE_COUNT; RULE++)
				if (!strcmp(BOOK.RULES[RULE].FROM, SINGLE))
				{
					ADD_KNOWN(&KNOWN, KEY, 0, -1, -1, -1);
					SUBJECT_FACTS++;
					break ;
				}
		}
	}

	/* every kind the thing is brings what that kind is, can and has */
	for (INDEX = 0; INDEX < KNOWN.COUNT; INDEX++)
	{
		if (KNOWN.IS_NOT[INDEX] || strncmp(KNOWN.KEYS[INDEX], "is:", 3))
			continue ;

		for (RULE = 0; RULE < BOOK.RULE_COUNT; RULE++)
			if (
				!BOOK.RULES[RULE].SOME &&
				!strcmp(BOOK.RULES[RULE].FROM, KNOWN.KEYS[INDEX] + 3)
			)
				ADD_KNOWN(
					&KNOWN, BOOK.RULES[RULE].WHAT.KEY,
					BOOK.RULES[RULE].WHAT.IS_NOT, INDEX, RULE, -1
				);
	}

	for (INDEX = 0; INDEX < KNOWN.COUNT && FOUND < 0; INDEX++)
		if (!strcmp(KNOWN.KEYS[INDEX], ASKED.KEY))
			FOUND = INDEX;

	if (FOUND >= 0)
	{
		int			YES = KNOWN.IS_NOT[FOUND] == ASKED.IS_NOT;
		PREDICATE	SAID_AS = ASKED;

		SAY_CHAIN(&BOOK, &KNOWN, FOUND, CHAIN, sizeof CHAIN);
		SAID_AS.IS_NOT = KNOWN.IS_NOT[FOUND];
		SAY_PREDICATE(&SAID_AS, IS_KIND, 0, SAID, sizeof SAID);

		if (CHAIN[0])
			snprintf(
				REPLY, REPLY_SIZE, "%s. %s, so %s %s.", YES ? "Yes" : "No",
				CHAIN, SUBJECT_SHOWN, SAID
			);
		else
			snprintf(
				REPLY, REPLY_SIZE, "%s. You said %s %s.", YES ? "Yes" : "No",
				SUBJECT_SHOWN, SAID
			);

		/* "so all dogs are not fish" -> "so no dogs are fish" */
		{
			char	*ALL = strstr(REPLY, "so all ");
			char	*NOT;

			if (ALL)
				NOT = strstr(ALL, " are not ");
			else
				NOT = NULL;

			if (NOT)
			{
				char	FIXED[1600];

				snprintf(
					FIXED, sizeof FIXED, "%.*sso no %.*s are %s",
					(int)(ALL - REPLY), REPLY, (int)(NOT - (ALL + 7)), ALL + 7,
					NOT + 9
				);
				snprintf(REPLY, REPLY_SIZE, "%s", FIXED);
			}
		}

		snprintf(
			ACTION, ACTION_SIZE, "logic %s %s -> %s", SUBJECT, ASKED.SHOWN,
			YES ? "yes" : "no"
		);
		return (1);
	}

	/* "all dogs are loyal, Rex is not loyal, is Rex a dog?": no, a dog would
	 * be loyal */
	for (RULE = 0; RULE < BOOK.RULE_COUNT && ASKED.TYPE == PREDICATE_IS; RULE++)
	{
		if (
			BOOK.RULES[RULE].SOME ||
			strcmp(BOOK.RULES[RULE].FROM, ASKED.KEY + 3)
		)
			continue ;

		for (INDEX = 0; INDEX < KNOWN.COUNT; INDEX++)
			if (
				!strcmp(KNOWN.KEYS[INDEX], BOOK.RULES[RULE].WHAT.KEY) &&
				KNOWN.IS_NOT[INDEX] != BOOK.RULES[RULE].WHAT.IS_NOT
			)
			{
				PREDICATE	SAID_AS = ASKED;
				int			YES = ASKED.IS_NOT;

				SAY_CHAIN(&BOOK, &KNOWN, INDEX, CHAIN, sizeof CHAIN);
				SAID_AS.IS_NOT = 1;
				SAY_PREDICATE(&SAID_AS, IS_KIND, 0, SAID, sizeof SAID);
				snprintf(
					REPLY, REPLY_SIZE, "%s. %c%s, and %s, so %s %s.",
					YES ? "Yes" : "No",
					toupper((uint8_t)BOOK.RULES[RULE].TEXT[0]),
					BOOK.RULES[RULE].TEXT + 1, CHAIN[0] ? CHAIN : SUBJECT_SHOWN,
					SUBJECT_SHOWN, SAID
				);

				/* "and Rex is not loyal": the chain reads on after "and" */
				{
					char	*AND = strstr(REPLY, ", and ");

					if (
						AND &&
						isupper((uint8_t)AND[6]) &&
						islower((uint8_t)AND[7]) &&
						KNOWN.FACT[INDEX] < 0
					)
						AND[6] = (char)tolower((uint8_t)AND[6]);
				}

				snprintf(
					ACTION, ACTION_SIZE, "logic %s %s -> %s", SUBJECT,
					ASKED.SHOWN, YES ? "yes" : "no"
				);
				return (1);
			}
	}

	/* "all birds can fly, Tom can fly, is Tom a bird?": not everything that
	 * flies is a bird */
	for (RULE = 0; RULE < BOOK.RULE_COUNT && !WHY[0]; RULE++)
	{
		if (
			BOOK.RULES[RULE].SOME ||
			BOOK.RULES[RULE].WHAT.IS_NOT ||
			ASKED.TYPE != PREDICATE_IS ||
			strcmp(BOOK.RULES[RULE].FROM, ASKED.KEY + 3)
		)
			continue ;

		for (INDEX = 0; INDEX < KNOWN.COUNT; INDEX++)
			if (
				!KNOWN.IS_NOT[INDEX] &&
				!strcmp(KNOWN.KEYS[INDEX], BOOK.RULES[RULE].WHAT.KEY)
			)
			{
				char	OWN[LOGIC_TEXT * 2];

				SAY_PREDICATE(&BOOK.RULES[RULE].WHAT, 0, 0, OWN, sizeof OWN);
				snprintf(
					WHY, sizeof WHY,
					"I can't tell. %c%s, but not everything that %s is %s %s, "
					"so "
					"%s might not be %s.",
					toupper((uint8_t)BOOK.RULES[RULE].TEXT[0]),
					BOOK.RULES[RULE].TEXT + 1, OWN,
					ARTICLE(BOOK.RULES[RULE].FROM), BOOK.RULES[RULE].FROM,
					SUBJECT_WORD, IS_KIND ? BOOK.RULES[RULE].FROM_SHOWN : "one"
				);
				break ;
			}
	}

	/* "some flowers fade quickly": maybe roses do, maybe they don't */
	for (RULE = 0; RULE < BOOK.RULE_COUNT && !WHY[0]; RULE++)
	{
		if (
			!BOOK.RULES[RULE].SOME ||
			strcmp(BOOK.RULES[RULE].WHAT.KEY, ASKED.KEY)
		)
			continue ;

		for (INDEX = 0; INDEX < KNOWN.COUNT; INDEX++)
			if (
				!KNOWN.IS_NOT[INDEX] &&
				!strncmp(KNOWN.KEYS[INDEX], "is:", 3) &&
				!strcmp(KNOWN.KEYS[INDEX] + 3, BOOK.RULES[RULE].FROM)
			)
			{
				SAY_CHAIN(&BOOK, &KNOWN, INDEX, CHAIN, sizeof CHAIN);
				snprintf(
					WHY, sizeof WHY,
					"I can't tell. %s%sonly %s, so %s might or might not.",
					CHAIN, CHAIN[0] ? ", but " : "", BOOK.RULES[RULE].TEXT,
					SUBJECT_WORD
				);

				if (!CHAIN[0])
					WHY[15] = (char)toupper((uint8_t)WHY[15]);

				break ;
			}
	}

	/* "Tom is not a cat": things that aren't cats can still be animals */
	for (INDEX = 0; INDEX < KNOWN.COUNT && !WHY[0]; INDEX++)
	{
		if (!KNOWN.IS_NOT[INDEX] || strncmp(KNOWN.KEYS[INDEX], "is:", 3))
			continue ;

		for (RULE = 0; RULE < BOOK.RULE_COUNT; RULE++)
			if (
				!BOOK.RULES[RULE].SOME &&
				!BOOK.RULES[RULE].WHAT.IS_NOT &&
				!strcmp(BOOK.RULES[RULE].FROM, KNOWN.KEYS[INDEX] + 3) &&
				!strcmp(BOOK.RULES[RULE].WHAT.KEY, ASKED.KEY)
			)
			{
				char	BASE[LOGIC_TEXT * 2];

				SAY_BASE(&ASKED, BASE, sizeof BASE);
				snprintf(
					WHY, sizeof WHY,
					"I can't tell. %s, but things that aren't %s can still %s.",
					KNOWN.FACT[INDEX] >= 0 ? BOOK.FACTS[KNOWN.FACT[INDEX]].TEXT
					: SUBJECT_SHOWN,
					BOOK.RULES[RULE].FROM_SHOWN, BASE
				);
				break ;
			}
	}

	if (!WHY[0] && !IS_KIND && !SUBJECT_FACTS)
		snprintf(
			WHY, sizeof WHY,
			"I can't tell from what you said: you didn't say what %s is.",
			SUBJECT_SHOWN
		);

	if (!WHY[0])
	{
		SAY_PREDICATE(&ASKED, IS_KIND, 0, SAID, sizeof SAID);
		snprintf(
			WHY, sizeof WHY,
			"I can't tell from what you said: nothing in it says whether %s "
			"%s.",
			SUBJECT_SHOWN, SAID
		);
	}

	snprintf(REPLY, REPLY_SIZE, "%s", WHY);
	snprintf(
		ACTION, ACTION_SIZE, "logic %s %s -> unknown", SUBJECT, ASKED.SHOWN
	);

	return (1);
}

/* ---------- "Alice is taller than Bob ... who is the tallest?" ---------- */

typedef struct
{
	const char	*MORE;
	const char	*MOST;
	const char	*LESS;
	const char	*LEAST;
} SCALE;

static const SCALE	SCALES[21] = {
	{ "taller", "tallest", "shorter", "shortest" },
	{ "older", "oldest", "younger", "youngest" },
	{ "bigger", "biggest", "smaller", "smallest" },
	{ "larger", "largest", "smaller", "smallest" },
	{ "faster", "fastest", "slower", "slowest" },
	{ "heavier", "heaviest", "lighter", "lightest" },
	{ "stronger", "strongest", "weaker", "weakest" },
	{ "higher", "highest", "lower", "lowest" },
	{ "longer", "longest", "shorter", "shortest" },
	{ "richer", "richest", "poorer", "poorest" },
	{ "smarter", "smartest", "dumber", "dumbest" },
	{ "hotter", "hottest", "colder", "coldest" },
	{ "warmer", "warmest", "cooler", "coolest" },
	{ "better", "best", "worse", "worst" },
	{ "happier", "happiest", "sadder", "saddest" },
	{ "louder", "loudest", "quieter", "quietest" },
	{ "wider", "widest", "narrower", "narrowest" },
	{ "deeper", "deepest", "shallower", "shallowest" },
	{ "earlier", "earliest", "later", "latest" },
	{ "cheaper", "cheapest", "dearer", "dearest" }, { NULL, NULL, NULL, NULL }
};

/* which scale and which way: "taller" -> tall scale, up */
static int
	FIND_SCALE(const char *WORD, int *SCALE_INDEX, int *UP, int *IS_MOST)
{
	int	INDEX;

	for (INDEX = 0; SCALES[INDEX].MORE; INDEX++)
	{
		if (
			!strcmp(WORD, SCALES[INDEX].MORE) ||
			!strcmp(WORD, SCALES[INDEX].MOST)
		)
		{
			*SCALE_INDEX = INDEX;
			*UP = 1;
			*IS_MOST = !strcmp(WORD, SCALES[INDEX].MOST);
			return (1);
		}

		if (
			!strcmp(WORD, SCALES[INDEX].LESS) ||
			!strcmp(WORD, SCALES[INDEX].LEAST)
		)
		{
			*SCALE_INDEX = INDEX;
			*UP = 0;
			*IS_MOST = !strcmp(WORD, SCALES[INDEX].LEAST);
			return (1);
		}
	}

	return (0);
}

static int
	SAME_SCALE(int FIRST, int SECOND)
{
	/* "shorter" is on the tall scale and on the long scale */
	return (
		FIRST == SECOND ||
		!strcmp(SCALES[FIRST].MORE, SCALES[SECOND].MORE) ||
		(
			!strcmp(SCALES[FIRST].LESS, SCALES[SECOND].LESS) &&
			!strcmp(SCALES[FIRST].LESS, "smaller")
		)
	);
}

static int
	ANSWER_ORDER(
		const READING *READ, char *ACTION, int ACTION_SIZE, char *REPLY,
		int REPLY_SIZE
	)
{
	char	NAMES[12][48];
	char	SHOWN[12][48];
	int		NAME_COUNT = 0;
	int		ABOVE[12][12];
	int		SCALE_INDEX = -1;
	int		FACT_COUNT = 0;
	int		INDEX;
	char	FACTS[600] = "";
	int		FACTS_POSITION = 0;

	memset(ABOVE, 0, sizeof ABOVE);

	if (!HAS(READ, " than "))
		return (0);

	/* "A is taller than B", "A is more X than B" is left out */
	for (INDEX = 1; INDEX + 3 < READ->COUNT; INDEX++)
	{
		int	THIS_SCALE;
		int	UP;
		int	IS_MOST;
		int	NAME_INDEX[2];
		int	SIDE;

		if (
			strcmp(READ->TOKENS[INDEX].TEXT, "is") &&
			strcmp(READ->TOKENS[INDEX].TEXT, "are") &&
			strcmp(READ->TOKENS[INDEX].TEXT, "was")
		)
			continue ;

		if (
			!FIND_SCALE(
				READ->TOKENS[INDEX + 1].TEXT, &THIS_SCALE, &UP, &IS_MOST
			) ||
			IS_MOST ||
			strcmp(READ->TOKENS[INDEX + 2].TEXT, "than") ||
			READ->TOKENS[INDEX - 1].KIND != TOKEN_WORD ||
			READ->TOKENS[INDEX + 3].KIND != TOKEN_WORD
		)
			continue ;

		/* a question ("is A taller than B?") is no fact */
		if (
			INDEX >= 2 &&
			(
				!strcmp(READ->TOKENS[INDEX - 2].TEXT, "is") ||
				!strcmp(READ->TOKENS[INDEX - 2].TEXT, "who")
			)
		)
			continue ;

		if (!strcmp(READ->TOKENS[INDEX - 1].TEXT, "who"))
			continue ;

		if (SCALE_INDEX >= 0 && !SAME_SCALE(SCALE_INDEX, THIS_SCALE))
			return (0);

		if (SCALE_INDEX < 0)
			SCALE_INDEX = THIS_SCALE;

		for (SIDE = 0; SIDE < 2; SIDE++)
		{
			const TOKEN	*NAME = &READ->TOKENS[SIDE ? INDEX + 3 : INDEX - 1];
			int			FOUND;

			for (FOUND = 0; FOUND < NAME_COUNT; FOUND++)
				if (!strcmp(NAMES[FOUND], NAME->TEXT))
					break ;

			if (FOUND == NAME_COUNT)
			{
				if (NAME_COUNT == 12)
					return (0);

				snprintf(NAMES[NAME_COUNT], 48, "%s", NAME->TEXT);
				snprintf(SHOWN[NAME_COUNT], 48, "%s", NAME->ORIGINAL);
				NAME_COUNT++;
			}

			NAME_INDEX[SIDE] = FOUND;
		}

		/* ABOVE[a][b]: a is higher on the "more" end than b */
		if (UP)
			ABOVE[NAME_INDEX[0]][NAME_INDEX[1]] = 1;
		else
			ABOVE[NAME_INDEX[1]][NAME_INDEX[0]] = 1;

		if (FACTS_POSITION < 500)
			FACTS_POSITION += snprintf(
				FACTS + FACTS_POSITION, sizeof FACTS - FACTS_POSITION,
				"%s%s is %s than %s", FACT_COUNT ? ", and " : "",
				SHOWN[NAME_INDEX[0]], READ->TOKENS[INDEX + 1].TEXT,
				SHOWN[NAME_INDEX[1]]
			);

		FACT_COUNT++;
	}

	if (FACT_COUNT < 1 || NAME_COUNT < 2)
		return (0);

	/* everything that follows: a above b above c means a above c */
	{
		int	MIDDLE;
		int	FIRST;
		int	SECOND;

		for (MIDDLE = 0; MIDDLE < NAME_COUNT; MIDDLE++)
			for (FIRST = 0; FIRST < NAME_COUNT; FIRST++)
				for (SECOND = 0; SECOND < NAME_COUNT; SECOND++)
					if (ABOVE[FIRST][MIDDLE] && ABOVE[MIDDLE][SECOND])
						ABOVE[FIRST][SECOND] = 1;

		for (FIRST = 0; FIRST < NAME_COUNT; FIRST++)
			if (ABOVE[FIRST][FIRST])
			{
				snprintf(
					REPLY, REPLY_SIZE,
					"That can't all be true: it goes round in a circle."
				);
				snprintf(ACTION, ACTION_SIZE, "order -> contradiction");
				return (1);
			}
	}

	/* the question: "who is the tallest?", "who is shorter, A or C?", "is A
	 * taller than C?", in the last sentence */
	{
		int	QUESTION_START = 0;
		int	PASS;

		for (INDEX = 0; INDEX < READ->COUNT; INDEX++)
			if (
				READ->TOKENS[INDEX].KIND == TOKEN_SYMBOL &&
				strchr(".!", READ->TOKENS[INDEX].TEXT[0]) &&
				INDEX + 1 < READ->COUNT
			)
				QUESTION_START = INDEX + 1;

		/* superlatives first, then comparatives */
		for (PASS = 0; PASS < 2; PASS++)
			for (INDEX = QUESTION_START; INDEX < READ->COUNT; INDEX++)
			{
				int	THIS_SCALE;
				int	UP;
				int	IS_MOST;
				int	ASKED[2] = { -1, -1 };
				int	ASKED_COUNT = 0;
				int	SCAN;

				if (
					!FIND_SCALE(
						READ->TOKENS[INDEX].TEXT, &THIS_SCALE, &UP, &IS_MOST
					)
				)
					continue ;

				if (
					!SAME_SCALE(THIS_SCALE, SCALE_INDEX) ||
					IS_MOST != (PASS == 0)
				)
					continue ;

				if (IS_MOST)
				{
					int	WINNER = -1;
					int	CANDIDATE;

					for (CANDIDATE = 0; CANDIDATE < NAME_COUNT; CANDIDATE++)
					{
						int	OTHER;
						int	BEATS_ALL = 1;

						for (OTHER = 0; OTHER < NAME_COUNT; OTHER++)
							if (
								OTHER != CANDIDATE &&
								!(
									UP ? ABOVE[CANDIDATE][OTHER]
									: ABOVE[OTHER][CANDIDATE]
								)
							)
								BEATS_ALL = 0;

						if (BEATS_ALL)
							WINNER = CANDIDATE;
					}

					if (WINNER < 0)
					{
						snprintf(
							REPLY, REPLY_SIZE,
							"I can't tell from what you said: %s. That doesn't "
							"say who is the %s.",
							FACTS, READ->TOKENS[INDEX].TEXT
						);
						snprintf(ACTION, ACTION_SIZE, "order -> can't tell");
						return (1);
					}

					snprintf(
						REPLY, REPLY_SIZE, "%s is the %s: %s.", SHOWN[WINNER],
						READ->TOKENS[INDEX].TEXT, FACTS
					);
					snprintf(
						ACTION, ACTION_SIZE, "order %s -> %s",
						READ->TOKENS[INDEX].TEXT, NAMES[WINNER]
					);
					return (1);
				}

				/* "is A taller than C?", "who is taller, A or C?" */
				for (
					SCAN = QUESTION_START;
					SCAN < READ->COUNT && ASKED_COUNT < 2;
					SCAN++
				)
				{
					int	NAME;

					for (NAME = 0; NAME < NAME_COUNT; NAME++)
						if (
							!strcmp(READ->TOKENS[SCAN].TEXT, NAMES[NAME]) &&
							(ASKED_COUNT == 0 || ASKED[0] != NAME)
						)
							ASKED[ASKED_COUNT++] = NAME;
				}

				/* "Mary is taller than Sue. Who is shorter?": of the two */
				if (
					ASKED_COUNT < 2 &&
					NAME_COUNT == 2 &&
					(HAS(READ, " who ") || HAS(READ, " which "))
				)
				{
					int	WINNER;

					if (UP)
					{
						if (ABOVE[0][1])
							WINNER = 0;
						else
							WINNER = 1;
					}
					else
					{
						if (ABOVE[0][1])
							WINNER = 1;
						else
							WINNER = 0;
					}

					snprintf(
						REPLY, REPLY_SIZE, "%s is %s: %s.", SHOWN[WINNER],
						READ->TOKENS[INDEX].TEXT, FACTS
					);
					snprintf(
						ACTION, ACTION_SIZE, "order %s -> %s",
						READ->TOKENS[INDEX].TEXT, NAMES[WINNER]
					);
					return (1);
				}

				if (ASKED_COUNT < 2)
					continue ;

				{
					int	HIGHER;

					if (UP)
						HIGHER = ASKED[0];
					else
						HIGHER = ASKED[1];

					int	LOWER;

					if (UP)
						LOWER = ASKED[1];
					else
						LOWER = ASKED[0];

					if (HAS(READ, " who ") || HAS(READ, " which "))
					{
						int	WINNER;

						if (ABOVE[HIGHER][LOWER])
							WINNER = ASKED[0];
						else if (ABOVE[LOWER][HIGHER])
							WINNER = ASKED[1];
						else
						{
							snprintf(
								REPLY, REPLY_SIZE,
								"I can't tell from what you said: %s.", FACTS
							);
							snprintf(
								ACTION, ACTION_SIZE, "order -> can't tell"
							);
							return (1);
						}

						snprintf(
							REPLY, REPLY_SIZE, "%s is %s: %s.", SHOWN[WINNER],
							READ->TOKENS[INDEX].TEXT, FACTS
						);
						snprintf(
							ACTION, ACTION_SIZE, "order %s -> %s",
							READ->TOKENS[INDEX].TEXT, NAMES[WINNER]
						);
						return (1);
					}

					if (ABOVE[HIGHER][LOWER])
						snprintf(
							REPLY, REPLY_SIZE, "Yes, %s is %s than %s: %s.",
							SHOWN[ASKED[0]], READ->TOKENS[INDEX].TEXT,
							SHOWN[ASKED[1]], FACTS
						);
					else if (ABOVE[LOWER][HIGHER])
						snprintf(
							REPLY, REPLY_SIZE, "No, %s is not %s than %s: %s.",
							SHOWN[ASKED[0]], READ->TOKENS[INDEX].TEXT,
							SHOWN[ASKED[1]], FACTS
						);
					else
						snprintf(
							REPLY, REPLY_SIZE,
							"I can't tell from what you said: %s.", FACTS
						);

					snprintf(
						ACTION, ACTION_SIZE, "order %s %s %s", NAMES[ASKED[0]],
						READ->TOKENS[INDEX].TEXT, NAMES[ASKED[1]]
					);
					return (1);
				}
			}
	}

	return (0);
}

/* ---------- letters and words ---------- */

static const char	*ORDINALS[13] = {
	"first", "second", "third", "fourth", "fifth", "sixth", "seventh", "eighth",
	"ninth", "tenth", "eleventh", "twelfth", NULL
};

/* the word asked about: in quotes, or after "of (the word)", "in (the
 * word)", "word" */
static int
	ASKED_WORD(const READING *READ, int FROM, char *WORD, int WORD_SIZE)
{
	const char	*QUOTE = strchr(READ->MESSAGE, '"');
	int			INDEX;

	if (QUOTE && strchr(QUOTE + 1, '"'))
	{
		int	LENGTH = (int)(strchr(QUOTE + 1, '"') - QUOTE - 1);

		if (LENGTH > 0 && LENGTH < WORD_SIZE)
		{
			snprintf(WORD, WORD_SIZE, "%.*s", LENGTH, QUOTE + 1);
			return (1);
		}
	}

	for (INDEX = FROM; INDEX < READ->COUNT; INDEX++)
	{
		const char	*TEXT = READ->TOKENS[INDEX].TEXT;

		if (
			(
				!strcmp(TEXT, "of") ||
				!strcmp(TEXT, "in") ||
				!strcmp(TEXT, "word")
			) &&
			INDEX + 1 < READ->COUNT
		)
		{
			int	NEXT = INDEX + 1;

			while (
				NEXT < READ->COUNT &&
				(
					!strcmp(READ->TOKENS[NEXT].TEXT, "the") ||
					!strcmp(READ->TOKENS[NEXT].TEXT, "word") ||
					!strcmp(READ->TOKENS[NEXT].TEXT, "name") ||
					!strcmp(READ->TOKENS[NEXT].TEXT, "'") ||
					READ->TOKENS[NEXT].TEXT[0] == '\''
				)
			)
				NEXT++;

			if (NEXT < READ->COUNT && READ->TOKENS[NEXT].KIND == TOKEN_WORD)
			{
				snprintf(WORD, WORD_SIZE, "%s", READ->TOKENS[NEXT].ORIGINAL);
				return (1);
			}
		}
	}

	return (0);
}

static int
	IS_VOWEL(char LETTER)
{
	return (strchr("aeiouAEIOU", LETTER) != NULL && LETTER);
}

static int
	ANSWER_LETTERS(
		const READING *READ, char *ACTION, int ACTION_SIZE, char *REPLY,
		int REPLY_SIZE
	)
{
	char	WORD[200];
	int		INDEX;

	/* "what is the third letter of the word house?" */
	if (HAS(READ, " letter of ") || HAS(READ, " letter in "))
	{
		int	ORDINAL = -1;
		int	FROM_END = 0;

		for (INDEX = 0; INDEX < READ->COUNT; INDEX++)
		{
			int	FOUND = FIND_WORD(ORDINALS, READ->TOKENS[INDEX].TEXT);

			if (FOUND >= 0)
				ORDINAL = FOUND;

			if (!strcmp(READ->TOKENS[INDEX].TEXT, "last"))
				FROM_END = 1;

			/* "the 3rd letter" */
			if (
				READ->TOKENS[INDEX].KIND == TOKEN_NUMBER &&
				INDEX + 1 < READ->COUNT &&
				(
					!strcmp(READ->TOKENS[INDEX + 1].TEXT, "rd") ||
					!strcmp(READ->TOKENS[INDEX + 1].TEXT, "th") ||
					!strcmp(READ->TOKENS[INDEX + 1].TEXT, "st") ||
					!strcmp(READ->TOKENS[INDEX + 1].TEXT, "nd")
				)
			)
				ORDINAL = (int)READ->TOKENS[INDEX].VALUE - 1;
		}

		if (
			(ORDINAL < 0 && !FROM_END) ||
			!ASKED_WORD(READ, 0, WORD, sizeof WORD)
		)
			return (0);

		{
			int	LENGTH = (int)strlen(WORD);
			int	AT;

			if (FROM_END)
				AT = LENGTH - 1 - (ORDINAL < 0 ? 0 : ORDINAL);
			else
				AT = ORDINAL;

			if (
				FROM_END &&
				ORDINAL >= 0 &&
				!HAS(READ, " second last ") &&
				!HAS(READ, " second to last ")
			)
				AT = LENGTH - 1;

			if (AT < 0 || AT >= LENGTH)
			{
				snprintf(
					REPLY, REPLY_SIZE, "\"%s\" only has %d letters.", WORD,
					LENGTH
				);
				snprintf(ACTION, ACTION_SIZE, "letter of %s -> none", WORD);
				return (1);
			}

			snprintf(
				REPLY, REPLY_SIZE, "The %s letter of \"%s\" is %c.",
				FROM_END ? "last" : ORDINALS[ORDINAL], WORD,
				tolower((uint8_t)WORD[AT])
			);
			snprintf(
				ACTION, ACTION_SIZE, "letter of %s -> %c", WORD,
				tolower((uint8_t)WORD[AT])
			);
			return (1);
		}
	}

	/* "what letter comes after F?" */
	if (
		(
			HAS(READ, " letter comes after ") ||
			HAS(READ, " letter comes before ") ||
			HAS(READ, " letter is after ") ||
			HAS(READ, " letter is before ") ||
			HAS(READ, " letter after ") ||
			HAS(READ, " letter before ")
		)
	)
	{
		int	AFTER = HAS(READ, " after ");

		for (INDEX = READ->COUNT - 1; INDEX >= 0; INDEX--)
			if (
				READ->TOKENS[INDEX].KIND == TOKEN_WORD &&
				strlen(READ->TOKENS[INDEX].TEXT) == 1
			)
			{
				char	LETTER = READ->TOKENS[INDEX].ORIGINAL[0];
				char	BASE;

				if (isupper((uint8_t)LETTER))
					BASE = 'A';
				else
					BASE = 'a';

				int	POSITION = LETTER - BASE + (AFTER ? 1 : -1);

				if (POSITION < 0 || POSITION > 25)
					snprintf(
						REPLY, REPLY_SIZE, "No letter comes %s %c.",
						AFTER ? "after" : "before", LETTER
					);
				else
					snprintf(
						REPLY, REPLY_SIZE, "%c comes %s %c.", BASE + POSITION,
						AFTER ? "after" : "before", LETTER
					);

				snprintf(
					ACTION, ACTION_SIZE, "letter %s %c",
					AFTER ? "after" : "before", LETTER
				);
				return (1);
			}

		return (0);
	}

	/* "how many vowels are in education?", "count the consonants in apple" */
	if (HAS(READ, " vowels ") || HAS(READ, " consonants "))
	{
		int	VOWELS = 0;
		int	CONSONANTS = 0;

		if (!ASKED_WORD(READ, 0, WORD, sizeof WORD))
		{
			/* "count the vowels education" */
			if (
				READ->COUNT < 1 ||
				READ->TOKENS[READ->COUNT - 1].KIND != TOKEN_WORD
			)
				return (0);

			snprintf(
				WORD, sizeof WORD, "%s", READ->TOKENS[READ->COUNT - 1].ORIGINAL
			);
		}

		for (INDEX = 0; WORD[INDEX]; INDEX++)
			if (isalpha((uint8_t)WORD[INDEX]))
			{
				if (IS_VOWEL(WORD[INDEX]))
					VOWELS++;
				else
					CONSONANTS++;
			}

		if (HAS(READ, " vowels "))
		{
			char	LIST[100] = "";
			int		POSITION = 0;

			for (INDEX = 0; WORD[INDEX] && POSITION < 90; INDEX++)
				if (IS_VOWEL(WORD[INDEX]))
					POSITION += snprintf(
						LIST + POSITION, sizeof LIST - POSITION, "%s%c",
						POSITION ? ", " : "", tolower((uint8_t)WORD[INDEX])
					);

			snprintf(
				REPLY, REPLY_SIZE, "\"%s\" has %d vowel%s%s%s%s", WORD, VOWELS,
				VOWELS == 1 ? "" : "s", VOWELS ? ": " : "", LIST, "."
			);
			snprintf(ACTION, ACTION_SIZE, "vowels in %s -> %d", WORD, VOWELS);
		}
		else
		{
			snprintf(
				REPLY, REPLY_SIZE, "\"%s\" has %d consonant%s.", WORD,
				CONSONANTS, CONSONANTS == 1 ? "" : "s"
			);
			snprintf(
				ACTION, ACTION_SIZE, "consonants in %s -> %d", WORD, CONSONANTS
			);
		}

		return (1);
	}

	/* "which word is longer: cat or elephant?" (words, not numbers) */
	if (
		(
			HAS(READ, " longer ") ||
			HAS(READ, " shorter ") ||
			HAS(READ, " more letters ") ||
			HAS(READ, " fewer letters ")
		) &&
		HAS(READ, " or ") &&
		(HAS(READ, " word ") || HAS(READ, " which "))
	)
	{
		int	OR_INDEX = -1;

		for (INDEX = 1; INDEX + 1 < READ->COUNT; INDEX++)
			if (!strcmp(READ->TOKENS[INDEX].TEXT, "or"))
				OR_INDEX = INDEX;

		if (
			OR_INDEX > 0 &&
			READ->TOKENS[OR_INDEX - 1].KIND == TOKEN_WORD &&
			READ->TOKENS[OR_INDEX + 1].KIND == TOKEN_WORD
		)
		{
			const TOKEN	*FIRST = &READ->TOKENS[OR_INDEX - 1];
			const TOKEN	*SECOND = &READ->TOKENS[OR_INDEX + 1];
			int			FIRST_LENGTH = (int)strlen(FIRST->TEXT);
			int			SECOND_LENGTH = (int)strlen(SECOND->TEXT);
			int			WANT_LONGER =
				HAS(READ, " longer ") || HAS(READ, " more letters ");
			const TOKEN	*WINNER;

			if (FIRST_LENGTH == SECOND_LENGTH)
			{
				snprintf(
					REPLY, REPLY_SIZE,
					"Neither: \"%s\" and \"%s\" both have %d letters.",
					FIRST->ORIGINAL, SECOND->ORIGINAL, FIRST_LENGTH
				);
				snprintf(ACTION, ACTION_SIZE, "word length -> same");
				return (1);
			}

			if ((FIRST_LENGTH > SECOND_LENGTH) == WANT_LONGER)
				WINNER = FIRST;
			else
				WINNER = SECOND;

			snprintf(
				REPLY, REPLY_SIZE,
				"\"%s\" is %s: \"%s\" has %d letters and \"%s\" has %d.",
				WINNER->ORIGINAL, WANT_LONGER ? "longer" : "shorter",
				FIRST->ORIGINAL, FIRST_LENGTH, SECOND->ORIGINAL, SECOND_LENGTH
			);
			snprintf(ACTION, ACTION_SIZE, "word length -> %s", WINNER->TEXT);
			return (1);
		}
	}

	/* "what is the first word of this sentence?" */
	if (
		(
			HAS(READ, " of this sentence ") ||
			HAS(READ, " of this question ") ||
			HAS(READ, " in this sentence ") ||
			HAS(READ, " in this question ")
		) &&
		(HAS(READ, " word ") || HAS(READ, " words "))
	)
	{
		int	WORDS = 0;
		int	FIRST_WORD = -1;
		int	LAST_WORD = -1;

		for (INDEX = 0; INDEX < READ->COUNT; INDEX++)
			if (READ->TOKENS[INDEX].KIND != TOKEN_SYMBOL)
			{
				WORDS++;

				if (FIRST_WORD < 0)
					FIRST_WORD = INDEX;

				LAST_WORD = INDEX;
			}

		if (FIRST_WORD < 0)
			return (0);

		if (HAS(READ, " how many words "))
		{
			snprintf(REPLY, REPLY_SIZE, "This sentence has %d words.", WORDS);
			snprintf(
				ACTION, ACTION_SIZE, "words in this sentence -> %d", WORDS
			);
			return (1);
		}

		if (HAS(READ, " last word "))
		{
			snprintf(
				REPLY, REPLY_SIZE, "The last word is \"%s\".",
				READ->TOKENS[LAST_WORD].ORIGINAL
			);
			snprintf(
				ACTION, ACTION_SIZE, "last word -> %s",
				READ->TOKENS[LAST_WORD].TEXT
			);
			return (1);
		}

		if (HAS(READ, " first word "))
		{
			snprintf(
				REPLY, REPLY_SIZE, "The first word is \"%s\".",
				READ->TOKENS[FIRST_WORD].ORIGINAL
			);
			snprintf(
				ACTION, ACTION_SIZE, "first word -> %s",
				READ->TOKENS[FIRST_WORD].TEXT
			);
			return (1);
		}
	}

	/* "write hello in capital letters", "make HELLO lowercase" */
	if (
		HAS(READ, " capital letters ") ||
		HAS(READ, " uppercase ") ||
		HAS(READ, " upper case ") ||
		HAS(READ, " all caps ") ||
		HAS(READ, " lowercase ") ||
		HAS(READ, " lower case ") ||
		HAS(READ, " small letters ")
	)
	{
		int			UPPER =
			!(HAS(READ, " lowercase ") || HAS(READ, " lower case ") ||
				HAS(READ, " small letters "));
		char		TEXT[300] = "";
		int			POSITION = 0;
		int			START = -1;
		int			END = -1;
		const char	*QUOTE = strchr(READ->MESSAGE, '"');

		if (QUOTE && strchr(QUOTE + 1, '"'))
			snprintf(
				TEXT, sizeof TEXT, "%.*s",
				(int)(strchr(QUOTE + 1, '"') - QUOTE - 1), QUOTE + 1
			);
		else
		{
			/* "write hello world in capital letters": the words between the
			 * verb and "in" */
			for (INDEX = 0; INDEX < READ->COUNT; INDEX++)
			{
				const char	*TEXT_WORD = READ->TOKENS[INDEX].TEXT;

				if (
					START < 0 &&
					(
						!strcmp(TEXT_WORD, "write") ||
						!strcmp(TEXT_WORD, "type") ||
						!strcmp(TEXT_WORD, "say") ||
						!strcmp(TEXT_WORD, "spell") ||
						!strcmp(TEXT_WORD, "put") ||
						!strcmp(TEXT_WORD, "make") ||
						!strcmp(TEXT_WORD, "convert") ||
						!strcmp(TEXT_WORD, "turn") ||
						!strcmp(TEXT_WORD, "print")
					)
				)
					START = INDEX + 1;
				else if (
					START >= 0 &&
					END < 0 &&
					(
						!strcmp(TEXT_WORD, "in") ||
						!strcmp(TEXT_WORD, "into") ||
						!strcmp(TEXT_WORD, "to") ||
						!strcmp(TEXT_WORD, "as") ||
						!strcmp(TEXT_WORD, "uppercase") ||
						!strcmp(TEXT_WORD, "lowercase")
					)
				)
					END = INDEX;
			}

			if (START < 0 || END <= START)
				return (0);

			for (INDEX = START; INDEX < END && POSITION < 280; INDEX++)
			{
				const TOKEN	*ITEM = &READ->TOKENS[INDEX];

				if (
					INDEX == START &&
					(!strcmp(ITEM->TEXT, "the") || !strcmp(ITEM->TEXT, "word"))
				)
					continue ;

				if (
					!strcmp(ITEM->TEXT, "word") &&
					INDEX == START + 1 &&
					!strcmp(READ->TOKENS[START].TEXT, "the")
				)
					continue ;

				POSITION += snprintf(
					TEXT + POSITION, sizeof TEXT - POSITION, "%s%s",
					POSITION && ITEM->KIND != TOKEN_SYMBOL ? " " : "",
					ITEM->ORIGINAL
				);
			}
		}

		if (!TEXT[0])
			return (0);

		for (INDEX = 0; TEXT[INDEX]; INDEX++)
			TEXT[INDEX] = UPPER ? (char)toupper((uint8_t)TEXT[INDEX])
								: (char)tolower((uint8_t)TEXT[INDEX]);

		snprintf(REPLY, REPLY_SIZE, "%s", TEXT);
		snprintf(
			ACTION, ACTION_SIZE, "%s -> %s", UPPER ? "uppercase" : "lowercase",
			TEXT
		);
		return (1);
	}

	return (0);
}

/* ---------- days and months ---------- */

static const char	*DAYS[8] = {
	"monday", "tuesday", "wednesday", "thursday", "friday", "saturday",
	"sunday", NULL
};
static const char	*MONTHS[13] = {
	"january", "february", "march", "april", "may", "june", "july", "august",
	"september", "october", "november", "december", NULL
};

static void
	CAPITAL(const char *WORD, char *OUTPUT, int OUTPUT_SIZE)
{
	snprintf(OUTPUT, OUTPUT_SIZE, "%s", WORD);
	OUTPUT[0] = (char)toupper((uint8_t)OUTPUT[0]);
}

static int
	ANSWER_DAYS(
		const READING *READ, char *ACTION, int ACTION_SIZE, char *REPLY,
		int REPLY_SIZE
	)
{
	int					DAY = -1;
	int					MONTH = -1;
	int					INDEX;
	double				SHIFT = 0;
	int					HAS_SHIFT = 0;
	const char *const	*NAMES;
	int					NAME_COUNT;
	int					FROM;
	char				FROM_TEXT[32];
	char				TO_TEXT[32];

	for (INDEX = 0; INDEX < READ->COUNT; INDEX++)
	{
		int	FOUND = FIND_WORD(DAYS, READ->TOKENS[INDEX].TEXT);

		if (FOUND >= 0 && DAY < 0)
			DAY = FOUND;

		FOUND = FIND_WORD(MONTHS, READ->TOKENS[INDEX].TEXT);

		/* "may" is a month only next to month words */
		if (FOUND >= 0 && MONTH < 0 && (FOUND != 4 || HAS(READ, " month ")))
			MONTH = FOUND;

		/* "in 3 days", "3 days after", "2 days ago", "5 months after" */
		if (
			IS_NUMBER(&READ->TOKENS[INDEX]) &&
			INDEX + 1 < READ->COUNT &&
			(
				!strcmp(READ->TOKENS[INDEX + 1].TEXT, "days") ||
				!strcmp(READ->TOKENS[INDEX + 1].TEXT, "day") ||
				!strcmp(READ->TOKENS[INDEX + 1].TEXT, "weeks") ||
				!strcmp(READ->TOKENS[INDEX + 1].TEXT, "week") ||
				!strcmp(READ->TOKENS[INDEX + 1].TEXT, "months") ||
				!strcmp(READ->TOKENS[INDEX + 1].TEXT, "month")
			)
		)
		{
			SHIFT = READ->TOKENS[INDEX].VALUE;

			if (READ->TOKENS[INDEX + 1].TEXT[0] == 'w')
				SHIFT *= 7;

			if (
				(
					INDEX + 2 < READ->COUNT &&
					(
						!strcmp(READ->TOKENS[INDEX + 2].TEXT, "ago") ||
						!strcmp(READ->TOKENS[INDEX + 2].TEXT, "before") ||
						!strcmp(READ->TOKENS[INDEX + 2].TEXT, "earlier")
					)
				) ||
				HAS(READ, " was it ")
			)
				SHIFT = -SHIFT;

			HAS_SHIFT = 1;
		}
	}

	if (HAS(READ, " day after tomorrow "))
	{
		SHIFT = 2;
		HAS_SHIFT = 1;
	}
	else if (HAS(READ, " day before yesterday "))
	{
		SHIFT = -2;
		HAS_SHIFT = 1;
	}
	else if (HAS(READ, " tomorrow ") && !HAS_SHIFT)
	{
		SHIFT = 1;
		HAS_SHIFT = 1;
	}
	else if (HAS(READ, " yesterday ") && !HAS_SHIFT)
	{
		SHIFT = -1;
		HAS_SHIFT = 1;
	}

	if (DAY >= 0 && MONTH < 0)
	{
		NAMES = DAYS;
		NAME_COUNT = 7;
		FROM = DAY;
	}
	else if (MONTH >= 0 && DAY < 0)
	{
		NAMES = MONTHS;
		NAME_COUNT = 12;
		FROM = MONTH;
	}
	else
		return (0);

	/* "what day comes after Monday?" */
	if (!HAS_SHIFT)
	{
		if (
			HAS(READ, " comes after ") ||
			HAS(READ, " is after ") ||
			HAS(READ, " follows ") ||
			HAS(READ, " after ")
		)
			SHIFT = 1;
		else if (
			HAS(READ, " comes before ") ||
			HAS(READ, " is before ") ||
			HAS(READ, " before ")
		)
			SHIFT = -1;
		else
			return (0);
	}
	else if (HAS(READ, " before ") && SHIFT > 0 && !HAS(READ, " if today is "))
		SHIFT = -SHIFT;

	if (!HAS(READ, " what ") && !HAS(READ, " which "))
		return (0);

	if (NAMES == MONTHS && !HAS(READ, " month "))
		return (0);

	if (NAMES == DAYS && !HAS(READ, " day "))
		return (0);

	{
		long	STEPS = (long)floor(SHIFT + 0.5);
		int		TO =
			(int)(((FROM + STEPS) % NAME_COUNT + NAME_COUNT) % NAME_COUNT);

		CAPITAL(NAMES[FROM], FROM_TEXT, sizeof FROM_TEXT);
		CAPITAL(NAMES[TO], TO_TEXT, sizeof TO_TEXT);

		if (
			HAS(READ, " if today is ") ||
			HAS(READ, " if it is ") ||
			HAS(READ, " if it's ")
		)
			snprintf(
				REPLY, REPLY_SIZE, "It %s %s.", STEPS < 0 ? "was" : "will be",
				TO_TEXT
			);
		else if (labs(STEPS) == 1)
			snprintf(
				REPLY, REPLY_SIZE, "%s comes %s %s.", TO_TEXT,
				STEPS > 0 ? "after" : "before", FROM_TEXT
			);
		else
			snprintf(
				REPLY, REPLY_SIZE, "%ld %s%s %s %s is %s.", labs(STEPS),
				NAMES == DAYS ? "day" : "month", labs(STEPS) == 1 ? "" : "s",
				STEPS > 0 ? "after" : "before", FROM_TEXT, TO_TEXT
			);

		snprintf(
			ACTION, ACTION_SIZE, "%s %+ld -> %s", NAMES[FROM], STEPS, NAMES[TO]
		);
		return (1);
	}
}

/* ---------- sorting ---------- */

static int
	COMPARE_UP(const void *FIRST, const void *SECOND)
{
	double	A = *(const double *)FIRST;
	double	B = *(const double *)SECOND;

	return ((A > B) - (A < B));
}

static int
	COMPARE_TEXT(const void *FIRST, const void *SECOND)
{
	const char	*A = FIRST;
	const char	*B = SECOND;
	char		LOWER_A[64];
	char		LOWER_B[64];
	int			INDEX;

	for (INDEX = 0; A[INDEX] && INDEX < 63; INDEX++)
		LOWER_A[INDEX] = (char)tolower((uint8_t)A[INDEX]);

	LOWER_A[INDEX] = 0;

	for (INDEX = 0; B[INDEX] && INDEX < 63; INDEX++)
		LOWER_B[INDEX] = (char)tolower((uint8_t)B[INDEX]);

	LOWER_B[INDEX] = 0;

	return (strcmp(LOWER_A, LOWER_B));
}

static int
	ANSWER_SORT(
		const READING *READ, char *ACTION, int ACTION_SIZE, char *REPLY,
		int REPLY_SIZE
	)
{
	int		DOWN;
	int		ALPHABET;
	double	VALUES[64];
	int		COUNT;
	int		INDEX;
	int		POSITION = 0;

	/* "reverse the order: ..." turns the list around, it does not sort it */
	if (HAS(READ, " reverse ") && !HAS(READ, " reverse alphabetical "))
		return (0);

	if (
		!HAS(READ, " sort ") &&
		!HAS(READ, " order ") &&
		!HAS(READ, " arrange ") &&
		!HAS(READ, " alphabetical ") &&
		!HAS(READ, " alphabetically ") &&
		!HAS(READ, " ascending ") &&
		!HAS(READ, " descending ") &&
		!HAS(READ, " smallest to largest ") &&
		!HAS(READ, " largest to smallest ")
	)
		return (0);

	DOWN = HAS(READ, " descending ") || HAS(READ, " largest to smallest ") ||
		HAS(READ, " biggest to smallest ") ||
		HAS(READ, " highest to lowest ") || HAS(READ, " largest first ") ||
		HAS(READ, " biggest first ") || HAS(READ, " reverse alphabetical ") ||
		HAS(READ, " z to a ") || HAS(READ, " greatest to least ");
	ALPHABET = HAS(READ, " alphabetical ") || HAS(READ, " alphabetically ") ||
		HAS(READ, " a to z ") || HAS(READ, " z to a ");

	COUNT = NUMBER_LIST(READ, 0, VALUES, 64, NULL, NULL);

	if (!ALPHABET && COUNT >= 2)
	{
		char	TEXT[64];

		qsort(VALUES, (size_t)COUNT, sizeof(double), COMPARE_UP);

		for (INDEX = 0; INDEX < COUNT && POSITION < REPLY_SIZE - 40; INDEX++)
		{
			SAY_NUMBER(
				VALUES[DOWN ? COUNT - 1 - INDEX : INDEX], TEXT, sizeof TEXT
			);
			POSITION += snprintf(
				REPLY + POSITION, REPLY_SIZE - POSITION, "%s%s",
				INDEX ? ", " : "", TEXT
			);
		}

		snprintf(REPLY + POSITION, REPLY_SIZE - POSITION, ".");
		snprintf(ACTION, ACTION_SIZE, "sort %d numbers", COUNT);
		return (1);
	}

	/* words after the colon, split by commas: "pear, apple, mango" */
	{
		const char	*COLON = strchr(READ->MESSAGE, ':');
		char		WORDS[40][64];
		int			WORD_COUNT = 0;
		const char	*CURSOR;

		if (!COLON)
			return (0);

		for (CURSOR = COLON + 1; *CURSOR && WORD_COUNT < 40;)
		{
			char	ITEM[64];
			int		LENGTH = 0;

			while (*CURSOR == ' ' || *CURSOR == ',')
				CURSOR++;

			while (*CURSOR && *CURSOR != ',' && LENGTH < 63)
				ITEM[LENGTH++] = *CURSOR++;

			while (LENGTH && strchr(" .?!", ITEM[LENGTH - 1]))
				LENGTH--;

			ITEM[LENGTH] = 0;

			/* "pear, apple and mango" */
			if (!strncmp(ITEM, "and ", 4))
				memmove(ITEM, ITEM + 4, strlen(ITEM + 4) + 1);

			{
				char	*AND = strstr(ITEM, " and ");

				if (AND && WORD_COUNT < 39)
				{
					*AND = 0;
					snprintf(WORDS[WORD_COUNT++], 64, "%s", ITEM);
					snprintf(ITEM, sizeof ITEM, "%s", AND + 5);
				}
			}

			if (ITEM[0])
				snprintf(WORDS[WORD_COUNT++], 64, "%s", ITEM);
		}

		if (WORD_COUNT < 2)
			return (0);

		qsort(WORDS, (size_t)WORD_COUNT, sizeof WORDS[0], COMPARE_TEXT);

		for (
			INDEX = 0;
			INDEX < WORD_COUNT && POSITION < REPLY_SIZE - 70;
			INDEX++
		)
			POSITION += snprintf(
				REPLY + POSITION, REPLY_SIZE - POSITION, "%s%s",
				INDEX ? ", " : "", WORDS[DOWN ? WORD_COUNT - 1 - INDEX : INDEX]
			);

		snprintf(REPLY + POSITION, REPLY_SIZE - POSITION, ".");
		snprintf(ACTION, ACTION_SIZE, "sort %d words", WORD_COUNT);
		return (1);
	}
}

/* ---------- units ---------- */

typedef struct
{
	const char	*NAMES;
	double		SIZE;
	int			KIND;
} UNIT;

static const UNIT	UNITS[25] = {
	{ " second seconds sec secs s ", 1, 1 },
	{ " minute minutes min mins ", 60, 1 },
	{ " hour hours hr hrs h ", 3600, 1 }, { " day days ", 86400, 1 },
	{ " week weeks ", 604800, 1 }, { " year years ", 31536000, 1 },
	{ " millimeter millimeters millimetre millimetres mm ", 0.001, 2 },
	{ " centimeter centimeters centimetre centimetres cm ", 0.01, 2 },
	{ " meter meters metre metres m ", 1, 2 },
	{ " kilometer kilometers kilometre kilometres km ", 1000, 2 },
	{ " inch inches in ", 0.0254, 2 }, { " foot feet ft ", 0.3048, 2 },
	{ " yard yards yd ", 0.9144, 2 }, { " mile miles mi ", 1609.344, 2 },
	{ " milligram milligrams mg ", 0.001, 3 }, { " gram grams g ", 1, 3 },
	{ " kilogram kilograms kilo kilos kg ", 1000, 3 },
	{ " pound pounds lb lbs ", 453.59237, 3 },
	{ " ounce ounces oz ", 28.349523125, 3 },
	{ " ton tons tonne tonnes ", 1000000, 3 },
	{ " milliliter milliliters millilitre millilitres ml ", 0.001, 4 },
	{ " liter liters litre litres l ", 1, 4 }, { " dozen dozens ", 12, 5 },
	{ " month months ", 1, 6 }, { NULL, 0, 0 }
};

static int
	FIND_UNIT(const char *WORD)
{
	char	PADDED[80];
	int		INDEX;

	snprintf(PADDED, sizeof PADDED, " %s ", WORD);

	for (INDEX = 0; UNITS[INDEX].NAMES; INDEX++)
		if (strstr(UNITS[INDEX].NAMES, PADDED))
			return (INDEX);

	return (-1);
}

static int
	ANSWER_UNITS(
		const READING *READ, char *ACTION, int ACTION_SIZE, char *REPLY,
		int REPLY_SIZE
	)
{
	int			TARGET = -1;
	int			SOURCE = -1;
	double		AMOUNT = 1;
	int			INDEX;
	char		AMOUNT_TEXT[64];
	char		RESULT_TEXT[64];
	double		RESULT;
	const char	*TARGET_WORD = NULL;
	const char	*SOURCE_WORD = NULL;
	int			LEAP = 0;
	char		TARGET_SHOWN[64];

	/* "how many minutes are in 3 hours", "how many seconds in a day" */
	if (HAS(READ, " how many "))
	{
		for (INDEX = 0; INDEX + 2 < READ->COUNT; INDEX++)
			if (
				!strcmp(READ->TOKENS[INDEX].TEXT, "how") &&
				!strcmp(READ->TOKENS[INDEX + 1].TEXT, "many")
			)
			{
				TARGET = FIND_UNIT(READ->TOKENS[INDEX + 2].TEXT);
				TARGET_WORD = READ->TOKENS[INDEX + 2].TEXT;

				/* "in" must follow: "how many minutes are in 3 hours" */
				{
					int	SCAN;

					for (SCAN = INDEX + 3; SCAN + 1 < READ->COUNT; SCAN++)
						if (
							!strcmp(READ->TOKENS[SCAN].TEXT, "in") ||
							!strcmp(READ->TOKENS[SCAN].TEXT, "per")
						)
						{
							int	AT = SCAN + 1;

							if (IS_NUMBER(&READ->TOKENS[AT]))
							{
								AMOUNT = READ->TOKENS[AT].VALUE;
								AT++;
							}
							else if (
								!strcmp(READ->TOKENS[AT].TEXT, "a") ||
								!strcmp(READ->TOKENS[AT].TEXT, "an") ||
								!strcmp(READ->TOKENS[AT].TEXT, "one")
							)
								AT++;

							/* "a leap year": 366 days */
							if (
								AT + 1 < READ->COUNT &&
								!strcmp(READ->TOKENS[AT].TEXT, "leap") &&
								!strncmp(READ->TOKENS[AT + 1].TEXT, "year", 4)
							)
							{
								LEAP = 1;
								AT++;
							}

							if (AT < READ->COUNT)
							{
								SOURCE = FIND_UNIT(READ->TOKENS[AT].TEXT);
								SOURCE_WORD = READ->TOKENS[AT].TEXT;
							}

							break ;
						}
				}

				break ;
			}
	}
	/* "convert 5 km to meters", "what is 3 hours in minutes", "5 km is how
	 * many meters" */
	else
	{
		for (INDEX = 0; INDEX + 3 < READ->COUNT; INDEX++)
			if (
				IS_NUMBER(&READ->TOKENS[INDEX]) &&
				FIND_UNIT(READ->TOKENS[INDEX + 1].TEXT) >= 0 &&
				(
					!strcmp(READ->TOKENS[INDEX + 2].TEXT, "to") ||
					!strcmp(READ->TOKENS[INDEX + 2].TEXT, "in") ||
					!strcmp(READ->TOKENS[INDEX + 2].TEXT, "into")
				)
			)
			{
				AMOUNT = READ->TOKENS[INDEX].VALUE;
				SOURCE = FIND_UNIT(READ->TOKENS[INDEX + 1].TEXT);
				SOURCE_WORD = READ->TOKENS[INDEX + 1].TEXT;
				TARGET = FIND_UNIT(READ->TOKENS[INDEX + 3].TEXT);
				TARGET_WORD = READ->TOKENS[INDEX + 3].TEXT;
				break ;
			}

		if (
			TARGET < 0 ||
			!(
				HAS(READ, " convert ") ||
				HAS(READ, " what is ") ||
				HAS(READ, " how much is ") ||
				HAS(READ, " what's ")
			)
		)
			return (0);
	}

	if (
		TARGET < 0 ||
		SOURCE < 0 ||
		TARGET == SOURCE ||
		UNITS[TARGET].KIND != UNITS[SOURCE].KIND
	)
	{
		/* "how many months are in 2 years" */
		if (
			TARGET >= 0 &&
			SOURCE >= 0 &&
			UNITS[TARGET].KIND == 6 &&
			strstr(UNITS[SOURCE].NAMES, " year ")
		)
			RESULT = AMOUNT * 12;
		else if (
			TARGET >= 0 &&
			SOURCE >= 0 &&
			UNITS[SOURCE].KIND == 6 &&
			strstr(UNITS[TARGET].NAMES, " year ")
		)
			RESULT = AMOUNT / 12;
		else
			return (0);
	}
	else
		RESULT = AMOUNT * UNITS[SOURCE].SIZE / UNITS[TARGET].SIZE;

	/* a leap year has one day more */
	if (LEAP && UNITS[TARGET].KIND == UNITS[SOURCE].KIND)
		RESULT =
			AMOUNT * (UNITS[SOURCE].SIZE * 366.0 / 365.0) / UNITS[TARGET].SIZE;

	/* "1 meter", not "1 meters" */
	if (fabs(RESULT - 1) < 1e-9)
		SINGULAR(TARGET_WORD, TARGET_SHOWN, sizeof TARGET_SHOWN);
	else
		snprintf(TARGET_SHOWN, sizeof TARGET_SHOWN, "%s", TARGET_WORD);

	TARGET_WORD = TARGET_SHOWN;

	SAY_NUMBER(AMOUNT, AMOUNT_TEXT, sizeof AMOUNT_TEXT);
	SAY_NUMBER(
		floor(RESULT * 1e6 + 0.5) / 1e6, RESULT_TEXT, sizeof RESULT_TEXT
	);

	{
		char	SOURCE_SHOWN[64];

		/* "a day", "3 hours", "a leap year" */
		if (AMOUNT == 1 && LEAP)
			snprintf(
				SOURCE_SHOWN, sizeof SOURCE_SHOWN, "a leap %s", SOURCE_WORD
			);
		else if (AMOUNT == 1 && !HAS(READ, " 1 "))
			snprintf(
				SOURCE_SHOWN, sizeof SOURCE_SHOWN, "%s %s",
				ARTICLE(SOURCE_WORD), SOURCE_WORD
			);
		else
			snprintf(
				SOURCE_SHOWN, sizeof SOURCE_SHOWN, "%s %s", AMOUNT_TEXT,
				SOURCE_WORD
			);

		/* "about 104.29 weeks" */
		if (
			fabs(RESULT - floor(RESULT + 0.5)) > 1e-6 &&
			fabs(RESULT * 100 - floor(RESULT * 100 + 0.5)) > 1e-6
		)
		{
			char	ROUNDED[64];

			SAY_NUMBER(
				floor(RESULT * 100 + 0.5) / 100, ROUNDED, sizeof ROUNDED
			);
			snprintf(RESULT_TEXT, sizeof RESULT_TEXT, "about %s", ROUNDED);
		}

		if (HAS(READ, " how many "))
			snprintf(
				REPLY, REPLY_SIZE, "There %s %s %s in %s%s",
				RESULT == 1 ? "is" : "are", RESULT_TEXT, TARGET_WORD,
				SOURCE_SHOWN,
				strstr(UNITS[TARGET].NAMES, " day ") &&
						strstr(SOURCE_SHOWN, "year") && !LEAP
					? " (366 in a leap year)."
					: "."
			);
		else
			snprintf(
				REPLY, REPLY_SIZE, "%s is %s %s.", SOURCE_SHOWN, RESULT_TEXT,
				TARGET_WORD
			);
	}

	/* "1 hour is 60 minutes, so 3 hours is 3 * 60 = 180 minutes" */
	if (
		TARGET >= 0 &&
		SOURCE >= 0 &&
		UNITS[TARGET].KIND == UNITS[SOURCE].KIND &&
		TARGET != SOURCE &&
		!LEAP
	)
	{
		double	FACTOR = UNITS[SOURCE].SIZE / UNITS[TARGET].SIZE;
		char	SOURCE_ONE[32] = "";
		char	SOURCE_MANY[32] = "";
		char	TARGET_ONE[32] = "";
		char	TARGET_MANY[32] = "";
		char	FACTOR_TEXT[64];
		char	EXACT_TEXT[64];

		sscanf(UNITS[SOURCE].NAMES, "%31s %31s", SOURCE_ONE, SOURCE_MANY);
		sscanf(UNITS[TARGET].NAMES, "%31s %31s", TARGET_ONE, TARGET_MANY);
		SAY_NUMBER(
			floor(RESULT * 1e6 + 0.5) / 1e6, EXACT_TEXT, sizeof EXACT_TEXT
		);

		if (FACTOR < 1 && fabs(1 / FACTOR - floor(1 / FACTOR + 0.5)) < 1e-9)
		{
			SAY_NUMBER(1 / FACTOR, FACTOR_TEXT, sizeof FACTOR_TEXT);
			ADD_WHY(
				REPLY, REPLY_SIZE,
				"1 %s is %s %s, so %s %s is %s / %s = "
				"%s %s.",
				TARGET_ONE, FACTOR_TEXT, SOURCE_MANY, AMOUNT_TEXT,
				AMOUNT == 1 ? SOURCE_ONE : SOURCE_MANY, AMOUNT_TEXT,
				FACTOR_TEXT, EXACT_TEXT,
				fabs(RESULT - 1) < 1e-9 ? TARGET_ONE : TARGET_MANY
			);
		}
		else
		{
			SAY_NUMBER(FACTOR, FACTOR_TEXT, sizeof FACTOR_TEXT);
			ADD_WHY(
				REPLY, REPLY_SIZE,
				"1 %s is %s %s, so %s %s is %s * %s = "
				"%s %s.",
				SOURCE_ONE, FACTOR_TEXT,
				fabs(FACTOR - 1) < 1e-9 ? TARGET_ONE : TARGET_MANY, AMOUNT_TEXT,
				AMOUNT == 1 ? SOURCE_ONE : SOURCE_MANY, AMOUNT_TEXT,
				FACTOR_TEXT, EXACT_TEXT,
				fabs(RESULT - 1) < 1e-9 ? TARGET_ONE : TARGET_MANY
			);
		}
	}

	snprintf(
		ACTION, ACTION_SIZE, "convert %s %s to %s -> %s", AMOUNT_TEXT,
		SOURCE_WORD, TARGET_WORD, RESULT_TEXT
	);

	return (1);
}

/* ---------- largest, smallest, sum, average of a list ---------- */

static int
	ANSWER_LIST_MATH(
		const READING *READ, char *ACTION, int ACTION_SIZE, char *REPLY,
		int REPLY_SIZE
	)
{
	static const char	*BIGGEST[7] = {
		" largest ", " biggest ", " highest ", " greatest ", " maximum ",
		" max ", NULL
	};
	static const char	*SMALLEST[6] = {
		" smallest ", " lowest ", " least ", " minimum ", " min ", NULL
	};
	double				VALUES[64];
	int					COUNT;
	int					INDEX;
	int					KIND = 0;
	char				TEXT[64];

	for (INDEX = 0; BIGGEST[INDEX]; INDEX++)
		if (HAS(READ, BIGGEST[INDEX]))
			KIND = 1;

	for (INDEX = 0; SMALLEST[INDEX]; INDEX++)
		if (HAS(READ, SMALLEST[INDEX]))
			KIND = 2;

	if (HAS(READ, " average ") || HAS(READ, " mean "))
		KIND = 3;
	else if (
		HAS(READ, " sum of ") ||
		HAS(READ, " total of ") ||
		HAS(READ, " add up ")
	)
		KIND = 4;
	else if (HAS(READ, " median "))
		KIND = 5;
	else if (HAS(READ, " product of "))
		KIND = 6;

	if (!KIND)
		return (0);

	/* "the sum of 2 and 3" is a sum too, but "largest of 2 numbers"
	 * is not a list */
	COUNT = NUMBER_LIST(READ, 0, VALUES, 64, NULL, NULL);

	if (COUNT < (KIND >= 3 ? 2 : 2))
		return (0);

	/* "which is bigger, 9.11 or 9.9" is the calculator's compare */
	if ((KIND == 1 || KIND == 2) && COUNT == 2 && HAS(READ, " or "))
		return (0);

	if (KIND == 1 || KIND == 2)
	{
		double	BEST = VALUES[0];

		for (INDEX = 1; INDEX < COUNT; INDEX++)
			if (KIND == 1 ? VALUES[INDEX] > BEST : VALUES[INDEX] < BEST)
				BEST = VALUES[INDEX];

		SAY_NUMBER(BEST, TEXT, sizeof TEXT);
		snprintf(
			REPLY, REPLY_SIZE, "The %s number is %s.",
			KIND == 1 ? "largest" : "smallest", TEXT
		);
		snprintf(
			ACTION, ACTION_SIZE, "%s of %d numbers -> %s",
			KIND == 1 ? "max" : "min", COUNT, TEXT
		);
		return (1);
	}

	if (KIND == 3 || KIND == 4 || KIND == 6)
	{
		double	TOTAL;

		if (KIND == 6)
			TOTAL = 1;
		else
			TOTAL = 0;

		char	TOTAL_TEXT[64];
		char	COUNT_TEXT[32];

		for (INDEX = 0; INDEX < COUNT; INDEX++)
		{
			if (KIND == 6)
				TOTAL = TOTAL * VALUES[INDEX];
			else
				TOTAL = TOTAL + VALUES[INDEX];
		}

		SAY_NUMBER(TOTAL, TOTAL_TEXT, sizeof TOTAL_TEXT);

		if (KIND == 4)
			snprintf(REPLY, REPLY_SIZE, "The sum is %s.", TOTAL_TEXT);
		else if (KIND == 6)
			snprintf(REPLY, REPLY_SIZE, "The product is %s.", TOTAL_TEXT);
		else
		{
			SAY_NUMBER(TOTAL / COUNT, TEXT, sizeof TEXT);
			snprintf(COUNT_TEXT, sizeof COUNT_TEXT, "%d", COUNT);
			snprintf(
				REPLY, REPLY_SIZE, "The average is %s: %s / %s = %s.", TEXT,
				TOTAL_TEXT, COUNT_TEXT, TEXT
			);
		}

		snprintf(
			ACTION, ACTION_SIZE, "%s of %d numbers -> %s",
			KIND == 3 ? "average"
				: KIND == 4 ? "sum"
							: "product",
			COUNT, KIND == 3 ? TEXT : TOTAL_TEXT
		);
		return (1);
	}

	/* the median */
	qsort(VALUES, (size_t)COUNT, sizeof(double), COMPARE_UP);
	SAY_NUMBER(
		COUNT % 2 ? VALUES[COUNT / 2]
		: (VALUES[COUNT / 2 - 1] + VALUES[COUNT / 2]) / 2,
		TEXT, sizeof TEXT
	);
	snprintf(REPLY, REPLY_SIZE, "The median is %s.", TEXT);
	snprintf(ACTION, ACTION_SIZE, "median of %d numbers -> %s", COUNT, TEXT);

	return (1);
}

/* ---------- clock times ---------- */

static void
	SAY_CLOCK(int MINUTES, int TWELVE_HOUR, char *OUTPUT, int OUTPUT_SIZE)
{
	int	HOUR;
	int	MINUTE;

	MINUTES = ((MINUTES % 1440) + 1440) % 1440;
	HOUR = MINUTES / 60;
	MINUTE = MINUTES % 60;

	if (TWELVE_HOUR)
		snprintf(
			OUTPUT, OUTPUT_SIZE, "%d:%02d %s", HOUR % 12 ? HOUR % 12 : 12,
			MINUTE, HOUR < 12 ? "am" : "pm"
		);
	else
		snprintf(OUTPUT, OUTPUT_SIZE, "%d:%02d", HOUR, MINUTE);
}

static int
	ANSWER_TIMES(
		const READING *READ, char *ACTION, int ACTION_SIZE, char *REPLY,
		int REPLY_SIZE
	)
{
	int		CLOCKS[2];
	int		HALVES[2] = { 0, 0 };
	int		CLOCK_COUNT = 0;
	int		INDEX;
	double	SHIFT_MINUTES = 0;
	int		HAS_SHIFT = 0;
	char	FROM_TEXT[32];
	char	TO_TEXT[32];

	for (INDEX = 0; INDEX < READ->COUNT; INDEX++)
	{
		const TOKEN	*ITEM = &READ->TOKENS[INDEX];

		if (ITEM->KIND != TOKEN_NUMBER)
			continue ;

		/* "10:30", "5 pm", "5pm" */
		if (
			ITEM->HOUR >= 0 ||
			(
				INDEX + 1 < READ->COUNT &&
				(
					!strcmp(READ->TOKENS[INDEX + 1].TEXT, "am") ||
					!strcmp(READ->TOKENS[INDEX + 1].TEXT, "pm")
				) &&
				ITEM->VALUE >= 1 &&
				ITEM->VALUE <= 12 &&
				ITEM->VALUE == floor(ITEM->VALUE)
			)
		)
		{
			int	HOUR;

			if (ITEM->HOUR >= 0)
				HOUR = ITEM->HOUR;
			else
				HOUR = (int)ITEM->VALUE;

			int	MINUTE;

			if (ITEM->HOUR >= 0)
				MINUTE = ITEM->MINUTE;
			else
				MINUTE = 0;

			if (CLOCK_COUNT == 2 || HOUR > 23 || MINUTE > 59)
				continue ;

			if (
				INDEX + 1 < READ->COUNT &&
				!strcmp(READ->TOKENS[INDEX + 1].TEXT, "pm")
			)
			{
				HALVES[CLOCK_COUNT] = 2;
				HOUR = HOUR % 12 + 12;
			}
			else if (
				INDEX + 1 < READ->COUNT &&
				!strcmp(READ->TOKENS[INDEX + 1].TEXT, "am")
			)
			{
				HALVES[CLOCK_COUNT] = 1;
				HOUR = HOUR % 12;
			}

			CLOCKS[CLOCK_COUNT++] = HOUR * 60 + MINUTE;
			continue ;
		}

		/* "3 hours", "45 minutes" */
		if (INDEX + 1 < READ->COUNT)
		{
			const char	*NEXT = READ->TOKENS[INDEX + 1].TEXT;

			if (!strcmp(NEXT, "hours") || !strcmp(NEXT, "hour"))
			{
				SHIFT_MINUTES += ITEM->VALUE * 60;
				HAS_SHIFT = 1;
			}
			else if (
				!strcmp(NEXT, "minutes") ||
				!strcmp(NEXT, "minute") ||
				!strcmp(NEXT, "mins")
			)
			{
				SHIFT_MINUTES += ITEM->VALUE;
				HAS_SHIFT = 1;
			}
		}
	}

	if (
		!CLOCK_COUNT ||
		!(
			HAS(READ, " time ") ||
			HAS(READ, " how long ") ||
			HAS(READ, " how many minutes ") ||
			HAS(READ, " how many hours ") ||
			HAS(READ, " plus ") ||
			HAS(READ, " minus ") ||
			(
				HAS_SHIFT &&
				CLOCK_COUNT == 1 &&
				(HAS(READ, " when ") || HAS(READ, " what time ")) &&
				(
					HAS(READ, " arrive") ||
					HAS(READ, " end") ||
					HAS(READ, " finish") ||
					HAS(READ, " get there ") ||
					HAS(READ, " be done ") ||
					HAS(READ, " land") ||
					HAS(READ, " be over ") ||
					HAS(READ, " get back ") ||
					HAS(READ, " get home ")
				)
			)
		)
	)
		return (0);

	/* "how long is it from 9:00 to 17:30?" */
	if (CLOCK_COUNT == 2 && !HAS_SHIFT)
	{
		int		DIFFERENCE = ((CLOCKS[1] - CLOCKS[0]) % 1440 + 1440) % 1440;
		char	LENGTH_TEXT[64];

		if (DIFFERENCE % 60 == 0)
			snprintf(
				LENGTH_TEXT, sizeof LENGTH_TEXT, "%d hour%s", DIFFERENCE / 60,
				DIFFERENCE == 60 ? "" : "s"
			);
		else if (DIFFERENCE < 60)
			snprintf(LENGTH_TEXT, sizeof LENGTH_TEXT, "%d minutes", DIFFERENCE);
		else
			snprintf(
				LENGTH_TEXT, sizeof LENGTH_TEXT, "%d hour%s and %d minute%s",
				DIFFERENCE / 60, DIFFERENCE / 60 == 1 ? "" : "s",
				DIFFERENCE % 60, DIFFERENCE % 60 == 1 ? "" : "s"
			);

		SAY_CLOCK(CLOCKS[0], HALVES[0] != 0, FROM_TEXT, sizeof FROM_TEXT);
		SAY_CLOCK(CLOCKS[1], HALVES[1] != 0, TO_TEXT, sizeof TO_TEXT);
		snprintf(
			REPLY, REPLY_SIZE, "From %s to %s is %s (%d minutes).", FROM_TEXT,
			TO_TEXT, LENGTH_TEXT, DIFFERENCE
		);
		snprintf(ACTION, ACTION_SIZE, "time between -> %d minutes", DIFFERENCE);
		return (1);
	}

	if (CLOCK_COUNT != 1 || !HAS_SHIFT)
		return (0);

	if (
		HAS(READ, " before ") ||
		HAS(READ, " earlier ") ||
		HAS(READ, " ago ") ||
		HAS(READ, " minus ")
	)
		SHIFT_MINUTES = -SHIFT_MINUTES;

	SAY_CLOCK(CLOCKS[0], HALVES[0] != 0, FROM_TEXT, sizeof FROM_TEXT);
	SAY_CLOCK(
		CLOCKS[0] + (int)floor(SHIFT_MINUTES + 0.5), HALVES[0] != 0, TO_TEXT,
		sizeof TO_TEXT
	);

	if (!HALVES[0])
	{
		char	TWELVE[32];

		SAY_CLOCK(
			CLOCKS[0] + (int)floor(SHIFT_MINUTES + 0.5), 1, TWELVE,
			sizeof TWELVE
		);
		snprintf(REPLY, REPLY_SIZE, "It will be %s (%s).", TO_TEXT, TWELVE);
	}
	else
		snprintf(REPLY, REPLY_SIZE, "It will be %s.", TO_TEXT);

	if (SHIFT_MINUTES < 0)
		memcpy(REPLY, "It was", 6),
			memmove(REPLY + 6, REPLY + 10, strlen(REPLY + 10) + 1);

	/* "3:15 pm + 2 hours = 5:15 pm, and then 5:15 pm + 50 minutes = 6:05
	 * pm" */
	{
		int	SHIFT = (int)floor(fabs(SHIFT_MINUTES) + 0.5);
		int	SIGN;

		if (SHIFT_MINUTES < 0)
			SIGN = -1;
		else
			SIGN = 1;

		int		HOURS = SHIFT / 60;
		int		MINUTES = SHIFT % 60;
		char	MIDDLE[32];

		if (HOURS && MINUTES)
		{
			SAY_CLOCK(
				CLOCKS[0] + SIGN * HOURS * 60, HALVES[0] != 0, MIDDLE,
				sizeof MIDDLE
			);
			ADD_WHY(
				REPLY, REPLY_SIZE,
				"%s %c %d hour%s = %s, and then %s %c %d "
				"minute%s = %s.",
				FROM_TEXT, SIGN < 0 ? '-' : '+', HOURS, HOURS == 1 ? "" : "s",
				MIDDLE, MIDDLE, SIGN < 0 ? '-' : '+', MINUTES,
				MINUTES == 1 ? "" : "s", TO_TEXT
			);
		}
		else if (SHIFT)
			ADD_WHY(
				REPLY, REPLY_SIZE, "%s %c %d %s%s = %s.", FROM_TEXT,
				SIGN < 0 ? '-' : '+', HOURS ? HOURS : MINUTES,
				HOURS ? "hour" : "minute",
				(HOURS ? HOURS : MINUTES) == 1 ? "" : "s", TO_TEXT
			);
	}

	snprintf(
		ACTION, ACTION_SIZE, "clock %s %+.0f minutes -> %s", FROM_TEXT,
		SHIFT_MINUTES, TO_TEXT
	);

	return (1);
}

/* ---------- fractions, halves and doubles ---------- */

static long
	GREATEST_DIVISOR(long FIRST, long SECOND)
{
	FIRST = labs(FIRST);
	SECOND = labs(SECOND);

	while (SECOND)
	{
		long	REST = FIRST % SECOND;

		FIRST = SECOND;
		SECOND = REST;
	}

	if (FIRST)
		return (FIRST);

	return (1);
}

static void
	SAY_FRACTION(long TOP, long BOTTOM, char *OUTPUT, int OUTPUT_SIZE)
{
	long	DIVISOR = GREATEST_DIVISOR(TOP, BOTTOM);

	TOP /= DIVISOR;
	BOTTOM /= DIVISOR;

	if (BOTTOM < 0)
	{
		TOP = -TOP;
		BOTTOM = -BOTTOM;
	}

	if (BOTTOM == 1)
		snprintf(OUTPUT, OUTPUT_SIZE, "%ld", TOP);
	else
		snprintf(OUTPUT, OUTPUT_SIZE, "%ld/%ld", TOP, BOTTOM);
}

static int
	ANSWER_FRACTIONS(
		const READING *READ, char *ACTION, int ACTION_SIZE, char *REPLY,
		int REPLY_SIZE
	)
{
	int		INDEX;
	int		FRACTIONS = 0;
	long	TOP = 0;
	long	BOTTOM = 1;
	char	OPERATOR = '+';
	int		TERMS = 0;
	char	EXPRESSION[200] = "";
	int		POSITION = 0;
	char	RESULT_TEXT[64];
	long	KEPT_TOP[2] = { 0, 0 };
	long	KEPT_BOTTOM[2] = { 1, 1 };

	/* "1/4 of 100", "3/4 of 20" */
	for (INDEX = 0; INDEX + 2 < READ->COUNT; INDEX++)
		if (
			READ->TOKENS[INDEX].IS_FRACTION &&
			!strcmp(READ->TOKENS[INDEX + 1].TEXT, "of") &&
			IS_NUMBER(&READ->TOKENS[INDEX + 2]) &&
			!READ->TOKENS[INDEX + 2].IS_FRACTION
		)
		{
			char	WHOLE[64];

			SAY_NUMBER(
				READ->TOKENS[INDEX].VALUE * READ->TOKENS[INDEX + 2].VALUE,
				RESULT_TEXT, sizeof RESULT_TEXT
			);
			SAY_NUMBER(READ->TOKENS[INDEX + 2].VALUE, WHOLE, sizeof WHOLE);
			snprintf(
				REPLY, REPLY_SIZE, "%s of %s is %s.",
				READ->TOKENS[INDEX].ORIGINAL, WHOLE, RESULT_TEXT
			);
			snprintf(
				ACTION, ACTION_SIZE, "calc %s * %s -> %s",
				READ->TOKENS[INDEX].ORIGINAL, WHOLE, RESULT_TEXT
			);
			return (1);
		}

	/* "half of 50", "a third of 90", "double 21", "twice 7", "triple 5" */
	{
		static const char	*PARTS[8][2] = {
			{ " half of ", "2" }, { " a third of ", "3" },
			{ " one third of ", "3" }, { " a quarter of ", "4" },
			{ " one quarter of ", "4" }, { " a fourth of ", "4" },
			{ " a fifth of ", "5" }, { NULL, NULL }
		};
		static const char	*MANY[5][2] = {
			{ " double ", "2" }, { " twice ", "2" }, { " triple ", "3" },
			{ " three times ", "3" }, { NULL, NULL }
		};
		int					PART;

		for (PART = 0; PARTS[PART][0]; PART++)
			if (HAS(READ, PARTS[PART][0]))
			{
				const char	*AT = strstr(READ->LOWER, PARTS[PART][0]) +
					strlen(PARTS[PART][0]);
				double		VALUE;
				char		*END;
				char		VALUE_TEXT[64];

				VALUE = strtod(AT, &END);

				if (END == AT || (*END && *END != ' '))
					continue ;

				SAY_NUMBER(
					VALUE / atoi(PARTS[PART][1]), RESULT_TEXT,
					sizeof RESULT_TEXT
				);
				SAY_NUMBER(VALUE, VALUE_TEXT, sizeof VALUE_TEXT);
				snprintf(
					REPLY, REPLY_SIZE, "%s of %s is %s.",
					PART == 0 ? "Half"
						: PART <= 2 ? "A third"
						: PART <= 5 ? "A quarter"
									: "A fifth",
					VALUE_TEXT, RESULT_TEXT
				);
				snprintf(
					ACTION, ACTION_SIZE, "calc %s / %s -> %s", VALUE_TEXT,
					PARTS[PART][1], RESULT_TEXT
				);
				return (1);
			}

		for (PART = 0; MANY[PART][0]; PART++)
			if (
				HAS(READ, MANY[PART][0]) &&
				(
					HAS(READ, " what is ") ||
					HAS(READ, " what's ") ||
					HAS(READ, " how much is ")
				)
			)
			{
				const char	*AT =
					strstr(READ->LOWER, MANY[PART][0]) + strlen(MANY[PART][0]);
				double		VALUE;
				char		*END;
				char		VALUE_TEXT[64];

				VALUE = strtod(AT, &END);

				if (END == AT || (*END && *END != ' '))
					continue ;

				SAY_NUMBER(
					VALUE * atoi(MANY[PART][1]), RESULT_TEXT, sizeof RESULT_TEXT
				);
				SAY_NUMBER(VALUE, VALUE_TEXT, sizeof VALUE_TEXT);
				snprintf(
					REPLY, REPLY_SIZE, "%s x %s = %s.", VALUE_TEXT,
					MANY[PART][1], RESULT_TEXT
				);
				snprintf(
					ACTION, ACTION_SIZE, "calc %s * %s -> %s", VALUE_TEXT,
					MANY[PART][1], RESULT_TEXT
				);
				return (1);
			}
	}

	/* "1/2 + 1/3": exact */
	for (INDEX = 0; INDEX < READ->COUNT; INDEX++)
		if (READ->TOKENS[INDEX].IS_FRACTION)
			FRACTIONS++;

	if (!FRACTIONS)
		return (0);

	/* "26 + 32/42 * 2" is the calculator's: only sums of fractions here */
	{
		int	ADDS = 0;
		int	TIMES = 0;

		for (INDEX = 0; INDEX < READ->COUNT; INDEX++)
		{
			const TOKEN	*ITEM = &READ->TOKENS[INDEX];

			if (IS_NUMBER(ITEM) && !ITEM->IS_FRACTION)
				return (0);

			if (ITEM->KIND == TOKEN_SYMBOL && strchr("+-", ITEM->TEXT[0]))
				ADDS++;

			if (
				(ITEM->KIND == TOKEN_SYMBOL && strchr("*x/", ITEM->TEXT[0])) ||
				!strcmp(ITEM->TEXT, "times")
			)
				TIMES++;

			if (!strcmp(ITEM->TEXT, "plus") || !strcmp(ITEM->TEXT, "minus"))
				ADDS++;
		}

		if (ADDS && TIMES)
			return (0);
	}

	for (INDEX = 0; INDEX < READ->COUNT; INDEX++)
	{
		const TOKEN	*ITEM = &READ->TOKENS[INDEX];
		long		ITEM_TOP;
		long		ITEM_BOTTOM;

		if (TERMS < 2 && ITEM->KIND == TOKEN_NUMBER && ITEM->HOUR < 0)
		{
			if (ITEM->IS_FRACTION)
				KEPT_TOP[TERMS] = ITEM->NUMERATOR;
			else
				KEPT_TOP[TERMS] = (long)ITEM->VALUE;

			if (ITEM->IS_FRACTION)
				KEPT_BOTTOM[TERMS] = ITEM->DENOMINATOR;
			else
				KEPT_BOTTOM[TERMS] = 1;
		}

		if (
			ITEM->KIND == TOKEN_SYMBOL &&
			strchr("+-*x", ITEM->TEXT[0]) &&
			TERMS
		)
		{
			OPERATOR = ITEM->TEXT[0] == 'x' ? '*' : ITEM->TEXT[0];
			POSITION += snprintf(
				EXPRESSION + POSITION, sizeof EXPRESSION - POSITION, " %c ",
				ITEM->TEXT[0]
			);
			continue ;
		}

		if (
			ITEM->KIND == TOKEN_WORD &&
			(
				!strcmp(ITEM->TEXT, "plus") ||
				!strcmp(ITEM->TEXT, "minus") ||
				!strcmp(ITEM->TEXT, "times")
			) &&
			TERMS
		)
		{
			if (ITEM->TEXT[0] == 'p')
				OPERATOR = '+';
			else if (ITEM->TEXT[0] == 'm')
				OPERATOR = '-';
			else
				OPERATOR = '*';

			POSITION += snprintf(
				EXPRESSION + POSITION, sizeof EXPRESSION - POSITION, " %c ",
				OPERATOR
			);
			continue ;
		}

		if (ITEM->KIND != TOKEN_NUMBER || ITEM->HOUR >= 0)
			continue ;

		if (ITEM->IS_FRACTION)
		{
			ITEM_TOP = ITEM->NUMERATOR;
			ITEM_BOTTOM = ITEM->DENOMINATOR;
		}
		else if (ITEM->VALUE == floor(ITEM->VALUE) && fabs(ITEM->VALUE) < 1e9)
		{
			ITEM_TOP = (long)ITEM->VALUE;
			ITEM_BOTTOM = 1;
		}
		else
			return (0);

		if (!ITEM_BOTTOM)
			return (0);

		if (!TERMS)
		{
			TOP = ITEM_TOP;
			BOTTOM = ITEM_BOTTOM;
		}
		else if (OPERATOR == '+')
		{
			TOP = TOP * ITEM_BOTTOM + ITEM_TOP * BOTTOM;
			BOTTOM *= ITEM_BOTTOM;
		}
		else if (OPERATOR == '-')
		{
			TOP = TOP * ITEM_BOTTOM - ITEM_TOP * BOTTOM;
			BOTTOM *= ITEM_BOTTOM;
		}
		else
		{
			TOP *= ITEM_TOP;
			BOTTOM *= ITEM_BOTTOM;
		}

		{
			long	DIVISOR = GREATEST_DIVISOR(TOP, BOTTOM);

			TOP /= DIVISOR;
			BOTTOM /= DIVISOR;
		}

		POSITION += snprintf(
			EXPRESSION + POSITION, sizeof EXPRESSION - POSITION, "%s",
			ITEM->ORIGINAL
		);
		TERMS++;
	}

	if (TERMS < 2)
		return (0);

	SAY_FRACTION(TOP, BOTTOM, RESULT_TEXT, sizeof RESULT_TEXT);

	if (BOTTOM != 1 && GREATEST_DIVISOR(TOP, BOTTOM) != labs(BOTTOM))
	{
		char	DECIMAL[64];

		SAY_NUMBER((double)TOP / (double)BOTTOM, DECIMAL, sizeof DECIMAL);
		snprintf(
			REPLY, REPLY_SIZE, "%s = %s (about %s).", EXPRESSION, RESULT_TEXT,
			DECIMAL
		);
	}
	else
		snprintf(REPLY, REPLY_SIZE, "%s = %s.", EXPRESSION, RESULT_TEXT);

	/* "2/3 = 8/12 and 1/4 = 3/12, so 8/12 + 3/12 = 11/12" */
	if (TERMS == 2 && KEPT_BOTTOM[0] > 0 && KEPT_BOTTOM[1] > 0)
	{
		long	COMMON = KEPT_BOTTOM[0] /
			GREATEST_DIVISOR(KEPT_BOTTOM[0], KEPT_BOTTOM[1]) * KEPT_BOTTOM[1];
		long	FIRST_TOP = KEPT_TOP[0] * (COMMON / KEPT_BOTTOM[0]);
		long	SECOND_TOP = KEPT_TOP[1] * (COMMON / KEPT_BOTTOM[1]);
		long	RAW;

		if (OPERATOR == '+')
			RAW = FIRST_TOP + SECOND_TOP;
		else
			RAW = FIRST_TOP - SECOND_TOP;

		if (
			(OPERATOR == '+' || OPERATOR == '-') &&
			KEPT_BOTTOM[0] != KEPT_BOTTOM[1]
		)
			ADD_WHY(
				REPLY, REPLY_SIZE,
				"Give them the same bottom number, %ld: "
				"%ld/%ld = %ld/%ld and %ld/%ld = %ld/%ld. Then %ld/%ld %c "
				"%ld/%ld "
				"= %ld/%ld%s%s.",
				COMMON, KEPT_TOP[0], KEPT_BOTTOM[0], FIRST_TOP, COMMON,
				KEPT_TOP[1], KEPT_BOTTOM[1], SECOND_TOP, COMMON, FIRST_TOP,
				COMMON, OPERATOR, SECOND_TOP, COMMON, RAW, COMMON,
				RAW * BOTTOM != TOP * COMMON || BOTTOM == COMMON
					? ""
					: ", which is ",
				RAW * BOTTOM != TOP * COMMON || BOTTOM == COMMON ? ""
				: RESULT_TEXT
			);
		else if (OPERATOR == '+' || OPERATOR == '-')
			ADD_WHY(
				REPLY, REPLY_SIZE,
				"They have the same bottom number, so "
				"%s the tops: %ld %c %ld = %ld, which gives %ld/%ld%s%s.",
				OPERATOR == '+' ? "add" : "subtract", KEPT_TOP[0], OPERATOR,
				KEPT_TOP[1], RAW, RAW, COMMON, BOTTOM == COMMON ? "" : ", or ",
				BOTTOM == COMMON ? "" : RESULT_TEXT
			);
		else if (OPERATOR == '*')
			ADD_WHY(
				REPLY, REPLY_SIZE,
				"Multiply the tops and the bottoms: %ld "
				"* %ld = %ld and %ld * %ld = %ld, so %ld/%ld%s%s.",
				KEPT_TOP[0], KEPT_TOP[1], KEPT_TOP[0] * KEPT_TOP[1],
				KEPT_BOTTOM[0], KEPT_BOTTOM[1], KEPT_BOTTOM[0] * KEPT_BOTTOM[1],
				KEPT_TOP[0] * KEPT_TOP[1], KEPT_BOTTOM[0] * KEPT_BOTTOM[1],
				KEPT_BOTTOM[0] * KEPT_BOTTOM[1] == BOTTOM ? "" : ", which is ",
				KEPT_BOTTOM[0] * KEPT_BOTTOM[1] == BOTTOM ? "" : RESULT_TEXT
			);
	}

	snprintf(
		ACTION, ACTION_SIZE, "fractions %s -> %s", EXPRESSION, RESULT_TEXT
	);

	return (1);
}

/* ---------- rates and ages ---------- */

static int
	ANSWER_RATES(
		const READING *READ, char *ACTION, int ACTION_SIZE, char *REPLY,
		int REPLY_SIZE
	)
{
	int		INDEX;
	double	RATE = 0;
	char	RATE_UNIT[64] = "";
	char	TIME_UNIT[64] = "";
	int		HAS_RATE = 0;
	char	TEXT[64];

	/* "a train travels 120 miles in 2 hours. What is its speed?" */
	if (
		HAS(READ, " speed ") ||
		HAS(READ, " how fast ") ||
		HAS(READ, " per hour ") ||
		HAS(READ, " an hour ") ||
		HAS(READ, " per minute ") ||
		HAS(READ, " per day ") ||
		HAS(READ, " each hour ") ||
		HAS(READ, " rate ")
	)
		for (INDEX = 0; INDEX + 4 < READ->COUNT; INDEX++)
			if (
				IS_NUMBER(&READ->TOKENS[INDEX]) &&
				READ->TOKENS[INDEX + 1].KIND == TOKEN_WORD &&
				(
					FIND_UNIT(READ->TOKENS[INDEX + 1].TEXT) < 0 ||
					UNITS[FIND_UNIT(READ->TOKENS[INDEX + 1].TEXT)].KIND != 1
				) &&
				!strcmp(READ->TOKENS[INDEX + 2].TEXT, "in") &&
				IS_NUMBER(&READ->TOKENS[INDEX + 3]) &&
				READ->TOKENS[INDEX + 3].VALUE > 0 &&
				FIND_UNIT(READ->TOKENS[INDEX + 4].TEXT) >= 0 &&
				UNITS[FIND_UNIT(READ->TOKENS[INDEX + 4].TEXT)].KIND == 1
			)
			{
				char	AMOUNT[64];
				char	SPAN[64];
				char	TIME_ONE[32] = "";
				char	TIME_MANY[32] = "";

				sscanf(
					UNITS[FIND_UNIT(READ->TOKENS[INDEX + 4].TEXT)].NAMES,
					"%31s %31s", TIME_ONE, TIME_MANY
				);
				SAY_NUMBER(
					READ->TOKENS[INDEX].VALUE / READ->TOKENS[INDEX + 3].VALUE,
					TEXT, sizeof TEXT
				);
				SAY_NUMBER(READ->TOKENS[INDEX].VALUE, AMOUNT, sizeof AMOUNT);
				SAY_NUMBER(READ->TOKENS[INDEX + 3].VALUE, SPAN, sizeof SPAN);
				snprintf(
					REPLY, REPLY_SIZE, "%s %s per %s: %s %s / %s %s = %s.",
					TEXT, READ->TOKENS[INDEX + 1].TEXT, TIME_ONE, AMOUNT,
					READ->TOKENS[INDEX + 1].TEXT, SPAN,
					READ->TOKENS[INDEX + 4].TEXT, TEXT
				);
				ADD_WHY(
					REPLY, REPLY_SIZE,
					"Speed is how far it goes divided by "
					"how long it takes: %s %s / %s %s = %s %s per %s.",
					AMOUNT, READ->TOKENS[INDEX + 1].TEXT, SPAN,
					READ->TOKENS[INDEX + 4].TEXT, TEXT,
					READ->TOKENS[INDEX + 1].TEXT, TIME_ONE
				);
				snprintf(
					ACTION, ACTION_SIZE, "calc %s / %s -> %s", AMOUNT, SPAN,
					TEXT
				);
				return (1);
			}

	/* "60 km per hour", "50 miles an hour", "5 toys every hour" */
	for (INDEX = 0; INDEX + 3 < READ->COUNT; INDEX++)
		if (
			IS_NUMBER(&READ->TOKENS[INDEX]) &&
			READ->TOKENS[INDEX + 1].KIND == TOKEN_WORD &&
			(
				!strcmp(READ->TOKENS[INDEX + 2].TEXT, "per") ||
				!strcmp(READ->TOKENS[INDEX + 2].TEXT, "an") ||
				!strcmp(READ->TOKENS[INDEX + 2].TEXT, "a") ||
				!strcmp(READ->TOKENS[INDEX + 2].TEXT, "every") ||
				!strcmp(READ->TOKENS[INDEX + 2].TEXT, "each")
			) &&
			FIND_UNIT(READ->TOKENS[INDEX + 3].TEXT) >= 0 &&
			UNITS[FIND_UNIT(READ->TOKENS[INDEX + 3].TEXT)].KIND == 1
		)
		{
			RATE = READ->TOKENS[INDEX].VALUE;
			snprintf(
				RATE_UNIT, sizeof RATE_UNIT, "%s", READ->TOKENS[INDEX + 1].TEXT
			);
			snprintf(
				TIME_UNIT, sizeof TIME_UNIT, "%s", READ->TOKENS[INDEX + 3].TEXT
			);
			HAS_RATE = 1;
			break ;
		}

	if (!HAS_RATE || RATE <= 0)
		return (0);

	/* "how far does it go in 3 hours?", "how many toys in 8 hours?" */
	if (
		HAS(READ, " how far ") ||
		HAS(READ, " how many ") ||
		HAS(READ, " how much ")
	)
	{
		int	SCAN;

		for (SCAN = INDEX + 4; SCAN + 1 < READ->COUNT; SCAN++)
			if (
				IS_NUMBER(&READ->TOKENS[SCAN]) &&
				FIND_UNIT(READ->TOKENS[SCAN + 1].TEXT) >= 0 &&
				UNITS[FIND_UNIT(READ->TOKENS[SCAN + 1].TEXT)].KIND == 1
			)
			{
				double	SPAN = READ->TOKENS[SCAN].VALUE *
					UNITS[FIND_UNIT(READ->TOKENS[SCAN + 1].TEXT)].SIZE /
					UNITS[FIND_UNIT(TIME_UNIT)].SIZE;
				char	SPAN_TEXT[64];

				SAY_NUMBER(RATE * SPAN, TEXT, sizeof TEXT);
				SAY_NUMBER(
					READ->TOKENS[SCAN].VALUE, SPAN_TEXT, sizeof SPAN_TEXT
				);
				snprintf(
					REPLY, REPLY_SIZE, "%s %s: %s %s per %s for %s %s.", TEXT,
					RATE_UNIT, READ->TOKENS[INDEX].ORIGINAL, RATE_UNIT,
					TIME_UNIT, SPAN_TEXT, READ->TOKENS[SCAN + 1].TEXT
				);
				snprintf(
					ACTION, ACTION_SIZE, "calc %s * %s -> %s",
					READ->TOKENS[INDEX].ORIGINAL, SPAN_TEXT, TEXT
				);
				return (1);
			}

		return (0);
	}

	/* "how long does it take to go 120 km?" */
	if (HAS(READ, " how long "))
	{
		int	SCAN;

		for (SCAN = 0; SCAN + 1 < READ->COUNT; SCAN++)
			if (
				SCAN != INDEX &&
				IS_NUMBER(&READ->TOKENS[SCAN]) &&
				!strcmp(READ->TOKENS[SCAN + 1].TEXT, RATE_UNIT)
			)
			{
				char	DISTANCE[64];

				SAY_NUMBER(READ->TOKENS[SCAN].VALUE / RATE, TEXT, sizeof TEXT);
				SAY_NUMBER(READ->TOKENS[SCAN].VALUE, DISTANCE, sizeof DISTANCE);
				snprintf(
					REPLY, REPLY_SIZE, "%s %s%s: %s %s at %s %s per %s.", TEXT,
					TIME_UNIT[strlen(TIME_UNIT) - 1] == 's' ? "" : TIME_UNIT,
					TIME_UNIT[strlen(TIME_UNIT) - 1] == 's'
						? TIME_UNIT
						: (READ->TOKENS[SCAN].VALUE / RATE == 1 ? "" : "s"),
					DISTANCE, RATE_UNIT, READ->TOKENS[INDEX].ORIGINAL,
					RATE_UNIT, TIME_UNIT
				);
				snprintf(
					ACTION, ACTION_SIZE, "calc %s / %s -> %s", DISTANCE,
					READ->TOKENS[INDEX].ORIGINAL, TEXT
				);
				return (1);
			}
	}

	return (0);
}

/* ---------- "Tom is 5 years older than Jim. Jim is 10. How old is Tom?" -- */

#define MAX_PEOPLE 12

typedef struct
{
	char	KEY[64];
	char	SHOWN[64];
	double	AGE;
	int		KNOWN;
	char	WHY[200];
	char	MATH[96];
} AGE_PERSON;

typedef struct
{
	AGE_PERSON	PEOPLE[MAX_PEOPLE];
	int			COUNT;
} AGE_BOOK;

static const char	*OWNER_WORDS[8] = {
	"my", "his", "her", "their", "our", "your", "the", NULL
};

/* the person whose words end just before INDEX ("my brother", "Tom",
 * "Tom's sister"), from START; -1 when there is none */
static int
	PERSON_BEFORE(
		const READING *READ, int INDEX, char *KEY, int KEY_SIZE, char *SHOWN,
		int SHOWN_SIZE
	)
{
	int	START = INDEX - 1;

	/* "I am 25", "how old will I be" */
	if (START >= 0 && !strcmp(READ->TOKENS[START].TEXT, "i"))
	{
		snprintf(KEY, KEY_SIZE, "i");
		snprintf(SHOWN, SHOWN_SIZE, "I");
		return (START);
	}

	if (
		START < 0 ||
		READ->TOKENS[START].KIND != TOKEN_WORD ||
		IN_LIST(READ->TOKENS[START].TEXT, NOT_A_NAME)
	)
		return (-1);

	if (
		START > 0 &&
		(
			IN_LIST(READ->TOKENS[START - 1].TEXT, OWNER_WORDS) ||
			(
				strlen(READ->TOKENS[START - 1].TEXT) > 2 &&
				!strcmp(
					READ->TOKENS[START - 1].TEXT +
						strlen(READ->TOKENS[START - 1].TEXT) - 2,
					"'s"
				)
			)
		)
	)
		START--;

	SPAN_TEXT(READ, START, INDEX, SHOWN, SHOWN_SIZE);
	snprintf(KEY, KEY_SIZE, "%s", SHOWN);

	{
		char	*CURSOR;

		for (CURSOR = KEY; *CURSOR; CURSOR++)
			*CURSOR = (char)tolower((uint8_t)*CURSOR);
	}

	return (START);
}

/* the person whose words start at INDEX ("than my brother", "than Jim") */
static int
	PERSON_AT(
		const READING *READ, int INDEX, char *KEY, int KEY_SIZE, char *SHOWN,
		int SHOWN_SIZE
	)
{
	int	END = INDEX;

	if (END < READ->COUNT && !strcmp(READ->TOKENS[END].TEXT, "i"))
	{
		snprintf(KEY, KEY_SIZE, "i");
		snprintf(SHOWN, SHOWN_SIZE, "I");
		return (END + 1);
	}

	if (
		END < READ->COUNT &&
		(
			IN_LIST(READ->TOKENS[END].TEXT, OWNER_WORDS) ||
			(
				strlen(READ->TOKENS[END].TEXT) > 2 &&
				!strcmp(
					READ->TOKENS[END].TEXT + strlen(READ->TOKENS[END].TEXT) - 2,
					"'s"
				)
			)
		)
	)
		END++;

	if (
		END >= READ->COUNT ||
		READ->TOKENS[END].KIND != TOKEN_WORD ||
		IN_LIST(READ->TOKENS[END].TEXT, NOT_A_NAME)
	)
		return (-1);

	if (PERSON_BEFORE(READ, END + 1, KEY, KEY_SIZE, SHOWN, SHOWN_SIZE) >= 0)
		return (END + 1);

	return (-1);
}

static int
	FIND_PERSON(AGE_BOOK *BOOK, const char *KEY, const char *SHOWN)
{
	int	INDEX;

	for (INDEX = 0; INDEX < BOOK->COUNT; INDEX++)
		if (!strcmp(BOOK->PEOPLE[INDEX].KEY, KEY))
			return (INDEX);

	if (BOOK->COUNT >= MAX_PEOPLE)
		return (-1);

	memset(&BOOK->PEOPLE[BOOK->COUNT], 0, sizeof BOOK->PEOPLE[0]);
	snprintf(BOOK->PEOPLE[BOOK->COUNT].KEY, 64, "%s", KEY);
	snprintf(BOOK->PEOPLE[BOOK->COUNT].SHOWN, 64, "%s", SHOWN);

	return (BOOK->COUNT++);
}

/* "my brother" said back: "your brother"; "Tom" stays "Tom" */
static void
	SAY_PERSON(const char *SHOWN, int AT_START, char *OUTPUT, int SIZE)
{
	if (!strcmp(SHOWN, "I"))
		snprintf(OUTPUT, SIZE, "%s", AT_START ? "You" : "you");
	else if (!strncasecmp(SHOWN, "my ", 3))
		snprintf(OUTPUT, SIZE, "%s %s", AT_START ? "Your" : "your", SHOWN + 3);
	else if (!strncasecmp(SHOWN, "our ", 4))
		snprintf(OUTPUT, SIZE, "%s %s", AT_START ? "Your" : "your", SHOWN + 4);
	else
	{
		snprintf(OUTPUT, SIZE, "%s", SHOWN);

		if (AT_START)
			OUTPUT[0] = (char)toupper((uint8_t)OUTPUT[0]);
	}
}

typedef struct
{
	int		FROM;
	int		TO;
	double	DIFFERENCE;
	double	FACTOR;
} AGE_LINK;

static int
	ANSWER_AGES(
		const READING *READ, char *ACTION, int ACTION_SIZE, char *REPLY,
		int REPLY_SIZE
	)
{
	static AGE_BOOK	BOOK;
	AGE_LINK		LINKS[16];
	int				LINK_COUNT = 0;
	int				INDEX;
	int				LAST_PERSON = -1;
	int				ASKED = -1;
	double			ASKED_SHIFT = 0;
	int				ROUND;
	char			KEY[64];
	char			SHOWN[64];
	char			TEXT[64];

	if (
		!HAS(READ, " how old ") &&
		!HAS(READ, " what age ") &&
		!HAS(READ, " age ")
	)
		return (0);

	memset(&BOOK, 0, sizeof BOOK);

	for (INDEX = 0; INDEX < READ->COUNT; INDEX++)
	{
		const char	*WORD = READ->TOKENS[INDEX].TEXT;
		int			PERSON;

		/* the question: "how old is Tom", "how old will Tom be in 5 years" */
		if (
			!strcmp(WORD, "old") &&
			INDEX > 0 &&
			!strcmp(READ->TOKENS[INDEX - 1].TEXT, "how")
		)
		{
			int	AT = INDEX + 1;
			int	END;

			if (
				AT < READ->COUNT &&
				(
					!strcmp(TEXT_AT(READ, AT), "is") ||
					!strcmp(TEXT_AT(READ, AT), "was") ||
					!strcmp(TEXT_AT(READ, AT), "will") ||
					!strcmp(TEXT_AT(READ, AT), "are")
				)
			)
				AT++;

			END = PERSON_AT(READ, AT, KEY, sizeof KEY, SHOWN, sizeof SHOWN);

			/* "how old was he": the one the story was about */
			if (
				END < 0 &&
				(
					!strcmp(TEXT_AT(READ, AT), "he") ||
					!strcmp(TEXT_AT(READ, AT), "she") ||
					!strcmp(TEXT_AT(READ, AT), "they")
				) &&
				LAST_PERSON >= 0
			)
			{
				ASKED = LAST_PERSON;
				END = AT + 1;
			}
			else if (END < 0)
				continue ;
			else
				ASKED = FIND_PERSON(&BOOK, KEY, SHOWN);

			/* "be in 5 years", "5 years ago" */
			for (AT = END; AT + 1 < READ->COUNT; AT++)
				if (
					IS_NUMBER(&READ->TOKENS[AT]) &&
					!strncmp(TEXT_AT(READ, AT + 1), "year", 4)
				)
				{
					ASKED_SHIFT = READ->TOKENS[AT].VALUE;

					if (
						!strcmp(TEXT_AT(READ, AT + 2), "ago") ||
						!strcmp(TEXT_AT(READ, INDEX + 1), "was")
					)
						ASKED_SHIFT = -ASKED_SHIFT;

					break ;
				}

			continue ;
		}

		if (
			strcmp(WORD, "is") &&
			strcmp(WORD, "was") &&
			strcmp(WORD, "will") &&
			strcmp(WORD, "turns") &&
			strcmp(WORD, "turned") &&
			strcmp(WORD, "am")
		)
			continue ;

		if (
			PERSON_BEFORE(READ, INDEX, KEY, sizeof KEY, SHOWN, sizeof SHOWN) < 0
		)
			continue ;

		PERSON = FIND_PERSON(&BOOK, KEY, SHOWN);

		if (PERSON < 0)
			continue ;

		{
			int		AT = INDEX + 1;
			double	SHIFT = 0;

			if (!strcmp(WORD, "will") && !strcmp(TEXT_AT(READ, AT), "be"))
				AT++;

			/* "her brother is twice her age", "Tom is twice as old as Ann",
			 * "Ann is half his age", "Bo is 3 times as old as Al" */
			{
				double	FACTOR = 0;
				int		NEXT = AT;
				int		OTHER = -1;

				if (!strcmp(TEXT_AT(READ, NEXT), "twice"))
				{
					FACTOR = 2;
					NEXT++;
				}
				else if (!strcmp(TEXT_AT(READ, NEXT), "half"))
				{
					FACTOR = 0.5;
					NEXT++;
				}
				else if (
					NEXT + 1 < READ->COUNT &&
					IS_NUMBER(&READ->TOKENS[NEXT]) &&
					!strcmp(TEXT_AT(READ, NEXT + 1), "times")
				)
				{
					FACTOR = READ->TOKENS[NEXT].VALUE;
					NEXT += 2;
				}

				if (FACTOR > 0)
				{
					if (
						!strcmp(TEXT_AT(READ, NEXT), "as") &&
						!strcmp(TEXT_AT(READ, NEXT + 1), "old") &&
						!strcmp(TEXT_AT(READ, NEXT + 2), "as") &&
						PERSON_AT(
							READ, NEXT + 3, KEY, sizeof KEY, SHOWN, sizeof SHOWN
						) >= 0
					)
						OTHER = FIND_PERSON(&BOOK, KEY, SHOWN);
					else if (
						(
							!strcmp(TEXT_AT(READ, NEXT), "her") ||
							!strcmp(TEXT_AT(READ, NEXT), "his") ||
							!strcmp(TEXT_AT(READ, NEXT), "their")
						) &&
						!strcmp(TEXT_AT(READ, NEXT + 1), "age")
					)
						OTHER = LAST_PERSON;
					else if (
						!strcmp(TEXT_AT(READ, NEXT + 1), "age") &&
						strlen(TEXT_AT(READ, NEXT)) > 2 &&
						!strcmp(
							TEXT_AT(READ, NEXT) + strlen(TEXT_AT(READ, NEXT)) -
								2,
							"'s"
						)
					)
					{
						char	OWNER[64];

						snprintf(
							OWNER, sizeof OWNER, "%.*s",
							(int)strlen(TEXT_AT(READ, NEXT)) - 2,
							TEXT_AT(READ, NEXT)
						);
						snprintf(
							SHOWN, sizeof SHOWN, "%.*s",
							(int)strlen(READ->TOKENS[NEXT].ORIGINAL) - 2,
							READ->TOKENS[NEXT].ORIGINAL
						);
						OTHER = FIND_PERSON(&BOOK, OWNER, SHOWN);
					}

					if (OTHER >= 0 && OTHER != PERSON && LINK_COUNT < 16)
					{
						LINKS[LINK_COUNT].FROM = PERSON;
						LINKS[LINK_COUNT].TO = OTHER;
						LINKS[LINK_COUNT].DIFFERENCE = 0;
						LINKS[LINK_COUNT].FACTOR = FACTOR;
						LINK_COUNT++;
					}

					LAST_PERSON = PERSON;
					continue ;
				}
			}

			if (
				!IS_NUMBER(&READ->TOKENS[AT < READ->COUNT ? AT : 0]) ||
				AT >= READ->COUNT
			)
				continue ;

			/* "5 years older than Jim", "3 years younger" */
			if (
				!strncmp(TEXT_AT(READ, AT + 1), "year", 4) &&
				(
					!strcmp(TEXT_AT(READ, AT + 2), "older") ||
					!strcmp(TEXT_AT(READ, AT + 2), "younger")
				)
			)
			{
				int	OTHER = -1;

				SHIFT = READ->TOKENS[AT].VALUE;

				if (TEXT_AT(READ, AT + 2)[0] == 'y')
					SHIFT = -SHIFT;

				if (
					!strcmp(TEXT_AT(READ, AT + 3), "than") &&
					PERSON_AT(
						READ, AT + 4, KEY, sizeof KEY, SHOWN, sizeof SHOWN
					) >= 0
				)
					OTHER = FIND_PERSON(&BOOK, KEY, SHOWN);
				else
					OTHER = LAST_PERSON;

				if (OTHER >= 0 && OTHER != PERSON && LINK_COUNT < 16)
				{
					LINKS[LINK_COUNT].FROM = PERSON;
					LINKS[LINK_COUNT].TO = OTHER;
					LINKS[LINK_COUNT].DIFFERENCE = SHIFT;
					LINKS[LINK_COUNT].FACTOR = 0;
					LINK_COUNT++;
				}

				LAST_PERSON = PERSON;
				continue ;
			}

			/* "Tom is 10", "Tom will be 12 in 2 years", "Tom was 8 3 years
			 * ago" */
			if (
				TEXT_AT(READ, AT + 1)[0] &&
				strcmp(TEXT_AT(READ, AT + 1), "years") &&
				strcmp(TEXT_AT(READ, AT + 1), "year") &&
				READ->TOKENS[AT + 1].KIND == TOKEN_WORD &&
				strcmp(TEXT_AT(READ, AT + 1), "in") &&
				strcmp(TEXT_AT(READ, AT + 1), "and") &&
				strcmp(TEXT_AT(READ, AT + 1), "so") &&
				strcmp(TEXT_AT(READ, AT + 1), "but") &&
				strcmp(TEXT_AT(READ, AT + 1), "today") &&
				strcmp(TEXT_AT(READ, AT + 1), "now")
			)
				continue ;

			{
				double	VALUE = READ->TOKENS[AT].VALUE;
				int		SCAN;

				for (
					SCAN = AT + 1;
					SCAN + 1 < READ->COUNT && SCAN < AT + 6;
					SCAN++
				)
					if (
						IS_NUMBER(&READ->TOKENS[SCAN]) &&
						!strncmp(TEXT_AT(READ, SCAN + 1), "year", 4)
					)
					{
						if (!strcmp(TEXT_AT(READ, SCAN + 2), "ago"))
							VALUE += READ->TOKENS[SCAN].VALUE;
						else if (!strcmp(WORD, "will"))
							VALUE -= READ->TOKENS[SCAN].VALUE;

						break ;
					}

				/* "in 3 years Bob will be 15", "2 years ago Amy was 8" */
				if (SCAN + 1 >= READ->COUNT || SCAN >= AT + 6)
					for (
						SCAN = INDEX - 2;
						SCAN >= 0 && SCAN >= INDEX - 7;
						SCAN--
					)
						if (
							IS_NUMBER(&READ->TOKENS[SCAN]) &&
							!strncmp(TEXT_AT(READ, SCAN + 1), "year", 4)
						)
						{
							if (!strcmp(TEXT_AT(READ, SCAN + 2), "ago"))
								VALUE += READ->TOKENS[SCAN].VALUE;
							else if (
								!strcmp(WORD, "will") &&
								!strcmp(TEXT_AT(READ, SCAN - 1), "in")
							)
								VALUE -= READ->TOKENS[SCAN].VALUE;

							break ;
						}

				BOOK.PEOPLE[PERSON].AGE = VALUE;
				BOOK.PEOPLE[PERSON].KNOWN = 1;
				LAST_PERSON = PERSON;
			}
		}
	}

	if (ASKED < 0)
		return (0);

	/* ages pass along the links until nothing new is learned */
	for (ROUND = 0; ROUND < 12; ROUND++)
	{
		int	CHANGED = 0;

		for (INDEX = 0; INDEX < LINK_COUNT; INDEX++)
		{
			AGE_PERSON	*FROM = &BOOK.PEOPLE[LINKS[INDEX].FROM];
			AGE_PERSON	*TO = &BOOK.PEOPLE[LINKS[INDEX].TO];
			char		AMOUNT[32];

			SAY_NUMBER(fabs(LINKS[INDEX].DIFFERENCE), AMOUNT, sizeof AMOUNT);

			/* "twice her age": a times link */
			if (LINKS[INDEX].FACTOR > 0)
			{
				char	OTHER[64];
				char	SELF[64];
				char	TIMES[64];

				SAY_NUMBER(LINKS[INDEX].FACTOR, TIMES, sizeof TIMES);

				if (TO->KNOWN && !FROM->KNOWN)
				{
					FROM->AGE = TO->AGE * LINKS[INDEX].FACTOR;
					FROM->KNOWN = 1;
					SAY_PERSON(TO->SHOWN, 0, OTHER, sizeof OTHER);
					SAY_NUMBER(TO->AGE, TEXT, sizeof TEXT);
					snprintf(
						FROM->WHY, sizeof FROM->WHY, "%s is %s, and %s %s = %g",
						OTHER, TEXT, TEXT,
						LINKS[INDEX].FACTOR == 0.5 ? "/ 2" : "x",
						LINKS[INDEX].FACTOR == 0.5 ? TO->AGE / 2 : FROM->AGE
					);

					if (LINKS[INDEX].FACTOR != 0.5)
						snprintf(
							FROM->WHY, sizeof FROM->WHY,
							"%s is %s, and %s x %s "
							"= %g",
							OTHER, TEXT, TEXT, TIMES, FROM->AGE
						);

					CHANGED = 1;
				}
				else if (FROM->KNOWN && !TO->KNOWN)
				{
					TO->AGE = FROM->AGE / LINKS[INDEX].FACTOR;
					TO->KNOWN = 1;
					SAY_PERSON(FROM->SHOWN, 0, SELF, sizeof SELF);
					SAY_NUMBER(FROM->AGE, TEXT, sizeof TEXT);
					snprintf(
						TO->WHY, sizeof TO->WHY, "%s is %s, and %s / %s = %g",
						SELF, TEXT, TEXT, TIMES, TO->AGE
					);
					CHANGED = 1;
				}

				continue ;
			}

			if (TO->KNOWN && !FROM->KNOWN)
			{
				char	OTHER[64];
				char	SELF[64];

				FROM->AGE = TO->AGE + LINKS[INDEX].DIFFERENCE;
				FROM->KNOWN = 1;
				SAY_PERSON(TO->SHOWN, 0, OTHER, sizeof OTHER);
				SAY_PERSON(FROM->SHOWN, 0, SELF, sizeof SELF);
				SAY_NUMBER(TO->AGE, TEXT, sizeof TEXT);

				{
					char	RESULT_TEXT[32];

					SAY_NUMBER(FROM->AGE, RESULT_TEXT, sizeof RESULT_TEXT);
					snprintf(
						FROM->MATH, sizeof FROM->MATH, "%s %c %s = %s", TEXT,
						LINKS[INDEX].DIFFERENCE > 0 ? '+' : '-', AMOUNT,
						RESULT_TEXT
					);
				}

				snprintf(
					FROM->WHY, sizeof FROM->WHY,
					"%s is %s, and %s is %s %s "
					"%s",
					OTHER, TEXT, SELF, AMOUNT,
					fabs(LINKS[INDEX].DIFFERENCE) == 1 ? "year" : "years",
					LINKS[INDEX].DIFFERENCE > 0 ? "older" : "younger"
				);
				CHANGED = 1;
			}
			else if (FROM->KNOWN && !TO->KNOWN)
			{
				char	OTHER[64];
				char	SELF[64];

				TO->AGE = FROM->AGE - LINKS[INDEX].DIFFERENCE;
				TO->KNOWN = 1;
				SAY_PERSON(FROM->SHOWN, 0, SELF, sizeof SELF);
				SAY_PERSON(TO->SHOWN, 0, OTHER, sizeof OTHER);
				SAY_NUMBER(FROM->AGE, TEXT, sizeof TEXT);

				{
					char	RESULT_TEXT[32];

					SAY_NUMBER(TO->AGE, RESULT_TEXT, sizeof RESULT_TEXT);
					snprintf(
						TO->MATH, sizeof TO->MATH, "%s %c %s = %s", TEXT,
						LINKS[INDEX].DIFFERENCE > 0 ? '-' : '+', AMOUNT,
						RESULT_TEXT
					);
				}

				snprintf(
					TO->WHY, sizeof TO->WHY, "%s is %s and %s %s %s than %s",
					SELF, TEXT, AMOUNT,
					fabs(LINKS[INDEX].DIFFERENCE) == 1 ? "year" : "years",
					LINKS[INDEX].DIFFERENCE > 0 ? "older" : "younger", OTHER
				);
				CHANGED = 1;
			}
		}

		if (!CHANGED)
			break ;
	}

	if (!BOOK.PEOPLE[ASKED].KNOWN)
		return (0);

	{
		AGE_PERSON	*WHO = &BOOK.PEOPLE[ASKED];
		char		NAME[64];
		double		AGE = WHO->AGE + ASKED_SHIFT;

		if (AGE < 0)
			return (0);

		SAY_PERSON(WHO->SHOWN, 1, NAME, sizeof NAME);
		SAY_NUMBER(AGE, TEXT, sizeof TEXT);

		if (ASKED_SHIFT > 0)
			snprintf(REPLY, REPLY_SIZE, "%s will be %s.", NAME, TEXT);
		else if (ASKED_SHIFT < 0)
			snprintf(REPLY, REPLY_SIZE, "%s was %s.", NAME, TEXT);
		else if (WHO->WHY[0])
			snprintf(REPLY, REPLY_SIZE, "%s is %s: %s.", NAME, TEXT, WHO->WHY);
		else
			snprintf(REPLY, REPLY_SIZE, "%s is %s.", NAME, TEXT);

		/* "Because Tom is 10, and Mary is 3 years older: 10 + 3 = 13." */
		if (WHO->WHY[0] && WHO->MATH[0] && !ASKED_SHIFT)
			ADD_WHY(REPLY, REPLY_SIZE, "Because %s: %s.", WHO->WHY, WHO->MATH);

		snprintf(
			ACTION, ACTION_SIZE, "age %s %+.0f -> %s", WHO->KEY, ASKED_SHIFT,
			TEXT
		);
		return (1);
	}
}

/* ---------- trick questions ---------- */

/* the mass unit at INDEX ("kilogram", "pound"), or -1 */
static int
	MASS_UNIT_AT(const READING *READ, int INDEX)
{
	int	UNIT;

	if (INDEX < 0 || INDEX >= READ->COUNT)
		return (-1);

	UNIT = FIND_UNIT(READ->TOKENS[INDEX].TEXT);

	if (UNIT >= 0 && UNITS[UNIT].KIND == 3)
		return (UNIT);

	return (-1);
}

static int
	ANSWER_TRICKS(
		const READING *READ, char *ACTION, int ACTION_SIZE, char *REPLY,
		int REPLY_SIZE
	)
{
	int	INDEX;

	/* "which is heavier, a kilogram of feathers or a kilogram of steel?" */
	if (
		HAS(READ, " heavier ") ||
		HAS(READ, " weighs more ") ||
		HAS(READ, " lighter ") ||
		HAS(READ, " weighs less ") ||
		HAS(READ, " weigh more ") ||
		HAS(READ, " weigh less ")
	)
	{
		double	AMOUNT[2];
		int		UNIT[2];
		char	THING[2][64];
		int		FOUND = 0;

		for (INDEX = 0; INDEX + 2 < READ->COUNT && FOUND < 2; INDEX++)
		{
			int		AT = INDEX;
			double	VALUE;

			if (IS_NUMBER(&READ->TOKENS[AT]))
				VALUE = READ->TOKENS[AT].VALUE;
			else if (
				!strcmp(TEXT_AT(READ, AT), "a") ||
				!strcmp(TEXT_AT(READ, AT), "an")
			)
				VALUE = 1;
			else
				continue ;

			if (
				MASS_UNIT_AT(READ, AT + 1) < 0 ||
				strcmp(TEXT_AT(READ, AT + 2), "of") ||
				!IS_WORD_TOKEN(READ, AT + 3)
			)
				continue ;

			AMOUNT[FOUND] = VALUE;
			UNIT[FOUND] = MASS_UNIT_AT(READ, AT + 1);
			snprintf(
				THING[FOUND], 64, "%s %s of %s", READ->TOKENS[AT].ORIGINAL,
				READ->TOKENS[AT + 1].ORIGINAL, READ->TOKENS[AT + 3].ORIGINAL
			);
			THING[FOUND][0] = (char)tolower((uint8_t)THING[FOUND][0]);
			FOUND++;
			INDEX = AT + 3;
		}

		if (FOUND == 2)
		{
			double	FIRST = AMOUNT[0] * UNITS[UNIT[0]].SIZE;
			double	SECOND = AMOUNT[1] * UNITS[UNIT[1]].SIZE;

			if (SAME(FIRST, SECOND))
			{
				snprintf(
					REPLY, REPLY_SIZE,
					"Neither: %s and %s weigh exactly the same. Only the stuff "
					"is "
					"different.",
					THING[0], THING[1]
				);
				snprintf(ACTION, ACTION_SIZE, "compare weights -> same");
				return (1);
			}

			{
				int	HEAVIER;

				if (FIRST > SECOND)
					HEAVIER = 0;
				else
					HEAVIER = 1;

				int	WANT_LIGHT = HAS(READ, " lighter ") || HAS(READ, " less ");
				int	WINNER;

				if (WANT_LIGHT)
					WINNER = 1 - HEAVIER;
				else
					WINNER = HEAVIER;

				snprintf(
					REPLY, REPLY_SIZE, "%c%s is %s.",
					toupper((uint8_t)THING[WINNER][0]), THING[WINNER] + 1,
					WANT_LIGHT ? "lighter" : "heavier"
				);
				snprintf(
					ACTION, ACTION_SIZE, "compare weights -> %s", THING[WINNER]
				);
				return (1);
			}
		}
	}

	/* "which month has 28 days?", "how many months have 28 days?" */
	if (
		HAS(READ, " month") &&
		(
			HAS(READ, " 28 days ") ||
			HAS(READ, " twenty eight days ") ||
			HAS(READ, " twenty-eight days ")
		)
	)
	{
		if (HAS(READ, " how many "))
			snprintf(
				REPLY, REPLY_SIZE,
				"All 12 of them: every month has at least 28 days. February is "
				"the only one with just 28 (29 in a leap year)."
			);
		else
			snprintf(
				REPLY, REPLY_SIZE,
				"Every month has at least 28 days. February is the only one "
				"with "
				"just 28 (29 in a leap year)."
			);

		snprintf(ACTION, ACTION_SIZE, "months with 28 days -> all");
		return (1);
	}

	/* "all but 8 die. How many are left?" */
	if (
		HAS(READ, " all but ") &&
		(
			HAS(READ, " left ") ||
			HAS(READ, " remain") ||
			HAS(READ, " alive ") ||
			HAS(READ, " survive")
		)
	)
	{
		const char	*AT = strstr(READ->LOWER, " all but ") + 9;
		double		VALUE;
		char		*END;
		char		VALUE_TEXT[64];
		char		NOUN[64] = "";

		VALUE = strtod(AT, &END);

		if (END != AT)
		{
			/* "has 15 sheep": the noun after the first number */
			for (INDEX = 0; INDEX + 1 < READ->COUNT; INDEX++)
				if (
					IS_NUMBER(&READ->TOKENS[INDEX]) &&
					READ->TOKENS[INDEX].VALUE != VALUE &&
					IS_WORD_TOKEN(READ, INDEX + 1)
				)
				{
					snprintf(
						NOUN, sizeof NOUN, " %s",
						READ->TOKENS[INDEX + 1].ORIGINAL
					);
					break ;
				}

			SAY_NUMBER(VALUE, VALUE_TEXT, sizeof VALUE_TEXT);
			snprintf(
				REPLY, REPLY_SIZE,
				"%s%s %s left: \"all but %s\" means all except %s.", VALUE_TEXT,
				NOUN, VALUE == 1 ? "is" : "are", VALUE_TEXT, VALUE_TEXT
			);
			ADD_WHY(
				REPLY, REPLY_SIZE,
				"\"All but %s\" means every one of them "
				"except %s. So %s%s %s still there, whatever the number at the "
				"start was. It is a trick question: it makes you want to "
				"subtract.",
				VALUE_TEXT, VALUE_TEXT, VALUE_TEXT, NOUN,
				VALUE == 1 ? "is" : "are"
			);
			snprintf(
				ACTION, ACTION_SIZE, "all but %s -> %s", VALUE_TEXT, VALUE_TEXT
			);
			return (1);
		}
	}

	return (0);
}

/* ---------- "reply with exactly three words" ---------- */

static int
	ANSWER_WORD_COUNT_ORDER(
		const READING *READ, char *ACTION, int ACTION_SIZE, char *REPLY,
		int REPLY_SIZE
	)
{
	static const char	*SENTENCES[11] = {
		"", "Okay.", "Sure thing.", "Here you go.", "These are four words.",
		"This reply has five words.", "This reply has exactly six words.",
		"Here is a reply of seven words.",
		"Here is a reply that has eight words.",
		"Here is a reply that has exactly nine words.",
		"Here is a reply that has exactly ten words, see?"
	};
	int					INDEX;
	int					WORDS = 0;
	int					CONTENT = 0;

	if (!HAS(READ, " words ") && !HAS(READ, " word "))
		return (0);

	if (
		!HAS(READ, " reply ") &&
		!HAS(READ, " answer ") &&
		!HAS(READ, " respond ") &&
		!HAS(READ, " say ") &&
		!HAS(READ, " write ") &&
		!HAS(READ, " use ")
	)
		return (0);

	for (INDEX = 0; INDEX + 1 < READ->COUNT; INDEX++)
		if (
			IS_NUMBER(&READ->TOKENS[INDEX]) &&
			(
				!strcmp(TEXT_AT(READ, INDEX + 1), "words") ||
				!strcmp(TEXT_AT(READ, INDEX + 1), "word")
			)
		)
			WORDS = (int)READ->TOKENS[INDEX].VALUE;

	if (WORDS < 1 || WORDS > 10)
		return (0);

	/* only the order itself: nothing else to talk about */
	for (INDEX = 0; INDEX < READ->COUNT; INDEX++)
	{
		static const char	*ORDER_WORDS[33] = {
			"reply", "answer", "respond", "say", "write", "use", "with", "in",
			"exactly", "only", "just", "words", "word", "please", "me", "to",
			"something", "anything", "a", "an", "sentence", "of", "using",
			"your", "next", "message", "can", "you", "could", "lucy", "now",
			"the", NULL
		};

		if (
			READ->TOKENS[INDEX].KIND == TOKEN_WORD &&
			!IN_LIST(READ->TOKENS[INDEX].TEXT, ORDER_WORDS)
		)
			CONTENT++;
	}

	if (CONTENT)
		return (0);

	snprintf(REPLY, REPLY_SIZE, "%s", SENTENCES[WORDS]);
	snprintf(
		ACTION, ACTION_SIZE, "say %d words -> %s", WORDS, SENTENCES[WORDS]
	);

	return (1);
}

/* ---------- "5 machines make 5 widgets in 5 minutes" ---------- */

typedef struct
{
	double	VALUE;
	char	NOUN[48];
	char	SHOWN[48];
	int		IS_TIME;
	int		AT;
} COUNTED;

/* every "number noun" in the message from FROM to TO */
static int
	COUNTED_THINGS(
		const READING *READ, int FROM, int TO, COUNTED *OUTPUT, int LIMIT
	)
{
	int	COUNT = 0;
	int	INDEX;

	for (INDEX = FROM; INDEX + 1 < TO && COUNT < LIMIT; INDEX++)
	{
		int	NOUN_AT = INDEX + 1;
		int	UNIT;

		if (!IS_NUMBER(&READ->TOKENS[INDEX]) || !IS_WORD_TOKEN(READ, NOUN_AT))
			continue ;

		/* "5 more widgets": the widgets */
		if (
			!strcmp(TEXT_AT(READ, NOUN_AT), "more") &&
			IS_WORD_TOKEN(READ, NOUN_AT + 1)
		)
			NOUN_AT++;

		OUTPUT[COUNT].VALUE = READ->TOKENS[INDEX].VALUE;
		SINGULAR(READ->TOKENS[NOUN_AT].TEXT, OUTPUT[COUNT].NOUN, 48);
		snprintf(OUTPUT[COUNT].SHOWN, 48, "%s", READ->TOKENS[NOUN_AT].TEXT);
		UNIT = FIND_UNIT(READ->TOKENS[NOUN_AT].TEXT);
		OUTPUT[COUNT].IS_TIME = UNIT >= 0 && UNITS[UNIT].KIND == 1;
		OUTPUT[COUNT].AT = INDEX;
		COUNT++;
	}

	return (COUNT);
}

static void
	SAY_AMOUNT(double VALUE, const char *NOUN, char *OUTPUT, int SIZE)
{
	char	NUMBER[64];
	char	SINGLE[48];

	SAY_NUMBER(VALUE, NUMBER, sizeof NUMBER);
	SINGULAR(NOUN, SINGLE, sizeof SINGLE);

	if (SAME(VALUE, 1))
		snprintf(OUTPUT, SIZE, "%s %s", NUMBER, SINGLE);
	else if (!strcmp(SINGLE, NOUN))
		snprintf(OUTPUT, SIZE, "%s %ss", NUMBER, NOUN);
	else
		snprintf(OUTPUT, SIZE, "%s %s", NUMBER, NOUN);
}

static int
	ANSWER_WORK_RATE(
		const READING *READ, char *ACTION, int ACTION_SIZE, char *REPLY,
		int REPLY_SIZE
	)
{
	COUNTED	GIVEN[6];
	COUNTED	ASKED[6];
	int		GIVEN_COUNT;
	int		ASKED_COUNT;
	int		QUESTION_AT = -1;
	int		INDEX;
	int		WORKER = -1;
	int		PRODUCT = -1;
	int		TIME = -1;
	double	WORKERS = 0;
	double	PRODUCTS = 0;
	double	SPAN = 0;
	char	TIME_UNIT[48] = "";
	char	TEXT[64];

	if (
		!HAS(READ, " how long ") &&
		!HAS(READ, " how many ") &&
		!HAS(READ, " how much time ")
	)
		return (0);

	for (INDEX = 0; INDEX + 1 < READ->COUNT; INDEX++)
		if (
			!strcmp(READ->TOKENS[INDEX].TEXT, "how") &&
			(
				!strcmp(READ->TOKENS[INDEX + 1].TEXT, "long") ||
				!strcmp(READ->TOKENS[INDEX + 1].TEXT, "many") ||
				!strcmp(READ->TOKENS[INDEX + 1].TEXT, "much")
			)
		)
		{
			QUESTION_AT = INDEX;
			break ;
		}

	if (QUESTION_AT < 0)
		return (0);

	GIVEN_COUNT = COUNTED_THINGS(READ, 0, QUESTION_AT, GIVEN, 6);
	ASKED_COUNT = COUNTED_THINGS(READ, QUESTION_AT, READ->COUNT, ASKED, 6);

	/* the story: workers, things made and a time, each once */
	if (GIVEN_COUNT != 3)
		return (0);

	for (INDEX = 0; INDEX < 3; INDEX++)
		if (GIVEN[INDEX].IS_TIME)
			TIME = INDEX;
		else if (WORKER < 0)
			WORKER = INDEX;
		else
			PRODUCT = INDEX;

	if (
		TIME < 0 ||
		WORKER < 0 ||
		PRODUCT < 0 ||
		!strcmp(GIVEN[WORKER].NOUN, GIVEN[PRODUCT].NOUN)
	)
		return (0);

	/* the question names two of the three; the third is asked */
	for (INDEX = 0; INDEX < ASKED_COUNT; INDEX++)
		if (ASKED[INDEX].IS_TIME)
		{
			int	FROM_UNIT = FIND_UNIT(ASKED[INDEX].SHOWN);
			int	TO_UNIT = FIND_UNIT(GIVEN[TIME].SHOWN);

			SPAN = ASKED[INDEX].VALUE * UNITS[FROM_UNIT].SIZE /
				UNITS[TO_UNIT].SIZE;
		}
		else if (!strcmp(ASKED[INDEX].NOUN, GIVEN[WORKER].NOUN))
			WORKERS = ASKED[INDEX].VALUE;
		else if (!strcmp(ASKED[INDEX].NOUN, GIVEN[PRODUCT].NOUN))
			PRODUCTS = ASKED[INDEX].VALUE;
		else
			return (0);

	snprintf(TIME_UNIT, sizeof TIME_UNIT, "%s", GIVEN[TIME].SHOWN);

	{
		/* things one worker makes in one unit of time */
		double	RATE =
			GIVEN[PRODUCT].VALUE / (GIVEN[WORKER].VALUE * GIVEN[TIME].VALUE);
		double	ANSWER;
		char	EACH[128];
		char	PART_A[128];
		char	PART_B[128];
		char	PER_WORKER[128];

		if (RATE <= 0)
			return (0);

		SAY_AMOUNT(
			GIVEN[PRODUCT].VALUE / GIVEN[WORKER].VALUE, GIVEN[PRODUCT].SHOWN,
			PER_WORKER, sizeof PER_WORKER
		);
		SAY_AMOUNT(GIVEN[TIME].VALUE, TIME_UNIT, EACH, sizeof EACH);

		if (
			WORKERS > 0 &&
			PRODUCTS > 0 &&
			SPAN == 0 &&
			(
				HAS(READ, " how long ") ||
				HAS(READ, " how much time ") ||
				HAS(READ, " how many minutes ") ||
				HAS(READ, " how many hours ") ||
				HAS(READ, " how many days ") ||
				HAS(READ, " how many seconds ")
			)
		)
		{
			ANSWER = PRODUCTS / (WORKERS * RATE);
			SAY_AMOUNT(ANSWER, TIME_UNIT, TEXT, sizeof TEXT);
			SAY_AMOUNT(WORKERS, GIVEN[WORKER].SHOWN, PART_A, sizeof PART_A);
			SAY_AMOUNT(PRODUCTS, GIVEN[PRODUCT].SHOWN, PART_B, sizeof PART_B);
			snprintf(
				REPLY, REPLY_SIZE,
				"%s. Each %s makes %s in %s, so %s make %s in %s.", TEXT,
				GIVEN[WORKER].NOUN, PER_WORKER, EACH, PART_A, PART_B, TEXT
			);
		}
		else if (WORKERS > 0 && SPAN > 0 && PRODUCTS == 0)
		{
			ANSWER = WORKERS * RATE * SPAN;
			SAY_AMOUNT(ANSWER, GIVEN[PRODUCT].SHOWN, TEXT, sizeof TEXT);
			SAY_AMOUNT(WORKERS, GIVEN[WORKER].SHOWN, PART_A, sizeof PART_A);
			SAY_AMOUNT(SPAN, TIME_UNIT, PART_B, sizeof PART_B);
			snprintf(
				REPLY, REPLY_SIZE,
				"%s. Each %s makes %s in %s, so %s make %s in %s.", TEXT,
				GIVEN[WORKER].NOUN, PER_WORKER, EACH, PART_A, TEXT, PART_B
			);
		}
		else if (PRODUCTS > 0 && SPAN > 0 && WORKERS == 0)
		{
			ANSWER = PRODUCTS / (RATE * SPAN);
			SAY_AMOUNT(ANSWER, GIVEN[WORKER].SHOWN, TEXT, sizeof TEXT);
			SAY_AMOUNT(PRODUCTS, GIVEN[PRODUCT].SHOWN, PART_A, sizeof PART_A);
			SAY_AMOUNT(SPAN, TIME_UNIT, PART_B, sizeof PART_B);
			snprintf(
				REPLY, REPLY_SIZE,
				"%s. Each %s makes %s in %s, so it takes %s to make %s in %s.",
				TEXT, GIVEN[WORKER].NOUN, PER_WORKER, EACH, TEXT, PART_A, PART_B
			);
		}
		else
			return (0);

		snprintf(ACTION, ACTION_SIZE, "work rate -> %s", TEXT);
		return (1);
	}
}

/* ---------- "double 8 and then add 3", "I think of a number ..." ------- */

enum
{
	STEP_ADD = 1,
	STEP_SUBTRACT,
	STEP_MULTIPLY,
	STEP_DIVIDE,
	STEP_SQUARE
};

typedef struct
{
	int		KIND;
	double	VALUE;
} STEP;

/* the steps done to a number, from INDEX on: "add 5", "multiply it by 3",
 * "double it", "take away 2", "divide by 4", "square it" */
static int
	READ_STEPS(
		const READING *READ, int INDEX, int TO, STEP *STEPS, int LIMIT,
		int *START_VALUE_AT
	)
{
	int	COUNT = 0;

	if (START_VALUE_AT)
		*START_VALUE_AT = -1;

	for (; INDEX < TO && COUNT < LIMIT; INDEX++)
	{
		const char	*WORD = READ->TOKENS[INDEX].TEXT;
		int			NEXT = INDEX + 1;
		int			KIND = 0;
		double		VALUE = 0;

		if (READ->TOKENS[INDEX].KIND != TOKEN_WORD)
			continue ;

		/* "it", "the number", "that" after the verb */
		while (
			NEXT < TO &&
			(
				!strcmp(TEXT_AT(READ, NEXT), "it") ||
				!strcmp(TEXT_AT(READ, NEXT), "that") ||
				!strcmp(TEXT_AT(READ, NEXT), "the") ||
				!strcmp(TEXT_AT(READ, NEXT), "number") ||
				!strcmp(TEXT_AT(READ, NEXT), "result") ||
				!strcmp(TEXT_AT(READ, NEXT), "this")
			)
		)
			NEXT++;

		if (
			!strcmp(WORD, "double") ||
			!strcmp(WORD, "doubles") ||
			!strcmp(WORD, "twice")
		)
		{
			KIND = STEP_MULTIPLY;
			VALUE = 2;
		}
		else if (!strcmp(WORD, "triple") || !strcmp(WORD, "triples"))
		{
			KIND = STEP_MULTIPLY;
			VALUE = 3;
		}
		else if (!strcmp(WORD, "halve") || !strcmp(WORD, "halves"))
		{
			KIND = STEP_DIVIDE;
			VALUE = 2;
		}
		else if (!strcmp(WORD, "square") && strcmp(TEXT_AT(READ, NEXT), "root"))
			KIND = STEP_SQUARE;
		else if (
			(
				!strcmp(WORD, "add") ||
				!strcmp(WORD, "adds") ||
				!strcmp(WORD, "plus")
			) &&
			NEXT < TO &&
			IS_NUMBER(&READ->TOKENS[NEXT])
		)
		{
			KIND = STEP_ADD;
			VALUE = READ->TOKENS[NEXT].VALUE;
		}
		else if (
			(
				!strcmp(WORD, "subtract") ||
				!strcmp(WORD, "subtracts") ||
				!strcmp(WORD, "minus") ||
				!strcmp(WORD, "take") ||
				!strcmp(WORD, "takes") ||
				!strcmp(WORD, "remove")
			) &&
			NEXT < TO
		)
		{
			if (
				!strcmp(TEXT_AT(READ, NEXT), "away") ||
				!strcmp(TEXT_AT(READ, NEXT), "off")
			)
				NEXT++;

			if (NEXT < TO && IS_NUMBER(&READ->TOKENS[NEXT]))
			{
				KIND = STEP_SUBTRACT;
				VALUE = READ->TOKENS[NEXT].VALUE;
			}
		}
		else if (
			(
				!strcmp(WORD, "multiply") ||
				!strcmp(WORD, "multiplies") ||
				!strcmp(WORD, "times")
			) &&
			NEXT < TO
		)
		{
			if (!strcmp(TEXT_AT(READ, NEXT), "by"))
				NEXT++;

			if (NEXT < TO && IS_NUMBER(&READ->TOKENS[NEXT]))
			{
				KIND = STEP_MULTIPLY;
				VALUE = READ->TOKENS[NEXT].VALUE;
			}
		}
		else if (
			(!strcmp(WORD, "divide") || !strcmp(WORD, "divides")) &&
			NEXT < TO
		)
		{
			if (!strcmp(TEXT_AT(READ, NEXT), "by"))
				NEXT++;

			if (
				NEXT < TO &&
				IS_NUMBER(&READ->TOKENS[NEXT]) &&
				READ->TOKENS[NEXT].VALUE != 0
			)
			{
				KIND = STEP_DIVIDE;
				VALUE = READ->TOKENS[NEXT].VALUE;
			}
		}

		if (!KIND)
			continue ;

		/* "double 8": the number the steps start from */
		if (
			KIND != STEP_ADD &&
			KIND != STEP_SUBTRACT &&
			START_VALUE_AT &&
			*START_VALUE_AT < 0 &&
			COUNT == 0 &&
			NEXT < TO &&
			IS_NUMBER(&READ->TOKENS[NEXT]) &&
			(KIND == STEP_SQUARE || VALUE == 2 || VALUE == 3) &&
			(
				!strcmp(WORD, "double") ||
				!strcmp(WORD, "triple") ||
				!strcmp(WORD, "halve") ||
				!strcmp(WORD, "square")
			)
		)
			*START_VALUE_AT = NEXT;

		STEPS[COUNT].KIND = KIND;
		STEPS[COUNT].VALUE = VALUE;
		COUNT++;

		if (
			KIND == STEP_ADD ||
			KIND == STEP_SUBTRACT ||
			(
				KIND != STEP_SQUARE &&
				strcmp(WORD, "double") &&
				strcmp(WORD, "triple") &&
				strcmp(WORD, "halve") &&
				strcmp(WORD, "twice")
			)
		)
			INDEX = NEXT;
	}

	return (COUNT);
}

static double
	DO_STEP(const STEP *ONE, double VALUE)
{
	switch (ONE->KIND)
	{
		case STEP_ADD:
		{
			return (VALUE + ONE->VALUE);
		}
		case STEP_SUBTRACT:
		{
			return (VALUE - ONE->VALUE);
		}
		case STEP_MULTIPLY:
		{
			return (VALUE * ONE->VALUE);
		}
		case STEP_DIVIDE:
		{
			return (VALUE / ONE->VALUE);
		}
		case STEP_SQUARE:
		{
			return (VALUE * VALUE);
		}
	}

	return (VALUE);
}

static double
	UNDO_STEP(const STEP *ONE, double VALUE)
{
	switch (ONE->KIND)
	{
		case STEP_ADD:
		{
			return (VALUE - ONE->VALUE);
		}
		case STEP_SUBTRACT:
		{
			return (VALUE + ONE->VALUE);
		}
		case STEP_MULTIPLY:
		{
			return (VALUE / ONE->VALUE);
		}
		case STEP_DIVIDE:
		{
			return (VALUE * ONE->VALUE);
		}
		case STEP_SQUARE:
		{
			if (VALUE >= 0)
				return (sqrt(VALUE));

			return (NAN);
		}
	}

	return (VALUE);
}

/* "8 x 2 = 16, and 16 + 3 = 19" */
static void
	SAY_STEPS(
		const STEP *STEPS, int COUNT, double START, char *OUTPUT, int SIZE
	)
{
	int		INDEX;
	int		LENGTH = 0;
	double	VALUE = START;

	OUTPUT[0] = 0;

	for (INDEX = 0; INDEX < COUNT && LENGTH < SIZE - 1; INDEX++)
	{
		char	BEFORE[64];
		char	AMOUNT[64];
		char	AFTER[64];
		double	NEXT = DO_STEP(&STEPS[INDEX], VALUE);

		SAY_NUMBER(VALUE, BEFORE, sizeof BEFORE);
		SAY_NUMBER(STEPS[INDEX].VALUE, AMOUNT, sizeof AMOUNT);
		SAY_NUMBER(NEXT, AFTER, sizeof AFTER);

		if (STEPS[INDEX].KIND == STEP_SQUARE)
			LENGTH += snprintf(
				OUTPUT + LENGTH, SIZE - LENGTH, "%s%s x %s = %s",
				INDEX ? ", and " : "", BEFORE, BEFORE, AFTER
			);
		else
			LENGTH += snprintf(
				OUTPUT + LENGTH, SIZE - LENGTH, "%s%s %s %s = %s",
				INDEX ? ", and " : "", BEFORE,
				STEPS[INDEX].KIND == STEP_ADD ? "+"
					: STEPS[INDEX].KIND == STEP_SUBTRACT ? "-"
					: STEPS[INDEX].KIND == STEP_MULTIPLY ? "x"
				: "/",
				AMOUNT, AFTER
			);

		VALUE = NEXT;
	}
}

/* the number after "get", "equals", "is", "makes", "gives" from INDEX on */
static int
	RESULT_NUMBER(const READING *READ, int FROM, double *VALUE)
{
	int	INDEX;

	for (INDEX = FROM; INDEX + 1 < READ->COUNT; INDEX++)
		if (
			(
				!strcmp(READ->TOKENS[INDEX].TEXT, "get") ||
				!strcmp(READ->TOKENS[INDEX].TEXT, "gets") ||
				!strcmp(READ->TOKENS[INDEX].TEXT, "got") ||
				!strcmp(READ->TOKENS[INDEX].TEXT, "equals") ||
				!strcmp(READ->TOKENS[INDEX].TEXT, "makes") ||
				!strcmp(READ->TOKENS[INDEX].TEXT, "gives") ||
				!strcmp(READ->TOKENS[INDEX].TEXT, "is") ||
				!strcmp(READ->TOKENS[INDEX].TEXT, "=") ||
				!strcmp(READ->TOKENS[INDEX].TEXT, "becomes")
			) &&
			IS_NUMBER(&READ->TOKENS[INDEX + 1])
		)
		{
			*VALUE = READ->TOKENS[INDEX + 1].VALUE;
			return (INDEX + 1);
		}

	return (-1);
}

static int
	ANSWER_THINK_NUMBER(
		const READING *READ, char *ACTION, int ACTION_SIZE, char *REPLY,
		int REPLY_SIZE
	)
{
	STEP	STEPS[8];
	int		STEP_COUNT;
	int		INDEX;
	int		NUMBER_AT = -1;
	int		RESULT_AT;
	double	RESULT;
	double	VALUE;
	char	TEXT[64];
	char	WORK[400];

	if (
		!HAS(READ, " a number ") &&
		!HAS(READ, " the number ") &&
		!HAS(READ, " my number ")
	)
		return (0);

	if (
		!HAS(READ, " what is the number ") &&
		!HAS(READ, " what's the number ") &&
		!HAS(READ, " what number ") &&
		!HAS(READ, " find the number ") &&
		!HAS(READ, " what is my number ") &&
		!HAS(READ, " which number ") &&
		!HAS(READ, " what was the number ") &&
		!HAS(READ, " what is it ") &&
		!HAS(READ, " guess the number ") &&
		!HAS(READ, " what's my number ")
	)
		return (0);

	for (INDEX = 0; INDEX + 1 < READ->COUNT; INDEX++)
		if (
			(
				!strcmp(READ->TOKENS[INDEX].TEXT, "a") ||
				!strcmp(READ->TOKENS[INDEX].TEXT, "my")
			) &&
			!strcmp(READ->TOKENS[INDEX + 1].TEXT, "number")
		)
		{
			NUMBER_AT = INDEX + 1;
			break ;
		}

	if (NUMBER_AT < 0)
		return (0);

	RESULT_AT = RESULT_NUMBER(READ, NUMBER_AT + 1, &RESULT);

	if (RESULT_AT < 0)
		return (0);

	STEP_COUNT = READ_STEPS(READ, NUMBER_AT + 1, RESULT_AT, STEPS, 8, NULL);

	if (!STEP_COUNT)
		return (0);

	VALUE = RESULT;

	for (INDEX = STEP_COUNT - 1; INDEX >= 0; INDEX--)
		VALUE = UNDO_STEP(&STEPS[INDEX], VALUE);

	if (isnan(VALUE))
		return (0);

	SAY_NUMBER(VALUE, TEXT, sizeof TEXT);
	SAY_STEPS(STEPS, STEP_COUNT, VALUE, WORK, sizeof WORK);
	snprintf(REPLY, REPLY_SIZE, "The number is %s: %s.", TEXT, WORK);
	snprintf(ACTION, ACTION_SIZE, "solve number -> %s", TEXT);

	return (1);
}

static int
	ANSWER_STEP_CHAIN(
		const READING *READ, char *ACTION, int ACTION_SIZE, char *REPLY,
		int REPLY_SIZE
	)
{
	STEP	STEPS[8];
	int		STEP_COUNT;
	int		START_AT = -1;
	int		INDEX;
	double	VALUE;
	char	TEXT[64];
	char	WORK[400];

	if (
		!HAS(READ, " what do you get ") &&
		!HAS(READ, " what is the result ") &&
		!HAS(READ, " what's the result ") &&
		!HAS(READ, " what number do you ") &&
		!HAS(READ, " what do i get ") &&
		!HAS(READ, " what does that make ") &&
		!HAS(READ, " what is the answer ") &&
		!HAS(READ, " what's the answer ") &&
		!HAS(READ, " what do we get ") &&
		!HAS(READ, " what is it now ") &&
		!HAS(READ, " what number is it ") &&
		!HAS(READ, " what's left ") &&
		!HAS(READ, " what does it make ") &&
		!HAS(READ, " what is the total ")
	)
		return (0);

	/* "start with 10", "take 7", "begin with 4" */
	for (INDEX = 0; INDEX + 1 < READ->COUNT; INDEX++)
		if (
			(
				!strcmp(READ->TOKENS[INDEX].TEXT, "with") &&
				INDEX > 0 &&
				(
					!strcmp(READ->TOKENS[INDEX - 1].TEXT, "start") ||
					!strcmp(READ->TOKENS[INDEX - 1].TEXT, "begin")
				)
			) &&
			IS_NUMBER(&READ->TOKENS[INDEX + 1])
		)
		{
			START_AT = INDEX + 1;
			break ;
		}

	STEP_COUNT = READ_STEPS(
		READ, START_AT >= 0 ? START_AT + 1 : 0, READ->COUNT, STEPS, 8,
		START_AT >= 0 ? NULL : &START_AT
	);

	/* "take 7, double it": the number taken first */
	if (START_AT < 0)
		for (INDEX = 0; INDEX + 1 < READ->COUNT; INDEX++)
			if (
				!strcmp(READ->TOKENS[INDEX].TEXT, "take") &&
				IS_NUMBER(&READ->TOKENS[INDEX + 1]) &&
				strcmp(TEXT_AT(READ, INDEX + 2), "away") &&
				strcmp(TEXT_AT(READ, INDEX + 2), "from")
			)
			{
				START_AT = INDEX + 1;
				STEP_COUNT =
					READ_STEPS(READ, START_AT + 1, READ->COUNT, STEPS, 8, NULL);
				break ;
			}

	if (START_AT < 0 || STEP_COUNT < 2)
		return (0);

	VALUE = READ->TOKENS[START_AT].VALUE;

	for (INDEX = 0; INDEX < STEP_COUNT; INDEX++)
		VALUE = DO_STEP(&STEPS[INDEX], VALUE);

	SAY_NUMBER(VALUE, TEXT, sizeof TEXT);
	SAY_STEPS(
		STEPS, STEP_COUNT, READ->TOKENS[START_AT].VALUE, WORK, sizeof WORK
	);
	snprintf(REPLY, REPLY_SIZE, "%s: %s.", TEXT, WORK);
	snprintf(ACTION, ACTION_SIZE, "steps -> %s", TEXT);

	return (1);
}

/* ---------- "25% off $20", "increase 80 by 10%" ---------- */

/* the price in the message: "$20", "20 dollars", "€15" */
static int
	PRICE_IN(
		const READING *READ, double *VALUE, char *SIGN, int SIGN_SIZE,
		int SKIP_AT
	)
{
	int	INDEX;

	for (INDEX = 0; INDEX < READ->COUNT; INDEX++)
	{
		if (INDEX == SKIP_AT || !IS_NUMBER(&READ->TOKENS[INDEX]))
			continue ;

		if (
			INDEX > 0 &&
			READ->TOKENS[INDEX - 1].KIND == TOKEN_SYMBOL &&
			strchr("$", READ->TOKENS[INDEX - 1].TEXT[0])
		)
		{
			*VALUE = READ->TOKENS[INDEX].VALUE;
			snprintf(SIGN, SIGN_SIZE, "$");
			return (INDEX);
		}

		if (
			!strncmp(TEXT_AT(READ, INDEX + 1), "dollar", 6) ||
			!strncmp(TEXT_AT(READ, INDEX + 1), "euro", 4) ||
			!strncmp(TEXT_AT(READ, INDEX + 1), "buck", 4)
		)
		{
			*VALUE = READ->TOKENS[INDEX].VALUE;
			snprintf(
				SIGN, SIGN_SIZE, "%s",
				!strncmp(TEXT_AT(READ, INDEX + 1), "euro", 4) ? " euros"
				: " dollars"
			);
			return (INDEX);
		}
	}

	return (-1);
}

static void
	SAY_MONEY(double VALUE, const char *SIGN, char *OUTPUT, int SIZE)
{
	char	NUMBER[64];

	if (fabs(VALUE - floor(VALUE + 0.5)) < 1e-9)
		SAY_NUMBER(VALUE, NUMBER, sizeof NUMBER);
	else
		snprintf(NUMBER, sizeof NUMBER, "%.2f", VALUE);

	if (SIGN[0] == '$')
		snprintf(OUTPUT, SIZE, "$%s", NUMBER);
	else
		snprintf(OUTPUT, SIZE, "%s%s", NUMBER, SIGN);
}

static int
	ANSWER_PERCENT_CHANGE(
		const READING *READ, char *ACTION, int ACTION_SIZE, char *REPLY,
		int REPLY_SIZE
	)
{
	int		INDEX;
	int		PERCENT_AT = -1;
	double	PERCENT = 0;
	double	PRICE;
	char	SIGN[16] = "";
	int		PRICE_AT;

	if (!HAS(READ, " % ") && !HAS(READ, " percent "))
		return (0);

	for (INDEX = 0; INDEX + 1 < READ->COUNT; INDEX++)
		if (
			IS_NUMBER(&READ->TOKENS[INDEX]) &&
			(
				!strcmp(READ->TOKENS[INDEX + 1].TEXT, "%") ||
				!strcmp(READ->TOKENS[INDEX + 1].TEXT, "percent")
			)
		)
		{
			PERCENT_AT = INDEX;
			PERCENT = READ->TOKENS[INDEX].VALUE;
			break ;
		}

	if (PERCENT_AT < 0)
		return (0);

	/* "a $20 shirt is 25% off", "a discount of 25%" */
	if (
		HAS(READ, " off ") ||
		HAS(READ, " discount ") ||
		HAS(READ, " sale ") ||
		HAS(READ, " cheaper ") ||
		HAS(READ, " reduced ")
	)
	{
		char	OLD[64];
		char	CUT[64];
		char	NEW[64];
		char	PERCENT_TEXT[64];

		PRICE_AT = PRICE_IN(READ, &PRICE, SIGN, sizeof SIGN, PERCENT_AT);

		if (PRICE_AT < 0)
			return (0);

		SAY_MONEY(PRICE, SIGN, OLD, sizeof OLD);
		SAY_MONEY(PRICE * PERCENT / 100, SIGN, CUT, sizeof CUT);
		SAY_MONEY(PRICE * (1 - PERCENT / 100), SIGN, NEW, sizeof NEW);
		SAY_NUMBER(PERCENT, PERCENT_TEXT, sizeof PERCENT_TEXT);
		snprintf(
			REPLY, REPLY_SIZE, "%s: %s%% of %s is %s, and %s - %s = %s.", NEW,
			PERCENT_TEXT, OLD, CUT, OLD, CUT, NEW
		);
		snprintf(
			ACTION, ACTION_SIZE, "calc %s * (1 - %s / 100) -> %s", OLD,
			PERCENT_TEXT, NEW
		);
		return (1);
	}

	/* "increase 80 by 10%", "80 plus 10%", "a 10% raise on $50" */
	if (
		HAS(READ, " increase") ||
		HAS(READ, " decrease") ||
		HAS(READ, " raise ") ||
		HAS(READ, " more than ") ||
		HAS(READ, " less than ") ||
		HAS(READ, " goes up ") ||
		HAS(READ, " goes down ")
	)
	{
		int		DOWN = HAS(READ, " decrease") || HAS(READ, " less than ") ||
			HAS(READ, " goes down ");
		double	BASE = 0;
		int		FOUND = 0;
		char	BASE_TEXT[64];
		char	CHANGE[64];
		char	NEW[64];
		char	PERCENT_TEXT[64];

		PRICE_AT = PRICE_IN(READ, &PRICE, SIGN, sizeof SIGN, PERCENT_AT);

		if (PRICE_AT >= 0)
		{
			BASE = PRICE;
			FOUND = 1;
		}
		else
			for (INDEX = 0; INDEX < READ->COUNT; INDEX++)
				if (INDEX != PERCENT_AT && IS_NUMBER(&READ->TOKENS[INDEX]))
				{
					BASE = READ->TOKENS[INDEX].VALUE;
					FOUND++;
				}

		if (FOUND != 1)
			return (0);

		if (SIGN[0])
		{
			SAY_MONEY(BASE, SIGN, BASE_TEXT, sizeof BASE_TEXT);
			SAY_MONEY(BASE * PERCENT / 100, SIGN, CHANGE, sizeof CHANGE);
			SAY_MONEY(
				BASE * (1 + (DOWN ? -1 : 1) * PERCENT / 100), SIGN, NEW,
				sizeof NEW
			);
		}
		else
		{
			SAY_NUMBER(BASE, BASE_TEXT, sizeof BASE_TEXT);
			SAY_NUMBER(BASE * PERCENT / 100, CHANGE, sizeof CHANGE);
			SAY_NUMBER(
				BASE * (1 + (DOWN ? -1 : 1) * PERCENT / 100), NEW, sizeof NEW
			);
		}

		SAY_NUMBER(PERCENT, PERCENT_TEXT, sizeof PERCENT_TEXT);
		snprintf(
			REPLY, REPLY_SIZE, "%s: %s%% of %s is %s, and %s %s %s = %s.", NEW,
			PERCENT_TEXT, BASE_TEXT, CHANGE, BASE_TEXT, DOWN ? "-" : "+",
			CHANGE, NEW
		);
		snprintf(
			ACTION, ACTION_SIZE, "calc %s %s %s%% -> %s", BASE_TEXT,
			DOWN ? "-" : "+", PERCENT_TEXT, NEW
		);
		return (1);
	}

	return (0);
}

/* "what percent of 50 is 10?", "10 is what percent of 50?", "10 out of 50
 * as a percentage" */
static int
	ANSWER_WHAT_PERCENT(
		const READING *READ, char *ACTION, int ACTION_SIZE, char *REPLY,
		int REPLY_SIZE
	)
{
	double	PART = 0;
	double	WHOLE = 0;
	int		INDEX;
	char	PART_TEXT[64];
	char	WHOLE_TEXT[64];
	char	TEXT[64];

	if (HAS(READ, " what percent of ") || HAS(READ, " what percentage of "))
	{
		/* "what percent of 50 is 10", "10 is what percent of 50" */
		for (INDEX = 0; INDEX + 2 < READ->COUNT; INDEX++)
			if (
				!strcmp(READ->TOKENS[INDEX].TEXT, "of") &&
				IS_NUMBER(&READ->TOKENS[INDEX + 1])
			)
			{
				int	SCAN;

				WHOLE = READ->TOKENS[INDEX + 1].VALUE;

				for (SCAN = 0; SCAN < READ->COUNT; SCAN++)
					if (SCAN != INDEX + 1 && IS_NUMBER(&READ->TOKENS[SCAN]))
						PART = READ->TOKENS[SCAN].VALUE;

				break ;
			}
	}
	else if (
		HAS(READ, " out of ") &&
		(HAS(READ, " percent") || HAS(READ, " % "))
	)
	{
		for (INDEX = 1; INDEX + 2 < READ->COUNT; INDEX++)
			if (
				!strcmp(READ->TOKENS[INDEX].TEXT, "out") &&
				!strcmp(READ->TOKENS[INDEX + 1].TEXT, "of") &&
				IS_NUMBER(&READ->TOKENS[INDEX - 1]) &&
				IS_NUMBER(&READ->TOKENS[INDEX + 2])
			)
			{
				PART = READ->TOKENS[INDEX - 1].VALUE;
				WHOLE = READ->TOKENS[INDEX + 2].VALUE;
				break ;
			}
	}
	else
		return (0);

	if (WHOLE == 0)
		return (0);

	SAY_NUMBER(PART, PART_TEXT, sizeof PART_TEXT);
	SAY_NUMBER(WHOLE, WHOLE_TEXT, sizeof WHOLE_TEXT);
	SAY_NUMBER(floor(PART / WHOLE * 100 * 1e4 + 0.5) / 1e4, TEXT, sizeof TEXT);
	snprintf(
		REPLY, REPLY_SIZE, "%s%%: %s / %s x 100 = %s.", TEXT, PART_TEXT,
		WHOLE_TEXT, TEXT
	);
	snprintf(
		ACTION, ACTION_SIZE, "calc %s / %s * 100 -> %s", PART_TEXT, WHOLE_TEXT,
		TEXT
	);

	return (1);
}

/* ---------- remainders, rounding, sums of runs, primes, squares ------- */

static long long
	GREATEST_COMMON(long long FIRST, long long SECOND)
{
	while (SECOND)
	{
		long long	REST = FIRST % SECOND;

		FIRST = SECOND;
		SECOND = REST;
	}

	if (FIRST < 0)
		return (-FIRST);

	return (FIRST);
}

/* the first two plain numbers in the message */
static int
	TWO_NUMBERS(const READING *READ, double *FIRST, double *SECOND)
{
	int	FOUND = 0;
	int	INDEX;

	for (INDEX = 0; INDEX < READ->COUNT && FOUND < 2; INDEX++)
		if (IS_NUMBER(&READ->TOKENS[INDEX]) && !READ->TOKENS[INDEX].IS_FRACTION)
		{
			if (FOUND == 0)
				*FIRST = READ->TOKENS[INDEX].VALUE;
			else
				*SECOND = READ->TOKENS[INDEX].VALUE;

			FOUND++;
		}

	return (FOUND);
}

static int
	ANSWER_NUMBER_MORE(
		const READING *READ, char *ACTION, int ACTION_SIZE, char *REPLY,
		int REPLY_SIZE
	)
{
	double	FIRST = 0;
	double	SECOND = 0;
	char	A[64];
	char	B[64];
	char	TEXT[64];
	int		FOUND = TWO_NUMBERS(READ, &FIRST, &SECOND);

	/* "the remainder when 17 is divided by 5", "17 mod 5" */
	if (
		(
			HAS(READ, " remainder ") ||
			HAS(READ, " mod ") ||
			HAS(READ, " modulo ") ||
			HAS(READ, " left over ")
		) &&
		FOUND == 2 &&
		(
			HAS(READ, " divided ") ||
			HAS(READ, " mod ") ||
			HAS(READ, " modulo ") ||
			HAS(READ, " by ")
		)
	)
	{
		long long	TOP = (long long)FIRST;
		long long	BOTTOM = (long long)SECOND;
		char		WHOLE[64];

		if (BOTTOM == 0 || FIRST != (double)TOP || SECOND != (double)BOTTOM)
			return (0);

		snprintf(A, sizeof A, "%lld", TOP);
		snprintf(B, sizeof B, "%lld", BOTTOM);
		snprintf(TEXT, sizeof TEXT, "%lld", TOP % BOTTOM);
		snprintf(WHOLE, sizeof WHOLE, "%lld", TOP / BOTTOM);
		snprintf(
			REPLY, REPLY_SIZE,
			"The remainder is %s: %s goes into %s %s times (%s x %s = %lld), "
			"with %s left over.",
			TEXT, B, A, WHOLE, B, WHOLE, BOTTOM * (TOP / BOTTOM), TEXT
		);
		snprintf(ACTION, ACTION_SIZE, "calc %s mod %s -> %s", A, B, TEXT);
		return (1);
	}

	/* "round 3.7 to the nearest whole number", "round 1234 to the nearest
	 * hundred", "round 3.14159 to 2 decimal places" */
	if (HAS(READ, " round ") || HAS(READ, " rounded "))
	{
		double	STEP = 1;
		double	VALUE;
		char	TO[64] = "the nearest whole number";
		int		INDEX;
		int		AT = -1;
		int		ROUNDED_FORM = 0;

		/* "round 3.7", "round off 3.7", "round up 2.5" */
		for (INDEX = 0; INDEX + 1 < READ->COUNT && AT < 0; INDEX++)
			if (!strcmp(READ->TOKENS[INDEX].TEXT, "round"))
			{
				int	NEXT = INDEX + 1;

				if (
					!strcmp(TEXT_AT(READ, NEXT), "off") ||
					!strcmp(TEXT_AT(READ, NEXT), "up") ||
					!strcmp(TEXT_AT(READ, NEXT), "down")
				)
					NEXT++;

				if (NEXT < READ->COUNT && IS_NUMBER(&READ->TOKENS[NEXT]))
					AT = NEXT;
			}

		/* "what is 7.5 rounded to the nearest whole number?" */
		for (INDEX = 1; INDEX < READ->COUNT && AT < 0; INDEX++)
			if (
				!strcmp(READ->TOKENS[INDEX].TEXT, "rounded") &&
				IS_NUMBER(&READ->TOKENS[INDEX - 1])
			)
			{
				AT = INDEX - 1;
				ROUNDED_FORM = 1;
			}

		if (AT < 0)
			return (0);

		FIRST = READ->TOKENS[AT].VALUE;

		/* a bare "round 3.7" is the calculator's */
		if (
			!HAS(READ, " nearest ") &&
			!HAS(READ, " decimal place") &&
			!HAS(READ, " whole number ") &&
			!HAS(READ, " integer ")
		)
			return (0);

		if (HAS(READ, " nearest ten "))
		{
			STEP = 10;
			snprintf(TO, sizeof TO, "the nearest ten");
		}
		else if (HAS(READ, " nearest hundred "))
		{
			STEP = 100;
			snprintf(TO, sizeof TO, "the nearest hundred");
		}
		else if (HAS(READ, " nearest thousand "))
		{
			STEP = 1000;
			snprintf(TO, sizeof TO, "the nearest thousand");
		}
		else if (HAS(READ, " nearest tenth "))
		{
			STEP = 0.1;
			snprintf(TO, sizeof TO, "the nearest tenth");
		}
		else if (HAS(READ, " nearest hundredth "))
		{
			STEP = 0.01;
			snprintf(TO, sizeof TO, "the nearest hundredth");
		}
		else if (HAS(READ, " decimal place") && FOUND == 2)
		{
			STEP = pow(10, -SECOND);
			snprintf(
				TO, sizeof TO, "%g decimal place%s", SECOND,
				SECOND == 1 ? "" : "s"
			);
		}

		/* "round 3.7 to a whole number": the calculator's, like a bare
		 * "round 3.7"; "3.7 rounded to ..." it can't read */
		if (STEP == 1 && !ROUNDED_FORM && !HAS(READ, " nearest "))
			return (0);

		VALUE = floor(FIRST / STEP + 0.5) * STEP;
		SAY_NUMBER(FIRST, A, sizeof A);

		if (STEP < 1)
			snprintf(
				TEXT, sizeof TEXT, "%.*f", (int)floor(-log10(STEP) + 0.5), VALUE
			);
		else
			SAY_NUMBER(VALUE, TEXT, sizeof TEXT);

		snprintf(REPLY, REPLY_SIZE, "%s rounded to %s is %s.", A, TO, TEXT);
		snprintf(ACTION, ACTION_SIZE, "round %s -> %s", A, TEXT);
		return (1);
	}

	/* "the sum of the numbers from 1 to 10", "add up all the numbers from 1
	 * to 100" */
	if (
		(
			HAS(READ, " sum of ") ||
			HAS(READ, " add up ") ||
			HAS(READ, " total of ") ||
			HAS(READ, " add all ")
		) &&
		(HAS(READ, " from ") || HAS(READ, " between ")) &&
		FOUND == 2 &&
		(HAS(READ, " to ") || HAS(READ, " and ") || HAS(READ, " through ")) &&
		FIRST == floor(FIRST) &&
		SECOND == floor(SECOND) &&
		SECOND >= FIRST &&
		SECOND - FIRST < 1e9
	)
	{
		double	TOTAL = (FIRST + SECOND) * (SECOND - FIRST + 1) / 2;

		SAY_NUMBER(FIRST, A, sizeof A);
		SAY_NUMBER(SECOND, B, sizeof B);
		SAY_NUMBER(TOTAL, TEXT, sizeof TEXT);

		{
			char	COUNT[64];

			SAY_NUMBER(SECOND - FIRST + 1, COUNT, sizeof COUNT);
			snprintf(
				REPLY, REPLY_SIZE,
				"%s: there are %s numbers, and each pair from the two ends "
				"adds "
				"up to %s + %s, so (%s + %s) x %s / 2 = %s.",
				TEXT, COUNT, A, B, A, B, COUNT, TEXT
			);
		}

		snprintf(ACTION, ACTION_SIZE, "sum %s to %s -> %s", A, B, TEXT);
		return (1);
	}

	/* "the smallest prime number" */
	if (
		(
			HAS(READ, " smallest prime ") ||
			HAS(READ, " first prime ") ||
			HAS(READ, " lowest prime ")
		) &&
		FOUND == 0
	)
	{
		snprintf(
			REPLY, REPLY_SIZE,
			"2: it can only be divided by 1 and itself, and it is the only "
			"even "
			"prime."
		);
		snprintf(ACTION, ACTION_SIZE, "smallest prime -> 2");
		return (1);
	}

	/* "the largest prime below 100" */
	if (
		(
			HAS(READ, " largest prime ") ||
			HAS(READ, " biggest prime ") ||
			HAS(READ, " greatest prime ")
		) &&
		FOUND == 1 &&
		(
			HAS(READ, " below ") ||
			HAS(READ, " under ") ||
			HAS(READ, " less than ")
		) &&
		FIRST > 2 &&
		FIRST < 1e12
	)
	{
		long long	VALUE = (long long)ceil(FIRST) - 1;

		while (VALUE > 1 && !IS_PRIME_NUMBER(VALUE))
			VALUE--;

		SAY_NUMBER(FIRST, A, sizeof A);
		snprintf(TEXT, sizeof TEXT, "%lld", VALUE);
		snprintf(
			REPLY, REPLY_SIZE, "%s is the largest prime number below %s.", TEXT,
			A
		);
		snprintf(ACTION, ACTION_SIZE, "largest prime below %s -> %s", A, TEXT);
		return (1);
	}

	/* "the first 5 prime numbers", "list the prime numbers up to 30" */
	if (
		HAS(READ, " prime numbers ") &&
		FOUND == 1 &&
		FIRST >= 1 &&
		FIRST <= 200 &&
		(
			HAS(READ, " first ") ||
			HAS(READ, " up to ") ||
			HAS(READ, " below ") ||
			HAS(READ, " under ") ||
			HAS(READ, " less than ")
		)
	)
	{
		int			BY_COUNT = HAS(READ, " first ");
		long long	VALUE;
		int			COUNT = 0;
		int			LENGTH = 0;
		char		LIST[1200] = "";

		for (
			VALUE = 2;
			LENGTH < 1100 &&
				(BY_COUNT ? COUNT < FIRST
					: (HAS(READ, " up to ") ? VALUE <= FIRST
						: VALUE < FIRST));
			VALUE++
		)
			if (IS_PRIME_NUMBER(VALUE))
			{
				LENGTH += snprintf(
					LIST + LENGTH, sizeof LIST - LENGTH, "%s%lld",
					COUNT ? ", " : "", VALUE
				);
				COUNT++;
			}

		if (!COUNT)
			return (0);

		snprintf(REPLY, REPLY_SIZE, "%s.", LIST);
		snprintf(ACTION, ACTION_SIZE, "primes -> %d", COUNT);
		return (1);
	}

	/* "is 9 a square number?", "is 27 a perfect cube?" */
	if (
		(
			HAS(READ, " square number ") ||
			HAS(READ, " perfect square ") ||
			HAS(READ, " a square ") ||
			HAS(READ, " cube number ") ||
			HAS(READ, " perfect cube ") ||
			HAS(READ, " a cube ")
		) &&
		FOUND == 1 &&
		(HAS(READ, " is ") || HAS(READ, " are ")) &&
		FIRST == floor(FIRST) &&
		FIRST >= 0
	)
	{
		int			CUBE = HAS(READ, " cube");
		long long	ROOT =
			(long long)floor((CUBE ? cbrt(FIRST) : sqrt(FIRST)) + 0.5);
		long long	BACK;

		if (CUBE)
			BACK = ROOT * ROOT * ROOT;
		else
			BACK = ROOT * ROOT;

		SAY_NUMBER(FIRST, A, sizeof A);

		if ((double)BACK == FIRST)
		{
			if (CUBE)
				snprintf(
					REPLY, REPLY_SIZE,
					"Yes, %s is a cube number: %lld x %lld "
					"x %lld = %s.",
					A, ROOT, ROOT, ROOT, A
				);
			else
				snprintf(
					REPLY, REPLY_SIZE,
					"Yes, %s is a square number: %lld x "
					"%lld = %s.",
					A, ROOT, ROOT, A
				);
		}
		else
		{
			long long	LOW;

			if (CUBE)
				LOW = (long long)floor(cbrt(FIRST));
			else
				LOW = (long long)floor(sqrt(FIRST));

			if (CUBE)
				snprintf(
					REPLY, REPLY_SIZE,
					"No, %s is not a cube number: it falls "
					"between %lld and %lld.",
					A, LOW * LOW * LOW, (LOW + 1) * (LOW + 1) * (LOW + 1)
				);
			else
				snprintf(
					REPLY, REPLY_SIZE,
					"No, %s is not a square number: it "
					"falls between %lld and %lld.",
					A, LOW * LOW, (LOW + 1) * (LOW + 1)
				);
		}

		snprintf(
			ACTION, ACTION_SIZE, "%s %s -> %s", CUBE ? "cube" : "square", A,
			(double)BACK == FIRST ? "yes" : "no"
		);
		return (1);
	}

	/* "the greatest common divisor of 12 and 18", "lcm of 4 and 6" */
	if (
		FOUND == 2 &&
		FIRST == floor(FIRST) &&
		SECOND == floor(SECOND) &&
		FIRST > 0 &&
		SECOND > 0 &&
		FIRST < 1e12 &&
		SECOND < 1e12
	)
	{
		int	GCD = HAS(READ, " greatest common ") || HAS(READ, " gcd ") ||
			HAS(READ, " highest common ") || HAS(READ, " hcf ") ||
			HAS(READ, " gcf ");
		int	LCM = HAS(READ, " least common multiple ") || HAS(READ, " lcm ") ||
			HAS(READ, " lowest common multiple ") ||
			HAS(READ, " smallest common multiple ");

		if (GCD || LCM)
		{
			long long	X = (long long)FIRST;
			long long	Y = (long long)SECOND;
			long long	G = GREATEST_COMMON(X, Y);

			snprintf(A, sizeof A, "%lld", X);
			snprintf(B, sizeof B, "%lld", Y);

			if (GCD)
			{
				snprintf(TEXT, sizeof TEXT, "%lld", G);
				snprintf(
					REPLY, REPLY_SIZE,
					"%s: it is the biggest number that "
					"divides both %s and %s.",
					TEXT, A, B
				);
			}
			else
			{
				snprintf(TEXT, sizeof TEXT, "%lld", X / G * Y);
				snprintf(
					REPLY, REPLY_SIZE,
					"%s: it is the smallest number that "
					"both %s and %s go into.",
					TEXT, A, B
				);
			}

			snprintf(
				ACTION, ACTION_SIZE, "%s %s %s -> %s", GCD ? "gcd" : "lcm", A,
				B, TEXT
			);
			return (1);
		}
	}

	/* "the factors of 12", "the prime factors of 60" */
	if (
		(HAS(READ, " factors of ") || HAS(READ, " divisors of ")) &&
		FOUND == 1 &&
		FIRST == floor(FIRST) &&
		FIRST >= 1 &&
		FIRST <= 1e12
	)
	{
		long long	VALUE = (long long)FIRST;
		char		LIST[1200] = "";
		int			LENGTH = 0;

		snprintf(A, sizeof A, "%lld", VALUE);

		if (HAS(READ, " prime factors "))
		{
			long long	REST = VALUE;
			long long	DIVISOR;

			for (
				DIVISOR = 2;
				DIVISOR * DIVISOR <= REST && LENGTH < 1100;
				DIVISOR++
			)
				while (REST % DIVISOR == 0)
				{
					LENGTH += snprintf(
						LIST + LENGTH, sizeof LIST - LENGTH, "%s%lld",
						LENGTH ? " x " : "", DIVISOR
					);
					REST /= DIVISOR;
				}

			if (REST > 1)
				LENGTH += snprintf(
					LIST + LENGTH, sizeof LIST - LENGTH, "%s%lld",
					LENGTH ? " x " : "", REST
				);

			snprintf(REPLY, REPLY_SIZE, "%s = %s.", A, LIST);
			snprintf(ACTION, ACTION_SIZE, "prime factors %s -> %s", A, LIST);
			return (1);
		}

		if (VALUE > 1000000)
			return (0);

		{
			long long	DIVISOR;
			int			COUNT = 0;

			for (DIVISOR = 1; DIVISOR <= VALUE && LENGTH < 1100; DIVISOR++)
				if (VALUE % DIVISOR == 0)
				{
					LENGTH += snprintf(
						LIST + LENGTH, sizeof LIST - LENGTH, "%s%lld",
						COUNT ? ", " : "", DIVISOR
					);
					COUNT++;
				}

			snprintf(REPLY, REPLY_SIZE, "The factors of %s are %s.", A, LIST);
			snprintf(ACTION, ACTION_SIZE, "factors %s -> %d", A, COUNT);
			return (1);
		}
	}

	return (0);
}

/* ---------- "which is larger: 2/3 or 3/4?" ---------- */

static int
	ANSWER_COMPARE_FRACTIONS(
		const READING *READ, char *ACTION, int ACTION_SIZE, char *REPLY,
		int REPLY_SIZE
	)
{
	const TOKEN	*NUMBERS[2];
	int			FOUND = 0;
	int			INDEX;
	int			WANT_BIG;
	int			HAS_FRACTION = 0;

	if (!HAS(READ, " or "))
		return (0);

	if (
		HAS(READ, " larger ") ||
		HAS(READ, " bigger ") ||
		HAS(READ, " greater ") ||
		HAS(READ, " more ") ||
		HAS(READ, " largest ") ||
		HAS(READ, " biggest ") ||
		HAS(READ, " higher ")
	)
		WANT_BIG = 1;
	else if (
		HAS(READ, " smaller ") ||
		HAS(READ, " less ") ||
		HAS(READ, " smallest ") ||
		HAS(READ, " lower ") ||
		HAS(READ, " fewer ")
	)
		WANT_BIG = 0;
	else
		return (0);

	for (INDEX = 0; INDEX < READ->COUNT && FOUND < 3; INDEX++)
		if (IS_NUMBER(&READ->TOKENS[INDEX]))
		{
			if (FOUND < 2)
				NUMBERS[FOUND] = &READ->TOKENS[INDEX];

			HAS_FRACTION |= READ->TOKENS[INDEX].IS_FRACTION;
			FOUND++;
		}

	if (FOUND != 2 || !HAS_FRACTION)
		return (0);

	{
		const TOKEN	*WINNER;
		const TOKEN	*OTHER;
		char		WIN_VALUE[64];
		char		OTHER_VALUE[64];

		if (SAME(NUMBERS[0]->VALUE, NUMBERS[1]->VALUE))
		{
			snprintf(
				REPLY, REPLY_SIZE, "Neither: %s and %s are the same number.",
				NUMBERS[0]->ORIGINAL, NUMBERS[1]->ORIGINAL
			);
			snprintf(
				ACTION, ACTION_SIZE, "compare %s %s -> same",
				NUMBERS[0]->ORIGINAL, NUMBERS[1]->ORIGINAL
			);
			return (1);
		}

		if ((NUMBERS[0]->VALUE > NUMBERS[1]->VALUE) == WANT_BIG)
		{
			WINNER = NUMBERS[0];
			OTHER = NUMBERS[1];
		}
		else
		{
			WINNER = NUMBERS[1];
			OTHER = NUMBERS[0];
		}

		SAY_NUMBER(
			floor(WINNER->VALUE * 1000 + 0.5) / 1000, WIN_VALUE,
			sizeof WIN_VALUE
		);
		SAY_NUMBER(
			floor(OTHER->VALUE * 1000 + 0.5) / 1000, OTHER_VALUE,
			sizeof OTHER_VALUE
		);
		snprintf(
			REPLY, REPLY_SIZE, "%s is %s: %s is about %s and %s is about %s.",
			WINNER->ORIGINAL, WANT_BIG ? "larger" : "smaller", WINNER->ORIGINAL,
			WIN_VALUE, OTHER->ORIGINAL, OTHER_VALUE
		);
		snprintf(
			ACTION, ACTION_SIZE, "compare %s %s -> %s", NUMBERS[0]->ORIGINAL,
			NUMBERS[1]->ORIGINAL, WINNER->ORIGINAL
		);
		return (1);
	}
}

/* ---------- "what letter does apple start with?" ---------- */

static int
	ANSWER_START_LETTER(
		const READING *READ, char *ACTION, int ACTION_SIZE, char *REPLY,
		int REPLY_SIZE
	)
{
	char	WORD[64] = "";
	int		END;
	int		INDEX;

	if (
		!HAS(READ, " start with ") &&
		!HAS(READ, " begin with ") &&
		!HAS(READ, " end with ") &&
		!HAS(READ, " starts with ") &&
		!HAS(READ, " begins with ") &&
		!HAS(READ, " ends with ")
	)
		return (0);

	if (
		!HAS(READ, " what letter ") &&
		!HAS(READ, " which letter ") &&
		!HAS(READ, " what does ") &&
		!HAS(READ, " what sound ")
	)
		return (0);

	END = HAS(READ, " end with ") || HAS(READ, " ends with ");

	/* the word: in quotes, or after "does" / "the word" */
	{
		const char	*QUOTE = strchr(READ->MESSAGE, '"');

		if (QUOTE && strchr(QUOTE + 1, '"'))
			snprintf(
				WORD, sizeof WORD, "%.*s",
				(int)(strchr(QUOTE + 1, '"') - QUOTE - 1), QUOTE + 1
			);
		else
			for (INDEX = 0; INDEX + 1 < READ->COUNT; INDEX++)
				if (
					(
						!strcmp(READ->TOKENS[INDEX].TEXT, "does") ||
						!strcmp(READ->TOKENS[INDEX].TEXT, "word")
					) &&
					IS_WORD_TOKEN(READ, INDEX + 1) &&
					strcmp(TEXT_AT(READ, INDEX + 1), "the") &&
					strcmp(TEXT_AT(READ, INDEX + 1), "word")
				)
				{
					snprintf(
						WORD, sizeof WORD, "%s",
						READ->TOKENS[INDEX + 1].ORIGINAL
					);
					break ;
				}
	}

	if (!WORD[0] || strchr(WORD, ' '))
		return (0);

	{
		char	LETTER =
			(char)tolower((uint8_t)(END ? WORD[strlen(WORD) - 1] : WORD[0]));

		if (!isalpha((uint8_t)LETTER))
			return (0);

		snprintf(
			REPLY, REPLY_SIZE, "\"%s\" %s with %c.", WORD,
			END ? "ends" : "starts", LETTER
		);
		snprintf(
			ACTION, ACTION_SIZE, "%s letter of %s -> %c",
			END ? "last" : "first", WORD, LETTER
		);
		return (1);
	}
}

/* ---------- what Lucy's memory packages know ---------- */

static const SKILL_HOST	*CURRENT_HOST;

static int
	KNOW(const char *THING, const char *PROPERTY, char *VALUE, int SIZE)
{
	if (!CURRENT_HOST || CURRENT_HOST->VERSION < 2 || !CURRENT_HOST->KNOW)
		return (0);

	return (
		CURRENT_HOST->KNOW(CURRENT_HOST->ENGINE, THING, PROPERTY, VALUE, SIZE)
	);
}

static int
	THINGS_WITH(const char *PROPERTY, const char *VALUE, char *OUTPUT, int SIZE)
{
	if (OUTPUT && SIZE)
		OUTPUT[0] = 0;

	if (
		!CURRENT_HOST ||
		CURRENT_HOST->VERSION < 2 ||
		!CURRENT_HOST->THINGS_WITH
	)
		return (0);

	return (
		CURRENT_HOST->THINGS_WITH(
			CURRENT_HOST->ENGINE, PROPERTY, VALUE, OUTPUT, SIZE
		)
	);
}

/* the package that would know WORD: 1 when it is not installed yet */
static int
	PACKAGE_FOR(const char *WORD, char *OUTPUT, int SIZE)
{
	if (
		!CURRENT_HOST ||
		CURRENT_HOST->VERSION < 2 ||
		!CURRENT_HOST->PACKAGE_FOR
	)
		return (0);

	return (
		CURRENT_HOST->PACKAGE_FOR(CURRENT_HOST->ENGINE, WORD, OUTPUT, SIZE)
	);
}

static int
	HEARD_OF(const char *WORD)
{
	if (!CURRENT_HOST || CURRENT_HOST->VERSION < 2 || !CURRENT_HOST->HEARD_OF)
		return (0);

	return (CURRENT_HOST->HEARD_OF(CURRENT_HOST->ENGINE, WORD));
}

/* anything known about THING at all */
static int
	KNOWS_THING(const char *THING)
{
	char	VALUE[240];

	return (
		KNOW(THING, "is", VALUE, sizeof VALUE) ||
		KNOW(THING, "kind", VALUE, sizeof VALUE) ||
		KNOW(THING, "opposite", VALUE, sizeof VALUE)
	);
}

/* how Lucy says KEY: her mind/language/sayings.txt when she has one,
 * DEFAULT otherwise; {1} and {2} are FIRST and SECOND */
static void
	SAY_LINE(
		const char *KEY, const char *DEFAULT, const char *FIRST,
		const char *SECOND, char *OUTPUT, int OUTPUT_SIZE
	)
{
	const char	*CURSOR;
	int			POSITION = 0;

	if (CURRENT_HOST && CURRENT_HOST->VERSION >= 3 && CURRENT_HOST->SAY)
	{
		CURRENT_HOST->SAY(
			CURRENT_HOST->ENGINE, KEY, DEFAULT, FIRST, SECOND, OUTPUT,
			OUTPUT_SIZE
		);
		return ;
	}

	for (CURSOR = DEFAULT; *CURSOR && POSITION < OUTPUT_SIZE - 1; CURSOR++)
	{
		if (
			CURSOR[0] == '{' &&
			(CURSOR[1] == '1' || CURSOR[1] == '2') &&
			CURSOR[2] == '}'
		)
		{
			const char	*DETAIL = CURSOR[1] == '1' ? FIRST : SECOND;

			while (DETAIL && *DETAIL && POSITION < OUTPUT_SIZE - 1)
				OUTPUT[POSITION++] = *DETAIL++;

			CURSOR += 2;
			continue ;
		}

		OUTPUT[POSITION++] = *CURSOR;
	}

	OUTPUT[POSITION] = 0;
}

/* "I don't know France yet. Did you put places.mem in my memory folder?" */
static int
	SAY_PACKAGE_HINT(
		const char *THING, char *ACTION, int ACTION_SIZE, char *REPLY,
		int REPLY_SIZE
	)
{
	char	PACKAGE[96];

	if (
		KNOWS_THING(THING) ||
		HEARD_OF(THING) ||
		PACKAGE_FOR(THING, PACKAGE, sizeof PACKAGE) != 1
	)
		return (0);

	SAY_LINE(
		"package hint",
		"I don't have that information yet. Did you download the package "
		"{1} and put it in my mind/memory folder? Once it is there, I'll know "
		"it right away, without a restart.",
		PACKAGE, THING, REPLY, REPLY_SIZE
	);
	snprintf(ACTION, ACTION_SIZE, "package for %s -> %s", THING, PACKAGE);

	return (1);
}

/* "a cat", "cats", "the dog": the thing's name as the packages file it */
static void
	THING_AT(
		const READING *READ, int INDEX, int END, char *OUTPUT, int SIZE,
		char *SHOWN, int SHOWN_SIZE
	)
{
	int	LENGTH = 0;
	int	SHOWN_LENGTH = 0;

	OUTPUT[0] = 0;

	if (SHOWN && SHOWN_SIZE)
		SHOWN[0] = 0;

	while (
		INDEX < END &&
		(
			!strcmp(TEXT_AT(READ, INDEX), "a") ||
			!strcmp(TEXT_AT(READ, INDEX), "an") ||
			!strcmp(TEXT_AT(READ, INDEX), "the") ||
			!strcmp(TEXT_AT(READ, INDEX), "one")
		)
	)
		INDEX++;

	for (; INDEX < END && LENGTH < SIZE - 1; INDEX++)
	{
		if (READ->TOKENS[INDEX].KIND != TOKEN_WORD)
			break ;

		LENGTH += snprintf(
			OUTPUT + LENGTH, SIZE - LENGTH, "%s%s", LENGTH ? " " : "",
			READ->TOKENS[INDEX].TEXT
		);

		if (SHOWN && SHOWN_LENGTH < SHOWN_SIZE - 1)
			SHOWN_LENGTH += snprintf(
				SHOWN + SHOWN_LENGTH, SHOWN_SIZE - SHOWN_LENGTH, "%s%s",
				SHOWN_LENGTH ? " " : "", READ->TOKENS[INDEX].ORIGINAL
			);
	}
}

static const char	*PROPERTY_NAMES[50][2] = {
	/* what is asked -> the property in the packages */
	{ "legs", "legs" }, { "leg", "legs" },
	{ "wheels", "wheels" }, { "wheel", "wheels" },
	{ "sides", "sides" }, { "side", "sides" },
	{ "corners", "corners" }, { "angles", "angles" },
	{ "faces", "faces" }, { "edges", "edges" },
	{ "wings", "wings" }, { "arms", "arms" },
	{ "fingers", "fingers" }, { "toes", "toes" },
	{ "eyes", "eyes" }, { "ears", "ears" },
	{ "strings", "strings" }, { "keys", "keys" },
	{ "players", "players" }, { "days", "days" },
	{ "hours", "hours" }, { "minutes", "minutes" },
	{ "seconds", "seconds" }, { "weeks", "weeks" },
	{ "months", "months" }, { "years", "years" },
	{ "moons", "moons" }, { "planets", "planets" },
	{ "letters", "letters" }, { "vowels", "vowels" },
	{ "colors", "colors" }, { "colours", "colors" },
	{ "hands", "hands" }, { "continents", "continents" },
	{ "oceans", "oceans" }, { "seasons", "seasons" },
	{ "pedals", "pedals" }, { "color", "color" },
	{ "colour", "color" }, { "capital", "capital" },
	{ "language", "language" }, { "continent", "continent" },
	{ "country", "country" }, { "sound", "sound" },
	{ "noise", "sound" }, { "baby", "young" },
	{ "young", "young" }, { "opposite", "opposite" },
	{ "antonym", "opposite" }, { NULL, NULL }
};

static const char
	*PROPERTY_FOR(const char *WORD)
{
	int	INDEX;

	for (INDEX = 0; PROPERTY_NAMES[INDEX][0]; INDEX++)
		if (!strcmp(PROPERTY_NAMES[INDEX][0], WORD))
			return (PROPERTY_NAMES[INDEX][1]);

	return (NULL);
}

/* "3 cats" in "how many legs do 3 cats and a bird have": the count and the
 * thing, one by one; how many were read */
typedef struct
{
	double	COUNT;
	char	THING[64];
	char	SHOWN[64];
	int		GENERIC;
} COUNTED_THING;

static int
	READ_THING_LIST(
		const READING *READ, int FROM, int TO, COUNTED_THING *OUTPUT, int LIMIT
	)
{
	int	COUNT = 0;
	int	INDEX = FROM;

	while (INDEX < TO && COUNT < LIMIT)
	{
		double	HOW_MANY = 1;
		int		START;
		int		LENGTH = 0;

		while (
			INDEX < TO &&
			(
				!strcmp(TEXT_AT(READ, INDEX), "and") ||
				!strcmp(TEXT_AT(READ, INDEX), ",") ||
				!strcmp(TEXT_AT(READ, INDEX), "plus")
			)
		)
			INDEX++;

		if (INDEX >= TO)
			break ;

		OUTPUT[COUNT].GENERIC = 1;

		if (IS_NUMBER(&READ->TOKENS[INDEX]))
		{
			HOW_MANY = READ->TOKENS[INDEX].VALUE;
			OUTPUT[COUNT].GENERIC = 0;
			INDEX++;
		}
		else if (
			!strcmp(TEXT_AT(READ, INDEX), "a") ||
			!strcmp(TEXT_AT(READ, INDEX), "an") ||
			!strcmp(TEXT_AT(READ, INDEX), "one") ||
			!strcmp(TEXT_AT(READ, INDEX), "the")
		)
		{
			OUTPUT[COUNT].GENERIC = 0;
			INDEX++;
		}

		START = INDEX;
		OUTPUT[COUNT].THING[0] = 0;
		OUTPUT[COUNT].SHOWN[0] = 0;

		while (
			INDEX < TO &&
			READ->TOKENS[INDEX].KIND == TOKEN_WORD &&
			strcmp(TEXT_AT(READ, INDEX), "and") &&
			strcmp(TEXT_AT(READ, INDEX), "plus") &&
			INDEX - START < 3
		)
		{
			LENGTH += snprintf(
				OUTPUT[COUNT].THING + LENGTH, 64 - LENGTH, "%s%s",
				LENGTH ? " " : "", READ->TOKENS[INDEX].TEXT
			);
			INDEX++;
		}

		if (INDEX == START)
			return (0);

		SPAN_TEXT(READ, START, INDEX, OUTPUT[COUNT].SHOWN, 64);
		OUTPUT[COUNT].COUNT = HOW_MANY;

		/* "spiders" with no number or article: spiders in general */
		OUTPUT[COUNT].GENERIC = OUTPUT[COUNT].GENERIC &&
			IS_PLURAL_WORD(READ->TOKENS[INDEX - 1].TEXT);
		COUNT++;
	}

	return (COUNT);
}

/* "How many legs does a spider have?", "How many legs do 3 cats and a bird
 * have?", "How many sides does a triangle have?" */
static int
	ANSWER_THING_COUNT(
		const READING *READ, char *ACTION, int ACTION_SIZE, char *REPLY,
		int REPLY_SIZE
	)
{
	int				INDEX;
	int				HOW_AT = -1;
	const char		*PROPERTY;
	int				VERB_AT = -1;
	int				HAVE_AT = -1;
	COUNTED_THING	THINGS[8];
	int				THING_COUNT;
	double			TOTAL = 0;
	char			WORK[600] = "";
	int				WORK_LENGTH = 0;
	char			TEXT[64];
	int				NAMED_ONE = 0;

	for (INDEX = 0; INDEX + 2 < READ->COUNT; INDEX++)
		if (
			!strcmp(READ->TOKENS[INDEX].TEXT, "how") &&
			!strcmp(READ->TOKENS[INDEX + 1].TEXT, "many")
		)
		{
			HOW_AT = INDEX;
			break ;
		}

	if (HOW_AT < 0)
		return (0);

	PROPERTY = PROPERTY_FOR(TEXT_AT(READ, HOW_AT + 2));

	/* only what can be counted: legs, wheels, sides ... */
	if (
		!PROPERTY ||
		!strcmp(PROPERTY, "color") ||
		!strcmp(PROPERTY, "capital") ||
		!strcmp(PROPERTY, "language") ||
		!strcmp(PROPERTY, "continent") ||
		!strcmp(PROPERTY, "country") ||
		!strcmp(PROPERTY, "sound") ||
		!strcmp(PROPERTY, "young") ||
		!strcmp(PROPERTY, "opposite")
	)
		return (0);

	/* "do 3 cats have", "does a spider have", "has a car got", "are on a
	 * car", "are in a week" */
	for (INDEX = HOW_AT + 3; INDEX < READ->COUNT; INDEX++)
	{
		const char	*WORD = READ->TOKENS[INDEX].TEXT;

		if (
			VERB_AT < 0 &&
			(
				!strcmp(WORD, "do") ||
				!strcmp(WORD, "does") ||
				!strcmp(WORD, "has") ||
				!strcmp(WORD, "are") ||
				!strcmp(WORD, "is") ||
				!strcmp(WORD, "did")
			)
		)
			VERB_AT = INDEX;
		else if (
			VERB_AT >= 0 &&
			(
				!strcmp(WORD, "have") ||
				!strcmp(WORD, "got") ||
				!strcmp(WORD, "there") ||
				READ->TOKENS[INDEX].KIND == TOKEN_SYMBOL
			)
		)
		{
			HAVE_AT = INDEX;
			break ;
		}
	}

	if (VERB_AT < 0)
		return (0);

	if (HAVE_AT < 0)
		HAVE_AT = READ->COUNT;

	/* "how many days are in a week": the thing after "in" / "on" */
	{
		int	FROM = VERB_AT + 1;

		if (
			!strcmp(TEXT_AT(READ, VERB_AT), "are") ||
			!strcmp(TEXT_AT(READ, VERB_AT), "is")
		)
		{
			if (!strcmp(TEXT_AT(READ, FROM), "there"))
				FROM++;

			if (
				strcmp(TEXT_AT(READ, FROM), "in") &&
				strcmp(TEXT_AT(READ, FROM), "on") &&
				strcmp(TEXT_AT(READ, FROM), "of")
			)
				return (0);

			FROM++;
			HAVE_AT = READ->COUNT;

			for (INDEX = FROM; INDEX < READ->COUNT; INDEX++)
				if (READ->TOKENS[INDEX].KIND == TOKEN_SYMBOL)
				{
					HAVE_AT = INDEX;
					break ;
				}
		}

		THING_COUNT = READ_THING_LIST(READ, FROM, HAVE_AT, THINGS, 8);
	}

	if (!THING_COUNT)
		return (0);

	/* "how many moons does mars have?": "does" asks about one thing, even
	 * when its name ends in s, and a name without "a" or "the" is said
	 * without one ("Mars has 2 moons.") */
	if (
		THING_COUNT == 1 &&
		THINGS[0].GENERIC &&
		(
			!strcmp(TEXT_AT(READ, VERB_AT), "does") ||
			!strcmp(TEXT_AT(READ, VERB_AT), "has")
		)
	)
	{
		THINGS[0].GENERIC = 0;
		NAMED_ONE = 1;
	}

	for (INDEX = 0; INDEX < THING_COUNT; INDEX++)
	{
		char	VALUE[240];
		char	EACH[64];
		char	ITEM[64];
		double	AMOUNT;
		char	*END;

		if (!KNOW(THINGS[INDEX].THING, PROPERTY, VALUE, sizeof VALUE))
		{
			/* a thing nobody packed: say which package would know */
			if (!KNOWS_THING(THINGS[INDEX].THING))
				return (
					SAY_PACKAGE_HINT(
						THINGS[INDEX].THING, ACTION, ACTION_SIZE, REPLY,
						REPLY_SIZE
					)
				);

			return (0);
		}

		AMOUNT = strtod(VALUE, &END);

		if (END == VALUE || *END)
		{
			/* "28 to 31", "about 4": said as it is, for one thing */
			if (THING_COUNT == 1 && THINGS[0].COUNT == 1)
			{
				snprintf(
					REPLY, REPLY_SIZE, "%c%s %s %s %s.",
					toupper((uint8_t)THINGS[0].SHOWN[0]), THINGS[0].SHOWN + 1,
					!strcmp(TEXT_AT(READ, VERB_AT), "are") ? "has" : "has",
					VALUE, TEXT_AT(READ, HOW_AT + 2)
				);
				snprintf(
					ACTION, ACTION_SIZE, "know %s %s -> %s", THINGS[0].THING,
					PROPERTY, VALUE
				);
				return (1);
			}

			return (0);
		}

		TOTAL += AMOUNT * THINGS[INDEX].COUNT;
		SAY_NUMBER(AMOUNT, EACH, sizeof EACH);
		SAY_NUMBER(THINGS[INDEX].COUNT, ITEM, sizeof ITEM);

		if (THINGS[INDEX].COUNT == 1)
			WORK_LENGTH += snprintf(
				WORK + WORK_LENGTH, sizeof WORK - WORK_LENGTH, "%s%s %s has %s",
				WORK_LENGTH ? ", " : "", ARTICLE(THINGS[INDEX].SHOWN),
				THINGS[INDEX].SHOWN, EACH
			);
		else
		{
			char	PRODUCT[64];

			SAY_NUMBER(AMOUNT * THINGS[INDEX].COUNT, PRODUCT, sizeof PRODUCT);
			WORK_LENGTH += snprintf(
				WORK + WORK_LENGTH, sizeof WORK - WORK_LENGTH,
				"%seach of the %s %s has %s, and %s x %s = %s",
				WORK_LENGTH ? ", " : "", ITEM, THINGS[INDEX].SHOWN, EACH, ITEM,
				EACH, PRODUCT
			);
		}
	}

	SAY_NUMBER(TOTAL, TEXT, sizeof TEXT);

	if (THING_COUNT == 1 && THINGS[0].GENERIC)
		snprintf(
			REPLY, REPLY_SIZE, "%c%s have %s %s.",
			toupper((uint8_t)THINGS[0].SHOWN[0]), THINGS[0].SHOWN + 1, TEXT,
			TEXT_AT(READ, HOW_AT + 2)
		);
	else if (THING_COUNT == 1 && NAMED_ONE)
		snprintf(
			REPLY, REPLY_SIZE, "%c%s has %s %s.",
			toupper((uint8_t)THINGS[0].SHOWN[0]), THINGS[0].SHOWN + 1, TEXT,
			TOTAL == 1 ? PROPERTY : TEXT_AT(READ, HOW_AT + 2)
		);
	else if (THING_COUNT == 1 && THINGS[0].COUNT == 1)
		snprintf(
			REPLY, REPLY_SIZE, "%c%s %s has %s %s.",
			toupper((uint8_t)ARTICLE(THINGS[0].SHOWN)[0]),
			ARTICLE(THINGS[0].SHOWN) + 1, THINGS[0].SHOWN, TEXT,
			TOTAL == 1 ? PROPERTY : TEXT_AT(READ, HOW_AT + 2)
		);
	else if (THING_COUNT == 1)
		snprintf(
			REPLY, REPLY_SIZE, "%s %s: %s.", TEXT, TEXT_AT(READ, HOW_AT + 2),
			WORK
		);
	else
		snprintf(
			REPLY, REPLY_SIZE, "%s %s: %s, so %s in all.", TEXT,
			TEXT_AT(READ, HOW_AT + 2), WORK, TEXT
		);

	/* "A snake has 0 legs" reads better as "no legs" */
	if (TOTAL == 0 && THING_COUNT == 1 && THINGS[0].GENERIC)
		snprintf(
			REPLY, REPLY_SIZE, "%c%s have no %s.",
			toupper((uint8_t)THINGS[0].SHOWN[0]), THINGS[0].SHOWN + 1,
			TEXT_AT(READ, HOW_AT + 2)
		);
	else if (TOTAL == 0 && THING_COUNT == 1)
		snprintf(
			REPLY, REPLY_SIZE, "%c%s %s has no %s.",
			toupper((uint8_t)ARTICLE(THINGS[0].SHOWN)[0]),
			ARTICLE(THINGS[0].SHOWN) + 1, THINGS[0].SHOWN,
			TEXT_AT(READ, HOW_AT + 2)
		);

	snprintf(
		ACTION, ACTION_SIZE, "know %s %s -> %s", THINGS[0].THING, PROPERTY, TEXT
	);

	return (1);
}

/* "What color is a banana?", "What is the capital of France?", "What sound
 * does a cow make?", "What is a baby cat called?", "What is the opposite
 * of hot?" */
static int
	ANSWER_THING_PROPERTY(
		const READING *READ, char *ACTION, int ACTION_SIZE, char *REPLY,
		int REPLY_SIZE
	)
{
	char		THING[64] = "";
	char		SHOWN[64] = "";
	const char	*PROPERTY = NULL;
	char		VALUE[240];
	int			INDEX;
	int			END = READ->COUNT;
	int			THE = 0;
	int			BARE = 0;

	for (INDEX = 0; INDEX < READ->COUNT; INDEX++)
		if (
			READ->TOKENS[INDEX].KIND == TOKEN_SYMBOL &&
			strchr("?.!", READ->TOKENS[INDEX].TEXT[0])
		)
		{
			END = INDEX;
			break ;
		}

	for (INDEX = 0; INDEX + 1 < END && !PROPERTY; INDEX++)
	{
		const char	*WORD = READ->TOKENS[INDEX].TEXT;

		/* "what is the capital of France", "what's the opposite of hot",
		 * "the opposite of up" */
		if (
			!strcmp(WORD, "of") &&
			INDEX > 0 &&
			PROPERTY_FOR(READ->TOKENS[INDEX - 1].TEXT) &&
			(
				HAS(READ, " what ") ||
				HAS(READ, " what's ") ||
				HAS(READ, " whats ") ||
				HAS(READ, " tell me ") ||
				HAS(READ, " name ") ||
				INDEX == 2
			)
		)
		{
			PROPERTY = PROPERTY_FOR(READ->TOKENS[INDEX - 1].TEXT);
			THING_AT(
				READ, INDEX + 1, END, THING, sizeof THING, SHOWN, sizeof SHOWN
			);
		}

		/* "what color is a banana", "what sound does a cow make", "what
		 * language is spoken in Spain" */
		else if (
			(!strcmp(WORD, "what") || !strcmp(WORD, "which")) &&
			PROPERTY_FOR(TEXT_AT(READ, INDEX + 1)) &&
			(
				!strcmp(TEXT_AT(READ, INDEX + 2), "is") ||
				!strcmp(TEXT_AT(READ, INDEX + 2), "are") ||
				!strcmp(TEXT_AT(READ, INDEX + 2), "does") ||
				!strcmp(TEXT_AT(READ, INDEX + 2), "do")
			)
		)
		{
			int	FROM = INDEX + 3;
			int	TO = END;

			PROPERTY = PROPERTY_FOR(TEXT_AT(READ, INDEX + 1));
			THE = !strcmp(TEXT_AT(READ, FROM), "the");
			BARE = strcmp(TEXT_AT(READ, FROM), "a") &&
				strcmp(TEXT_AT(READ, FROM), "an") && !THE;

			/* "make", "have" at the end belong to the question */
			while (
				TO > FROM &&
				(
					!strcmp(TEXT_AT(READ, TO - 1), "make") ||
					!strcmp(TEXT_AT(READ, TO - 1), "have") ||
					!strcmp(TEXT_AT(READ, TO - 1), "usually") ||
					!strcmp(TEXT_AT(READ, TO - 1), "normally")
				)
			)
				TO--;

			THING_AT(READ, FROM, TO, THING, sizeof THING, SHOWN, sizeof SHOWN);
		}

		/* "what is a baby cat called", "what do you call a baby dog" */
		else if (
			!strcmp(WORD, "baby") ||
			(
				!strcmp(WORD, "young") &&
				IS_WORD_TOKEN(READ, INDEX + 1) &&
				HAS(READ, " called ")
			)
		)
		{
			int	TO = INDEX + 1;

			while (
				TO < END &&
				READ->TOKENS[TO].KIND == TOKEN_WORD &&
				strcmp(TEXT_AT(READ, TO), "called") &&
				strcmp(TEXT_AT(READ, TO), "is") &&
				strcmp(TEXT_AT(READ, TO), "named")
			)
				TO++;

			if (
				(
					HAS(READ, " called ") ||
					HAS(READ, " call ") ||
					HAS(READ, " name ") ||
					HAS(READ, " named ")
				) &&
				TO > INDEX + 1
			)
			{
				PROPERTY = "young";
				THING_AT(
					READ, INDEX + 1, TO, THING, sizeof THING, SHOWN,
					sizeof SHOWN
				);
			}
		}
	}

	if (!PROPERTY || !THING[0])
		return (0);

	if (!KNOW(THING, PROPERTY, VALUE, sizeof VALUE))
	{
		if (!KNOWS_THING(THING))
			return (
				SAY_PACKAGE_HINT(THING, ACTION, ACTION_SIZE, REPLY, REPLY_SIZE)
			);

		return (0);
	}

	if (!strcmp(PROPERTY, "opposite") && strchr(VALUE, ','))
	{
		char	*COMMA = strchr(VALUE, ',');

		*COMMA = 0;
		snprintf(
			REPLY, REPLY_SIZE, "The opposite of %s is %s (or%s).", SHOWN, VALUE,
			COMMA + 1
		);
	}
	else if (!strcmp(PROPERTY, "opposite"))
		snprintf(REPLY, REPLY_SIZE, "The opposite of %s is %s.", SHOWN, VALUE);
	else if (!strcmp(PROPERTY, "young"))
		snprintf(
			REPLY, REPLY_SIZE, "A baby %s is called %s %s.", SHOWN,
			ARTICLE(VALUE), VALUE
		);
	else if (!strcmp(PROPERTY, "sound"))
		snprintf(
			REPLY, REPLY_SIZE, "%c%s %s says \"%s\".",
			toupper((uint8_t)ARTICLE(SHOWN)[0]), ARTICLE(SHOWN) + 1, SHOWN,
			VALUE
		);
	else if (!strcmp(PROPERTY, "capital"))
		snprintf(REPLY, REPLY_SIZE, "The capital of %s is %s.", SHOWN, VALUE);
	else if (!strcmp(PROPERTY, "color") && THE)
		snprintf(REPLY, REPLY_SIZE, "The %s is %s.", SHOWN, VALUE);
	else if (!strcmp(PROPERTY, "color") && BARE)
		snprintf(
			REPLY, REPLY_SIZE, "%c%s %s %s.", toupper((uint8_t)SHOWN[0]),
			SHOWN + 1, IS_PLURAL_WORD(THING) ? "are" : "is", VALUE
		);
	else if (!strcmp(PROPERTY, "color"))
		snprintf(
			REPLY, REPLY_SIZE, "%c%s %s is usually %s.",
			toupper((uint8_t)ARTICLE(SHOWN)[0]), ARTICLE(SHOWN) + 1, SHOWN,
			VALUE
		);
	else
		snprintf(
			REPLY, REPLY_SIZE, "The %s of %s is %s.", PROPERTY, SHOWN, VALUE
		);

	/* "The opposite of Hot is cold" keeps the asker's spelling but not a
	 * capital in the middle */
	snprintf(ACTION, ACTION_SIZE, "know %s %s -> %s", THING, PROPERTY, VALUE);

	return (1);
}

/* "Is a whale a fish?", "Can a penguin fly?", "Is a tomato a fruit?" */
/* where "a" or "an" comes again from FROM on, -1 when it doesn't */
static int
	SECOND_ARTICLE(const READING *READ, int FROM, int END)
{
	int	INDEX;

	for (INDEX = FROM; INDEX < END; INDEX++)
		if (
			!strcmp(TEXT_AT(READ, INDEX), "a") ||
			!strcmp(TEXT_AT(READ, INDEX), "an")
		)
			return (INDEX);

	return (-1);
}

static int
	ANSWER_THING_IS(
		const READING *READ, char *ACTION, int ACTION_SIZE, char *REPLY,
		int REPLY_SIZE
	)
{
	char	THING[64];
	char	SHOWN[64];
	int		AT;
	int		END = READ->COUNT;
	int		INDEX;

	if (READ->COUNT < 4 || READ->TOKENS[READ->COUNT - 1].TEXT[0] != '?')
		return (0);

	/* the question is the last sentence */
	for (AT = READ->COUNT - 2; AT > 0; AT--)
		if (
			READ->TOKENS[AT].KIND == TOKEN_SYMBOL &&
			strchr(".!?", READ->TOKENS[AT].TEXT[0])
		)
			break ;

	if (AT > 0)
		AT++;

	END = READ->COUNT - 1;

	/* "is a whale a fish", "are whales fish", "is a tomato a fruit or a
	 * vegetable" ("is a stone hard?" names no kind: it is asked below) */
	if (
		(
			!strcmp(TEXT_AT(READ, AT), "is") ||
			!strcmp(TEXT_AT(READ, AT), "are")
		) &&
		(
			!strcmp(TEXT_AT(READ, AT + 1), "a") ||
			!strcmp(TEXT_AT(READ, AT + 1), "an")
		) &&
		SECOND_ARTICLE(READ, AT + 3, END) >= 0
	)
	{
		int		KIND_AT = -1;
		char	KIND[64];
		char	KINDS[240];

		for (INDEX = AT + 3; INDEX < END; INDEX++)
			if (
				!strcmp(TEXT_AT(READ, INDEX), "a") ||
				!strcmp(TEXT_AT(READ, INDEX), "an")
			)
			{
				KIND_AT = INDEX;
				break ;
			}

		if (KIND_AT < 0)
			return (0);

		THING_AT(
			READ, AT + 1, KIND_AT, THING, sizeof THING, SHOWN, sizeof SHOWN
		);

		{
			int	KIND_END = KIND_AT + 1;

			while (
				KIND_END < END &&
				READ->TOKENS[KIND_END].KIND == TOKEN_WORD &&
				strcmp(TEXT_AT(READ, KIND_END), "or")
			)
				KIND_END++;

			THING_AT(READ, KIND_AT, KIND_END, KIND, sizeof KIND, NULL, 0);
		}

		if (!THING[0] || !KIND[0])
			return (0);

		if (!KNOW(THING, "kind", KINDS, sizeof KINDS))
		{
			if (!KNOWS_THING(THING))
				return (
					SAY_PACKAGE_HINT(
						THING, ACTION, ACTION_SIZE, REPLY, REPLY_SIZE
					)
				);

			return (0);
		}

		/* only kinds the packages know: "is a cat a hunter?" stays open */
		if (!THINGS_WITH("kind", KIND, NULL, 0))
			return (0);

		{
			char	PADDED[260];
			char	WANTED[80];
			char	*FIRST_KIND;
			char	*COMMA;

			snprintf(PADDED, sizeof PADDED, ", %s,", KINDS);
			snprintf(WANTED, sizeof WANTED, ", %s,", KIND);

			/* the kind that tells most: "a mammal", not "an animal" */
			FIRST_KIND = strrchr(KINDS, ',');

			if (FIRST_KIND)
				FIRST_KIND = FIRST_KIND + 2;
			else
				FIRST_KIND = KINDS;

			if (
				!strcmp(FIRST_KIND, "pet") ||
				!strcmp(FIRST_KIND, "food") ||
				!strcmp(FIRST_KIND, "person") ||
				!strcmp(FIRST_KIND, "place")
			)
			{
				COMMA = strchr(KINDS, ',');

				if (COMMA && COMMA[1])
				{
					char	SECOND[64];

					snprintf(SECOND, sizeof SECOND, "%s", COMMA + 2);

					if (strchr(SECOND, ','))
						*strchr(SECOND, ',') = 0;

					snprintf(KINDS, sizeof KINDS, "%s", SECOND);
					FIRST_KIND = KINDS;
				}
			}

			/* "a fruit or a vegetable?" is not a yes-or-no question */
			if (strstr(PADDED, WANTED) && HAS(READ, " or "))
			{
				char	NOTE[240] = "";

				KNOW(THING, "note", NOTE, sizeof NOTE);
				snprintf(
					REPLY, REPLY_SIZE, "%c%s %s is %s %s%s%s.",
					toupper((uint8_t)ARTICLE(SHOWN)[0]), ARTICLE(SHOWN) + 1,
					SHOWN, ARTICLE(KIND), KIND, NOTE[0] ? ", " : "", NOTE
				);

				/* "a fruit, a fruit though ..." says it once */
				if (
					NOTE[0] &&
					!strncmp(NOTE, "a ", 2) &&
					strstr(NOTE, KIND) == NOTE + 2
				)
					snprintf(
						REPLY, REPLY_SIZE, "%c%s %s is %s.",
						toupper((uint8_t)ARTICLE(SHOWN)[0]), ARTICLE(SHOWN) + 1,
						SHOWN, NOTE
					);
			}
			else if (strstr(PADDED, WANTED))
				snprintf(
					REPLY, REPLY_SIZE, "Yes, %s %s is %s %s.", ARTICLE(SHOWN),
					SHOWN, ARTICLE(KIND), KIND
				);
			else
				snprintf(
					REPLY, REPLY_SIZE, "No, %s %s is %s %s, not %s %s.",
					ARTICLE(SHOWN), SHOWN, ARTICLE(FIRST_KIND), FIRST_KIND,
					ARTICLE(KIND), KIND
				);

			snprintf(
				ACTION, ACTION_SIZE, "know %s kind %s -> %s", THING, KIND,
				strstr(PADDED, WANTED) ? "yes" : "no"
			);
			return (1);
		}
	}

	/* "can a penguin fly", "can fish swim" */
	if (!strcmp(TEXT_AT(READ, AT), "can"))
	{
		int		VERB_AT = -1;
		char	CAN[240] = "";
		char	CANNOT[240] = "";
		char	VERB[64];
		char	PADDED_VERB[80];
		char	PADDED[260];
		char				OBJECT[200] = "";
		int					MANY;
		static const char	*ABILITIES[25] = {
			"fly", "swim", "run", "climb", "jump", "talk", "walk", "sing",
			"read", "write", "speak", "drive", "bark", "meow", "breathe", "dig",
			"crawl", "hop", "dance", "think", "see", "hear", "laugh", "cry",
			NULL
		};

		/* "can a fish climb a tree": the first word after the thing that
		 * names something done, then what it is done to */
		for (INDEX = AT + 2; INDEX < END && VERB_AT < 0; INDEX++)
			if (
				READ->TOKENS[INDEX].KIND == TOKEN_WORD &&
				IN_LIST(TEXT_AT(READ, INDEX), ABILITIES)
			)
				VERB_AT = INDEX;

		if (VERB_AT < 0)
			for (INDEX = AT + 2; INDEX < END; INDEX++)
				if (READ->TOKENS[INDEX].KIND == TOKEN_WORD)
					VERB_AT = INDEX;

		if (
			VERB_AT < 0 ||
			(VERB_AT != END - 1 && !IN_LIST(TEXT_AT(READ, VERB_AT), ABILITIES))
		)
			return (0);

		if (VERB_AT + 1 < END)
			SPAN_TEXT(READ, VERB_AT + 1, END, OBJECT, sizeof OBJECT);

		THING_AT(
			READ, AT + 1, VERB_AT, THING, sizeof THING, SHOWN, sizeof SHOWN
		);

		if (!THING[0])
			return (0);

		snprintf(VERB, sizeof VERB, "%s", TEXT_AT(READ, VERB_AT));

		/* "can a fish": one fish; "can fish": fish in general */
		MANY = IS_PLURAL_WORD(THING) && strcmp(TEXT_AT(READ, AT + 1), "a") &&
			strcmp(TEXT_AT(READ, AT + 1), "an");
		snprintf(PADDED_VERB, sizeof PADDED_VERB, ", %s,", VERB);
		KNOW(THING, "can", CAN, sizeof CAN);
		KNOW(THING, "cannot", CANNOT, sizeof CANNOT);
		snprintf(PADDED, sizeof PADDED, ", %s,", CANNOT);

		if (strstr(PADDED, PADDED_VERB))
		{
			snprintf(
				REPLY, REPLY_SIZE, "No, %s %s can't %s.",
				MANY ? "" : ARTICLE(SHOWN), SHOWN, VERB
			);

			if (REPLY[4] == ' ')
				memmove(REPLY + 4, REPLY + 5, strlen(REPLY + 5) + 1);

			snprintf(ACTION, ACTION_SIZE, "know %s can %s -> no", THING, VERB);
			return (1);
		}

		snprintf(PADDED, sizeof PADDED, ", %s,", CAN);

		if (strstr(PADDED, PADDED_VERB))
		{
			snprintf(
				REPLY, REPLY_SIZE, "Yes, %s %s can %s.",
				MANY ? "" : ARTICLE(SHOWN), SHOWN, VERB
			);

			if (REPLY[5] == ' ')
				memmove(REPLY + 5, REPLY + 6, strlen(REPLY + 6) + 1);

			snprintf(ACTION, ACTION_SIZE, "know %s can %s -> yes", THING, VERB);
			return (1);
		}

		/* "can a fish fly?": it can swim, and flying is not one of its
		 * abilities */
		if (
			CAN[0] &&
			IN_LIST(
				VERB,
				(const char *[13]){ "fly", "swim", "run", "climb", "jump",
									"talk", "walk", "sing", "read", "write",
									"speak", "drive", NULL }
			)
		)
		{
			char	WHAT_IT_CAN[260];
			char	*COMMA;

			snprintf(WHAT_IT_CAN, sizeof WHAT_IT_CAN, "%s", CAN);
			COMMA = strrchr(WHAT_IT_CAN, ',');

			if (COMMA)
			{
				char	LAST[120];

				snprintf(LAST, sizeof LAST, "%s", COMMA + 1);
				snprintf(
					COMMA, sizeof WHAT_IT_CAN - (size_t)(COMMA - WHAT_IT_CAN),
					" and%s", LAST
				);
			}

			snprintf(
				REPLY, REPLY_SIZE, "No, %s %s can't %s%s%s; %s can %s.",
				MANY ? "" : ARTICLE(SHOWN), SHOWN, VERB, OBJECT[0] ? " " : "",
				OBJECT, MANY ? "they" : "it", WHAT_IT_CAN
			);

			if (REPLY[4] == ' ')
				memmove(REPLY + 4, REPLY + 5, strlen(REPLY + 5) + 1);

			snprintf(ACTION, ACTION_SIZE, "know %s can %s -> no", THING, VERB);
			return (1);
		}
	}

	/* "is fire hot?", "are lemons sour?", "is snow white?" */
	if (
		(
			!strcmp(TEXT_AT(READ, AT), "is") ||
			!strcmp(TEXT_AT(READ, AT), "are")
		) &&
		END - AT >= 3 &&
		END - AT <= 5 &&
		READ->TOKENS[END - 1].KIND == TOKEN_WORD
	)
	{
		const char	*WANTED = TEXT_AT(READ, END - 1);
		char		TRAITS[240] = "";
		char		COLORS[240] = "";
		char		OPPOSITES[240] = "";
		char		WANTED_KINDS[240] = "";
		char		PADDED[300];
		char		PADDED_WANTED[80];
		int			MANY;

		THING_AT(
			READ, AT + 1, END - 1, THING, sizeof THING, SHOWN, sizeof SHOWN
		);

		if (!THING[0] || !strcmp(THING, WANTED))
			return (0);

		MANY = !strcmp(TEXT_AT(READ, AT), "are");
		KNOW(THING, "traits", TRAITS, sizeof TRAITS);
		KNOW(THING, "color", COLORS, sizeof COLORS);
		snprintf(PADDED, sizeof PADDED, ", %s, %s,", TRAITS, COLORS);
		snprintf(PADDED_WANTED, sizeof PADDED_WANTED, ", %s,", WANTED);

		/* the thing as it was asked: "the sun", "a stone", "lemons" */
		SPAN_TEXT(READ, AT + 1, END - 1, SHOWN, sizeof SHOWN);

		if (
			SHOWN[0] &&
			isupper((uint8_t)SHOWN[0]) &&
			(SHOWN[1] == ' ' || islower((uint8_t)SHOWN[1])) &&
			!KNOWS_THING(SHOWN)
		)
			SHOWN[0] = (char)tolower((uint8_t)SHOWN[0]);

		if ((TRAITS[0] || COLORS[0]) && strstr(PADDED, PADDED_WANTED))
		{
			snprintf(
				REPLY, REPLY_SIZE, "Yes, %s %s %s.", SHOWN, MANY ? "are" : "is",
				WANTED
			);
			snprintf(
				ACTION, ACTION_SIZE, "know %s is %s -> yes", THING, WANTED
			);
			return (1);
		}

		/* "is ice hot?": no, it is the opposite */
		if (TRAITS[0] && KNOW(WANTED, "opposite", OPPOSITES, sizeof OPPOSITES))
		{
			char	*WORD;
			char	*SAVE = NULL;
			char	LIST[240];

			snprintf(LIST, sizeof LIST, "%s", OPPOSITES);

			for (
				WORD = strtok_r(LIST, ", ", &SAVE);
				WORD;
				WORD = strtok_r(NULL, ", ", &SAVE)
			)
			{
				char	PADDED_OPPOSITE[80];

				snprintf(
					PADDED_OPPOSITE, sizeof PADDED_OPPOSITE, ", %s,", WORD
				);

				if (strstr(PADDED, PADDED_OPPOSITE))
				{
					snprintf(
						REPLY, REPLY_SIZE, "No, %s %s %s, not %s.", SHOWN,
						MANY ? "are" : "is", WORD, WANTED
					);
					snprintf(
						ACTION, ACTION_SIZE, "know %s is %s -> no", THING,
						WANTED
					);
					return (1);
				}
			}
		}

		/* "is snow red?": no, snow is white */
		if (
			COLORS[0] &&
			KNOW(WANTED, "kind", WANTED_KINDS, sizeof WANTED_KINDS) &&
			strstr(WANTED_KINDS, "color")
		)
		{
			snprintf(
				REPLY, REPLY_SIZE, "No, %s %s %s.", SHOWN, MANY ? "are" : "is",
				COLORS
			);
			snprintf(
				ACTION, ACTION_SIZE, "know %s color %s -> no", THING, WANTED
			);
			return (1);
		}
	}

	return (0);
}

/* "Hot is to cold as up is to what?" */
static int
	ANSWER_ANALOGY(
		const READING *READ, char *ACTION, int ACTION_SIZE, char *REPLY,
		int REPLY_SIZE
	)
{
	static const char	*RELATIONS[15] = {
		"opposite", "young", "sound", "female", "male", "capital", "country",
		"color", "can", "eats", "next", "before", "home", "lives", NULL
	};
	const char			*WORDS[3] = { NULL, NULL, NULL };
	const char			*SHOWN[3] = { NULL, NULL, NULL };
	int					FOUND = 0;
	int					INDEX;

	if (!HAS(READ, " is to ") || !HAS(READ, " as "))
		return (0);

	/* "A is to B as C is to" */
	for (INDEX = 0; INDEX + 3 < READ->COUNT && FOUND < 3; INDEX++)
		if (
			!strcmp(READ->TOKENS[INDEX + 1].TEXT, "is") &&
			!strcmp(READ->TOKENS[INDEX + 2].TEXT, "to") &&
			READ->TOKENS[INDEX].KIND == TOKEN_WORD
		)
		{
			if (FOUND == 0)
			{
				WORDS[0] = READ->TOKENS[INDEX].TEXT;
				SHOWN[0] = READ->TOKENS[INDEX].ORIGINAL;

				if (READ->TOKENS[INDEX + 3].KIND != TOKEN_WORD)
					return (0);

				WORDS[1] = READ->TOKENS[INDEX + 3].TEXT;
				SHOWN[1] = READ->TOKENS[INDEX + 3].ORIGINAL;
				FOUND = 2;
			}
			else if (FOUND == 2)
			{
				WORDS[2] = READ->TOKENS[INDEX].TEXT;
				SHOWN[2] = READ->TOKENS[INDEX].ORIGINAL;
				FOUND = 3;
			}
		}

	if (FOUND != 3)
		return (0);

	for (INDEX = 0; RELATIONS[INDEX]; INDEX++)
	{
		char	VALUE[240];
		char	ANSWER[240];
		char	PADDED[260];
		char	WANTED[80];

		if (!KNOW(WORDS[0], RELATIONS[INDEX], VALUE, sizeof VALUE))
			continue ;

		/* "Hot" first in the message is the word hot */
		if (
			strcmp(RELATIONS[INDEX], "country") &&
			strcmp(RELATIONS[INDEX], "capital")
		)
			SHOWN[0] = WORDS[0];

		snprintf(PADDED, sizeof PADDED, ", %s,", VALUE);
		snprintf(WANTED, sizeof WANTED, ", %s,", WORDS[1]);

		{
			char	*CURSOR;

			for (CURSOR = PADDED; *CURSOR; CURSOR++)
				*CURSOR = (char)tolower((uint8_t)*CURSOR);
		}

		if (!strstr(PADDED, WANTED))
			continue ;

		if (!KNOW(WORDS[2], RELATIONS[INDEX], ANSWER, sizeof ANSWER))
			continue ;

		/* the first of several: "light" -> "dark, heavy" */
		if (strchr(ANSWER, ','))
			*strchr(ANSWER, ',') = 0;

		{
			const char	*RELATION = RELATIONS[INDEX];

			/* "Paris is to France": Paris is the capital of France */
			if (!strcmp(RELATION, "country"))
				snprintf(
					REPLY, REPLY_SIZE,
					"%c%s: %s is the capital of %s, and %s "
					"is the capital of %s.",
					toupper((uint8_t)ANSWER[0]), ANSWER + 1, SHOWN[0], SHOWN[1],
					SHOWN[2], ANSWER
				);
			else if (!strcmp(RELATION, "young"))
				snprintf(
					REPLY, REPLY_SIZE,
					"%c%s: a baby %s is %s %s, and a baby "
					"%s is %s %s.",
					toupper((uint8_t)ANSWER[0]), ANSWER + 1, WORDS[0],
					ARTICLE(WORDS[1]), WORDS[1], WORDS[2], ARTICLE(ANSWER),
					ANSWER
				);
			/* "bird is to fly as fish is to": swim */
			else if (
				!strcmp(RELATION, "can") ||
				!strcmp(RELATION, "eats") ||
				!strcmp(RELATION, "lives")
			)
				snprintf(
					REPLY, REPLY_SIZE,
					"%c%s: %s %s %s%s %s, and %s %s %s%s "
					"%s.",
					toupper((uint8_t)ANSWER[0]), ANSWER + 1, ARTICLE(WORDS[0]),
					WORDS[0],
					!strcmp(RELATION, "can") ? "can"
						: !strcmp(RELATION, "eats") ? "eats"
													: "lives",
					!strcmp(RELATION, "lives") ? " in" : "", WORDS[1],
					ARTICLE(WORDS[2]), WORDS[2],
					!strcmp(RELATION, "can") ? "can"
						: !strcmp(RELATION, "eats") ? "eats"
													: "lives",
					!strcmp(RELATION, "lives") ? " in" : "", ANSWER
				);
			else
			{
				const char	*HOW = !strcmp(RELATION, "opposite")
					? "the opposite of"
					: !strcmp(RELATION, "sound") ? "the sound of"
					: !strcmp(RELATION, "capital") ? "the capital of"
					: !strcmp(RELATION, "next") ? "what comes after"
					: !strcmp(RELATION, "before") ? "what comes before"
					: !strcmp(RELATION, "color") ? "the color of"
				: "to";

				snprintf(
					REPLY, REPLY_SIZE, "%c%s: %s is %s %s, and %s is %s %s.",
					toupper((uint8_t)ANSWER[0]), ANSWER + 1, SHOWN[1], HOW,
					SHOWN[0], ANSWER, HOW, SHOWN[2]
				);
			}

			snprintf(ACTION, ACTION_SIZE, "analogy %s -> %s", RELATION, ANSWER);
			return (1);
		}
	}

	/* a word no package has yet: which package would */
	for (INDEX = 0; INDEX < 3; INDEX++)
		if (!KNOWS_THING(WORDS[INDEX]))
			return (
				SAY_PACKAGE_HINT(
					WORDS[INDEX], ACTION, ACTION_SIZE, REPLY, REPLY_SIZE
				)
			);

	return (0);
}

/* "List three colors.", "Name 5 animals.", "Give me two fruits." */
static int
	ANSWER_LIST_KIND(
		const READING *READ, char *ACTION, int ACTION_SIZE, char *REPLY,
		int REPLY_SIZE
	)
{
	int		INDEX;
	int		WANT = 0;
	char	KIND[64];
	char	ALL[4000];
	int		COUNT;

	if (
		!HAS(READ, " list ") &&
		!HAS(READ, " name ") &&
		!HAS(READ, " give me ") &&
		!HAS(READ, " tell me ") &&
		!HAS(READ, " what are some ") &&
		!HAS(READ, " can you name ") &&
		!HAS(READ, " say ")
	)
		return (0);

	/* the order itself: "list 3 colors", "name five animals", "give me two
	 * fruits", "can you tell me 4 shapes", "what are some 3 colors" */
	for (INDEX = 0; INDEX + 2 < READ->COUNT; INDEX++)
	{
		const char	*WORD = READ->TOKENS[INDEX].TEXT;
		int			AT = INDEX + 1;

		if (
			strcmp(WORD, "list") &&
			strcmp(WORD, "name") &&
			strcmp(WORD, "give") &&
			strcmp(WORD, "tell") &&
			strcmp(WORD, "say") &&
			strcmp(WORD, "some")
		)
			continue ;

		/* the verb starts the message, or follows "can you", "please" */
		if (
			INDEX > 0 &&
			strcmp(TEXT_AT(READ, INDEX - 1), "you") &&
			strcmp(TEXT_AT(READ, INDEX - 1), "please") &&
			strcmp(TEXT_AT(READ, INDEX - 1), "are") &&
			READ->TOKENS[INDEX - 1].KIND != TOKEN_SYMBOL
		)
			continue ;

		if (!strcmp(TEXT_AT(READ, AT), "me"))
			AT++;

		if (
			AT + 1 < READ->COUNT &&
			IS_NUMBER(&READ->TOKENS[AT]) &&
			READ->TOKENS[AT + 1].KIND == TOKEN_WORD &&
			READ->TOKENS[AT].VALUE >= 1 &&
			READ->TOKENS[AT].VALUE <= 20 &&
			READ->TOKENS[AT].VALUE == floor(READ->TOKENS[AT].VALUE)
		)
		{
			WANT = (int)READ->TOKENS[AT].VALUE;
			THING_AT(READ, AT + 1, READ->COUNT, KIND, sizeof KIND, NULL, 0);
			break ;
		}
	}

	if (!WANT || !KIND[0] || READ->COUNT > 14)
		return (0);

	/* "colors please": the kind is the first word or two */
	{
		char	*SPACE = strchr(KIND, ' ');

		if (SPACE)
		{
			char	TWO[64];

			snprintf(TWO, sizeof TWO, "%s", KIND);

			if (!THINGS_WITH("kind", TWO, NULL, 0))
				*SPACE = 0;
		}
	}

	COUNT = THINGS_WITH("kind", KIND, ALL, sizeof ALL);

	if (COUNT < WANT)
		return (0);

	{
		char	LIST[1200] = "";
		int		LENGTH = 0;
		char	*ITEM;
		char	*SAVED;
		int		TAKEN = 0;

		for (
			ITEM = strtok_r(ALL, ",", &SAVED);
			ITEM && TAKEN < WANT;
			ITEM = strtok_r(NULL, ",", &SAVED)
		)
		{
			while (*ITEM == ' ')
				ITEM++;

			LENGTH += snprintf(
				LIST + LENGTH, sizeof LIST - LENGTH, "%s%s",
				TAKEN == 0 ? ""
					: TAKEN == WANT - 1 ? " and "
										: ", ",
				ITEM
			);
			TAKEN++;
		}

		snprintf(
			REPLY, REPLY_SIZE, "%c%s.", toupper((uint8_t)LIST[0]), LIST + 1
		);
		snprintf(ACTION, ACTION_SIZE, "list %d %s -> %s", WANT, KIND, LIST);
		return (1);
	}
}

/* "Which one doesn't belong: apple, banana, carrot, grape?" */
static int
	ANSWER_ODD_ONE_OUT(
		const READING *READ, char *ACTION, int ACTION_SIZE, char *REPLY,
		int REPLY_SIZE
	)
{
	char	WORDS[8][64];
	char	KINDS[8][240];
	int		COUNT = 0;
	int		INDEX;
	int		START = -1;

	if (
		!HAS(READ, " belong") &&
		!HAS(READ, " odd one ") &&
		!HAS(READ, " is different ") &&
		!HAS(READ, " does not fit ") &&
		!HAS(READ, " doesn't fit ") &&
		!HAS(READ, " not like the others ")
	)
		return (0);

	/* the list: after a colon, or the longest run of words with commas */
	for (INDEX = 0; INDEX < READ->COUNT; INDEX++)
		if (!strcmp(READ->TOKENS[INDEX].TEXT, ":"))
			START = INDEX + 1;

	if (START < 0)
		for (INDEX = 1; INDEX < READ->COUNT; INDEX++)
			if (!strcmp(READ->TOKENS[INDEX].TEXT, ",") && START < 0)
				START = INDEX - 1;

	if (START < 0)
		return (0);

	for (INDEX = START; INDEX < READ->COUNT && COUNT < 8; INDEX++)
	{
		const TOKEN	*ITEM = &READ->TOKENS[INDEX];

		if (
			ITEM->KIND != TOKEN_WORD ||
			!strcmp(ITEM->TEXT, "and") ||
			!strcmp(ITEM->TEXT, "or")
		)
			continue ;

		snprintf(WORDS[COUNT], 64, "%s", ITEM->TEXT);

		if (!KNOW(WORDS[COUNT], "kind", KINDS[COUNT], 240))
			return (0);

		COUNT++;
	}

	if (COUNT < 3)
		return (0);

	/* a kind all but one share */
	for (INDEX = 0; INDEX < COUNT; INDEX++)
	{
		char	LIST[240];
		char	*KIND;
		char	*SAVED;

		snprintf(LIST, sizeof LIST, "%s", KINDS[(INDEX + 1) % COUNT]);

		for (
			KIND = strtok_r(LIST, ",", &SAVED);
			KIND;
			KIND = strtok_r(NULL, ",", &SAVED)
		)
		{
			char	WANTED[80];
			int		OTHER;
			int		SHARED = 1;
			char	PADDED[260];

			while (*KIND == ' ')
				KIND++;

			if (
				!strcmp(KIND, "food") ||
				!strcmp(KIND, "animal") ||
				!strcmp(KIND, "person")
			)
				continue ;

			snprintf(WANTED, sizeof WANTED, ", %s,", KIND);

			for (OTHER = 0; OTHER < COUNT && SHARED; OTHER++)
			{
				snprintf(PADDED, sizeof PADDED, ", %s,", KINDS[OTHER]);

				if ((OTHER == INDEX) == (strstr(PADDED, WANTED) != NULL))
					SHARED = 0;
			}

			if (SHARED)
			{
				char	OWN[240];
				char	*OWN_KIND;

				/* its first kind tells it apart: "an animal" among colors */
				snprintf(OWN, sizeof OWN, "%s", KINDS[INDEX]);
				OWN_KIND = OWN;

				if (strchr(OWN, ','))
					*strchr(OWN, ',') = 0;

				snprintf(
					REPLY, REPLY_SIZE,
					"%c%s: it is %s %s, and the others are %ss.",
					toupper((uint8_t)WORDS[INDEX][0]), WORDS[INDEX] + 1,
					ARTICLE(OWN_KIND), OWN_KIND, KIND
				);
				snprintf(
					ACTION, ACTION_SIZE, "odd one out -> %s", WORDS[INDEX]
				);
				return (1);
			}
		}
	}

	return (0);
}

/* the BACK-th newest line of the recent messages (1 is the newest); 0 when
 * there are not that many */
static int
	RECENT_LINE(const char *RECENT, int BACK, char *OUTPUT, int SIZE)
{
	const char	*LINES[16];
	int			COUNT = 0;
	const char	*CURSOR = RECENT;
	const char	*END;

	/* the last 16 lines are kept */
	while (*CURSOR)
	{
		if (COUNT == 16)
		{
			memmove(LINES, LINES + 1, 15 * sizeof LINES[0]);
			COUNT = 15;
		}

		LINES[COUNT++] = CURSOR;
		END = strchr(CURSOR, '\n');

		if (!END)
			break ;

		CURSOR = END + 1;
	}

	if (BACK < 1 || BACK > COUNT)
		return (0);

	CURSOR = LINES[COUNT - BACK];
	END = strchr(CURSOR, '\n');
	snprintf(
		OUTPUT, SIZE, "%.*s", END ? (int)(END - CURSOR) : (int)strlen(CURSOR),
		CURSOR
	);

	return (OUTPUT[0] != 0);
}

/* "John has a red car. What color is John's car?" (in this message or one
 * a moment ago) */
static int
	ANSWER_STORY_DETAIL(
		const READING *READ, char *ACTION, int ACTION_SIZE, char *REPLY,
		int REPLY_SIZE
	)
{
	static READING	STORY;
	char			RECENT[6000] = "";
	char			OWNER[64] = "";
	char			OWNER_SHOWN[64] = "";
	char			NOUN[64] = "";
	const char		*PROPERTY = NULL;
	int				INDEX;
	int				PASS;

	/* "what color is John's car", "what color is the ball" */
	for (INDEX = 0; INDEX + 3 < READ->COUNT; INDEX++)
		if (
			!strcmp(READ->TOKENS[INDEX].TEXT, "what") &&
			PROPERTY_FOR(READ->TOKENS[INDEX + 1].TEXT) &&
			(
				!strcmp(READ->TOKENS[INDEX + 2].TEXT, "is") ||
				!strcmp(READ->TOKENS[INDEX + 2].TEXT, "was")
			)
		)
		{
			const char	*WORD = READ->TOKENS[INDEX + 3].TEXT;
			size_t		LENGTH = strlen(WORD);

			PROPERTY = PROPERTY_FOR(READ->TOKENS[INDEX + 1].TEXT);

			if (
				LENGTH > 2 &&
				!strcmp(WORD + LENGTH - 2, "'s") &&
				IS_WORD_TOKEN(READ, INDEX + 4)
			)
			{
				snprintf(OWNER, sizeof OWNER, "%.*s", (int)LENGTH - 2, WORD);
				snprintf(
					OWNER_SHOWN, sizeof OWNER_SHOWN, "%.*s",
					(int)strlen(READ->TOKENS[INDEX + 3].ORIGINAL) - 2,
					READ->TOKENS[INDEX + 3].ORIGINAL
				);
				snprintf(NOUN, sizeof NOUN, "%s", READ->TOKENS[INDEX + 4].TEXT);
			}
			else if (
				(
					!strcmp(WORD, "the") ||
					!strcmp(WORD, "my") ||
					!strcmp(WORD, "his") ||
					!strcmp(WORD, "her")
				) &&
				IS_WORD_TOKEN(READ, INDEX + 4)
			)
			{
				snprintf(OWNER, sizeof OWNER, "%s", WORD);
				snprintf(OWNER_SHOWN, sizeof OWNER_SHOWN, "%s", WORD);
				snprintf(NOUN, sizeof NOUN, "%s", READ->TOKENS[INDEX + 4].TEXT);
			}

			break ;
		}

	if (!PROPERTY || !NOUN[0] || strcmp(PROPERTY, "color"))
		return (0);

	if (CURRENT_HOST && CURRENT_HOST->VERSION >= 2 && CURRENT_HOST->RECENT)
		CURRENT_HOST->RECENT(CURRENT_HOST->ENGINE, RECENT, sizeof RECENT);

	/* this message first, then the ones before it, newest first */
	for (PASS = 0; PASS < 9; PASS++)
	{
		const READING	*TEXT_READ = READ;

		if (PASS >= 1)
		{
			char	LINE[1200];

			if (!RECENT_LINE(RECENT, PASS, LINE, sizeof LINE))
				break ;

			/* "my car", "the ball" from earlier messages are the memory's to
			 * answer; a named owner ("John's car") is read here */
			if (
				!strcmp(OWNER, "my") ||
				!strcmp(OWNER, "the") ||
				!strcmp(OWNER, "his") ||
				!strcmp(OWNER, "her")
			)
				break ;

			READ_MESSAGE(LINE, &STORY);
			TEXT_READ = &STORY;
		}

		for (INDEX = TEXT_READ->COUNT - 1; INDEX >= 1; INDEX--)
		{
			char	KINDS[240];
			int		ADJECTIVE;

			if (strcmp(TEXT_READ->TOKENS[INDEX].TEXT, NOUN))
				continue ;

			/* "John has a red car", "his red car", "a big red car" */
			for (
				ADJECTIVE = INDEX - 1;
				ADJECTIVE >= 0 && ADJECTIVE >= INDEX - 3;
				ADJECTIVE--
			)
			{
				const char	*WORD = TEXT_READ->TOKENS[ADJECTIVE].TEXT;
				char		PADDED[260];

				if (TEXT_READ->TOKENS[ADJECTIVE].KIND != TOKEN_WORD)
					break ;

				if (!KNOW(WORD, "kind", KINDS, sizeof KINDS))
					continue ;

				snprintf(PADDED, sizeof PADDED, ", %s,", KINDS);

				if (!strstr(PADDED, ", color,"))
					continue ;

				/* the owner must match when one was asked */
				if (
					strcmp(OWNER, "the") &&
					strcmp(OWNER, "my") &&
					strcmp(OWNER, "his") &&
					strcmp(OWNER, "her")
				)
				{
					int	SCAN;
					int	OWNED = 0;

					for (
						SCAN = ADJECTIVE - 1;
						SCAN >= 0 && SCAN >= ADJECTIVE - 4;
						SCAN--
					)
						if (
							!strcmp(TEXT_AT(TEXT_READ, SCAN), OWNER) ||
							(
								!strncmp(
									TEXT_AT(TEXT_READ, SCAN), OWNER,
									strlen(OWNER)
								) &&
								!strcmp(
									TEXT_AT(TEXT_READ, SCAN) + strlen(OWNER),
									"'s"
								)
							)
						)
							OWNED = 1;

					if (!OWNED)
						continue ;
				}

				if (!strcmp(OWNER, "my"))
					snprintf(REPLY, REPLY_SIZE, "Your %s is %s.", NOUN, WORD);
				else if (
					!strcmp(OWNER, "the") ||
					!strcmp(OWNER, "his") ||
					!strcmp(OWNER, "her")
				)
					snprintf(
						REPLY, REPLY_SIZE, "%c%s %s is %s.",
						toupper((uint8_t)OWNER[0]), OWNER + 1, NOUN, WORD
					);
				else
					snprintf(
						REPLY, REPLY_SIZE, "%s's %s is %s.", OWNER_SHOWN, NOUN,
						WORD
					);

				snprintf(
					ACTION, ACTION_SIZE, "story %s %s -> %s", NOUN, PROPERTY,
					WORD
				);
				return (1);
			}

			/* "the ball is blue" */
			if (
				INDEX + 2 < TEXT_READ->COUNT &&
				(
					!strcmp(TEXT_AT(TEXT_READ, INDEX + 1), "is") ||
					!strcmp(TEXT_AT(TEXT_READ, INDEX + 1), "was")
				) &&
				KNOW(TEXT_AT(TEXT_READ, INDEX + 2), "kind", KINDS, sizeof KINDS)
			)
			{
				char	PADDED[260];

				snprintf(PADDED, sizeof PADDED, ", %s,", KINDS);

				if (strstr(PADDED, ", color,"))
				{
					snprintf(
						REPLY, REPLY_SIZE, "It is %s.",
						TEXT_AT(TEXT_READ, INDEX + 2)
					);
					snprintf(
						ACTION, ACTION_SIZE, "story %s %s -> %s", NOUN,
						PROPERTY, TEXT_AT(TEXT_READ, INDEX + 2)
					);
					return (1);
				}
			}
		}
	}

	return (0);
}

/* "I have 3 apples and my sister has 5. ... How many apples do we have
 * together?" */
static int
	ANSWER_TOGETHER(
		const READING *READ, char *ACTION, int ACTION_SIZE, char *REPLY,
		int REPLY_SIZE
	)
{
	static READING	STORY;
	char			RECENT[6000] = "";
	char			NOUN[64] = "";
	char			OWNERS[8][64];
	double			COUNTS[8];
	int				OWNER_COUNT = 0;
	int				INDEX;
	int				PASS;

	if (
		!HAS(READ, " how many ") ||
		!(
			HAS(READ, " together ") ||
			HAS(READ, " in total ") ||
			HAS(READ, " altogether ") ||
			HAS(READ, " combined ") ||
			HAS(READ, " all together ") ||
			HAS(READ, " do we have ") ||
			HAS(READ, " in all ")
		)
	)
		return (0);

	for (INDEX = 0; INDEX + 2 < READ->COUNT; INDEX++)
		if (
			!strcmp(READ->TOKENS[INDEX].TEXT, "how") &&
			!strcmp(READ->TOKENS[INDEX + 1].TEXT, "many") &&
			IS_WORD_TOKEN(READ, INDEX + 2)
		)
		{
			SINGULAR(READ->TOKENS[INDEX + 2].TEXT, NOUN, sizeof NOUN);
			break ;
		}

	if (!NOUN[0])
		return (0);

	if (CURRENT_HOST && CURRENT_HOST->VERSION >= 2 && CURRENT_HOST->RECENT)
		CURRENT_HOST->RECENT(CURRENT_HOST->ENGINE, RECENT, sizeof RECENT);

	/* "I have 3 apples", "my sister has 5 apples", "Tom has 4": the newest
	 * count each owner said, from the recent messages (oldest first), then
	 * this one */
	for (PASS = 8; PASS >= 0; PASS--)
	{
		const READING	*TEXT_READ = READ;

		if (PASS >= 1)
		{
			char	LINE[1200];

			if (!RECENT_LINE(RECENT, PASS, LINE, sizeof LINE))
				continue ;

			READ_MESSAGE(LINE, &STORY);
			TEXT_READ = &STORY;
		}

		for (INDEX = 1; INDEX + 1 < TEXT_READ->COUNT; INDEX++)
		{
			const char	*VERB = TEXT_READ->TOKENS[INDEX - 1].TEXT;
			char		NOUN_HERE[64] = "";
			char		OWNER[64];
			int			OWNER_AT;
			int			FOUND;

			if (
				!IS_NUMBER(&TEXT_READ->TOKENS[INDEX]) ||
				(
					strcmp(VERB, "have") &&
					strcmp(VERB, "has") &&
					strcmp(VERB, "had")
				)
			)
				continue ;

			if (IS_WORD_TOKEN(TEXT_READ, INDEX + 1))
				SINGULAR(
					TEXT_READ->TOKENS[INDEX + 1].TEXT, NOUN_HERE,
					sizeof NOUN_HERE
				);

			if (strcmp(NOUN_HERE, NOUN))
				continue ;

			OWNER_AT = INDEX - 2;

			if (OWNER_AT < 0 || TEXT_READ->TOKENS[OWNER_AT].KIND != TOKEN_WORD)
				continue ;

			if (
				OWNER_AT > 0 &&
				(
					!strcmp(TEXT_AT(TEXT_READ, OWNER_AT - 1), "my") ||
					!strcmp(TEXT_AT(TEXT_READ, OWNER_AT - 1), "his") ||
					!strcmp(TEXT_AT(TEXT_READ, OWNER_AT - 1), "her")
				)
			)
				snprintf(
					OWNER, sizeof OWNER, "%s %s",
					TEXT_AT(TEXT_READ, OWNER_AT - 1),
					TEXT_READ->TOKENS[OWNER_AT].ORIGINAL
				);
			else
				snprintf(
					OWNER, sizeof OWNER, "%s",
					TEXT_READ->TOKENS[OWNER_AT].ORIGINAL
				);

			for (FOUND = 0; FOUND < OWNER_COUNT; FOUND++)
				if (!strcasecmp(OWNERS[FOUND], OWNER))
					break ;

			if (FOUND == OWNER_COUNT)
			{
				if (OWNER_COUNT == 8)
					continue ;

				snprintf(OWNERS[OWNER_COUNT++], 64, "%s", OWNER);
			}

			COUNTS[FOUND] = TEXT_READ->TOKENS[INDEX].VALUE;
		}
	}

	if (OWNER_COUNT < 2)
		return (0);

	{
		double	TOTAL = 0;
		char	TEXT[64];
		char	WORK[800] = "";
		int		LENGTH = 0;

		for (INDEX = 0; INDEX < OWNER_COUNT; INDEX++)
		{
			char	AMOUNT[64];
			char	WHO[80];

			TOTAL += COUNTS[INDEX];
			SAY_NUMBER(COUNTS[INDEX], AMOUNT, sizeof AMOUNT);

			if (!strcasecmp(OWNERS[INDEX], "i"))
				snprintf(WHO, sizeof WHO, "you have");
			else if (!strncasecmp(OWNERS[INDEX], "my ", 3))
				snprintf(WHO, sizeof WHO, "your %s has", OWNERS[INDEX] + 3);
			else
				snprintf(WHO, sizeof WHO, "%s has", OWNERS[INDEX]);

			LENGTH += snprintf(
				WORK + LENGTH, sizeof WORK - LENGTH, "%s%s %s",
				INDEX == 0 ? ""
					: INDEX == OWNER_COUNT - 1 ? " and "
				: ", ",
				WHO, AMOUNT
			);
		}

		SAY_NUMBER(TOTAL, TEXT, sizeof TEXT);
		snprintf(REPLY, REPLY_SIZE, "%s %ss: %s.", TEXT, NOUN, WORK);
		snprintf(ACTION, ACTION_SIZE, "calc total %s -> %s", NOUN, TEXT);
		return (1);
	}
}

/* ---------- "a pencil costs 2 dollars, how much do 6 pencils cost" ----- */

static int
	ANSWER_UNIT_PRICE(
		const READING *READ, char *ACTION, int ACTION_SIZE, char *REPLY,
		int REPLY_SIZE
	)
{
	char	ITEM[64] = "";
	double	PRICE = 0;
	double	UNIT_FROM = 1;
	char	SIGN[16] = "";
	int		INDEX;
	int		PRICE_AT;

	if (!HAS(READ, " cost") && !HAS(READ, " price") && !HAS(READ, " how much "))
		return (0);

	PRICE_AT = PRICE_IN(READ, &PRICE, SIGN, sizeof SIGN, -1);

	if (PRICE_AT < 0)
		return (0);

	/* "a pencil costs $2", "one pencil is 2 dollars", "pencils cost $2 each" */
	for (INDEX = 1; INDEX < PRICE_AT; INDEX++)
		if (
			(
				!strcmp(READ->TOKENS[INDEX].TEXT, "costs") ||
				!strcmp(READ->TOKENS[INDEX].TEXT, "cost") ||
				!strcmp(READ->TOKENS[INDEX].TEXT, "is")
			) &&
			READ->TOKENS[INDEX - 1].KIND == TOKEN_WORD &&
			INDEX + 3 >= PRICE_AT
		)
		{
			SINGULAR(READ->TOKENS[INDEX - 1].TEXT, ITEM, sizeof ITEM);
			break ;
		}

	if (!ITEM[0])
		return (0);

	/* "3 pens cost $6": the price is for all of them, unless "each" */
	{
		int		ITEM_AT;
		double	BOUGHT = 1;

		for (ITEM_AT = 1; ITEM_AT < PRICE_AT; ITEM_AT++)
		{
			char	NOUN[64];

			SINGULAR(READ->TOKENS[ITEM_AT].TEXT, NOUN, sizeof NOUN);

			if (!strcmp(NOUN, ITEM) && IS_NUMBER(&READ->TOKENS[ITEM_AT - 1]))
			{
				BOUGHT = READ->TOKENS[ITEM_AT - 1].VALUE;
				break ;
			}
		}

		if (
			BOUGHT > 1 &&
			!(
				PRICE_AT + 1 < READ->COUNT &&
				(
					!strcmp(READ->TOKENS[PRICE_AT + 1].TEXT, "each") ||
					!strcmp(READ->TOKENS[PRICE_AT + 1].TEXT, "apiece") ||
					!strcmp(READ->TOKENS[PRICE_AT + 1].TEXT, "per")
				)
			)
		)
			UNIT_FROM = BOUGHT;
	}

	/* "how much do 6 pencils cost", "how much are 6 pencils" */
	for (INDEX = PRICE_AT + 1; INDEX + 1 < READ->COUNT; INDEX++)
	{
		char	NOUN[64];

		if (!IS_NUMBER(&READ->TOKENS[INDEX]) || !IS_WORD_TOKEN(READ, INDEX + 1))
			continue ;

		SINGULAR(READ->TOKENS[INDEX + 1].TEXT, NOUN, sizeof NOUN);

		if (strcmp(NOUN, ITEM))
			continue ;

		{
			char	TOTAL[64];
			char	EACH[64];
			char	COUNT[64];
			char	GIVEN[64];
			char	GIVEN_COUNT[64];
			double	EACH_PRICE = PRICE / UNIT_FROM;

			SAY_MONEY(
				EACH_PRICE * READ->TOKENS[INDEX].VALUE, SIGN, TOTAL,
				sizeof TOTAL
			);
			SAY_MONEY(EACH_PRICE, SIGN, EACH, sizeof EACH);
			SAY_MONEY(PRICE, SIGN, GIVEN, sizeof GIVEN);
			SAY_NUMBER(READ->TOKENS[INDEX].VALUE, COUNT, sizeof COUNT);
			SAY_NUMBER(UNIT_FROM, GIVEN_COUNT, sizeof GIVEN_COUNT);

			if (UNIT_FROM > 1)
			{
				snprintf(
					REPLY, REPLY_SIZE,
					"%s: one %s costs %s / %s = %s, so %s "
					"%s cost %s x %s = %s.",
					TOTAL, ITEM, GIVEN, GIVEN_COUNT, EACH, COUNT,
					READ->TOKENS[INDEX + 1].TEXT, COUNT, EACH, TOTAL
				);
				snprintf(
					ACTION, ACTION_SIZE, "calc %s / %s * %s -> %s", GIVEN,
					GIVEN_COUNT, COUNT, TOTAL
				);
			}
			else
			{
				snprintf(
					REPLY, REPLY_SIZE, "%s: %s %s at %s each is %s x %s = %s.",
					TOTAL, COUNT, READ->TOKENS[INDEX + 1].TEXT, EACH, COUNT,
					EACH, TOTAL
				);
				snprintf(
					ACTION, ACTION_SIZE, "calc %s * %s -> %s", COUNT, EACH,
					TOTAL
				);
			}

			return (1);
		}
	}

	return (0);
}

/* ---------- "24 students, half of them are girls, how many boys" ------- */

static int
	ANSWER_PART_REST(
		const READING *READ, char *ACTION, int ACTION_SIZE, char *REPLY,
		int REPLY_SIZE
	)
{
	double	TOTAL = 0;
	char	WHOLE_NOUN[64] = "";
	double	PART = -1;
	char	PART_NOUN[64] = "";
	char	ASKED_NOUN[64] = "";
	int		INDEX;
	int		ASKED_NOT = 0;
	char	TEXT[64];

	if (
		!HAS(READ, " how many ") ||
		(!HAS(READ, " of them ") && !HAS(READ, " of the "))
	)
		return (0);

	/* "there are 24 students", "a class has 24 students" */
	for (INDEX = 0; INDEX + 1 < READ->COUNT; INDEX++)
		if (
			IS_NUMBER(&READ->TOKENS[INDEX]) &&
			IS_WORD_TOKEN(READ, INDEX + 1) &&
			strcmp(TEXT_AT(READ, INDEX + 1), "of")
		)
		{
			TOTAL = READ->TOKENS[INDEX].VALUE;
			snprintf(
				WHOLE_NOUN, sizeof WHOLE_NOUN, "%s",
				READ->TOKENS[INDEX + 1].TEXT
			);
			break ;
		}

	if (TOTAL <= 0)
		return (0);

	/* "half of them are girls", "10 of them are boys", "a third of the
	 * students are girls", "25% of them are girls" */
	for (INDEX = 0; INDEX + 3 < READ->COUNT; INDEX++)
	{
		int	OF_AT = -1;

		if (!strcmp(READ->TOKENS[INDEX].TEXT, "half"))
		{
			PART = TOTAL / 2;
			OF_AT = INDEX + 1;
		}
		else if (
			(
				!strcmp(READ->TOKENS[INDEX].TEXT, "third") ||
				!strcmp(READ->TOKENS[INDEX].TEXT, "quarter") ||
				!strcmp(READ->TOKENS[INDEX].TEXT, "fourth") ||
				!strcmp(READ->TOKENS[INDEX].TEXT, "fifth")
			) &&
			INDEX > 0 &&
			(
				!strcmp(TEXT_AT(READ, INDEX - 1), "a") ||
				!strcmp(TEXT_AT(READ, INDEX - 1), "one")
			)
		)
		{
			PART = TOTAL /
				(READ->TOKENS[INDEX].TEXT[0] == 't' ? 3
					: READ->TOKENS[INDEX].TEXT[1] == 'i' ? 5
					: 4);
			OF_AT = INDEX + 1;
		}
		else if (
			IS_NUMBER(&READ->TOKENS[INDEX]) &&
			READ->TOKENS[INDEX].VALUE != TOTAL
		)
		{
			if (
				!strcmp(TEXT_AT(READ, INDEX + 1), "%") ||
				!strcmp(TEXT_AT(READ, INDEX + 1), "percent")
			)
			{
				PART = TOTAL * READ->TOKENS[INDEX].VALUE / 100;
				OF_AT = INDEX + 2;
			}
			else if (READ->TOKENS[INDEX].IS_FRACTION)
			{
				PART = TOTAL * READ->TOKENS[INDEX].VALUE;
				OF_AT = INDEX + 1;
			}
			else
			{
				PART = READ->TOKENS[INDEX].VALUE;
				OF_AT = INDEX + 1;
			}
		}

		if (OF_AT < 0 || strcmp(TEXT_AT(READ, OF_AT), "of"))
		{
			PART = -1;
			continue ;
		}

		/* "of them are girls", "of the students are girls" */
		{
			int	AT = OF_AT + 1;

			if (
				!strcmp(TEXT_AT(READ, AT), "them") ||
				!strcmp(TEXT_AT(READ, AT), "these") ||
				!strcmp(TEXT_AT(READ, AT), "those")
			)
				AT++;
			else if (!strcmp(TEXT_AT(READ, AT), "the"))
				AT += 2;
			else
			{
				PART = -1;
				continue ;
			}

			if (
				strcmp(TEXT_AT(READ, AT), "are") &&
				strcmp(TEXT_AT(READ, AT), "were") &&
				strcmp(TEXT_AT(READ, AT), "is")
			)
			{
				PART = -1;
				continue ;
			}

			if (!IS_WORD_TOKEN(READ, AT + 1))
			{
				PART = -1;
				continue ;
			}

			snprintf(PART_NOUN, sizeof PART_NOUN, "%s", TEXT_AT(READ, AT + 1));
			break ;
		}
	}

	if (PART < 0 || !PART_NOUN[0] || PART > TOTAL || PART != floor(PART))
		return (0);

	/* "how many boys are there", "how many are not girls" */
	for (INDEX = 0; INDEX + 2 < READ->COUNT; INDEX++)
		if (
			!strcmp(READ->TOKENS[INDEX].TEXT, "how") &&
			!strcmp(READ->TOKENS[INDEX + 1].TEXT, "many")
		)
		{
			if (
				!strcmp(TEXT_AT(READ, INDEX + 2), "are") &&
				!strcmp(TEXT_AT(READ, INDEX + 3), "not")
			)
			{
				ASKED_NOT = 1;
				snprintf(
					ASKED_NOUN, sizeof ASKED_NOUN, "%s",
					TEXT_AT(READ, INDEX + 4)
				);
			}
			else
				snprintf(
					ASKED_NOUN, sizeof ASKED_NOUN, "%s",
					TEXT_AT(READ, INDEX + 2)
				);

			break ;
		}

	if (!ASKED_NOUN[0] || !strcmp(ASKED_NOUN, WHOLE_NOUN))
		return (0);

	/* "one of them is gone. How many are left?" */
	if (
		!strcmp(ASKED_NOUN, "are") ||
		!strcmp(ASKED_NOUN, "is") ||
		!strcmp(ASKED_NOUN, "do") ||
		!strcmp(ASKED_NOUN, "were")
	)
	{
		char	PART_TEXT[64];
		char	TOTAL_TEXT[64];

		if (!HAS(READ, " left ") && !HAS(READ, " remain"))
			return (0);

		SAY_NUMBER(PART, PART_TEXT, sizeof PART_TEXT);
		SAY_NUMBER(TOTAL, TOTAL_TEXT, sizeof TOTAL_TEXT);
		SAY_NUMBER(TOTAL - PART, TEXT, sizeof TEXT);
		snprintf(
			REPLY, REPLY_SIZE,
			"%s %s left: %s of the %s %s %s %s, so %s - "
			"%s = %s.",
			TEXT, TOTAL - PART == 1 ? "is" : "are", PART_TEXT, TOTAL_TEXT,
			WHOLE_NOUN, PART == 1 ? "is" : "are", PART_NOUN, TOTAL_TEXT,
			PART_TEXT, TEXT
		);
		snprintf(
			ACTION, ACTION_SIZE, "calc %s - %s -> %s", TOTAL_TEXT, PART_TEXT,
			TEXT
		);
		return (1);
	}

	{
		char	PART_TEXT[64];
		char	TOTAL_TEXT[64];
		int		SAME_GROUP = !ASKED_NOT && !strcmp(ASKED_NOUN, PART_NOUN);

		SAY_NUMBER(PART, PART_TEXT, sizeof PART_TEXT);
		SAY_NUMBER(TOTAL, TOTAL_TEXT, sizeof TOTAL_TEXT);

		if (SAME_GROUP)
		{
			snprintf(REPLY, REPLY_SIZE, "%s %s.", PART_TEXT, PART_NOUN);
			snprintf(ACTION, ACTION_SIZE, "part -> %s", PART_TEXT);
			return (1);
		}

		SAY_NUMBER(TOTAL - PART, TEXT, sizeof TEXT);

		if (ASKED_NOT)
			snprintf(
				REPLY, REPLY_SIZE,
				"%s: %s of the %s %s are %s, so %s - %s "
				"= %s are not.",
				TEXT, PART_TEXT, TOTAL_TEXT, WHOLE_NOUN, PART_NOUN, TOTAL_TEXT,
				PART_TEXT, TEXT
			);
		else
			snprintf(
				REPLY, REPLY_SIZE,
				"%s %s: %s of the %s %s are %s, so %s - "
				"%s = %s are %s.",
				TEXT, ASKED_NOUN, PART_TEXT, TOTAL_TEXT, WHOLE_NOUN, PART_NOUN,
				TOTAL_TEXT, PART_TEXT, TEXT, ASKED_NOUN
			);

		snprintf(
			ACTION, ACTION_SIZE, "calc %s - %s -> %s", TOTAL_TEXT, PART_TEXT,
			TEXT
		);
		return (1);
	}
}

/* ---------- "3 packs of 6 eggs: how many eggs" ---------- */

static int
	ANSWER_GROUPS(
		const READING *READ, char *ACTION, int ACTION_SIZE, char *REPLY,
		int REPLY_SIZE
	)
{
	int	INDEX;

	if (!HAS(READ, " how many "))
		return (0);

	/* "3 packs of 6 eggs", "4 boxes of 12 pencils" */
	for (INDEX = 0; INDEX + 4 < READ->COUNT; INDEX++)
		if (
			IS_NUMBER(&READ->TOKENS[INDEX]) &&
			IS_WORD_TOKEN(READ, INDEX + 1) &&
			!strcmp(READ->TOKENS[INDEX + 2].TEXT, "of") &&
			IS_NUMBER(&READ->TOKENS[INDEX + 3]) &&
			IS_WORD_TOKEN(READ, INDEX + 4)
		)
		{
			char	THING[64];
			char	ASKED[64] = "";
			int		SCAN;

			SINGULAR(READ->TOKENS[INDEX + 4].TEXT, THING, sizeof THING);

			for (SCAN = INDEX + 5; SCAN + 2 < READ->COUNT; SCAN++)
				if (
					!strcmp(READ->TOKENS[SCAN].TEXT, "how") &&
					!strcmp(READ->TOKENS[SCAN + 1].TEXT, "many")
				)
				{
					SINGULAR(READ->TOKENS[SCAN + 2].TEXT, ASKED, sizeof ASKED);
					break ;
				}

			if (strcmp(ASKED, THING))
				continue ;

			{
				char	GROUPS[64];
				char	EACH[64];
				char	TOTAL[64];

				SAY_NUMBER(READ->TOKENS[INDEX].VALUE, GROUPS, sizeof GROUPS);
				SAY_NUMBER(READ->TOKENS[INDEX + 3].VALUE, EACH, sizeof EACH);
				SAY_NUMBER(
					READ->TOKENS[INDEX].VALUE * READ->TOKENS[INDEX + 3].VALUE,
					TOTAL, sizeof TOTAL
				);
				snprintf(
					REPLY, REPLY_SIZE, "%s %s: %s %s of %s is %s x %s = %s.",
					TOTAL, READ->TOKENS[INDEX + 4].TEXT, GROUPS,
					READ->TOKENS[INDEX + 1].TEXT, EACH, GROUPS, EACH, TOTAL
				);
				snprintf(
					ACTION, ACTION_SIZE, "calc %s * %s -> %s", GROUPS, EACH,
					TOTAL
				);
				return (1);
			}
		}

	return (0);
}

/* ---------- "which comes first in the alphabet, banana or apple?" ------- */

static int
	ANSWER_ALPHABET_ORDER(
		const READING *READ, char *ACTION, int ACTION_SIZE, char *REPLY,
		int REPLY_SIZE
	)
{
	int	INDEX;

	if (
		!(HAS(READ, " alphabet") || HAS(READ, " alphabetical")) ||
		!HAS(READ, " or ") ||
		!(
			HAS(READ, " first ") ||
			HAS(READ, " last ") ||
			HAS(READ, " before ") ||
			HAS(READ, " after ")
		)
	)
		return (0);

	for (INDEX = 1; INDEX + 1 < READ->COUNT; INDEX++)
		if (
			!strcmp(READ->TOKENS[INDEX].TEXT, "or") &&
			IS_WORD_TOKEN(READ, INDEX - 1) &&
			IS_WORD_TOKEN(READ, INDEX + 1)
		)
		{
			const TOKEN	*FIRST = &READ->TOKENS[INDEX - 1];
			const TOKEN	*SECOND = &READ->TOKENS[INDEX + 1];
			int			WANT_LAST = HAS(READ, " last ") || HAS(READ, " after ");
			const TOKEN	*WINNER;
			int			ORDER = strcmp(FIRST->TEXT, SECOND->TEXT);

			if (!ORDER)
				return (0);

			if ((ORDER < 0) != WANT_LAST)
				WINNER = FIRST;
			else
				WINNER = SECOND;

			snprintf(
				REPLY, REPLY_SIZE, "%s comes %s in the alphabet.",
				WINNER->ORIGINAL, WANT_LAST ? "last" : "first"
			);
			REPLY[0] = (char)toupper((uint8_t)REPLY[0]);
			snprintf(
				ACTION, ACTION_SIZE, "alphabet %s %s -> %s", FIRST->TEXT,
				SECOND->TEXT, WINNER->TEXT
			);
			return (1);
		}

	return (0);
}

/* ---------- "answer with only yes or no" ---------- */

static void
	KEEP_ONLY(const READING *READ, char *REPLY, int REPLY_SIZE)
{
	if (
		HAS(READ, " only yes or no ") ||
		HAS(READ, " just yes or no ") ||
		HAS(READ, " yes or no only ") ||
		HAS(READ, " with yes or no ") ||
		HAS(READ, " answer yes or no ") ||
		HAS(READ, " say yes or no ")
	)
	{
		if (!strncmp(REPLY, "Yes", 3))
			snprintf(REPLY, REPLY_SIZE, "Yes.");
		else if (!strncmp(REPLY, "No", 2))
			snprintf(REPLY, REPLY_SIZE, "No.");

		return ;
	}

	if (
		HAS(READ, " only the number ") ||
		HAS(READ, " just the number ") ||
		HAS(READ, " only a number ") ||
		HAS(READ, " just a number ") ||
		HAS(READ, " number only ") ||
		HAS(READ, " with a number ") ||
		HAS(READ, " with one number ") ||
		HAS(READ, " only the answer ") ||
		HAS(READ, " just the answer ")
	)
	{
		const char	*CURSOR = REPLY;
		char		NUMBER[64];
		int			LENGTH = 0;

		/* the first number said ("The next number is 10: ...") */
		while (
			*CURSOR &&
			!isdigit((uint8_t)*CURSOR) &&
			!(*CURSOR == '-' && isdigit((uint8_t)CURSOR[1]))
		)
			CURSOR++;

		while (
			*CURSOR &&
			(isdigit((uint8_t)*CURSOR) || strchr("-./", *CURSOR)) &&
			LENGTH < 63
		)
		{
			if (*CURSOR == '.' && !isdigit((uint8_t)CURSOR[1]))
				break ;

			NUMBER[LENGTH++] = *CURSOR++;
		}

		NUMBER[LENGTH] = 0;

		if (LENGTH)
			snprintf(REPLY, REPLY_SIZE, "%s", NUMBER);
	}
}

/* ---------- "If it rains, the ground gets wet. It rains. Is the ground
 * wet?" ---------- */

#define CLAUSE_WORDS 12

/* a clause by its meaning words: "the ground gets wet" -> "ground wet",
 * "it is raining" -> "rain"; NOT when it is said the other way */
typedef struct
{
	char	WORDS[CLAUSE_WORDS][32];
	int		COUNT;
	int		NOT;
	char	SHOWN[200];
} CLAUSE;

static void
	READ_CLAUSE(const char *TEXT, int LENGTH, CLAUSE *OUTPUT)
{
	static const char	*SKIP[56] = {
		"the", "a", "an", "it", "is", "are", "was", "were", "be", "been",
		"will", "would", "does", "do", "did", "gets", "get", "got", "getting",
		"then", "so", "also", "too", "that", "there", "really", "always",
		"become", "becomes", "turns", "turn", "going", "goes", "go", "to",
		"can", "could", "should", "shall", "of", "they", "he", "she", "we",
		"you", "i", "his", "her", "their", "my", "your", "its", "outside",
		"today", "now", NULL
	};
	char				WORD[64];
	int					POSITION = 0;
	int					INDEX;

	memset(OUTPUT, 0, sizeof *OUTPUT);
	snprintf(OUTPUT->SHOWN, sizeof OUTPUT->SHOWN, "%.*s", LENGTH, TEXT);

	for (INDEX = 0; INDEX <= LENGTH; INDEX++)
	{
		char	CHARACTER = INDEX < LENGTH ? TEXT[INDEX] : ' ';

		if (isalnum((uint8_t)CHARACTER) || CHARACTER == '\'')
		{
			if (POSITION < 62)
				WORD[POSITION++] = (char)tolower((uint8_t)CHARACTER);

			continue ;
		}

		if (!POSITION)
			continue ;

		WORD[POSITION] = 0;
		POSITION = 0;

		if (
			!strcmp(WORD, "not") ||
			!strcmp(WORD, "no") ||
			!strcmp(WORD, "never") ||
			strstr(WORD, "n't")
		)
		{
			OUTPUT->NOT = !OUTPUT->NOT;
			continue ;
		}

		{
			int	SKIPPED = 0;
			int	SKIP_INDEX;
			int	WORD_LENGTH = (int)strlen(WORD);

			for (SKIP_INDEX = 0; SKIP[SKIP_INDEX] && !SKIPPED; SKIP_INDEX++)
				if (!strcmp(WORD, SKIP[SKIP_INDEX]))
					SKIPPED = 1;

			if (SKIPPED || OUTPUT->COUNT >= CLAUSE_WORDS)
				continue ;

			/* "raining", "rains", "rained" -> "rain"; "studies" -> "study";
			 * "passes" -> "pass" */
			if (WORD_LENGTH > 5 && !strcmp(WORD + WORD_LENGTH - 3, "ing"))
				WORD[WORD_LENGTH - 3] = 0;
			else if (WORD_LENGTH > 4 && !strcmp(WORD + WORD_LENGTH - 3, "ied"))
				strcpy(WORD + WORD_LENGTH - 3, "y");
			else if (WORD_LENGTH > 4 && !strcmp(WORD + WORD_LENGTH - 2, "ed"))
				WORD[WORD_LENGTH - 2] = 0;
			else if (WORD_LENGTH > 4 && !strcmp(WORD + WORD_LENGTH - 3, "ies"))
				strcpy(WORD + WORD_LENGTH - 3, "y");
			else if (
				WORD_LENGTH > 4 &&
				(
					!strcmp(WORD + WORD_LENGTH - 4, "sses") ||
					!strcmp(WORD + WORD_LENGTH - 4, "ches") ||
					!strcmp(WORD + WORD_LENGTH - 4, "shes") ||
					!strcmp(WORD + WORD_LENGTH - 3, "xes") ||
					!strcmp(WORD + WORD_LENGTH - 3, "zes")
				)
			)
				WORD[WORD_LENGTH - 2] = 0;
			else if (
				WORD_LENGTH > 3 &&
				WORD[WORD_LENGTH - 1] == 's' &&
				WORD[WORD_LENGTH - 2] != 's'
			)
				WORD[WORD_LENGTH - 1] = 0;

			snprintf(OUTPUT->WORDS[OUTPUT->COUNT++], 32, "%s", WORD);
		}
	}
}

/* the same meaning words, either way round */
static int
	SAME_CLAUSE(const CLAUSE *FIRST, const CLAUSE *SECOND)
{
	int	INDEX;
	int	OTHER;

	if (!FIRST->COUNT || FIRST->COUNT != SECOND->COUNT)
		return (0);

	for (INDEX = 0; INDEX < FIRST->COUNT; INDEX++)
	{
		int	FOUND = 0;

		for (OTHER = 0; OTHER < SECOND->COUNT && !FOUND; OTHER++)
			if (!strcmp(FIRST->WORDS[INDEX], SECOND->WORDS[OTHER]))
				FOUND = 1;

		if (!FOUND)
			return (0);
	}

	return (1);
}

/* the asked clause against the then part, where the asked one may name the
 * one the if part is about ("does Tom pass" for "he passes" after "if Tom
 * studies") */
static int
	SAME_CLAUSE_ABOUT(
		const CLAUSE *ASKED_CLAUSE, const CLAUSE *EFFECT, const CLAUSE *CAUSE
	)
{
	CLAUSE	TRIMMED = *ASKED_CLAUSE;
	int		INDEX;
	int		OTHER;

	if (SAME_CLAUSE(ASKED_CLAUSE, EFFECT))
		return (1);

	TRIMMED.COUNT = 0;

	for (INDEX = 0; INDEX < ASKED_CLAUSE->COUNT; INDEX++)
	{
		int	IN_EFFECT = 0;
		int	IN_CAUSE = 0;

		for (OTHER = 0; OTHER < EFFECT->COUNT; OTHER++)
			if (!strcmp(ASKED_CLAUSE->WORDS[INDEX], EFFECT->WORDS[OTHER]))
				IN_EFFECT = 1;

		for (OTHER = 0; OTHER < CAUSE->COUNT; OTHER++)
			if (!strcmp(ASKED_CLAUSE->WORDS[INDEX], CAUSE->WORDS[OTHER]))
				IN_CAUSE = 1;

		if (!IN_EFFECT && IN_CAUSE)
			continue ;

		snprintf(
			TRIMMED.WORDS[TRIMMED.COUNT++], 32, "%s", ASKED_CLAUSE->WORDS[INDEX]
		);
	}

	return (
		TRIMMED.COUNT < ASKED_CLAUSE->COUNT &&
		SAME_CLAUSE(&TRIMMED, EFFECT)
	);
}

/* "it rains" said as part of a reply: no capital, no end mark */
static void
	CLAUSE_SAID(const CLAUSE *ITEM, char *OUTPUT, int OUTPUT_SIZE)
{
	size_t	LENGTH;
	char	*START;

	snprintf(OUTPUT, OUTPUT_SIZE, "%s", ITEM->SHOWN);
	START = OUTPUT;

	while (*START == ' ')
		START++;

	memmove(OUTPUT, START, strlen(START) + 1);
	LENGTH = strlen(OUTPUT);

	while (LENGTH && strchr(" .,!?", OUTPUT[LENGTH - 1]))
		OUTPUT[--LENGTH] = 0;

	if (
		OUTPUT[0] &&
		isupper((uint8_t)OUTPUT[0]) &&
		!(OUTPUT[1] == ' ' && OUTPUT[0] == 'I') &&
		(!isalpha((uint8_t)OUTPUT[1]) || islower((uint8_t)OUTPUT[1]))
	)
	{
		/* names stay as they are: only the usual first words are lowered */
		static const char	*FIRST_WORDS[20] = {
			"It", "The", "A", "An", "If", "When", "There", "They", "He", "She",
			"We", "You", "My", "Your", "His", "Her", "Their", "This", "That",
			NULL
		};
		int					INDEX;

		for (INDEX = 0; FIRST_WORDS[INDEX]; INDEX++)
			if (
				!strncmp(
					OUTPUT, FIRST_WORDS[INDEX], strlen(FIRST_WORDS[INDEX])
				) &&
				!isalpha((uint8_t)OUTPUT[strlen(FIRST_WORDS[INDEX])])
			)
				OUTPUT[0] = (char)tolower((uint8_t)OUTPUT[0]);
	}
}

static int
	ANSWER_IF_THEN(
		const READING *READ, char *ACTION, int ACTION_SIZE, char *REPLY,
		int REPLY_SIZE
	)
{
	const char	*MESSAGE = READ->MESSAGE;
	CLAUSE		CAUSES[6];
	CLAUSE		EFFECTS[6];
	CLAUSE		FACTS[8];
	CLAUSE		ASKED;
	int			RULE_COUNT = 0;
	int			FACT_COUNT = 0;
	const char	*START = MESSAGE;
	const char	*QUESTION = NULL;
	int			RULE;
	int			FACT;

	if (!HAS(READ, " if ") && !HAS(READ, " when "))
		return (0);

	if (!strchr(MESSAGE, '?'))
		return (0);

	/* the sentences: rules, facts, and the question last */
	while (*START)
	{
		const char	*END = START;
		int			LENGTH;

		while (*START == ' ')
			START++;

		END = START;

		while (*END && !strchr(".?!", *END))
			END++;

		LENGTH = (int)(END - START);

		if (LENGTH > 0)
		{
			char	LOWER[400];
			int		INDEX;

			for (INDEX = 0; INDEX < LENGTH && INDEX < 399; INDEX++)
				LOWER[INDEX] = (char)tolower((uint8_t)START[INDEX]);

			LOWER[INDEX] = 0;

			if (*END == '?')
			{
				const char	*FROM = START;

				/* "is the ground wet", "will the ground get wet" */
				if (
					!strncmp(LOWER, "is ", 3) ||
					!strncmp(LOWER, "are ", 4) ||
					!strncmp(LOWER, "does ", 5) ||
					!strncmp(LOWER, "do ", 3) ||
					!strncmp(LOWER, "will ", 5) ||
					!strncmp(LOWER, "did ", 4) ||
					!strncmp(LOWER, "was ", 4) ||
					!strncmp(LOWER, "can ", 4)
				)
					FROM = strchr(START, ' ') + 1;
				else
					return (0);

				READ_CLAUSE(FROM, (int)(END - FROM), &ASKED);
				QUESTION = START;
			}
			else if (
				(
					!strncmp(LOWER, "if ", 3) ||
					!strncmp(LOWER, "when ", 5) ||
					!strncmp(LOWER, "whenever ", 9)
				) &&
				RULE_COUNT < 6
			)
			{
				const char	*CAUSE_AT = strchr(START, ' ') + 1;
				const char	*SPLIT = NULL;
				const char	*EFFECT_AT;
				const char	*SCAN;

				for (SCAN = CAUSE_AT; SCAN < END && !SPLIT; SCAN++)
					if (*SCAN == ',')
						SPLIT = SCAN;
					else if (!strncasecmp(SCAN, " then ", 6))
						SPLIT = SCAN;

				if (!SPLIT)
					return (0);

				EFFECT_AT = SPLIT + 1;

				while (*EFFECT_AT == ' ')
					EFFECT_AT++;

				if (!strncasecmp(EFFECT_AT, "then ", 5))
					EFFECT_AT += 5;

				READ_CLAUSE(
					CAUSE_AT, (int)(SPLIT - CAUSE_AT), &CAUSES[RULE_COUNT]
				);
				READ_CLAUSE(
					EFFECT_AT, (int)(END - EFFECT_AT), &EFFECTS[RULE_COUNT]
				);

				if (CAUSES[RULE_COUNT].COUNT && EFFECTS[RULE_COUNT].COUNT)
					RULE_COUNT++;
			}
			else if (FACT_COUNT < 8)
			{
				READ_CLAUSE(START, LENGTH, &FACTS[FACT_COUNT]);

				if (FACTS[FACT_COUNT].COUNT)
					FACT_COUNT++;
			}
		}

		if (!*END)
			break ;

		START = END + 1;
	}

	if (!QUESTION || !RULE_COUNT || !ASKED.COUNT)
		return (0);

	for (RULE = 0; RULE < RULE_COUNT; RULE++)
	{
		char	CAUSE_TEXT[200];
		char	EFFECT_TEXT[200];

		CLAUSE_SAID(&CAUSES[RULE], CAUSE_TEXT, sizeof CAUSE_TEXT);
		CLAUSE_SAID(&EFFECTS[RULE], EFFECT_TEXT, sizeof EFFECT_TEXT);

		/* asked about what follows */
		if (SAME_CLAUSE_ABOUT(&ASKED, &EFFECTS[RULE], &CAUSES[RULE]))
		{
			for (FACT = 0; FACT < FACT_COUNT; FACT++)
				if (SAME_CLAUSE(&FACTS[FACT], &CAUSES[RULE]))
				{
					char	FACT_TEXT[200];
					int		HAPPENS = FACTS[FACT].NOT == CAUSES[RULE].NOT;
					int		YES = (EFFECTS[RULE].NOT == ASKED.NOT);

					CLAUSE_SAID(&FACTS[FACT], FACT_TEXT, sizeof FACT_TEXT);

					if (!HAPPENS)
					{
						snprintf(
							REPLY, REPLY_SIZE,
							"I can't tell. The rule only "
							"says what happens if %s, and you said %s. "
							"Something "
							"else could still make it so.",
							CAUSE_TEXT, FACT_TEXT
						);
						snprintf(ACTION, ACTION_SIZE, "if then -> can't tell");
						ADD_WHY(
							REPLY, REPLY_SIZE,
							"\"If %s, %s\" only says what "
							"happens when %s. It says nothing about the other "
							"case, so I can't tell. Thinking it does is a "
							"common "
							"slip.",
							CAUSE_TEXT, EFFECT_TEXT, CAUSE_TEXT
						);
						return (1);
					}

					snprintf(
						REPLY, REPLY_SIZE, "%s. If %s, %s, and %s, so %s.",
						YES ? "Yes" : "No", CAUSE_TEXT, EFFECT_TEXT, FACT_TEXT,
						EFFECT_TEXT
					);
					snprintf(
						ACTION, ACTION_SIZE, "if then -> %s", YES ? "yes" : "no"
					);
					ADD_WHY(
						REPLY, REPLY_SIZE,
						"The rule is \"if %s, %s\", and "
						"you told me %s. When the if part holds, the then part "
						"follows, so %s.",
						CAUSE_TEXT, EFFECT_TEXT, FACT_TEXT, EFFECT_TEXT
					);
					return (1);
				}

			return (0);
		}

		/* asked about the if part */
		if (SAME_CLAUSE(&ASKED, &CAUSES[RULE]))
			for (FACT = 0; FACT < FACT_COUNT; FACT++)
				if (SAME_CLAUSE(&FACTS[FACT], &EFFECTS[RULE]))
				{
					char	FACT_TEXT[200];

					CLAUSE_SAID(&FACTS[FACT], FACT_TEXT, sizeof FACT_TEXT);

					/* the then part is not so: neither is the if part */
					if (FACTS[FACT].NOT != EFFECTS[RULE].NOT)
					{
						int	YES = ASKED.NOT != CAUSES[RULE].NOT;

						snprintf(
							REPLY, REPLY_SIZE,
							"%s. If %s, %s. But %s, so "
							"it can't be that %s.",
							YES ? "Yes" : "No", CAUSE_TEXT, EFFECT_TEXT,
							FACT_TEXT, CAUSE_TEXT
						);
						snprintf(
							ACTION, ACTION_SIZE, "if then back -> %s",
							YES ? "yes" : "no"
						);
						ADD_WHY(
							REPLY, REPLY_SIZE,
							"Whenever %s, %s. You said "
							"%s, so the if part (%s) can't be true.",
							CAUSE_TEXT, EFFECT_TEXT, FACT_TEXT, CAUSE_TEXT
						);
						return (1);
					}

					snprintf(
						REPLY, REPLY_SIZE,
						"I can't tell. If %s, %s, but "
						"that could also happen for another reason.",
						CAUSE_TEXT, EFFECT_TEXT
					);
					snprintf(ACTION, ACTION_SIZE, "if then back -> can't tell");
					ADD_WHY(
						REPLY, REPLY_SIZE,
						"The rule only goes one way: from "
						"%s to %s. Knowing %s doesn't tell me %s; thinking it "
						"does is a common slip (affirming the then part).",
						CAUSE_TEXT, EFFECT_TEXT, FACT_TEXT, CAUSE_TEXT
					);
					return (1);
				}
	}

	return (0);
}

/* ---------- a question that misses something, and the answer that fills
 * it ---------- */

/* "what comes next?", "next number?" with nothing to go on */
static int
	IS_BARE_NEXT_ASK(const char *TEXT)
{
	static const char	*ASKS[27] = {
		" what comes next ", " what's next ", " whats next ", " what is next ",
		" what is the next number ", " what's the next number ",
		" whats the next number ", " what is the next letter ",
		" what's the next letter ", " next number ", " next letter ",
		" what number comes next ", " what letter comes next ",
		" continue the sequence ", " complete the sequence ",
		" continue the pattern ", " complete the pattern ",
		" finish the sequence ", " what comes next in the sequence ",
		" what comes next in the pattern ", " and what comes next ",
		" what is the next one ", " what's the next one ", " so what's next ",
		" ok what comes next ", " what comes after that ", NULL
	};
	char				PLAIN[300];
	int					POSITION = 0;
	int					INDEX;

	PLAIN[POSITION++] = ' ';

	for (; *TEXT && POSITION < (int)sizeof PLAIN - 2; TEXT++)
		if (isalnum((uint8_t)*TEXT) || *TEXT == '\'')
			PLAIN[POSITION++] = (char)tolower((uint8_t)*TEXT);
		else if (PLAIN[POSITION - 1] != ' ')
			PLAIN[POSITION++] = ' ';

	if (PLAIN[POSITION - 1] != ' ')
		PLAIN[POSITION++] = ' ';

	PLAIN[POSITION] = 0;

	for (INDEX = 0; ASKS[INDEX]; INDEX++)
		if (!strcmp(PLAIN, ASKS[INDEX]))
			return (1);

	return (0);
}

/* how many numbers (or single letters) the text lists in a row */
static int
	LIST_ITEMS(const READING *READ)
{
	double	VALUES[64];
	int		COUNT = NUMBER_LIST(READ, 0, VALUES, 64, NULL, NULL);
	int		LETTERS = 0;
	int		BEST = 0;
	int		INDEX;

	for (INDEX = 0; INDEX < READ->COUNT; INDEX++)
	{
		const TOKEN	*ITEM = &READ->TOKENS[INDEX];

		if (
			ITEM->KIND == TOKEN_WORD &&
			strlen(ITEM->TEXT) == 1 &&
			strcmp(ITEM->ORIGINAL, "a") &&
			strcmp(ITEM->ORIGINAL, "I")
		)
			LETTERS++;
		else if (
			!(
				ITEM->KIND == TOKEN_SYMBOL &&
				(ITEM->TEXT[0] == ',' || ITEM->TEXT[0] == '-')
			)
		)
		{
			if (LETTERS > BEST)
				BEST = LETTERS;

			LETTERS = 0;
		}
	}

	if (LETTERS > BEST)
		BEST = LETTERS;

	if (COUNT > BEST)
		return (COUNT);

	return (BEST);
}

/* "convert 5 to meters": a number with the unit to reach but none to start
 * from; the number's place in the message, -1 when it is not that */
static int
	CONVERT_WITHOUT_UNIT(const READING *READ)
{
	int	INDEX;

	if (
		!HAS(READ, " convert ") &&
		!HAS(READ, " change ") &&
		!HAS(READ, " turn ")
	)
		return (-1);

	for (INDEX = 0; INDEX + 2 < READ->COUNT; INDEX++)
		if (
			IS_NUMBER(&READ->TOKENS[INDEX]) &&
			(
				!strcmp(READ->TOKENS[INDEX + 1].TEXT, "to") ||
				!strcmp(READ->TOKENS[INDEX + 1].TEXT, "into")
			) &&
			FIND_UNIT(READ->TOKENS[INDEX + 2].TEXT) >= 0
		)
			return (INDEX);

	return (-1);
}

/* "how many legs does it have?", "what color is it?": a question about a
 * thing only called "it"; where "it" is, -1 when it is not that */
static int
	ASKS_ABOUT_IT(const READING *READ)
{
	int	INDEX;

	for (INDEX = 0; INDEX < READ->COUNT; INDEX++)
		if (!strcmp(READ->TOKENS[INDEX].TEXT, "it"))
		{
			if (
				(
					HAS(READ, " how many ") &&
					(
						HAS(READ, " does it have ") ||
						HAS(READ, " has it got ") ||
						HAS(READ, " does it got ")
					)
				) ||
				HAS(READ, " what color is it ") ||
				HAS(READ, " what colour is it ") ||
				HAS(READ, " can it fly ") ||
				HAS(READ, " can it swim ") ||
				HAS(READ, " is it an animal ") ||
				HAS(READ, " is it alive ") ||
				HAS(READ, " what kind of thing is it ") ||
				HAS(READ, " what is it made of ")
			)
				return (INDEX);

			return (-1);
		}

	return (-1);
}

/* the things recent messages were about, newest first ("I saw a cat in
 * the garden" -> "garden", "cat"); how many */
static int
	RECENT_THINGS(char (*OUTPUT)[64], int LIMIT)
{
	static READING	PAST;
	char			RECENT[6000];
	char			LINE[800];
	int				COUNT = 0;
	int				BACK;

	if (!CURRENT_HOST || CURRENT_HOST->VERSION < 2 || !CURRENT_HOST->RECENT)
		return (0);

	CURRENT_HOST->RECENT(CURRENT_HOST->ENGINE, RECENT, sizeof RECENT);

	for (BACK = 1; BACK <= 3 && COUNT < LIMIT; BACK++)
	{
		int	INDEX;

		if (!RECENT_LINE(RECENT, BACK, LINE, sizeof LINE))
			break ;

		READ_MESSAGE(LINE, &PAST);

		for (INDEX = PAST.COUNT - 1; INDEX >= 0 && COUNT < LIMIT; INDEX--)
			if (
				PAST.TOKENS[INDEX].KIND == TOKEN_WORD &&
				strlen(PAST.TOKENS[INDEX].TEXT) >= 3 &&
				strcmp(PAST.TOKENS[INDEX].TEXT, "the") &&
				strcmp(PAST.TOKENS[INDEX].TEXT, "and") &&
				KNOWS_THING(PAST.TOKENS[INDEX].TEXT)
			)
				snprintf(OUTPUT[COUNT++], 64, "%s", PAST.TOKENS[INDEX].TEXT);
	}

	return (COUNT);
}

/* the readers that answer about things, run on a message rebuilt from the
 * one before */
static int
	ANSWER_ABOUT_THING(
		const READING *READ, char *ACTION, int ACTION_SIZE, char *REPLY,
		int REPLY_SIZE
	)
{
	return (
		ANSWER_THING_COUNT(READ, ACTION, ACTION_SIZE, REPLY, REPLY_SIZE) ||
		ANSWER_THING_PROPERTY(READ, ACTION, ACTION_SIZE, REPLY, REPLY_SIZE) ||
		ANSWER_THING_IS(READ, ACTION, ACTION_SIZE, REPLY, REPLY_SIZE)
	);
}

/* TEXT with the word at TOKEN replaced by WITH */
static void
	REPLACE_TOKEN(
		const READING *READ, int TOKEN_INDEX, const char *WITH, char *OUTPUT,
		int OUTPUT_SIZE
	)
{
	const TOKEN	*ITEM = &READ->TOKENS[TOKEN_INDEX];

	snprintf(
		OUTPUT, OUTPUT_SIZE, "%.*s%s%s", ITEM->START, READ->MESSAGE, WITH,
		READ->MESSAGE + ITEM->END
	);
}

/* this message finishes the question asked a moment ago: "2, 4, 6, 8"
 * after "what comes next?", "feet" after "convert 5 to meters", "a spider"
 * after "how many legs does it have?" */
static int
	FINISH_LAST_QUESTION(
		const char *MESSAGE, char *ACTION, int ACTION_SIZE, char *REPLY,
		int REPLY_SIZE
	)
{
	static READING	NOW;
	static READING	BEFORE;
	static READING	BOTH;
	char			RECENT[6000];
	char			PREVIOUS[800];
	char			JOINED[1700];

	if (!CURRENT_HOST || CURRENT_HOST->VERSION < 2 || !CURRENT_HOST->RECENT)
		return (0);

	CURRENT_HOST->RECENT(CURRENT_HOST->ENGINE, RECENT, sizeof RECENT);

	if (!RECENT_LINE(RECENT, 1, PREVIOUS, sizeof PREVIOUS))
		return (0);

	READ_MESSAGE(MESSAGE, &NOW);
	READ_MESSAGE(PREVIOUS, &BEFORE);

	/* "what comes next?" then the list, or the list then the question */
	if (
		(
			IS_BARE_NEXT_ASK(PREVIOUS) &&
			LIST_ITEMS(&NOW) >= 3 &&
			NOW.COUNT <= 70
		) ||
		(IS_BARE_NEXT_ASK(MESSAGE) && LIST_ITEMS(&BEFORE) >= 3)
	)
	{
		snprintf(
			JOINED, sizeof JOINED, "%s, what comes next?",
			IS_BARE_NEXT_ASK(MESSAGE) ? PREVIOUS : MESSAGE
		);
		READ_MESSAGE(JOINED, &BOTH);

		if (ANSWER_SEQUENCE(&BOTH, ACTION, ACTION_SIZE, REPLY, REPLY_SIZE))
			return (1);
	}

	/* the unit that was missing */
	{
		int	NUMBER_AT = CONVERT_WITHOUT_UNIT(&BEFORE);
		int	INDEX;

		if (NUMBER_AT >= 0 && NOW.COUNT <= 6)
			for (INDEX = 0; INDEX < NOW.COUNT; INDEX++)
				if (
					NOW.TOKENS[INDEX].KIND == TOKEN_WORD &&
					FIND_UNIT(NOW.TOKENS[INDEX].TEXT) >= 0 &&
					strcmp(NOW.TOKENS[INDEX].TEXT, "in")
				)
				{
					const TOKEN	*NUMBER = &BEFORE.TOKENS[NUMBER_AT];

					snprintf(
						JOINED, sizeof JOINED, "%.*s %s%s", NUMBER->END,
						PREVIOUS, NOW.TOKENS[INDEX].TEXT, PREVIOUS + NUMBER->END
					);
					READ_MESSAGE(JOINED, &BOTH);

					if (
						ANSWER_UNITS(
							&BOTH, ACTION, ACTION_SIZE, REPLY, REPLY_SIZE
						)
					)
						return (1);

					break ;
				}
	}

	/* the thing "it" was */
	{
		int		IT_AT = ASKS_ABOUT_IT(&BEFORE);
		char	THING[200];
		int		POSITION = 0;
		int		INDEX;

		if (IT_AT >= 0 && NOW.COUNT <= 6 && !strchr(MESSAGE, '?'))
		{
			for (INDEX = 0; INDEX < NOW.COUNT; INDEX++)
			{
				const char	*WORD = NOW.TOKENS[INDEX].TEXT;

				if (NOW.TOKENS[INDEX].KIND != TOKEN_WORD)
					continue ;

				if (
					(
						!strcmp(WORD, "i") ||
						!strcmp(WORD, "mean") ||
						!strcmp(WORD, "it") ||
						!strcmp(WORD, "it's") ||
						!strcmp(WORD, "its") ||
						!strcmp(WORD, "is") ||
						!strcmp(WORD, "meant") ||
						!strcmp(WORD, "oh")
					) &&
					!POSITION
				)
					continue ;

				POSITION += snprintf(
					THING + POSITION, sizeof THING - POSITION, "%s%s",
					POSITION ? " " : "", NOW.TOKENS[INDEX].ORIGINAL
				);
			}

			if (POSITION)
			{
				REPLACE_TOKEN(&BEFORE, IT_AT, THING, JOINED, sizeof JOINED);
				READ_MESSAGE(JOINED, &BOTH);

				if (
					ANSWER_ABOUT_THING(
						&BOTH, ACTION, ACTION_SIZE, REPLY, REPLY_SIZE
					)
				)
					return (1);
			}
		}
	}

	return (0);
}

/* nothing to answer with yet: ask for what is missing */
static int
	ASK_WHAT_IS_MISSING(
		const READING *READ, char *ACTION, int ACTION_SIZE, char *REPLY,
		int REPLY_SIZE
	)
{
	int	AT;

	/* "3, 6, 9" and nothing else: the pattern, or what to do with them */
	{
		double	VALUES[64];
		int		COUNT = NUMBER_LIST(READ, 0, VALUES, 64, NULL, NULL);
		int		WORDS = 0;
		int		INDEX;

		for (INDEX = 0; INDEX < READ->COUNT; INDEX++)
			if (
				READ->TOKENS[INDEX].KIND == TOKEN_WORD &&
				strcmp(READ->TOKENS[INDEX].TEXT, "and")
			)
				WORDS++;

		if (COUNT >= 3 && !WORDS)
		{
			double	NEXT;
			char	WHY[300];
			char	NEXT_TEXT[64];

			if (
				NEXT_IN_SEQUENCE(
					VALUES, COUNT, &NEXT, WHY, sizeof WHY, "number"
				)
			)
			{
				SAY_NUMBER(NEXT, NEXT_TEXT, sizeof NEXT_TEXT);
				snprintf(
					REPLY, REPLY_SIZE,
					"That looks like a sequence: %s, so "
					"the next one is %s.",
					WHY, NEXT_TEXT
				);
				ADD_SEQUENCE_WORK(VALUES, COUNT, NEXT, WHY, REPLY, REPLY_SIZE);
				snprintf(ACTION, ACTION_SIZE, "sequence -> %s", NEXT_TEXT);
				return (1);
			}

			SAY_LINE(
				"ask numbers",
				"What should I do with these numbers? I can add them up, sort "
				"them, find the average, or find the next one.",
				NULL, NULL, REPLY, REPLY_SIZE
			);
			snprintf(ACTION, ACTION_SIZE, "ask what to do -> asked");
			return (1);
		}
	}

	if (IS_BARE_NEXT_ASK(READ->MESSAGE))
	{
		SAY_LINE(
			"ask sequence",
			"Next after what? Give me a few numbers or letters in order, like "
			"2, 4, 6, 8, and I'll find the next one.",
			NULL, NULL, REPLY, REPLY_SIZE
		);
		snprintf(ACTION, ACTION_SIZE, "ask which sequence -> asked");
		return (1);
	}

	if ((AT = CONVERT_WITHOUT_UNIT(READ)) >= 0)
	{
		char	AMOUNT[64];

		snprintf(AMOUNT, sizeof AMOUNT, "%s", READ->TOKENS[AT].ORIGINAL);
		SAY_LINE(
			"ask unit",
			"{1} what? Tell me the unit too, like: convert {1} feet to meters.",
			AMOUNT, NULL, REPLY, REPLY_SIZE
		);
		snprintf(ACTION, ACTION_SIZE, "ask which unit -> asked");
		return (1);
	}

	if ((AT = ASKS_ABOUT_IT(READ)) >= 0)
	{
		char			THINGS[6][64];
		int				THING_COUNT = RECENT_THINGS(THINGS, 6);
		int				WHICH;
		char			JOINED[1700];
		static READING	BOTH;

		/* "it" is a thing talked about a moment ago, the newest one this
		 * question fits */
		for (WHICH = 0; WHICH < THING_COUNT; WHICH++)
		{
			char	WITH[80];

			snprintf(WITH, sizeof WITH, "a %s", THINGS[WHICH]);
			REPLACE_TOKEN(READ, AT, WITH, JOINED, sizeof JOINED);
			READ_MESSAGE(JOINED, &BOTH);
			ACTION[0] = REPLY[0] = 0;

			if (
				ANSWER_ABOUT_THING(
					&BOTH, ACTION, ACTION_SIZE, REPLY, REPLY_SIZE
				) &&
				!strncmp(ACTION, "know ", 5) &&
				!strstr(ACTION, "unknown") &&
				!strstr(ACTION, "nothing")
			)
				return (1);
		}

		ACTION[0] = REPLY[0] = 0;

		SAY_LINE(
			"ask it",
			"What do you mean by \"it\"? Tell me which animal or thing, like: "
			"a spider.",
			NULL, NULL, REPLY, REPLY_SIZE
		);
		snprintf(ACTION, ACTION_SIZE, "ask what it is -> asked");
		return (1);
	}

	return (0);
}

/* ---------- the skill ---------- */

static int
	REASON_ANSWER(
		const SKILL_HOST *HOST, const char *MESSAGE, char *ACTION,
		int ACTION_SIZE, char *REPLY, int REPLY_SIZE
	)
{
	static READING	READ;
	int				(*READERS[37])(const READING *, char *, int, char *, int) =
		{ ANSWER_TRICKS,
			ANSWER_WORD_COUNT_ORDER,
			ANSWER_SEQUENCE,
			ANSWER_ORDER,
			ANSWER_SYLLOGISM,
			ANSWER_IF_THEN,
			ANSWER_WORK_RATE,
			ANSWER_THINK_NUMBER,
			ANSWER_STEP_CHAIN,
			ANSWER_AGES,
			ANSWER_RATES,
			ANSWER_UNIT_PRICE,
			ANSWER_PART_REST,
			ANSWER_GROUPS,
			ANSWER_ALPHABET_ORDER,
			ANSWER_PERCENT_CHANGE,
			ANSWER_WHAT_PERCENT,
			ANSWER_UNITS,
			ANSWER_TOGETHER,
			ANSWER_STORY_DETAIL,
			ANSWER_THING_COUNT,
			ANSWER_THING_PROPERTY,
			ANSWER_THING_IS,
			ANSWER_ANALOGY,
			ANSWER_LIST_KIND,
			ANSWER_ODD_ONE_OUT,
			ANSWER_TIMES,
			ANSWER_DAYS,
			ANSWER_SORT,
			ANSWER_NUMBER_MORE,
			ANSWER_LIST_MATH,
			ANSWER_COMPARE_FRACTIONS,
			ANSWER_START_LETTER,
			ANSWER_LETTERS,
			ANSWER_FRACTIONS,
			ANSWER_NUMBER_FACTS,
			NULL };
	int				INDEX;

	CURRENT_HOST = HOST;

	if (!MESSAGE || strlen(MESSAGE) > 1500)
		return (0);

	READ_MESSAGE(MESSAGE, &READ);

	for (INDEX = 0; READERS[INDEX]; INDEX++)
	{
		ACTION[0] = REPLY[0] = 0;

		if (READERS[INDEX](&READ, ACTION, ACTION_SIZE, REPLY, REPLY_SIZE))
		{
			KEEP_ONLY(&READ, REPLY, REPLY_SIZE);
			return (1);
		}
	}

	/* the rest of a question asked a moment ago, or a question back */
	ACTION[0] = REPLY[0] = 0;

	if (FINISH_LAST_QUESTION(MESSAGE, ACTION, ACTION_SIZE, REPLY, REPLY_SIZE))
		return (1);

	ACTION[0] = REPLY[0] = 0;
	READ_MESSAGE(MESSAGE, &READ);

	if (ASK_WHAT_IS_MISSING(&READ, ACTION, ACTION_SIZE, REPLY, REPLY_SIZE))
		return (1);

	return (0);
}

static const SKILL_MEMORY	REASON_MEMORY = {
	SKILL_VERSION, "reason",
	"puzzles with one right answer: sequences, primes, logic and if-then "
	"rules, orders, letters, days, sorting, units, averages, clock times, "
	"fractions, prices, rates, ages, and facts from the knowledge packages",
	0, NULL, REASON_ANSWER
};

SKILL_EXPORT const SKILL_MEMORY
	*__memory__(void)
{
	return (&REASON_MEMORY);
}
