#include "utilities.h"
#include <sstream>

std::string read_ifstream(const std::ifstream * file) {
    std::stringstream stream;
    stream << file->rdbuf();
    return stream.str();
}

