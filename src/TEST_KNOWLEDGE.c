#include "KNOWLEDGE.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char	*CORE_SCRIPT =
	"> A blick is a zorp.\n= saved: blick is a zorp\n= want to know: what zorp "
	"is\n"
	"> Zorps can fly.\n> Can a blick fly?\n= yes: blick can fly\n= because: "
	"blick is a zorp; zorp can fly\n"
	"> Blicks cannot fly.\n= exception: blick cannot fly, although zorp can "
	"fly\n> Can a blick fly?\n= no: blick cannot fly\n"
	"> Can a blick swim?\n= don't know: blick can swim\n= missing: whether "
	"blick can swim\n"
	"> A zorp is red.\n> A zorp is not red.\n= conflict:\n> Actually, a zorp "
	"is not red.\n= replaced:\n"
	"> Fomp produces glim.\n> What produces glim?\n= list: fomp\n"
	"> List files\n= (nothing)\n> I like music.\n= (nothing)\n";

static const char	*TEACH_SENTENCES[9] = {
	"A blick is a zorp.", "Zorps can fly.", "Blicks cannot fly.",
	"Wugs have 6 legs.", "A zorp is a kind of wug.", "Fomp produces glim.",
	"Water does not absorb glim.", "A wug is round and soft.", "Cat has 4 legs."
};
static const char	*ASK_QUESTIONS[8] = {
	"Can a blick fly?", "How many legs does a blick have?", "What is a blick?",
	"What produces glim?", "Does water absorb glim?", "What is round?",
	"What is a cat?", "Can a zorp fly?"
};

int
	TEST_KNOWLEDGE(void)
{
	int	FAILURE_COUNT = 0;
	int	CHECK_COUNT = 0;

	printf("know: facts, inheritance, exceptions, conflicts and curiosity "
		"(made-up words)\n");

	int	CORE_FAILURES = KNOWLEDGE_SCRIPT(CORE_SCRIPT, 0, &CHECK_COUNT);

	printf(
		"  built-in reasoning script: %d of %d checks %s\n",
		CHECK_COUNT - CORE_FAILURES, CHECK_COUNT, CORE_FAILURES ? "FAIL" : "ok"
	);
	FAILURE_COUNT += CORE_FAILURES;

	FILE	*PROBE_STREAM = fopen("tests/know_v1.txt", "rb");

	if (PROBE_STREAM)
	{
		fclose(PROBE_STREAM);

		FILE	*STREAM = fopen("tests/know_v1.txt", "rb");

		fseek(STREAM, 0, SEEK_END);

		long	FILE_LENGTH = ftell(STREAM);

		fseek(STREAM, 0, SEEK_SET);

		char	*SCRIPT_TEXT = malloc((size_t)FILE_LENGTH + 1);
		size_t	READ_COUNT = fread(SCRIPT_TEXT, 1, (size_t)FILE_LENGTH, STREAM);

		SCRIPT_TEXT[READ_COUNT] = 0;
		fclose(STREAM);

		int	FILE_FAILURES = KNOWLEDGE_SCRIPT(SCRIPT_TEXT, 0, &CHECK_COUNT);

		printf(
			"  tests/know_v1.txt: %d of %d checks %s\n",
			CHECK_COUNT - FILE_FAILURES, CHECK_COUNT,
			FILE_FAILURES ? "FAIL" : "ok"
		);
		FAILURE_COUNT += FILE_FAILURES;
		free(SCRIPT_TEXT);
	}

	KNOWLEDGE	*ORIGINAL_KNOWLEDGE = KNOWLEDGE_NEW();
	char		ANSWER[8192];
	char		RELOADED_ANSWER[8192];
	char		ORIGINAL_LIST[65536];
	char		RELOADED_LIST[65536];
	size_t		SENTENCE_INDEX;

	for (
		SENTENCE_INDEX = 0;
		SENTENCE_INDEX < sizeof TEACH_SENTENCES / sizeof TEACH_SENTENCES[0];
		SENTENCE_INDEX++
	)
		KNOWLEDGE_HEAR(
			ORIGINAL_KNOWLEDGE, TEACH_SENTENCES[SENTENCE_INDEX], ANSWER,
			sizeof ANSWER
		);

	const char	*PATH = "tknow_tmp.txt";

	KNOWLEDGE_SAVE(ORIGINAL_KNOWLEDGE, PATH);

	KNOWLEDGE	*RELOADED_KNOWLEDGE = KNOWLEDGE_NEW();

	KNOWLEDGE_LOAD(RELOADED_KNOWLEDGE, PATH);
	KNOWLEDGE_LIST(ORIGINAL_KNOWLEDGE, ORIGINAL_LIST, sizeof ORIGINAL_LIST);
	KNOWLEDGE_LIST(RELOADED_KNOWLEDGE, RELOADED_LIST, sizeof RELOADED_LIST);

	int	IS_SAME = !strcmp(ORIGINAL_LIST, RELOADED_LIST);

	for (
		SENTENCE_INDEX = 0;
		SENTENCE_INDEX < sizeof ASK_QUESTIONS / sizeof ASK_QUESTIONS[0];
		SENTENCE_INDEX++
	)
	{
		KNOWLEDGE_HEAR(
			ORIGINAL_KNOWLEDGE, ASK_QUESTIONS[SENTENCE_INDEX], ANSWER,
			sizeof ANSWER
		);
		KNOWLEDGE_HEAR(
			RELOADED_KNOWLEDGE, ASK_QUESTIONS[SENTENCE_INDEX], RELOADED_ANSWER,
			sizeof RELOADED_ANSWER
		);

		if (strcmp(ANSWER, RELOADED_ANSWER))
		{
			IS_SAME = 0;
			printf(
				"    after reload \"%s\" differs:\n      %s\n      %s\n",
				ASK_QUESTIONS[SENTENCE_INDEX], ANSWER, RELOADED_ANSWER
			);
		}
	}

	printf(
		"  save -> load keeps every fact, open question and answer: %s\n",
		IS_SAME ? "ok" : "FAIL"
	);
	FAILURE_COUNT += !IS_SAME;
	remove(PATH);
	KNOWLEDGE_FREE(ORIGINAL_KNOWLEDGE);
	KNOWLEDGE_FREE(RELOADED_KNOWLEDGE);
	printf("know: %s\n", FAILURE_COUNT ? "FAILED" : "PASSED");

	return (FAILURE_COUNT);
}
