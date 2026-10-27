#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace fs
{

struct FstabEntry
{
	std::string spec;
	std::string target;
	std::string type;
	std::vector<std::string> options;
	int dump = 0;
	int pass = 0;

	[[nodiscard]] bool has_option(std::string_view option) const;
};

std::vector<FstabEntry> parse_fstab(std::string_view content);
std::optional<std::vector<FstabEntry>> read_fstab();

bool mount_fstab();

}
