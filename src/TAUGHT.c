#include "TAUGHT.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static const char	*STOP_WORDS[113] = {
	"a", "an", "the", "and", "or", "but", "of", "to", "in", "on", "at", "by",
	"for", "with", "from", "into", "onto", "as", "is", "are", "was", "were",
	"be", "been", "being", "am", "it", "its", "it's", "this", "that", "these",
	"those", "there", "here", "what", "which", "who", "whom", "whose", "when",
	"where", "why", "how", "i", "me", "my", "we", "us", "our", "you", "your",
	"he", "him", "his", "she", "her", "they", "them", "their", "do", "does",
	"did", "have", "has", "had", "can", "could", "will", "would", "shall",
	"should", "may", "might", "must", "so", "if", "then", "than", "too", "very",
	"just", "also", "about", "some", "any", "all", "most", "more", "much",
	"many", "such", "like", "up", "out", "over", "not", "no", "never", "isn't",
	"aren't", "wasn't", "weren't", "don't", "doesn't", "didn't", "can't",
	"cannot", "won't", "basically", "actually", "really", NULL
};

static const char	*NEGATION_WORDS[25] = {
	"not", "never", "no", "isn't", "aren't", "wasn't", "weren't", "doesn't",
	"don't", "didn't", "can't", "cannot", "won't", "nobody", "nothing", "none",
	"neither", "nor", "hasn't", "haven't", "hadn't", "shouldn't", "wouldn't",
	"couldn't", NULL
};

static const char	*FILLER_WORDS[35] = {
	"basically", "so", "well", "actually", "also", "and", "but", "now",
	"anyway", "alright", "alriighty", "allright", "ok", "okay", "haha",
	"hahaha", "ah", "oh", "yes", "yeah", "no", "right", "see", "hmm", "aha",
	"gotcha", "sure", "lol", "um", "uh", "plus", "then", "code", "lucy", NULL
};

static const char	*QUESTION_STARTS[39] = {
	"what", "what's", "whats", "who", "who's", "whom", "whose", "where",
	"where's", "why", "which", "do", "does", "did", "can", "could", "would",
	"will", "should", "shall", "may", "might", "is", "are", "was", "were",
	"have", "has", "had", "am", "isn't", "aren't", "don't", "doesn't", "didn't",
	"won't", "can't", "how", NULL
};

static const char	*COMMAND_STARTS[87] = {
	"ask", "tell", "say", "give", "explain", "answer", "try", "remember",
	"forget", "delete", "make", "create", "write", "move", "rename", "list",
	"show", "read", "open", "call", "check", "count", "reverse", "spell",
	"calculate", "compute", "add", "put", "let's", "lets", "let", "don't", "do",
	"stop", "go", "come", "look", "wait", "keep", "use", "type", "find",
	"search", "help", "describe", "define", "translate", "repeat", "think",
	"learn", "note", "save", "run", "start", "end", "close", "quit", "be",
	"imagine", "pretend", "guess", "choose", "pick", "please", "thanks",
	"thank", "hi", "hello", "hey", "bye", "goodbye", "welcome", "sorry",
	"congrats", "continue", "play", "finish", "turn", "set", "send", "remind",
	"bring", "get", "maybe", "perhaps", "probably", NULL
};

static const char	*PERSONAL_STARTS[45] = {
	"i", "i'm", "im", "i've", "i'd", "i'll", "my", "me", "we", "we're", "we'll",
	"we'd", "we've", "our", "us", "mine", "you", "you're", "youre", "your",
	"you've", "you'll", "you'd", "yours", "that", "that's", "thats", "there",
	"there's", "here", "here's", "these", "those", "he", "he's", "she", "she's",
	"them", "his", "her", "nothing", "everything", "something", "nobody", NULL
};

static const char	*PRONOUN_STARTS[7] = {
	"it", "it's", "its", "this", "they", "they're", NULL
};

static const char	*COPULA_WORDS[7] = {
	"is", "are", "means", "mean", "refers", "stands", NULL
};

static const char	*EVIDENCE_MARKERS[47] = {
	"according to ", "i read it in ", "i read it on ", "i read in ",
	"i read on ", "i read that in ", "i read this in ", "i saw it in ",
	"i saw it on ", "i saw that in ", "i found it in ", "i found it on ",
	"i found this in ", "i found this on ", "i learned it in ",
	"i learned it from ", "i learned that in ", "i heard it on ", "source: ",
	"source is ", "the source is ", "my source is ", "proof: ", "the proof is ",
	"evidence: ", "the evidence is ", "you can check ", "you can read it in ",
	"you can read it on ", "you can see it in ", "see: ", "it says so in ",
	"it's written in ", "it is written in ", "scientists found ",
	"scientists have found ", "scientists discovered ", "studies show ",
	"research shows ", "the article says ", "the book says ", "the article ",
	"http", "www.", "as shown in ", "as written in ", NULL
};

static const char	*EVIDENCE_ONLY_STARTS[28] = {
	"according to ", "i read ", "i saw ", "i found ", "i learned ",
	"i heard it ", "i checked ", "source", "proof", "evidence",
	"you can check ", "you can read ", "you can see ", "see: ", "it says ",
	"it's written ", "it is written ", "it's from ", "it is from ",
	"that's from ", "that is from ", "here is the ", "here's the ", "http",
	"www.", "the article says ", "the book says ", NULL
};

static const char	*SOURCE_QUESTIONS[23] = {
	"who told you", "how do you know", "how did you know",
	"where did you learn", "where did you hear", "where did you read",
	"where did you get", "what is your source", "what's your source",
	"whats your source", "what source", "why do you think",
	"why do you believe", "who said that", "who said so", "any proof",
	"prove it", "your proof", "who taught you", "where is that from",
	"where's that from", "where does that come from", NULL
};

static int
	IN_LIST(const char *WORD, const char **LIST)
{
	int	INDEX;

	for (INDEX = 0; LIST[INDEX]; INDEX++)
		if (!strcmp(WORD, LIST[INDEX]))
			return (1);

	return (0);
}

static void
	LOWER_COPY(const char *SOURCE, char *DESTINATION, int SIZE)
{
	int	INDEX;

	for (INDEX = 0; SOURCE[INDEX] && INDEX < SIZE - 1; INDEX++)
		DESTINATION[INDEX] = (char)tolower((unsigned char)SOURCE[INDEX]);

	DESTINATION[INDEX] = 0;
}

/* lower case words separated by single spaces, with a space at both ends */
static void
	NORMALIZE(const char *SOURCE, char *DESTINATION, int SIZE)
{
	int	POSITION = 0;
	int	INDEX;

	if (SIZE < 3)
	{
		DESTINATION[0] = 0;
		return ;
	}

	DESTINATION[POSITION++] = ' ';

	for (INDEX = 0; SOURCE[INDEX] && POSITION < SIZE - 2; INDEX++)
	{
		unsigned char	CHARACTER = (unsigned char)SOURCE[INDEX];

		if (
			isalnum(CHARACTER) ||
			(
				CHARACTER == '\'' &&
				INDEX > 0 &&
				isalpha((unsigned char)SOURCE[INDEX - 1])
			) ||
			CHARACTER >= 128
		)
			DESTINATION[POSITION++] = (char)tolower(CHARACTER);
		else if (DESTINATION[POSITION - 1] != ' ')
			DESTINATION[POSITION++] = ' ';
	}

	if (DESTINATION[POSITION - 1] != ' ')
		DESTINATION[POSITION++] = ' ';

	DESTINATION[POSITION] = 0;
}

static void
	SINGULAR(const char *WORD, char *OUTPUT, int SIZE)
{
	size_t	LENGTH = strlen(WORD);

	snprintf(OUTPUT, SIZE, "%s", WORD);

	if (
		LENGTH <= 3 ||
		WORD[LENGTH - 1] != 's' ||
		WORD[LENGTH - 2] == 's' ||
		strchr(WORD, '\'')
	)
		return ;

	if (LENGTH > 4 && !strcmp(WORD + LENGTH - 3, "ies"))
	{
		snprintf(OUTPUT, SIZE, "%.*sy", (int)(LENGTH - 3), WORD);
		return ;
	}

	if (
		LENGTH > 4 &&
		(
			!strcmp(WORD + LENGTH - 4, "ches") ||
			!strcmp(WORD + LENGTH - 4, "shes") ||
			!strcmp(WORD + LENGTH - 3, "xes") ||
			!strcmp(WORD + LENGTH - 4, "sses")
		)
	)
	{
		snprintf(OUTPUT, SIZE, "%.*s", (int)(LENGTH - 2), WORD);
		return ;
	}

	if (WORD[LENGTH - 2] == 'u' || WORD[LENGTH - 2] == 'i')
		return ;

	snprintf(OUTPUT, SIZE, "%.*s", (int)(LENGTH - 1), WORD);
}

/* one form for "went", "go", "goes", "going"; "evolved", "evolve" */
static void
	WORD_STEM(const char *WORD, char *OUTPUT, int SIZE)
{
	static const char	*IRREGULAR[177] = {
		"went", "go", "gone", "go", "goes", "go", "ate", "eat", "eaten", "eat",
		"saw", "see", "seen", "see", "came", "come", "took", "take", "taken",
		"take", "made", "make", "got", "get", "gotten", "get", "gave", "give",
		"given", "give", "knew", "know", "known", "know", "ran", "run", "flew",
		"fly", "flown", "fly", "flies", "fly", "grew", "grow", "grown", "grow",
		"began", "begin", "begun", "begin", "found", "find", "told", "tell",
		"said", "say", "says", "say", "thought", "think", "left", "leave",
		"died", "die", "dies", "die", "dying", "die", "lied", "lie", "was",
		"be", "were", "be", "been", "be", "is", "be", "are", "be", "am", "be",
		"did", "do", "does", "do", "done", "do", "had", "have", "has", "have",
		"wrote", "write", "written", "write", "spoke", "speak", "spoken",
		"speak", "lay", "lie", "laid", "lay", "built", "build", "bought", "buy",
		"brought", "bring", "caught", "catch", "taught", "teach", "fought",
		"fight", "held", "hold", "kept", "keep", "lost", "lose", "met", "meet",
		"paid", "pay", "sold", "sell", "sent", "send", "sat", "sit", "stood",
		"stand", "won", "win", "felt", "feel", "fell", "fall", "fallen", "fall",
		"drank", "drink", "drunk", "drink", "swam", "swim", "sang", "sing",
		"sung", "sing", "rode", "ride", "ridden", "ride", "rose", "rise",
		"risen", "rise", "children", "child", "people", "person", "men", "man",
		"women", "woman", "mice", "mouse", "feet", "foot", "teeth", "tooth",
		"geese", "goose", NULL
	};
	int					INDEX;
	char				WORK[64];
	size_t				LENGTH;

	for (INDEX = 0; IRREGULAR[INDEX]; INDEX += 2)
		if (!strcmp(WORD, IRREGULAR[INDEX]))
		{
			snprintf(OUTPUT, SIZE, "%s", IRREGULAR[INDEX + 1]);
			return ;
		}

	SINGULAR(WORD, WORK, sizeof WORK);
	LENGTH = strlen(WORK);

	if (LENGTH > 5 && !strcmp(WORK + LENGTH - 3, "ing"))
		WORK[LENGTH -= 3] = 0;
	else if (LENGTH > 4 && !strcmp(WORK + LENGTH - 2, "ed"))
		WORK[LENGTH -= 2] = 0;

	if (LENGTH > 3 && WORK[LENGTH - 1] == 'e')
		WORK[--LENGTH] = 0;

	if (LENGTH > 6)
		WORK[6] = 0;

	snprintf(OUTPUT, SIZE, "%s", WORK);
}

/* the words of a normalized text, split in place */
static int
	SPLIT_WORDS(char *NORMALIZED, char **WORDS, int MAXIMUM)
{
	int		COUNT = 0;
	char	*CURSOR = NORMALIZED;

	while (*CURSOR && COUNT < MAXIMUM)
	{
		while (*CURSOR == ' ')
			CURSOR++;

		if (!*CURSOR)
			break ;

		WORDS[COUNT++] = CURSOR;

		while (*CURSOR && *CURSOR != ' ')
			CURSOR++;

		if (*CURSOR)
			*CURSOR++ = 0;
	}

	return (COUNT);
}

