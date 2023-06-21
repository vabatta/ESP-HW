#pragma once

#define UUID_TO_STRING(uuid)                                                              \
	({                                                                                      \
		char uuid_str[37];                                                                    \
		sprintf(uuid_str,                                                                     \
						"%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X",       \
						uuid[15], uuid[14], uuid[13], uuid[12], uuid[11], uuid[10], uuid[9], uuid[8], \
						uuid[7], uuid[6], uuid[5], uuid[4], uuid[3], uuid[2], uuid[1], uuid[0]);      \
		uuid_str;                                                                             \
	})

#define ADDR_TO_STRING(addr)                                       \
	({                                                               \
		char addr_str[18];                                             \
		sprintf(addr_str,                                              \
						"%02X:%02X:%02X:%02X:%02X:%02X",                       \
						addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]); \
		addr_str;                                                      \
	})
