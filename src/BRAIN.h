#ifndef BRAIN_H
#define BRAIN_H
#include "CODE.h"
#include "LANGUAGE.h"

#include "PATHS.h"

typedef struct
{
	WEB			*NEURAL_WEB;
	LANGUAGE	*LEXICON;
} BRAIN;

int		MAKE_DIRECTORY(const char *PATH);
int		MAKE_DIRECTORY_PATH(const char *PATH);
int		FILE_EXISTS(const char *PATH);
int		COPY_FILE_CONTENTS(
	const char *SOURCE_PATH, const char *DESTINATION_PATH
);
int		MIND_MIGRATE(char *NEWS, int NEWS_SIZE);
int		BRAIN_OPEN(BRAIN *LOADED_BRAIN, int ALLOW_START, int WANT_OPTIMIZER);
int		BRAIN_SAVE(
	BRAIN *LOADED_BRAIN, const char *WEB_PATH, const char *VOCABULARY_PATH,
	int WITH_OPTIMIZER
);
int		BRAIN_ADD_WORD(BRAIN *LOADED_BRAIN, int32_t LEXEME);
int32_t	BRAIN_SPECIAL_TOKEN(BRAIN *LOADED_BRAIN, const char *SPECIAL_NAME);

#endif
