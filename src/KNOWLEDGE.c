#include "KNOWLEDGE.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAXIMUM_TOKENS 64
#define COUNT_NONE (-3)
#define COUNT_MANY (-1)

typedef struct
{
	char	WORDS[MAXIMUM_TOKENS][40];
	int		COUNT;
	int		IS_QUESTION;
	int		IS_CORRECTION;
} TOKEN_LIST;

typedef struct
{
	int		KIND;
	int		RELATION;
	int		COUNT;
	int		IS_NEGATED;
	int		PROPERTY_COUNT;
	char	SUBJECT[40];
	char	VERB[40];
	char	OBJECT_NAME[40];
	char	PROPERTIES[8][40];
} PARSED_SENTENCE;

typedef struct
{
	int	NODES[256];
	int	VIA_FACTS[256];
	int	COUNT;
} ANCESTRY;

enum
{
	SENTENCE_NONE,
	SENTENCE_STATEMENT,
	SENTENCE_CHECK,
	SENTENCE_COUNT,
	SENTENCE_DESCRIBE,
	SENTENCE_LIST,
	SENTENCE_REVERSE
};

static const char	*FUNCTION_WORDS =
	"a an the every all some each any is are am was were be been being has "
	"have had do does did can could will would should shall may might must not "
	"no nor to of in on at for with by from into onto about as and or but if "
	"then than so this that these those it its i me my mine you your yours we "
	"us our they them their he him his she her there here what which who whom "
	"whose how why when where very really also too just yes yeah yep okay ok "
	"please thing things something anything nothing everything one ones lot "
	"lots many much more most few less always usually often sometimes never "
	"quite pretty";
static const char	*ADVERB_WORDS =
	"very really always usually often sometimes quite pretty";
static const char	*KNOWLEDGE_PREPOSITIONS =
	"in on at with from into onto under over near inside outside through to "
	"for about by";
static const char	*KNOWLEDGE_DETERMINERS = "a an the every all some each any";
static const char	*IMPERATIVE_WORDS =
	"list show open read write make create delete remove move rename copy "
	"count say tell give put play stop start find search save load close run "
	"go come look let help calculate add subtract multiply divide repeat "
	"reverse print type check set call remember forget keep send get take "
	"bring turn try use";

static int
	IN_WORD_LIST(const char *SEARCH_WORD, const char *SPACED_LIST)
{
	size_t		SEARCH_LENGTH = strlen(SEARCH_WORD);
	const char	*LIST_CURSOR = SPACED_LIST;

	for (;;)
	{
		const char	*ENTRY_END = strchr(LIST_CURSOR, ' ');
		size_t		ENTRY_LENGTH;

		if (ENTRY_END)
			ENTRY_LENGTH = (size_t)(ENTRY_END - LIST_CURSOR);
		else
			ENTRY_LENGTH = strlen(LIST_CURSOR);

		if (
			ENTRY_LENGTH == SEARCH_LENGTH &&
			!strncmp(LIST_CURSOR, SEARCH_WORD, SEARCH_LENGTH)
		)
			return (1);

		if (!ENTRY_END)
			return (0);

		LIST_CURSOR = ENTRY_END + 1;
	}
}

static int
	ENDS_WITH(const char *FULL_STRING, const char *SUFFIX)
{
	size_t	STRING_LENGTH = strlen(FULL_STRING);
	size_t	SUFFIX_LENGTH = strlen(SUFFIX);

	return (
		STRING_LENGTH >= SUFFIX_LENGTH &&
		!strcmp(FULL_STRING + STRING_LENGTH - SUFFIX_LENGTH, SUFFIX)
	);
}

static int
	IS_LOWERCASE_WORD(const char *CANDIDATE)
{
	size_t	CANDIDATE_LENGTH = strlen(CANDIDATE);

	if (CANDIDATE_LENGTH < 2)
		return (0);

	size_t	LETTER_INDEX;

	for (LETTER_INDEX = 0; LETTER_INDEX < CANDIDATE_LENGTH; LETTER_INDEX++)
		if (CANDIDATE[LETTER_INDEX] < 'a' || CANDIDATE[LETTER_INDEX] > 'z')
			return (0);

	return (!IN_WORD_LIST(CANDIDATE, FUNCTION_WORDS));
}

static const char	*IRREGULAR_PLURALS[13][2] = {
	{ "people", "person" }, { "men", "man" }, { "women", "woman" },
	{ "children", "child" }, { "feet", "foot" }, { "teeth", "tooth" },
	{ "mice", "mouse" }, { "geese", "goose" }, { "wolves", "wolf" },
	{ "leaves", "leaf" }, { "knives", "knife" }, { "halves", "half" },
	{ "oxen", "ox" }
};

static void
	MAKE_SINGULAR(const char *NOUN, char *SINGULAR_OUTPUT)
{
	size_t	NOUN_LENGTH = strlen(NOUN);

	snprintf(SINGULAR_OUTPUT, 40, "%s", NOUN);

	size_t	IRREGULAR_INDEX;

	for (
		IRREGULAR_INDEX = 0;
		IRREGULAR_INDEX <
			sizeof IRREGULAR_PLURALS / sizeof IRREGULAR_PLURALS[0];
		IRREGULAR_INDEX++
	)
		if (!strcmp(NOUN, IRREGULAR_PLURALS[IRREGULAR_INDEX][0]))
		{
			snprintf(
				SINGULAR_OUTPUT, 40, "%s", IRREGULAR_PLURALS[IRREGULAR_INDEX][1]
			);
			return ;
		}

	if (NOUN_LENGTH <= 3)
		return ;

	if (NOUN_LENGTH > 4 && ENDS_WITH(NOUN, "ies"))
	{
		SINGULAR_OUTPUT[NOUN_LENGTH - 3] = 'y';
		SINGULAR_OUTPUT[NOUN_LENGTH - 2] = 0;
		return ;
	}

	if (
		ENDS_WITH(NOUN, "ches") ||
		ENDS_WITH(NOUN, "shes") ||
		ENDS_WITH(NOUN, "sses") ||
		ENDS_WITH(NOUN, "xes") ||
		ENDS_WITH(NOUN, "zes")
	)
	{
		SINGULAR_OUTPUT[NOUN_LENGTH - 2] = 0;
		return ;
	}

	if (ENDS_WITH(NOUN, "ss") || ENDS_WITH(NOUN, "us") || ENDS_WITH(NOUN, "is"))
		return ;

	if (NOUN[NOUN_LENGTH - 1] == 's')
		SINGULAR_OUTPUT[NOUN_LENGTH - 1] = 0;
}

static void
	MAKE_PLURAL(const char *NOUN, char *PLURAL_OUTPUT)
{
	size_t	NOUN_LENGTH = strlen(NOUN);
	size_t	IRREGULAR_INDEX;

	for (
		IRREGULAR_INDEX = 0;
		IRREGULAR_INDEX <
			sizeof IRREGULAR_PLURALS / sizeof IRREGULAR_PLURALS[0];
		IRREGULAR_INDEX++
	)
		if (!strcmp(NOUN, IRREGULAR_PLURALS[IRREGULAR_INDEX][1]))
		{
			snprintf(
				PLURAL_OUTPUT, 40, "%s", IRREGULAR_PLURALS[IRREGULAR_INDEX][0]
			);
			return ;
		}

	if (
		NOUN_LENGTH > 1 &&
		NOUN[NOUN_LENGTH - 1] == 'y' &&
		!strchr("aeiou", NOUN[NOUN_LENGTH - 2])
	)
	{
		snprintf(PLURAL_OUTPUT, 40, "%.*sies", (int)(NOUN_LENGTH - 1), NOUN);
		return ;
	}

	if (
		ENDS_WITH(NOUN, "s") ||
		ENDS_WITH(NOUN, "x") ||
		ENDS_WITH(NOUN, "z") ||
		ENDS_WITH(NOUN, "ch") ||
		ENDS_WITH(NOUN, "sh")
	)
	{
		snprintf(PLURAL_OUTPUT, 40, "%ses", NOUN);
		return ;
	}

	snprintf(PLURAL_OUTPUT, 40, "%ss", NOUN);
}

static int
	VERB_LEMMA(const char *VERB_FORM, char *LEMMA_OUTPUT)
{
	size_t	VERB_LENGTH = strlen(VERB_FORM);

	snprintf(LEMMA_OUTPUT, 40, "%s", VERB_FORM);

	if (
		VERB_LENGTH < 3 ||
		VERB_FORM[VERB_LENGTH - 1] != 's' ||
		ENDS_WITH(VERB_FORM, "ss")
	)
		return (0);

	if (!strcmp(VERB_FORM, "does"))
	{
		snprintf(LEMMA_OUTPUT, 40, "do");
		return (1);
	}

	if (!strcmp(VERB_FORM, "goes"))
	{
		snprintf(LEMMA_OUTPUT, 40, "go");
		return (1);
	}

	if (VERB_LENGTH > 4 && ENDS_WITH(VERB_FORM, "ies"))
	{
		LEMMA_OUTPUT[VERB_LENGTH - 3] = 'y';
		LEMMA_OUTPUT[VERB_LENGTH - 2] = 0;
		return (1);
	}

	if (
		ENDS_WITH(VERB_FORM, "ches") ||
		ENDS_WITH(VERB_FORM, "shes") ||
		ENDS_WITH(VERB_FORM, "sses") ||
		ENDS_WITH(VERB_FORM, "xes") ||
		ENDS_WITH(VERB_FORM, "zes")
	)
	{
		LEMMA_OUTPUT[VERB_LENGTH - 2] = 0;
		return (1);
	}

	LEMMA_OUTPUT[VERB_LENGTH - 1] = 0;

	return (1);
}

static void
	VERB_THIRD_PERSON(const char *BASE_VERB, char *CONJUGATED_OUTPUT)
{
	char		HEAD_VERB[40];
	const char	*SPACE_POSITION = strchr(BASE_VERB, ' ');

	snprintf(
		HEAD_VERB, sizeof HEAD_VERB, "%.*s",
		SPACE_POSITION ? (int)(SPACE_POSITION - BASE_VERB)
		: (int)strlen(BASE_VERB),
		BASE_VERB
	);

	const char	*VERB_REST = SPACE_POSITION ? SPACE_POSITION : "";
	size_t		HEAD_LENGTH = strlen(HEAD_VERB);

	if (!strcmp(HEAD_VERB, "have"))
		snprintf(CONJUGATED_OUTPUT, 48, "has%s", VERB_REST);
	else if (!strcmp(HEAD_VERB, "be"))
		snprintf(CONJUGATED_OUTPUT, 48, "is%s", VERB_REST);
	else if (!strcmp(HEAD_VERB, "do") || !strcmp(HEAD_VERB, "go"))
		snprintf(CONJUGATED_OUTPUT, 48, "%ses%s", HEAD_VERB, VERB_REST);
	else if (
		HEAD_LENGTH > 1 &&
		HEAD_VERB[HEAD_LENGTH - 1] == 'y' &&
		!strchr("aeiou", HEAD_VERB[HEAD_LENGTH - 2])
	)
		snprintf(
			CONJUGATED_OUTPUT, 48, "%.*sies%s", (int)(HEAD_LENGTH - 1),
			HEAD_VERB, VERB_REST
		);
	else if (
		ENDS_WITH(HEAD_VERB, "s") ||
		ENDS_WITH(HEAD_VERB, "x") ||
		ENDS_WITH(HEAD_VERB, "z") ||
		ENDS_WITH(HEAD_VERB, "ch") ||
		ENDS_WITH(HEAD_VERB, "sh")
	)
		snprintf(CONJUGATED_OUTPUT, 48, "%ses%s", HEAD_VERB, VERB_REST);
	else
		snprintf(CONJUGATED_OUTPUT, 48, "%ss%s", HEAD_VERB, VERB_REST);
}

static int
	PARSE_COUNT_WORD(const char *NUMBER_TOKEN)
{
	static const char	*NUMBER_WORDS[21] = {
		"zero", "one", "two", "three", "four", "five", "six", "seven", "eight",
		"nine", "ten", "eleven", "twelve", "thirteen", "fourteen", "fifteen",
		"sixteen", "seventeen", "eighteen", "nineteen", "twenty"
	};

	if (isdigit((unsigned char)NUMBER_TOKEN[0]))
	{
		const char	*DIGIT_CURSOR;

		for (DIGIT_CURSOR = NUMBER_TOKEN; *DIGIT_CURSOR; DIGIT_CURSOR++)
			if (!isdigit((unsigned char)*DIGIT_CURSOR))
				return (COUNT_NONE);

		return (atoi(NUMBER_TOKEN));
	}

	int	NUMBER_VALUE;

	for (NUMBER_VALUE = 0; NUMBER_VALUE < 21; NUMBER_VALUE++)
		if (!strcmp(NUMBER_TOKEN, NUMBER_WORDS[NUMBER_VALUE]))
			return (NUMBER_VALUE);

	if (!strcmp(NUMBER_TOKEN, "a") || !strcmp(NUMBER_TOKEN, "an"))
		return (1);

	if (IN_WORD_LIST(NUMBER_TOKEN, "many some several"))
		return (COUNT_MANY);

	return (COUNT_NONE);
}

static void
	TOKEN_LIST_ADD(TOKEN_LIST *TOKENS, const char *NEW_TOKEN)
{
	if (TOKENS->COUNT < MAXIMUM_TOKENS && *NEW_TOKEN)
		snprintf(TOKENS->WORDS[TOKENS->COUNT++], 40, "%s", NEW_TOKEN);
}

