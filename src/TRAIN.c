#include "DATA.h"
#include "THREAD.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <signal.h>

static volatile int	STOP_REQUESTED = 0;

static void
	ON_INTERRUPT_SIGNAL(int SIGNAL_NUMBER)
{
	(void)SIGNAL_NUMBER;
	STOP_REQUESTED = 1;
}

typedef struct
{
	WEB		*NEURAL_WEB;
	REPLICA	*WORKER_REPLICA;
	int32_t	*SEQUENCES;
	float	*WEIGHTS;
	int		*LENGTHS;
	int		SEQUENCE_COUNT;
	int		RUN_MODE;
	int		*NEXT_INDEX;
	double	LOSS;
	double	WEIGHT_SUM;
	double	REPLY_LOSS;
	double	REPLY_WEIGHT_SUM;
} TRAINING_JOB;

static void
	RUN_TRAINING_JOB(void *JOB_ARGUMENT)
{
	TRAINING_JOB	*JOB = JOB_ARGUMENT;
	WEB				*NEURAL_WEB = JOB->NEURAL_WEB;
	KERNEL_CONTEXT	*KERNEL_STATE;

	if (JOB->WORKER_REPLICA)
		KERNEL_STATE = JOB->WORKER_REPLICA->KERNEL_STATE;
	else
		KERNEL_STATE = NEURAL_WEB->KERNEL_STATE;

	uint8_t	*ACTIVATIONS;

	if (JOB->WORKER_REPLICA)
		ACTIVATIONS = JOB->WORKER_REPLICA->ACTIVATIONS;
	else
		ACTIVATIONS = NEURAL_WEB->ACTIVATIONS;

	int	MAX_POSITIONS = NEURAL_WEB->CONFIGURATION.MAXIMUM_LENGTH;

	JOB->LOSS = JOB->WEIGHT_SUM = JOB->REPLY_LOSS = JOB->REPLY_WEIGHT_SUM = 0;

	for (;;)
	{
		int	SEQUENCE_INDEX =
			__atomic_fetch_add(JOB->NEXT_INDEX, 1, __ATOMIC_RELAXED);

		if (SEQUENCE_INDEX >= JOB->SEQUENCE_COUNT)
			break ;

		int32_t	*SEQUENCE =
			JOB->SEQUENCES + (int64_t)SEQUENCE_INDEX * (MAX_POSITIONS + 1);
		float	*SEQUENCE_WEIGHTS =
			JOB->WEIGHTS + (int64_t)SEQUENCE_INDEX * (MAX_POSITIONS + 1);
		int		SEQUENCE_LENGTH = JOB->LENGTHS[SEQUENCE_INDEX];

		if (SEQUENCE_LENGTH < 2)
			continue ;

		int		POSITION_COUNT = SEQUENCE_LENGTH - 1;
		int32_t	*INPUT_TOKENS =
			(int32_t *)(ACTIVATIONS + NEURAL_WEB->TOKENS_OFFSET);
		int32_t	*TARGETS =
			(int32_t *)(ACTIVATIONS + NEURAL_WEB->TARGETS_OFFSET);
		float	*POSITION_WEIGHTS =
			(float *)(ACTIVATIONS + NEURAL_WEB->LOSS_WEIGHTS_OFFSET);
		int		POSITION;

		for (POSITION = 0; POSITION < POSITION_COUNT; POSITION++)
		{
			INPUT_TOKENS[POSITION] = SEQUENCE[POSITION];
			TARGETS[POSITION] = SEQUENCE[POSITION + 1];
			POSITION_WEIGHTS[POSITION] = SEQUENCE_WEIGHTS[POSITION + 1];

			if (
				POSITION_WEIGHTS[POSITION] == 0 ||
				(POSITION_WEIGHTS[POSITION] < 0 && JOB->RUN_MODE != MODE_TRAIN)
			)
				TARGETS[POSITION] = -1;
		}

		if (JOB->RUN_MODE == MODE_TRAIN)
			LEARN_AVOID_WEIGHTS(
				NEURAL_WEB, KERNEL_STATE, ACTIVATIONS, POSITION_COUNT
			);

		KERNEL_STATE->SEQUENCE_LENGTH = POSITION_COUNT;
		KERNEL_STATE->START_POSITION = 0;
		KERNEL_STATE->MODE = JOB->RUN_MODE;
		WEB_RUN_KERNEL_STATE(NEURAL_WEB, KERNEL_STATE);

		for (POSITION = 0; POSITION < POSITION_COUNT; POSITION++)
		{
			if (TARGETS[POSITION] < 0 || POSITION_WEIGHTS[POSITION] < 0)
				continue ;

			double	POSITION_LOSS =
				WEB_LOSS_AT(NEURAL_WEB, ACTIVATIONS, POSITION);

			JOB->LOSS += POSITION_WEIGHTS[POSITION] * POSITION_LOSS;
			JOB->WEIGHT_SUM += POSITION_WEIGHTS[POSITION];

			if (POSITION_WEIGHTS[POSITION] >= (float)0.99)
			{
				JOB->REPLY_LOSS += POSITION_LOSS;
				JOB->REPLY_WEIGHT_SUM += 1;
			}
		}
	}
}

typedef struct
{
	WEB		*NEURAL_WEB;
	REPLICA	**REPLICAS;
	int		REPLICA_COUNT;
	int		PART;
	int		PART_COUNT;
	int		IS_SYNCHRONIZING;
} REDUCE_JOB;

static void
	RUN_REDUCE_JOB(void *JOB_ARGUMENT)
{
	REDUCE_JOB	*REDUCTION = JOB_ARGUMENT;

	if (REDUCTION->IS_SYNCHRONIZING)
		REPLICA_SYNCHRONIZE_RANGE(
			REDUCTION->NEURAL_WEB, REDUCTION->REPLICAS,
			REDUCTION->REPLICA_COUNT, REDUCTION->PART, REDUCTION->PART_COUNT
		);
	else
		REPLICA_REDUCE_RANGE(
			REDUCTION->NEURAL_WEB, REDUCTION->REPLICAS,
			REDUCTION->REPLICA_COUNT, REDUCTION->PART, REDUCTION->PART_COUNT
		);
}

static void
	REDUCE_ALL_REPLICAS(
		WEB *NEURAL_WEB, REPLICA **REPLICAS, int THREAD_COUNT, int SYNC_REPLICAS
	)
{
	if (THREAD_COUNT < 2)
		return ;

	if (SYNC_REPLICAS)
	{
		int	INDEX;

		for (INDEX = 1; INDEX < THREAD_COUNT; INDEX++)
			REPLICA_PREPARE(NEURAL_WEB, REPLICAS[INDEX]);
	}

	REDUCE_JOB	REDUCE_JOBS[64];
	void		*JOB_ARGUMENTS[64];
	int			INDEX;

	for (INDEX = 0; INDEX < THREAD_COUNT; INDEX++)
	{
		REDUCE_JOBS[INDEX].NEURAL_WEB = NEURAL_WEB;
		REDUCE_JOBS[INDEX].REPLICAS = REPLICAS;
		REDUCE_JOBS[INDEX].REPLICA_COUNT = THREAD_COUNT;
		REDUCE_JOBS[INDEX].PART = INDEX;
		REDUCE_JOBS[INDEX].PART_COUNT = THREAD_COUNT;
		REDUCE_JOBS[INDEX].IS_SYNCHRONIZING = SYNC_REPLICAS;
		JOB_ARGUMENTS[INDEX] = &REDUCE_JOBS[INDEX];
	}

	THREAD_RUN(RUN_REDUCE_JOB, JOB_ARGUMENTS, THREAD_COUNT);
}

typedef struct
{
	int			INITIALIZE_NEW;
	int			THREAD_COUNT;
	int			BATCH_SIZE;
	int			WORD_COUNT;
	int			DIMENSION;
	int			LAYER_COUNT;
	int			HEAD_COUNT;
	int			FEED_FORWARD_COUNT;
	int			CONTEXT_LENGTH;
	int			VOCABULARY_MAXIMUM;
	int			RESHAPE_CONTEXT;
	int			RESHAPE_VOCABULARY;
	int			RESHAPE_FEED_FORWARD;
	int			RESHAPE_HEADS;
	int			ADD_LAYERS;
	int			WIDTH;
	int			LAYER_FEED_FORWARD;
	int			LAYER_HEADS;
	int			ADD_RECIPE_COUNT;
	int			ROTARY_POSITIONS;
	const char	*RECIPE_FILES[16];
	const char	*ADD_RECIPE_NAME;
	const char	*GROW_RECIPE;
	int			RECIPE_FILE_COUNT;
	double		MAXIMUM_PARAMETERS;
	double		GRADIENT_SKIP;
	double		MINUTES;
	double		LEARNING_RATE;
	double		SAVE_EVERY_MINUTES;
	int64_t		STEP_LIMIT;
	int			SAVE_TO_START;
	int			ALLOW_GROWTH;
	int			QUIET;
	int			BATCH_SIZE_SET;
	int			PRUNE_WORDS;
	int			WORD_BUDGET;
	int			MEASURE_ONLY;
	int			LEARNING_RATE_MODE;
	int			WARM_UP_STEPS;
	int			TREE_DEPTH;
	int			TREE_COUNT;
	const char	*VALIDATION_FILE;
	const char	*VALIDATION_OUTPUT_FILE;
	char		*FILES[256];
	double		FILE_SHARES[256];
	uint64_t	DATA_SEED;
	int			FILE_COUNT;
} TRAIN_OPTIONS;

static void
	PRINT_USAGE(void)
{
	printf(
		"ai-train: trains Lucy's brain on text files\n"
		"  ai-train [options] file1.txt file2.txt ...\n"
		"  --init            build a brand new brain + vocabulary from these "
		"files (writes mind_original/start.* too)\n"
		"  --minutes M       stop after M minutes (default 60)\n"
		"  --steps N         stop after N update steps\n"
		"  --threads N       worker threads (default: all cores)\n"
		"  --batch B         windows per update (default 16)\n"
		"  --lr X            learning rate (default 0.001)\n"
		"  --lr-mode M       age: lower the rate as the brain gets older (old "
		"default, floor 10%%)\n"
		"                    steady: warm up, then keep X for the whole run\n"
		"                    cool: fall from X to almost 0 by the end of the "
		"run (1 - sqrt curve); use for the last part of a round\n"
		"  --warm-steps N    warm-up steps at the start of a steady or cool "
		"run (default 300)\n"
		"  --valid FILE      validation file for honest loss (default "
		"data/valid.txt if present)\n"
		"  --valid-file F    write the latest validation perplexity x10 to F "
		"(for scripts)\n"
		"  --to-start        also save the result as the starting brain "
		"(mind_original/start.*)\n"
		"  FILE@S            use only a share S (0..1) of that file's "
		"conversations in this run, e.g. data/persona_chat_clean.txt@0.15\n"
		"  --data-seed N     which conversations a shared file contributes "
		"(change it every run to rotate them)\n"
		"  --measure         change nothing: report loss and reply perplexity "
		"on a sample of each file (and --valid), then exit\n"
		"  --no-grow         disable neuron growth\n"
		"  --save-every M    save every M minutes (default 10)\n"
		"  --prune-words N   keep only the N most used word neurons (rare "
		"words are then spelled), then train\n"
		"  --word-budget N   allow up to N word neurons; new frequent words "
		"(40+ uses) get neurons until the budget is full\n"
		"  --reshape T,V,F,H rebuild the brain with context T tokens, "
		"vocabulary room V, plus F new FFN neurons and H new heads per layer "
		"(new neurons start silent)\n"
		"  --add-layers N    add N new layers, spread between the old ones; "
		"they start silent, so answers stay the same\n"
		"  --layer-ffn F     FFN neurons in each new layer (default 512)   "
		"--layer-heads H  heads in each new layer (default: like the old "
		"layers)\n"
		"  --width D         widen every word vector to D numbers (multiple of "
		"32); the old numbers are kept, the new ones start silent\n"
		"  --recipe FILE     load a neuron recipe (a new neuron type built "
		"from proven blocks, see recipes/)\n"
		"  --add-recipe NAME,N  add N neurons of that recipe to every FFN "
		"layer (they start silent)\n"
		"  --grow-recipe NAME   when the brain grows, grow neurons of this "
		"recipe instead of plain FFN neurons\n"
		"  --add-tree D[,N]  add N tree neurons of depth D to every FFN layer "
		"(silent at first): each word walks one path of D+1 of their 2^(D+1)-1 "
		"nodes\n"
		"  --rope            new attention heads (from --reshape, --add-layers "
		"and growth) see relative positions (rotary)\n"
		"  --max-params N    let the brain grow up to N parameters (default "
		"12000000)\n"
		"  --grad-skip X     skip output-layer gradient terms smaller than X "
		"times the position weight (default 0.000001, 0 = exact)\n"
		"  --words N --dim D --layers L --heads H --ffn F --ctx T --vmax V   "
		"(only with --init)\n"
	);
}