/* content words (singular), each with spaces around it, joined: " cat leg " */
static void
	CONTENT_OF(const char *TEXT, char *OUTPUT, int SIZE, int KEEP_NUMBERS)
{
	char	NORMALIZED[1200];
	char	*WORDS[160];
	int		COUNT;
	int		INDEX;
	int		POSITION = 0;

	NORMALIZE(TEXT, NORMALIZED, sizeof NORMALIZED);
	COUNT = SPLIT_WORDS(NORMALIZED, WORDS, 160);
	OUTPUT[POSITION++] = ' ';
	OUTPUT[POSITION] = 0;

	for (INDEX = 0; INDEX < COUNT; INDEX++)
	{
		char	STEM[64];

		if (IN_LIST(WORDS[INDEX], STOP_WORDS))
			continue ;

		/* "8 bits": a one-digit number still counts when numbers do */
		if (
			strlen(WORDS[INDEX]) < 2 &&
			!(KEEP_NUMBERS && isdigit((unsigned char)WORDS[INDEX][0]))
		)
			continue ;

		if (!KEEP_NUMBERS && isdigit((unsigned char)WORDS[INDEX][0]))
			continue ;

		WORD_STEM(WORDS[INDEX], STEM, sizeof STEM);

		if (POSITION + (int)strlen(STEM) + 2 >= SIZE)
			break ;

		POSITION += snprintf(OUTPUT + POSITION, SIZE - POSITION, "%s ", STEM);
	}
}

static int
	COUNT_WORDS(const char *SPACED)
{
	int	COUNT = 0;
	int	INDEX;

	for (INDEX = 1; SPACED[INDEX]; INDEX++)
		if (SPACED[INDEX] == ' ' && SPACED[INDEX - 1] != ' ')
			COUNT++;

	return (COUNT);
}

/* how many words of A are also in B (both " word word ") */
static int
	SHARED_WORDS(const char *FIRST, const char *SECOND)
{
	int			COUNT = 0;
	const char	*CURSOR = FIRST;

	while (*CURSOR)
	{
		while (*CURSOR == ' ')
			CURSOR++;

		if (!*CURSOR)
			break ;

		const char	*WORD_END = strchr(CURSOR, ' ');
		char		PATTERN[80];
		int			LENGTH;

		if (WORD_END)
			LENGTH = (int)(WORD_END - CURSOR);
		else
			LENGTH = (int)strlen(CURSOR);

		if (LENGTH < 70)
		{
			snprintf(PATTERN, sizeof PATTERN, " %.*s ", LENGTH, CURSOR);

			if (strstr(SECOND, PATTERN))
				COUNT++;
		}

		CURSOR += LENGTH;
	}

	return (COUNT);
}

static int
	HAS_NEGATION(const char *TEXT)
{
	char	NORMALIZED[1200];
	char	*WORDS[160];
	int		COUNT;
	int		INDEX;

	NORMALIZE(TEXT, NORMALIZED, sizeof NORMALIZED);
	COUNT = SPLIT_WORDS(NORMALIZED, WORDS, 160);

	for (INDEX = 0; INDEX < COUNT; INDEX++)
		if (
			IN_LIST(WORDS[INDEX], NEGATION_WORDS) ||
			(
				strlen(WORDS[INDEX]) > 3 &&
				!strcmp(WORDS[INDEX] + strlen(WORDS[INDEX]) - 3, "n't")
			)
		)
			return (1);

	return (0);
}

static int
	HAS_FIRST_PERSON(const char *TEXT)
{
	static const char	*FIRST_PERSON[11] = {
		"i", "i'm", "im", "i've", "i'd", "i'll", "me", "my", "mine", "myself",
		NULL
	};
	char				NORMALIZED[1200];
	char				UNQUOTED[1200];
	char				*WORDS[160];
	int					COUNT;
	int					INDEX;
	int					POSITION = 0;
	int					IN_QUOTE = 0;

	for (INDEX = 0; TEXT[INDEX] && POSITION < (int)sizeof UNQUOTED - 1; INDEX++)
	{
		if (TEXT[INDEX] == '"')
			IN_QUOTE = !IN_QUOTE;
		else if (!IN_QUOTE)
			UNQUOTED[POSITION++] = TEXT[INDEX];
	}

	UNQUOTED[POSITION] = 0;
	NORMALIZE(UNQUOTED, NORMALIZED, sizeof NORMALIZED);
	COUNT = SPLIT_WORDS(NORMALIZED, WORDS, 160);

	for (INDEX = 0; INDEX < COUNT; INDEX++)
		if (IN_LIST(WORDS[INDEX], FIRST_PERSON))
			return (1);

	return (0);
}

/* "now it's 12:10", "today is ...", "time does not work like that": about this
 * moment or pointing back into the chat */
static int
	IS_MOMENT_BOUND(const char *TEXT)
{
	static const char	*MOMENT_WORDS[15] = {
		"now", "today", "tonight", "yesterday", "tomorrow", "earlier",
		"currently", "later", "recently", "lately", "again", "typo", "here",
		"hurry", NULL
	};
	static const char	*POINTING_ENDS[9] = {
		"that", "this", "it", "them", "those", "these", "xd", "lol", NULL
	};
	char				NORMALIZED[1200];
	char				*WORDS[160];
	int					COUNT;
	int					INDEX;

	NORMALIZE(TEXT, NORMALIZED, sizeof NORMALIZED);
	COUNT = SPLIT_WORDS(NORMALIZED, WORDS, 160);

	for (INDEX = 0; INDEX < COUNT; INDEX++)
		if (IN_LIST(WORDS[INDEX], MOMENT_WORDS))
			return (1);

	return (COUNT && IN_LIST(WORDS[COUNT - 1], POINTING_ENDS));
}

/* casual talk ("tbh he kinda has a point", "hes right isnt he") is not
 * someone teaching */
static int
	IS_CHATTY(const char *TEXT)
{
	static const char	*CHAT_WORDS[45] = {
		"tbh", "lol", "lmao", "omg", "idk", "u", "ur", "pls", "plz", "fr", "rn",
		"btw", "imo", "ngl", "smh", "sus", "bruh", "xd", "yall", "ya", "kinda",
		"sorta", "gonna", "wanna", "gotta", "lemme", "dunno", "haha", "hahaha",
		"hehe", "thx", "thanks", "tmrw", "tho", "im", "bf", "gf", "cuz", "bc",
		"srsly", "jk", "nvm", "ikr", "please", NULL
	};
	static const char	*TAG_ENDS[20] = {
		" isn't he ", " isnt he ", " isn't she ", " isnt she ", " isn't it ",
		" isnt it ", " aren't they ", " arent they ", " don't they ",
		" dont they ", " doesn't it ", " doesnt it ", " right ", " correct ",
		" huh ", " yeah ", " don't you think ", " dont you think ", " no ", NULL
	};
	char				NORMALIZED[1200];
	char				COPY[1200];
	char				*WORDS[160];
	int					COUNT;
	int					INDEX;
	size_t				LENGTH;

	NORMALIZE(TEXT, NORMALIZED, sizeof NORMALIZED);
	LENGTH = strlen(NORMALIZED);

	for (INDEX = 0; TAG_ENDS[INDEX]; INDEX++)
	{
		size_t	TAG_LENGTH = strlen(TAG_ENDS[INDEX]);

		if (
			LENGTH >= TAG_LENGTH &&
			!strcmp(NORMALIZED + LENGTH - TAG_LENGTH, TAG_ENDS[INDEX])
		)
			return (1);
	}

	snprintf(COPY, sizeof COPY, "%s", NORMALIZED);
	COUNT = SPLIT_WORDS(COPY, WORDS, 160);

	for (INDEX = 0; INDEX < COUNT; INDEX++)
		if (IN_LIST(WORDS[INDEX], CHAT_WORDS))
			return (1);

	return (0);
}

static int
	HAS_SECOND_PERSON(const char *TEXT)
{
	static const char	*SECOND_PERSON[9] = {
		"you", "you're", "your", "yours", "you've", "you'll", "you'd",
		"yourself", NULL
	};
	char				NORMALIZED[1200];
	char				*WORDS[160];
	int					COUNT;
	int					INDEX;

	NORMALIZE(TEXT, NORMALIZED, sizeof NORMALIZED);
	COUNT = SPLIT_WORDS(NORMALIZED, WORDS, 160);

	for (INDEX = 0; INDEX < COUNT; INDEX++)
		if (IN_LIST(WORDS[INDEX], SECOND_PERSON))
			return (1);

	return (0);
}

static void
	NUMBERS_OF(const char *TEXT, char *OUTPUT, int SIZE)
{
	int	POSITION = 0;
	int	INDEX;

	OUTPUT[POSITION++] = ' ';
	OUTPUT[POSITION] = 0;

	for (INDEX = 0; TEXT[INDEX]; INDEX++)
	{
		if (!isdigit((unsigned char)TEXT[INDEX]))
			continue ;

		if (INDEX > 0 && isdigit((unsigned char)TEXT[INDEX - 1]))
			continue ;

		int	LENGTH = 0;

		while (
			isdigit((unsigned char)TEXT[INDEX + LENGTH]) ||
			(
				(TEXT[INDEX + LENGTH] == '.' || TEXT[INDEX + LENGTH] == ',') &&
				isdigit((unsigned char)TEXT[INDEX + LENGTH + 1])
			)
		)
			LENGTH++;

		if (POSITION + LENGTH + 2 >= SIZE)
			break ;

		POSITION += snprintf(
			OUTPUT + POSITION, SIZE - POSITION, "%.*s ", LENGTH, TEXT + INDEX
		);
	}
}

static void
	TRIM(char *TEXT)
{
	size_t	LENGTH = strlen(TEXT);
	size_t	START = 0;

	while (TEXT[START] == ' ' || TEXT[START] == '\t' || TEXT[START] == '-')
		START++;

	if (START)
		memmove(TEXT, TEXT + START, LENGTH - START + 1);

	LENGTH = strlen(TEXT);

	while (
		LENGTH &&
		(
			TEXT[LENGTH - 1] == ' ' ||
			TEXT[LENGTH - 1] == '\t' ||
			TEXT[LENGTH - 1] == '\r'
		)
	)
		TEXT[--LENGTH] = 0;
}

/* the first word of a sentence, lower case, without punctuation around it */
static void
	FIRST_WORD(const char *TEXT, char *OUTPUT, int SIZE)
{
	int	POSITION = 0;

	while (*TEXT && !isalnum((unsigned char)*TEXT))
		TEXT++;

	while (
		*TEXT &&
		(isalnum((unsigned char)*TEXT) || *TEXT == '\'') &&
		POSITION < SIZE - 1
	)
		OUTPUT[POSITION++] = (char)tolower((unsigned char)*TEXT++);

	OUTPUT[POSITION] = 0;
}

/* "Haha, no, time does not work like that" -> "time does not work like that" */
static const char
	*SKIP_FILLERS(const char *TEXT)
{
	for (;;)
	{
		char		WORD[40];
		const char	*CURSOR = TEXT;

		while (*CURSOR == ' ' || *CURSOR == '-' || *CURSOR == ',')
			CURSOR++;

		FIRST_WORD(CURSOR, WORD, sizeof WORD);

		if (!WORD[0] || !IN_LIST(WORD, FILLER_WORDS))
			return (CURSOR);

		const char	*AFTER = CURSOR;

		while (*AFTER && (isalnum((unsigned char)*AFTER) || *AFTER == '\''))
			AFTER++;

		/* "so" and "now" only count as fillers when followed by a comma */
		if (
			(
				!strcmp(WORD, "so") ||
				!strcmp(WORD, "now") ||
				!strcmp(WORD, "see") ||
				!strcmp(WORD, "then") ||
				!strcmp(WORD, "plus") ||
				!strcmp(WORD, "code") ||
				!strcmp(WORD, "lucy")
			) &&
			*AFTER != ','
		)
			return (CURSOR);

		while (*AFTER == ',' || *AFTER == '!' || *AFTER == '.' || *AFTER == ' ')
			AFTER++;

		TEXT = AFTER;
	}
}

static int
	ENDS_WITH_QUESTION(const char *TEXT)
{
	int		IN_QUOTE = 0;
	int		LAST = 0;
	size_t	INDEX;

	for (INDEX = 0; TEXT[INDEX]; INDEX++)
	{
		if (TEXT[INDEX] == '"')
			IN_QUOTE = !IN_QUOTE;
		else if (!IN_QUOTE && !isspace((unsigned char)TEXT[INDEX]))
			LAST = TEXT[INDEX];
	}

	return (LAST == '?');
}

static int
	MARKER_AT(const char *LOWER, const char **MARKERS, const char **AFTER)
{
	int	INDEX;

	for (INDEX = 0; MARKERS[INDEX]; INDEX++)
	{
		const char	*FOUND = strstr(LOWER, MARKERS[INDEX]);

		if (
			FOUND &&
			(
				FOUND == LOWER ||
				FOUND[-1] == ' ' ||
				FOUND[-1] == '"' ||
				FOUND[-1] == '('
			)
		)
		{
			if (AFTER)
				*AFTER = FOUND;

			return (1);
		}
	}

	return (0);
}

/* the support a sentence gives for itself: "according to X", "I read it in X",
 * a link, "because ..." */
