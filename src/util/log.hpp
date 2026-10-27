#pragma once

#include <cstdint>
#include <format>
#include <string_view>
#include <utility>

namespace util::log
{

enum class Level : std::uint8_t
{
	info,
	success,
	warn,
	error,
};

void write(Level level, std::string_view message);

template <Level level, typename... Args>
void log(std::format_string<Args...> fmt, Args &&...args)
{
	write(level, std::format(fmt, std::forward<Args>(args)...));
}

template <typename... Args>
void info(std::format_string<Args...> fmt, Args &&...args)
{
	log<Level::info>(fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void success(std::format_string<Args...> fmt, Args &&...args)
{
	log<Level::success>(fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void warn(std::format_string<Args...> fmt, Args &&...args)
{
	log<Level::warn>(fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void error(std::format_string<Args...> fmt, Args &&...args)
{
	log<Level::error>(fmt, std::forward<Args>(args)...);
}

}

template <>
struct std::formatter<util::log::Level> : std::formatter<std::string_view>
{
	auto format(util::log::Level level, std::format_context &ctx) const
	{
		using enum util::log::Level;
		std::string_view name = "?";
		switch (level)
		{
		case info:
			name = "info";
			break;
		case success:
			name = "ok";
			break;
		case warn:
			name = "warn";
			break;
		case error:
			name = "error";
			break;
		}
		return std::formatter<std::string_view>::format(name, ctx);
	}
};
