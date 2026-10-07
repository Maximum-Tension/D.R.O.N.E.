#include "AGENTS.h"
#include "THREAD.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

MAILBOX
	*MAILBOX_NEW(void)
{
	MAILBOX	*BOX = calloc(1, sizeof(MAILBOX));

	if (!BOX)
		return (NULL);

	BOX->LOCK = THREAD_NEW_MUTEX();
	BOX->CAPACITY = 64;
	BOX->ITEMS = calloc(BOX->CAPACITY, sizeof(char *));
	BOX->TIMES = calloc(BOX->CAPACITY, sizeof(double));

	return (BOX);
}

void
	MAILBOX_FREE(MAILBOX *BOX)
{
	char	*TEXT;

	if (!BOX)
		return ;

	while ((TEXT = MAILBOX_TAKE(BOX)))
		free(TEXT);

	free(BOX->ITEMS);
	free(BOX->TIMES);
	free(BOX);
}

void
	MAILBOX_PUT(MAILBOX *BOX, const char *TEXT)
{
	char	*COPY;

	if (!BOX || !TEXT)
		return ;

	COPY = malloc(strlen(TEXT) + 1);

	if (!COPY)
		return ;

	strcpy(COPY, TEXT);
	THREAD_LOCK(BOX->LOCK);

	if (BOX->COUNT == BOX->CAPACITY)
	{
		char	**BIGGER = calloc(BOX->CAPACITY * 2, sizeof(char *));
		double	*LATER = calloc(BOX->CAPACITY * 2, sizeof(double));
		int		INDEX;

		if (!BIGGER || !LATER)
		{
			THREAD_UNLOCK(BOX->LOCK);
			free(BIGGER);
			free(LATER);
			free(COPY);
			return ;
		}

		for (INDEX = 0; INDEX < BOX->COUNT; INDEX++)
		{
			BIGGER[INDEX] = BOX->ITEMS[(BOX->HEAD + INDEX) % BOX->CAPACITY];
			LATER[INDEX] = BOX->TIMES[(BOX->HEAD + INDEX) % BOX->CAPACITY];
		}

		free(BOX->ITEMS);
		free(BOX->TIMES);
		BOX->ITEMS = BIGGER;
		BOX->TIMES = LATER;
		BOX->HEAD = 0;
		BOX->CAPACITY *= 2;
	}

	BOX->ITEMS[(BOX->HEAD + BOX->COUNT) % BOX->CAPACITY] = COPY;
	BOX->TIMES[(BOX->HEAD + BOX->COUNT) % BOX->CAPACITY] = THREAD_NOW_SECONDS();
	BOX->COUNT++;
	THREAD_UNLOCK(BOX->LOCK);
}

/* the oldest message, or NULL when the mailbox is empty; the caller frees it */
char
	*MAILBOX_TAKE(MAILBOX *BOX)
{
	char	*TEXT = NULL;

	if (!BOX)
		return (NULL);

	THREAD_LOCK(BOX->LOCK);

	if (BOX->COUNT)
	{
		TEXT = BOX->ITEMS[BOX->HEAD];
		BOX->ITEMS[BOX->HEAD] = NULL;
		BOX->HEAD = (BOX->HEAD + 1) % BOX->CAPACITY;
		BOX->COUNT--;
	}

	THREAD_UNLOCK(BOX->LOCK);

	return (TEXT);
}

/* waits up to MILLISECONDS for a message (a negative wait never gives up
 * until the mailbox is closed and empty) */
char
	*MAILBOX_WAIT(MAILBOX *BOX, int MILLISECONDS)
{
	int	WAITED = 0;

	for (;;)
	{
		char	*TEXT = MAILBOX_TAKE(BOX);

		if (TEXT)
			return (TEXT);

		if (MAILBOX_IS_DONE(BOX))
			return (NULL);

		if (MILLISECONDS >= 0 && WAITED >= MILLISECONDS)
			return (NULL);

		THREAD_SLEEP_MILLISECONDS(15);
		WAITED += 15;
	}
}

/* the oldest message and the moment it came in */
static char
	*TAKE_TIMED(MAILBOX *BOX, double *TIME)
{
	char	*TEXT = NULL;

	THREAD_LOCK(BOX->LOCK);

	if (BOX->COUNT)
	{
		TEXT = BOX->ITEMS[BOX->HEAD];
		*TIME = BOX->TIMES[BOX->HEAD];
		BOX->ITEMS[BOX->HEAD] = NULL;
		BOX->HEAD = (BOX->HEAD + 1) % BOX->CAPACITY;
		BOX->COUNT--;
	}

	THREAD_UNLOCK(BOX->LOCK);

	return (TEXT);
}

