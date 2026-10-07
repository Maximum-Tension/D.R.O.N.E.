#ifndef SKILLS_H
#define SKILLS_H
#include "SKILL.h"

/* Lucy's skills: the libraries in mind/skills, loaded when she starts and
 * again whenever one is added or changed (no restart).  Each library is
 * loaded from a copy, so the file itself can be replaced while she runs. */

#define MAX_SKILLS 32

typedef struct
{
	char				NAME[128];
	char				PATH[600];
	char				LOADED_PATH[700];
	long long			STAMP;
	void				*LIBRARY;
	const SKILL_MEMORY	*MEMORY;
} LOADED_SKILL;

typedef struct
{
	char			FOLDER[512];
	LOADED_SKILL	SKILLS[MAX_SKILLS];
	int				COUNT;
	SKILL_HOST		HOST;
	int				COPY_NUMBER;
} SKILL_SET;

int					SKILLS_OPEN(SKILL_SET *SET, const char *FOLDER);
int					SKILLS_REFRESH(SKILL_SET *SET, char *NEWS, int NEWS_SIZE);
void				SKILLS_CLOSE(SKILL_SET *SET);
const SKILL_ACTION	*SKILLS_FIND_ACTION(
	SKILL_SET *SET, const char *NAME, const SKILL_MEMORY **OWNER
);
int					SKILLS_RUN(
	SKILL_SET *SET, const char *NAME, const char *ARGUMENT, char *RESULT,
	int RESULT_SIZE
);
int					SKILLS_ANSWER(
	SKILL_SET *SET, const char *MESSAGE, char *ACTION, int ACTION_SIZE,
	char *REPLY, int REPLY_SIZE
);
int					SKILLS_SAY(
	const SKILL_ACTION *ACTION, const char *ARGUMENT, const char *RESULT,
	char *OUTPUT, int OUTPUT_SIZE
);
int					SKILLS_MATCH(
	SKILL_SET *SET, const char *MESSAGE, const char *const *KNOWN_ACTIONS,
	const SKILL_ACTION **ACTION, char *ARGUMENT, int ARGUMENT_SIZE
);
void				SKILLS_LIST(SKILL_SET *SET, char *OUTPUT, int OUTPUT_SIZE);

#endif