static void
	TOKENIZE(const char *INPUT_STRING, TOKEN_LIST *TOKENS)
{
	memset(TOKENS, 0, sizeof *TOKENS);

	char		CURRENT_TOKEN[64];
	int			TOKEN_LENGTH = 0;
	const char	*CURSOR;

	for (CURSOR = INPUT_STRING;; CURSOR++)
	{
		int	CHARACTER = (unsigned char)*CURSOR;

		if (CHARACTER == '?')
			TOKENS->IS_QUESTION = 1;

		if (CHARACTER && (isalnum(CHARACTER) || CHARACTER == '\''))
		{
			if (TOKEN_LENGTH < 60)
				CURRENT_TOKEN[TOKEN_LENGTH++] = (char)tolower(CHARACTER);

			continue ;
		}

		CURRENT_TOKEN[TOKEN_LENGTH] = 0;

		if (TOKEN_LENGTH)
		{
			char	*APOSTROPHE = strchr(CURRENT_TOKEN, '\'');

			if (!APOSTROPHE)
				TOKEN_LIST_ADD(TOKENS, CURRENT_TOKEN);
			else if (
				!strcmp(CURRENT_TOKEN, "can't") ||
				!strcmp(CURRENT_TOKEN, "cannot")
			)
			{
				TOKEN_LIST_ADD(TOKENS, "can");
				TOKEN_LIST_ADD(TOKENS, "not");
			}
			else if (!strcmp(CURRENT_TOKEN, "won't"))
			{
				TOKEN_LIST_ADD(TOKENS, "will");
				TOKEN_LIST_ADD(TOKENS, "not");
			}
			else if (ENDS_WITH(CURRENT_TOKEN, "n't"))
			{
				CURRENT_TOKEN[strlen(CURRENT_TOKEN) - 3] = 0;
				TOKEN_LIST_ADD(TOKENS, CURRENT_TOKEN);
				TOKEN_LIST_ADD(TOKENS, "not");
			}
			else if (
				!strcmp(APOSTROPHE, "'s") &&
				(
					!strcmp(CURRENT_TOKEN, "it's") ||
					!strcmp(CURRENT_TOKEN, "what's") ||
					!strcmp(CURRENT_TOKEN, "that's") ||
					!strcmp(CURRENT_TOKEN, "there's")
				)
			)
			{
				*APOSTROPHE = 0;
				TOKEN_LIST_ADD(TOKENS, CURRENT_TOKEN);
				TOKEN_LIST_ADD(TOKENS, "is");
			}
			else if (!strcmp(APOSTROPHE, "'re"))
			{
				*APOSTROPHE = 0;
				TOKEN_LIST_ADD(TOKENS, CURRENT_TOKEN);
				TOKEN_LIST_ADD(TOKENS, "are");
			}
			else
				TOKEN_LIST_ADD(TOKENS, CURRENT_TOKEN);
		}

		TOKEN_LENGTH = 0;

		if (!CHARACTER)
			break ;
	}

	int	TOKEN_INDEX;

	for (TOKEN_INDEX = 0; TOKEN_INDEX < TOKENS->COUNT; TOKEN_INDEX++)
		if (!strcmp(TOKENS->WORDS[TOKEN_INDEX], "cannot"))
		{
			snprintf(TOKENS->WORDS[TOKEN_INDEX], 40, "can");

			if (TOKENS->COUNT < MAXIMUM_TOKENS)
			{
				memmove(
					TOKENS->WORDS[TOKEN_INDEX + 2],
					TOKENS->WORDS[TOKEN_INDEX + 1],
					(size_t)(TOKENS->COUNT - TOKEN_INDEX - 1) * 40
				);
				snprintf(TOKENS->WORDS[TOKEN_INDEX + 1], 40, "not");
				TOKENS->COUNT++;
			}
		}

	int	LEADING_COUNT = 0;

	while (LEADING_COUNT < TOKENS->COUNT)
	{
		const char	*LEADING_TOKEN = TOKENS->WORDS[LEADING_COUNT];

		if (IN_WORD_LIST(LEADING_TOKEN, "actually correction wait"))
		{
			TOKENS->IS_CORRECTION = 1;
			LEADING_COUNT++;
			continue ;
		}

		if (!strcmp(LEADING_TOKEN, "no") && TOKENS->COUNT - LEADING_COUNT > 3)
		{
			TOKENS->IS_CORRECTION = 1;
			LEADING_COUNT++;
			continue ;
		}

		if (IN_WORD_LIST(LEADING_TOKEN, "and so also well oh um hey then"))
		{
			LEADING_COUNT++;
			continue ;
		}

		break ;
	}

	if (LEADING_COUNT)
	{
		memmove(
			TOKENS->WORDS[0], TOKENS->WORDS[LEADING_COUNT],
			(size_t)(TOKENS->COUNT - LEADING_COUNT) * 40
		);
		TOKENS->COUNT -= LEADING_COUNT;
	}

	while (
		TOKENS->COUNT &&
		IN_WORD_LIST(TOKENS->WORDS[TOKENS->COUNT - 1], "too also")
	)
		TOKENS->COUNT--;

	if (
		TOKENS->COUNT &&
		IN_WORD_LIST(TOKENS->WORDS[0], "is are does do can what which who how")
	)
		TOKENS->IS_QUESTION = 1;
}

static int
	NOUN_PHRASE_HEAD(
		TOKEN_LIST *TOKENS, int *TOKEN_POSITION, char *HEAD_NOUN, int *IS_PLURAL
	)
{
	int	NOUN_INDEX = *TOKEN_POSITION;

	if (
		NOUN_INDEX < TOKENS->COUNT &&
		IN_WORD_LIST(TOKENS->WORDS[NOUN_INDEX], KNOWLEDGE_DETERMINERS)
	)
		NOUN_INDEX++;

	if (
		NOUN_INDEX >= TOKENS->COUNT ||
		!IS_LOWERCASE_WORD(TOKENS->WORDS[NOUN_INDEX])
	)
		return (0);

	MAKE_SINGULAR(TOKENS->WORDS[NOUN_INDEX], HEAD_NOUN);

	if (IS_PLURAL)
		*IS_PLURAL = strcmp(HEAD_NOUN, TOKENS->WORDS[NOUN_INDEX]) != 0;

	*TOKEN_POSITION = NOUN_INDEX + 1;

	return (1);
}

static int
	NOUN_PHRASE_OBJECT(
		TOKEN_LIST *TOKENS, int START_INDEX, int END_INDEX,
		PARSED_SENTENCE *PARSED, int KEEP_MODIFIERS
	)
{
	if (
		START_INDEX < END_INDEX &&
		IN_WORD_LIST(TOKENS->WORDS[START_INDEX], KNOWLEDGE_DETERMINERS)
	)
		START_INDEX++;

	if (START_INDEX >= END_INDEX)
		return (0);

	int	TOKEN_INDEX;

	for (TOKEN_INDEX = START_INDEX; TOKEN_INDEX < END_INDEX; TOKEN_INDEX++)
		if (!IS_LOWERCASE_WORD(TOKENS->WORDS[TOKEN_INDEX]))
			return (0);

	MAKE_SINGULAR(TOKENS->WORDS[END_INDEX - 1], PARSED->OBJECT_NAME);

	if (KEEP_MODIFIERS)
	{
		int	TOKEN_INDEX;

		for (
			TOKEN_INDEX = START_INDEX;
			TOKEN_INDEX < END_INDEX - 1 && PARSED->PROPERTY_COUNT < 8;
			TOKEN_INDEX++
		)
			snprintf(
				PARSED->PROPERTIES[PARSED->PROPERTY_COUNT++], 40, "%s",
				TOKENS->WORDS[TOKEN_INDEX]
			);
	}

	return (1);
}

static int
	VERB_AT(
		TOKEN_LIST *TOKENS, int *TOKEN_POSITION, int IS_BASE_FORM,
		char *VERB_OUTPUT
	)
{
	if (
		*TOKEN_POSITION >= TOKENS->COUNT ||
		!IS_LOWERCASE_WORD(TOKENS->WORDS[*TOKEN_POSITION])
	)
		return (0);

	char	LEMMA[40];

	if (IS_BASE_FORM)
		snprintf(LEMMA, sizeof LEMMA, "%s", TOKENS->WORDS[*TOKEN_POSITION]);
	else if (!VERB_LEMMA(TOKENS->WORDS[*TOKEN_POSITION], LEMMA))
		return (0);

	(*TOKEN_POSITION)++;

	if (
		*TOKEN_POSITION < TOKENS->COUNT &&
		IN_WORD_LIST(TOKENS->WORDS[*TOKEN_POSITION], KNOWLEDGE_PREPOSITIONS)
	)
	{
		snprintf(
			VERB_OUTPUT, 40, "%s %s", LEMMA, TOKENS->WORDS[*TOKEN_POSITION]
		);
		(*TOKEN_POSITION)++;
	}
	else
		snprintf(VERB_OUTPUT, 40, "%s", LEMMA);

	return (1);
}

static int
	PARSE_IS_TAIL(
		TOKEN_LIST *TOKENS, int TOKEN_INDEX, int IS_PLURAL,
		PARSED_SENTENCE *PARSED
	)
{
	int	TOKEN_COUNT = TOKENS->COUNT;

	if (TOKEN_INDEX < TOKEN_COUNT && !strcmp(TOKENS->WORDS[TOKEN_INDEX], "not"))
	{
		PARSED->IS_NEGATED = 1;
		TOKEN_INDEX++;
	}

	while (
		TOKEN_INDEX < TOKEN_COUNT &&
		IN_WORD_LIST(TOKENS->WORDS[TOKEN_INDEX], ADVERB_WORDS)
	)
		TOKEN_INDEX++;

	if (
		TOKEN_INDEX >= TOKEN_COUNT ||
		IN_WORD_LIST(TOKENS->WORDS[TOKEN_INDEX], KNOWLEDGE_PREPOSITIONS)
	)
		return (0);

	if (
		!strcmp(TOKENS->WORDS[TOKEN_INDEX], "a") ||
		!strcmp(TOKENS->WORDS[TOKEN_INDEX], "an")
	)
	{
		TOKEN_INDEX++;

		if (
			TOKEN_INDEX + 1 < TOKEN_COUNT &&
			IN_WORD_LIST(TOKENS->WORDS[TOKEN_INDEX], "kind type sort") &&
			!strcmp(TOKENS->WORDS[TOKEN_INDEX + 1], "of")
		)
			TOKEN_INDEX += 2;

		if (
			!NOUN_PHRASE_OBJECT(
				TOKENS, TOKEN_INDEX, TOKEN_COUNT, PARSED, !PARSED->IS_NEGATED
			)
		)
			return (0);

		PARSED->RELATION = RELATION_IS_A;
		return (1);
	}

	char	LAST_SINGULAR[40];

	MAKE_SINGULAR(TOKENS->WORDS[TOKEN_COUNT - 1], LAST_SINGULAR);

	if (
		IS_PLURAL &&
		IS_LOWERCASE_WORD(TOKENS->WORDS[TOKEN_COUNT - 1]) &&
		strcmp(LAST_SINGULAR, TOKENS->WORDS[TOKEN_COUNT - 1])
	)
	{
		if (
			!NOUN_PHRASE_OBJECT(
				TOKENS, TOKEN_INDEX, TOKEN_COUNT, PARSED, !PARSED->IS_NEGATED
			)
		)
			return (0);

		PARSED->RELATION = RELATION_IS_A;
		return (1);
	}

	PARSED->RELATION = RELATION_IS;

	for (; TOKEN_INDEX < TOKEN_COUNT; TOKEN_INDEX++)
	{
		if (!strcmp(TOKENS->WORDS[TOKEN_INDEX], "and"))
			continue ;

		if (
			!IS_LOWERCASE_WORD(TOKENS->WORDS[TOKEN_INDEX]) ||
			PARSED->PROPERTY_COUNT >= 8
		)
			return (0);

		snprintf(
			PARSED->PROPERTIES[PARSED->PROPERTY_COUNT++], 40, "%s",
			TOKENS->WORDS[TOKEN_INDEX]
		);
	}

	return (PARSED->PROPERTY_COUNT > 0);
}

static int
	PARSE_STATEMENT(TOKEN_LIST *TOKENS, PARSED_SENTENCE *PARSED)
{
	int	TOKEN_INDEX = 0;
	int	IS_PLURAL = 0;
	int	TOKEN_COUNT = TOKENS->COUNT;

	if (TOKEN_COUNT && IN_WORD_LIST(TOKENS->WORDS[0], IMPERATIVE_WORDS))
		return (0);

	if (
		!NOUN_PHRASE_HEAD(TOKENS, &TOKEN_INDEX, PARSED->SUBJECT, &IS_PLURAL) ||
		TOKEN_INDEX >= TOKEN_COUNT
	)
		return (0);

	const char	*VERB_TOKEN = TOKENS->WORDS[TOKEN_INDEX];

	PARSED->KIND = SENTENCE_STATEMENT;

	if (!strcmp(VERB_TOKEN, "is") || !strcmp(VERB_TOKEN, "are"))
		return (PARSE_IS_TAIL(TOKENS, TOKEN_INDEX + 1, IS_PLURAL, PARSED));

	if (!strcmp(VERB_TOKEN, "has") || !strcmp(VERB_TOKEN, "have"))
	{
		TOKEN_INDEX++;

		if (
			TOKEN_INDEX < TOKEN_COUNT &&
			!strcmp(TOKENS->WORDS[TOKEN_INDEX], "not")
		)
		{
			PARSED->IS_NEGATED = 1;
			TOKEN_INDEX++;
		}

		if (
			TOKEN_INDEX < TOKEN_COUNT &&
			!strcmp(TOKENS->WORDS[TOKEN_INDEX], "no")
		)
		{
			PARSED->IS_NEGATED = 1;
			TOKEN_INDEX++;
		}
		else if (TOKEN_INDEX < TOKEN_COUNT)
		{
			int	PARSED_COUNT = PARSE_COUNT_WORD(TOKENS->WORDS[TOKEN_INDEX]);

			if (PARSED_COUNT != COUNT_NONE)
			{
				PARSED->COUNT = PARSED_COUNT;
				TOKEN_INDEX++;
			}
		}

		if (!NOUN_PHRASE_OBJECT(TOKENS, TOKEN_INDEX, TOKEN_COUNT, PARSED, 0))
			return (0);

		PARSED->RELATION = RELATION_HAS;
		return (1);
	}

	if (
		(!strcmp(VERB_TOKEN, "does") || !strcmp(VERB_TOKEN, "do")) &&
		TOKEN_INDEX + 1 < TOKEN_COUNT &&
		!strcmp(TOKENS->WORDS[TOKEN_INDEX + 1], "not")
	)
	{
		TOKEN_INDEX += 2;
		PARSED->IS_NEGATED = 1;

		if (
			TOKEN_INDEX < TOKEN_COUNT &&
			!strcmp(TOKENS->WORDS[TOKEN_INDEX], "have")
		)
		{
			TOKEN_INDEX++;

			if (TOKEN_INDEX < TOKEN_COUNT)
			{
				int	PARSED_COUNT = PARSE_COUNT_WORD(TOKENS->WORDS[TOKEN_INDEX]);

				if (PARSED_COUNT != COUNT_NONE)
				{
					PARSED->COUNT = PARSED_COUNT;
					TOKEN_INDEX++;
				}
			}

			if (
				!NOUN_PHRASE_OBJECT(TOKENS, TOKEN_INDEX, TOKEN_COUNT, PARSED, 0)
			)
				return (0);

			PARSED->RELATION = RELATION_HAS;
			return (1);
		}

		if (!VERB_AT(TOKENS, &TOKEN_INDEX, 1, PARSED->VERB))
			return (0);

		if (
			TOKEN_INDEX < TOKEN_COUNT &&
			!NOUN_PHRASE_OBJECT(TOKENS, TOKEN_INDEX, TOKEN_COUNT, PARSED, 0)
		)
			return (0);

		PARSED->RELATION = RELATION_DO;
		return (1);
	}

	if (!strcmp(VERB_TOKEN, "can"))
	{
		TOKEN_INDEX++;

		if (
			TOKEN_INDEX < TOKEN_COUNT &&
			!strcmp(TOKENS->WORDS[TOKEN_INDEX], "not")
		)
		{
			PARSED->IS_NEGATED = 1;
			TOKEN_INDEX++;
		}

		if (!VERB_AT(TOKENS, &TOKEN_INDEX, 1, PARSED->VERB))
			return (0);

		if (
			TOKEN_INDEX < TOKEN_COUNT &&
			!NOUN_PHRASE_OBJECT(TOKENS, TOKEN_INDEX, TOKEN_COUNT, PARSED, 0)
		)
			return (0);

		PARSED->RELATION = RELATION_CAN;
		return (1);
	}

	if (!VERB_AT(TOKENS, &TOKEN_INDEX, IS_PLURAL, PARSED->VERB))
		return (0);

	if (
		TOKEN_INDEX < TOKEN_COUNT &&
		!NOUN_PHRASE_OBJECT(TOKENS, TOKEN_INDEX, TOKEN_COUNT, PARSED, 0)
	)
		return (0);

	PARSED->RELATION = RELATION_DO;

	return (1);
}

