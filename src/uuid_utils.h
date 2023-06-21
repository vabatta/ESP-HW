#ifndef _UUID_UTILS_H_
#define _UUID_UTILS_H_

#include <inttypes.h>

/**
 * Convert a UUID string to a byte array.
 *
 * @param uuid_string The UUID string to convert.
 * @param bytes The byte array to store the converted UUID.
 * @param reverse_order Whether to reverse the order of the bytes.
 *
 * @return 0 on success, others if not a valid uuid-128.
 */
extern int convert_uuid_to_bytes(const char *uuid_string, uint8_t *bytes, uint8_t reverse_order);

#endif // _UUID_UTILS_H_