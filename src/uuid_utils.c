// Library
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
// Local
#include "uuid_utils.h"

int convert_uuid_to_bytes(const char *uuid_string, uint8_t *bytes, uint8_t reverse_order)
{
	size_t uuid_string_length = strlen(uuid_string);
	if (uuid_string_length != 36)
	{
		// Invalid UUID string length
		return 1;
	}

	int start = 0;
	int end = 16;
	int step = 1;

	if (reverse_order)
	{
		start = 15;
		end = -1;
		step = -1;
	}

	for (int i = start, j = 0; i != end; i += step, j += 2)
	{
		while (uuid_string[j] == '-' || uuid_string[j] == '\0')
		{
			j++; // Skip dashes and null characters
		}

		sscanf(&uuid_string[j], "%2hhx", &bytes[i]);
	}

	return 0;
}