static int
	EVIDENCE_IN(const char *SENTENCE, char *OUTPUT, int SIZE)
{
	char		LOWER[600];
	const char	*FOUND = NULL;

	LOWER_COPY(SENTENCE, LOWER, sizeof LOWER);

	if (!MARKER_AT(LOWER, EVIDENCE_MARKERS, &FOUND))
		return (0);

	const char	*ORIGINAL = SENTENCE + (FOUND - LOWER);

	snprintf(OUTPUT, SIZE, "%s", ORIGINAL);
	TRIM(OUTPUT);

	size_t	LENGTH = strlen(OUTPUT);

	while (LENGTH && (OUTPUT[LENGTH - 1] == '.' || OUTPUT[LENGTH - 1] == '!'))
		OUTPUT[--LENGTH] = 0;

	return (LENGTH > 3);
}

static int
	EVIDENCE_ONLY(const char *SENTENCE)
{
	char	LOWER[600];

	LOWER_COPY(SKIP_FILLERS(SENTENCE), LOWER, sizeof LOWER);

	int	INDEX;

	for (INDEX = 0; EVIDENCE_ONLY_STARTS[INDEX]; INDEX++)
		if (
			!strncmp(
				LOWER, EVIDENCE_ONLY_STARTS[INDEX],
				strlen(EVIDENCE_ONLY_STARTS[INDEX])
			)
		)
			return (1);

	return (0);
}

int
	TAUGHT_ASKS_SOURCE(const char *SENTENCE)
{
	char	NORMALIZED[600];

	NORMALIZE(SENTENCE, NORMALIZED, sizeof NORMALIZED);

	int	INDEX;

	for (INDEX = 0; SOURCE_QUESTIONS[INDEX]; INDEX++)
	{
		char	PATTERN[80];

		snprintf(PATTERN, sizeof PATTERN, " %s ", SOURCE_QUESTIONS[INDEX]);

		if (strstr(NORMALIZED, PATTERN))
			return (1);
	}

	return (0);
}

/* the subject of a definition: "A name is a word ..." -> "name";
 * '"Are you alright?" means ...' -> "are you alright" */
static int
	DEFINITION_KEY(
		const char *SENTENCE, char *KEY, int KEY_SIZE, char *SUBJECT,
		int SUBJECT_SIZE, int *IS_SPECIFIC
	)
{
	*IS_SPECIFIC = 0;
	KEY[0] = SUBJECT[0] = 0;

	if (SENTENCE[0] == '"')
	{
		const char	*CLOSE = strchr(SENTENCE + 1, '"');

		if (!CLOSE || CLOSE - SENTENCE > 70)
			return (0);

		char	AFTER[80];

		LOWER_COPY(CLOSE + 1, AFTER, sizeof AFTER);

		if (
			strncmp(AFTER, " means ", 7) &&
			strncmp(AFTER, " is ", 4) &&
			strncmp(AFTER, " stands for ", 12) &&
			strncmp(AFTER, " refers to ", 11)
		)
			return (0);

		char	QUOTED[80];
		char	NORMALIZED[100];

		snprintf(
			QUOTED, sizeof QUOTED, "%.*s", (int)(CLOSE - SENTENCE - 1),
			SENTENCE + 1
		);
		NORMALIZE(QUOTED, NORMALIZED, sizeof NORMALIZED);
		TRIM(NORMALIZED);
		snprintf(KEY, KEY_SIZE, "%s", NORMALIZED);
		snprintf(SUBJECT, SUBJECT_SIZE, "\"%s\"", QUOTED);
		return (KEY[0] != 0);
	}

	char	NORMALIZED[600];
	char	*WORDS[120];
	int		COUNT;

	NORMALIZE(SENTENCE, NORMALIZED, sizeof NORMALIZED);
	COUNT = SPLIT_WORDS(NORMALIZED, WORDS, 120);

	int	FIRST = 0;

	if (
		COUNT &&
		(
			!strcmp(WORDS[0], "a") ||
			!strcmp(WORDS[0], "an") ||
			!strcmp(WORDS[0], "the") ||
			(!strcmp(WORDS[0], "one") && COUNT > 2 && strcmp(WORDS[1], "of"))
		)
	)
		FIRST = 1;

	if (COUNT && !strcmp(WORDS[0], "the"))
		*IS_SPECIFIC = 1;

	int	COPULA = -1;
	int	INDEX;

	for (INDEX = FIRST + 1; INDEX < COUNT && INDEX <= FIRST + 4; INDEX++)
		if (IN_LIST(WORDS[INDEX], COPULA_WORDS))
		{
			COPULA = INDEX;
			break ;
		}

	if (COPULA < 0 || COPULA + 1 >= COUNT)
		return (0);

	/* "refers" / "stands" need their "to" / "for" */
	if (
		(!strcmp(WORDS[COPULA], "refers") && strcmp(WORDS[COPULA + 1], "to")) ||
		(!strcmp(WORDS[COPULA], "stands") && strcmp(WORDS[COPULA + 1], "for"))
	)
		return (0);

	for (INDEX = FIRST; INDEX < COPULA; INDEX++)
	{
		if (
			IN_LIST(WORDS[INDEX], PERSONAL_STARTS) ||
			IN_LIST(WORDS[INDEX], PRONOUN_STARTS) ||
			IN_LIST(WORDS[INDEX], QUESTION_STARTS) ||
			(IN_LIST(WORDS[INDEX], STOP_WORDS) && strcmp(WORDS[INDEX], "of"))
		)
			return (0);

		if (isdigit((unsigned char)WORDS[INDEX][0]))
			*IS_SPECIFIC = 1;
	}

	/* a capital letter inside the subject marks a name: "Bobo is ..." */
	{
		const char	*CURSOR = SENTENCE;
		int			WORD_NUMBER = 0;

		while (*CURSOR && WORD_NUMBER < COPULA)
		{
			while (*CURSOR && !isalnum((unsigned char)*CURSOR))
				CURSOR++;

			if (
				WORD_NUMBER >= FIRST &&
				isupper((unsigned char)*CURSOR) &&
				WORD_NUMBER > 0
			)
				*IS_SPECIFIC = 1;

			if (
				WORD_NUMBER == 0 &&
				FIRST == 0 &&
				isupper((unsigned char)*CURSOR)
			)
			{
				/* the sentence start is always capital: a name only when the
				 * word is not a common lower case word elsewhere */
			}

			while (
				*CURSOR &&
				(isalnum((unsigned char)*CURSOR) || *CURSOR == '\'')
			)
				CURSOR++;

			WORD_NUMBER++;
		}
	}

	int	POSITION = 0;

	KEY[0] = 0;

	for (INDEX = FIRST; INDEX < COPULA; INDEX++)
	{
		char	WORD[64];

		if (INDEX == COPULA - 1)
			SINGULAR(WORDS[INDEX], WORD, sizeof WORD);
		else
			snprintf(WORD, sizeof WORD, "%s", WORDS[INDEX]);

		POSITION += snprintf(
			KEY + POSITION, KEY_SIZE - POSITION, "%s%s", POSITION ? " " : "",
			WORD
		);

		if (POSITION >= KEY_SIZE - 1)
			break ;
	}

	/* the subject as written, for "it" in the next sentence */
	{
		const char	*CURSOR = SENTENCE;
		int			WORD_NUMBER = 0;

		while (*CURSOR && WORD_NUMBER < COPULA)
		{
			while (*CURSOR && !isalnum((unsigned char)*CURSOR))
				CURSOR++;

			while (
				*CURSOR &&
				(isalnum((unsigned char)*CURSOR) || *CURSOR == '\'')
			)
				CURSOR++;

			WORD_NUMBER++;
		}

		snprintf(
			SUBJECT, SUBJECT_SIZE, "%.*s", (int)(CURSOR - SENTENCE), SENTENCE
		);

		int		QUOTES = 0;
		size_t	SUBJECT_INDEX;

		for (SUBJECT_INDEX = 0; SUBJECT[SUBJECT_INDEX]; SUBJECT_INDEX++)
			if (SUBJECT[SUBJECT_INDEX] == '"')
				QUOTES++;

		if (QUOTES % 2 && strlen(SUBJECT) + 1 < (size_t)SUBJECT_SIZE)
			strcat(SUBJECT, "\"");
	}

	return (KEY[0] != 0);
}

void
	TAUGHT_SENTENCE(const TAUGHT_FACT *FACT, char *OUTPUT, int OUTPUT_SIZE)
{
	snprintf(OUTPUT, OUTPUT_SIZE, "%s", FACT->TEXT);
}

int
	TAUGHT_SPLIT(const char *TEXT, char (*SENTENCES)[600], int MAXIMUM)
{
	int		COUNT = 0;
	int		POSITION = 0;
	int		IN_QUOTE = 0;
	size_t	INDEX;

	for (INDEX = 0;; INDEX++)
	{
		char	CHARACTER = TEXT[INDEX];
		int		CUT = 0;

		if (CHARACTER == '"')
			IN_QUOTE = !IN_QUOTE;

		if (!CHARACTER)
			CUT = 1;
		else if (CHARACTER == '\n')
			CUT = 1;
		else if (
			!IN_QUOTE &&
			(
				CHARACTER == '.' ||
				CHARACTER == '!' ||
				CHARACTER == '?' ||
				CHARACTER == ';'
			) &&
			(
				TEXT[INDEX + 1] == ' ' ||
				TEXT[INDEX + 1] == 0 ||
				TEXT[INDEX + 1] == '\n'
			)
		)
		{
			/* "e.g." "Mr." and other short abbreviations stay inside */
			int	LETTERS = 0;

			while (
				LETTERS < (int)INDEX &&
				isalpha((unsigned char)TEXT[INDEX - 1 - LETTERS])
			)
				LETTERS++;

			char	ABBREVIATION[8] = "";

			if (LETTERS && LETTERS < 7)
			{
				int	LETTER_INDEX;

				for (LETTER_INDEX = 0; LETTER_INDEX < LETTERS; LETTER_INDEX++)
					ABBREVIATION[LETTER_INDEX] = (char)tolower((unsigned char
					)TEXT[INDEX - LETTERS + LETTER_INDEX]);

				ABBREVIATION[LETTERS] = 0;
			}

			if (
				CHARACTER != '.' ||
				!(
					LETTERS == 1 ||
					!strcmp(ABBREVIATION, "mr") ||
					!strcmp(ABBREVIATION, "mrs") ||
					!strcmp(ABBREVIATION, "ms") ||
					!strcmp(ABBREVIATION, "dr") ||
					!strcmp(ABBREVIATION, "vs") ||
					!strcmp(ABBREVIATION, "etc") ||
					!strcmp(ABBREVIATION, "st") ||
					!strcmp(ABBREVIATION, "jr") ||
					!strcmp(ABBREVIATION, "prof") ||
					!strcmp(ABBREVIATION, "approx")
				)
			)
				CUT = 1;
		}
		else if (
			!IN_QUOTE &&
			CHARACTER == '-' &&
			INDEX > 0 &&
			TEXT[INDEX - 1] == ' ' &&
			TEXT[INDEX + 1] == ' '
		)
			CUT = 1;

		if (!CUT)
		{
			if (POSITION < 598)
				SENTENCES[COUNT][POSITION++] = CHARACTER;

			continue ;
		}

		if (
			CHARACTER &&
			CHARACTER != '\n' &&
			CHARACTER != '-' &&
			POSITION < 598
		)
			SENTENCES[COUNT][POSITION++] = CHARACTER;

		SENTENCES[COUNT][POSITION] = 0;
		TRIM(SENTENCES[COUNT]);

		if (SENTENCES[COUNT][0] && COUNT < MAXIMUM - 1)
			COUNT++;

		POSITION = 0;
		IN_QUOTE = 0;

		if (!CHARACTER)
			break ;
	}

	return (COUNT);
}

TAUGHT_STORE
	*TAUGHT_NEW(void)
{
	TAUGHT_STORE	*STORE = calloc(1, sizeof *STORE);

	if (!STORE)
		return (NULL);

	STORE->LAST_SAVED = STORE->LAST_USED = STORE->PENDING = -1;

	return (STORE);
}

void
	TAUGHT_FREE(TAUGHT_STORE *STORE)
{
	if (!STORE)
		return ;

	free(STORE->FACTS);
	free(STORE);
}

static int
	ADD_FACT(TAUGHT_STORE *STORE, const TAUGHT_FACT *FACT)
{
	if (STORE->COUNT >= STORE->CAPACITY)
	{
		int	NEW_CAPACITY;

		if (STORE->CAPACITY)
			NEW_CAPACITY = STORE->CAPACITY * 2;
		else
			NEW_CAPACITY = 64;

		TAUGHT_FACT	*GROWN =
			realloc(STORE->FACTS, NEW_CAPACITY * sizeof *GROWN);

		if (!GROWN)
			return (-1);

		STORE->FACTS = GROWN;
		STORE->CAPACITY = NEW_CAPACITY;
	}

	STORE->FACTS[STORE->COUNT] = *FACT;
	STORE->IS_DIRTY = 1;

	return (STORE->COUNT++);
}

/* tabs and new lines never go into the file's fields */
static void
	FIELD_CLEAN(char *TEXT)
{
	for (; *TEXT; TEXT++)
		if (*TEXT == '\t' || *TEXT == '\n' || *TEXT == '\r')
			*TEXT = ' ';
}