static int
	PARSE_QUESTION(TOKEN_LIST *TOKENS, PARSED_SENTENCE *PARSED)
{
	int	TOKEN_COUNT = TOKENS->COUNT;
	int	TOKEN_INDEX;
	int	IS_PLURAL = 0;

	if (!TOKEN_COUNT)
		return (0);

	const char	*QUESTION_WORD = TOKENS->WORDS[0];

	if (!strcmp(QUESTION_WORD, "is") || !strcmp(QUESTION_WORD, "are"))
	{
		TOKEN_INDEX = 1;

		if (
			!NOUN_PHRASE_HEAD(TOKENS, &TOKEN_INDEX, PARSED->SUBJECT, &IS_PLURAL)
		)
			return (0);

		PARSED->KIND = SENTENCE_CHECK;

		if (!PARSE_IS_TAIL(TOKENS, TOKEN_INDEX, IS_PLURAL, PARSED))
			return (0);

		if (PARSED->RELATION == RELATION_IS)
		{
			if (PARSED->PROPERTY_COUNT != 1)
				return (0);

			snprintf(PARSED->OBJECT_NAME, 40, "%s", PARSED->PROPERTIES[0]);
		}

		PARSED->PROPERTY_COUNT = 0;
		return (1);
	}

	if (!strcmp(QUESTION_WORD, "does") || !strcmp(QUESTION_WORD, "do"))
	{
		TOKEN_INDEX = 1;

		if (
			!NOUN_PHRASE_HEAD(
				TOKENS, &TOKEN_INDEX, PARSED->SUBJECT, &IS_PLURAL
			) ||
			TOKEN_INDEX >= TOKEN_COUNT
		)
			return (0);

		PARSED->KIND = SENTENCE_CHECK;

		if (!strcmp(TOKENS->WORDS[TOKEN_INDEX], "have"))
		{
			TOKEN_INDEX++;

			if (TOKEN_INDEX < TOKEN_COUNT)
			{
				int	PARSED_COUNT = PARSE_COUNT_WORD(TOKENS->WORDS[TOKEN_INDEX]);

				if (PARSED_COUNT != COUNT_NONE)
				{
					PARSED->COUNT = PARSED_COUNT;
					TOKEN_INDEX++;
				}
			}

			if (
				!NOUN_PHRASE_OBJECT(TOKENS, TOKEN_INDEX, TOKEN_COUNT, PARSED, 0)
			)
				return (0);

			PARSED->RELATION = RELATION_HAS;
			return (1);
		}

		if (!VERB_AT(TOKENS, &TOKEN_INDEX, 1, PARSED->VERB))
			return (0);

		if (
			TOKEN_INDEX < TOKEN_COUNT &&
			!NOUN_PHRASE_OBJECT(TOKENS, TOKEN_INDEX, TOKEN_COUNT, PARSED, 0)
		)
			return (0);

		PARSED->RELATION = RELATION_DO;
		return (1);
	}

	if (!strcmp(QUESTION_WORD, "can"))
	{
		TOKEN_INDEX = 1;

		if (
			!NOUN_PHRASE_HEAD(TOKENS, &TOKEN_INDEX, PARSED->SUBJECT, &IS_PLURAL)
		)
			return (0);

		if (!VERB_AT(TOKENS, &TOKEN_INDEX, 1, PARSED->VERB))
			return (0);

		if (
			TOKEN_INDEX < TOKEN_COUNT &&
			!NOUN_PHRASE_OBJECT(TOKENS, TOKEN_INDEX, TOKEN_COUNT, PARSED, 0)
		)
			return (0);

		PARSED->KIND = SENTENCE_CHECK;
		PARSED->RELATION = RELATION_CAN;
		return (1);
	}

	if (
		!strcmp(QUESTION_WORD, "how") &&
		TOKEN_COUNT > 5 &&
		!strcmp(TOKENS->WORDS[1], "many")
	)
	{
		if (!IS_LOWERCASE_WORD(TOKENS->WORDS[2]))
			return (0);

		MAKE_SINGULAR(TOKENS->WORDS[2], PARSED->OBJECT_NAME);

		if (strcmp(TOKENS->WORDS[3], "does") && strcmp(TOKENS->WORDS[3], "do"))
			return (0);

		TOKEN_INDEX = 4;

		if (
			!NOUN_PHRASE_HEAD(
				TOKENS, &TOKEN_INDEX, PARSED->SUBJECT, &IS_PLURAL
			) ||
			TOKEN_INDEX != TOKEN_COUNT - 1 ||
			strcmp(TOKENS->WORDS[TOKEN_INDEX], "have")
		)
			return (0);

		PARSED->KIND = SENTENCE_COUNT;
		PARSED->RELATION = RELATION_HAS;
		return (1);
	}

	if (IN_WORD_LIST(QUESTION_WORD, "what which who"))
	{
		TOKEN_INDEX = 1;

		int	ASKS_PLURAL = 0;

		if (
			TOKEN_INDEX < TOKEN_COUNT &&
			IN_WORD_LIST(TOKENS->WORDS[TOKEN_INDEX], "thing things")
		)
		{
			ASKS_PLURAL = !strcmp(TOKENS->WORDS[TOKEN_INDEX], "things");
			TOKEN_INDEX++;
		}

		if (TOKEN_INDEX >= TOKEN_COUNT)
			return (0);

		const char	*VERB_TOKEN = TOKENS->WORDS[TOKEN_INDEX];

		if (!strcmp(VERB_TOKEN, "is") || !strcmp(VERB_TOKEN, "are"))
		{
			TOKEN_INDEX++;

			if (
				!NOUN_PHRASE_HEAD(
					TOKENS, &TOKEN_INDEX, PARSED->SUBJECT, &IS_PLURAL
				) ||
				TOKEN_INDEX != TOKEN_COUNT
			)
				return (0);

			PARSED->KIND = SENTENCE_DESCRIBE;
			return (1);
		}

		if (!strcmp(VERB_TOKEN, "does") || !strcmp(VERB_TOKEN, "do"))
		{
			TOKEN_INDEX++;

			if (
				!NOUN_PHRASE_HEAD(
					TOKENS, &TOKEN_INDEX, PARSED->SUBJECT, &IS_PLURAL
				) ||
				TOKEN_INDEX >= TOKEN_COUNT
			)
				return (0);

			PARSED->KIND = SENTENCE_LIST;

			if (
				!strcmp(TOKENS->WORDS[TOKEN_INDEX], "have") &&
				TOKEN_INDEX == TOKEN_COUNT - 1
			)
			{
				PARSED->RELATION = RELATION_HAS;
				return (1);
			}

			if (
				!VERB_AT(TOKENS, &TOKEN_INDEX, 1, PARSED->VERB) ||
				TOKEN_INDEX != TOKEN_COUNT
			)
				return (0);

			PARSED->RELATION = RELATION_DO;
			return (1);
		}

		if (!strcmp(VERB_TOKEN, "can"))
		{
			int		PROBE_INDEX = TOKEN_INDEX + 1;
			char	TEMPORARY_SUBJECT[40];

			if (
				NOUN_PHRASE_HEAD(
					TOKENS, &PROBE_INDEX, TEMPORARY_SUBJECT, &IS_PLURAL
				) &&
				PROBE_INDEX == TOKEN_COUNT - 1 &&
				!strcmp(TOKENS->WORDS[PROBE_INDEX], "do")
			)
			{
				snprintf(PARSED->SUBJECT, 40, "%s", TEMPORARY_SUBJECT);
				PARSED->KIND = SENTENCE_LIST;
				PARSED->RELATION = RELATION_CAN;
				return (1);
			}

			TOKEN_INDEX++;

			if (!VERB_AT(TOKENS, &TOKEN_INDEX, 1, PARSED->VERB))
				return (0);

			if (
				TOKEN_INDEX < TOKEN_COUNT &&
				!NOUN_PHRASE_OBJECT(TOKENS, TOKEN_INDEX, TOKEN_COUNT, PARSED, 0)
			)
				return (0);

			PARSED->KIND = SENTENCE_REVERSE;
			PARSED->RELATION = RELATION_CAN;
			return (1);
		}

		if (!strcmp(VERB_TOKEN, "has") || !strcmp(VERB_TOKEN, "have"))
		{
			TOKEN_INDEX++;

			if (TOKEN_INDEX < TOKEN_COUNT)
			{
				int	PARSED_COUNT = PARSE_COUNT_WORD(TOKENS->WORDS[TOKEN_INDEX]);

				if (PARSED_COUNT != COUNT_NONE)
				{
					PARSED->COUNT = PARSED_COUNT;
					TOKEN_INDEX++;
				}
			}

			if (
				!NOUN_PHRASE_OBJECT(TOKENS, TOKEN_INDEX, TOKEN_COUNT, PARSED, 0)
			)
				return (0);

			PARSED->KIND = SENTENCE_REVERSE;
			PARSED->RELATION = RELATION_HAS;
			return (1);
		}

		if (!VERB_AT(TOKENS, &TOKEN_INDEX, ASKS_PLURAL, PARSED->VERB))
			return (0);

		if (
			TOKEN_INDEX < TOKEN_COUNT &&
			!NOUN_PHRASE_OBJECT(TOKENS, TOKEN_INDEX, TOKEN_COUNT, PARSED, 0)
		)
			return (0);

		PARSED->KIND = SENTENCE_REVERSE;
		PARSED->RELATION = RELATION_DO;
		return (1);
	}

	return (0);
}

KNOWLEDGE
	*KNOWLEDGE_NEW(void)
{
	KNOWLEDGE	*KNOWLEDGE_BASE = calloc(1, sizeof *KNOWLEDGE_BASE);

	KNOWLEDGE_BASE->ABOUT_SYMBOL = -1;

	return (KNOWLEDGE_BASE);
}

void
	KNOWLEDGE_FREE(KNOWLEDGE *KNOWLEDGE_BASE)
{
	if (!KNOWLEDGE_BASE)
		return ;

	free(KNOWLEDGE_BASE->SYMBOLS);
	free(KNOWLEDGE_BASE->ASKED_COUNTS);
	free(KNOWLEDGE_BASE->SYMBOL_KINDS);
	free(KNOWLEDGE_BASE->FACTS);
	free(KNOWLEDGE_BASE);
}

void
	KNOWLEDGE_RESET(KNOWLEDGE *KNOWLEDGE_BASE)
{
	KNOWLEDGE_BASE->SYMBOL_COUNT = 0;
	KNOWLEDGE_BASE->FACT_COUNT = 0;
	KNOWLEDGE_BASE->OPEN_QUESTION_COUNT = 0;
	KNOWLEDGE_BASE->TURN = 0;
	KNOWLEDGE_BASE->IS_DIRTY = 1;
	KNOWLEDGE_BASE->ABOUT_SYMBOL = -1;
	KNOWLEDGE_BASE->LAST_REPLY[0] = 0;
}

static int
	SYMBOL_FIND(KNOWLEDGE *KNOWLEDGE_BASE, const char *SYMBOL_NAME)
{
	if (!SYMBOL_NAME || !*SYMBOL_NAME)
		return (-1);

	int	SYMBOL_INDEX;

	for (
		SYMBOL_INDEX = 0;
		SYMBOL_INDEX < KNOWLEDGE_BASE->SYMBOL_COUNT;
		SYMBOL_INDEX++
	)
		if (!strcmp(KNOWLEDGE_BASE->SYMBOLS[SYMBOL_INDEX], SYMBOL_NAME))
			return (SYMBOL_INDEX);

	return (-1);
}

static int
	SYMBOL_GET(
		KNOWLEDGE *KNOWLEDGE_BASE, const char *SYMBOL_NAME, int SYMBOL_KIND
	)
{
	if (!SYMBOL_NAME || !*SYMBOL_NAME)
		return (-1);

	int	SYMBOL_INDEX = SYMBOL_FIND(KNOWLEDGE_BASE, SYMBOL_NAME);

	if (SYMBOL_INDEX < 0)
	{
		if (KNOWLEDGE_BASE->SYMBOL_COUNT == KNOWLEDGE_BASE->SYMBOL_CAPACITY)
		{
			if (KNOWLEDGE_BASE->SYMBOL_CAPACITY)
				KNOWLEDGE_BASE->SYMBOL_CAPACITY =
					KNOWLEDGE_BASE->SYMBOL_CAPACITY * 2;
			else
				KNOWLEDGE_BASE->SYMBOL_CAPACITY = 64;

			KNOWLEDGE_BASE->SYMBOLS = realloc(
				KNOWLEDGE_BASE->SYMBOLS,
				(size_t)KNOWLEDGE_BASE->SYMBOL_CAPACITY *
					sizeof *KNOWLEDGE_BASE->SYMBOLS
			);
			KNOWLEDGE_BASE->ASKED_COUNTS = realloc(
				KNOWLEDGE_BASE->ASKED_COUNTS,
				(size_t)KNOWLEDGE_BASE->SYMBOL_CAPACITY *
					sizeof *KNOWLEDGE_BASE->ASKED_COUNTS
			);
			KNOWLEDGE_BASE->SYMBOL_KINDS = realloc(
				KNOWLEDGE_BASE->SYMBOL_KINDS,
				(size_t)KNOWLEDGE_BASE->SYMBOL_CAPACITY
			);
		}

		SYMBOL_INDEX = KNOWLEDGE_BASE->SYMBOL_COUNT++;
		snprintf(
			KNOWLEDGE_BASE->SYMBOLS[SYMBOL_INDEX],
			sizeof KNOWLEDGE_BASE->SYMBOLS[SYMBOL_INDEX], "%s", SYMBOL_NAME
		);
		KNOWLEDGE_BASE->ASKED_COUNTS[SYMBOL_INDEX] = 0;
		KNOWLEDGE_BASE->SYMBOL_KINDS[SYMBOL_INDEX] = 0;
	}

	KNOWLEDGE_BASE->SYMBOL_KINDS[SYMBOL_INDEX] |= (unsigned char)SYMBOL_KIND;

	return (SYMBOL_INDEX);
}

static int
	FACT_ADD(
		KNOWLEDGE *KNOWLEDGE_BASE, int SOURCE_KIND, int RELATION_KIND,
		int SUBJECT_ID, int VERB_ID, int OBJECT_ID, int QUANTITY, int NEGATED
	)
{
	if (KNOWLEDGE_BASE->FACT_COUNT == KNOWLEDGE_BASE->FACT_CAPACITY)
	{
		if (KNOWLEDGE_BASE->FACT_CAPACITY)
			KNOWLEDGE_BASE->FACT_CAPACITY = KNOWLEDGE_BASE->FACT_CAPACITY * 2;
		else
			KNOWLEDGE_BASE->FACT_CAPACITY = 128;

		KNOWLEDGE_BASE->FACTS = realloc(
			KNOWLEDGE_BASE->FACTS,
			(size_t)KNOWLEDGE_BASE->FACT_CAPACITY *
				sizeof *KNOWLEDGE_BASE->FACTS
		);
	}

	KNOWLEDGE_FACT	*NEW_FACT =
		&KNOWLEDGE_BASE->FACTS[KNOWLEDGE_BASE->FACT_COUNT];

	NEW_FACT->SUBJECT = SUBJECT_ID;
	NEW_FACT->RELATION = RELATION_KIND;
	NEW_FACT->VERB = VERB_ID;
	NEW_FACT->OBJECT_NAME = OBJECT_ID;
	NEW_FACT->COUNT = QUANTITY;
	NEW_FACT->IS_NEGATED = NEGATED;
	NEW_FACT->SOURCE = SOURCE_KIND;
	NEW_FACT->TURN = KNOWLEDGE_BASE->TURN;
	NEW_FACT->IS_ALIVE = 1;
	KNOWLEDGE_BASE->IS_DIRTY = 1;

	return (KNOWLEDGE_BASE->FACT_COUNT++);
}

static const char
	*ARTICLE_FOR(const char *NOUN)
{
	return (strchr("aeiou", NOUN[0]) ? "an" : "a");
}

