#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>

#define EV_CHECK(x)\
{assert(x);}

#define EV_CHECK_VKRESULT(x)\
{assert(x == VK_SUCCESS);}

#define EV_ALLOC(t, s) (t*)malloc(sizeof(t) * s)
#define EV_FREE(t) free(t)
