#ifndef FILE_SYSTEM_H
#define FILE_SYSTEM_H

typedef struct
{
	char	NAME[128];
	int		IS_DIRECTORY;
} FILE_ENTRY;

int	FILE_SYSTEM_LIST(const char *PATH, FILE_ENTRY *OUTPUT, int OUTPUT_SIZE);
int	FILE_SYSTEM_IS_DIRECTORY(const char *PATH);
int	FILE_SYSTEM_EXISTS(const char *PATH);
int	FORMAT_LIKE_LINES(
	const char *SOURCE_TEXT, const char *VALUE_TEXT, char *OUTPUT,
	int OUTPUT_SIZE
);
int	FILE_SYSTEM_RESOLVE(
	const char *ROOT_PATH, const char *RELATIVE_PATH, char *OUTPUT,
	int OUTPUT_SIZE, int ALLOW_NEW
);

#endif
