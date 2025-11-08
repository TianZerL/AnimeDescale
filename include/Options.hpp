#ifndef OPTIONS_HPP
#define OPTIONS_HPP

#include <string>
#include <vector>

#include "AC/Specs.hpp"

struct Options
{
    std::vector<std::string> inputs{};
    std::string outputDir{};
    bool detectMode = false;
    bool crop = false;
    bool gray = false;
};

Options parse(int argc, const char* const* argv) noexcept;

#endif
