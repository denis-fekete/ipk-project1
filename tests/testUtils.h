#ifndef TEST_UTILS_H
#define TEST_UTILS_H

#include "stdlib.h"
#include "stdio.h"

#define TEST_NAME(name)         \
    const char* testname = name;

#define ASSERT(cond)            \
    if(!(cond)) {

#define ON_FAIL(message)        \
    fprintf(stderr, "%s:", testname);      \
    fprintf(stderr, message);   \
    fprintf(stderr, "\n");      \
    return 1;                   \
    }


#endif /*TEST_UTILS_H*/