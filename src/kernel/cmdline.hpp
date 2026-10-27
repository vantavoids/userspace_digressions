#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace kernel
{

class Cmdline
{
public:
	static const Cmdline &get();

	[[nodiscard]] std::optional<std::string_view> value(std::string_view key) const;
	[[nodiscard]] bool has(std::string_view flag) const;

private:
	explicit Cmdline(std::string_view raw);

	std::vector<std::string> args;
};

}
