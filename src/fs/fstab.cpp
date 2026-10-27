#include "fs/fstab.hpp"
#include "fs/blockdev.hpp"
#include "fs/fsck.hpp"
#include "fs/mount.hpp"
#include "util/file.hpp"
#include "util/log.hpp"

#include <algorithm>
#include <cerrno>
#include <charconv>
#include <cstddef>
#include <filesystem>
#include <format>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <sys/mount.h>
#include <system_error>
#include <utility>
#include <vector>

namespace fs
{

namespace
{

std::vector<std::string_view> split_fields(std::string_view line)
{
	constexpr std::string_view blanks = " \t";
	std::vector<std::string_view> fields;
	std::size_t pos = 0;
	while ((pos = line.find_first_not_of(blanks, pos)) != std::string_view::npos)
	{
		const auto end = line.find_first_of(blanks, pos);
		fields.push_back(line.substr(pos, end - pos));
		pos = end;
	}
	return fields;
}

bool is_octal(char c)
{
	return c >= '0' && c <= '7';
}

// fstab(5) encodes blanks and backslashes as \040, \011, \134...
std::string unescape(std::string_view field)
{
	std::string result;
	for (std::size_t i = 0; i < field.size(); ++i)
	{
		if (field[i] == '\\' && i + 3 < field.size()
			&& is_octal(field[i + 1]) && is_octal(field[i + 2]) && is_octal(field[i + 3]))
		{
			result += static_cast<char>((field[i + 1] - '0') * 64 + (field[i + 2] - '0') * 8 + (field[i + 3] - '0'));
			i += 3;
		}
		else
			result += field[i];
	}
	return result;
}

int parse_number(std::string_view field)
{
	int value = 0;
	std::from_chars(field.data(), field.data() + field.size(), value);
	return value;
}

bool mount_entry(const FstabEntry &entry)
{
	if (entry.type == "swap" || entry.target == "/" || entry.target == "none" || entry.has_option("noauto"))
		return true;

	const auto fail = [&entry](std::string_view reason) {
		if (entry.has_option("nofail"))
		{
			util::log::warn("{}: {} (nofail)", entry.target, reason);
			return true;
		}
		util::log::error("{}: {}", entry.target, reason);
		return false;
	};

	if (is_mount_point(entry.target.c_str()))
	{
		util::log::info("{} already mounted", entry.target);
		return true;
	}

	const auto device = blockdev::resolve(entry.spec);
	if (!device)
		return fail(std::format("cannot find {}", entry.spec));

	std::error_code ec;
	const bool is_block = std::filesystem::is_block_file(*device, ec);
	if (is_block && entry.pass > 0 && fsck(*device) == FsckResult::failed)
		return fail("fsck failed, not mounting");

	auto type = entry.type;
	if (type == "auto")
	{
		const auto info = is_block ? blockdev::probe(*device) : std::nullopt;
		if (!info)
			return fail(std::format("cannot detect the filesystem on {}", *device));
		type = info->type;
	}

	std::filesystem::create_directories(entry.target, ec);
	if (ec)
		return fail(std::format("mkdir: {}", ec.message()));

	const auto options = MountOptions::parse(entry.options);
	const char *data = options.data.empty() ? nullptr : options.data.c_str();
	if (::mount(device->c_str(), entry.target.c_str(), type.c_str(), options.flags, data) != 0)
		return fail(std::format("mount {} ({}): {}", *device, type, std::error_code(errno, std::system_category()).message()));

	util::log::success("mounted {} on {}", *device, entry.target);
	return true;
}

}

bool FstabEntry::has_option(std::string_view option) const
{
	return std::ranges::contains(options, option);
}

std::vector<FstabEntry> parse_fstab(std::string_view content)
{
	std::vector<FstabEntry> entries;
	for (const auto [number, range] : content | std::views::split('\n') | std::views::enumerate)
	{
		const auto fields = split_fields(std::string_view(range));
		if (fields.empty() || fields[0].starts_with('#'))
			continue;
		if (fields.size() < 4)
		{
			util::log::warn("fstab:{}: expected at least 4 fields, ignoring", number + 1);
			continue;
		}

		FstabEntry entry{
			.spec = unescape(fields[0]),
			.target = unescape(fields[1]),
			.type = std::string(fields[2]),
			.options = fields[3] | std::views::split(',') | std::ranges::to<std::vector<std::string>>(),
		};
		if (fields.size() > 4)
			entry.dump = parse_number(fields[4]);
		if (fields.size() > 5)
			entry.pass = parse_number(fields[5]);
		entries.push_back(std::move(entry));
	}
	return entries;
}

std::optional<std::vector<FstabEntry>> read_fstab()
{
	const auto content = util::read_file("/etc/fstab");
	if (!content)
		return std::nullopt;
	return parse_fstab(*content);
}

bool mount_fstab()
{
	const auto entries = read_fstab();
	if (!entries)
	{
		util::log::warn("cannot read /etc/fstab, no extra filesystems mounted");
		return true;
	}

	bool ok = true;
	for (const auto &entry : *entries)
		ok = mount_entry(entry) && ok;
	return ok;
}

}