static void
	RENDER_FACT(
		KNOWLEDGE *KNOWLEDGE_BASE, int SUBJECT_ID, int RELATION_KIND,
		int VERB_ID, int OBJECT_ID, int QUANTITY, int NEGATED, char *SENTENCE,
		int SENTENCE_SIZE
	)
{
	const char	*SUBJECT_TEXT =
		SUBJECT_ID >= 0 ? KNOWLEDGE_BASE->SYMBOLS[SUBJECT_ID] : "?";
	const char	*OBJECT_TEXT =
		OBJECT_ID >= 0 ? KNOWLEDGE_BASE->SYMBOLS[OBJECT_ID] : "";
	char		PLURAL_NOUN[40];
	char		CONJUGATED_VERB[48];

	switch (RELATION_KIND)
	{
		case RELATION_IS_A:
		{
			snprintf(
				SENTENCE, SENTENCE_SIZE, "%s is %s%s %s", SUBJECT_TEXT,
				NEGATED ? "not " : "", ARTICLE_FOR(OBJECT_TEXT), OBJECT_TEXT
			);
		}
		break ;
		case RELATION_IS:
		{
			snprintf(
				SENTENCE, SENTENCE_SIZE, "%s is %s%s", SUBJECT_TEXT,
				NEGATED ? "not " : "", OBJECT_TEXT
			);
		}
		break ;
		case RELATION_HAS:
		{
			MAKE_PLURAL(OBJECT_TEXT, PLURAL_NOUN);

			if (NEGATED)
				snprintf(
					SENTENCE, SENTENCE_SIZE, "%s has no %s", SUBJECT_TEXT,
					PLURAL_NOUN
				);
			else if (QUANTITY == 1)
				snprintf(
					SENTENCE, SENTENCE_SIZE, "%s has %s %s", SUBJECT_TEXT,
					ARTICLE_FOR(OBJECT_TEXT), OBJECT_TEXT
				);
			else if (QUANTITY >= 0)
				snprintf(
					SENTENCE, SENTENCE_SIZE, "%s has %d %s", SUBJECT_TEXT,
					QUANTITY, PLURAL_NOUN
				);
			else
				snprintf(
					SENTENCE, SENTENCE_SIZE, "%s has %s", SUBJECT_TEXT,
					PLURAL_NOUN
				);
		}
		break ;
		case RELATION_CAN:
		{
			snprintf(
				SENTENCE, SENTENCE_SIZE, "%s %s %s%s%s", SUBJECT_TEXT,
				NEGATED ? "cannot" : "can",
				VERB_ID >= 0 ? KNOWLEDGE_BASE->SYMBOLS[VERB_ID] : "?",
				OBJECT_ID >= 0 ? " " : "", OBJECT_TEXT
			);
		}
		break ;
		default:
		{
			if (NEGATED)
				snprintf(
					SENTENCE, SENTENCE_SIZE, "%s does not %s%s%s", SUBJECT_TEXT,
					VERB_ID >= 0 ? KNOWLEDGE_BASE->SYMBOLS[VERB_ID] : "?",
					OBJECT_ID >= 0 ? " " : "", OBJECT_TEXT
				);
			else
			{
				VERB_THIRD_PERSON(
					VERB_ID >= 0 ? KNOWLEDGE_BASE->SYMBOLS[VERB_ID] : "?",
					CONJUGATED_VERB
				);
				snprintf(
					SENTENCE, SENTENCE_SIZE, "%s %s%s%s", SUBJECT_TEXT,
					CONJUGATED_VERB, OBJECT_ID >= 0 ? " " : "", OBJECT_TEXT
				);
			}
		}
	}
}

static void
	RENDER_STORED_FACT(
		KNOWLEDGE *KNOWLEDGE_BASE, int FACT_INDEX, int SUBJECT_OVERRIDE,
		char *SENTENCE, int SENTENCE_SIZE
	)
{
	KNOWLEDGE_FACT	*STORED_FACT = &KNOWLEDGE_BASE->FACTS[FACT_INDEX];

	RENDER_FACT(
		KNOWLEDGE_BASE,
		SUBJECT_OVERRIDE >= 0 ? SUBJECT_OVERRIDE : STORED_FACT->SUBJECT,
		STORED_FACT->RELATION, STORED_FACT->VERB, STORED_FACT->OBJECT_NAME,
		STORED_FACT->COUNT, STORED_FACT->IS_NEGATED, SENTENCE, SENTENCE_SIZE
	);
}

static void
	APPEND_FORMATTED(
		char *OUTPUT, int OUTPUT_SIZE, const char *FORMAT,
		const char *ARGUMENT_STRING
	)
{
	size_t	USED_LENGTH = strlen(OUTPUT);

	if ((int)USED_LENGTH < OUTPUT_SIZE - 1)
		snprintf(
			OUTPUT + USED_LENGTH, (size_t)OUTPUT_SIZE - USED_LENGTH, FORMAT,
			ARGUMENT_STRING
		);
}

static void
	COLLECT_ANCESTORS(
		KNOWLEDGE *KNOWLEDGE_BASE, int START_SYMBOL, ANCESTRY *LINEAGE
	)
{
	LINEAGE->COUNT = 1;
	LINEAGE->NODES[0] = START_SYMBOL;
	LINEAGE->VIA_FACTS[0] = -1;

	int	NODE_INDEX;

	for (NODE_INDEX = 0; NODE_INDEX < LINEAGE->COUNT; NODE_INDEX++)
	{
		int	CURRENT_NODE = LINEAGE->NODES[NODE_INDEX];
		int	FACT_INDEX;

		for (
			FACT_INDEX = 0;
			FACT_INDEX < KNOWLEDGE_BASE->FACT_COUNT;
			FACT_INDEX++
		)
		{
			KNOWLEDGE_FACT	*FACT = &KNOWLEDGE_BASE->FACTS[FACT_INDEX];

			if (
				!FACT->IS_ALIVE ||
				FACT->RELATION != RELATION_IS_A ||
				FACT->IS_NEGATED ||
				FACT->SUBJECT != CURRENT_NODE
			)
				continue ;

			int	ALREADY_SEEN = 0;
			int	SEEN_INDEX;

			for (SEEN_INDEX = 0; SEEN_INDEX < LINEAGE->COUNT; SEEN_INDEX++)
				if (LINEAGE->NODES[SEEN_INDEX] == FACT->OBJECT_NAME)
				{
					ALREADY_SEEN = 1;
					break ;
				}

			if (!ALREADY_SEEN && LINEAGE->COUNT < 256)
			{
				LINEAGE->NODES[LINEAGE->COUNT] = FACT->OBJECT_NAME;
				LINEAGE->VIA_FACTS[LINEAGE->COUNT] = FACT_INDEX;
				LINEAGE->COUNT++;
			}
		}
	}
}

static void
	RENDER_CHAIN(
		KNOWLEDGE *KNOWLEDGE_BASE, ANCESTRY *LINEAGE, int NODE_INDEX,
		char *OUTPUT, int OUTPUT_SIZE
	)
{
	int	FACT_PATH[256];
	int	PATH_LENGTH = 0;

	while (NODE_INDEX > 0 && PATH_LENGTH < 256)
	{
		int	FACT_INDEX = LINEAGE->VIA_FACTS[NODE_INDEX];

		FACT_PATH[PATH_LENGTH++] = FACT_INDEX;

		int	PREVIOUS_SYMBOL = KNOWLEDGE_BASE->FACTS[FACT_INDEX].SUBJECT;
		int	PREVIOUS_NODE = 0;
		int	SEARCH_INDEX;

		for (SEARCH_INDEX = 0; SEARCH_INDEX < LINEAGE->COUNT; SEARCH_INDEX++)
			if (LINEAGE->NODES[SEARCH_INDEX] == PREVIOUS_SYMBOL)
			{
				PREVIOUS_NODE = SEARCH_INDEX;
				break ;
			}

		NODE_INDEX = PREVIOUS_NODE;
	}

	char	SENTENCE[160];
	int		PATH_INDEX;

	for (PATH_INDEX = PATH_LENGTH - 1; PATH_INDEX >= 0; PATH_INDEX--)
	{
		RENDER_STORED_FACT(
			KNOWLEDGE_BASE, FACT_PATH[PATH_INDEX], -1, SENTENCE, sizeof SENTENCE
		);
		APPEND_FORMATTED(OUTPUT, OUTPUT_SIZE, "%s; ", SENTENCE);
	}
}

static int
	FACT_MATCHES(
		KNOWLEDGE_FACT *FACT, int RELATION_KIND, int VERB_ID, int OBJECT_ID
	)
{
	if (!FACT->IS_ALIVE || FACT->RELATION != RELATION_KIND)
		return (0);

	if (RELATION_KIND == RELATION_CAN || RELATION_KIND == RELATION_DO)
		return (
			FACT->VERB == VERB_ID &&
			(OBJECT_ID < 0 || FACT->OBJECT_NAME == OBJECT_ID)
		);

	return (FACT->OBJECT_NAME == OBJECT_ID);
}

static int
	CLOSEST_FACT(
		KNOWLEDGE *KNOWLEDGE_BASE, ANCESTRY *LINEAGE, int RELATION_KIND,
		int VERB_ID, int OBJECT_ID, int *FOUND_NODE
	)
{
	int	NODE_INDEX;

	for (NODE_INDEX = 0; NODE_INDEX < LINEAGE->COUNT; NODE_INDEX++)
	{
		int	BEST_FACT = -1;
		int	FACT_INDEX;

		for (
			FACT_INDEX = 0;
			FACT_INDEX < KNOWLEDGE_BASE->FACT_COUNT;
			FACT_INDEX++
		)
		{
			KNOWLEDGE_FACT	*FACT = &KNOWLEDGE_BASE->FACTS[FACT_INDEX];

			if (
				FACT->SUBJECT != LINEAGE->NODES[NODE_INDEX] ||
				!FACT_MATCHES(FACT, RELATION_KIND, VERB_ID, OBJECT_ID)
			)
				continue ;

			if (
				BEST_FACT < 0 ||
				FACT->TURN >= KNOWLEDGE_BASE->FACTS[BEST_FACT].TURN
			)
				BEST_FACT = FACT_INDEX;
		}

		if (BEST_FACT >= 0)
		{
			*FOUND_NODE = NODE_INDEX;
			return (BEST_FACT);
		}
	}

	return (-1);
}

static int
	FACT_HOLDS(
		KNOWLEDGE *KNOWLEDGE_BASE, int THING_ID, int RELATION_KIND, int VERB_ID,
		int OBJECT_ID
	)
{
	ANCESTRY	LINEAGE;

	COLLECT_ANCESTORS(KNOWLEDGE_BASE, THING_ID, &LINEAGE);

	if (RELATION_KIND == RELATION_IS_A)
	{
		int	NODE_INDEX;

		for (NODE_INDEX = 1; NODE_INDEX < LINEAGE.COUNT; NODE_INDEX++)
			if (LINEAGE.NODES[NODE_INDEX] == OBJECT_ID)
				return (1);

		return (0);
	}

	int	NODE_INDEX;
	int	FACT_INDEX = CLOSEST_FACT(
		KNOWLEDGE_BASE, &LINEAGE, RELATION_KIND, VERB_ID, OBJECT_ID, &NODE_INDEX
	);

	return (FACT_INDEX >= 0 && !KNOWLEDGE_BASE->FACTS[FACT_INDEX].IS_NEGATED);
}

static void
	APPEND_BECAUSE(
		KNOWLEDGE *KNOWLEDGE_BASE, ANCESTRY *LINEAGE, int NODE_INDEX,
		int FACT_INDEX, char *OUTPUT, int OUTPUT_SIZE
	)
{
	APPEND_FORMATTED(OUTPUT, OUTPUT_SIZE, "%s", " | because: ");

	if (NODE_INDEX == 0 && FACT_INDEX >= 0)
	{
		char	SENTENCE[160];

		RENDER_STORED_FACT(
			KNOWLEDGE_BASE, FACT_INDEX, -1, SENTENCE, sizeof SENTENCE
		);
		APPEND_FORMATTED(OUTPUT, OUTPUT_SIZE, "%s (told)", SENTENCE);
		return ;
	}

	RENDER_CHAIN(KNOWLEDGE_BASE, LINEAGE, NODE_INDEX, OUTPUT, OUTPUT_SIZE);

	if (FACT_INDEX >= 0)
	{
		char	SENTENCE[160];

		RENDER_STORED_FACT(
			KNOWLEDGE_BASE, FACT_INDEX, -1, SENTENCE, sizeof SENTENCE
		);
		APPEND_FORMATTED(OUTPUT, OUTPUT_SIZE, "%s", SENTENCE);
	}
	else
	{
		size_t	OUTPUT_LENGTH = strlen(OUTPUT);

		if (OUTPUT_LENGTH >= 2 && OUTPUT[OUTPUT_LENGTH - 2] == ';')
			OUTPUT[OUTPUT_LENGTH - 2] = 0;
	}
}

static int
	IS_UNKNOWN_THING(KNOWLEDGE *KNOWLEDGE_BASE, int SYMBOL_ID)
{
	if (
		SYMBOL_ID < 0 ||
		!(KNOWLEDGE_BASE->SYMBOL_KINDS[SYMBOL_ID] & SYMBOL_THING)
	)
		return (0);

	int	FACT_INDEX;

	for (FACT_INDEX = 0; FACT_INDEX < KNOWLEDGE_BASE->FACT_COUNT; FACT_INDEX++)
	{
		KNOWLEDGE_FACT	*FACT = &KNOWLEDGE_BASE->FACTS[FACT_INDEX];

		if (
			FACT->IS_ALIVE &&
			FACT->RELATION == RELATION_IS_A &&
			!FACT->IS_NEGATED &&
			FACT->SUBJECT == SYMBOL_ID
		)
			return (0);
	}

	return (1);
}

static int
	COUNT_MENTIONS(KNOWLEDGE *KNOWLEDGE_BASE, int SYMBOL_ID)
{
	int	MENTION_COUNT = 0;
	int	FACT_INDEX;

	for (FACT_INDEX = 0; FACT_INDEX < KNOWLEDGE_BASE->FACT_COUNT; FACT_INDEX++)
		if (
			KNOWLEDGE_BASE->FACTS[FACT_INDEX].IS_ALIVE &&
			(
				KNOWLEDGE_BASE->FACTS[FACT_INDEX].SUBJECT == SYMBOL_ID ||
				KNOWLEDGE_BASE->FACTS[FACT_INDEX].OBJECT_NAME == SYMBOL_ID
			)
		)
			MENTION_COUNT++;

	return (MENTION_COUNT);
}

static void
	APPEND_KNOWN_ABOUT(
		KNOWLEDGE *KNOWLEDGE_BASE, int THING_ID, ANCESTRY *LINEAGE,
		char *OUTPUT, int OUTPUT_SIZE
	)
{
	int		SHOWN_COUNT = 0;
	char	SENTENCE[160];
	int		FACT_INDEX;

	for (
		FACT_INDEX = 0;
		FACT_INDEX < KNOWLEDGE_BASE->FACT_COUNT && SHOWN_COUNT < 3;
		FACT_INDEX++
	)
	{
		if (
			!KNOWLEDGE_BASE->FACTS[FACT_INDEX].IS_ALIVE ||
			KNOWLEDGE_BASE->FACTS[FACT_INDEX].SUBJECT != THING_ID
		)
			continue ;

		RENDER_STORED_FACT(
			KNOWLEDGE_BASE, FACT_INDEX, -1, SENTENCE, sizeof SENTENCE
		);
		APPEND_FORMATTED(
			OUTPUT, OUTPUT_SIZE, SHOWN_COUNT ? "; %s" : " | known: %s", SENTENCE
		);
		SHOWN_COUNT++;
	}

	if (!SHOWN_COUNT)
		APPEND_FORMATTED(
			OUTPUT, OUTPUT_SIZE, " | known: nothing about %s yet",
			KNOWLEDGE_BASE->SYMBOLS[THING_ID]
		);

	(void)LINEAGE;
}

