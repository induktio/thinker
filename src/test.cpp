
#include "test.h"

#ifndef BUILD_DEBUG

void extra_setup(Config* UNUSED(cf)) { }

#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wformat"

void extra_setup(Config* UNUSED(cf)) {



}

#pragma GCC diagnostic pop
#pragma GCC diagnostic pop
#endif