int
	TAUGHT_LOAD(TAUGHT_STORE *STORE, const char *PATH)
{
	snprintf(STORE->PATH, sizeof STORE->PATH, "%s", PATH);

	FILE	*STREAM = fopen(PATH, "rb");

	if (!STREAM)
		return (0);

	char	LINE[2048];

	while (fgets(LINE, sizeof LINE, STREAM))
	{
		if (LINE[0] == '#' || LINE[0] == '\n' || LINE[0] == '\r')
			continue ;

		char		*FIELDS[8];
		int			FIELD_COUNT = 0;
		char		*CURSOR = LINE;
		TAUGHT_FACT	FACT;

		LINE[strcspn(LINE, "\r\n")] = 0;

		while (FIELD_COUNT < 8)
		{
			FIELDS[FIELD_COUNT++] = CURSOR;
			CURSOR = strchr(CURSOR, '\t');

			if (!CURSOR)
				break ;

			*CURSOR++ = 0;
		}

		if (FIELD_COUNT < 5)
			continue ;

		memset(&FACT, 0, sizeof FACT);

		if (!strcmp(FIELDS[0], "disputed"))
			FACT.STATUS = TAUGHT_DISPUTED;
		else if (!strncmp(FIELDS[0], "replaced", 8))
			FACT.STATUS = TAUGHT_REPLACED;
		else
			FACT.STATUS = TAUGHT_BELIEVED;

		FACT.REPLACED_BY = -1;

		if (FACT.STATUS == TAUGHT_REPLACED && FIELDS[0][8] == ':')
			FACT.REPLACED_BY = atoi(FIELDS[0] + 9);

		FACT.TIME = atoll(FIELDS[1]);
		snprintf(FACT.TEACHER, sizeof FACT.TEACHER, "%s", FIELDS[2]);
		snprintf(FACT.KEY, sizeof FACT.KEY, "%s", FIELDS[3]);
		snprintf(FACT.TEXT, sizeof FACT.TEXT, "%s", FIELDS[4]);

		if (FIELD_COUNT > 5)
			snprintf(FACT.EVIDENCE, sizeof FACT.EVIDENCE, "%s", FIELDS[5]);

		ADD_FACT(STORE, &FACT);
	}

	fclose(STREAM);
	STORE->IS_DIRTY = 0;

	return (STORE->COUNT);
}

int
	TAUGHT_SAVE(TAUGHT_STORE *STORE)
{
	if (!STORE->PATH[0])
		return (0);

	char	TEMPORARY_PATH[300];

	snprintf(TEMPORARY_PATH, sizeof TEMPORARY_PATH, "%s.tmp", STORE->PATH);

	FILE	*STREAM = fopen(TEMPORARY_PATH, "wb");

	if (!STREAM)
		return (0);

	fprintf(
		STREAM,
		"# What people taught me. One fact per line, fields separated by "
		"tabs:\n"
		"# status (believed, disputed, replaced:<line>), time, who taught it, "
		"what it defines, the fact, its evidence\n"
	);

	int	INDEX;

	for (INDEX = 0; INDEX < STORE->COUNT; INDEX++)
	{
		TAUGHT_FACT	*FACT = &STORE->FACTS[INDEX];
		char		STATUS[32];

		if (FACT->STATUS == TAUGHT_DISPUTED)
			snprintf(STATUS, sizeof STATUS, "disputed");
		else if (FACT->STATUS == TAUGHT_REPLACED)
			snprintf(STATUS, sizeof STATUS, "replaced:%d", FACT->REPLACED_BY);
		else
			snprintf(STATUS, sizeof STATUS, "believed");

		FIELD_CLEAN(FACT->TEACHER);
		FIELD_CLEAN(FACT->KEY);
		FIELD_CLEAN(FACT->TEXT);
		FIELD_CLEAN(FACT->EVIDENCE);
		fprintf(
			STREAM, "%s\t%lld\t%s\t%s\t%s\t%s\n", STATUS, FACT->TIME,
			FACT->TEACHER, FACT->KEY, FACT->TEXT, FACT->EVIDENCE
		);
	}

	fclose(STREAM);
	remove(STORE->PATH);

	if (rename(TEMPORARY_PATH, STORE->PATH))
		return (0);

	STORE->IS_DIRTY = 0;

	return (1);
}

static void
	DATE_OF(long long SECONDS, char *OUTPUT, int SIZE)
{
	static const char	*MONTHS[12] = {
		"January", "February", "March", "April", "May", "June", "July",
		"August", "September", "October", "November", "December"
	};
	time_t				WHEN = (time_t)SECONDS;
	struct tm			*PARTS = localtime(&WHEN);

	if (!PARTS || SECONDS <= 0)
	{
		OUTPUT[0] = 0;
		return ;
	}

	snprintf(
		OUTPUT, SIZE, "%s %d, %d", MONTHS[PARTS->tm_mon], PARTS->tm_mday,
		PARTS->tm_year + 1900
	);
}

/* "Sylwia told me on October 5, 2026, from: the Nature news website." */
int
	TAUGHT_SOURCE(
		TAUGHT_STORE *STORE, int FACT_INDEX, char *OUTPUT, int OUTPUT_SIZE
	)
{
	if (FACT_INDEX < 0 || FACT_INDEX >= STORE->COUNT)
		return (0);

	TAUGHT_FACT	*FACT = &STORE->FACTS[FACT_INDEX];
	char		DATE[64];
	char		WHO[64];

	DATE_OF(FACT->TIME, DATE, sizeof DATE);

	/* studied from a file: the file is the source */
	if (!strncmp(FACT->TEACHER, "the file ", 9))
	{
		snprintf(
			OUTPUT, OUTPUT_SIZE, "I read that in %s%s%s.", FACT->TEACHER,
			DATE[0] ? " on " : "", DATE
		);
		return (1);
	}

	TAUGHT_WHO(FACT->TEACHER, WHO, sizeof WHO);

	char	EVIDENCE[400];
	size_t	LENGTH;

	snprintf(EVIDENCE, sizeof EVIDENCE, "%s", FACT->EVIDENCE);
	LENGTH = strlen(EVIDENCE);

	while (
		LENGTH &&
		(EVIDENCE[LENGTH - 1] == '.' || EVIDENCE[LENGTH - 1] == ' ')
	)
		EVIDENCE[--LENGTH] = 0;

	if (EVIDENCE[0])
		snprintf(
			OUTPUT, OUTPUT_SIZE, "%s told me that%s%s, and said: \"%s.\"", WHO,
			DATE[0] ? " on " : "", DATE, EVIDENCE
		);
	else
		snprintf(
			OUTPUT, OUTPUT_SIZE,
			"%s told me that%s%s. No source was given for it.", WHO,
			DATE[0] ? " on " : "", DATE
		);

	return (1);
}

/* facts that cannot both be true: the same thing said with and without "not",
 * other numbers for the same thing, or another value for one specific thing */
static int
	CONTRADICTS(
		const TAUGHT_FACT *OLD, const char *TEXT, const char *KEY,
		int IS_SPECIFIC
	)
{
	char	OLD_CONTENT[1200];
	char	NEW_CONTENT[1200];

	if (HAS_SECOND_PERSON(OLD->TEXT) || HAS_SECOND_PERSON(TEXT))
		return (0);

	CONTENT_OF(OLD->TEXT, OLD_CONTENT, sizeof OLD_CONTENT, 0);
	CONTENT_OF(TEXT, NEW_CONTENT, sizeof NEW_CONTENT, 0);

	int	OLD_COUNT = COUNT_WORDS(OLD_CONTENT);
	int	NEW_COUNT = COUNT_WORDS(NEW_CONTENT);
	int	SHARED = SHARED_WORDS(NEW_CONTENT, OLD_CONTENT);
	int	SMALLER;

	if (OLD_COUNT < NEW_COUNT)
		SMALLER = OLD_COUNT;
	else
		SMALLER = NEW_COUNT;

	if (SMALLER >= 2 && SHARED * 4 >= SMALLER * 3)
	{
		if (HAS_NEGATION(OLD->TEXT) != HAS_NEGATION(TEXT))
			return (1);

		char	OLD_NUMBERS[200];
		char	NEW_NUMBERS[200];

		NUMBERS_OF(OLD->TEXT, OLD_NUMBERS, sizeof OLD_NUMBERS);
		NUMBERS_OF(TEXT, NEW_NUMBERS, sizeof NEW_NUMBERS);

		if (
			COUNT_WORDS(OLD_NUMBERS) &&
			COUNT_WORDS(NEW_NUMBERS) &&
			strcmp(OLD_NUMBERS, NEW_NUMBERS)
		)
			return (1);
	}

	if (IS_SPECIFIC && KEY[0] && OLD->KEY[0] && !strcmp(OLD->KEY, KEY))
	{
		/* "The capital of Zorbia is Blip" against "... is Flam" */
		if (SHARED < NEW_COUNT)
			return (1);
	}

	return (0);
}

static int
	SAME_FACT(const TAUGHT_FACT *OLD, const char *TEXT)
{
	char	OLD_NORMALIZED[1200];
	char	NEW_NORMALIZED[1200];

	NORMALIZE(OLD->TEXT, OLD_NORMALIZED, sizeof OLD_NORMALIZED);
	NORMALIZE(TEXT, NEW_NORMALIZED, sizeof NEW_NORMALIZED);

	if (!strcmp(OLD_NORMALIZED, NEW_NORMALIZED))
		return (1);

	char	OLD_CONTENT[1200];
	char	NEW_CONTENT[1200];

	CONTENT_OF(OLD->TEXT, OLD_CONTENT, sizeof OLD_CONTENT, 1);
	CONTENT_OF(TEXT, NEW_CONTENT, sizeof NEW_CONTENT, 1);

	int	OLD_COUNT = COUNT_WORDS(OLD_CONTENT);
	int	NEW_COUNT = COUNT_WORDS(NEW_CONTENT);

	return (
		OLD_COUNT == NEW_COUNT &&
		SHARED_WORDS(NEW_CONTENT, OLD_CONTENT) == NEW_COUNT &&
		HAS_NEGATION(OLD->TEXT) == HAS_NEGATION(TEXT)
	);
}

/* "A name is a word ..." -> 1 and "name" */
int
	TAUGHT_DEFINES(const char *SENTENCE, char *KEY, int KEY_SIZE)
{
	char	SUBJECT[120];
	int		IS_SPECIFIC = 0;

	return (
		DEFINITION_KEY(
			SKIP_FILLERS(SENTENCE), KEY, KEY_SIZE, SUBJECT, sizeof SUBJECT,
			&IS_SPECIFIC
		) &&
		KEY[0]
	);
}

/* "forget Eli likes pizza": the engine forgot it, so it is no longer believed
 * here either */
int
	TAUGHT_FORGET(TAUGHT_STORE *STORE, const char *TEXT)
{
	int	INDEX;
	int	COUNT = 0;

	if (!STORE || !TEXT || !*TEXT)
		return (0);

	for (INDEX = 0; INDEX < STORE->COUNT; INDEX++)
	{
		TAUGHT_FACT	*FACT = &STORE->FACTS[INDEX];

		if (FACT->STATUS == TAUGHT_REPLACED || !SAME_FACT(FACT, TEXT))
			continue ;

		FACT->STATUS = TAUGHT_REPLACED;
		FACT->REPLACED_BY = -1;
		COUNT++;
	}

	if (COUNT)
		STORE->IS_DIRTY = 1;

	return (COUNT);
}

static void
	QUOTE_FACT(const TAUGHT_FACT *FACT, char *OUTPUT, int SIZE)
{
	char	TEXT[480];

	snprintf(TEXT, sizeof TEXT, "%s", FACT->TEXT);

	size_t	LENGTH = strlen(TEXT);

	while (LENGTH && (TEXT[LENGTH - 1] == '.' || TEXT[LENGTH - 1] == '!'))
		TEXT[--LENGTH] = 0;

	if (
		TEXT[0] &&
		isupper((unsigned char)TEXT[0]) &&
		!isupper((unsigned char)TEXT[1])
	)
		TEXT[0] = (char)tolower((unsigned char)TEXT[0]);

	snprintf(OUTPUT, SIZE, "%s", TEXT);
}

/* who taught it, ready to start a sentence: "Sylwia", "The file notes.txt",
 * "Someone" */
void
	TAUGHT_WHO(const char *TEACHER, char *OUTPUT, int SIZE)
{
	if (TEACHER[0] && strcmp(TEACHER, "someone"))
		snprintf(OUTPUT, SIZE, "%s", TEACHER);
	else
		snprintf(OUTPUT, SIZE, "Someone");

	OUTPUT[0] = (char)toupper((unsigned char)OUTPUT[0]);
}

static void
	TEACHER_NAME(const char *TEACHER, char *OUTPUT, int SIZE)
{
	TAUGHT_WHO(TEACHER, OUTPUT, SIZE);
}