static void
	APPEND_MISSING(
		KNOWLEDGE *KNOWLEDGE_BASE, ANCESTRY *LINEAGE, int RELATION_KIND,
		int VERB_ID, int OBJECT_ID, int QUANTITY, char *OUTPUT, int OUTPUT_SIZE
	)
{
	char	SENTENCE[160];

	RENDER_FACT(
		KNOWLEDGE_BASE, LINEAGE->NODES[0], RELATION_KIND, VERB_ID, OBJECT_ID,
		QUANTITY, 0, SENTENCE, sizeof SENTENCE
	);
	APPEND_FORMATTED(OUTPUT, OUTPUT_SIZE, " | missing: whether %s", SENTENCE);

	int	NODE_INDEX;

	for (
		NODE_INDEX = 1;
		NODE_INDEX < LINEAGE->COUNT && NODE_INDEX < 4;
		NODE_INDEX++
	)
	{
		RENDER_FACT(
			KNOWLEDGE_BASE, LINEAGE->NODES[NODE_INDEX], RELATION_KIND, VERB_ID,
			OBJECT_ID, QUANTITY, 0, SENTENCE, sizeof SENTENCE
		);
		APPEND_FORMATTED(OUTPUT, OUTPUT_SIZE, " (or whether %s)", SENTENCE);
	}
}

static int
	ANSWER_CHECK(
		KNOWLEDGE *KNOWLEDGE_BASE, int THING_ID, int RELATION_KIND, int VERB_ID,
		int OBJECT_ID, int QUANTITY, char *OUTPUT, int OUTPUT_SIZE
	)
{
	ANCESTRY	LINEAGE;
	char		SENTENCE[160];

	COLLECT_ANCESTORS(KNOWLEDGE_BASE, THING_ID, &LINEAGE);

	if (RELATION_KIND == RELATION_IS_A)
	{
		RENDER_FACT(
			KNOWLEDGE_BASE, THING_ID, RELATION_KIND, VERB_ID, OBJECT_ID,
			QUANTITY, 0, SENTENCE, sizeof SENTENCE
		);

		if (THING_ID == OBJECT_ID)
		{
			snprintf(OUTPUT, OUTPUT_SIZE, "yes: %s", SENTENCE);
			return (HEAR_ANSWER);
		}

		{
			int	NODE_INDEX;

			for (NODE_INDEX = 1; NODE_INDEX < LINEAGE.COUNT; NODE_INDEX++)
				if (LINEAGE.NODES[NODE_INDEX] == OBJECT_ID)
				{
					snprintf(OUTPUT, OUTPUT_SIZE, "yes: %s", SENTENCE);

					if (NODE_INDEX == 1)
						APPEND_BECAUSE(
							KNOWLEDGE_BASE, &LINEAGE, 0, LINEAGE.VIA_FACTS[1],
							OUTPUT, OUTPUT_SIZE
						);
					else
						APPEND_BECAUSE(
							KNOWLEDGE_BASE, &LINEAGE, NODE_INDEX, -1, OUTPUT,
							OUTPUT_SIZE
						);

					return (HEAR_ANSWER);
				}
		}

		int	NODE_INDEX;
		int	FACT_INDEX = CLOSEST_FACT(
			KNOWLEDGE_BASE, &LINEAGE, RELATION_KIND, VERB_ID, OBJECT_ID,
			&NODE_INDEX
		);

		if (FACT_INDEX >= 0 && KNOWLEDGE_BASE->FACTS[FACT_INDEX].IS_NEGATED)
		{
			RENDER_FACT(
				KNOWLEDGE_BASE, THING_ID, RELATION_KIND, VERB_ID, OBJECT_ID,
				QUANTITY, 1, SENTENCE, sizeof SENTENCE
			);
			snprintf(OUTPUT, OUTPUT_SIZE, "no: %s", SENTENCE);
			APPEND_BECAUSE(
				KNOWLEDGE_BASE, &LINEAGE, NODE_INDEX, FACT_INDEX, OUTPUT,
				OUTPUT_SIZE
			);
			return (HEAR_ANSWER);
		}

		snprintf(
			OUTPUT, OUTPUT_SIZE, "don't know: is %s %s %s",
			KNOWLEDGE_BASE->SYMBOLS[THING_ID],
			ARTICLE_FOR(KNOWLEDGE_BASE->SYMBOLS[OBJECT_ID]),
			KNOWLEDGE_BASE->SYMBOLS[OBJECT_ID]
		);
		APPEND_KNOWN_ABOUT(
			KNOWLEDGE_BASE, THING_ID, &LINEAGE, OUTPUT, OUTPUT_SIZE
		);
		APPEND_MISSING(
			KNOWLEDGE_BASE, &LINEAGE, RELATION_KIND, VERB_ID, OBJECT_ID,
			QUANTITY, OUTPUT, OUTPUT_SIZE
		);
		return (HEAR_UNKNOWN);
	}

	int	NODE_INDEX;
	int	FACT_INDEX = CLOSEST_FACT(
		KNOWLEDGE_BASE, &LINEAGE, RELATION_KIND, VERB_ID, OBJECT_ID, &NODE_INDEX
	);

	if (FACT_INDEX < 0)
	{
		RENDER_FACT(
			KNOWLEDGE_BASE, THING_ID, RELATION_KIND, VERB_ID, OBJECT_ID,
			QUANTITY, 0, SENTENCE, sizeof SENTENCE
		);
		snprintf(OUTPUT, OUTPUT_SIZE, "don't know: %s", SENTENCE);
		APPEND_KNOWN_ABOUT(
			KNOWLEDGE_BASE, THING_ID, &LINEAGE, OUTPUT, OUTPUT_SIZE
		);
		APPEND_MISSING(
			KNOWLEDGE_BASE, &LINEAGE, RELATION_KIND, VERB_ID, OBJECT_ID,
			QUANTITY, OUTPUT, OUTPUT_SIZE
		);
		return (HEAR_UNKNOWN);
	}

	KNOWLEDGE_FACT	*FOUND_FACT = &KNOWLEDGE_BASE->FACTS[FACT_INDEX];

	if (
		RELATION_KIND == RELATION_HAS &&
		QUANTITY >= 0 &&
		!FOUND_FACT->IS_NEGATED
	)
	{
		if (FOUND_FACT->COUNT < 0)
		{
			RENDER_STORED_FACT(
				KNOWLEDGE_BASE, FACT_INDEX, THING_ID, SENTENCE, sizeof SENTENCE
			);
			snprintf(
				OUTPUT, OUTPUT_SIZE, "don't know how many: %s, but no number",
				SENTENCE
			);
			APPEND_BECAUSE(
				KNOWLEDGE_BASE, &LINEAGE, NODE_INDEX, FACT_INDEX, OUTPUT,
				OUTPUT_SIZE
			);
			return (HEAR_UNKNOWN);
		}

		RENDER_STORED_FACT(
			KNOWLEDGE_BASE, FACT_INDEX, THING_ID, SENTENCE, sizeof SENTENCE
		);
		snprintf(
			OUTPUT, OUTPUT_SIZE, "%s: %s",
			FOUND_FACT->COUNT == QUANTITY ? "yes" : "no", SENTENCE
		);
		APPEND_BECAUSE(
			KNOWLEDGE_BASE, &LINEAGE, NODE_INDEX, FACT_INDEX, OUTPUT,
			OUTPUT_SIZE
		);
		return (HEAR_ANSWER);
	}

	RENDER_STORED_FACT(
		KNOWLEDGE_BASE, FACT_INDEX, THING_ID, SENTENCE, sizeof SENTENCE
	);
	snprintf(
		OUTPUT, OUTPUT_SIZE, "%s: %s", FOUND_FACT->IS_NEGATED ? "no" : "yes",
		SENTENCE
	);
	APPEND_BECAUSE(
		KNOWLEDGE_BASE, &LINEAGE, NODE_INDEX, FACT_INDEX, OUTPUT, OUTPUT_SIZE
	);

	return (HEAR_ANSWER);
}

static int
	ANSWER_COUNT(
		KNOWLEDGE *KNOWLEDGE_BASE, int THING_ID, int OBJECT_ID, char *OUTPUT,
		int OUTPUT_SIZE
	)
{
	ANCESTRY	LINEAGE;
	char		SENTENCE[160];

	COLLECT_ANCESTORS(KNOWLEDGE_BASE, THING_ID, &LINEAGE);

	int	NODE_INDEX;
	int	FACT_INDEX = CLOSEST_FACT(
		KNOWLEDGE_BASE, &LINEAGE, RELATION_HAS, -1, OBJECT_ID, &NODE_INDEX
	);

	if (FACT_INDEX < 0)
	{
		char	PLURAL_NOUN[40];

		MAKE_PLURAL(KNOWLEDGE_BASE->SYMBOLS[OBJECT_ID], PLURAL_NOUN);
		snprintf(
			OUTPUT, OUTPUT_SIZE, "don't know: how many %s %s has", PLURAL_NOUN,
			KNOWLEDGE_BASE->SYMBOLS[THING_ID]
		);
		APPEND_KNOWN_ABOUT(
			KNOWLEDGE_BASE, THING_ID, &LINEAGE, OUTPUT, OUTPUT_SIZE
		);
		APPEND_MISSING(
			KNOWLEDGE_BASE, &LINEAGE, RELATION_HAS, -1, OBJECT_ID, COUNT_MANY,
			OUTPUT, OUTPUT_SIZE
		);
		return (HEAR_UNKNOWN);
	}

	KNOWLEDGE_FACT	*FOUND_FACT = &KNOWLEDGE_BASE->FACTS[FACT_INDEX];

	RENDER_STORED_FACT(
		KNOWLEDGE_BASE, FACT_INDEX, THING_ID, SENTENCE, sizeof SENTENCE
	);

	if (!FOUND_FACT->IS_NEGATED && FOUND_FACT->COUNT < 0)
	{
		snprintf(
			OUTPUT, OUTPUT_SIZE, "don't know how many: %s, but no number",
			SENTENCE
		);
		APPEND_BECAUSE(
			KNOWLEDGE_BASE, &LINEAGE, NODE_INDEX, FACT_INDEX, OUTPUT,
			OUTPUT_SIZE
		);
		return (HEAR_UNKNOWN);
	}

	char	NUMBER_STRING[16];

	snprintf(
		NUMBER_STRING, sizeof NUMBER_STRING, "%d",
		FOUND_FACT->IS_NEGATED ? 0 : FOUND_FACT->COUNT
	);
	snprintf(OUTPUT, OUTPUT_SIZE, "answer: %s", NUMBER_STRING);
	APPEND_BECAUSE(
		KNOWLEDGE_BASE, &LINEAGE, NODE_INDEX, FACT_INDEX, OUTPUT, OUTPUT_SIZE
	);

	return (HEAR_ANSWER);
}

static int
	ANSWER_DESCRIBE(
		KNOWLEDGE *KNOWLEDGE_BASE, int THING_ID, char *OUTPUT, int OUTPUT_SIZE
	)
{
	ANCESTRY	LINEAGE;
	char		SENTENCE[160];

	COLLECT_ANCESTORS(KNOWLEDGE_BASE, THING_ID, &LINEAGE);
	snprintf(
		OUTPUT, OUTPUT_SIZE, "about %s:", KNOWLEDGE_BASE->SYMBOLS[THING_ID]
	);

	int	SEEN_KEYS[512][3];
	int	KEY_COUNT = 0;
	int	SHOWN_COUNT = 0;
	int	NODE_INDEX;

	for (NODE_INDEX = 0; NODE_INDEX < LINEAGE.COUNT; NODE_INDEX++)
	{
		int	ENTRY_INDEX;

		for (
			ENTRY_INDEX = 0;
			ENTRY_INDEX < KNOWLEDGE_BASE->FACT_COUNT;
			ENTRY_INDEX++
		)
		{
			KNOWLEDGE_FACT	*FACT = &KNOWLEDGE_BASE->FACTS[ENTRY_INDEX];

			if (!FACT->IS_ALIVE || FACT->SUBJECT != LINEAGE.NODES[NODE_INDEX])
				continue ;

			if (NODE_INDEX > 0 && FACT->RELATION == RELATION_IS_A)
				continue ;

			int	IS_DUPLICATE = 0;
			int	KEY_INDEX;

			for (KEY_INDEX = 0; KEY_INDEX < KEY_COUNT; KEY_INDEX++)
				if (
					SEEN_KEYS[KEY_INDEX][0] == FACT->RELATION &&
					SEEN_KEYS[KEY_INDEX][1] == FACT->VERB &&
					SEEN_KEYS[KEY_INDEX][2] == FACT->OBJECT_NAME
				)
				{
					IS_DUPLICATE = 1;
					break ;
				}

			if (IS_DUPLICATE)
				continue ;

			if (KEY_COUNT < 512)
			{
				SEEN_KEYS[KEY_COUNT][0] = FACT->RELATION;
				SEEN_KEYS[KEY_COUNT][1] = FACT->VERB;
				SEEN_KEYS[KEY_COUNT][2] = FACT->OBJECT_NAME;
				KEY_COUNT++;
			}

			if (FACT->IS_NEGATED && NODE_INDEX > 0)
				continue ;

			RENDER_STORED_FACT(
				KNOWLEDGE_BASE, ENTRY_INDEX, THING_ID, SENTENCE, sizeof SENTENCE
			);
			APPEND_FORMATTED(
				OUTPUT, OUTPUT_SIZE, SHOWN_COUNT ? "; %s" : " %s", SENTENCE
			);

			if (NODE_INDEX > 0)
				APPEND_FORMATTED(
					OUTPUT, OUTPUT_SIZE, " (from %s)",
					KNOWLEDGE_BASE->SYMBOLS[LINEAGE.NODES[NODE_INDEX]]
				);

			SHOWN_COUNT++;
		}

		if (NODE_INDEX > 0)
		{
			RENDER_FACT(
				KNOWLEDGE_BASE, THING_ID, RELATION_IS_A, -1,
				LINEAGE.NODES[NODE_INDEX], 0, 0, SENTENCE, sizeof SENTENCE
			);

			int	IS_DUPLICATE = 0;
			int	KEY_INDEX;

			for (KEY_INDEX = 0; KEY_INDEX < KEY_COUNT; KEY_INDEX++)
				if (
					SEEN_KEYS[KEY_INDEX][0] == RELATION_IS_A &&
					SEEN_KEYS[KEY_INDEX][2] == LINEAGE.NODES[NODE_INDEX]
				)
				{
					IS_DUPLICATE = 1;
					break ;
				}

			if (!IS_DUPLICATE)
			{
				APPEND_FORMATTED(
					OUTPUT, OUTPUT_SIZE, SHOWN_COUNT ? "; %s" : " %s", SENTENCE
				);
				SHOWN_COUNT++;

				if (KEY_COUNT < 512)
				{
					SEEN_KEYS[KEY_COUNT][0] = RELATION_IS_A;
					SEEN_KEYS[KEY_COUNT][1] = -1;
					SEEN_KEYS[KEY_COUNT][2] = LINEAGE.NODES[NODE_INDEX];
					KEY_COUNT++;
				}
			}
		}
	}

	int	SUBKIND_COUNT = 0;
	int	ENTRY_INDEX;

	for (
		ENTRY_INDEX = 0;
		ENTRY_INDEX < KNOWLEDGE_BASE->FACT_COUNT;
		ENTRY_INDEX++
	)
	{
		KNOWLEDGE_FACT	*FACT = &KNOWLEDGE_BASE->FACTS[ENTRY_INDEX];

		if (
			!FACT->IS_ALIVE ||
			FACT->RELATION != RELATION_IS_A ||
			FACT->IS_NEGATED ||
			FACT->OBJECT_NAME != THING_ID
		)
			continue ;

		APPEND_FORMATTED(
			OUTPUT, OUTPUT_SIZE, SUBKIND_COUNT ? ", %s" : " | kinds of it: %s",
			KNOWLEDGE_BASE->SYMBOLS[FACT->SUBJECT]
		);
		SUBKIND_COUNT++;
	}

	int	HOLDER_COUNT = 0;

	if (KNOWLEDGE_BASE->SYMBOL_KINDS[THING_ID] & SYMBOL_PROPERTY)
	{
		int	ENTRY_INDEX;

		for (
			ENTRY_INDEX = 0;
			ENTRY_INDEX < KNOWLEDGE_BASE->SYMBOL_COUNT;
			ENTRY_INDEX++
		)
		{
			if (
				ENTRY_INDEX == THING_ID ||
				!(KNOWLEDGE_BASE->SYMBOL_KINDS[ENTRY_INDEX] & SYMBOL_THING) ||
				!FACT_HOLDS(
					KNOWLEDGE_BASE, ENTRY_INDEX, RELATION_IS, -1, THING_ID
				)
			)
				continue ;

			if (!HOLDER_COUNT)
			{
				if (!SHOWN_COUNT && !SUBKIND_COUNT)
					OUTPUT[0] = 0;
				else
					APPEND_FORMATTED(OUTPUT, OUTPUT_SIZE, "%s", " | ");

				APPEND_FORMATTED(
					OUTPUT, OUTPUT_SIZE,
					"things that are %s: ", KNOWLEDGE_BASE->SYMBOLS[THING_ID]
				);
				APPEND_FORMATTED(
					OUTPUT, OUTPUT_SIZE, "%s",
					KNOWLEDGE_BASE->SYMBOLS[ENTRY_INDEX]
				);
			}
			else
				APPEND_FORMATTED(
					OUTPUT, OUTPUT_SIZE, ", %s",
					KNOWLEDGE_BASE->SYMBOLS[ENTRY_INDEX]
				);

			HOLDER_COUNT++;
		}
	}

	if (!SHOWN_COUNT && !SUBKIND_COUNT && !HOLDER_COUNT)
	{
		snprintf(
			OUTPUT, OUTPUT_SIZE,
			"don't know: what %s %s is | known: nothing about %s yet",
			ARTICLE_FOR(KNOWLEDGE_BASE->SYMBOLS[THING_ID]),
			KNOWLEDGE_BASE->SYMBOLS[THING_ID], KNOWLEDGE_BASE->SYMBOLS[THING_ID]
		);
		return (HEAR_UNKNOWN);
	}

	if (IS_UNKNOWN_THING(KNOWLEDGE_BASE, THING_ID))
	{
		APPEND_FORMATTED(
			OUTPUT, OUTPUT_SIZE, " | don't know: what %s is",
			KNOWLEDGE_BASE->SYMBOLS[THING_ID]
		);
		return (HEAR_UNKNOWN);
	}

	return (HEAR_ANSWER);
}

