#include "SENSE.h"
#include <stdio.h>

int
	TEST_SENSE(void)
{
	static const char	*SCRIPT_FILES[12] = {
		"tests/sense_v1.txt", "tests/sense_v2.txt", "tests/sense_v3.txt",
		"tests/sense_v4.txt", "tests/sense_v5.txt", "tests/sense_v6.txt",
		"tests/sense_v7.txt", "tests/sense_v8.txt", "tests/sense_v9.txt",
		"tests/sense_v10.txt", "tests/sense_v11.txt", "tests/sense_v12.txt"
	};
	int					FAIL_COUNT = 0;

	printf("sense: reading messages, keeping track of the situation, deciding "
		"actions and thoughts\n");

	int	INDEX;

	for (
		INDEX = 0;
		INDEX < (int)(sizeof SCRIPT_FILES / sizeof SCRIPT_FILES[0]);
		INDEX++
	)
	{
		printf("  %s: ", SCRIPT_FILES[INDEX]);
		fflush(stdout);
		FAIL_COUNT += SENSE_RUN_SCRIPT_FILE(SCRIPT_FILES[INDEX], 0);
	}

	printf("sense: %s\n", FAIL_COUNT ? "FAILED" : "PASSED");

	return (FAIL_COUNT);
}
