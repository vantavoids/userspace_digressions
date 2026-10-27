#include "util/log.hpp"

#include <cstdlib>
#include <print>
#include <string_view>
#include <unistd.h>

namespace
{

using util::log::Level;

constexpr std::string_view reset = "\033[0m";

constexpr std::string_view color(Level level)
{
	switch (level)
	{
	case Level::info:
		return "\033[36m";
	case Level::success:
		return "\033[32m";
	case Level::warn:
		return "\033[33m";
	case Level::error:
		return "\033[31m";
	}
	return reset;
}

bool use_colors()
{
	static const bool enabled = isatty(STDERR_FILENO) && std::getenv("NO_COLOR") == nullptr;
	return enabled;
}

}

void util::log::write(Level level, std::string_view message)
{
	const auto on = use_colors() ? color(level) : "";
	const auto off = use_colors() ? reset : "";
	std::println(stderr, "{0}[{1}systemfeur{0}]{1} {0}{2:<5}{1} {3}", on, off, level, message);
}
