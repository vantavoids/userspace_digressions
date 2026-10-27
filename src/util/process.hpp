#pragma once

#include <expected>
#include <string>
#include <system_error>
#include <vector>

namespace util::process
{

// returns the exit code, or 128 + signal number if the child was killed.
std::expected<int, std::error_code> run(const std::vector<std::string> &argv);

}