static int
	ANSWER_LIST(
		KNOWLEDGE *KNOWLEDGE_BASE, int THING_ID, int RELATION_KIND, int VERB_ID,
		char *OUTPUT, int OUTPUT_SIZE
	)
{
	ANCESTRY	LINEAGE;
	char		SENTENCE[160];

	COLLECT_ANCESTORS(KNOWLEDGE_BASE, THING_ID, &LINEAGE);

	int	SEEN_KEYS[256][2];
	int	KEY_COUNT = 0;
	int	SHOWN_COUNT = 0;

	snprintf(OUTPUT, OUTPUT_SIZE, "list:");

	int	NODE_INDEX;

	for (NODE_INDEX = 0; NODE_INDEX < LINEAGE.COUNT; NODE_INDEX++)
	{
		int	FACT_INDEX;

		for (
			FACT_INDEX = 0;
			FACT_INDEX < KNOWLEDGE_BASE->FACT_COUNT;
			FACT_INDEX++
		)
		{
			KNOWLEDGE_FACT	*FACT = &KNOWLEDGE_BASE->FACTS[FACT_INDEX];

			if (
				!FACT->IS_ALIVE ||
				FACT->SUBJECT != LINEAGE.NODES[NODE_INDEX] ||
				FACT->RELATION != RELATION_KIND ||
				(RELATION_KIND == RELATION_DO && FACT->VERB != VERB_ID)
			)
				continue ;

			int	IS_DUPLICATE = 0;
			int	KEY_INDEX;

			for (KEY_INDEX = 0; KEY_INDEX < KEY_COUNT; KEY_INDEX++)
				if (
					SEEN_KEYS[KEY_INDEX][0] == FACT->VERB &&
					SEEN_KEYS[KEY_INDEX][1] == FACT->OBJECT_NAME
				)
				{
					IS_DUPLICATE = 1;
					break ;
				}

			if (IS_DUPLICATE)
				continue ;

			if (KEY_COUNT < 256)
			{
				SEEN_KEYS[KEY_COUNT][0] = FACT->VERB;
				SEEN_KEYS[KEY_COUNT][1] = FACT->OBJECT_NAME;
				KEY_COUNT++;
			}

			if (FACT->IS_NEGATED)
				continue ;

			if (RELATION_KIND == RELATION_HAS)
			{
				char	PLURAL_NOUN[40];

				MAKE_PLURAL(
					KNOWLEDGE_BASE->SYMBOLS[FACT->OBJECT_NAME], PLURAL_NOUN
				);

				if (FACT->COUNT == 1)
					snprintf(
						SENTENCE, sizeof SENTENCE, "%s %s",
						ARTICLE_FOR(KNOWLEDGE_BASE->SYMBOLS[FACT->OBJECT_NAME]),
						KNOWLEDGE_BASE->SYMBOLS[FACT->OBJECT_NAME]
					);
				else if (FACT->COUNT >= 0)
					snprintf(
						SENTENCE, sizeof SENTENCE, "%d %s", FACT->COUNT,
						PLURAL_NOUN
					);
				else
					snprintf(SENTENCE, sizeof SENTENCE, "%s", PLURAL_NOUN);
			}
			else if (RELATION_KIND == RELATION_CAN)
				snprintf(
					SENTENCE, sizeof SENTENCE, "%s%s%s",
					KNOWLEDGE_BASE->SYMBOLS[FACT->VERB],
					FACT->OBJECT_NAME >= 0 ? " " : "",
					FACT->OBJECT_NAME >= 0
						? KNOWLEDGE_BASE->SYMBOLS[FACT->OBJECT_NAME]
						: ""
				);
			else
				snprintf(
					SENTENCE, sizeof SENTENCE, "%s",
					FACT->OBJECT_NAME >= 0
						? KNOWLEDGE_BASE->SYMBOLS[FACT->OBJECT_NAME]
						: "(nothing named)"
				);

			APPEND_FORMATTED(
				OUTPUT, OUTPUT_SIZE, SHOWN_COUNT ? ", %s" : " %s", SENTENCE
			);

			if (NODE_INDEX > 0)
				APPEND_FORMATTED(
					OUTPUT, OUTPUT_SIZE, " (from %s)",
					KNOWLEDGE_BASE->SYMBOLS[LINEAGE.NODES[NODE_INDEX]]
				);

			SHOWN_COUNT++;
		}
	}

	if (SHOWN_COUNT)
		return (HEAR_ANSWER);

	if (RELATION_KIND == RELATION_HAS)
		snprintf(
			OUTPUT, OUTPUT_SIZE, "don't know: what %s has",
			KNOWLEDGE_BASE->SYMBOLS[THING_ID]
		);
	else if (RELATION_KIND == RELATION_CAN)
		snprintf(
			OUTPUT, OUTPUT_SIZE, "don't know: what %s can do",
			KNOWLEDGE_BASE->SYMBOLS[THING_ID]
		);
	else
		snprintf(
			OUTPUT, OUTPUT_SIZE, "don't know: what %s does to things with %s",
			KNOWLEDGE_BASE->SYMBOLS[THING_ID], KNOWLEDGE_BASE->SYMBOLS[VERB_ID]
		);

	APPEND_KNOWN_ABOUT(KNOWLEDGE_BASE, THING_ID, &LINEAGE, OUTPUT, OUTPUT_SIZE);

	return (HEAR_UNKNOWN);
}

static int
	ANSWER_REVERSE(
		KNOWLEDGE *KNOWLEDGE_BASE, int RELATION_KIND, int VERB_ID,
		int OBJECT_ID, char *OUTPUT, int OUTPUT_SIZE
	)
{
	int	SHOWN_COUNT = 0;

	snprintf(OUTPUT, OUTPUT_SIZE, "list:");

	int	SYMBOL_INDEX;

	for (
		SYMBOL_INDEX = 0;
		SYMBOL_INDEX < KNOWLEDGE_BASE->SYMBOL_COUNT;
		SYMBOL_INDEX++
	)
	{
		if (
			!(KNOWLEDGE_BASE->SYMBOL_KINDS[SYMBOL_INDEX] & SYMBOL_THING) ||
			!FACT_HOLDS(
				KNOWLEDGE_BASE, SYMBOL_INDEX, RELATION_KIND, VERB_ID, OBJECT_ID
			)
		)
			continue ;

		APPEND_FORMATTED(
			OUTPUT, OUTPUT_SIZE, SHOWN_COUNT ? ", %s" : " %s",
			KNOWLEDGE_BASE->SYMBOLS[SYMBOL_INDEX]
		);
		SHOWN_COUNT++;
	}

	if (SHOWN_COUNT)
		return (HEAR_ANSWER);

	char	SENTENCE[160];

	RENDER_FACT(
		KNOWLEDGE_BASE, -1, RELATION_KIND, VERB_ID, OBJECT_ID, COUNT_MANY, 0,
		SENTENCE, sizeof SENTENCE
	);
	snprintf(
		OUTPUT, OUTPUT_SIZE, "don't know: nothing I know %s", SENTENCE + 2
	);

	return (HEAR_UNKNOWN);
}

static void
	OPEN_QUESTION_ADD(KNOWLEDGE *KNOWLEDGE_BASE, int SYMBOL_ID)
{
	int	QUESTION_INDEX;

	for (
		QUESTION_INDEX = 0;
		QUESTION_INDEX < KNOWLEDGE_BASE->OPEN_QUESTION_COUNT;
		QUESTION_INDEX++
	)
		if (KNOWLEDGE_BASE->OPEN_QUESTIONS[QUESTION_INDEX] == SYMBOL_ID)
			return ;

	if (KNOWLEDGE_BASE->OPEN_QUESTION_COUNT == 64)
	{
		memmove(
			KNOWLEDGE_BASE->OPEN_QUESTIONS, KNOWLEDGE_BASE->OPEN_QUESTIONS + 1,
			63 * sizeof KNOWLEDGE_BASE->OPEN_QUESTIONS[0]
		);
		KNOWLEDGE_BASE->OPEN_QUESTION_COUNT--;
	}

	KNOWLEDGE_BASE->OPEN_QUESTIONS[KNOWLEDGE_BASE->OPEN_QUESTION_COUNT++] =
		SYMBOL_ID;
	KNOWLEDGE_BASE->IS_DIRTY = 1;
}

static int
	OPEN_QUESTION_CLOSE(KNOWLEDGE *KNOWLEDGE_BASE, int SYMBOL_ID)
{
	int	QUESTION_INDEX;

	for (
		QUESTION_INDEX = 0;
		QUESTION_INDEX < KNOWLEDGE_BASE->OPEN_QUESTION_COUNT;
		QUESTION_INDEX++
	)
		if (KNOWLEDGE_BASE->OPEN_QUESTIONS[QUESTION_INDEX] == SYMBOL_ID)
		{
			memmove(
				KNOWLEDGE_BASE->OPEN_QUESTIONS + QUESTION_INDEX,
				KNOWLEDGE_BASE->OPEN_QUESTIONS + QUESTION_INDEX + 1,
				(size_t)(KNOWLEDGE_BASE->OPEN_QUESTION_COUNT - QUESTION_INDEX -
					1) *
					sizeof KNOWLEDGE_BASE->OPEN_QUESTIONS[0]
			);
			KNOWLEDGE_BASE->OPEN_QUESTION_COUNT--;
			KNOWLEDGE_BASE->IS_DIRTY = 1;
			return (1);
		}

	return (0);
}

