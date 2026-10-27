#include "kernel/cmdline.hpp"
#include "util/file.hpp"

#include <algorithm>
#include <optional>
#include <ranges>
#include <string_view>

namespace kernel
{

const Cmdline &Cmdline::get()
{
	static const Cmdline cmdline(util::read_file("/proc/cmdline").value_or(""));
	return cmdline;
}

Cmdline::Cmdline(std::string_view raw)
{
	for (auto word : raw | std::views::split(' '))
	{
		std::string_view arg(word);
		if (!arg.empty() && arg.back() == '\n')
			arg.remove_suffix(1);
		if (!arg.empty())
			args.emplace_back(arg);
	}
}

std::optional<std::string_view> Cmdline::value(std::string_view key) const
{
	// the last occurrence wins, like in the kernel :D
	for (const auto &arg : args | std::views::reverse)
		if (arg.size() > key.size() && arg.starts_with(key) && arg[key.size()] == '=')
			return std::string_view(arg).substr(key.size() + 1);
	return std::nullopt;
}

bool Cmdline::has(std::string_view flag) const
{
	return std::ranges::contains(args, flag);
}

}
