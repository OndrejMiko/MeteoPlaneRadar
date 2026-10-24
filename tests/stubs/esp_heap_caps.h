#pragma once
#include <cstdlib>
#include <cassert>
#define MALLOC_CAP_SPIRAM 1
#define MALLOC_CAP_8BIT 2
inline bool testFailPsram=false;
inline void* heap_caps_malloc(size_t n,int flags){assert(flags&MALLOC_CAP_SPIRAM);return testFailPsram?nullptr:malloc(n);}
inline void* heap_caps_calloc(size_t n,size_t s,int flags){assert(flags&MALLOC_CAP_SPIRAM);return testFailPsram?nullptr:calloc(n,s);}
