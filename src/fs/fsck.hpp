#pragma once

#include <cstdint>
#include <string>

namespace fs
{

enum class FsckResult : std::uint8_t
{
	clean,
	corrected,
	reboot_required,
	failed,
	unavailable,
};

FsckResult fsck(const std::string &device);

}
