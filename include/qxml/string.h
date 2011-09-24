#ifndef __QXML_STRING_H__
#define __QXML_STRING_H__

#include <stddef.h>

size_t qxml_string_escapelent(char* out, size_t olen, const char* str, size_t len);
size_t qxml_string_escapet(char* out, size_t olen, const char* str);
size_t qxml_string_escapelen(char* out, size_t olen, const char* str, size_t len);
size_t qxml_string_escape(char* out, size_t olen, const char* str);

/* todo: unescape &#XXXX; */
size_t qxml_string_unescapelen(char* out, size_t olen, const char* str, size_t len);
size_t qxml_string_unescape(char* out, size_t olen, const char* str);

#endif /* __QXML_STRING_H__ */