static int
	PARSE_OPTIONS(TRAIN_OPTIONS *OPTIONS, int ARGUMENT_COUNT, char **ARGUMENTS)
{
	memset(OPTIONS, 0, sizeof *OPTIONS);
	OPTIONS->THREAD_COUNT = THREAD_CORE_COUNT();
	OPTIONS->BATCH_SIZE = 16;
	OPTIONS->WORD_COUNT = 7000;
	OPTIONS->DIMENSION = 256;
	OPTIONS->LAYER_COUNT = 4;
	OPTIONS->HEAD_COUNT = 8;
	OPTIONS->FEED_FORWARD_COUNT = 768;
	OPTIONS->CONTEXT_LENGTH = 256;
	OPTIONS->VOCABULARY_MAXIMUM = 16384;
	OPTIONS->MINUTES = 60;
	OPTIONS->LEARNING_RATE = 1e-3;
	OPTIONS->SAVE_EVERY_MINUTES = 10;
	OPTIONS->ALLOW_GROWTH = 1;
	OPTIONS->GRADIENT_SKIP = 1e-6;
	OPTIONS->WARM_UP_STEPS = 300;

	int	ARGUMENT_INDEX;

	for (ARGUMENT_INDEX = 1; ARGUMENT_INDEX < ARGUMENT_COUNT; ARGUMENT_INDEX++)
	{
		const char	*ARGUMENT = ARGUMENTS[ARGUMENT_INDEX];
		const char	*ARGUMENT_VALUE = ARGUMENT_INDEX + 1 < ARGUMENT_COUNT
			? ARGUMENTS[ARGUMENT_INDEX + 1]
			: NULL;
#define OPTION_WITH_VALUE(OPTION_NAME) \
	(!strcmp(ARGUMENT, OPTION_NAME) && ARGUMENT_VALUE && (ARGUMENT_INDEX++, 1))
		if (!strcmp(ARGUMENT, "--init"))
			OPTIONS->INITIALIZE_NEW = 1;
		else if (!strcmp(ARGUMENT, "--to-start"))
			OPTIONS->SAVE_TO_START = 1;
		else if (!strcmp(ARGUMENT, "--no-grow"))
			OPTIONS->ALLOW_GROWTH = 0;
		else if (!strcmp(ARGUMENT, "--rope"))
			OPTIONS->ROTARY_POSITIONS = 1;
		else if (!strcmp(ARGUMENT, "--quiet"))
			OPTIONS->QUIET = 1;
		else if (!strcmp(ARGUMENT, "--measure"))
			OPTIONS->MEASURE_ONLY = 1;
		else if (OPTION_WITH_VALUE("--lr-mode"))
		{
			if (!strcmp(ARGUMENT_VALUE, "steady"))
				OPTIONS->LEARNING_RATE_MODE = 1;
			else if (!strcmp(ARGUMENT_VALUE, "cool"))
				OPTIONS->LEARNING_RATE_MODE = 2;
			else
				OPTIONS->LEARNING_RATE_MODE = 0;

			if (
				strcmp(ARGUMENT_VALUE, "steady") &&
				strcmp(ARGUMENT_VALUE, "cool") &&
				strcmp(ARGUMENT_VALUE, "age")
			)
			{
				printf(
					"unknown --lr-mode %s (age, steady or cool)\n",
					ARGUMENT_VALUE
				);
				return (-1);
			}
		}
		else if (OPTION_WITH_VALUE("--warm-steps"))
			OPTIONS->WARM_UP_STEPS = atoi(ARGUMENT_VALUE);
		else if (!strcmp(ARGUMENT, "--help") || !strcmp(ARGUMENT, "-h"))
		{
			PRINT_USAGE();
			return (-1);
		}
		else if (OPTION_WITH_VALUE("--minutes"))
			OPTIONS->MINUTES = atof(ARGUMENT_VALUE);
		else if (OPTION_WITH_VALUE("--steps"))
			OPTIONS->STEP_LIMIT = atoll(ARGUMENT_VALUE);
		else if (OPTION_WITH_VALUE("--threads"))
			OPTIONS->THREAD_COUNT = atoi(ARGUMENT_VALUE);
		else if (OPTION_WITH_VALUE("--batch"))
		{
			OPTIONS->BATCH_SIZE = atoi(ARGUMENT_VALUE);
			OPTIONS->BATCH_SIZE_SET = 1;
		}
		else if (OPTION_WITH_VALUE("--lr"))
			OPTIONS->LEARNING_RATE = atof(ARGUMENT_VALUE);
		else if (OPTION_WITH_VALUE("--valid"))
			OPTIONS->VALIDATION_FILE = ARGUMENT_VALUE;
		else if (OPTION_WITH_VALUE("--valid-file"))
			OPTIONS->VALIDATION_OUTPUT_FILE = ARGUMENT_VALUE;
		else if (OPTION_WITH_VALUE("--save-every"))
			OPTIONS->SAVE_EVERY_MINUTES = atof(ARGUMENT_VALUE);
		else if (OPTION_WITH_VALUE("--words"))
			OPTIONS->WORD_COUNT = atoi(ARGUMENT_VALUE);
		else if (OPTION_WITH_VALUE("--dim"))
			OPTIONS->DIMENSION = atoi(ARGUMENT_VALUE);
		else if (OPTION_WITH_VALUE("--layers"))
			OPTIONS->LAYER_COUNT = atoi(ARGUMENT_VALUE);
		else if (OPTION_WITH_VALUE("--heads"))
			OPTIONS->HEAD_COUNT = atoi(ARGUMENT_VALUE);
		else if (OPTION_WITH_VALUE("--ffn"))
			OPTIONS->FEED_FORWARD_COUNT = atoi(ARGUMENT_VALUE);
		else if (OPTION_WITH_VALUE("--ctx"))
			OPTIONS->CONTEXT_LENGTH = atoi(ARGUMENT_VALUE);
		else if (OPTION_WITH_VALUE("--vmax"))
			OPTIONS->VOCABULARY_MAXIMUM = atoi(ARGUMENT_VALUE);
		else if (OPTION_WITH_VALUE("--reshape"))
			sscanf(
				ARGUMENT_VALUE, "%d,%d,%d,%d", &OPTIONS->RESHAPE_CONTEXT,
				&OPTIONS->RESHAPE_VOCABULARY, &OPTIONS->RESHAPE_FEED_FORWARD,
				&OPTIONS->RESHAPE_HEADS
			);
		else if (OPTION_WITH_VALUE("--max-params"))
			OPTIONS->MAXIMUM_PARAMETERS = atof(ARGUMENT_VALUE);
		else if (OPTION_WITH_VALUE("--add-layers"))
			OPTIONS->ADD_LAYERS = atoi(ARGUMENT_VALUE);
		else if (OPTION_WITH_VALUE("--recipe"))
		{
			if (OPTIONS->RECIPE_FILE_COUNT < 16)
				OPTIONS->RECIPE_FILES[OPTIONS->RECIPE_FILE_COUNT++] =
					ARGUMENT_VALUE;
		}
		else if (OPTION_WITH_VALUE("--add-tree"))
		{
			OPTIONS->TREE_DEPTH = atoi(ARGUMENT_VALUE);
			OPTIONS->TREE_COUNT = 1;

			const char	*COMMA = strchr(ARGUMENT_VALUE, ',');

			if (COMMA)
				OPTIONS->TREE_COUNT = atoi(COMMA + 1);

			if (
				OPTIONS->TREE_DEPTH < 1 ||
				OPTIONS->TREE_DEPTH > 16 ||
				OPTIONS->TREE_COUNT < 1
			)
			{
				printf("--add-tree DEPTH[,N]: depth 1..16\n");
				return (-1);
			}
		}
		else if (OPTION_WITH_VALUE("--add-recipe"))
		{
			static char	RECIPE_NAME[64];
			int			RECIPE_COUNT = 1;
			char		*COMMA_POSITION;

			snprintf(RECIPE_NAME, sizeof RECIPE_NAME, "%s", ARGUMENT_VALUE);

			if ((COMMA_POSITION = strchr(RECIPE_NAME, ',')))
			{
				*COMMA_POSITION = 0;
				RECIPE_COUNT = atoi(COMMA_POSITION + 1);
			}

			OPTIONS->ADD_RECIPE_NAME = RECIPE_NAME;
			OPTIONS->ADD_RECIPE_COUNT = RECIPE_COUNT;
		}
		else if (OPTION_WITH_VALUE("--grow-recipe"))
			OPTIONS->GROW_RECIPE = ARGUMENT_VALUE;
		else if (OPTION_WITH_VALUE("--width"))
			OPTIONS->WIDTH = atoi(ARGUMENT_VALUE);
		else if (OPTION_WITH_VALUE("--layer-ffn"))
			OPTIONS->LAYER_FEED_FORWARD = atoi(ARGUMENT_VALUE);
		else if (OPTION_WITH_VALUE("--layer-heads"))
			OPTIONS->LAYER_HEADS = atoi(ARGUMENT_VALUE);
		else if (OPTION_WITH_VALUE("--grad-skip"))
			OPTIONS->GRADIENT_SKIP = atof(ARGUMENT_VALUE);
		else if (OPTION_WITH_VALUE("--prune-words"))
			OPTIONS->PRUNE_WORDS = atoi(ARGUMENT_VALUE);
		else if (OPTION_WITH_VALUE("--word-budget"))
			OPTIONS->WORD_BUDGET = atoi(ARGUMENT_VALUE);
		else if (OPTION_WITH_VALUE("--data-seed"))
			OPTIONS->DATA_SEED = strtoull(ARGUMENT_VALUE, NULL, 10);
		else if (ARGUMENT[0] == '-')
		{
			printf("unknown option %s\n", ARGUMENT);
			PRINT_USAGE();
			return (-1);
		}
		else
		{
			char	*FILE_LIST[256];
			char	PATH[1024];
			double	SHARE = 1.0;

			snprintf(PATH, sizeof PATH, "%s", ARGUMENT);

			char	*AT_SIGN = strrchr(PATH, '@');

			if (
				AT_SIGN &&
				AT_SIGN[1] &&
				(isdigit((unsigned char)AT_SIGN[1]) || AT_SIGN[1] == '.')
			)
			{
				SHARE = atof(AT_SIGN + 1);
				*AT_SIGN = 0;

				if (SHARE <= 0 || SHARE > 1)
				{
					printf("bad share in %s (use file@0.25)\n", ARGUMENT);
					return (-1);
				}
			}

			int	FILE_COUNT = LIST_TEXT_FILES(PATH, FILE_LIST, 256);
			int	FILE_INDEX;

			for (FILE_INDEX = 0; FILE_INDEX < FILE_COUNT; FILE_INDEX++)
			{
				int			IS_VALIDATION = OPTIONS->VALIDATION_FILE &&
					!strcmp(FILE_LIST[FILE_INDEX], OPTIONS->VALIDATION_FILE);
				const char	*BASE_NAME = strrchr(FILE_LIST[FILE_INDEX], '/');
				const char	*BACKSLASH_NAME =
					strrchr(FILE_LIST[FILE_INDEX], '\\');

				if (BACKSLASH_NAME > BASE_NAME)
					BASE_NAME = BACKSLASH_NAME;

				if (BASE_NAME)
					BASE_NAME = BASE_NAME + 1;
				else
					BASE_NAME = FILE_LIST[FILE_INDEX];

				if (!strcmp(BASE_NAME, "valid.txt"))
					IS_VALIDATION = 1;

				if (!IS_VALIDATION && OPTIONS->FILE_COUNT < 256)
				{
					OPTIONS->FILE_SHARES[OPTIONS->FILE_COUNT] = SHARE;
					OPTIONS->FILES[OPTIONS->FILE_COUNT++] =
						FILE_LIST[FILE_INDEX];
				}
			}
		}
	}

	if (OPTIONS->THREAD_COUNT < 1)
		OPTIONS->THREAD_COUNT = 1;

	if (OPTIONS->THREAD_COUNT > 64)
		OPTIONS->THREAD_COUNT = 64;

	if (!OPTIONS->FILE_COUNT)
	{
		PRINT_USAGE();
		return (-1);
	}

	if (!OPTIONS->VALIDATION_FILE && FILE_EXISTS("data/valid.txt"))
		OPTIONS->VALIDATION_FILE = "data/valid.txt";

	return (0);
}

