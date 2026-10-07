#include "CODE.h"
#include "THREAD.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <xmmintrin.h>

static const char
	*BENCH_KIND_NAME(int KIND_INDEX)
{
	static const char	*KIND_NAMES[16] = {
		"?", "entry", "exit", "update-start", "gate", "loss", "embed", "norm",
		"ffn", "head", "word", "grow", "gate2", "copy", "organ", "adam"
	};

	if (KIND_INDEX >= 0 && KIND_INDEX <= KIND_ADAM)
		return (KIND_NAMES[KIND_INDEX]);

	return ("?");
}

int
	main(int ARGUMENT_COUNT, char **ARGUMENTS)
{
	if (ARGUMENT_COUNT < 3)
	{
		printf("bench <brain.web> <T> [iters] [mode: train|infer|play|map]\n");
		return (1);
	}

	const char	*RUN_MODE = ARGUMENT_COUNT > 4 ? ARGUMENTS[4] : "train";
	int			PLAY_MODE = !strcmp(RUN_MODE, "play");
	WEB			*NEURAL_WEB;

	if (PLAY_MODE)
		NEURAL_WEB = WEB_LOAD_PLAY(ARGUMENTS[1]);
	else
		NEURAL_WEB = WEB_LOAD(ARGUMENTS[1]);

	if (!NEURAL_WEB)
		return (1);

	int	POSITION_COUNT = atoi(ARGUMENTS[2]);
	int	ITERATIONS;

	if (ARGUMENT_COUNT > 3)
		ITERATIONS = atoi(ARGUMENTS[3]);
	else
		ITERATIONS = 5;

	if (POSITION_COUNT > NEURAL_WEB->CONFIGURATION.MAXIMUM_LENGTH)
		POSITION_COUNT = NEURAL_WEB->CONFIGURATION.MAXIMUM_LENGTH;

	if (!strcmp(RUN_MODE, "dump"))
	{
		int		WANTED_ORGAN = atoi(ARGUMENTS[5]);
		int		WANTED_AREA = atoi(ARGUMENTS[6]);
		int		WANTED_ROLE = atoi(ARGUMENTS[7]);
		int32_t	INDEX;

		for (INDEX = 0; INDEX < NEURAL_WEB->PIECE_COUNT; INDEX++)
		{
			CODE_PIECE	*PIECE_ENTRY = &NEURAL_WEB->PIECES[INDEX];
			NEURON		*CURRENT_NEURON =
				&NEURAL_WEB->NEURONS[PIECE_ENTRY->ID / 4];

			if (
				CURRENT_NEURON->KIND == KIND_ORGAN &&
				CURRENT_NEURON->KIND_DETAIL == WANTED_ORGAN &&
				CURRENT_NEURON->AREA_INDEX == WANTED_AREA &&
				PIECE_ENTRY->ID % 4 == WANTED_ROLE
			)
			{
				fwrite(
					NEURAL_WEB->EXECUTABLE + PIECE_ENTRY->OFFSET, 1,
					PIECE_ENTRY->LENGTH, stdout
				);
				return (0);
			}
		}

		return (1);
	}

	if (!strcmp(RUN_MODE, "map"))
	{
		int32_t	INDEX;

		for (INDEX = 0; INDEX < NEURAL_WEB->PIECE_COUNT; INDEX++)
		{
			CODE_PIECE	*PIECE_ENTRY = &NEURAL_WEB->PIECES[INDEX];
			NEURON		*CURRENT_NEURON =
				&NEURAL_WEB->NEURONS[PIECE_ENTRY->ID / 4];

			if (!CURRENT_NEURON->ALIVE)
				continue ;

			int	KIND_NUMBER;

			if (CURRENT_NEURON->KIND == KIND_ORGAN)
				KIND_NUMBER = CURRENT_NEURON->KIND_DETAIL;
			else
				KIND_NUMBER = CURRENT_NEURON->KIND;

			printf(
				"%d %d %s%s %d %d\n", PIECE_ENTRY->OFFSET, PIECE_ENTRY->LENGTH,
				CURRENT_NEURON->KIND == KIND_ORGAN ? "organ-" : "",
				BENCH_KIND_NAME(KIND_NUMBER), CURRENT_NEURON->AREA_INDEX,
				PIECE_ENTRY->ID % 4
			);
		}

		return (0);
	}

	if (getenv("FTZ"))
		_mm_setcsr(_mm_getcsr() | 0X8040);

	NEURAL_WEB->KERNEL_STATE->GRADIENT_SCALE = (float)1.0 / POSITION_COUNT;

	if (getenv("GEPS"))
		NEURAL_WEB->KERNEL_STATE->GRADIENT_SKIP_THRESHOLD =
			(float)atof(getenv("GEPS"));
	else
		NEURAL_WEB->KERNEL_STATE->GRADIENT_SKIP_THRESHOLD = 0;

	uint64_t	RANDOM_STATE = 7;
	int32_t		*INPUT_TOKENS =
		(int32_t *)(NEURAL_WEB->ACTIVATIONS + NEURAL_WEB->TOKENS_OFFSET);
	int			POSITION;

	for (POSITION = 0; POSITION < POSITION_COUNT; POSITION++)
		INPUT_TOKENS[POSITION] = (int32_t)(RANDOM_NEXT(&RANDOM_STATE) %
			(uint64_t)NEURAL_WEB->WORD_COUNT);

	printf(
		"brain: T=%d d=%d L=%d V=%d params=%lld  mode=%s pid=%d\n",
		POSITION_COUNT, NEURAL_WEB->CONFIGURATION.DIMENSION,
		NEURAL_WEB->CONFIGURATION.LAYER_COUNT, NEURAL_WEB->WORD_COUNT,
		(long long)WEB_PARAMETER_COUNT(NEURAL_WEB), RUN_MODE, (int)getpid()
	);
	fflush(stdout);

	double	START_TIME = THREAD_NOW_SECONDS();
	int		OUTER_INDEX;

	for (OUTER_INDEX = 0; OUTER_INDEX < ITERATIONS; OUTER_INDEX++)
	{
		if (!strcmp(RUN_MODE, "train"))
		{
			int32_t	SEQUENCE[4200];
			float	TOKEN_WEIGHTS[4200];
			int		POSITION;

			for (POSITION = 0; POSITION <= POSITION_COUNT; POSITION++)
			{
				SEQUENCE[POSITION] =
					(int32_t)(RANDOM_NEXT(&RANDOM_STATE) %
						(uint64_t)NEURAL_WEB->WORD_COUNT);
				TOKEN_WEIGHTS[POSITION] = (float)1.0;
			}

			LEARN_WINDOW(
				NEURAL_WEB, SEQUENCE, POSITION_COUNT + 1, TOKEN_WEIGHTS, NULL
			);
		}
		else if (!strcmp(RUN_MODE, "trainupd"))
		{
			int32_t	SEQUENCE[4200];
			float	TOKEN_WEIGHTS[4200];
			int		POSITION;

			for (POSITION = 0; POSITION <= POSITION_COUNT; POSITION++)
			{
				SEQUENCE[POSITION] =
					(int32_t)(RANDOM_NEXT(&RANDOM_STATE) %
						(uint64_t)NEURAL_WEB->WORD_COUNT);
				TOKEN_WEIGHTS[POSITION] = (float)1.0;
			}

			LEARN_WINDOW(
				NEURAL_WEB, SEQUENCE, POSITION_COUNT + 1, TOKEN_WEIGHTS, NULL
			);
			LEARN_UPDATE(NEURAL_WEB);
		}
		else if (!strcmp(RUN_MODE, "gen"))
		{
			int	POSITION;

			for (POSITION = 1; POSITION <= POSITION_COUNT; POSITION++)
			{
				NEURAL_WEB->KERNEL_STATE->START_POSITION = POSITION - 1;
				NEURAL_WEB->KERNEL_STATE->SEQUENCE_LENGTH = POSITION;
				WEB_RUN(NEURAL_WEB, MODE_INFER);
			}
		}
		else
		{
			NEURAL_WEB->KERNEL_STATE->START_POSITION = 0;
			NEURAL_WEB->KERNEL_STATE->SEQUENCE_LENGTH = POSITION_COUNT;
			WEB_RUN(NEURAL_WEB, MODE_INFER);
		}
	}

	double	ELAPSED_TIME = THREAD_NOW_SECONDS() - START_TIME;

	printf(
		"%s: %.3f s per window of %d tokens = %.0f tok/s (single thread)\n",
		RUN_MODE, ELAPSED_TIME / ITERATIONS, POSITION_COUNT,
		ITERATIONS * POSITION_COUNT / ELAPSED_TIME
	);

	return (0);
}