/* like MAILBOX_WAIT, but lines that came in one burst (a pasted text: no one
 * types two lines within a few milliseconds) come back as one message, joined
 * by line breaks */
char
	*MAILBOX_WAIT_PASTE(MAILBOX *BOX, int MILLISECONDS)
{
	char	*TEXT = NULL;
	double	LAST_TIME = 0;
	int		WAITED = 0;
	int		QUIET_ROUNDS = 0;

	if (!BOX)
		return (NULL);

	while (!(TEXT = TAKE_TIMED(BOX, &LAST_TIME)))
	{
		if (MAILBOX_IS_DONE(BOX))
			return (NULL);

		if (MILLISECONDS >= 0 && WAITED >= MILLISECONDS)
			return (NULL);

		THREAD_SLEEP_MILLISECONDS(15);
		WAITED += 15;
	}

	/* more lines of the same burst may still be on their way */
	while (QUIET_ROUNDS < 5)
	{
		char	*MORE = NULL;
		double	MORE_TIME = 0;
		int		IS_SEPARATE = 0;

		THREAD_LOCK(BOX->LOCK);

		if (BOX->COUNT && BOX->TIMES[BOX->HEAD] - LAST_TIME < 0.05)
		{
			/* a command on its own line is never part of a paste */
			if (BOX->ITEMS[BOX->HEAD][0] == '/')
				IS_SEPARATE = 1;
			else
			{
				MORE = BOX->ITEMS[BOX->HEAD];
				MORE_TIME = BOX->TIMES[BOX->HEAD];
				BOX->ITEMS[BOX->HEAD] = NULL;
				BOX->HEAD = (BOX->HEAD + 1) % BOX->CAPACITY;
				BOX->COUNT--;
			}
		}
		else if (BOX->COUNT)
			IS_SEPARATE = 1;

		THREAD_UNLOCK(BOX->LOCK);

		if (IS_SEPARATE)
			break ;

		if (!MORE)
		{
			QUIET_ROUNDS++;
			THREAD_SLEEP_MILLISECONDS(10);
			continue ;
		}

		QUIET_ROUNDS = 0;
		LAST_TIME = MORE_TIME;

		char	*JOINED = malloc(strlen(TEXT) + strlen(MORE) + 2);

		if (JOINED)
		{
			sprintf(JOINED, "%s\n%s", TEXT, MORE);
			free(TEXT);
			TEXT = JOINED;
		}

		free(MORE);
	}

	return (TEXT);
}

/* a copy of the oldest message, left in the mailbox */
int
	MAILBOX_PEEK(MAILBOX *BOX, char *OUTPUT, int OUTPUT_SIZE)
{
	int	FOUND = 0;

	if (!BOX || OUTPUT_SIZE <= 0)
		return (0);

	OUTPUT[0] = 0;
	THREAD_LOCK(BOX->LOCK);

	if (BOX->COUNT)
	{
		snprintf(OUTPUT, OUTPUT_SIZE, "%s", BOX->ITEMS[BOX->HEAD]);
		FOUND = 1;
	}

	THREAD_UNLOCK(BOX->LOCK);

	return (FOUND);
}

int
	MAILBOX_COUNT(MAILBOX *BOX)
{
	int	COUNT;

	if (!BOX)
		return (0);

	THREAD_LOCK(BOX->LOCK);
	COUNT = BOX->COUNT;
	THREAD_UNLOCK(BOX->LOCK);

	return (COUNT);
}

void
	MAILBOX_CLOSE(MAILBOX *BOX)
{
	if (!BOX)
		return ;

	THREAD_LOCK(BOX->LOCK);
	BOX->IS_CLOSED = 1;
	THREAD_UNLOCK(BOX->LOCK);
}

/* closed and nothing left to read */
int
	MAILBOX_IS_DONE(MAILBOX *BOX)
{
	int	IS_DONE;

	if (!BOX)
		return (1);

	THREAD_LOCK(BOX->LOCK);
	IS_DONE = BOX->IS_CLOSED && !BOX->COUNT;
	THREAD_UNLOCK(BOX->LOCK);

	return (IS_DONE);
}

/* one line of any length, without its line break; NULL at the end of the
 * stream.  The caller frees it. */