static LANGUAGE	*SORT_LEXICON;

static int
	COMPARE_WORD_COUNTS(const void *LEFT_ITEM, const void *RIGHT_ITEM)
{
	int32_t		LEFT_LEXEME = *(const int32_t *)LEFT_ITEM;
	int32_t		RIGHT_LEXEME = *(const int32_t *)RIGHT_ITEM;
	uint32_t	LEFT_COUNT = SORT_LEXICON->USE_COUNTS[LEFT_LEXEME];
	uint32_t	RIGHT_COUNT = SORT_LEXICON->USE_COUNTS[RIGHT_LEXEME];

	if (LEFT_COUNT > RIGHT_COUNT)
		return (-1);

	if (LEFT_COUNT < RIGHT_COUNT)
		return (1);

	return (LEFT_LEXEME - RIGHT_LEXEME);
}

static void
	SAVE_BRAIN_FILES(BRAIN *LOADED_BRAIN, int TO_START)
{
	MAKE_DIRECTORY_PATH(GLOBAL_DIRECTORY);

	if (BRAIN_SAVE(LOADED_BRAIN, GLOBAL_WEB_PATH, GLOBAL_VOCABULARY_PATH, 1))
		fprintf(stderr, "save failed\n");

	if (TO_START)
	{
		MAKE_DIRECTORY_PATH(ORIGINAL_DIRECTORY);
		BRAIN_SAVE(LOADED_BRAIN, START_WEB_PATH, START_VOCABULARY_PATH, 0);
	}
}

