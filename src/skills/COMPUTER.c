#include "../SKILL.h"
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#ifdef _WIN32
#include <direct.h>
#include <windows.h>
#else
#include <dirent.h>
#include <unistd.h>
#endif

/* The computer skill: Lucy's hands on the computer.  Making folders,
 * writing, adding to and reading files, listing, moving, copying and
 * throwing away (into the trash folder, so it can come back), and reading
 * the computer's clock.  Everything happens inside her files folder; a path
 * that tries to leave it is refused.  Two paths are given with a tab
 * between them ("notes.txt\tdocs").  Without this skill Lucy can still
 * think and talk, but she cannot touch the computer. */

#define BIG_TEXT 65536

/* a path inside the files folder: no "..", no drive, not absolute */
static int
	SAFE_PATH(
		const SKILL_HOST *HOST, const char *RELATIVE_PATH, char *OUTPUT,
		int OUTPUT_SIZE
	)
{
	const char	*CURSOR;
	const char	*ROOT =
		HOST && HOST->FILES_FOLDER ? HOST->FILES_FOLDER : "files";

	if (!RELATIVE_PATH)
		return (0);

	while (*RELATIVE_PATH == ' ')
		RELATIVE_PATH++;

	if (
		RELATIVE_PATH[0] == '/' ||
		RELATIVE_PATH[0] == '\\' ||
		(RELATIVE_PATH[0] && RELATIVE_PATH[1] == ':') ||
		strstr(RELATIVE_PATH, "..")
	)
		return (0);

	for (CURSOR = RELATIVE_PATH; *CURSOR; CURSOR++)
		if (*CURSOR == '\\' || (unsigned char)*CURSOR < 0X20)
			return (0);

	if (!RELATIVE_PATH[0])
		snprintf(OUTPUT, OUTPUT_SIZE, "%s", ROOT);
	else
		snprintf(OUTPUT, OUTPUT_SIZE, "%s/%s", ROOT, RELATIVE_PATH);

	/* no trailing slash */
	{
		size_t	LENGTH = strlen(OUTPUT);

		while (LENGTH > 1 && OUTPUT[LENGTH - 1] == '/')
			OUTPUT[--LENGTH] = 0;
	}

	return (1);
}

/* "notes.txt\tdocs" -> "notes.txt", "docs" */
static int
	TWO_PARTS(
		const char *ARGUMENT, char *FIRST, int FIRST_SIZE, char *SECOND,
		int SECOND_SIZE
	)
{
	const char	*TAB = ARGUMENT ? strchr(ARGUMENT, '\t') : NULL;

	if (!TAB)
		return (0);

	snprintf(FIRST, FIRST_SIZE, "%.*s", (int)(TAB - ARGUMENT), ARGUMENT);
	snprintf(SECOND, SECOND_SIZE, "%s", TAB + 1);

	return (FIRST[0] && SECOND[0]);
}

static int
	IS_FOLDER(const char *PATH)
{
	struct stat	INFORMATION;

	return (!stat(PATH, &INFORMATION) && S_ISDIR(INFORMATION.st_mode));
}

static int
	IS_THERE(const char *PATH)
{
	struct stat	INFORMATION;

	return (!stat(PATH, &INFORMATION));
}

static int
	MAKE_ONE_FOLDER(const char *PATH)
{
#ifdef _WIN32
	return (_mkdir(PATH));
#else
	return (mkdir(PATH, 0755));
#endif
}

/* the names in a folder, folders marked */
typedef struct
{
	char	NAME[256];
	int		IS_FOLDER;
} ENTRY;

static int
	ENTRY_ORDER(const void *LEFT, const void *RIGHT)
{
	return (strcmp(((const ENTRY *)LEFT)->NAME, ((const ENTRY *)RIGHT)->NAME));
}