char
	*READ_WHOLE_LINE(FILE *STREAM)
{
	size_t	CAPACITY = 4096;
	size_t	LENGTH = 0;
	char	*LINE = malloc(CAPACITY);

	if (!LINE)
		return (NULL);

	for (;;)
	{
		if (!fgets(LINE + LENGTH, (int)(CAPACITY - LENGTH), STREAM))
		{
			if (!LENGTH)
			{
				free(LINE);
				return (NULL);
			}

			break ;
		}

		LENGTH += strlen(LINE + LENGTH);

		if (LENGTH && LINE[LENGTH - 1] == '\n')
			break ;

		if (LENGTH + 1 < CAPACITY)
			continue ;

		/* no room left and no line break yet: the line goes on */
		char	*BIGGER = realloc(LINE, CAPACITY * 2);

		if (!BIGGER)
			break ;

		LINE = BIGGER;
		CAPACITY *= 2;
	}

	while (
		LENGTH &&
		(
			LINE[LENGTH - 1] == '\n' ||
			LINE[LENGTH - 1] == '\r' ||
			LINE[LENGTH - 1] == ' '
		)
	)
		LINE[--LENGTH] = 0;

	return (LINE);
}

typedef struct
{
	MAILBOX	*BOX;
	FILE	*STREAM;
} INPUT_AGENT_PACK;

/* the input agent: reads every line the user types, even while Lucy is
 * busy, and posts it to the conversation agent's mailbox */
static void
	INPUT_AGENT(void *ARGUMENT)
{
	INPUT_AGENT_PACK	*PACK = ARGUMENT;
	char				*LINE;

	while ((LINE = READ_WHOLE_LINE(PACK->STREAM)))
	{
		MAILBOX_PUT(PACK->BOX, LINE);
		free(LINE);
	}

	MAILBOX_CLOSE(PACK->BOX);
	free(PACK);
}

void
	INPUT_AGENT_START(MAILBOX *BOX, FILE *STREAM)
{
	INPUT_AGENT_PACK	*PACK = malloc(sizeof(INPUT_AGENT_PACK));

	if (!PACK)
		return ;

	PACK->BOX = BOX;
	PACK->STREAM = STREAM;
	THREAD_SPAWN(INPUT_AGENT, PACK);
}

JOB_BOARD
	*JOB_BOARD_NEW(void)
{
	JOB_BOARD	*BOARD = calloc(1, sizeof(JOB_BOARD));

	if (!BOARD)
		return (NULL);

	BOARD->LOCK = THREAD_NEW_MUTEX();
	BOARD->NEXT_ID = 1;

	return (BOARD);
}

static void
	JOB_AGENT(void *ARGUMENT)
{
	JOB	*WORK = ARGUMENT;

	WORK->RUN(WORK);

	if (WORK->STATE == JOB_RUNNING)
	{
		if (WORK->CANCEL)
			WORK->STATE = JOB_STOPPED;
		else
			WORK->STATE = JOB_FINISHED;
	}

	WORK->DONE = 1;
}

/* a job agent: RUN does the work, on its own thread when ON_THREAD (else
 * right here, so scripts and tests stay in order) */
JOB
	*JOB_START(
		JOB_BOARD *BOARD, const char *KIND, const char *TITLE, JOB_FUNCTION RUN,
		void *DATA, void *OWNER, int ON_THREAD
	)
{
	JOB	*WORK;
	int	INDEX;

	if (!BOARD)
		return (NULL);

	WORK = calloc(1, sizeof(JOB));

	if (!WORK)
		return (NULL);

	snprintf(WORK->KIND, sizeof WORK->KIND, "%s", KIND);
	snprintf(WORK->TITLE, sizeof WORK->TITLE, "%s", TITLE);
	WORK->RUN = RUN;
	WORK->DATA = DATA;
	WORK->OWNER = OWNER;
	WORK->INBOX = MAILBOX_NEW();
	WORK->STARTED = THREAD_NOW_SECONDS();
	THREAD_LOCK(BOARD->LOCK);

	/* a finished job that was already reported makes room */
	if (BOARD->COUNT == 32)
		for (INDEX = 0; INDEX < BOARD->COUNT; INDEX++)
			if (BOARD->JOBS[INDEX]->DONE && BOARD->JOBS[INDEX]->REPORTED)
			{
				MAILBOX_FREE(BOARD->JOBS[INDEX]->INBOX);
				free(BOARD->JOBS[INDEX]);
				BOARD->JOBS[INDEX] = BOARD->JOBS[--BOARD->COUNT];
				break ;
			}

	if (BOARD->COUNT == 32)
	{
		THREAD_UNLOCK(BOARD->LOCK);
		MAILBOX_FREE(WORK->INBOX);
		free(WORK);
		return (NULL);
	}

	WORK->ID = BOARD->NEXT_ID++;
	BOARD->JOBS[BOARD->COUNT++] = WORK;
	THREAD_UNLOCK(BOARD->LOCK);

	if (ON_THREAD)
		THREAD_SPAWN(JOB_AGENT, WORK);
	else
		JOB_AGENT(WORK);

	return (WORK);
}

