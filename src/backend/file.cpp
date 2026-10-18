// file.cpp: reading a binary file.
#include "file.h"
#include <fstream>
#include <stdexcept>
#include <string>

std::vector<char> read_file(const char* filename) {
	// ios::ate opens the file with the read position AT THE END, so tellg()
	// right away gives the file's size. ios::binary: no newline translation
	// (Windows would otherwise turn "\r\n" into "\n" and corrupt SPIR-V).
	std::ifstream file(filename, std::ios::ate | std::ios::binary);
	if (!file.is_open()) {
		// The author only logs this and carries on: tellg() then returns -1,
		// which becomes a huge size_t, and the vector allocation fails far
		// from the cause (bugsExplained.md, entry 20).
		throw std::runtime_error(std::string("Failed to open ") + filename);
	}
	std::streamsize size = file.tellg();
	std::vector<char> buffer(static_cast<std::size_t>(size));
	file.seekg(0);
	if (!file.read(buffer.data(), size)) {
		throw std::runtime_error(std::string("Failed to read ") + filename);
	}
	return buffer;   // the ifstream closes itself (RAII)
}
