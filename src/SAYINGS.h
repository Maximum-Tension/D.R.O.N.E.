#ifndef SAYINGS_H
#define SAYINGS_H

/* Lucy's own fixed sentences: what she says when she works something out
 * herself (a reminder set, a package missing, how she found an answer).
 * They live in mind/language/sayings.txt, one per line:
 *
 *     reminder set: Okay, I'll send you a message {1}.
 *
 * {1}, {2} ... are the details of the moment.  A key may have several
 * lines; Lucy takes turns with them.  The file is read again when it
 * changes, so an edit is used from the next message on, and a key the file
 * doesn't have is said the way the code says it. */

typedef struct
{
	char	KEY[48];
	char	*TEXT;
	int		USED;
} SAYING_LINE;

typedef struct
{
	char		PATH[600];
	long long	STAMP;
	SAYING_LINE	*LINES;
	int			COUNT;
	int			CAPACITY;
} SAYINGS;

int			SAYINGS_OPEN(SAYINGS *SET, const char *PATH);
int			SAYINGS_REFRESH(SAYINGS *SET);
void		SAYINGS_CLOSE(SAYINGS *SET);
const char	*SAYINGS_FIND(SAYINGS *SET, const char *KEY);
int			SAYINGS_SAY(
	SAYINGS *SET, const char *KEY, const char *DEFAULT,
	const char *const *DETAILS, int DETAIL_COUNT, char *OUTPUT, int OUTPUT_SIZE
);
void		SAYINGS_FILL(
	const char *TEMPLATE, const char *const *DETAILS, int DETAIL_COUNT,
	char *OUTPUT, int OUTPUT_SIZE
);

#endif
