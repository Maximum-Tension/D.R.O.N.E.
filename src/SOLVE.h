#ifndef SOLVE_H
#define SOLVE_H

/* Equations with unknowns, written as math ("[Bat] + [Ball] = 1.10",
 * "2x + 3 = 7") or as a word problem ("A bat and a ball cost $1.10 together.
 * The bat costs $1 more than the ball."), kept across a few messages and
 * solved exactly when someone asks for one of the unknowns. */

#define SOLVE_MAX_NAMES 8
#define SOLVE_MAX_ROWS 12

typedef struct
{
	char	NAMES[SOLVE_MAX_NAMES][48];
	char	SHOWN[SOLVE_MAX_NAMES][56];
	int		NAME_COUNT;
	double	ROWS[SOLVE_MAX_ROWS][SOLVE_MAX_NAMES + 1];
	int		ROW_COUNT;
	int		IS_MONEY;
	int		IS_WORDS;
	char	VERB[16];
	char	UNIT[32];
	int		AGE;
	char	CHECKED[600];
} SOLVE_STORE;

void	SOLVE_CLEAR(SOLVE_STORE *STORE);
int		SOLVE_READ(SOLVE_STORE *STORE, const char *TEXT);
int		SOLVE_ASKED(SOLVE_STORE *STORE, const char *TEXT, int *WHICH);
int		SOLVE_ANSWER(
	SOLVE_STORE *STORE, int WHICH, char *OUTPUT, int OUTPUT_SIZE
);
void	SOLVE_TICK(SOLVE_STORE *STORE);
int		SOLVE_CHECK(SOLVE_STORE *STORE, char *OUTPUT, int OUTPUT_SIZE);
int		SOLVE_EVALUATE(
	SOLVE_STORE *STORE, const char *TEXT, char *OUTPUT, int OUTPUT_SIZE
);
int		SOLVE_BIG(
	const char *TEXT, char *EXPRESSION, int EXPRESSION_SIZE, char *RESULT,
	int RESULT_SIZE
);
char	*FIND_NO_CASE(const char *TEXT, const char *WORD);

#endif
