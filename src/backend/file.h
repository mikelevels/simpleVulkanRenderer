// file.h: reading a whole file into memory (used for SPIR-V shader code).
#pragma once
#include <vector>

// Return the bytes of `filename`. Throws std::runtime_error if the file
// can't be opened or read.
std::vector<char> read_file(const char* filename);