/* when the fact that lost had a source too, Lucy says so: two sources
 * disagree, and the one she gave up is named */
static void
	ADD_LOST_SOURCE(
		const TAUGHT_STORE *STORE, int LOST_INDEX, char *SAY, int SIZE
	)
{
	const TAUGHT_FACT	*LOST;
	char				WHO[64];
	char				EVIDENCE[400];
	size_t				LENGTH;
	size_t				HAVE = strlen(SAY);

	if (LOST_INDEX < 0 || LOST_INDEX >= STORE->COUNT)
		return ;

	LOST = &STORE->FACTS[LOST_INDEX];

	if (!LOST->EVIDENCE[0])
		return ;

	TAUGHT_WHO(LOST->TEACHER, WHO, sizeof WHO);
	snprintf(EVIDENCE, sizeof EVIDENCE, "%s", LOST->EVIDENCE);
	LENGTH = strlen(EVIDENCE);

	while (
		LENGTH &&
		(EVIDENCE[LENGTH - 1] == '.' || EVIDENCE[LENGTH - 1] == ' ')
	)
		EVIDENCE[--LENGTH] = 0;

	snprintf(
		SAY + HAVE, SIZE - (int)HAVE,
		" But %s had a source too (\"%s\"), so the two sources disagree. If "
		"you find out which one is right, tell me.",
		WHO, EVIDENCE
	);
}

/* the dispute said out loud: what was taught before, its evidence, and the
 * question back */
static void
	SAY_DISPUTE(
		const TAUGHT_FACT *OLD, const char *NEW_TEXT, int REPEATED,
		char *OUTPUT, int SIZE
	)
{
	char	OLD_QUOTE[480];
	char	NEW_QUOTE[480];
	char	WHO[64];
	char	EVIDENCE[400];
	size_t	LENGTH;

	QUOTE_FACT(OLD, OLD_QUOTE, sizeof OLD_QUOTE);

	{
		TAUGHT_FACT	TEMPORARY;

		memset(&TEMPORARY, 0, sizeof TEMPORARY);
		snprintf(TEMPORARY.TEXT, sizeof TEMPORARY.TEXT, "%s", NEW_TEXT);
		QUOTE_FACT(&TEMPORARY, NEW_QUOTE, sizeof NEW_QUOTE);
	}

	TEACHER_NAME(OLD->TEACHER, WHO, sizeof WHO);
	snprintf(EVIDENCE, sizeof EVIDENCE, "%s", OLD->EVIDENCE);
	LENGTH = strlen(EVIDENCE);

	while (
		LENGTH &&
		(EVIDENCE[LENGTH - 1] == '.' || EVIDENCE[LENGTH - 1] == ' ')
	)
		EVIDENCE[--LENGTH] = 0;

	if (REPEATED)
		snprintf(
			OUTPUT, SIZE,
			"You still haven't shown me where that comes from. %s taught me "
			"that %s. How do you know?",
			WHO, OLD_QUOTE
		);
	else if (EVIDENCE[0])
		snprintf(
			OUTPUT, SIZE,
			"I can't agree yet. %s taught me that %s, and said: \"%s.\" Then "
			"what about that? How do you know that %s?",
			WHO, OLD_QUOTE, EVIDENCE, NEW_QUOTE
		);
	else
		snprintf(
			OUTPUT, SIZE,
			"I can't agree yet. %s taught me that %s. How do you know that %s? "
			"If you show me where it comes from, I will believe you.",
			WHO, OLD_QUOTE, NEW_QUOTE
		);
}

/* "it" at the start of a sentence after a definition in the same message */
static int
	PRONOUN_SUBJECT(const char *SENTENCE, int *WORD_LENGTH)
{
	static const char	*VERBS_AFTER[70] = {
		"is", "was", "are", "were", "means", "meant", "has", "had", "have",
		"can", "could", "will", "would", "does", "did", "do", "makes", "made",
		"shows", "helps", "happens", "happen", "comes", "come", "goes", "go",
		"may", "might", "must", "should", "refers", "stands", "became",
		"becomes", "seems", "looks", "lives", "live", "eats", "eat", "uses",
		"use", "used", "gives", "give", "takes", "take", "needs", "need",
		"contains", "include", "includes", "describes", "explains", "says",
		"tells", "isn't", "wasn't", "aren't", "doesn't", "don't", "can't",
		"also", "usually", "often", "always", "never", "only", "just", NULL
	};
	char				WORD[32];
	char				NEXT[32];
	const char			*AFTER = SENTENCE;

	FIRST_WORD(SENTENCE, WORD, sizeof WORD);

	if (!IN_LIST(WORD, PRONOUN_STARTS))
		return (0);

	/* "This new dinosaur was found": "this" points at a noun, it is not the
	 * subject on its own */
	if (!strcmp(WORD, "this"))
	{
		while (*AFTER && !isalnum((unsigned char)*AFTER))
			AFTER++;

		while (*AFTER && (isalnum((unsigned char)*AFTER) || *AFTER == '\''))
			AFTER++;

		FIRST_WORD(AFTER, NEXT, sizeof NEXT);

		if (!IN_LIST(NEXT, VERBS_AFTER))
			return (0);
	}

	*WORD_LENGTH = (int)strlen(WORD);

	return (1);
}

static char	LAST_SUBJECT[120];
static int	LAST_SUBJECT_SERIAL = -1;

/* a sentence with no finite verb, only an -ing word in its first three
 * words: a fragment, not a fact */
static int
	IS_FRAGMENT(const char *TEXT)
{
	static const char	*FINITE[32] = {
		"is", "are", "was", "were", "am", "has", "have", "had", "do", "does",
		"did", "can", "could", "will", "would", "shall", "should", "may",
		"might", "must", "means", "mean", "isn't", "aren't", "wasn't",
		"weren't", "doesn't", "don't", "didn't", "can't", "won't", NULL
	};
	char				NORMALIZED[1200];
	char				*WORDS[160];
	int					COUNT;
	int					INDEX;
	int					EARLY_ING = 0;

	NORMALIZE(TEXT, NORMALIZED, sizeof NORMALIZED);
	COUNT = SPLIT_WORDS(NORMALIZED, WORDS, 160);

	for (INDEX = 0; INDEX < COUNT; INDEX++)
	{
		size_t	LENGTH = strlen(WORDS[INDEX]);

		if (IN_LIST(WORDS[INDEX], FINITE))
			return (0);

		if (
			INDEX < 3 &&
			LENGTH > 4 &&
			!strcmp(WORDS[INDEX] + LENGTH - 3, "ing")
		)
			EARLY_ING = 1;
	}

	return (EARLY_ING);
}

/* an order, a sum, a remark about the chat or a file: things said to Lucy,
 * not facts about the world.  "Code, read che2.", "Chapter1 is done, move it to
 * final.", "Seven plus six is fourteen.", "Wow, that's amazing!" */
static int
	NOT_A_STATEMENT(const char *TEXT, int IS_DEFINITION)
{
	static const char	*ORDER_VERBS[118] = {
		"ask", "tell", "say", "give", "explain", "answer", "remember", "forget",
		"delete", "make", "create", "creat", "write", "move", "rename", "list",
		"show", "read", "open", "call", "check", "count", "reverse", "spell",
		"calculate", "compute", "add", "put", "stop", "wait", "keep", "type",
		"find", "search", "help", "describe", "define", "translate", "repeat",
		"note", "save", "run", "start", "close", "quit", "imagine", "pretend",
		"guess", "choose", "pick", "continue", "play", "finish", "turn", "set",
		"send", "remind", "bring", "get", "solve", "echo", "name", "organize",
		"organise", "recall", "remove", "erase", "export", "append", "memorize",
		"memorise", "buy", "stick", "throw", "restore", "take", "drop", "copy",
		"paste", "print", "sort", "clear", "empty", "fix", "update", "change",
		"replace", "insert", "load", "store", "label", "mark", "email", "text",
		"pause", "skip", "draw", "build", "cook", "order", "book", "schedule",
		"summarize", "summarise", "rewrite", "edit", "format", "convert",
		"compare", "multiply", "divide", "subtract", "reply", "respond",
		"learn", "use", "chose", NULL
	};
	static const char	*REMARK_STARTS[50] = {
		"aww", "aw", "wow", "whoa", "ooh", "oops", "yay", "nah", "nope", "yep",
		"yup", "nooo", "noo", "fine", "wrong", "correct", "hmm", "god", "damn",
		"sounds", "seems", "looks", "just", "only", "if", "their", "theirs",
		"good", "great", "nice", "cool", "awesome", "perfect", "exactly",
		"cute", "lovely", "sweet", "math", "formula", "story", "puzzle",
		"first", "next", "finally", "select", "round", "settle", "square", "yo",
		NULL
	};
	static const char	*SUM_WORDS[7] = {
		"plus", "minus", "times", "sum", "product", "percent", NULL
	};
	static const char	*FORMULA_WORDS[7] = {
		"divided", "multiplied", "squared", "cubed", "sqrt", "pi", NULL
	};
	static const char	*NUMBER_NAMES[21] = {
		"zero", "one", "two", "three", "four", "five", "six", "seven", "eight",
		"nine", "ten", "eleven", "twelve", "twenty", "thirty", "forty", "fifty",
		"hundred", "thousand", "million", NULL
	};
	static const char	*FILE_WORDS[7] = {
		"file", "files", "folder", "folders", "directory", "txt", NULL
	};
	char				LOWER[600];
	char				NORMALIZED[1200];
	char				*WORDS[160];
	char				WORD[40];
	char				SECOND[40];
	int					COUNT;
	int					INDEX;
	int					HAS_NUMBER = 0;
	int					HAS_SUM = 0;
	const char			*CURSOR;

	LOWER_COPY(TEXT, LOWER, sizeof LOWER);
	FIRST_WORD(LOWER, WORD, sizeof WORD);

	if (IN_LIST(WORD, ORDER_VERBS) || IN_LIST(WORD, REMARK_STARTS))
		return (1);

	/* "Lucy make a folder called x", "Sarah, make a file called b.txt": every
	 * clause after a comma or colon is checked for an order too */
	for (CURSOR = LOWER; *CURSOR; CURSOR++)
	{
		if (*CURSOR != ',' && *CURSOR != ':' && *CURSOR != ';')
			continue ;

		FIRST_WORD(CURSOR + 1, SECOND, sizeof SECOND);

		if (IN_LIST(SECOND, ORDER_VERBS))
			return (1);
	}

	CURSOR = LOWER;

	while (*CURSOR && !isalnum((unsigned char)*CURSOR))
		CURSOR++;

	while (*CURSOR && (isalnum((unsigned char)*CURSOR) || *CURSOR == '\''))
		CURSOR++;

	FIRST_WORD(CURSOR, SECOND, sizeof SECOND);

	if (
		(!strcmp(WORD, "code") || !strcmp(WORD, "lucy")) &&
		IN_LIST(SECOND, ORDER_VERBS)
	)
		return (1);

	/* file names belong to the file tools */
	for (CURSOR = LOWER; *CURSOR; CURSOR++)
		if (
			*CURSOR == '.' &&
			CURSOR > LOWER &&
			isalnum((unsigned char)CURSOR[-1]) &&
			isalpha((unsigned char)CURSOR[1]) &&
			isalpha((unsigned char)CURSOR[2])
		)
			return (1);

	/* sums are worked out, never taught: "one plus one is five" stays wrong */
	for (CURSOR = LOWER; *CURSOR; CURSOR++)
	{
		if (isdigit((unsigned char)*CURSOR))
			HAS_NUMBER = 1;

		if (
			strchr("+*/^=", *CURSOR) ||
			(*CURSOR == '-' && isdigit((unsigned char)CURSOR[1]))
		)
			HAS_SUM = 1;
	}

	NORMALIZE(TEXT, NORMALIZED, sizeof NORMALIZED);
	COUNT = SPLIT_WORDS(NORMALIZED, WORDS, 160);

	for (INDEX = 0; INDEX < COUNT; INDEX++)
	{
		if (IN_LIST(WORDS[INDEX], NUMBER_NAMES))
			HAS_NUMBER = 1;

		if (IN_LIST(WORDS[INDEX], SUM_WORDS))
			HAS_SUM = 1;

		/* "Gold bonus is pi times b squared": a formula for the math core */
		if (IN_LIST(WORDS[INDEX], FORMULA_WORDS))
			return (1);

		if (!IS_DEFINITION && IN_LIST(WORDS[INDEX], FILE_WORDS))
			return (1);
	}

	if (HAS_NUMBER && HAS_SUM)
		return (1);

	/* said to Lucy about Lucy or this chat ("Sounds like you have a lot of
	 * interests", "If you understand, say yes") */
	if (!IS_DEFINITION && HAS_SECOND_PERSON(TEXT))
		return (1);

	return (0);
}

/* a message written in texting slang ("settle a bet pls!! ... loser buys
 * dinner") is chat, not someone teaching: two slang words mark it (laughing,
 * "Haha! No, ... XD", is not slang: people laugh while they teach) */
