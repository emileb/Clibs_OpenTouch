#pragma once

#include "zstring.h"

// Converts a vertex+fragment pair composed for the mobile GL renderer (desktop GLSL with a GLES header)
// to GLSL ES 3.10 via glslang + SPIRV-Cross. Both stages go through one glslang program so
// varying/uniform locations agree. Sources are replaced in place; returns false with error text on failure.
bool GL_ConvertProgramToGLES(const char *name, FString &vertSrc, FString &fragSrc, FString &error);
