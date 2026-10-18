/// @file file.h
/// @brief Reading a whole file into memory (used for SPIR-V shader code).
#pragma once
#include <vector>

/// Read every byte of a file.
/// @param filename  path of the file to read
/// @return the file's bytes
/// @throws std::runtime_error if the file can't be opened or read
std::vector<char> read_file(const char* filename);
