#pragma once

#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <utility>

namespace util
{

inline std::optional<std::string> read_file(const std::filesystem::path &path)
{
	std::ifstream file(path);
	if (!file)
		return std::nullopt;
	std::ostringstream content;
	content << file.rdbuf();
	return std::move(content).str();
}

}