static void
	INSERT_FACT(
		KNOWLEDGE *KNOWLEDGE_BASE, int CORRECTING, int RELATION_KIND,
		int SUBJECT_ID, int VERB_ID, int OBJECT_ID, int QUANTITY, int NEGATED,
		char *OUTPUT, int OUTPUT_SIZE, int *RESULT_KIND
	)
{
	char	SENTENCE[160];
	char	OLD_SENTENCE[160];

	RENDER_FACT(
		KNOWLEDGE_BASE, SUBJECT_ID, RELATION_KIND, VERB_ID, OBJECT_ID, QUANTITY,
		NEGATED, SENTENCE, sizeof SENTENCE
	);

	int	FACT_INDEX;

	for (FACT_INDEX = 0; FACT_INDEX < KNOWLEDGE_BASE->FACT_COUNT; FACT_INDEX++)
	{
		KNOWLEDGE_FACT	*FACT = &KNOWLEDGE_BASE->FACTS[FACT_INDEX];

		if (
			!FACT->IS_ALIVE ||
			FACT->SUBJECT != SUBJECT_ID ||
			!FACT_MATCHES(FACT, RELATION_KIND, VERB_ID, OBJECT_ID) ||
			(RELATION_KIND >= RELATION_CAN && FACT->OBJECT_NAME != OBJECT_ID)
		)
			continue ;

		int	IS_SAME = FACT->IS_NEGATED == NEGATED &&
			(RELATION_KIND != RELATION_HAS || NEGATED ||
				FACT->COUNT == QUANTITY || QUANTITY == COUNT_MANY);

		if (IS_SAME)
		{
			if (
				RELATION_KIND == RELATION_HAS &&
				FACT->COUNT == COUNT_MANY &&
				QUANTITY != COUNT_MANY
			)
			{
				FACT->COUNT = QUANTITY;
				FACT->TURN = KNOWLEDGE_BASE->TURN;
				KNOWLEDGE_BASE->IS_DIRTY = 1;
				APPEND_FORMATTED(
					OUTPUT, OUTPUT_SIZE, "updated: %s\n", SENTENCE
				);

				if (*RESULT_KIND < HEAR_SAVED)
					*RESULT_KIND = HEAR_SAVED;

				return ;
			}

			FACT->TURN = KNOWLEDGE_BASE->TURN;
			APPEND_FORMATTED(
				OUTPUT, OUTPUT_SIZE, "already known: %s\n", SENTENCE
			);

			if (*RESULT_KIND < HEAR_KNEW)
				*RESULT_KIND = HEAR_KNEW;

			return ;
		}

		RENDER_STORED_FACT(
			KNOWLEDGE_BASE, FACT_INDEX, -1, OLD_SENTENCE, sizeof OLD_SENTENCE
		);

		if (CORRECTING)
		{
			FACT->IS_ALIVE = 0;
			FACT_ADD(
				KNOWLEDGE_BASE, FACT_SOURCE_TOLD, RELATION_KIND, SUBJECT_ID,
				VERB_ID, OBJECT_ID, QUANTITY, NEGATED
			);

			size_t	OUTPUT_LENGTH = strlen(OUTPUT);

			snprintf(
				OUTPUT + OUTPUT_LENGTH, (size_t)OUTPUT_SIZE - OUTPUT_LENGTH,
				"replaced: %s -> %s\n", OLD_SENTENCE, SENTENCE
			);

			if (*RESULT_KIND < HEAR_SAVED)
				*RESULT_KIND = HEAR_SAVED;

			return ;
		}

		size_t	OUTPUT_LENGTH = strlen(OUTPUT);

		snprintf(
			OUTPUT + OUTPUT_LENGTH, (size_t)OUTPUT_SIZE - OUTPUT_LENGTH,
			"conflict: new \"%s\" vs known \"%s\" | kept the known one (say "
			"\"actually ...\" to correct it)\n",
			SENTENCE, OLD_SENTENCE
		);
		*RESULT_KIND = HEAR_CONFLICT;
		return ;
	}

	ANCESTRY	LINEAGE;

	COLLECT_ANCESTORS(KNOWLEDGE_BASE, SUBJECT_ID, &LINEAGE);

	int	NODE_INDEX = 0;
	int	CLOSEST_INDEX;

	if (RELATION_KIND == RELATION_IS_A)
		CLOSEST_INDEX = -1;
	else
		CLOSEST_INDEX = CLOSEST_FACT(
			KNOWLEDGE_BASE, &LINEAGE, RELATION_KIND, VERB_ID, OBJECT_ID,
			&NODE_INDEX
		);

	if (RELATION_KIND == RELATION_IS_A && !NEGATED)
	{
		int	ANCESTOR_INDEX;

		for (
			ANCESTOR_INDEX = 1;
			ANCESTOR_INDEX < LINEAGE.COUNT;
			ANCESTOR_INDEX++
		)
			if (LINEAGE.NODES[ANCESTOR_INDEX] == OBJECT_ID)
			{
				CLOSEST_INDEX = -2;
				NODE_INDEX = ANCESTOR_INDEX;
				break ;
			}
	}

	if (
		CLOSEST_INDEX == -2 ||
		(
			CLOSEST_INDEX >= 0 &&
			NODE_INDEX > 0 &&
			KNOWLEDGE_BASE->FACTS[CLOSEST_INDEX].IS_NEGATED == NEGATED &&
			(
				RELATION_KIND != RELATION_HAS ||
				NEGATED ||
				QUANTITY == COUNT_MANY ||
				KNOWLEDGE_BASE->FACTS[CLOSEST_INDEX].COUNT == QUANTITY
			)
		)
	)
	{
		size_t	OUTPUT_LENGTH = strlen(OUTPUT);

		snprintf(
			OUTPUT + OUTPUT_LENGTH, (size_t)OUTPUT_SIZE - OUTPUT_LENGTH,
			"already known: %s | follows from: ", SENTENCE
		);
		RENDER_CHAIN(KNOWLEDGE_BASE, &LINEAGE, NODE_INDEX, OUTPUT, OUTPUT_SIZE);

		if (CLOSEST_INDEX >= 0)
		{
			RENDER_STORED_FACT(
				KNOWLEDGE_BASE, CLOSEST_INDEX, -1, OLD_SENTENCE,
				sizeof OLD_SENTENCE
			);
			APPEND_FORMATTED(OUTPUT, OUTPUT_SIZE, "%s", OLD_SENTENCE);
		}
		else
		{
			OUTPUT_LENGTH = strlen(OUTPUT);

			if (OUTPUT_LENGTH >= 2 && OUTPUT[OUTPUT_LENGTH - 2] == ';')
				OUTPUT[OUTPUT_LENGTH - 2] = 0;
		}

		APPEND_FORMATTED(OUTPUT, OUTPUT_SIZE, "%s", "\n");

		if (*RESULT_KIND < HEAR_KNEW)
			*RESULT_KIND = HEAR_KNEW;

		return ;
	}

	FACT_ADD(
		KNOWLEDGE_BASE, FACT_SOURCE_TOLD, RELATION_KIND, SUBJECT_ID, VERB_ID,
		OBJECT_ID, QUANTITY, NEGATED
	);
	APPEND_FORMATTED(OUTPUT, OUTPUT_SIZE, "saved: %s\n", SENTENCE);

	if (*RESULT_KIND < HEAR_SAVED)
		*RESULT_KIND = HEAR_SAVED;

	if (CLOSEST_INDEX >= 0 && NODE_INDEX > 0)
	{
		RENDER_STORED_FACT(
			KNOWLEDGE_BASE, CLOSEST_INDEX, -1, OLD_SENTENCE, sizeof OLD_SENTENCE
		);

		size_t	OUTPUT_LENGTH = strlen(OUTPUT);

		snprintf(
			OUTPUT + OUTPUT_LENGTH, (size_t)OUTPUT_SIZE - OUTPUT_LENGTH,
			"exception: %s, although %s\n", SENTENCE, OLD_SENTENCE
		);
	}

	if (RELATION_KIND == RELATION_IS_A && !NEGATED)
	{
		ANCESTRY	OBJECT_LINEAGE;

		COLLECT_ANCESTORS(KNOWLEDGE_BASE, OBJECT_ID, &OBJECT_LINEAGE);

		int	ANCESTOR_INDEX;

		for (
			ANCESTOR_INDEX = 1;
			ANCESTOR_INDEX < OBJECT_LINEAGE.COUNT;
			ANCESTOR_INDEX++
		)
			if (OBJECT_LINEAGE.NODES[ANCESTOR_INDEX] == SUBJECT_ID)
			{
				APPEND_FORMATTED(
					OUTPUT, OUTPUT_SIZE,
					"note: %s and its kinds now form a loop\n",
					KNOWLEDGE_BASE->SYMBOLS[SUBJECT_ID]
				);
				break ;
			}

		if (OPEN_QUESTION_CLOSE(KNOWLEDGE_BASE, SUBJECT_ID))
			APPEND_FORMATTED(
				OUTPUT, OUTPUT_SIZE,
				"learned: what %s is (open question closed)\n",
				KNOWLEDGE_BASE->SYMBOLS[SUBJECT_ID]
			);
	}
}

static void
	ASK_OPEN_QUESTIONS(
		KNOWLEDGE *KNOWLEDGE_BASE, int *CANDIDATES, int CANDIDATE_COUNT,
		int SUBJECT_ID, char *OUTPUT, int OUTPUT_SIZE
	)
{
	int	ITEM_INDEX;

	for (ITEM_INDEX = 0; ITEM_INDEX < CANDIDATE_COUNT; ITEM_INDEX++)
		if (IS_UNKNOWN_THING(KNOWLEDGE_BASE, CANDIDATES[ITEM_INDEX]))
			OPEN_QUESTION_ADD(KNOWLEDGE_BASE, CANDIDATES[ITEM_INDEX]);

	int	PICKED[2];
	int	PICKED_COUNT = 0;
	int	PICK_ROUND;

	for (PICK_ROUND = 0; PICK_ROUND < 2; PICK_ROUND++)
	{
		int	BEST_SYMBOL = -1;
		int	BEST_SCORE = -1000000;
		int	ITEM_INDEX;

		for (
			ITEM_INDEX = 0;
			ITEM_INDEX < KNOWLEDGE_BASE->OPEN_QUESTION_COUNT;
			ITEM_INDEX++
		)
		{
			int	SYMBOL_ID = KNOWLEDGE_BASE->OPEN_QUESTIONS[ITEM_INDEX];

			if (
				KNOWLEDGE_BASE->ASKED_COUNTS[SYMBOL_ID] ||
				!IS_UNKNOWN_THING(KNOWLEDGE_BASE, SYMBOL_ID) ||
				(PICKED_COUNT && PICKED[0] == SYMBOL_ID)
			)
				continue ;

			int	IN_MESSAGE = 0;
			int	CANDIDATE_INDEX;

			for (
				CANDIDATE_INDEX = 0;
				CANDIDATE_INDEX < CANDIDATE_COUNT;
				CANDIDATE_INDEX++
			)
				if (CANDIDATES[CANDIDATE_INDEX] == SYMBOL_ID)
					IN_MESSAGE = 1;

			if (!IN_MESSAGE)
				continue ;

			int	SCORE = COUNT_MENTIONS(KNOWLEDGE_BASE, SYMBOL_ID) * 2 +
				(SYMBOL_ID == SUBJECT_ID ? 3 : 0);

			if (SCORE > BEST_SCORE)
			{
				BEST_SCORE = SCORE;
				BEST_SYMBOL = SYMBOL_ID;
			}
		}

		if (BEST_SYMBOL < 0)
			break ;

		PICKED[PICKED_COUNT++] = BEST_SYMBOL;
	}

	if (!PICKED_COUNT)
		return ;

	APPEND_FORMATTED(OUTPUT, OUTPUT_SIZE, "%s", "want to know: ");

	for (ITEM_INDEX = 0; ITEM_INDEX < PICKED_COUNT; ITEM_INDEX++)
	{
		KNOWLEDGE_BASE->ASKED_COUNTS[PICKED[ITEM_INDEX]]++;
		APPEND_FORMATTED(
			OUTPUT, OUTPUT_SIZE, ITEM_INDEX ? ", what %s is" : "what %s is",
			KNOWLEDGE_BASE->SYMBOLS[PICKED[ITEM_INDEX]]
		);
	}

	APPEND_FORMATTED(OUTPUT, OUTPUT_SIZE, "%s", "\n");
	KNOWLEDGE_BASE->IS_DIRTY = 1;
}

int
	KNOWLEDGE_HEAR(
		KNOWLEDGE *KNOWLEDGE_BASE, const char *HEARD_TEXT, char *OUTPUT,
		int OUTPUT_SIZE
	)
{
	TOKEN_LIST		TOKENS;
	PARSED_SENTENCE	PARSED;

	OUTPUT[0] = 0;
	KNOWLEDGE_BASE->ABOUT_SYMBOL = -1;
	TOKENIZE(HEARD_TEXT, &TOKENS);
	memset(&PARSED, 0, sizeof PARSED);
	PARSED.COUNT = COUNT_MANY;

	if (
		TOKENS.COUNT &&
		IN_WORD_LIST(
			TOKENS.WORDS[0],
			"what whats what's who where when why how which wats wat whos"
		)
	)
		TOKENS.IS_QUESTION = 1;

	int	ITEM_INDEX;

	for (ITEM_INDEX = 0; ITEM_INDEX < TOKENS.COUNT; ITEM_INDEX++)
		if (
			IN_WORD_LIST(
				TOKENS.WORDS[ITEM_INDEX],
				"thanks thank sorry hi hello bye lol haha please pls ty thx "
				"oops yeah yep nah ok okay"
			)
		)
			return (HEAR_NONE);

	if (TOKENS.COUNT && IN_WORD_LIST(TOKENS.WORDS[0], "let lets let's shall"))
		return (HEAR_NONE);

	int	PARSE_SUCCESS;

	if (TOKENS.IS_QUESTION)
		PARSE_SUCCESS = PARSE_QUESTION(&TOKENS, &PARSED);
	else
		PARSE_SUCCESS = PARSE_STATEMENT(&TOKENS, &PARSED);

	if (!PARSE_SUCCESS)
		return (HEAR_NONE);

	KNOWLEDGE_BASE->TURN++;

	int	RESULT_KIND = HEAR_NONE;

	if (PARSED.KIND == SENTENCE_STATEMENT)
	{
		int	SUBJECT_ID =
			SYMBOL_GET(KNOWLEDGE_BASE, PARSED.SUBJECT, SYMBOL_THING);
		int	VERB_ID = -1;
		int	OBJECT_ID = -1;
		int	CANDIDATES[16];
		int	CANDIDATE_COUNT = 0;

		CANDIDATES[CANDIDATE_COUNT++] = SUBJECT_ID;

		if (PARSED.VERB[0])
			VERB_ID = SYMBOL_GET(KNOWLEDGE_BASE, PARSED.VERB, SYMBOL_VERB);

		if (PARSED.RELATION == RELATION_IS)
		{
			int	ITEM_INDEX;

			for (
				ITEM_INDEX = 0;
				ITEM_INDEX < PARSED.PROPERTY_COUNT;
				ITEM_INDEX++
			)
			{
				OBJECT_ID = SYMBOL_GET(
					KNOWLEDGE_BASE, PARSED.PROPERTIES[ITEM_INDEX],
					SYMBOL_PROPERTY
				);
				INSERT_FACT(
					KNOWLEDGE_BASE, TOKENS.IS_CORRECTION, RELATION_IS,
					SUBJECT_ID, -1, OBJECT_ID, 0, PARSED.IS_NEGATED, OUTPUT,
					OUTPUT_SIZE, &RESULT_KIND
				);
			}
		}
		else
		{
			if (PARSED.OBJECT_NAME[0])
				OBJECT_ID = SYMBOL_GET(
					KNOWLEDGE_BASE, PARSED.OBJECT_NAME, SYMBOL_THING
				);

			INSERT_FACT(
				KNOWLEDGE_BASE, TOKENS.IS_CORRECTION, PARSED.RELATION,
				SUBJECT_ID, VERB_ID, OBJECT_ID,
				PARSED.RELATION == RELATION_HAS ? PARSED.COUNT : 0,
				PARSED.IS_NEGATED, OUTPUT, OUTPUT_SIZE, &RESULT_KIND
			);

			if (OBJECT_ID >= 0 && CANDIDATE_COUNT < 16)
				CANDIDATES[CANDIDATE_COUNT++] = OBJECT_ID;

			if (PARSED.RELATION == RELATION_IS_A)
			{
				int	ITEM_INDEX;

				for (
					ITEM_INDEX = 0;
					ITEM_INDEX < PARSED.PROPERTY_COUNT;
					ITEM_INDEX++
				)
					INSERT_FACT(
						KNOWLEDGE_BASE, TOKENS.IS_CORRECTION, RELATION_IS,
						SUBJECT_ID, -1,
						SYMBOL_GET(
							KNOWLEDGE_BASE, PARSED.PROPERTIES[ITEM_INDEX],
							SYMBOL_PROPERTY
						),
						0, 0, OUTPUT, OUTPUT_SIZE, &RESULT_KIND
					);
			}
		}

		KNOWLEDGE_BASE->ABOUT_SYMBOL = SUBJECT_ID;

		if (RESULT_KIND != HEAR_CONFLICT)
			ASK_OPEN_QUESTIONS(
				KNOWLEDGE_BASE, CANDIDATES, CANDIDATE_COUNT, SUBJECT_ID, OUTPUT,
				OUTPUT_SIZE
			);
	}
	else
	{
		int	THING_ID = SYMBOL_FIND(KNOWLEDGE_BASE, PARSED.SUBJECT);
		int	VERB_ID = SYMBOL_FIND(KNOWLEDGE_BASE, PARSED.VERB);
		int	OBJECT_ID = SYMBOL_FIND(KNOWLEDGE_BASE, PARSED.OBJECT_NAME);

		if (PARSED.KIND == SENTENCE_REVERSE)
		{
			if (OBJECT_ID >= 0)
				KNOWLEDGE_BASE->ABOUT_SYMBOL = OBJECT_ID;
			else
				KNOWLEDGE_BASE->ABOUT_SYMBOL = VERB_ID;

			if (
				(PARSED.OBJECT_NAME[0] && OBJECT_ID < 0) ||
				(PARSED.VERB[0] && VERB_ID < 0)
			)
			{
				snprintf(
					OUTPUT, OUTPUT_SIZE, "don't know: never heard of %s",
					PARSED.OBJECT_NAME[0] && OBJECT_ID < 0 ? PARSED.OBJECT_NAME
					: PARSED.VERB
				);
				RESULT_KIND = HEAR_UNKNOWN;
			}
			else
				RESULT_KIND = ANSWER_REVERSE(
					KNOWLEDGE_BASE, PARSED.RELATION, VERB_ID, OBJECT_ID, OUTPUT,
					OUTPUT_SIZE
				);
		}
		else if (THING_ID < 0)
		{
			snprintf(
				OUTPUT, OUTPUT_SIZE, "don't know: never heard of %s",
				PARSED.SUBJECT
			);
			RESULT_KIND = HEAR_UNKNOWN;
		}
		else
		{
			KNOWLEDGE_BASE->ABOUT_SYMBOL = THING_ID;

			if (PARSED.KIND == SENTENCE_DESCRIBE)
				RESULT_KIND = ANSWER_DESCRIBE(
					KNOWLEDGE_BASE, THING_ID, OUTPUT, OUTPUT_SIZE
				);
			else if (PARSED.KIND == SENTENCE_COUNT)
			{
				if (OBJECT_ID < 0)
				{
					snprintf(
						OUTPUT, OUTPUT_SIZE, "don't know: never heard of %s",
						PARSED.OBJECT_NAME
					);
					RESULT_KIND = HEAR_UNKNOWN;
				}
				else
					RESULT_KIND = ANSWER_COUNT(
						KNOWLEDGE_BASE, THING_ID, OBJECT_ID, OUTPUT, OUTPUT_SIZE
					);
			}
			else if (PARSED.KIND == SENTENCE_LIST)
			{
				if (PARSED.RELATION == RELATION_DO && VERB_ID < 0)
				{
					snprintf(
						OUTPUT, OUTPUT_SIZE, "don't know: never heard of %s",
						PARSED.VERB
					);
					RESULT_KIND = HEAR_UNKNOWN;
				}
				else
					RESULT_KIND = ANSWER_LIST(
						KNOWLEDGE_BASE, THING_ID, PARSED.RELATION, VERB_ID,
						OUTPUT, OUTPUT_SIZE
					);
			}
			else
			{
				if (PARSED.VERB[0] && VERB_ID < 0)
					VERB_ID = SYMBOL_GET(KNOWLEDGE_BASE, PARSED.VERB, 0);

				if (PARSED.OBJECT_NAME[0] && OBJECT_ID < 0)
					OBJECT_ID =
						SYMBOL_GET(KNOWLEDGE_BASE, PARSED.OBJECT_NAME, 0);

				RESULT_KIND = ANSWER_CHECK(
					KNOWLEDGE_BASE, THING_ID, PARSED.RELATION, VERB_ID,
					OBJECT_ID,
					PARSED.RELATION == RELATION_HAS ? PARSED.COUNT : 0, OUTPUT,
					OUTPUT_SIZE
				);
			}
		}
	}

	size_t	OUTPUT_LENGTH = strlen(OUTPUT);

	while (OUTPUT_LENGTH && OUTPUT[OUTPUT_LENGTH - 1] == '\n')
		OUTPUT[--OUTPUT_LENGTH] = 0;

	snprintf(
		KNOWLEDGE_BASE->LAST_REPLY, sizeof KNOWLEDGE_BASE->LAST_REPLY, "%s",
		OUTPUT
	);

	return (RESULT_KIND);
}

