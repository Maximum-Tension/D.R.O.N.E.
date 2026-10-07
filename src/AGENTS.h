#ifndef AGENTS_H
#define AGENTS_H
#include <stdio.h>

/* Lucy's agents share one program: an input agent that keeps reading what
 * the user types, the conversation agent that answers, and job agents that do
 * long work (studying a file) on their own threads.  They talk through
 * mailboxes, so nothing typed while Lucy is busy gets lost. */

typedef struct
{
	void	*LOCK;
	char	**ITEMS;
	double	*TIMES;
	int		HEAD;
	int		COUNT;
	int		CAPACITY;
	int		IS_CLOSED;
} MAILBOX;

enum
{
	JOB_RUNNING = 0,
	JOB_FINISHED,
	JOB_STOPPED,
	JOB_FAILED
};

typedef struct JOB JOB;
typedef void (*JOB_FUNCTION)(JOB *WORK);

struct JOB
{
	int				ID;
	char			KIND[32];
	char			TITLE[200];
	volatile int	STATE;
	volatile int	CANCEL;
	volatile int	DONE;
	volatile int	REPORTED;
	int				PROGRESS;
	int				TOTAL;
	double			STARTED;
	char			RESULT[3000];
	MAILBOX			*INBOX;
	JOB_FUNCTION	RUN;
	void			*DATA;
	void			*OWNER;
};

typedef struct
{
	void	*LOCK;
	JOB		*JOBS[32];
	int		COUNT;
	int		NEXT_ID;
} JOB_BOARD;

MAILBOX		*MAILBOX_NEW(void);
void		MAILBOX_FREE(MAILBOX *BOX);
void		MAILBOX_PUT(MAILBOX *BOX, const char *TEXT);
char		*MAILBOX_TAKE(MAILBOX *BOX);
char		*MAILBOX_WAIT(MAILBOX *BOX, int MILLISECONDS);
char		*MAILBOX_WAIT_PASTE(MAILBOX *BOX, int MILLISECONDS);
int			MAILBOX_PEEK(MAILBOX *BOX, char *OUTPUT, int OUTPUT_SIZE);
int			MAILBOX_COUNT(MAILBOX *BOX);
void		MAILBOX_CLOSE(MAILBOX *BOX);
int			MAILBOX_IS_DONE(MAILBOX *BOX);
char		*READ_WHOLE_LINE(FILE *STREAM);
void		INPUT_AGENT_START(MAILBOX *BOX, FILE *STREAM);
JOB_BOARD	*JOB_BOARD_NEW(void);
JOB			*JOB_START(
	JOB_BOARD *BOARD, const char *KIND, const char *TITLE, JOB_FUNCTION RUN,
	void *DATA, void *OWNER, int ON_THREAD
);
int			JOB_RUNNING_COUNT(JOB_BOARD *BOARD);
int			JOB_STOP_ALL(JOB_BOARD *BOARD);
int			JOB_KIND_COUNT(JOB_BOARD *BOARD, const char *KIND);
int			JOB_STOP_KIND(JOB_BOARD *BOARD, const char *KIND);
JOB			*JOB_TAKE_FINISHED(JOB_BOARD *BOARD);
void		JOB_LIST(JOB_BOARD *BOARD, char *OUTPUT, int OUTPUT_SIZE);
int			IS_STOP_MESSAGE(const char *TEXT);
int			IS_MORE_INFORMATION(const char *TEXT);

#endif
