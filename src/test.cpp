
#include "test.h"

#ifndef BUILD_DEBUG

void extra_setup(Config* UNUSED(cf)) { }

#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Wunused-parameter"

void extra_setup(Config* cf) {



}

#pragma GCC diagnostic pop
#endif

