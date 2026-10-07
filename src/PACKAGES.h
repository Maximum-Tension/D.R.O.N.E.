#ifndef PACKAGES_H
#define PACKAGES_H

/* Knowledge packages: the .mem files in Lucy's memory folder.  Each line
 * tells what one thing is and what is known about it:
 *
 *     cat: A cat is a small furry animal. | kind: animal, pet | legs: 4
 *
 * A package dropped into the folder is read before the next message, one
 * that is changed is read again and one that is taken away is forgotten, so
 * Lucy never needs a restart to know more.  catalog.txt in the same folder
 * names the packages that exist and the words each one covers, so Lucy can
 * say which package to add when she is asked about something she doesn't
 * know yet. */

#define MAX_PACKAGES 64

typedef struct
{
	char		NAME[64];
	char		PATH[600];
	long long	STAMP;
	char		ABOUT[240];
	int			ENTRY_COUNT;
	int			IS_LOADED;
} PACKAGE;

typedef struct
{
	char	THING[48];
	char	PROPERTY[32];
	char	VALUE[240];
	short	PACKAGE_INDEX;
	short	IS_DERIVED;
	int		ENTRY;
} PACKAGE_FACT;

typedef struct
{
	char			FOLDERS[2][512];
	int				FOLDER_COUNT;
	PACKAGE			PACKAGES[MAX_PACKAGES];
	int				COUNT;
	PACKAGE_FACT	*FACTS;
	int				FACT_COUNT;
	int				FACT_CAPACITY;
	char			(*CATALOG_WORDS)[48];
	short			*CATALOG_OWNERS;
	int				CATALOG_COUNT;
	int				CATALOG_CAPACITY;
	char			CATALOG_NAMES[MAX_PACKAGES][64];
	char			CATALOG_ABOUT[MAX_PACKAGES][160];
	int				CATALOG_NAME_COUNT;
	char			CATALOG_PATH[600];
	long long		CATALOG_STAMP;
	int				NEXT_ENTRY;
} PACKAGE_SET;

int		PACKAGES_OPEN(
	PACKAGE_SET *SET, const char *FOLDER, const char *SECOND_FOLDER
);
int		PACKAGES_REFRESH(PACKAGE_SET *SET, char *NEWS, int NEWS_SIZE);
void	PACKAGES_CLOSE(PACKAGE_SET *SET);
int		PACKAGES_KNOW(
	PACKAGE_SET *SET, const char *THING, const char *PROPERTY, char *VALUE,
	int VALUE_SIZE
);
int		PACKAGES_THINGS_WITH(
	PACKAGE_SET *SET, const char *PROPERTY, const char *VALUE, char *OUTPUT,
	int OUTPUT_SIZE
);
int		PACKAGES_KNOWS_THING(PACKAGE_SET *SET, const char *THING);
int		PACKAGES_SOURCE(
	PACKAGE_SET *SET, const char *THING, char *OUTPUT, int OUTPUT_SIZE
);
int		PACKAGES_PACKAGE_FOR(
	PACKAGE_SET *SET, const char *WORD, char *OUTPUT, int OUTPUT_SIZE
);
int		PACKAGES_DEFINITIONS(
	PACKAGE_SET *SET, int PACKAGE_INDEX, char *OUTPUT, int OUTPUT_SIZE
);
void	PACKAGES_LIST(PACKAGE_SET *SET, char *OUTPUT, int OUTPUT_SIZE);
void	PACKAGES_THING_NAME(const char *WORD, char *OUTPUT, int OUTPUT_SIZE);

#endif