/* jobs at work (a reminder only waits for its time, so it is not one) */
int
	JOB_RUNNING_COUNT(JOB_BOARD *BOARD)
{
	int	COUNT = 0;
	int	INDEX;

	if (!BOARD)
		return (0);

	THREAD_LOCK(BOARD->LOCK);

	for (INDEX = 0; INDEX < BOARD->COUNT; INDEX++)
		if (
			!BOARD->JOBS[INDEX]->DONE &&
			strcmp(BOARD->JOBS[INDEX]->KIND, "reminder")
		)
			COUNT++;

	THREAD_UNLOCK(BOARD->LOCK);

	return (COUNT);
}

/* jobs of one kind still waiting or working ("reminder") */
int
	JOB_KIND_COUNT(JOB_BOARD *BOARD, const char *KIND)
{
	int	COUNT = 0;
	int	INDEX;

	if (!BOARD)
		return (0);

	THREAD_LOCK(BOARD->LOCK);

	for (INDEX = 0; INDEX < BOARD->COUNT; INDEX++)
		if (
			!BOARD->JOBS[INDEX]->DONE &&
			!strcmp(BOARD->JOBS[INDEX]->KIND, KIND)
		)
			COUNT++;

	THREAD_UNLOCK(BOARD->LOCK);

	return (COUNT);
}

/* asks the jobs of one kind to stop ("cancel the reminder") */
int
	JOB_STOP_KIND(JOB_BOARD *BOARD, const char *KIND)
{
	int	COUNT = 0;
	int	INDEX;

	if (!BOARD)
		return (0);

	THREAD_LOCK(BOARD->LOCK);

	for (INDEX = 0; INDEX < BOARD->COUNT; INDEX++)
		if (
			!BOARD->JOBS[INDEX]->DONE &&
			!strcmp(BOARD->JOBS[INDEX]->KIND, KIND)
		)
		{
			BOARD->JOBS[INDEX]->CANCEL = 1;
			BOARD->JOBS[INDEX]->REPORTED = 1;
			COUNT++;
		}

	THREAD_UNLOCK(BOARD->LOCK);

	return (COUNT);
}

/* asks every job at work to stop (reminders keep waiting); it stops at its
 * next step */
int
	JOB_STOP_ALL(JOB_BOARD *BOARD)
{
	int	COUNT = 0;
	int	INDEX;

	if (!BOARD)
		return (0);

	THREAD_LOCK(BOARD->LOCK);

	for (INDEX = 0; INDEX < BOARD->COUNT; INDEX++)
		if (
			!BOARD->JOBS[INDEX]->DONE &&
			strcmp(BOARD->JOBS[INDEX]->KIND, "reminder")
		)
		{
			BOARD->JOBS[INDEX]->CANCEL = 1;
			COUNT++;
		}

	THREAD_UNLOCK(BOARD->LOCK);

	return (COUNT);
}

/* a finished job nobody has heard about yet */
JOB
	*JOB_TAKE_FINISHED(JOB_BOARD *BOARD)
{
	JOB	*FOUND = NULL;
	int	INDEX;

	if (!BOARD)
		return (NULL);

	THREAD_LOCK(BOARD->LOCK);

	for (INDEX = 0; INDEX < BOARD->COUNT && !FOUND; INDEX++)
		if (BOARD->JOBS[INDEX]->DONE && !BOARD->JOBS[INDEX]->REPORTED)
		{
			FOUND = BOARD->JOBS[INDEX];
			FOUND->REPORTED = 1;
		}

	THREAD_UNLOCK(BOARD->LOCK);

	return (FOUND);
}

void
	JOB_LIST(JOB_BOARD *BOARD, char *OUTPUT, int OUTPUT_SIZE)
{
	static const char	*STATE_NAMES[4] = {
		"working", "finished", "stopped", "failed"
	};
	int					POSITION = 0;
	int					INDEX;

	OUTPUT[0] = 0;

	if (!BOARD)
		return ;

	THREAD_LOCK(BOARD->LOCK);

	for (INDEX = 0; INDEX < BOARD->COUNT && POSITION < OUTPUT_SIZE; INDEX++)
	{
		JOB		*WORK = BOARD->JOBS[INDEX];
		double	SECONDS = THREAD_NOW_SECONDS() - WORK->STARTED;

		POSITION += snprintf(
			OUTPUT + POSITION, OUTPUT_SIZE - POSITION,
			"  [job %d] %s: %s, %d of %d done, %.0f s\n", WORK->ID, WORK->TITLE,
			STATE_NAMES[WORK->DONE ? WORK->STATE : JOB_RUNNING], WORK->PROGRESS,
			WORK->TOTAL, SECONDS
		);
	}

	THREAD_UNLOCK(BOARD->LOCK);

	if (!OUTPUT[0])
		snprintf(OUTPUT, OUTPUT_SIZE, "  [jobs] none\n");
}