void
	TAUGHT_NEW_MESSAGE(TAUGHT_STORE *STORE, const char *TEXT)
{
	static const char	*SLANG_WORDS[61] = {
		"tbh", "idk", "u", "ur", "pls", "plz", "fr", "rn", "btw", "imo", "ngl",
		"smh", "sus", "bruh", "yall", "ya", "kinda", "sorta", "gonna", "wanna",
		"gotta", "lemme", "dunno", "im", "bf", "gf", "tho", "thx", "tmrw", "hw",
		"thru", "cuz", "bc", "r", "q", "srsly", "jk", "ty", "k", "wat", "wut",
		"nvm", "ikr", "bday", "hes", "shes", "dont", "cant", "wont", "isnt",
		"thats", "whats", "theyre", "youre", "ive", "whens", "wheres", "hows",
		"whos", "wats", NULL
	};
	char				NORMALIZED[4096];
	char				*WORDS[600];
	int					COUNT;
	int					INDEX;
	int					SLANG;

	STORE->SERIAL++;
	STORE->CASUAL = 0;

	if (!TEXT)
		return ;

	NORMALIZE(TEXT, NORMALIZED, sizeof NORMALIZED);
	COUNT = SPLIT_WORDS(NORMALIZED, WORDS, 600);
	SLANG = 0;

	for (INDEX = 0; INDEX < COUNT; INDEX++)
		if (IN_LIST(WORDS[INDEX], SLANG_WORDS))
			SLANG++;

	STORE->CASUAL = SLANG >= 2;
}

int
	TAUGHT_HEAR(
		TAUGHT_STORE *STORE, const char *SENTENCE, const char *TEACHER,
		long long NOW, TAUGHT_HEARING *RESULT
	)
{
	char	CLEAN[600];
	char	EVIDENCE[400] = "";
	char	WORD[40];

	memset(RESULT, 0, sizeof *RESULT);
	RESULT->FACT_INDEX = RESULT->OTHER_INDEX = -1;

	if (!SENTENCE || !*SENTENCE)
		return (HEARD_NOTHING);

	snprintf(CLEAN, sizeof CLEAN, "%s", SKIP_FILLERS(SENTENCE));
	TRIM(CLEAN);

	/* "I am teaching you this again, a byte is 8 bits." / "Listen: ...":
	 * the lead-in is about the teaching, the fact comes after it */
	{
		static const char	*LEAD_WORDS[14] = {
			"teach", "teaching", "taught", "tell", "telling", "told", "explain",
			"explaining", "repeat", "repeating", "again", "listen", "remember",
			NULL
		};
		char				*BREAK = strstr(CLEAN, ", ");
		char				*COLON = strstr(CLEAN, ": ");
		char				LOWER_CLEAN[600];
		char				*YOU_THAT;

		if (!BREAK || (COLON && COLON < BREAK))
			BREAK = COLON;

		/* "I am telling you that dogs bark." */
		LOWER_COPY(CLEAN, LOWER_CLEAN, sizeof LOWER_CLEAN);
		YOU_THAT = strstr(LOWER_CLEAN, " you that ");

		if (YOU_THAT && YOU_THAT - LOWER_CLEAN < 60)
			BREAK = CLEAN + (YOU_THAT - LOWER_CLEAN) + 8;

		if (BREAK && BREAK - CLEAN < 80)
		{
			char	LEAD[100];
			char	NORMALIZED_LEAD[220];
			char	*WORDS[40];
			int		COUNT;
			int		INDEX;
			int		IS_LEAD = 0;

			snprintf(LEAD, sizeof LEAD, "%.*s", (int)(BREAK - CLEAN), CLEAN);
			NORMALIZE(LEAD, NORMALIZED_LEAD, sizeof NORMALIZED_LEAD);
			COUNT = SPLIT_WORDS(NORMALIZED_LEAD, WORDS, 40);

			for (INDEX = 0; INDEX < COUNT; INDEX++)
				if (IN_LIST(WORDS[INDEX], LEAD_WORDS))
					IS_LEAD = 1;

			/* only the speaker's own lead-in ("I am teaching you ...", "Let
			 * me tell you, ..."), never a request ("could you tell me X,
			 * ...") or an order ("remember my list: ...") */
			{
				static const char	*OWN_STARTS[11] = {
					"i", "i'm", "im", "let", "listen", "and", "so", "now",
					"okay", "ok", NULL
				};

				if (
					!COUNT ||
					!IN_LIST(WORDS[0], OWN_STARTS) ||
					strstr(NORMALIZED_LEAD, " could you ") ||
					strstr(NORMALIZED_LEAD, " can you ") ||
					strstr(NORMALIZED_LEAD, " would you ") ||
					strstr(NORMALIZED_LEAD, " tell me ") ||
					strstr(NORMALIZED_LEAD, " please ") ||
					strchr(BREAK + 2, ',')
				)
					IS_LEAD = 0;
			}

			if (IS_LEAD)
			{
				memmove(CLEAN, BREAK + 2, strlen(BREAK + 2) + 1);
				TRIM(CLEAN);
			}
		}
	}

	/* "remember this: a byte is 8 bits" teaches what follows */
	{
		static const char	*PREFIXES[12] = {
			"remember this:", "remember that:", "remember:", "memorize this:",
			"memorise this:", "note:", "fact:", "learn this:", "keep in mind:",
			"remember this ", "remember that ", NULL
		};
		char				LOWER[600];
		int					INDEX;

		LOWER_COPY(CLEAN, LOWER, sizeof LOWER);

		for (INDEX = 0; PREFIXES[INDEX]; INDEX++)
			if (!strncmp(LOWER, PREFIXES[INDEX], strlen(PREFIXES[INDEX])))
			{
				memmove(
					CLEAN, CLEAN + strlen(PREFIXES[INDEX]),
					strlen(CLEAN + strlen(PREFIXES[INDEX])) + 1
				);
				TRIM(CLEAN);
				break ;
			}
	}

	if (strlen(CLEAN) < 6)
		return (HEARD_NOTHING);

	if (LAST_SUBJECT_SERIAL != STORE->SERIAL)
		LAST_SUBJECT[0] = 0;

	int	HAS_EVIDENCE = EVIDENCE_IN(CLEAN, EVIDENCE, sizeof EVIDENCE);

	/* evidence on its own: it supports what this person said last */
	if (HAS_EVIDENCE && EVIDENCE_ONLY(CLEAN) && !ENDS_WITH_QUESTION(CLEAN))
	{
		int	TARGET = -1;

		if (
			STORE->PENDING >= 0 &&
			STORE->PENDING < STORE->COUNT &&
			!strcmp(STORE->FACTS[STORE->PENDING].TEACHER, TEACHER)
		)
			TARGET = STORE->PENDING;
		else if (
			STORE->LAST_SAVED >= 0 &&
			STORE->LAST_SAVED < STORE->COUNT &&
			!strcmp(STORE->FACTS[STORE->LAST_SAVED].TEACHER, TEACHER) &&
			STORE->FACTS[STORE->LAST_SAVED].SERIAL >= STORE->SERIAL - 1
		)
			TARGET = STORE->LAST_SAVED;

		if (TARGET < 0)
			return (HEARD_NOTHING);

		TAUGHT_FACT	*FACT = &STORE->FACTS[TARGET];
		int			OTHER;

		/* every fact from that same message gets the support */
		for (OTHER = 0; OTHER < STORE->COUNT; OTHER++)
		{
			TAUGHT_FACT	*SAME_MESSAGE = &STORE->FACTS[OTHER];
			size_t		HAVE = strlen(SAME_MESSAGE->EVIDENCE);

			if (
				OTHER != TARGET &&
				(
					SAME_MESSAGE->SERIAL != FACT->SERIAL ||
					SAME_MESSAGE->SERIAL <= 0 ||
					strcmp(SAME_MESSAGE->TEACHER, FACT->TEACHER) ||
					SAME_MESSAGE->STATUS == TAUGHT_REPLACED
				)
			)
				continue ;

			if (strstr(SAME_MESSAGE->EVIDENCE, CLEAN))
				continue ;

			snprintf(
				SAME_MESSAGE->EVIDENCE + HAVE,
				sizeof SAME_MESSAGE->EVIDENCE - HAVE, "%s%s", HAVE ? "; " : "",
				CLEAN
			);
			TRIM(SAME_MESSAGE->EVIDENCE);
		}

		STORE->IS_DIRTY = 1;
		RESULT->FACT_INDEX = TARGET;

		if (FACT->STATUS == TAUGHT_DISPUTED)
		{
			int	INDEX;

			FACT->STATUS = TAUGHT_BELIEVED;
			STORE->PENDING = -1;

			for (INDEX = 0; INDEX < STORE->COUNT; INDEX++)
				if (
					INDEX != TARGET &&
					STORE->FACTS[INDEX].STATUS == TAUGHT_BELIEVED &&
					CONTRADICTS(&STORE->FACTS[INDEX], FACT->TEXT, FACT->KEY, 0)
				)
				{
					STORE->FACTS[INDEX].STATUS = TAUGHT_REPLACED;
					STORE->FACTS[INDEX].REPLACED_BY = TARGET;
					RESULT->OTHER_INDEX = INDEX;
				}

			char	QUOTE[480];

			QUOTE_FACT(FACT, QUOTE, sizeof QUOTE);
			snprintf(
				RESULT->SAY, sizeof RESULT->SAY,
				"Okay, you showed me where it comes from: \"%s\" I believe "
				"you now: %s.",
				CLEAN, QUOTE
			);
			ADD_LOST_SOURCE(
				STORE, RESULT->OTHER_INDEX, RESULT->SAY, sizeof RESULT->SAY
			);
			STORE->LAST_SAVED = TARGET;
			return (RESULT->KIND = HEARD_ACCEPTED);
		}

		return (RESULT->KIND = HEARD_EVIDENCE);
	}

	if (ENDS_WITH_QUESTION(CLEAN))
		return (HEARD_NOTHING);

	FIRST_WORD(CLEAN, WORD, sizeof WORD);

	char	EARLY_KEY[80];
	char	EARLY_SUBJECT[120];
	int		EARLY_SPECIFIC;
	int		IS_DEFINITION = DEFINITION_KEY(
		CLEAN, EARLY_KEY, sizeof EARLY_KEY, EARLY_SUBJECT, sizeof EARLY_SUBJECT,
		&EARLY_SPECIFIC
	);

	if (
		!IS_DEFINITION &&
		(
			IN_LIST(WORD, QUESTION_STARTS) ||
			IN_LIST(WORD, COMMAND_STARTS) ||
			IN_LIST(WORD, PERSONAL_STARTS)
		)
	)
		return (HEARD_NOTHING);

	if (
		!IS_DEFINITION &&
		(
			!strcmp(WORD, "so") ||
			!strcmp(WORD, "and") ||
			!strcmp(WORD, "but") ||
			!strcmp(WORD, "then") ||
			!strcmp(WORD, "now")
		)
	)
	{
		char		SECOND[40];
		const char	*AFTER = CLEAN;

		while (*AFTER && !isalnum((unsigned char)*AFTER))
			AFTER++;

		while (*AFTER && (isalnum((unsigned char)*AFTER) || *AFTER == '\''))
			AFTER++;

		FIRST_WORD(AFTER, SECOND, sizeof SECOND);

		if (
			IN_LIST(SECOND, QUESTION_STARTS) ||
			IN_LIST(SECOND, COMMAND_STARTS) ||
			IN_LIST(SECOND, PERSONAL_STARTS) ||
			IN_LIST(SECOND, PRONOUN_STARTS)
		)
			return (HEARD_NOTHING);
	}

	/* "This new dinosaur was found in China" -> "The new dinosaur ...": the
	 * fact stands on its own once the message is gone */
	if (!strcmp(WORD, "this") || !strcmp(WORD, "these"))
	{
		int	SKIP = 0;

		if (!PRONOUN_SUBJECT(CLEAN, &SKIP))
		{
			char		REWRITTEN[600];
			const char	*REST = CLEAN;

			while (*REST && !isalnum((unsigned char)*REST))
				REST++;

			REST += strlen(WORD);
			snprintf(REWRITTEN, sizeof REWRITTEN, "The%s", REST);
			snprintf(CLEAN, sizeof CLEAN, "%s", REWRITTEN);
			snprintf(WORD, sizeof WORD, "the");
		}
	}

	/* "it's used as an invitation" right after "Let's is a contraction ..." */
	int	PRONOUN_LENGTH = 0;

	if (PRONOUN_SUBJECT(CLEAN, &PRONOUN_LENGTH))
	{
		if (!LAST_SUBJECT[0])
			return (HEARD_NOTHING);

		char		REWRITTEN[600];
		const char	*REST = CLEAN;

		while (*REST && !isalnum((unsigned char)*REST))
			REST++;

		REST += PRONOUN_LENGTH;

		const char	*VERB = "";

		if (!strcmp(WORD, "it's"))
			VERB = " is";
		else if (!strcmp(WORD, "they're"))
			VERB = " are";

		snprintf(
			REWRITTEN, sizeof REWRITTEN, "%s%s%s", LAST_SUBJECT, VERB, REST
		);
		snprintf(CLEAN, sizeof CLEAN, "%s", REWRITTEN);
	}

	char	CONTENT[1200];

	CONTENT_OF(CLEAN, CONTENT, sizeof CONTENT, 1);

	/* "A byte has 8 bits.": two words and a number are enough */
	{
		int	CONTENT_COUNT = COUNT_WORDS(CONTENT);
		int	HAS_DIGIT = 0;
		int	INDEX;

		for (INDEX = 0; CLEAN[INDEX]; INDEX++)
			if (isdigit((unsigned char)CLEAN[INDEX]))
				HAS_DIGIT = 1;

		if (
			CONTENT_COUNT < 3 &&
			!(CONTENT_COUNT >= 2 && (HAS_DIGIT || IS_DEFINITION))
		)
			return (HEARD_NOTHING);
	}

	/* "Sometimes being confusing but totally understandable.": no verb that
	 * says anything, only a word in -ing near the start */
	if (IS_FRAGMENT(CLEAN))
		return (HEARD_NOTHING);

	/* a sentence about the speaker or this chat ("..., and I asked ...") is not
	 * a fact about the world */
	if (HAS_FIRST_PERSON(CLEAN) || IS_MOMENT_BOUND(CLEAN) || IS_CHATTY(CLEAN))
		return (HEARD_NOTHING);

	/* "Since every human year is 7 dog years.": half a sentence; "Finish the
	 * story from before!!": a shout, not a fact; a slangy message: chat */
	static const char	*CLAUSE_STARTS[9] = {
		"since", "because", "although", "though", "unless", "cause", "cuz",
		"whereas", NULL
	};

	if (IN_LIST(WORD, CLAUSE_STARTS) || NOT_A_STATEMENT(CLEAN, IS_DEFINITION))
		return (HEARD_NOTHING);

	if (!IS_DEFINITION && (strstr(SENTENCE, "!!") || STORE->CASUAL))
		return (HEARD_NOTHING);

	TAUGHT_FACT	FACT;
	char		SUBJECT[120];
	int			IS_SPECIFIC = 0;

	memset(&FACT, 0, sizeof FACT);
	FACT.REPLACED_BY = -1;
	DEFINITION_KEY(
		CLEAN, FACT.KEY, sizeof FACT.KEY, SUBJECT, sizeof SUBJECT, &IS_SPECIFIC
	);

	if (FACT.KEY[0])
	{
		snprintf(RESULT->SUBJECT, sizeof RESULT->SUBJECT, "%s", SUBJECT);
		snprintf(LAST_SUBJECT, sizeof LAST_SUBJECT, "%s", SUBJECT);
		LAST_SUBJECT_SERIAL = STORE->SERIAL;
	}

	snprintf(FACT.TEXT, sizeof FACT.TEXT, "%s", CLEAN);

	if (FACT.TEXT[0] && islower((unsigned char)FACT.TEXT[0]))
		FACT.TEXT[0] = (char)toupper((unsigned char)FACT.TEXT[0]);

	{
		size_t	LENGTH = strlen(FACT.TEXT);

		if (
			LENGTH &&
			LENGTH + 1 < sizeof FACT.TEXT &&
			FACT.TEXT[LENGTH - 1] != '.' &&
			FACT.TEXT[LENGTH - 1] != '!' &&
			FACT.TEXT[LENGTH - 1] != '"'
		)
		{
			FACT.TEXT[LENGTH] = '.';
			FACT.TEXT[LENGTH + 1] = 0;
		}
	}

	snprintf(
		FACT.TEACHER, sizeof FACT.TEACHER, "%s",
		TEACHER && TEACHER[0] ? TEACHER : "someone"
	);
	FACT.TIME = NOW;
	FACT.SERIAL = STORE->SERIAL;

	if (HAS_EVIDENCE)
		snprintf(FACT.EVIDENCE, sizeof FACT.EVIDENCE, "%s", EVIDENCE);

	int	INDEX;

	/* already known */
	for (INDEX = 0; INDEX < STORE->COUNT; INDEX++)
	{
		TAUGHT_FACT	*OLD = &STORE->FACTS[INDEX];

		if (OLD->STATUS == TAUGHT_REPLACED || !SAME_FACT(OLD, FACT.TEXT))
			continue ;

		RESULT->FACT_INDEX = INDEX;

		if (HAS_EVIDENCE && !strstr(OLD->EVIDENCE, EVIDENCE))
		{
			size_t	HAVE = strlen(OLD->EVIDENCE);

			snprintf(
				OLD->EVIDENCE + HAVE, sizeof OLD->EVIDENCE - HAVE, "%s%s",
				HAVE ? "; " : "", EVIDENCE
			);
			STORE->IS_DIRTY = 1;
		}

		if (
			OLD->STATUS == TAUGHT_DISPUTED &&
			strcmp(OLD->TEACHER, FACT.TEACHER)
		)
		{
			/* someone else says the same disputed thing: still no proof */
		}

		if (
			OLD->STATUS == TAUGHT_DISPUTED &&
			!strcmp(OLD->TEACHER, FACT.TEACHER)
		)
		{
			int	OTHER;

			if (HAS_EVIDENCE)
			{
				OLD->STATUS = TAUGHT_BELIEVED;
				STORE->PENDING = -1;

				for (OTHER = 0; OTHER < STORE->COUNT; OTHER++)
					if (
						OTHER != INDEX &&
						STORE->FACTS[OTHER].STATUS == TAUGHT_BELIEVED &&
						CONTRADICTS(
							&STORE->FACTS[OTHER], OLD->TEXT, OLD->KEY, 0
						)
					)
					{
						STORE->FACTS[OTHER].STATUS = TAUGHT_REPLACED;
						STORE->FACTS[OTHER].REPLACED_BY = INDEX;
						RESULT->OTHER_INDEX = OTHER;
					}

				char	QUOTE[480];

				QUOTE_FACT(OLD, QUOTE, sizeof QUOTE);
				snprintf(
					RESULT->SAY, sizeof RESULT->SAY,
					"Okay, you showed me where it comes from: \"%s.\" I "
					"believe you now: %s.",
					EVIDENCE, QUOTE
				);
				ADD_LOST_SOURCE(
					STORE, RESULT->OTHER_INDEX, RESULT->SAY, sizeof RESULT->SAY
				);
				STORE->LAST_SAVED = INDEX;
				return (RESULT->KIND = HEARD_ACCEPTED);
			}

			for (OTHER = 0; OTHER < STORE->COUNT; OTHER++)
				if (
					STORE->FACTS[OTHER].STATUS == TAUGHT_BELIEVED &&
					CONTRADICTS(&STORE->FACTS[OTHER], OLD->TEXT, OLD->KEY, 0)
				)
				{
					RESULT->OTHER_INDEX = OTHER;
					SAY_DISPUTE(
						&STORE->FACTS[OTHER], OLD->TEXT, 1, RESULT->SAY,
						sizeof RESULT->SAY
					);
					STORE->PENDING = INDEX;
					return (RESULT->KIND = HEARD_DISPUTED);
				}
		}

		STORE->LAST_SAVED = INDEX;
		return (RESULT->KIND = HEARD_KNEW);
	}

	/* against what was taught before */
	for (INDEX = 0; INDEX < STORE->COUNT; INDEX++)
	{
		TAUGHT_FACT	*OLD = &STORE->FACTS[INDEX];

		if (OLD->STATUS != TAUGHT_BELIEVED)
			continue ;

		if (!CONTRADICTS(OLD, FACT.TEXT, FACT.KEY, IS_SPECIFIC))
			continue ;

		RESULT->OTHER_INDEX = INDEX;

		if (!strcmp(OLD->TEACHER, FACT.TEACHER))
		{
			/* the same person corrects themselves */
			int	NEW_INDEX = ADD_FACT(STORE, &FACT);

			if (NEW_INDEX < 0)
				return (HEARD_NOTHING);

			STORE->FACTS[INDEX].STATUS = TAUGHT_REPLACED;
			STORE->FACTS[INDEX].REPLACED_BY = NEW_INDEX;
			STORE->LAST_SAVED = RESULT->FACT_INDEX = NEW_INDEX;
			return (RESULT->KIND = HEARD_UPDATED);
		}

		if (HAS_EVIDENCE)
		{
			int		NEW_INDEX = ADD_FACT(STORE, &FACT);
			char	OLD_QUOTE[480];
			char	WHO[64];

			if (NEW_INDEX < 0)
				return (HEARD_NOTHING);

			OLD = &STORE->FACTS[INDEX];
			QUOTE_FACT(OLD, OLD_QUOTE, sizeof OLD_QUOTE);
			TEACHER_NAME(OLD->TEACHER, WHO, sizeof WHO);
			OLD->STATUS = TAUGHT_REPLACED;
			OLD->REPLACED_BY = NEW_INDEX;
			STORE->LAST_SAVED = RESULT->FACT_INDEX = NEW_INDEX;
			snprintf(
				RESULT->SAY, sizeof RESULT->SAY,
				"%s taught me that %s, but you showed me where yours comes "
				"from "
				"(\"%s\"), so I'll go with what you said.",
				WHO, OLD_QUOTE, EVIDENCE
			);
			ADD_LOST_SOURCE(STORE, INDEX, RESULT->SAY, sizeof RESULT->SAY);
			return (RESULT->KIND = HEARD_CHANGED_MIND);
		}

		FACT.STATUS = TAUGHT_DISPUTED;

		int	NEW_INDEX = ADD_FACT(STORE, &FACT);

		if (NEW_INDEX < 0)
			return (HEARD_NOTHING);

		STORE->PENDING = RESULT->FACT_INDEX = NEW_INDEX;
		SAY_DISPUTE(
			&STORE->FACTS[INDEX], FACT.TEXT, 0, RESULT->SAY, sizeof RESULT->SAY
		);
		return (RESULT->KIND = HEARD_DISPUTED);
	}

	int	NEW_INDEX = ADD_FACT(STORE, &FACT);

	if (NEW_INDEX < 0)
		return (HEARD_NOTHING);

	STORE->LAST_SAVED = RESULT->FACT_INDEX = NEW_INDEX;

	return (RESULT->KIND = HEARD_SAVED);
}