int
	main(int ARGUMENT_COUNT, char **ARGUMENTS)
{
	TRAIN_OPTIONS	OPTIONS;

	if (PARSE_OPTIONS(&OPTIONS, ARGUMENT_COUNT, ARGUMENTS))
		return (1);

	if (!CPU_SUPPORTED())
	{
		printf("This CPU lacks AVX2/FMA, which Code's JIT needs.\n");
		return (1);
	}

	signal(SIGINT, ON_INTERRUPT_SIGNAL);

	/* the folders of an older Lucy move into mind and mind_original */
	{
		char	MOVED[2000];

		if (MIND_MIGRATE(MOVED, sizeof MOVED))
			printf("moved to the new folders: %s\n", MOVED);
	}

	BRAIN	TRAINED_BRAIN = { 0 };

	if (OPTIONS.INITIALIZE_NEW)
	{
		if (FILE_EXISTS(LANGUAGE_PATH))
			TRAINED_BRAIN.LEXICON = LANGUAGE_LOAD(LANGUAGE_PATH);
		else
			TRAINED_BRAIN.LEXICON = LANGUAGE_NEW();

		if (!TRAINED_BRAIN.LEXICON)
			TRAINED_BRAIN.LEXICON = LANGUAGE_NEW();

		printf("counting words...\n");

		int	INDEX;

		for (INDEX = 0; INDEX < OPTIONS.FILE_COUNT; INDEX++)
			COUNT_FILE_WORDS(TRAINED_BRAIN.LEXICON, OPTIONS.FILES[INDEX]);

		int32_t	*SORTED_LEXEMES = malloc(TRAINED_BRAIN.LEXICON->COUNT * 4);

		for (INDEX = 0; INDEX < TRAINED_BRAIN.LEXICON->COUNT; INDEX++)
			SORTED_LEXEMES[INDEX] = INDEX;

		SORT_LEXICON = TRAINED_BRAIN.LEXICON;
		qsort(
			SORTED_LEXEMES, TRAINED_BRAIN.LEXICON->COUNT, 4, COMPARE_WORD_COUNTS
		);

		for (INDEX = 0; INDEX < TRAINED_BRAIN.LEXICON->COUNT; INDEX++)
			TRAINED_BRAIN.LEXICON->LEXEME_TO_TOKEN[INDEX] = -1;

		TRAINED_BRAIN.LEXICON->TOKEN_COUNT = TOKEN_FIRST_WORD;

		int	ADDED_COUNT = 0;

		for (
			INDEX = 0;
			INDEX < TRAINED_BRAIN.LEXICON->COUNT &&ADDED_COUNT <
				OPTIONS.WORD_COUNT;
			INDEX++
		)
		{
			if (TRAINED_BRAIN.LEXICON->USE_COUNTS[SORTED_LEXEMES[INDEX]] < 3)
				break ;

			LANGUAGE_VOCABULARY_ADD(
				TRAINED_BRAIN.LEXICON, SORTED_LEXEMES[INDEX]
			);
			ADDED_COUNT++;
		}

		free(SORTED_LEXEMES);
		LANGUAGE_VOCABULARY_ADD(
			TRAINED_BRAIN.LEXICON,
			LANGUAGE_ADD(TRAINED_BRAIN.LEXICON, SPECIAL_DO, 0)
		);
		LANGUAGE_VOCABULARY_ADD(
			TRAINED_BRAIN.LEXICON,
			LANGUAGE_ADD(TRAINED_BRAIN.LEXICON, SPECIAL_RESULT, 0)
		);
		LANGUAGE_VOCABULARY_ADD(
			TRAINED_BRAIN.LEXICON,
			LANGUAGE_ADD(TRAINED_BRAIN.LEXICON, SPECIAL_MORE, 0)
		);

		CONFIG	MODEL_CONFIG = {
			OPTIONS.DIMENSION, OPTIONS.LAYER_COUNT, 32, OPTIONS.CONTEXT_LENGTH,
			OPTIONS.VOCABULARY_MAXIMUM, OPTIONS.HEAD_COUNT,
			OPTIONS.FEED_FORWARD_COUNT
		};

		if (
			OPTIONS.DIMENSION % 32 ||
			OPTIONS.CONTEXT_LENGTH % 8 ||
			TRAINED_BRAIN.LEXICON->TOKEN_COUNT > OPTIONS.VOCABULARY_MAXIMUM
		)
		{
			printf("bad model size options\n");
			return (1);
		}

		TRAINED_BRAIN.NEURAL_WEB = WEB_NEW(MODEL_CONFIG, 20260925);

		if (!TRAINED_BRAIN.NEURAL_WEB)
			return (1);

		for (INDEX = 0; INDEX < TRAINED_BRAIN.LEXICON->TOKEN_COUNT; INDEX++)
			WEB_ADD_WORD(TRAINED_BRAIN.NEURAL_WEB);

		WEB_RELINK(TRAINED_BRAIN.NEURAL_WEB);
		MAKE_DIRECTORY_PATH(LANGUAGE_DIRECTORY);
		LANGUAGE_SAVE(TRAINED_BRAIN.LEXICON, LANGUAGE_PATH);
		OPTIONS.SAVE_TO_START = 1;
		printf(
			"lexicon %d words, model vocabulary %d tokens (%d words + %d "
			"spelling/special)\n",
			TRAINED_BRAIN.LEXICON->COUNT, TRAINED_BRAIN.LEXICON->TOKEN_COUNT,
			ADDED_COUNT, TOKEN_FIRST_WORD
		);
	}
	else
	{
		if (BRAIN_OPEN(&TRAINED_BRAIN, 1, 1))
			return (1);

		if (
			BRAIN_SPECIAL_TOKEN(&TRAINED_BRAIN, SPECIAL_DO) < 0 ||
			BRAIN_SPECIAL_TOKEN(&TRAINED_BRAIN, SPECIAL_RESULT) < 0 ||
			BRAIN_SPECIAL_TOKEN(&TRAINED_BRAIN, SPECIAL_MORE) < 0
		)
		{
			printf("cannot add action tokens\n");
			return (1);
		}

		if (
			OPTIONS.RESHAPE_CONTEXT > 0 ||
			OPTIONS.ADD_LAYERS > 0 ||
			OPTIONS.WIDTH > 0
		)
		{
			WEB		*OLD_WEB = TRAINED_BRAIN.NEURAL_WEB;
			SHAPE	NEW_SHAPE = {
				OPTIONS.RESHAPE_CONTEXT, OPTIONS.RESHAPE_VOCABULARY,
				OPTIONS.RESHAPE_FEED_FORWARD, OPTIONS.RESHAPE_HEADS,
				OPTIONS.ADD_LAYERS, OPTIONS.WIDTH,
				OPTIONS.LAYER_FEED_FORWARD > 0 ? OPTIONS.LAYER_FEED_FORWARD
				: 512,
				OPTIONS.LAYER_HEADS, OPTIONS.ROTARY_POSITIONS
			};
			WEB		*RESHAPED_WEB = WEB_RESHAPE_SHAPE(OLD_WEB, NEW_SHAPE);

			if (!RESHAPED_WEB)
				return (1);

			int	COUNT;

			if (OLD_WEB->CONFIGURATION.MAXIMUM_LENGTH < 200)
				COUNT = OLD_WEB->CONFIGURATION.MAXIMUM_LENGTH;
			else
				COUNT = 200;

			uint64_t	RANDOM_STATE = 99;
			int32_t		*OLD_TOKENS =
				(int32_t *)(OLD_WEB->ACTIVATIONS + OLD_WEB->TOKENS_OFFSET);
			int32_t		*NEW_TOKENS = (int32_t *)(RESHAPED_WEB->ACTIVATIONS +
				RESHAPED_WEB->TOKENS_OFFSET);
			int			INDEX;

			for (INDEX = 0; INDEX < COUNT; INDEX++)
				OLD_TOKENS[INDEX] = NEW_TOKENS[INDEX] =
					(int32_t)(RANDOM_NEXT(&RANDOM_STATE) %
						(uint64_t)OLD_WEB->WORD_COUNT);

			OLD_WEB->KERNEL_STATE->START_POSITION =
				RESHAPED_WEB->KERNEL_STATE->START_POSITION = 0;
			OLD_WEB->KERNEL_STATE->SEQUENCE_LENGTH =
				RESHAPED_WEB->KERNEL_STATE->SEQUENCE_LENGTH = COUNT;
			WEB_RUN(OLD_WEB, MODE_INFER);
			WEB_RUN(RESHAPED_WEB, MODE_INFER);

			float	MAX_DIFFERENCE = 0;
			float	*OLD_LOGITS = WEB_LAST_LOGITS(OLD_WEB);
			float	*NEW_LOGITS = WEB_LAST_LOGITS(RESHAPED_WEB);

			for (INDEX = 0; INDEX < OLD_WEB->WORD_COUNT; INDEX++)
			{
				float	LOGIT_DIFFERENCE =
					fabsf(OLD_LOGITS[INDEX] - NEW_LOGITS[INDEX]);

				if (LOGIT_DIFFERENCE > MAX_DIFFERENCE)
					MAX_DIFFERENCE = LOGIT_DIFFERENCE;
			}

			printf(
				"reshape: context %d -> %d tokens, vocabulary room %d -> %d, "
				"width %d -> %d, layers %d -> %d, +%d ffn and +%d heads per "
				"old layer; %lld -> %lld parameters\n",
				OLD_WEB->CONFIGURATION.MAXIMUM_LENGTH,
				RESHAPED_WEB->CONFIGURATION.MAXIMUM_LENGTH,
				OLD_WEB->CONFIGURATION.MAXIMUM_WORDS,
				RESHAPED_WEB->CONFIGURATION.MAXIMUM_WORDS,
				OLD_WEB->CONFIGURATION.DIMENSION,
				RESHAPED_WEB->CONFIGURATION.DIMENSION,
				OLD_WEB->CONFIGURATION.LAYER_COUNT,
				RESHAPED_WEB->CONFIGURATION.LAYER_COUNT,
				OPTIONS.RESHAPE_FEED_FORWARD, OPTIONS.RESHAPE_HEADS,
				(long long)WEB_PARAMETER_COUNT(OLD_WEB),
				(long long)WEB_PARAMETER_COUNT(RESHAPED_WEB)
			);
			printf(
				"reshape: same answers as before on a %d-token test: max logit "
				"difference %g\n",
				COUNT, MAX_DIFFERENCE
			);
			WEB_FREE(OLD_WEB);
			TRAINED_BRAIN.NEURAL_WEB = RESHAPED_WEB;
		}

		int	INDEX;

		for (INDEX = 0; INDEX < OPTIONS.RECIPE_FILE_COUNT; INDEX++)
		{
			FILE	*STREAM = fopen(OPTIONS.RECIPE_FILES[INDEX], "rb");

			if (!STREAM)
			{
				printf("cannot read recipe %s\n", OPTIONS.RECIPE_FILES[INDEX]);
				return (1);
			}

			char	*RECIPE_SOURCE = calloc(1, 1 << 16);
			size_t	BYTES_READ = fread(RECIPE_SOURCE, 1, (1 << 16) - 1, STREAM);

			fclose(STREAM);
			RECIPE_SOURCE[BYTES_READ] = 0;

			char	ERROR_MESSAGE[256];
			int		RECIPE_ID = WEB_RECIPE_ADD(
				TRAINED_BRAIN.NEURAL_WEB, RECIPE_SOURCE, ERROR_MESSAGE,
				sizeof ERROR_MESSAGE
			);

			free(RECIPE_SOURCE);

			if (RECIPE_ID < 0)
			{
				printf(
					"recipe %s: %s\n", OPTIONS.RECIPE_FILES[INDEX],
					ERROR_MESSAGE
				);
				return (1);
			}

			RECIPE	*LOADED_RECIPE =
				&TRAINED_BRAIN.NEURAL_WEB->RECIPES[RECIPE_ID];

			printf(
				"recipe %s: %d steps, %d params, %lld numbers per neuron\n",
				LOADED_RECIPE->NAME, LOADED_RECIPE->OPERATION_COUNT,
				LOADED_RECIPE->PARAMETER_COUNT,
				(long long)LOADED_RECIPE->PARAMETER_TOTAL
			);
		}

		if (OPTIONS.TREE_DEPTH)
		{
			int64_t	PARAMETERS_BEFORE =
				WEB_PARAMETER_COUNT(TRAINED_BRAIN.NEURAL_WEB);
			int		ADDED_COUNT = 0;
			int		OUTER_INDEX;

			for (
				OUTER_INDEX = 1;
				OUTER_INDEX <
					2 * TRAINED_BRAIN.NEURAL_WEB->CONFIGURATION.LAYER_COUNT;
				OUTER_INDEX += 2
			)
			{
				int	EXISTING_TREES = 0;
				int	INNER_INDEX;

				for (
					INNER_INDEX = 0;
					INNER_INDEX < TRAINED_BRAIN.NEURAL_WEB->AREAS[OUTER_INDEX]
					.MEMBER_COUNT;
					INNER_INDEX++
				)
				{
					NEURON	*CURRENT_NEURON =
						&TRAINED_BRAIN.NEURAL_WEB
					->NEURONS[TRAINED_BRAIN.NEURAL_WEB
						->AREAS[OUTER_INDEX]
						.MEMBERS[INNER_INDEX]];

					if (
						CURRENT_NEURON->ALIVE &&
						CURRENT_NEURON->KIND == KIND_TREE &&
						CURRENT_NEURON->KIND_DETAIL == OPTIONS.TREE_DEPTH
					)
						EXISTING_TREES++;
				}

				int	SECOND_INDEX;

				for (
					SECOND_INDEX = EXISTING_TREES;
					SECOND_INDEX < OPTIONS.TREE_COUNT;
					SECOND_INDEX++
				)
				{
					WEB_ADD_NEURON(
						TRAINED_BRAIN.NEURAL_WEB, KIND_TREE, OUTER_INDEX,
						WHY_GROWTH, OPTIONS.TREE_DEPTH
					);
					ADDED_COUNT++;
				}
			}

			if (ADDED_COUNT)
			{
				WEB_RELINK(TRAINED_BRAIN.NEURAL_WEB);
				printf(
					"added %d tree neuron(s) of depth %d (%lld nodes each, %d "
					"used per word) across %d FFN layers (silent at first); "
					"brain %lld -> %lld parameters\n",
					ADDED_COUNT, OPTIONS.TREE_DEPTH,
					(((long long)1 << (OPTIONS.TREE_DEPTH + 1)) - 1),
					OPTIONS.TREE_DEPTH + 1,
					TRAINED_BRAIN.NEURAL_WEB->CONFIGURATION.LAYER_COUNT,
					(long long)PARAMETERS_BEFORE,
					(long long)WEB_PARAMETER_COUNT(TRAINED_BRAIN.NEURAL_WEB)
				);
			}
			else
				printf(
					"every FFN layer already has %d tree neuron(s) of depth "
					"%d\n",
					OPTIONS.TREE_COUNT, OPTIONS.TREE_DEPTH
				);
		}

		if (OPTIONS.ADD_RECIPE_NAME)
		{
			int	RECIPE_ID = WEB_RECIPE_FIND(
				TRAINED_BRAIN.NEURAL_WEB, OPTIONS.ADD_RECIPE_NAME
			);

			if (RECIPE_ID < 0)
			{
				printf(
					"unknown recipe %s (load it with --recipe FILE)\n",
					OPTIONS.ADD_RECIPE_NAME
				);
				return (1);
			}

			int	OUTER_INDEX;

			for (
				OUTER_INDEX = 1;
				OUTER_INDEX <
					2 * TRAINED_BRAIN.NEURAL_WEB->CONFIGURATION.LAYER_COUNT;
				OUTER_INDEX += 2
			)
			{
				int	SECOND_INDEX;

				for (
					SECOND_INDEX = 0;
					SECOND_INDEX < OPTIONS.ADD_RECIPE_COUNT;
					SECOND_INDEX++
				)
					WEB_ADD_NEURON(
						TRAINED_BRAIN.NEURAL_WEB, KIND_RECIPE, OUTER_INDEX,
						WHY_GROWTH, RECIPE_ID
					);
			}

			WEB_RELINK(TRAINED_BRAIN.NEURAL_WEB);
			printf(
				"added %d '%s' neurons to each of %d layers (silent at first); "
				"brain now %lld parameters\n",
				OPTIONS.ADD_RECIPE_COUNT, OPTIONS.ADD_RECIPE_NAME,
				TRAINED_BRAIN.NEURAL_WEB->CONFIGURATION.LAYER_COUNT,
				(long long)WEB_PARAMETER_COUNT(TRAINED_BRAIN.NEURAL_WEB)
			);
		}

		TRAINED_BRAIN.NEURAL_WEB->GROWTH_ROTARY = OPTIONS.ROTARY_POSITIONS;

		if (OPTIONS.GROW_RECIPE)
		{
			TRAINED_BRAIN.NEURAL_WEB->GROWTH_RECIPE =
				WEB_RECIPE_FIND(TRAINED_BRAIN.NEURAL_WEB, OPTIONS.GROW_RECIPE);

			if (TRAINED_BRAIN.NEURAL_WEB->GROWTH_RECIPE < 0)
			{
				printf("unknown recipe %s\n", OPTIONS.GROW_RECIPE);
				return (1);
			}

			printf("growth will add '%s' neurons\n", OPTIONS.GROW_RECIPE);
		}

		if (OPTIONS.WORD_BUDGET > 0)
		{
			int32_t	WORD_ROOM =
				TRAINED_BRAIN.NEURAL_WEB->CONFIGURATION.MAXIMUM_WORDS -
				TOKEN_FIRST_WORD;

			if (OPTIONS.WORD_BUDGET < WORD_ROOM)
				TRAINED_BRAIN.NEURAL_WEB->WORD_LIMIT = OPTIONS.WORD_BUDGET;
			else
				TRAINED_BRAIN.NEURAL_WEB->WORD_LIMIT = WORD_ROOM;

			printf(
				"word budget now %d\n", TRAINED_BRAIN.NEURAL_WEB->WORD_LIMIT
			);
		}

		if (!OPTIONS.MEASURE_ONLY)
		{
			int			LEXICON_BEFORE = TRAINED_BRAIN.LEXICON->COUNT;
			LANGUAGE	*NEW_LEXICON = LANGUAGE_NEW();
			int			INDEX;

			for (INDEX = 0; INDEX < OPTIONS.FILE_COUNT; INDEX++)
				COUNT_FILE_WORDS(NEW_LEXICON, OPTIONS.FILES[INDEX]);

			int		GROWN_COUNT = 0;
			int		SWAPPED_COUNT = 0;
			int32_t	*CANDIDATES = malloc((NEW_LEXICON->COUNT + 1) * 4);
			int32_t	CANDIDATE_COUNT = 0;

			for (INDEX = 0; INDEX < NEW_LEXICON->COUNT; INDEX++)
			{
				int32_t	LEXEME_ID = LANGUAGE_FIND(
					TRAINED_BRAIN.LEXICON, NEW_LEXICON->STRINGS[INDEX]
				);

				if (LEXEME_ID < 0 && NEW_LEXICON->USE_COUNTS[INDEX] < 40)
					continue ;

				if (LEXEME_ID < 0)
				{
					LEXEME_ID = LANGUAGE_ADD(
						TRAINED_BRAIN.LEXICON, NEW_LEXICON->STRINGS[INDEX], 0
					);
					TRAINED_BRAIN.LEXICON->USE_COUNTS[LEXEME_ID] =
						NEW_LEXICON->USE_COUNTS[INDEX];
					TRAINED_BRAIN.LEXICON->CAPITAL_COUNTS[LEXEME_ID] =
						NEW_LEXICON->CAPITAL_COUNTS[INDEX];
				}

				if (
					TRAINED_BRAIN.LEXICON->LEXEME_TO_TOKEN[LEXEME_ID] < 0 &&
					NEW_LEXICON->USE_COUNTS[INDEX] >= 40 &&
					OPTIONS.PRUNE_WORDS <= 0
				)
					CANDIDATES[CANDIDATE_COUNT++] = LEXEME_ID;
			}

			LANGUAGE_FREE(NEW_LEXICON);
			SORT_LEXICON = TRAINED_BRAIN.LEXICON;
			qsort(CANDIDATES, CANDIDATE_COUNT, 4, COMPARE_WORD_COUNTS);

			int32_t	WORD_BUDGET;

			if (TRAINED_BRAIN.NEURAL_WEB->WORD_LIMIT > 0)
				WORD_BUDGET = TRAINED_BRAIN.NEURAL_WEB->WORD_LIMIT;
			else
				WORD_BUDGET =
					TRAINED_BRAIN.NEURAL_WEB->CONFIGURATION.MAXIMUM_WORDS -
					TOKEN_FIRST_WORD;

			int32_t	CANDIDATE_INDEX = 0;

			while (
				CANDIDATE_INDEX < CANDIDATE_COUNT &&
				TRAINED_BRAIN.NEURAL_WEB->WORD_COUNT - TOKEN_FIRST_WORD <
					WORD_BUDGET &&
				TRAINED_BRAIN.NEURAL_WEB->WORD_COUNT <
					TRAINED_BRAIN.NEURAL_WEB->CONFIGURATION.MAXIMUM_WORDS
			)
			{
				WEB_ADD_WORD(TRAINED_BRAIN.NEURAL_WEB);
				LANGUAGE_VOCABULARY_ADD(
					TRAINED_BRAIN.LEXICON, CANDIDATES[CANDIDATE_INDEX++]
				);
				GROWN_COUNT++;
			}

			if (CANDIDATE_INDEX < CANDIDATE_COUNT)
			{
				int32_t	VOCABULARY_SIZE = TRAINED_BRAIN.NEURAL_WEB->WORD_COUNT;
				int32_t	*ORDERED_LEXEMES = malloc(VOCABULARY_SIZE * 4);
				int32_t	COUNT = 0;
				int32_t	TOKEN;

				for (TOKEN = TOKEN_FIRST_WORD; TOKEN < VOCABULARY_SIZE; TOKEN++)
					if (
						!IS_SPECIAL_WORD(
							TRAINED_BRAIN.LEXICON,
							TRAINED_BRAIN.LEXICON->TOKEN_TO_LEXEME[TOKEN]
						)
					)
						ORDERED_LEXEMES[COUNT++] =
							TRAINED_BRAIN.LEXICON->TOKEN_TO_LEXEME[TOKEN];

				qsort(ORDERED_LEXEMES, COUNT, 4, COMPARE_WORD_COUNTS);

				uint8_t	*KEEP_FLAGS = malloc(VOCABULARY_SIZE);

				memset(KEEP_FLAGS, 1, VOCABULARY_SIZE);

				int32_t	LOWEST_RANK = COUNT - 1;

				while (
					CANDIDATE_INDEX + SWAPPED_COUNT < CANDIDATE_COUNT &&
					LOWEST_RANK >= 0 &&
					TRAINED_BRAIN.LEXICON->USE_COUNTS
							[CANDIDATES[CANDIDATE_INDEX + SWAPPED_COUNT]] >
						TRAINED_BRAIN.LEXICON
								->USE_COUNTS[ORDERED_LEXEMES[LOWEST_RANK]] *
							2
				)
				{
					KEEP_FLAGS[TRAINED_BRAIN.LEXICON->LEXEME_TO_TOKEN
						[ORDERED_LEXEMES[LOWEST_RANK]]] = 0;
					LOWEST_RANK--;
					SWAPPED_COUNT++;
				}

				if (SWAPPED_COUNT)
				{
					int32_t	*NEW_INDEX_MAP = malloc(VOCABULARY_SIZE * 4);
					int32_t	*OLD_TOKEN_LEXEMES = malloc(VOCABULARY_SIZE * 4);

					memcpy(
						OLD_TOKEN_LEXEMES,
						TRAINED_BRAIN.LEXICON->TOKEN_TO_LEXEME,
						VOCABULARY_SIZE * 4
					);
					WEB_PRUNE_WORDS(
						TRAINED_BRAIN.NEURAL_WEB, KEEP_FLAGS, NEW_INDEX_MAP
					);

					int32_t	INDEX;

					for (
						INDEX = 0;
						INDEX < TRAINED_BRAIN.LEXICON->COUNT;
						INDEX++
					)
						TRAINED_BRAIN.LEXICON->LEXEME_TO_TOKEN[INDEX] = -1;

					TRAINED_BRAIN.LEXICON->TOKEN_COUNT = TOKEN_FIRST_WORD;

					int32_t	TOKEN;

					for (
						TOKEN = TOKEN_FIRST_WORD;
						TOKEN < VOCABULARY_SIZE;
						TOKEN++
					)
						if (NEW_INDEX_MAP[TOKEN] >= 0)
							LANGUAGE_VOCABULARY_ADD(
								TRAINED_BRAIN.LEXICON, OLD_TOKEN_LEXEMES[TOKEN]
							);

					int	SAMPLE_INDEX;

					for (
						SAMPLE_INDEX = 0;
						SAMPLE_INDEX < SWAPPED_COUNT;
						SAMPLE_INDEX++
					)
					{
						WEB_ADD_WORD(TRAINED_BRAIN.NEURAL_WEB);
						LANGUAGE_VOCABULARY_ADD(
							TRAINED_BRAIN.LEXICON,
							CANDIDATES[CANDIDATE_INDEX + SAMPLE_INDEX]
						);
					}

					free(NEW_INDEX_MAP);
					free(OLD_TOKEN_LEXEMES);
				}

				free(ORDERED_LEXEMES);
				free(KEEP_FLAGS);
			}

			free(CANDIDATES);

			if (GROWN_COUNT || SWAPPED_COUNT)
				WEB_RELINK(TRAINED_BRAIN.NEURAL_WEB);

			if (SWAPPED_COUNT)
				printf(
					"vocabulary budget %d full: %d rarely used word neurons "
					"replaced by more frequent new words\n",
					WORD_BUDGET, SWAPPED_COUNT
				);

			if (
				OPTIONS.PRUNE_WORDS > 0 &&
				TRAINED_BRAIN.NEURAL_WEB->WORD_COUNT - TOKEN_FIRST_WORD >
					OPTIONS.PRUNE_WORDS
			)
			{
				int32_t	VOCABULARY_SIZE = TRAINED_BRAIN.NEURAL_WEB->WORD_COUNT;
				int32_t	*ORDERED_LEXEMES = malloc(VOCABULARY_SIZE * 4);
				int32_t	*NEW_INDEX_MAP = malloc(VOCABULARY_SIZE * 4);
				uint8_t	*KEEP_FLAGS = calloc(VOCABULARY_SIZE, 1);
				int		COUNT = 0;
				int32_t	TOKEN;

				for (TOKEN = TOKEN_FIRST_WORD; TOKEN < VOCABULARY_SIZE; TOKEN++)
					ORDERED_LEXEMES[COUNT++] = TOKEN;

				SORT_LEXICON = TRAINED_BRAIN.LEXICON;

				int	INDEX;

				for (INDEX = 0; INDEX < COUNT; INDEX++)
					ORDERED_LEXEMES[INDEX] =
						TRAINED_BRAIN.LEXICON
							->TOKEN_TO_LEXEME[ORDERED_LEXEMES[INDEX]];

				qsort(ORDERED_LEXEMES, COUNT, 4, COMPARE_WORD_COUNTS);

				for (TOKEN = 0; TOKEN < TOKEN_FIRST_WORD; TOKEN++)
					KEEP_FLAGS[TOKEN] = 1;

				for (TOKEN = TOKEN_FIRST_WORD; TOKEN < VOCABULARY_SIZE; TOKEN++)
					if (
						IS_SPECIAL_WORD(
							TRAINED_BRAIN.LEXICON,
							TRAINED_BRAIN.LEXICON->TOKEN_TO_LEXEME[TOKEN]
						)
					)
						KEEP_FLAGS[TOKEN] = 1;

				for (
					INDEX = 0;
					INDEX < OPTIONS.PRUNE_WORDS && INDEX < COUNT;
					INDEX++
				)
					KEEP_FLAGS[TRAINED_BRAIN.LEXICON
						->LEXEME_TO_TOKEN[ORDERED_LEXEMES[INDEX]]] =
						1;

				int32_t	*OLD_TOKEN_LEXEMES = malloc(VOCABULARY_SIZE * 4);

				memcpy(
					OLD_TOKEN_LEXEMES, TRAINED_BRAIN.LEXICON->TOKEN_TO_LEXEME,
					VOCABULARY_SIZE * 4
				);

				int32_t	NEW_VOCABULARY = WEB_PRUNE_WORDS(
					TRAINED_BRAIN.NEURAL_WEB, KEEP_FLAGS, NEW_INDEX_MAP
				);

				for (INDEX = 0; INDEX < TRAINED_BRAIN.LEXICON->COUNT; INDEX++)
					TRAINED_BRAIN.LEXICON->LEXEME_TO_TOKEN[INDEX] = -1;

				TRAINED_BRAIN.LEXICON->TOKEN_COUNT = TOKEN_FIRST_WORD;

				for (TOKEN = TOKEN_FIRST_WORD; TOKEN < VOCABULARY_SIZE; TOKEN++)
					if (NEW_INDEX_MAP[TOKEN] >= 0)
					{
						int32_t	ADDED_TOKEN = LANGUAGE_VOCABULARY_ADD(
							TRAINED_BRAIN.LEXICON, OLD_TOKEN_LEXEMES[TOKEN]
						);

						if (ADDED_TOKEN != NEW_INDEX_MAP[TOKEN])
						{
							printf("prune: vocabulary mismatch\n");
							return (1);
						}
					}

				TRAINED_BRAIN.NEURAL_WEB->WORD_LIMIT = OPTIONS.PRUNE_WORDS;
				printf(
					"pruned word neurons: %d -> %d, vocabulary budget now %d "
					"words (rare words are spelled; frequent new words can "
					"still take a slot)\n",
					VOCABULARY_SIZE, NEW_VOCABULARY, OPTIONS.PRUNE_WORDS
				);
				free(ORDERED_LEXEMES);
				free(NEW_INDEX_MAP);
				free(KEEP_FLAGS);
				free(OLD_TOKEN_LEXEMES);
			}

			printf(
				"lexicon %d words (%d new), %d new word neurons, %d replaced\n",
				TRAINED_BRAIN.LEXICON->COUNT,
				TRAINED_BRAIN.LEXICON->COUNT - LEXICON_BEFORE, GROWN_COUNT,
				SWAPPED_COUNT
			);
		}
	}

	WEB	*NEURAL_WEB = TRAINED_BRAIN.NEURAL_WEB;

	if (NEURAL_WEB->COPY_ID < 0)
	{
		WEB_ADD_COPY(NEURAL_WEB);
		printf(
			"growth: copy neuron %d created (purpose: point at a word in "
			"memory or context and say it)\n",
			NEURAL_WEB->COPY_ID
		);
	}

	DATASET	TRAINING_DATA = { 0 };
	DATASET	VALIDATION_DATA = { 0 };
	int64_t	CONVERSATION_RANGES[257];
	int		INDEX;

	for (INDEX = 0; INDEX < OPTIONS.FILE_COUNT; INDEX++)
	{
		CONVERSATION_RANGES[INDEX] = TRAINING_DATA.CONVERSATION_COUNT;
		LOAD_TRAINING_FILE(
			TRAINED_BRAIN.LEXICON, &TRAINING_DATA, OPTIONS.FILES[INDEX],
			NEURAL_WEB->CONFIGURATION.MAXIMUM_LENGTH
		);
	}

	CONVERSATION_RANGES[OPTIONS.FILE_COUNT] = TRAINING_DATA.CONVERSATION_COUNT;

	if (OPTIONS.VALIDATION_FILE)
		LOAD_TRAINING_FILE(
			TRAINED_BRAIN.LEXICON, &VALIDATION_DATA, OPTIONS.VALIDATION_FILE,
			NEURAL_WEB->CONFIGURATION.MAXIMUM_LENGTH
		);

	DATASET_SET_FIRST_PERSON(&TRAINING_DATA, TRAINED_BRAIN.LEXICON);
	DATASET_SET_FIRST_PERSON(&VALIDATION_DATA, TRAINED_BRAIN.LEXICON);

	int64_t	TRAIN_WINDOW_COUNT;
	int64_t	VALID_WINDOW_COUNT = 0;
	WINDOW	*WINDOWS = MAKE_WINDOWS(
		&TRAINING_DATA, NEURAL_WEB->CONFIGURATION.MAXIMUM_LENGTH,
		&TRAIN_WINDOW_COUNT
	);

	{
		int	ANY_SHARED = 0;
		int	INDEX;

		for (INDEX = 0; INDEX < OPTIONS.FILE_COUNT; INDEX++)
			if (OPTIONS.FILE_SHARES[INDEX] < 1)
				ANY_SHARED = 1;

		if (ANY_SHARED && !OPTIONS.MEASURE_ONLY)
		{
			int64_t	KEPT_COUNT = 0;
			int		FILE_INDEX = 0;
			int64_t	INDEX;

			for (INDEX = 0; INDEX < TRAIN_WINDOW_COUNT; INDEX++)
			{
				int32_t	CONVERSATION_ID = WINDOWS[INDEX].CONVERSATION_INDEX;

				while (
					FILE_INDEX < OPTIONS.FILE_COUNT - 1 &&
					CONVERSATION_ID >= CONVERSATION_RANGES[FILE_INDEX + 1]
				)
					FILE_INDEX++;

				while (
					FILE_INDEX > 0 &&
					CONVERSATION_ID < CONVERSATION_RANGES[FILE_INDEX]
				)
					FILE_INDEX--;

				uint64_t	HASH_VALUE = (uint64_t)CONVERSATION_ID *
						(unsigned long long)0X9E3779B97F4A7C15 ^
					(OPTIONS.DATA_SEED + 1) *
						(unsigned long long)0XBF58476D1CE4E5B9;

				HASH_VALUE ^= HASH_VALUE >> 31;
				HASH_VALUE *= (unsigned long long)0X94D049BB133111EB;
				HASH_VALUE ^= HASH_VALUE >> 29;

				if (
					OPTIONS.FILE_SHARES[FILE_INDEX] >= 1 ||
					(double)(HASH_VALUE >> 11) / 9007199254740992.0 <
						OPTIONS.FILE_SHARES[FILE_INDEX]
				)
					WINDOWS[KEPT_COUNT++] = WINDOWS[INDEX];
			}

			{
				int	INDEX;

				for (INDEX = 0; INDEX < OPTIONS.FILE_COUNT; INDEX++)
					if (OPTIONS.FILE_SHARES[INDEX] < 1)
						printf(
							"data: %s contributes %.0f%% of its %lld "
							"conversations this run (data seed %llu)\n",
							OPTIONS.FILES[INDEX],
							100 * OPTIONS.FILE_SHARES[INDEX],
							(long long)(CONVERSATION_RANGES[INDEX + 1] -
										CONVERSATION_RANGES[INDEX]),
							(unsigned long long)OPTIONS.DATA_SEED
						);
			}

			TRAIN_WINDOW_COUNT = KEPT_COUNT;
		}
	}

	WINDOW	*VALID_WINDOWS;

	if (OPTIONS.VALIDATION_FILE)
		VALID_WINDOWS = MAKE_WINDOWS(
			&VALIDATION_DATA, NEURAL_WEB->CONFIGURATION.MAXIMUM_LENGTH,
			&VALID_WINDOW_COUNT
		);
	else
		VALID_WINDOWS = NULL;

	if (VALID_WINDOW_COUNT > 256)
		VALID_WINDOW_COUNT = 256;

	printf(
		"data: %lld tokens, %lld conversations/texts, %lld training windows; "
		"validation windows %lld\n",
		(long long)TRAINING_DATA.TOKEN_COUNT,
		(long long)TRAINING_DATA.CONVERSATION_COUNT,
		(long long)TRAIN_WINDOW_COUNT, (long long)VALID_WINDOW_COUNT
	);

	if (!TRAIN_WINDOW_COUNT)
	{
		printf("no training data\n");
		return (1);
	}

	NEURAL_WEB->GROWTH_STATE.ENABLED = OPTIONS.ALLOW_GROWTH;

	if (!NEURAL_WEB->GROWTH_STATE.CHECK_INTERVAL)
	{
		NEURAL_WEB->GROWTH_STATE.CHECK_INTERVAL = 250;
		NEURAL_WEB->GROWTH_STATE.COOLDOWN = 1000;
		NEURAL_WEB->GROWTH_STATE.PLATEAU = (float)0.01;
		NEURAL_WEB->GROWTH_STATE.GROW_FEED_FORWARD = 32;
		NEURAL_WEB->GROWTH_STATE.MAXIMUM_HEADS = 12;
		NEURAL_WEB->GROWTH_STATE.STATISTICS_INTERVAL = 10;
		NEURAL_WEB->GROWTH_STATE.MAXIMUM_PARAMETERS = 12000000;
	}

	if (OPTIONS.MAXIMUM_PARAMETERS > 0)
		NEURAL_WEB->GROWTH_STATE.MAXIMUM_PARAMETERS =
			(int64_t)OPTIONS.MAXIMUM_PARAMETERS;

	if (
		!OPTIONS.BATCH_SIZE_SET &&
		OPTIONS.THREAD_COUNT * 2 > OPTIONS.BATCH_SIZE
	)
		OPTIONS.BATCH_SIZE = OPTIONS.THREAD_COUNT * 2;

	WEB_SET_OPTIMIZER(NEURAL_WEB, 1);

	if (OPTIONS.INITIALIZE_NEW && !NEURAL_WEB->WORD_LIMIT)
		NEURAL_WEB->WORD_LIMIT = OPTIONS.WORD_COUNT;

	int	THREAD_COUNT;

	if (OPTIONS.THREAD_COUNT < OPTIONS.BATCH_SIZE)
		THREAD_COUNT = OPTIONS.THREAD_COUNT;
	else
		THREAD_COUNT = OPTIONS.BATCH_SIZE;

	REPLICA	**REPLICAS = calloc(THREAD_COUNT, sizeof(REPLICA *));

	for (INDEX = 1; INDEX < THREAD_COUNT; INDEX++)
		REPLICAS[INDEX] = REPLICA_NEW(NEURAL_WEB);

	int				MAX_POSITIONS = NEURAL_WEB->CONFIGURATION.MAXIMUM_LENGTH;
	int32_t			*SEQUENCE =
		malloc((size_t)OPTIONS.BATCH_SIZE * (MAX_POSITIONS + 1) * 4);
	float			*TOKEN_WEIGHTS =
		malloc((size_t)OPTIONS.BATCH_SIZE * (MAX_POSITIONS + 1) * 4);
	int				*LENGTHS = malloc(OPTIONS.BATCH_SIZE * sizeof(int));
	TRAINING_JOB	*JOBS = calloc(THREAD_COUNT, sizeof(TRAINING_JOB));
	void			**JOB_ARGUMENTS = calloc(THREAD_COUNT, sizeof(void *));
	uint64_t		SHUFFLE_STATE = 12345 + NEURAL_WEB->STEP;
	int64_t			*PERMUTATION = malloc(TRAIN_WINDOW_COUNT * 8);

	{
		int64_t	INDEX;

		for (INDEX = 0; INDEX < TRAIN_WINDOW_COUNT; INDEX++)
			PERMUTATION[INDEX] = INDEX;
	}

	int64_t	PERMUTATION_POSITION = TRAIN_WINDOW_COUNT;
	int		EPOCH = 0;
	double	START_TIME = THREAD_NOW_SECONDS();
	double	LAST_REPORT_TIME = START_TIME;
	double	LAST_SAVE_TIME = START_TIME;
	int64_t	TOKEN_COUNT = 0;
	int64_t	START_STEP = NEURAL_WEB->STEP;
	double	REPORT_LOSS = 0;
	double	REPORT_WEIGHT = 0;
	double	REPORT_REPLY_LOSS = 0;
	double	REPORT_REPLY_WEIGHT = 0;
	int		ROTARY_HEADS = 0;

	for (INDEX = 0; INDEX < NEURAL_WEB->NEURON_COUNT; INDEX++)
		ROTARY_HEADS += NEURAL_WEB->NEURONS[INDEX].ALIVE &&
			NEURAL_WEB->NEURONS[INDEX].KIND == KIND_ATTENTION_HEAD &&
			NEURAL_WEB->NEURONS[INDEX].KIND_DETAIL == 1;

	if (!OPTIONS.MEASURE_ONLY)
	{
		if (OPTIONS.LEARNING_RATE_MODE == 1)
			printf(
				"learning rate: steady at %g after %d warm-up steps\n",
				OPTIONS.LEARNING_RATE, OPTIONS.WARM_UP_STEPS
			);
		else if (OPTIONS.LEARNING_RATE_MODE == 2)
			printf(
				"learning rate: cool-down from %g to %g over this run (1 - "
				"sqrt curve)\n",
				OPTIONS.LEARNING_RATE, OPTIONS.LEARNING_RATE * 0.02
			);
		else
		{
			double	AGE_STEP = (double)(NEURAL_WEB->STEP + 1);
			double	AGE_FACTOR;

			if (AGE_STEP < 400)
				AGE_FACTOR = AGE_STEP / 400;
			else
				AGE_FACTOR = sqrt(400.0 / AGE_STEP);

			if (AGE_FACTOR < 0.1)
				AGE_FACTOR = 0.1;

			printf(
				"learning rate: age schedule, starts at %g (asked %g; the "
				"brain is %lld steps old)\n",
				OPTIONS.LEARNING_RATE * AGE_FACTOR, OPTIONS.LEARNING_RATE,
				(long long)NEURAL_WEB->STEP
			);
		}
	}

	printf(
		"brain: %lld parameters, %d layers, width %d, %d neurons (%d ffn, %d "
		"heads of which %d rotary, %d recipe, %d words), %d threads, batch "
		"%d\n",
		(long long)WEB_PARAMETER_COUNT(NEURAL_WEB),
		NEURAL_WEB->CONFIGURATION.LAYER_COUNT,
		NEURAL_WEB->CONFIGURATION.DIMENSION,
		WEB_COUNT_NEURONS(NEURAL_WEB, -1, -1),
		WEB_COUNT_NEURONS(NEURAL_WEB, KIND_FEED_FORWARD, -1),
		WEB_COUNT_NEURONS(NEURAL_WEB, KIND_ATTENTION_HEAD, -1), ROTARY_HEADS,
		WEB_COUNT_NEURONS(NEURAL_WEB, KIND_RECIPE, -1), NEURAL_WEB->WORD_COUNT,
		THREAD_COUNT, OPTIONS.BATCH_SIZE
	);

	if (WEB_COUNT_NEURONS(NEURAL_WEB, KIND_TREE, -1))
		printf(
			"brain has %d tree neuron(s)\n",
			WEB_COUNT_NEURONS(NEURAL_WEB, KIND_TREE, -1)
		);

	fflush(stdout);

	if (OPTIONS.MEASURE_ONLY)
	{
		int	MEASURE_FILES =
			OPTIONS.FILE_COUNT + (OPTIONS.VALIDATION_FILE ? 1 : 0);

		printf(
			"%-44s %8s %10s %10s %12s\n", "file", "windows", "loss",
			"reply-loss", "perplexity"
		);

		int	COUNTER;

		for (COUNTER = 0; COUNTER < MEASURE_FILES; COUNTER++)
		{
			const char	*PATH = COUNTER < OPTIONS.FILE_COUNT
				? OPTIONS.FILES[COUNTER]
				: OPTIONS.VALIDATION_FILE;
			DATASET		MEASURE_DATA = { 0 };

			LOAD_TRAINING_FILE(
				TRAINED_BRAIN.LEXICON, &MEASURE_DATA, PATH, MAX_POSITIONS
			);
			DATASET_SET_FIRST_PERSON(&MEASURE_DATA, TRAINED_BRAIN.LEXICON);

			int64_t		MEASURE_WINDOW_COUNT;
			WINDOW		*MEASURE_WINDOWS = MAKE_WINDOWS(
				&MEASURE_DATA, MAX_POSITIONS, &MEASURE_WINDOW_COUNT
			);
			uint64_t	MEASURE_RANDOM = 777;
			int64_t		INDEX;

			for (INDEX = MEASURE_WINDOW_COUNT - 1; INDEX > 0; INDEX--)
			{
				int64_t	SWAP_PARTNER =
					RANDOM_NEXT(&MEASURE_RANDOM) % (INDEX + 1);
				WINDOW	SWAP_WINDOW = MEASURE_WINDOWS[INDEX];

				MEASURE_WINDOWS[INDEX] = MEASURE_WINDOWS[SWAP_PARTNER];
				MEASURE_WINDOWS[SWAP_PARTNER] = SWAP_WINDOW;
			}

			if (MEASURE_WINDOW_COUNT > 512)
				MEASURE_WINDOW_COUNT = 512;

			double	VALID_LOSS = 0;
			double	VALID_WEIGHT = 0;
			double	VALID_REPLY_LOSS = 0;
			double	VALID_REPLY_WEIGHT = 0;
			int64_t	BATCH_START;

			for (
				BATCH_START = 0;
				BATCH_START < MEASURE_WINDOW_COUNT;
				BATCH_START += OPTIONS.BATCH_SIZE
			)
			{
				int	BATCH_COUNT = (int)(MEASURE_WINDOW_COUNT - BATCH_START <
												OPTIONS.BATCH_SIZE
											? MEASURE_WINDOW_COUNT - BATCH_START
											: OPTIONS.BATCH_SIZE);
				int	BATCH_INDEX;

				for (BATCH_INDEX = 0; BATCH_INDEX < BATCH_COUNT; BATCH_INDEX++)
					LENGTHS[BATCH_INDEX] = BUILD_SEQUENCE(
						&MEASURE_DATA,
						&MEASURE_WINDOWS[BATCH_START + BATCH_INDEX],
						MAX_POSITIONS,
						SEQUENCE + (int64_t)BATCH_INDEX * (MAX_POSITIONS + 1),
						TOKEN_WEIGHTS +
							(int64_t)BATCH_INDEX * (MAX_POSITIONS + 1)
					);

				int	JOB_COUNT;

				if (BATCH_COUNT < THREAD_COUNT)
					JOB_COUNT = BATCH_COUNT;
				else
					JOB_COUNT = THREAD_COUNT;

				int	NEXT_WINDOW = 0;
				int	INDEX;

				for (INDEX = 0; INDEX < JOB_COUNT; INDEX++)
				{
					JOBS[INDEX].NEURAL_WEB = NEURAL_WEB;
					JOBS[INDEX].WORKER_REPLICA = REPLICAS[INDEX];
					JOBS[INDEX].RUN_MODE = MODE_EVALUATE;
					JOBS[INDEX].SEQUENCES = SEQUENCE;
					JOBS[INDEX].WEIGHTS = TOKEN_WEIGHTS;
					JOBS[INDEX].LENGTHS = LENGTHS;
					JOBS[INDEX].SEQUENCE_COUNT = BATCH_COUNT;
					JOBS[INDEX].NEXT_INDEX = &NEXT_WINDOW;
					JOB_ARGUMENTS[INDEX] = &JOBS[INDEX];
				}

				THREAD_RUN(RUN_TRAINING_JOB, JOB_ARGUMENTS, JOB_COUNT);

				for (INDEX = 0; INDEX < JOB_COUNT; INDEX++)
				{
					VALID_LOSS += JOBS[INDEX].LOSS;
					VALID_WEIGHT += JOBS[INDEX].WEIGHT_SUM;
					VALID_REPLY_LOSS += JOBS[INDEX].REPLY_LOSS;
					VALID_REPLY_WEIGHT += JOBS[INDEX].REPLY_WEIGHT_SUM;
				}
			}

			printf(
				"%-44.44s %8lld %10.3f %10.3f %12.2f\n", PATH,
				(long long)MEASURE_WINDOW_COUNT,
				VALID_LOSS / (VALID_WEIGHT > 0 ? VALID_WEIGHT : 1),
				VALID_REPLY_LOSS /
					(VALID_REPLY_WEIGHT > 0 ? VALID_REPLY_WEIGHT : 1),
				exp(VALID_REPLY_LOSS /
					(VALID_REPLY_WEIGHT > 0 ? VALID_REPLY_WEIGHT : 1))
			);
			fflush(stdout);
			free(MEASURE_WINDOWS);
			DATASET_FREE(&MEASURE_DATA);
		}

		return (0);
	}

	int64_t	VALIDATED_STEP = -1;

	for (;;)
	{
		int	FINISHED = STOP_REQUESTED ||
			(OPTIONS.STEP_LIMIT &&
				NEURAL_WEB->STEP - START_STEP >= OPTIONS.STEP_LIMIT) ||
			(!OPTIONS.STEP_LIMIT &&
				THREAD_NOW_SECONDS() - START_TIME > OPTIONS.MINUTES * 60);

		if (
			VALID_WINDOW_COUNT &&
			!STOP_REQUESTED &&
			NEURAL_WEB->STEP != START_STEP &&
			NEURAL_WEB->STEP != VALIDATED_STEP &&
			(FINISHED || NEURAL_WEB->STEP % 1000 == 0)
		)
		{
			VALIDATED_STEP = NEURAL_WEB->STEP;

			double	VALID_LOSS = 0;
			double	VALID_WEIGHT = 0;
			double	VALID_REPLY_LOSS = 0;
			double	VALID_REPLY_WEIGHT = 0;
			int64_t	BATCH_START;

			for (
				BATCH_START = 0;
				BATCH_START < VALID_WINDOW_COUNT;
				BATCH_START += OPTIONS.BATCH_SIZE
			)
			{
				int	BATCH_COUNT =
					(int)(VALID_WINDOW_COUNT - BATCH_START < OPTIONS.BATCH_SIZE
						? VALID_WINDOW_COUNT - BATCH_START
						: OPTIONS.BATCH_SIZE);
				int	BATCH_INDEX;

				for (BATCH_INDEX = 0; BATCH_INDEX < BATCH_COUNT; BATCH_INDEX++)
					LENGTHS[BATCH_INDEX] = BUILD_SEQUENCE(
						&VALIDATION_DATA,
						&VALID_WINDOWS[BATCH_START + BATCH_INDEX],
						MAX_POSITIONS,
						SEQUENCE + (int64_t)BATCH_INDEX * (MAX_POSITIONS + 1),
						TOKEN_WEIGHTS +
							(int64_t)BATCH_INDEX * (MAX_POSITIONS + 1)
					);

				int	JOB_COUNT;

				if (BATCH_COUNT < THREAD_COUNT)
					JOB_COUNT = BATCH_COUNT;
				else
					JOB_COUNT = THREAD_COUNT;

				int	NEXT_WINDOW = 0;
				int	INDEX;

				for (INDEX = 0; INDEX < JOB_COUNT; INDEX++)
				{
					JOBS[INDEX].NEURAL_WEB = NEURAL_WEB;
					JOBS[INDEX].WORKER_REPLICA = REPLICAS[INDEX];
					JOBS[INDEX].RUN_MODE = MODE_EVALUATE;
					JOBS[INDEX].SEQUENCES = SEQUENCE;
					JOBS[INDEX].WEIGHTS = TOKEN_WEIGHTS;
					JOBS[INDEX].LENGTHS = LENGTHS;
					JOBS[INDEX].SEQUENCE_COUNT = BATCH_COUNT;
					JOBS[INDEX].NEXT_INDEX = &NEXT_WINDOW;
					JOB_ARGUMENTS[INDEX] = &JOBS[INDEX];
				}

				THREAD_RUN(RUN_TRAINING_JOB, JOB_ARGUMENTS, JOB_COUNT);

				for (INDEX = 0; INDEX < JOB_COUNT; INDEX++)
				{
					VALID_LOSS += JOBS[INDEX].LOSS;
					VALID_WEIGHT += JOBS[INDEX].WEIGHT_SUM;
					VALID_REPLY_LOSS += JOBS[INDEX].REPLY_LOSS;
					VALID_REPLY_WEIGHT += JOBS[INDEX].REPLY_WEIGHT_SUM;
				}
			}

			printf(
				"  validation @%lld: loss %.3f  reply-loss %.3f (perplexity "
				"%.1f)\n",
				(long long)NEURAL_WEB->STEP, VALID_LOSS / VALID_WEIGHT,
				VALID_REPLY_LOSS /
					(VALID_REPLY_WEIGHT > 0 ? VALID_REPLY_WEIGHT : 1),
				exp(VALID_REPLY_LOSS /
					(VALID_REPLY_WEIGHT > 0 ? VALID_REPLY_WEIGHT : 1))
			);

			if (OPTIONS.VALIDATION_OUTPUT_FILE)
			{
				FILE	*VALID_STREAM =
					fopen(OPTIONS.VALIDATION_OUTPUT_FILE, "wb");

				if (VALID_STREAM)
				{
					fprintf(
						VALID_STREAM, "%d\n",
						(int)(10.0 *
							exp(VALID_REPLY_LOSS /
								(VALID_REPLY_WEIGHT > 0
									? VALID_REPLY_WEIGHT
									: 1)) +
							0.5)
					);
					fclose(VALID_STREAM);
				}
			}

			fflush(stdout);
		}

		if (STOP_REQUESTED)
		{
			printf("stopping (Ctrl+C)\n");
			break ;
		}

		if (FINISHED)
			break ;

		double	BATCH_WEIGHT = 0;
		int		BATCH_INDEX;

		for (BATCH_INDEX = 0; BATCH_INDEX < OPTIONS.BATCH_SIZE; BATCH_INDEX++)
		{
			if (PERMUTATION_POSITION >= TRAIN_WINDOW_COUNT)
			{
				int64_t	INDEX;

				for (INDEX = TRAIN_WINDOW_COUNT - 1; INDEX > 0; INDEX--)
				{
					int64_t	SWAP_PARTNER =
						RANDOM_NEXT(&SHUFFLE_STATE) % (INDEX + 1);
					int64_t	SWAP_VALUE = PERMUTATION[INDEX];

					PERMUTATION[INDEX] = PERMUTATION[SWAP_PARTNER];
					PERMUTATION[SWAP_PARTNER] = SWAP_VALUE;
				}

				PERMUTATION_POSITION = 0;
				EPOCH++;
			}

			LENGTHS[BATCH_INDEX] = BUILD_SEQUENCE(
				&TRAINING_DATA, &WINDOWS[PERMUTATION[PERMUTATION_POSITION++]],
				MAX_POSITIONS,
				SEQUENCE + (int64_t)BATCH_INDEX * (MAX_POSITIONS + 1),
				TOKEN_WEIGHTS + (int64_t)BATCH_INDEX * (MAX_POSITIONS + 1)
			);

			int	POSITION;

			for (POSITION = 1; POSITION < LENGTHS[BATCH_INDEX]; POSITION++)
				BATCH_WEIGHT += fabsf(
					TOKEN_WEIGHTS
						[(int64_t)BATCH_INDEX * (MAX_POSITIONS + 1) + POSITION]
				);

			TOKEN_COUNT += LENGTHS[BATCH_INDEX] - 1;
		}

		int64_t	STEP_NUMBER = NEURAL_WEB->STEP + 1;
		int64_t	WARM_STEPS = 400;
		float	LEARNING_RATE =
			(float)(OPTIONS.LEARNING_RATE *
					(STEP_NUMBER < WARM_STEPS
						? (double)STEP_NUMBER / WARM_STEPS
						: sqrt((double)WARM_STEPS / STEP_NUMBER)));

		if (LEARNING_RATE < OPTIONS.LEARNING_RATE * 0.1)
			LEARNING_RATE = (float)(OPTIONS.LEARNING_RATE * 0.1);

		if (OPTIONS.LEARNING_RATE_MODE)
		{
			int64_t	RUN_STEP = NEURAL_WEB->STEP - START_STEP + 1;
			double	WARM_RAMP;

			if (OPTIONS.WARM_UP_STEPS > 0 && RUN_STEP < OPTIONS.WARM_UP_STEPS)
				WARM_RAMP = (double)RUN_STEP / OPTIONS.WARM_UP_STEPS;
			else
				WARM_RAMP = 1.0;

			double	PROGRESS = 0;

			if (OPTIONS.LEARNING_RATE_MODE == 2)
			{
				if (OPTIONS.STEP_LIMIT)
					PROGRESS = (double)(NEURAL_WEB->STEP - START_STEP) /
						OPTIONS.STEP_LIMIT;
				else
					PROGRESS = (THREAD_NOW_SECONDS() - START_TIME) /
						(OPTIONS.MINUTES * 60);

				if (PROGRESS > 1)
					PROGRESS = 1;

				if (PROGRESS < 0)
					PROGRESS = 0;
			}

			double	RATE_FACTOR;

			if (OPTIONS.LEARNING_RATE_MODE == 2)
				RATE_FACTOR = 1.0 - sqrt(PROGRESS);
			else
				RATE_FACTOR = 1.0;

			if (RATE_FACTOR < 0.02)
				RATE_FACTOR = 0.02;

			LEARNING_RATE =
				(float)(OPTIONS.LEARNING_RATE * WARM_RAMP * RATE_FACTOR);
		}

		NEURAL_WEB->KERNEL_STATE->LEARNING_RATE = LEARNING_RATE;

		if (BATCH_WEIGHT > 0)
			NEURAL_WEB->KERNEL_STATE->GRADIENT_SCALE =
				(float)(1.0 / BATCH_WEIGHT);
		else
			NEURAL_WEB->KERNEL_STATE->GRADIENT_SCALE = 0;

		NEURAL_WEB->KERNEL_STATE->GRADIENT_SKIP_THRESHOLD =
			(float)OPTIONS.GRADIENT_SKIP;

		int	INDEX;

		for (INDEX = 1; INDEX < THREAD_COUNT; INDEX++)
		{
			REPLICAS[INDEX]->KERNEL_STATE->GRADIENT_SCALE =
				NEURAL_WEB->KERNEL_STATE->GRADIENT_SCALE;
			REPLICAS[INDEX]->KERNEL_STATE->GRADIENT_SKIP_THRESHOLD =
				NEURAL_WEB->KERNEL_STATE->GRADIENT_SKIP_THRESHOLD;
		}

		int	NEXT_SEQUENCE = 0;

		for (INDEX = 0; INDEX < THREAD_COUNT; INDEX++)
		{
			JOBS[INDEX].NEURAL_WEB = NEURAL_WEB;
			JOBS[INDEX].WORKER_REPLICA = REPLICAS[INDEX];
			JOBS[INDEX].RUN_MODE = MODE_TRAIN;
			JOBS[INDEX].SEQUENCES = SEQUENCE;
			JOBS[INDEX].WEIGHTS = TOKEN_WEIGHTS;
			JOBS[INDEX].LENGTHS = LENGTHS;
			JOBS[INDEX].SEQUENCE_COUNT = OPTIONS.BATCH_SIZE;
			JOBS[INDEX].NEXT_INDEX = &NEXT_SEQUENCE;
			JOB_ARGUMENTS[INDEX] = &JOBS[INDEX];
		}

		THREAD_RUN(RUN_TRAINING_JOB, JOB_ARGUMENTS, THREAD_COUNT);

		double	BATCH_LOSS = 0;
		double	BATCH_LOSS_WEIGHT = 0;

		for (INDEX = 0; INDEX < THREAD_COUNT; INDEX++)
		{
			BATCH_LOSS += JOBS[INDEX].LOSS;
			BATCH_LOSS_WEIGHT += JOBS[INDEX].WEIGHT_SUM;
			REPORT_REPLY_LOSS += JOBS[INDEX].REPLY_LOSS;
			REPORT_REPLY_WEIGHT += JOBS[INDEX].REPLY_WEIGHT_SUM;
		}

		REDUCE_ALL_REPLICAS(NEURAL_WEB, REPLICAS, THREAD_COUNT, 0);
		REPORT_LOSS += BATCH_LOSS;
		REPORT_WEIGHT += BATCH_LOSS_WEIGHT;
		LEARN_RECORD_LOSS(
			NEURAL_WEB,
			(float)(BATCH_LOSS / (BATCH_LOSS_WEIGHT > 0 ? BATCH_LOSS_WEIGHT : 1)
			)
		);

		int	NEURONS_BEFORE = NEURAL_WEB->NEURON_COUNT;

		LEARN_UPDATE(NEURAL_WEB);

		if (NEURAL_WEB->NEURON_COUNT != NEURONS_BEFORE && !OPTIONS.QUIET)
		{
			NEURON	*CURRENT_NEURON =
				&NEURAL_WEB->NEURONS[NEURAL_WEB->NEURON_COUNT - 1];

			printf(
				"  growth: step %lld, %d new %s neuron(s) in area %d (area "
				"error %.3g); brain now %lld parameters\n",
				(long long)NEURAL_WEB->STEP,
				NEURAL_WEB->NEURON_COUNT - NEURONS_BEFORE,
				CURRENT_NEURON->KIND == KIND_FEED_FORWARD ? "ffn"
					: CURRENT_NEURON->KIND == KIND_RECIPE
					? NEURAL_WEB->RECIPES[CURRENT_NEURON->KIND_DETAIL].NAME
					: "attention-head",
				CURRENT_NEURON->AREA_INDEX, CURRENT_NEURON->BIRTH_ERROR,
				(long long)WEB_PARAMETER_COUNT(NEURAL_WEB)
			);
		}

		REDUCE_ALL_REPLICAS(NEURAL_WEB, REPLICAS, THREAD_COUNT, 1);

		double	CURRENT_TIME = THREAD_NOW_SECONDS();

		if (CURRENT_TIME - LAST_REPORT_TIME > 30 || STOP_REQUESTED)
		{
			printf(
				"step %6lld  epoch %d  loss %.3f  reply-loss %.3f  lr %.5f  "
				"gnorm %.2f  %.0f tok/s  params %lld\n",
				(long long)NEURAL_WEB->STEP, EPOCH,
				REPORT_LOSS / (REPORT_WEIGHT > 0 ? REPORT_WEIGHT : 1),
				REPORT_REPLY_LOSS /
					(REPORT_REPLY_WEIGHT > 0 ? REPORT_REPLY_WEIGHT : 1),
				LEARNING_RATE, NEURAL_WEB->GRADIENT_NORM,
				TOKEN_COUNT / (CURRENT_TIME - LAST_REPORT_TIME),
				(long long)WEB_PARAMETER_COUNT(NEURAL_WEB)
			);
			fflush(stdout);
			REPORT_LOSS = REPORT_WEIGHT = REPORT_REPLY_LOSS =
				REPORT_REPLY_WEIGHT = 0;
			TOKEN_COUNT = 0;
			LAST_REPORT_TIME = CURRENT_TIME;
		}

		if (CURRENT_TIME - LAST_SAVE_TIME > OPTIONS.SAVE_EVERY_MINUTES * 60)
		{
			SAVE_BRAIN_FILES(&TRAINED_BRAIN, OPTIONS.SAVE_TO_START);
			LAST_SAVE_TIME = CURRENT_TIME;
			printf("  saved\n");
			fflush(stdout);
		}
	}

	SAVE_BRAIN_FILES(&TRAINED_BRAIN, OPTIONS.SAVE_TO_START);
	printf(
		"saved brain: %lld steps total, %lld parameters, %d neurons\n",
		(long long)NEURAL_WEB->STEP, (long long)WEB_PARAMETER_COUNT(NEURAL_WEB),
		WEB_COUNT_NEURONS(NEURAL_WEB, -1, -1)
	);

	for (INDEX = 1; INDEX < THREAD_COUNT; INDEX++)
		REPLICA_FREE(REPLICAS[INDEX]);

	return (0);
}
