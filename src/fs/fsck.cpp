#include "fs/fsck.hpp"
#include "util/log.hpp"
#include "util/process.hpp"

#include <string>

namespace fs
{

namespace
{

// fsck(8) exit status bits, anything else means the filesystem is unusable
constexpr int corrected_bit = 1;
constexpr int reboot_bit = 2;

}

FsckResult fsck(const std::string &device)
{
	util::log::info("checking {}", device);
	const auto status = util::process::run({"fsck", "-a", device});
	if (!status)
	{
		util::log::warn("cannot run fsck on {}: {}", device, status.error().message());
		return FsckResult::unavailable;
	}
	if (*status & ~(corrected_bit | reboot_bit))
	{
		util::log::error("fsck failed on {} (status {})", device, *status);
		return FsckResult::failed;
	}
	if (*status & reboot_bit)
	{
		util::log::warn("fsck repaired {} and requires a reboot", device);
		return FsckResult::reboot_required;
	}
	if (*status & corrected_bit)
	{
		util::log::info("fsck corrected errors on {}", device);
		return FsckResult::corrected;
	}
	return FsckResult::clean;
}

}