static void
	KEY_OF_PHRASE(const char *PHRASE, char *OUTPUT, int SIZE)
{
	char	NORMALIZED[300];
	char	*WORDS[40];
	int		COUNT;
	int		FIRST = 0;
	int		INDEX;
	int		POSITION = 0;

	NORMALIZE(PHRASE, NORMALIZED, sizeof NORMALIZED);
	COUNT = SPLIT_WORDS(NORMALIZED, WORDS, 40);

	while (
		FIRST < COUNT &&
		(
			!strcmp(WORDS[FIRST], "a") ||
			!strcmp(WORDS[FIRST], "an") ||
			!strcmp(WORDS[FIRST], "the")
		)
	)
		FIRST++;

	OUTPUT[0] = 0;

	for (INDEX = FIRST; INDEX < COUNT; INDEX++)
	{
		char	WORD[64];

		if (INDEX == COUNT - 1)
			SINGULAR(WORDS[INDEX], WORD, sizeof WORD);
		else
			snprintf(WORD, sizeof WORD, "%s", WORDS[INDEX]);

		POSITION += snprintf(
			OUTPUT + POSITION, SIZE - POSITION, "%s%s", POSITION ? " " : "",
			WORD
		);

		if (POSITION >= SIZE - 1)
			break ;
	}
}

/* the newest believed definition of a word or phrase */
int
	TAUGHT_FIND_KEY(TAUGHT_STORE *STORE, const char *PHRASE)
{
	char	KEY[120];
	int		INDEX;

	KEY_OF_PHRASE(PHRASE, KEY, sizeof KEY);

	if (!KEY[0])
		return (-1);

	for (INDEX = STORE->COUNT - 1; INDEX >= 0; INDEX--)
		if (
			STORE->FACTS[INDEX].STATUS == TAUGHT_BELIEVED &&
			!strcmp(STORE->FACTS[INDEX].KEY, KEY)
		)
			return (INDEX);

	return (-1);
}

/* believed facts holding every content word of the phrase, oldest first */
int
	TAUGHT_ABOUT(
		TAUGHT_STORE *STORE, const char *PHRASE, int *INDEXES, int MAXIMUM
	)
{
	char	WANTED[600];
	int		WANTED_COUNT;
	int		FOUND = 0;
	int		INDEX;

	CONTENT_OF(PHRASE, WANTED, sizeof WANTED, 1);
	WANTED_COUNT = COUNT_WORDS(WANTED);

	if (!WANTED_COUNT)
		return (0);

	for (INDEX = 0; INDEX < STORE->COUNT && FOUND < MAXIMUM; INDEX++)
	{
		char	CONTENT[1200];

		if (STORE->FACTS[INDEX].STATUS != TAUGHT_BELIEVED)
			continue ;

		CONTENT_OF(STORE->FACTS[INDEX].TEXT, CONTENT, sizeof CONTENT, 1);

		if (SHARED_WORDS(WANTED, CONTENT) == WANTED_COUNT)
			INDEXES[FOUND++] = INDEX;
	}

	return (FOUND);
}