static const char	*RELATION_NAMES[5] = { "isa", "is", "has", "can", "do" };

int
	KNOWLEDGE_SAVE(KNOWLEDGE *KNOWLEDGE_BASE, const char *PATH)
{
	FILE	*STREAM = fopen(PATH, "wb");

	if (!STREAM)
		return (-1);

	fprintf(STREAM, "t\t%d\n", KNOWLEDGE_BASE->TURN);

	int	ITEM_INDEX;

	for (ITEM_INDEX = 0; ITEM_INDEX < KNOWLEDGE_BASE->FACT_COUNT; ITEM_INDEX++)
	{
		KNOWLEDGE_FACT	*FACT = &KNOWLEDGE_BASE->FACTS[ITEM_INDEX];

		if (!FACT->IS_ALIVE)
			continue ;

		fprintf(
			STREAM, "f\t%d\t%s\t%s\t%s\t%s\t%d\t%d\t%d\n", FACT->SOURCE,
			RELATION_NAMES[FACT->RELATION],
			KNOWLEDGE_BASE->SYMBOLS[FACT->SUBJECT],
			FACT->VERB >= 0 ? KNOWLEDGE_BASE->SYMBOLS[FACT->VERB] : "-",
			FACT->OBJECT_NAME >= 0 ? KNOWLEDGE_BASE->SYMBOLS[FACT->OBJECT_NAME]
			: "-",
			FACT->COUNT, FACT->IS_NEGATED, FACT->TURN
		);
	}

	for (
		ITEM_INDEX = 0;
		ITEM_INDEX < KNOWLEDGE_BASE->SYMBOL_COUNT;
		ITEM_INDEX++
	)
		if (KNOWLEDGE_BASE->ASKED_COUNTS[ITEM_INDEX])
			fprintf(
				STREAM, "a\t%s\t%d\n", KNOWLEDGE_BASE->SYMBOLS[ITEM_INDEX],
				KNOWLEDGE_BASE->ASKED_COUNTS[ITEM_INDEX]
			);

	for (
		ITEM_INDEX = 0;
		ITEM_INDEX < KNOWLEDGE_BASE->OPEN_QUESTION_COUNT;
		ITEM_INDEX++
	)
		fprintf(
			STREAM, "w\t%s\n",
			KNOWLEDGE_BASE->SYMBOLS[KNOWLEDGE_BASE->OPEN_QUESTIONS[ITEM_INDEX]]
		);

	fclose(STREAM);
	KNOWLEDGE_BASE->IS_DIRTY = 0;

	return (0);
}

int
	KNOWLEDGE_LOAD(KNOWLEDGE *KNOWLEDGE_BASE, const char *PATH)
{
	FILE	*STREAM = fopen(PATH, "rb");

	if (!STREAM)
		return (-1);

	char	LINE_BUFFER[512];

	while (fgets(LINE_BUFFER, sizeof LINE_BUFFER, STREAM))
	{
		size_t	LINE_LENGTH = strlen(LINE_BUFFER);

		while (
			LINE_LENGTH &&
			(
				LINE_BUFFER[LINE_LENGTH - 1] == '\n' ||
				LINE_BUFFER[LINE_LENGTH - 1] == '\r'
			)
		)
			LINE_BUFFER[--LINE_LENGTH] = 0;

		char	*FIELDS[10];
		int		FIELD_COUNT = 0;
		char	*CURSOR = LINE_BUFFER;

		while (FIELD_COUNT < 10)
		{
			FIELDS[FIELD_COUNT++] = CURSOR;

			char	*TAB_POSITION = strchr(CURSOR, '\t');

			if (!TAB_POSITION)
				break ;

			*TAB_POSITION = 0;
			CURSOR = TAB_POSITION + 1;
		}

		if (!strcmp(FIELDS[0], "t") && FIELD_COUNT >= 2)
			KNOWLEDGE_BASE->TURN = atoi(FIELDS[1]);
		else if (!strcmp(FIELDS[0], "f") && FIELD_COUNT >= 9)
		{
			int	RELATION_KIND = -1;
			int	RELATION_INDEX;

			for (RELATION_INDEX = 0; RELATION_INDEX < 5; RELATION_INDEX++)
				if (!strcmp(FIELDS[2], RELATION_NAMES[RELATION_INDEX]))
					RELATION_KIND = RELATION_INDEX;

			if (RELATION_KIND < 0)
				continue ;

			int	SUBJECT_ID =
				SYMBOL_GET(KNOWLEDGE_BASE, FIELDS[3], SYMBOL_THING);
			int	VERB_ID;

			if (strcmp(FIELDS[4], "-"))
				VERB_ID = SYMBOL_GET(KNOWLEDGE_BASE, FIELDS[4], SYMBOL_VERB);
			else
				VERB_ID = -1;

			int	OBJECT_ID;

			if (strcmp(FIELDS[5], "-"))
				OBJECT_ID = SYMBOL_GET(
					KNOWLEDGE_BASE, FIELDS[5],
					RELATION_KIND == RELATION_IS ? SYMBOL_PROPERTY
					: SYMBOL_THING
				);
			else
				OBJECT_ID = -1;

			int	FACT_INDEX = FACT_ADD(
				KNOWLEDGE_BASE, atoi(FIELDS[1]), RELATION_KIND, SUBJECT_ID,
				VERB_ID, OBJECT_ID, atoi(FIELDS[6]), atoi(FIELDS[7])
			);

			KNOWLEDGE_BASE->FACTS[FACT_INDEX].TURN = atoi(FIELDS[8]);
		}
		else if (!strcmp(FIELDS[0], "a") && FIELD_COUNT >= 3)
		{
			int	SUBJECT_ID = SYMBOL_GET(KNOWLEDGE_BASE, FIELDS[1], 0);

			KNOWLEDGE_BASE->ASKED_COUNTS[SUBJECT_ID] = atoi(FIELDS[2]);
		}
		else if (!strcmp(FIELDS[0], "w") && FIELD_COUNT >= 2)
			OPEN_QUESTION_ADD(
				KNOWLEDGE_BASE,
				SYMBOL_GET(KNOWLEDGE_BASE, FIELDS[1], SYMBOL_THING)
			);
	}

	fclose(STREAM);
	KNOWLEDGE_BASE->IS_DIRTY = 0;

	return (0);
}

void
	KNOWLEDGE_LIST(KNOWLEDGE *KNOWLEDGE_BASE, char *OUTPUT, int OUTPUT_SIZE)
{
	char	FACT_TEXT[160];

	OUTPUT[0] = 0;

	int	LISTED_COUNT = 0;
	int	ITEM_INDEX;

	for (ITEM_INDEX = 0; ITEM_INDEX < KNOWLEDGE_BASE->FACT_COUNT; ITEM_INDEX++)
	{
		if (!KNOWLEDGE_BASE->FACTS[ITEM_INDEX].IS_ALIVE)
			continue ;

		RENDER_STORED_FACT(
			KNOWLEDGE_BASE, ITEM_INDEX, -1, FACT_TEXT, sizeof FACT_TEXT
		);

		size_t	USED_LENGTH = strlen(OUTPUT);

		snprintf(
			OUTPUT + USED_LENGTH, (size_t)OUTPUT_SIZE - USED_LENGTH,
			"  %s (told, turn %d)\n", FACT_TEXT,
			KNOWLEDGE_BASE->FACTS[ITEM_INDEX].TURN
		);
		LISTED_COUNT++;
	}

	if (!LISTED_COUNT)
		APPEND_FORMATTED(OUTPUT, OUTPUT_SIZE, "%s", "  no facts yet\n");

	if (KNOWLEDGE_BASE->OPEN_QUESTION_COUNT)
	{
		APPEND_FORMATTED(OUTPUT, OUTPUT_SIZE, "%s", "  open questions:");

		int	ITEM_INDEX;

		for (
			ITEM_INDEX = 0;
			ITEM_INDEX < KNOWLEDGE_BASE->OPEN_QUESTION_COUNT;
			ITEM_INDEX++
		)
			APPEND_FORMATTED(
				OUTPUT, OUTPUT_SIZE,
				ITEM_INDEX ? ", what %s is" : " what %s is",
				KNOWLEDGE_BASE
					->SYMBOLS[KNOWLEDGE_BASE->OPEN_QUESTIONS[ITEM_INDEX]]
			);

		APPEND_FORMATTED(OUTPUT, OUTPUT_SIZE, "%s", "\n");
	}
}

static void
	COPY_LOWERCASE(char *DESTINATION, const char *SOURCE, int DESTINATION_SIZE)
{
	int	CHARACTER_POSITION = 0;

	for (
		;
		SOURCE[CHARACTER_POSITION] && CHARACTER_POSITION < DESTINATION_SIZE - 1;
		CHARACTER_POSITION++
	)
		DESTINATION[CHARACTER_POSITION] =
			(char)tolower((unsigned char)SOURCE[CHARACTER_POSITION]);

	DESTINATION[CHARACTER_POSITION] = 0;
}

int
	KNOWLEDGE_SCRIPT(
		const char *SCRIPT_TEXT, int SHOW_OUTPUT, int *CHECK_COUNT_OUT
	)
{
	KNOWLEDGE	*KNOWLEDGE_BASE = KNOWLEDGE_NEW();
	char		REPLY[8192];
	char		LOWER_REPLY[8192];
	char		USER_INPUT[1024] = "";
	char		LINE_BUFFER[2048];
	int			FAIL_COUNT = 0;
	int			CHECK_COUNT = 0;
	int			HAS_REPLY = 0;

	REPLY[0] = 0;

	const char	*CURSOR = SCRIPT_TEXT;

	while (*CURSOR)
	{
		const char	*LINE_END = strchr(CURSOR, '\n');
		int			LINE_LENGTH;

		if (LINE_END)
			LINE_LENGTH = (int)(LINE_END - CURSOR);
		else
			LINE_LENGTH = (int)strlen(CURSOR);

		if (LINE_LENGTH > (int)sizeof LINE_BUFFER - 1)
			LINE_LENGTH = sizeof LINE_BUFFER - 1;

		memcpy(LINE_BUFFER, CURSOR, (size_t)LINE_LENGTH);
		LINE_BUFFER[LINE_LENGTH] = 0;

		while (
			LINE_LENGTH &&
			(
				LINE_BUFFER[LINE_LENGTH - 1] == '\r' ||
				LINE_BUFFER[LINE_LENGTH - 1] == ' '
			)
		)
			LINE_BUFFER[--LINE_LENGTH] = 0;

		if (LINE_END)
			CURSOR = LINE_END + 1;
		else
			CURSOR = CURSOR + strlen(CURSOR);

		if (!LINE_LENGTH || LINE_BUFFER[0] == '#')
			continue ;

		if (!strcmp(LINE_BUFFER, ":reset"))
		{
			KNOWLEDGE_RESET(KNOWLEDGE_BASE);
			continue ;
		}

		if (LINE_BUFFER[0] == '>')
		{
			snprintf(
				USER_INPUT, sizeof USER_INPUT, "%s",
				LINE_BUFFER + 1 + (LINE_BUFFER[1] == ' ')
			);
			KNOWLEDGE_HEAR(KNOWLEDGE_BASE, USER_INPUT, REPLY, sizeof REPLY);
			COPY_LOWERCASE(LOWER_REPLY, REPLY, sizeof LOWER_REPLY);
			HAS_REPLY = 1;

			if (SHOW_OUTPUT)
				printf(
					"> %s\n  %s\n", USER_INPUT,
					REPLY[0] ? REPLY : "(not a fact or question I can parse)"
				);

			continue ;
		}

		if ((LINE_BUFFER[0] == '=' || LINE_BUFFER[0] == '!') && HAS_REPLY)
		{
			char	EXPECTED[1024];

			COPY_LOWERCASE(
				EXPECTED, LINE_BUFFER + 1 + (LINE_BUFFER[1] == ' '),
				sizeof EXPECTED
			);

			int	IS_FOUND = strstr(LOWER_REPLY, EXPECTED) != NULL;

			if (!strcmp(EXPECTED, "(nothing)"))
				IS_FOUND = LOWER_REPLY[0] == 0;

			int	PASSED;

			if (LINE_BUFFER[0] == '=')
				PASSED = IS_FOUND;
			else
				PASSED = !IS_FOUND;

			CHECK_COUNT++;

			if (!PASSED)
			{
				FAIL_COUNT++;
				printf(
					"  FAIL > %s\n       expected %s\"%s\"\n       got: %s\n",
					USER_INPUT, LINE_BUFFER[0] == '=' ? "" : "NOT ", EXPECTED,
					REPLY[0] ? REPLY : "(nothing)"
				);
			}
		}
	}

	KNOWLEDGE_FREE(KNOWLEDGE_BASE);

	if (CHECK_COUNT_OUT)
		*CHECK_COUNT_OUT = CHECK_COUNT;

	return (FAIL_COUNT);
}

int
	KNOWLEDGE_SCRIPT_FILE(const char *PATH, int SHOW_OUTPUT)
{
	FILE	*STREAM = fopen(PATH, "rb");

	if (!STREAM)
	{
		printf("cannot open %s\n", PATH);
		return (1);
	}

	fseek(STREAM, 0, SEEK_END);

	long	FILE_LENGTH = ftell(STREAM);

	fseek(STREAM, 0, SEEK_SET);

	char	*SCRIPT_TEXT = malloc((size_t)FILE_LENGTH + 1);
	size_t	READ_COUNT = fread(SCRIPT_TEXT, 1, (size_t)FILE_LENGTH, STREAM);

	SCRIPT_TEXT[READ_COUNT] = 0;
	fclose(STREAM);

	int	CHECK_COUNT = 0;
	int	FAIL_COUNT = KNOWLEDGE_SCRIPT(SCRIPT_TEXT, SHOW_OUTPUT, &CHECK_COUNT);

	printf(
		"know: %d of %d checks passed%s\n", CHECK_COUNT - FAIL_COUNT,
		CHECK_COUNT, FAIL_COUNT ? "" : " - ALL PASSED"
	);
	free(SCRIPT_TEXT);

	if (FAIL_COUNT)
		return (1);

	return (0);
}
