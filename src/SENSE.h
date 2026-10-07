#ifndef SENSE_H
#define SENSE_H

enum
{
	SENSE_STEP_ACTION = 1,
	SENSE_STEP_THOUGHT = 2
};

enum
{
	SENSE_MODE_CHAT = 0,
	SENSE_MODE_NETWORK = 1,
	SENSE_MODE_DONE = 2
};

typedef struct
{
	void	*USER_DATA;
	int		(*PATH_KIND)(void *USER_DATA, const char *RELATIVE_PATH);
	int		(*FIND_BY_BASE)(
		void *USER_DATA, const char *BASE_NAME, char *OUTPUT, int OUTPUT_SIZE
	);
	int		(*WORD_KNOWN)(void *USER_DATA, const char *QUERY_WORD);
	int		(*ABOUT_PHRASE)(
		void *USER_DATA, const char *QUERY_PHRASE, char *OUTPUT, int OUTPUT_SIZE
	);
	int		(*NEAR_NAME)(
		void *USER_DATA, const char *QUERY_NAME, int WANTED_KIND, char *OUTPUT,
		int OUTPUT_SIZE
	);
	int		(*LIST_FOLDER)(
		void *USER_DATA, const char *FOLDER, char *OUTPUT, int OUTPUT_SIZE
	);
	/* the user's messages from the chat history, from `from` days ago to `to`
	   days ago (0 = today); from = -1: everything before this session; from =
	   -2: today before this session. Returns how many. */
	int	(*USER_HISTORY)(
		void *USER_DATA, int FROM_DAYS_AGO, int TO_DAYS_AGO, char *OUTPUT,
		int OUTPUT_SIZE
	);
	/* 1 if a line the user taught (memory, not the built-in knowledge pack)
	 * holds this word */
	int	(*IN_MEMORY)(void *USER_DATA, const char *QUERY_WORD);
} SENSE_WORLD;

typedef struct
{
	int		KIND;
	char	ACTION[320];
	char	RESULT[320];
} SENSE_STEP;

typedef struct SENSE SENSE;

SENSE	*SENSE_CREATE(const char *VERBS_PATH);
void	SENSE_DESTROY(SENSE *SENSE_STATE);
void	SENSE_RESET(SENSE *SENSE_STATE);
void	SENSE_HEAR(
	SENSE *SENSE_STATE, const SENSE_WORLD *WORLD, const char *HEARD_TEXT
);
void	SENSE_HEAR_PART(
	SENSE *SENSE_STATE, const SENSE_WORLD *WORLD, const char *HEARD_TEXT,
	int IS_FIRST_PART
);
void	SENSE_END_MESSAGE(SENSE *SENSE_STATE, const char *WHOLE_MESSAGE);
SENSE	*SENSE_CLONE(const SENSE *SENSE_STATE);
void	SENSE_COPY(SENSE *DESTINATION, const SENSE *SOURCE);
int		SENSE_PENDING_COUNT(SENSE *SENSE_STATE);
/* 1 if the message asks what the AI answered to an earlier question ("earlier I
 * asked X? What did you say?"): keep it whole */
int	SENSE_IS_ASKED_FORM(SENSE *SENSE_STATE, const char *MESSAGE_TEXT);
/* while on, a heard text is one part of a longer message: copy-back commands
 * are left to the voice */
void		SENSE_SET_PART_MODE(SENSE *SENSE_STATE, int ENABLED);
int			SENSE_USER_ACT(SENSE *SENSE_STATE);
int			SENSE_SAVE_PROFILE(SENSE *SENSE_STATE, const char *PATH);
int			SENSE_LOAD_PROFILE(SENSE *SENSE_STATE, const char *PATH);
void		SENSE_LEARN_FACT(SENSE *SENSE_STATE, const char *FACT_TEXT);
int			SENSE_IS_JUSTIFIED(
	SENSE *SENSE_STATE, const char *USER_TEXT, const char *VERB_CLASS
);
const char	*SENSE_VERB_CLASS(const char *VERB_NAME);
int			SENSE_IS_FAITHFUL(
	const char *CONTEXT_TEXT, const char *ACTION_TEXT
);
int			SENSE_IS_NEAR_NAME(const char *FIRST_NAME, const char *SECOND_NAME);
void		SENSE_OBSERVE(
	SENSE *SENSE_STATE, const char *USER_LINE, const char **ACTIONS,
	const char **ACTION_RESULTS, int ACTION_COUNT, const char *SPOKEN_LINE
);
int			SENSE_NEXT_STEP(
	SENSE *SENSE_STATE, const SENSE_WORLD *WORLD, SENSE_STEP *STEP
);
void		SENSE_TAKE_RESULT(SENSE *SENSE_STATE, const char *RESULT_TEXT);
int			SENSE_GET_MODE(
	SENSE *SENSE_STATE, char *NETWORK_VERBS, int OUTPUT_SIZE
);
void		SENSE_SET_SAID_ABOUT(SENSE *SENSE_STATE, const char *ABOUT);
void		SENSE_SAID(SENSE *SENSE_STATE, const char *SAID_TEXT);
void		SENSE_DID(
	SENSE *SENSE_STATE, const char *ACTION_TEXT, const char *RESULT_TEXT,
	int BY_NETWORK
);
int			SENSE_VERB_COUNT(SENSE *SENSE_STATE);
void		SENSE_SET_NAME(SENSE *SENSE_STATE, const char *AI_NAME);
int			SENSE_USER_NAME(SENSE *SENSE_STATE, char *OUTPUT, int OUTPUT_SIZE);
int			SENSE_PROFILE_VALUE(
	SENSE *SENSE_STATE, const char *SUBJECT, const char *ATTRIBUTE,
	char *OUTPUT, int OUTPUT_SIZE
);
int			SENSE_PROFILE_ABOUT(
	SENSE *SENSE_STATE, const char *OWNER, char *OUTPUT, int OUTPUT_SIZE
);
int			SENSE_LACKING_COUNT(SENSE *SENSE_STATE);
const char	*SENSE_LACKING_WORD(SENSE *SENSE_STATE, int LACK_INDEX);
void		SENSE_GOAL_TEXT(SENSE *SENSE_STATE, char *OUTPUT, int OUTPUT_SIZE);
int			SENSE_WORD_ANSWER(
	SENSE *SENSE_STATE, const char *EXPRESSION, char *OUTPUT, int OUTPUT_SIZE
);
int			SENSE_HISTORY_COUNT(SENSE *SENSE_STATE);
int			SENSE_HISTORY_AT(
	SENSE *SENSE_STATE, int HISTORY_INDEX, char *USER_OUTPUT, int USER_SIZE,
	char *SAID_OUTPUT, int SAID_SIZE
);
int			SENSE_NEW_FACTS(SENSE *SENSE_STATE, char *OUTPUT, int OUTPUT_SIZE);
int			SENSE_RUN_SCRIPT(
	const char *SCRIPT_TEXT, int SHOW_OUTPUT, int *CHECK_COUNT_OUTPUT
);
int			SENSE_RUN_SCRIPT_FILE(const char *PATH, int SHOW_DETAILS);
int			SENSE_FILTER_CHATS(
	const char *INPUT_PATH, const char *OUTPUT_PATH, int SHOW_DROPS
);

#endif