/* lower case words with single spaces and no punctuation, without "please",
 * "lucy" or "ok" around them */
static void
	PLAIN_WORDS(const char *TEXT, char *OUTPUT, int OUTPUT_SIZE)
{
	static const char	*AROUND[11] = {
		"please", "pls", "lucy", "code", "ok", "okay", "hey", "now", "just",
		"oh", NULL
	};
	char				BUFFER[400];
	int					POSITION = 0;
	int					INDEX;

	for (INDEX = 0; TEXT[INDEX] && POSITION < (int)sizeof BUFFER - 2; INDEX++)
	{
		unsigned char	CHARACTER = (unsigned char)TEXT[INDEX];

		if (isalnum(CHARACTER) || CHARACTER == '\'')
			BUFFER[POSITION++] = (char)tolower(CHARACTER);
		else if (POSITION && BUFFER[POSITION - 1] != ' ')
			BUFFER[POSITION++] = ' ';
	}

	while (POSITION && BUFFER[POSITION - 1] == ' ')
		POSITION--;

	BUFFER[POSITION] = 0;

	/* drop the polite and naming words at both ends */
	char	*START = BUFFER;
	int		CHANGED = 1;

	while (CHANGED)
	{
		CHANGED = 0;

		for (INDEX = 0; AROUND[INDEX]; INDEX++)
		{
			size_t	LENGTH = strlen(AROUND[INDEX]);
			size_t	TOTAL = strlen(START);

			if (
				!strncmp(START, AROUND[INDEX], LENGTH) &&
				(START[LENGTH] == ' ' || !START[LENGTH]) &&
				TOTAL > LENGTH
			)
			{
				START += LENGTH + (START[LENGTH] == ' ');
				CHANGED = 1;
			}
			else if (
				TOTAL > LENGTH + 1 &&
				!strcmp(START + TOTAL - LENGTH, AROUND[INDEX]) &&
				START[TOTAL - LENGTH - 1] == ' '
			)
			{
				START[TOTAL - LENGTH - 1] = 0;
				CHANGED = 1;
			}
		}
	}

	snprintf(OUTPUT, OUTPUT_SIZE, "%s", START);
}

/* "stop", "wait", "never mind", "cancel that", "stop reading" */
int
	IS_STOP_MESSAGE(const char *TEXT)
{
	static const char	*STOP_PHRASES[25] = {
		"stop", "wait", "cancel", "never mind", "nevermind", "nvm", "hold on",
		"halt", "abort", "enough", "forget it", "stop it", "stop that",
		"cancel that", "stop reading", "stop studying", "stop working",
		"stop the job", "cancel the job", "stop thinking", "wait stop",
		"stop stop", "that's enough", "thats enough", NULL
	};
	char				PLAIN[400];
	int					INDEX;

	PLAIN_WORDS(TEXT, PLAIN, sizeof PLAIN);

	for (INDEX = 0; STOP_PHRASES[INDEX]; INDEX++)
		if (!strcmp(PLAIN, STOP_PHRASES[INDEX]))
			return (1);

	return (0);
}

/* "also, ...", "oh and ...", "I forgot to say ...": more for what Lucy is
 * already working on */
int
	IS_MORE_INFORMATION(const char *TEXT)
{
	static const char	*STARTS[20] = {
		"also ", "and also ", "and ", "plus ", "btw ", "by the way ",
		"one more thing", "i forgot to say", "i forgot to mention",
		"i forgot to tell", "i should add", "i should mention", "ps ", "p s ",
		"another thing", "i mean ", "to add ", "adding ", "more info", NULL
	};
	char				PLAIN[400];
	int					INDEX;

	PLAIN_WORDS(TEXT, PLAIN, sizeof PLAIN);

	for (INDEX = 0; STARTS[INDEX]; INDEX++)
		if (!strncmp(PLAIN, STARTS[INDEX], strlen(STARTS[INDEX])))
			return (1);

	return (0);
}
