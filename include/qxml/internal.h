#ifndef __QXML_INTERNAL_H__
#define __QXML_INTERNAL_H__

#include <string.h>
static int _qxml_string_startswith(const char* input, const char* what)
{
    return !strncmp(input, what, strlen(what));
}

#endif /* __QXML_INTERNAL_H__ */