static int
	LIST_FOLDER(const char *PATH, ENTRY *ENTRIES, int LIMIT)
{
	int	COUNT = 0;

#ifdef _WIN32
	char				PATTERN[700];
	WIN32_FIND_DATAA	FOUND;
	HANDLE				SEARCH;

	snprintf(PATTERN, sizeof PATTERN, "%s\\*", PATH);
	SEARCH = FindFirstFileA(PATTERN, &FOUND);

	if (SEARCH == INVALID_HANDLE_VALUE)
		return (0);

	do
	{
		if (!strcmp(FOUND.cFileName, ".") || !strcmp(FOUND.cFileName, ".."))
			continue ;

		snprintf(
			ENTRIES[COUNT].NAME, sizeof ENTRIES[COUNT].NAME, "%s",
			FOUND.cFileName
		);
		ENTRIES[COUNT].IS_FOLDER =
			(FOUND.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
		COUNT++;
	} while (COUNT < LIMIT && FindNextFileA(SEARCH, &FOUND));

	FindClose(SEARCH);
#else
	DIR				*FOLDER = opendir(PATH);
	struct dirent	*ITEM;

	if (!FOLDER)
		return (0);

	while (COUNT < LIMIT && (ITEM = readdir(FOLDER)))
	{
		char	FULL[1200];

		if (
			ITEM->d_name[0] == '.' &&
			(!ITEM->d_name[1] || (ITEM->d_name[1] == '.' && !ITEM->d_name[2]))
		)
			continue ;

		snprintf(
			ENTRIES[COUNT].NAME, sizeof ENTRIES[COUNT].NAME, "%s", ITEM->d_name
		);
		snprintf(FULL, sizeof FULL, "%s/%s", PATH, ITEM->d_name);
		ENTRIES[COUNT].IS_FOLDER = IS_FOLDER(FULL);
		COUNT++;
	}

	closedir(FOLDER);
#endif
	qsort(ENTRIES, (size_t)COUNT, sizeof *ENTRIES, ENTRY_ORDER);

	return (COUNT);
}

static int
	COPY_CONTENTS(const char *FROM, const char *TO)
{
	FILE	*SOURCE = fopen(FROM, "rb");
	FILE	*TARGET;
	char	BUFFER[1 << 15];
	size_t	READ_COUNT;

	if (!SOURCE)
		return (-1);

	TARGET = fopen(TO, "wb");

	if (!TARGET)
	{
		fclose(SOURCE);
		return (-1);
	}

	while ((READ_COUNT = fread(BUFFER, 1, sizeof BUFFER, SOURCE)) > 0)
		fwrite(BUFFER, 1, READ_COUNT, TARGET);

	fclose(SOURCE);
	fclose(TARGET);

	return (0);
}

/* ---------- the actions ---------- */

static int
	DO_MKDIR(
		const SKILL_HOST *HOST, const char *ARGUMENT, char *RESULT, int SIZE
	)
{
	char	PATH[1200];

	if (!SAFE_PATH(HOST, ARGUMENT, PATH, sizeof PATH) || !ARGUMENT[0])
		return (snprintf(RESULT, SIZE, "bad name"), 1);

	if (IS_FOLDER(PATH))
		return (snprintf(RESULT, SIZE, "folder exists"), 1);

	if (IS_THERE(PATH))
		return (snprintf(RESULT, SIZE, "a file has that name"), 1);

	MAKE_ONE_FOLDER(PATH);

	if (!IS_FOLDER(PATH))
		return (snprintf(RESULT, SIZE, "cannot make it"), 1);

	snprintf(RESULT, SIZE, "folder made");

	return (0);
}

/* a new file, with one line in it when TEXT is given */
static int
	DO_WRITE(
		const SKILL_HOST *HOST, const char *ARGUMENT, char *RESULT, int SIZE
	)
{
	char	NAME[600];
	char	TEXT[BIG_TEXT];
	char	PATH[1200];
	FILE	*STREAM;

	if (!TWO_PARTS(ARGUMENT, NAME, sizeof NAME, TEXT, sizeof TEXT))
	{
		snprintf(NAME, sizeof NAME, "%s", ARGUMENT ? ARGUMENT : "");

		if (strchr(NAME, '\t'))
			*strchr(NAME, '\t') = 0;

		TEXT[0] = 0;
	}

	if (!SAFE_PATH(HOST, NAME, PATH, sizeof PATH) || !NAME[0])
		return (snprintf(RESULT, SIZE, "bad name"), 1);

	if (IS_FOLDER(PATH))
		return (snprintf(RESULT, SIZE, "that is a folder"), 1);

	if (IS_THERE(PATH))
		return (snprintf(RESULT, SIZE, "file exists"), 1);

	STREAM = fopen(PATH, "wb");

	if (!STREAM)
		return (snprintf(RESULT, SIZE, "cannot write"), 1);

	if (TEXT[0])
		fprintf(STREAM, "%s\n", TEXT);

	fclose(STREAM);
	snprintf(RESULT, SIZE, "written");

	return (0);
}

/* one more line at the end of a file (made when it is not there) */
static int
	DO_APPEND(
		const SKILL_HOST *HOST, const char *ARGUMENT, char *RESULT, int SIZE
	)
{
	char	NAME[600];
	char	TEXT[BIG_TEXT];
	char	PATH[1200];
	FILE	*STREAM;
	int		NEEDS_NEWLINE = 0;
	int		EXISTED;

	if (!TWO_PARTS(ARGUMENT, NAME, sizeof NAME, TEXT, sizeof TEXT))
		return (snprintf(RESULT, SIZE, "nothing to add"), 1);

	if (!SAFE_PATH(HOST, NAME, PATH, sizeof PATH))
		return (snprintf(RESULT, SIZE, "bad name"), 1);

	if (IS_FOLDER(PATH))
		return (snprintf(RESULT, SIZE, "that is a folder"), 1);

	EXISTED = IS_THERE(PATH);

	/* a last line without its end gets one first */
	if (EXISTED)
		STREAM = fopen(PATH, "rb");
	else
		STREAM = NULL;

	if (STREAM)
	{
		if (!fseek(STREAM, -1, SEEK_END) && fgetc(STREAM) != '\n')
			NEEDS_NEWLINE = 1;

		fclose(STREAM);
	}

	STREAM = fopen(PATH, "ab");

	if (!STREAM)
		return (snprintf(RESULT, SIZE, "cannot write"), 1);

	if (NEEDS_NEWLINE)
		fputc('\n', STREAM);

	fprintf(STREAM, "%s\n", TEXT);
	fclose(STREAM);
	snprintf(RESULT, SIZE, EXISTED ? "added" : "written");

	return (0);
}

static int
	DO_READ(
		const SKILL_HOST *HOST, const char *ARGUMENT, char *RESULT, int SIZE
	)
{
	char	PATH[1200];
	FILE	*STREAM;
	size_t	READ_COUNT;

	if (!SAFE_PATH(HOST, ARGUMENT, PATH, sizeof PATH) || !ARGUMENT[0])
		return (snprintf(RESULT, SIZE, "bad name"), 1);

	if (IS_FOLDER(PATH) || !IS_THERE(PATH))
		return (snprintf(RESULT, SIZE, "no such file"), 1);

	STREAM = fopen(PATH, "rb");

	if (!STREAM)
		return (snprintf(RESULT, SIZE, "cannot read"), 1);

	READ_COUNT = fread(RESULT, 1, (size_t)SIZE - 1, STREAM);
	RESULT[READ_COUNT] = 0;
	fclose(STREAM);

	return (0);
}

/* "a.txt, docs/b.txt, empty (empty folder)": a folder and one level below */
static int
	DO_LIST(
		const SKILL_HOST *HOST, const char *ARGUMENT, char *RESULT, int SIZE
	)
{
	char	PATH[1200];
	ENTRY	*ENTRIES = malloc(256 * sizeof *ENTRIES);
	ENTRY	*INSIDE = malloc(64 * sizeof *INSIDE);
	int		COUNT;
	int		INDEX;
	int		LENGTH = 0;

	RESULT[0] = 0;

	if (!ENTRIES || !INSIDE)
	{
		free(ENTRIES);
		free(INSIDE);
		return (snprintf(RESULT, SIZE, "cannot list"), 1);
	}

	if (
		!SAFE_PATH(HOST, ARGUMENT ? ARGUMENT : "", PATH, sizeof PATH) ||
		!IS_FOLDER(PATH)
	)
	{
		free(ENTRIES);
		free(INSIDE);
		return (snprintf(RESULT, SIZE, "no such folder"), 1);
	}

	COUNT = LIST_FOLDER(PATH, ENTRIES, 256);

	for (
		INDEX = 0;
		INDEX < COUNT && LENGTH < SIZE - 300 && LENGTH < 900;
		INDEX++
	)
	{
		char	SUB_PATH[1500];
		int		SUB_COUNT;
		int		SUB;

		/* the trash is no part of the files: it is listed when asked for */
		if (
			(!ARGUMENT || !ARGUMENT[0] || !strcmp(ARGUMENT, ".")) &&
			ENTRIES[INDEX].IS_FOLDER &&
			!strcmp(ENTRIES[INDEX].NAME, "trash")
		)
			continue ;

		if (!ENTRIES[INDEX].IS_FOLDER)
		{
			LENGTH += snprintf(
				RESULT + LENGTH, SIZE - LENGTH, "%s%s", LENGTH ? ", " : "",
				ENTRIES[INDEX].NAME
			);
			continue ;
		}

		snprintf(SUB_PATH, sizeof SUB_PATH, "%s/%s", PATH, ENTRIES[INDEX].NAME);
		SUB_COUNT = LIST_FOLDER(SUB_PATH, INSIDE, 64);

		if (!SUB_COUNT)
			LENGTH += snprintf(
				RESULT + LENGTH, SIZE - LENGTH, "%s%s (empty folder)",
				LENGTH ? ", " : "", ENTRIES[INDEX].NAME
			);

		for (
			SUB = 0;
			SUB < SUB_COUNT && LENGTH < SIZE - 300 && LENGTH < 900;
			SUB++
		)
			LENGTH += snprintf(
				RESULT + LENGTH, SIZE - LENGTH, "%s%s/%s%s", LENGTH ? ", " : "",
				ENTRIES[INDEX].NAME, INSIDE[SUB].NAME,
				INSIDE[SUB].IS_FOLDER ? " (folder)" : ""
			);
	}

	if (!LENGTH)
		snprintf(RESULT, SIZE, "no files");

	free(ENTRIES);
	free(INSIDE);

	return (0);
}

/* moves (or renames) FROM to TO; TO is the new path itself */
static int
	DO_MOVE(
		const SKILL_HOST *HOST, const char *ARGUMENT, char *RESULT, int SIZE
	)
{
	char	FROM_NAME[600];
	char	TO_NAME[600];
	char	FROM[1200];
	char	TO[1200];

	if (
		!TWO_PARTS(
			ARGUMENT, FROM_NAME, sizeof FROM_NAME, TO_NAME, sizeof TO_NAME
		) ||
		!SAFE_PATH(HOST, FROM_NAME, FROM, sizeof FROM) ||
		!SAFE_PATH(HOST, TO_NAME, TO, sizeof TO)
	)
		return (snprintf(RESULT, SIZE, "bad name"), 1);

	if (!IS_THERE(FROM))
		return (snprintf(RESULT, SIZE, "no such file"), 1);

	if (IS_THERE(TO))
		return (snprintf(RESULT, SIZE, "that name is taken"), 1);

	if (rename(FROM, TO))
		return (snprintf(RESULT, SIZE, "cannot move"), 1);

	snprintf(RESULT, SIZE, "moved");

	return (0);
}

/* a copy of a file; into a folder when TO is one */
static int
	DO_COPY(
		const SKILL_HOST *HOST, const char *ARGUMENT, char *RESULT, int SIZE
	)
{
	char	FROM_NAME[600];
	char	TO_NAME[600];
	char	FROM[1200];
	char	TO[1800];

	if (
		!TWO_PARTS(
			ARGUMENT, FROM_NAME, sizeof FROM_NAME, TO_NAME, sizeof TO_NAME
		) ||
		!SAFE_PATH(HOST, FROM_NAME, FROM, sizeof FROM) ||
		!SAFE_PATH(HOST, TO_NAME, TO, 1200)
	)
		return (snprintf(RESULT, SIZE, "bad name"), 1);

	if (!IS_THERE(FROM) || IS_FOLDER(FROM))
		return (snprintf(RESULT, SIZE, "no such file"), 1);

	if (IS_FOLDER(TO))
	{
		const char	*BASE = strrchr(FROM, '/') ? strrchr(FROM, '/') + 1 : FROM;
		size_t		LENGTH = strlen(TO);

		snprintf(TO + LENGTH, sizeof TO - LENGTH, "/%s", BASE);
	}

	if (IS_THERE(TO))
		return (snprintf(RESULT, SIZE, "that name is taken"), 1);

	if (COPY_CONTENTS(FROM, TO))
		return (snprintf(RESULT, SIZE, "cannot copy"), 1);

	snprintf(RESULT, SIZE, "copied");

	return (0);
}

/* into the trash folder, where it can be brought back from */
static int
	DO_TRASH(
		const SKILL_HOST *HOST, const char *ARGUMENT, char *RESULT, int SIZE
	)
{
	char		FROM[1200];
	char		TRASH[1200];
	char		TO[1800];
	const char	*BASE;

	if (!SAFE_PATH(HOST, ARGUMENT, FROM, sizeof FROM) || !ARGUMENT[0])
		return (snprintf(RESULT, SIZE, "bad name"), 1);

	if (!IS_THERE(FROM))
		return (snprintf(RESULT, SIZE, "no such file"), 1);

	SAFE_PATH(HOST, "trash", TRASH, sizeof TRASH);

	if (!strncmp(FROM, TRASH, strlen(TRASH)))
		return (snprintf(RESULT, SIZE, "already in trash"), 1);

	MAKE_ONE_FOLDER(TRASH);
	BASE = strrchr(FROM, '/') ? strrchr(FROM, '/') + 1 : FROM;
	snprintf(TO, sizeof TO, "%s/%s", TRASH, BASE);

	if (IS_THERE(TO))
		return (snprintf(RESULT, SIZE, "trash has a file with that name"), 1);

	if (rename(FROM, TO))
		return (snprintf(RESULT, SIZE, "cannot delete"), 1);

	snprintf(RESULT, SIZE, "moved to trash");

	return (0);
}

/* a file, or a folder with everything in it, gone for good */
static int
	ERASE_PATH(const char *PATH, int DEPTH)
{
	if (DEPTH > 16)
		return (-1);

	if (IS_FOLDER(PATH))
	{
		ENTRY	*ENTRIES = malloc(256 * sizeof *ENTRIES);
		int		COUNT;
		int		INDEX;

		if (!ENTRIES)
			return (-1);

		/* a big folder is emptied a slice at a time */
		while ((COUNT = LIST_FOLDER(PATH, ENTRIES, 256)) > 0)
		{
			for (INDEX = 0; INDEX < COUNT; INDEX++)
			{
				char	INSIDE[1500];

				snprintf(
					INSIDE, sizeof INSIDE, "%s/%s", PATH, ENTRIES[INDEX].NAME
				);

				if (ERASE_PATH(INSIDE, DEPTH + 1))
				{
					free(ENTRIES);
					return (-1);
				}
			}
		}

		free(ENTRIES);
#ifdef _WIN32
		return (_rmdir(PATH));
#else
		return (rmdir(PATH));
#endif
	}

	return (remove(PATH));
}

/* gone for good, and only what is in the trash already: nothing else can be
 * erased without first being thrown away */
static int
	DO_ERASE(
		const SKILL_HOST *HOST, const char *ARGUMENT, char *RESULT, int SIZE
	)
{
	char	PATH[1200];
	char	TRASH[1200];
	size_t	TRASH_LENGTH;

	if (!SAFE_PATH(HOST, ARGUMENT, PATH, sizeof PATH) || !ARGUMENT[0])
		return (snprintf(RESULT, SIZE, "bad name"), 1);

	SAFE_PATH(HOST, "trash", TRASH, sizeof TRASH);
	TRASH_LENGTH = strlen(TRASH);

	if (strncmp(PATH, TRASH, TRASH_LENGTH) || PATH[TRASH_LENGTH] != '/')
		return (snprintf(RESULT, SIZE, "only things in the trash"), 1);

	if (!IS_THERE(PATH))
		return (snprintf(RESULT, SIZE, "no such file"), 1);

	if (ERASE_PATH(PATH, 0) || IS_THERE(PATH))
		return (snprintf(RESULT, SIZE, "cannot erase"), 1);

	snprintf(RESULT, SIZE, "erased");

	return (0);
}

static int
	DO_EXISTS(
		const SKILL_HOST *HOST, const char *ARGUMENT, char *RESULT, int SIZE
	)
{
	char	PATH[1200];

	if (!SAFE_PATH(HOST, ARGUMENT, PATH, sizeof PATH))
		return (snprintf(RESULT, SIZE, "bad name"), 1);

	snprintf(
		RESULT, SIZE, "%s",
		IS_FOLDER(PATH) ? "folder"
			: IS_THERE(PATH) ? "file"
		: "no"
	);

	return (0);
}

/* "3:45 pm" from the computer's clock */
static int
	DO_TIME(
		const SKILL_HOST *HOST, const char *ARGUMENT, char *RESULT, int SIZE
	)
{
	time_t		NOW = time(NULL);
	struct tm	*LOCAL = localtime(&NOW);
	char		TEXT[64];
	char		*CURSOR;

	(void)HOST;
	(void)ARGUMENT;
	strftime(TEXT, sizeof TEXT, "%I:%M %p", LOCAL);
	snprintf(RESULT, SIZE, "%s", TEXT[0] == '0' ? TEXT + 1 : TEXT);

	for (CURSOR = RESULT; *CURSOR; CURSOR++)
		*CURSOR = (char)tolower((uint8_t)*CURSOR);

	return (0);
}

/* "tuesday, october 6, 2026" */
static int
	DO_DATE(
		const SKILL_HOST *HOST, const char *ARGUMENT, char *RESULT, int SIZE
	)
{
	time_t		NOW = time(NULL);
	struct tm	*LOCAL = localtime(&NOW);
	char		TEXT[64];
	char		*CURSOR;

	(void)HOST;
	(void)ARGUMENT;
	strftime(TEXT, sizeof TEXT, "%A, %B ", LOCAL);
	snprintf(
		RESULT, SIZE, "%s%d, %d", TEXT, LOCAL->tm_mday, LOCAL->tm_year + 1900
	);

	for (CURSOR = RESULT; *CURSOR; CURSOR++)
		*CURSOR = (char)tolower((uint8_t)*CURSOR);

	return (0);
}

static int
	DO_SYSTEM(
		const SKILL_HOST *HOST, const char *ARGUMENT, char *RESULT, int SIZE
	)
{
	(void)HOST;
	(void)ARGUMENT;
#ifdef _WIN32
	snprintf(RESULT, SIZE, "Windows");
#elif defined(__APPLE__)
	snprintf(RESULT, SIZE, "macOS");
#else
	snprintf(RESULT, SIZE, "Linux");
#endif
	return (0);
}

/* ---------- what Lucy knows about these actions ---------- */

static const SKILL_ACTION	COMPUTER_ACTIONS[13] = {
	{
		"mkdir", "mkdir FOLDER", "makes a folder in the files folder",
		"make a folder called {1}\nmake a folder named {1}\n"
		"create a folder called {1}\ncreate a folder named {1}\n"
		"make a new folder {1}\nnew folder {1}\nmkdir {1}",
		"folder made: Done, I made the folder {1}.\n"
		"folder exists: The folder {1} is already there.\n"
		"*: I couldn't make the folder {1}: {R}.",
		DO_MKDIR
	},
	{
		"write", "write FILE<tab>TEXT", "makes a new file, with a line of text",
		"write {1}\ncreate a file called {1}\nmake a file named {1}",
		"written: Done, I wrote {1}.\n"
		"file exists: {1} is already there, so I didn't touch it.\n"
		"*: I couldn't write {1}: {R}.",
		DO_WRITE
	},
	{
		"append", "append FILE<tab>TEXT", "adds a line at the end of a file",
		"add {1}\nappend {1}",
		"added: Done, I added it.\n"
		"written: Done, I made the file and wrote it.\n"
		"*: I couldn't add it: {R}.",
		DO_APPEND
	},
	{
		"read", "read FILE", "reads a file",
		"read {1}\nopen {1}\nwhat is in {1}\nshow me {1}",
		"no such file: There is no file {1} in my files folder.\n" "*: {R}",
		DO_READ
	},
	{
		"list", "list [FOLDER]", "lists the files in a folder",
		"list the files\nwhat files do you have\nwhich files are there\n"
		"show me the files\nlist the files in {1}\nwhat is in the folder {1}",
		"no files: The folder is empty.\n*: Here they are: {R}.", DO_LIST
	},
	{
		"move", "move FROM<tab>TO", "moves or renames a file or folder", "",
		"moved: Done.\nthat name is taken: Something already has that name.\n"
		"*: I couldn't move it: {R}.",
		DO_MOVE
	},
	{
		"copy", "copy FILE<tab>TO",
		"copies a file, into a folder or to a new " "name",
		"copy {1} to {2}\ncopy {1} into {2}\nmake a copy of {1} called {2}\n"
		"duplicate {1} as {2}",
		"copied: Done, I copied {1} to {2}.\n"
		"that name is taken: {2} is already there, so I didn't copy.\n"
		"no such file: There is no file {1} in my files folder.\n"
		"*: I couldn't copy {1}: {R}.",
		DO_COPY
	},
	{
		"trash", "trash FILE", "puts a file in the trash folder", "",
		"moved to trash: Done, {1} is in the trash now.\n"
		"*: I couldn't throw {1} away: {R}.",
		DO_TRASH
	},
	{
		"erase", "erase PATH",
		"deletes something in the trash for good (it can't come back)", "",
		"erased: Done, {1} is gone for good.\n"
		"only things in the trash: I only delete things for good once they "
		"are in the trash.\n"
		"*: I couldn't delete {1} for good: {R}.",
		DO_ERASE
	},
	{
		"exists", "exists PATH", "says whether a file or folder is there",
		"is there a file called {1}\ndo you have a file called {1}\n"
		"does {1} exist\nis there a folder called {1}",
		"file: Yes, {1} is there.\nfolder: Yes, the folder {1} is there.\n"
		"no: No, there is no {1} in my files folder.",
		DO_EXISTS
	},
	{
		"time", "time", "reads the time from the computer's clock",
		"what time is it\nwhat's the time\ntell me the time", "*: It's {R}.",
		DO_TIME
	},
	{
		"date", "date", "reads the date from the computer's clock",
		"what is the date\nwhat's the date today\nwhat day is it today",
		"*: Today is {R}.", DO_DATE
	},
	{
		"system", "system", "says which operating system the computer runs",
		"what operating system is this\nwhich operating system are you on\n"
		"what os is this computer running\nwhat system are you running on",
		"*: This computer runs {R}.", DO_SYSTEM
	},
};

/* what each action makes true and how Lucy checks it, how doing it is
 * said, and what she tries when it fails, safest first (SKILL.h) */
static const char	COMPUTER_KNOW_HOW[] =
	"mkdir: means make the folder {1}\n"
	"mkdir: makes exists {1} = folder\n"
	"mkdir: if folder exists: done = the folder {1} is already there\n"
	"mkdir: if user disagrees: look list {folder 1} = {place {folder 1}}\n"
	"mkdir: if a file has that name: there is a file called {name 1} there "
	"already; ask mkdir {free {1}} = make the folder as \"{free}\" instead\n"
	"write: means write the file {1}\n"
	"write: makes exists {1} = file\n"
	"write: if user disagrees: look list {folder 1} = {place {folder 1}}\n"
	"append: means add a line to {1}\n"
	"append: makes exists {1} = file\n"
	"read: means read {1}\n"
	"read: kind looks\n"
	"read: if no such file: there is no file {name 1} in your files; find\n"
	"list: kind looks\n"
	"exists: kind looks\n"
	"trash: means put {1} in the trash\n"
	"trash: makes exists {1} = no\n"
	"trash: undo move trash/{name 1}<tab>{1}\n"
	"trash: if no such file: there is no {name 1} in your files; find\n"
	"trash: if already in trash: done = {name 1} is already in the trash\n"
	"trash: if trash has a file with that name: the trash already has "
	"something called {name 1}; try move {1}<tab>{free trash/{name 1}} = "
	"put it in the trash as \"{free}\"; ask erase trash/{name 1} = delete "
	"the old {name 1} in the trash for good, to make room for this one\n"
	"trash: if cannot delete: something is keeping {name 1} busy; retry\n"
	"trash: if user disagrees: find; look list {folder 1} = "
	"{place {folder 1}}; look list trash = the trash; ask erase "
	"trash/{free or {name 1}} = delete \"{free or {name 1}}\" from the "
	"trash for good as well\n"
	"move: means move {1} to {2}\n"
	"move: makes exists {1} = no\n"
	"move: if user disagrees: look list {folder 1} = {place {folder 1}}; "
	"look list {folder 2} = {place {folder 2}}; find\n"
	"move: if no such file: there is no {name 1} in your files; find\n"
	"move: if that name is taken: there is already something called "
	"{name 2} there; ask move {1}<tab>{free {2}} = move it there as "
	"\"{free}\" instead\n"
	"copy: means copy {1} to {2}\n"
	"copy: if no such file: there is no file {name 1} in your files; find\n"
	"erase: means delete {1} for good\n"
	"erase: kind risky\n"
	"erase: makes exists {1} = no\n";

static const SKILL_MEMORY	COMPUTER_MEMORY = {
	SKILL_VERSION, "computer",
	"folders, files, the trash and the clock, inside the files folder",
	(int)(sizeof COMPUTER_ACTIONS / sizeof COMPUTER_ACTIONS[0]),
	COMPUTER_ACTIONS, NULL, COMPUTER_KNOW_HOW
};

SKILL_EXPORT const SKILL_MEMORY
	*__memory__(void)
{
	return (&COMPUTER_MEMORY);
}