/* "how many bits are in one byte?", "8 bits are what?": believed facts that
 * hold every word the question is about (at least two, numbers count) */
int
	TAUGHT_ANSWER(
		TAUGHT_STORE *STORE, const char *QUESTION, int *INDEXES, int MAXIMUM
	)
{
	static const char	*ASKING_WORDS[39] = {
		"called", "named", "mean", "means", "meaning", "inside", "contain",
		"contains", "exactly", "again", "tell", "know", "remember", "please",
		"anyway", "so", "ok", "okay", "hey", "lucy", "code", "you", "me",
		"there", "thing", "things", "stuff", "stuffs", "asking", "ask", "asked",
		"wondering", "question", "i", "am", "now", "then", "still", NULL
	};
	char				NORMALIZED[1200];
	char				FILTERED[1200];
	char				*WORDS[160];
	char				WANTED[1200];
	int					WANTED_COUNT;
	int					COUNT;
	int					INDEX;
	int					POSITION = 0;
	int					FOUND = 0;
	int					BEST_EXTRA[8];

	NORMALIZE(QUESTION, NORMALIZED, sizeof NORMALIZED);
	COUNT = SPLIT_WORDS(NORMALIZED, WORDS, 160);
	FILTERED[0] = 0;

	for (INDEX = 0; INDEX < COUNT; INDEX++)
		if (!IN_LIST(WORDS[INDEX], ASKING_WORDS))
			POSITION += snprintf(
				FILTERED + POSITION, sizeof FILTERED - POSITION, "%s ",
				WORDS[INDEX]
			);

	CONTENT_OF(FILTERED, WANTED, sizeof WANTED, 1);
	WANTED_COUNT = COUNT_WORDS(WANTED);

	if (WANTED_COUNT < 2 || MAXIMUM > 8)
		return (0);

	/* the facts with the fewest words besides the asked ones come first */
	for (INDEX = 0; INDEX < STORE->COUNT; INDEX++)
	{
		char	CONTENT[1200];
		int		EXTRA;
		int		SLOT;

		if (STORE->FACTS[INDEX].STATUS != TAUGHT_BELIEVED)
			continue ;

		CONTENT_OF(STORE->FACTS[INDEX].TEXT, CONTENT, sizeof CONTENT, 1);

		if (SHARED_WORDS(WANTED, CONTENT) != WANTED_COUNT)
			continue ;

		EXTRA = COUNT_WORDS(CONTENT) - WANTED_COUNT;

		for (SLOT = FOUND; SLOT > 0 && BEST_EXTRA[SLOT - 1] > EXTRA; SLOT--)
			if (SLOT < MAXIMUM)
			{
				INDEXES[SLOT] = INDEXES[SLOT - 1];
				BEST_EXTRA[SLOT] = BEST_EXTRA[SLOT - 1];
			}

		if (SLOT < MAXIMUM)
		{
			INDEXES[SLOT] = INDEX;
			BEST_EXTRA[SLOT] = EXTRA;

			if (FOUND < MAXIMUM)
				FOUND++;
		}
	}

	return (FOUND);
}

/* the newest believed facts one person taught, newest first */
int
	TAUGHT_BY(
		TAUGHT_STORE *STORE, const char *TEACHER, int *INDEXES, int MAXIMUM
	)
{
	int	FOUND = 0;
	int	INDEX;

	for (INDEX = STORE->COUNT - 1; INDEX >= 0 && FOUND < MAXIMUM; INDEX--)
		if (
			STORE->FACTS[INDEX].STATUS == TAUGHT_BELIEVED &&
			!strcmp(STORE->FACTS[INDEX].TEACHER, TEACHER)
		)
			INDEXES[FOUND++] = INDEX;

	return (FOUND);
}

int
	TAUGHT_HAS_WORD(TAUGHT_STORE *STORE, const char *WORD)
{
	int	FOUND[1];

	return (TAUGHT_ABOUT(STORE, WORD, FOUND, 1));
}

/* "what is a name?", "what does X mean?", "do you know what X is?" -> the
 * phrase asked about */
int
	TAUGHT_ASKED_KEY(const char *SENTENCE, char *OUTPUT, int OUTPUT_SIZE)
{
	static const char	*PREFIXES[24] = {
		" what is a ", " what is an ", " what is the ", " what is ",
		" what's a ", " what's an ", " what's the ", " what's ", " whats a ",
		" whats ", " what are ", " what was ", " define ", " meaning of ",
		" do you know what a ", " do you know what an ", " do you know what ",
		" do you remember what a ", " do you remember what ",
		" tell me what a ", " tell me what ", " what does ", " what do ", NULL
	};
	char				NORMALIZED[600];
	int					INDEX;

	OUTPUT[0] = 0;
	NORMALIZE(SENTENCE, NORMALIZED, sizeof NORMALIZED);

	/* a quoted phrase asked about: what does "are you alright" mean? */
	{
		const char	*OPEN = strchr(SENTENCE, '"');
		const char	*CLOSE = OPEN ? strchr(OPEN + 1, '"') : NULL;

		if (
			OPEN &&
			CLOSE &&
			CLOSE - OPEN < 70 &&
			(strstr(NORMALIZED, " mean ") || strstr(NORMALIZED, " means "))
		)
		{
			char	QUOTED[100];
			char	QUOTED_NORMALIZED[120];

			snprintf(
				QUOTED, sizeof QUOTED, "%.*s", (int)(CLOSE - OPEN - 1), OPEN + 1
			);
			NORMALIZE(QUOTED, QUOTED_NORMALIZED, sizeof QUOTED_NORMALIZED);
			TRIM(QUOTED_NORMALIZED);
			snprintf(OUTPUT, OUTPUT_SIZE, "%s", QUOTED_NORMALIZED);
			return (OUTPUT[0] != 0);
		}
	}

	for (INDEX = 0; PREFIXES[INDEX]; INDEX++)
	{
		char	*FOUND = strstr(NORMALIZED, PREFIXES[INDEX]);

		if (!FOUND)
			continue ;

		char	REST[300];

		snprintf(REST, sizeof REST, "%s", FOUND + strlen(PREFIXES[INDEX]));

		/* "what does X mean", "what do X mean" */
		if (
			!strcmp(PREFIXES[INDEX], " what does ") ||
			!strcmp(PREFIXES[INDEX], " what do ")
		)
		{
			char	*MEAN = strstr(REST, " mean ");

			if (!MEAN)
				return (0);

			*MEAN = 0;
		}
		else
		{
			char	*END = REST;
			int		WORDS = 0;

			/* up to 4 words, stopping at "is", "means", "again", "now" */
			while (*END && WORDS < 4)
			{
				char	*SPACE = strchr(END, ' ');
				char	WORD[40];

				if (!SPACE)
					break ;

				snprintf(WORD, sizeof WORD, "%.*s", (int)(SPACE - END), END);

				if (
					!strcmp(WORD, "is") ||
					!strcmp(WORD, "means") ||
					!strcmp(WORD, "mean") ||
					!strcmp(WORD, "again") ||
					!strcmp(WORD, "now") ||
					!strcmp(WORD, "exactly") ||
					!strcmp(WORD, "are") ||
					!strcmp(WORD, "anyway")
				)
					break ;

				END = SPACE + 1;
				WORDS++;
			}

			*END = 0;
		}

		TRIM(REST);

		if (!REST[0])
			continue ;

		snprintf(OUTPUT, OUTPUT_SIZE, "%s", REST);
		return (1);
	}

	return (0);
}

/* "what did I tell you about dinosaurs?", "what do you know about X?",
 * "do you remember what I said about X?" -> "dinosaurs" */
int
	TAUGHT_ASKED_ABOUT(const char *SENTENCE, char *OUTPUT, int OUTPUT_SIZE)
{
	static const char	*PATTERNS[11] = {
		" told you about ", " tell you about ", " taught you about ",
		" said about ", " explained about ", " know about ", " remember about ",
		" learned about ", " learn about ", " teach you about ", NULL
	};
	char				NORMALIZED[600];
	int					INDEX;

	OUTPUT[0] = 0;
	NORMALIZE(SENTENCE, NORMALIZED, sizeof NORMALIZED);

	if (
		!ENDS_WITH_QUESTION(SENTENCE) &&
		strncmp(NORMALIZED, " what ", 6) &&
		strncmp(NORMALIZED, " do you ", 8) &&
		strncmp(NORMALIZED, " tell me ", 9)
	)
		return (0);

	for (INDEX = 0; PATTERNS[INDEX]; INDEX++)
	{
		char	*FOUND = strstr(NORMALIZED, PATTERNS[INDEX]);

		if (!FOUND)
			continue ;

		char	REST[300];

		snprintf(REST, sizeof REST, "%s", FOUND + strlen(PATTERNS[INDEX]));
		TRIM(REST);

		if (!strncmp(REST, "the ", 4))
			memmove(REST, REST + 4, strlen(REST + 4) + 1);

		if (!strncmp(REST, "my ", 3) || !strncmp(REST, "me ", 3) || !REST[0])
			return (0);

		snprintf(OUTPUT, OUTPUT_SIZE, "%s", REST);
		return (1);
	}

	return (0);
}

/* every believed definition of a word or phrase, oldest first */
int
	TAUGHT_FIND_KEYS(
		TAUGHT_STORE *STORE, const char *PHRASE, int *INDEXES, int MAXIMUM
	)
{
	char	KEY[120];
	int		FOUND = 0;
	int		INDEX;

	KEY_OF_PHRASE(PHRASE, KEY, sizeof KEY);

	if (!KEY[0])
		return (0);

	for (INDEX = 0; INDEX < STORE->COUNT && FOUND < MAXIMUM; INDEX++)
		if (
			STORE->FACTS[INDEX].STATUS == TAUGHT_BELIEVED &&
			!strcmp(STORE->FACTS[INDEX].KEY, KEY)
		)
			INDEXES[FOUND++] = INDEX;

	return (FOUND);
}

/* a claim checked against what was taught: 1 agrees, -1 contradicts, 0 not
 * known; OUTPUT says which fact and who taught it */
int
	TAUGHT_CHECK_CLAIM(
		TAUGHT_STORE *STORE, const char *CLAIM, char *OUTPUT, int OUTPUT_SIZE,
		int *FACT_INDEX
	)
{
	char	CLAIM_CONTENT[1200];
	int		CLAIM_COUNT;
	int		BEST = -1;
	int		BEST_SHARED = 0;
	int		INDEX;

	OUTPUT[0] = 0;
	*FACT_INDEX = -1;
	CONTENT_OF(CLAIM, CLAIM_CONTENT, sizeof CLAIM_CONTENT, 0);
	CLAIM_COUNT = COUNT_WORDS(CLAIM_CONTENT);

	if (CLAIM_COUNT < 2)
		return (0);

	for (INDEX = 0; INDEX < STORE->COUNT; INDEX++)
	{
		char	CONTENT[1200];
		int		COUNT;
		int		SHARED;
		int		SMALLER;

		/* a fact from this very message cannot confirm itself */
		if (
			STORE->FACTS[INDEX].STATUS != TAUGHT_BELIEVED ||
			STORE->FACTS[INDEX].SERIAL == STORE->SERIAL
		)
			continue ;

		CONTENT_OF(STORE->FACTS[INDEX].TEXT, CONTENT, sizeof CONTENT, 0);
		COUNT = COUNT_WORDS(CONTENT);
		SHARED = SHARED_WORDS(CLAIM_CONTENT, CONTENT);

		if (COUNT < CLAIM_COUNT)
			SMALLER = COUNT;
		else
			SMALLER = CLAIM_COUNT;

		if (SMALLER >= 2 && SHARED * 4 >= SMALLER * 3 && SHARED > BEST_SHARED)
		{
			BEST = INDEX;
			BEST_SHARED = SHARED;
		}
	}

	if (BEST < 0)
		return (0);

	TAUGHT_FACT	*FACT = &STORE->FACTS[BEST];
	char		QUOTE[480];
	char		WHO[64];
	int			AGREES = !CONTRADICTS(FACT, CLAIM, "", 0);

	QUOTE_FACT(FACT, QUOTE, sizeof QUOTE);
	TEACHER_NAME(FACT->TEACHER, WHO, sizeof WHO);
	*FACT_INDEX = BEST;
	STORE->LAST_USED = BEST;
	snprintf(
		OUTPUT, OUTPUT_SIZE, "%s %s taught me that %s.",
		AGREES ? "Yes." : "No.", WHO, QUOTE
	);

	if (AGREES)
		return (1);

	return (-1);
}
