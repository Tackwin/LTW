#pragma once

#ifdef ES
#include <GLES3/gl32.h>
#define glObjectLabel(...)
#else
#include <GL/gl3w.h>
#endif
