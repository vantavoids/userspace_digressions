#include "fs/root_fs.hpp"
#include "fs/blockdev.hpp"
#include "fs/fsck.hpp"
#include "fs/fstab.hpp"
#include "fs/mount.hpp"
#include "kernel/cmdline.hpp"
#include "util/log.hpp"

#include <algorithm>
#include <cerrno>
#include <format>
#include <optional>
#include <string>
#include <sys/mount.h>
#include <sys/reboot.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <system_error>
#include <unistd.h>

namespace fs
{

namespace
{

std::optional<std::string> find_root_device()
{
	if (const auto root = kernel::Cmdline::get().value("root"))
		if (auto device = blockdev::resolve(*root))
			return device;

	// does not work for btrfs, which reports an anonymous device number for / :(
	struct stat st{};
	if (stat("/", &st) != 0)
		return std::nullopt;
	return blockdev::from_device_number(st.st_dev);
}

bool is_read_only(const char *path)
{
	struct statvfs st{};
	return statvfs(path, &st) == 0 && (st.f_flag & ST_RDONLY);
}

MountOptions root_mount_options()
{
	const auto entries = read_fstab();
	if (!entries)
		return {};
	const auto root = std::ranges::find(*entries, "/", &FstabEntry::target);
	if (root == entries->end())
		return {};
	return MountOptions::parse(root->options);
}

std::error_code remount_root(const MountOptions &options)
{
	const char *data = options.data.empty() ? nullptr : options.data.c_str();
	if (::mount(nullptr, "/", nullptr, MS_REMOUNT | options.flags, data) != 0)
		return {errno, std::system_category()};
	return {};
}

}

bool check_and_remount_root()
{
	if (!is_read_only("/"))
	{
		util::log::warn("/ is already read-write, skipping fsck (no 'ro' on the kernel command line?)");
		return true;
	}

	const auto device = find_root_device();
	if (!device)
	{
		util::log::error("cannot find the root device, leaving / read-only");
		return false;
	}

	switch (fsck(*device))
	{
	case FsckResult::clean:
	case FsckResult::corrected:
		break;
	case FsckResult::unavailable:
		util::log::warn("mounting / unchecked");
		break;
	case FsckResult::failed:
		util::log::error("leaving / read-only");
		return false;
	case FsckResult::reboot_required:
		sync();
		reboot(RB_AUTOBOOT);
		util::log::error("reboot: {}", std::error_code(errno, std::system_category()).message());
		return false;
	}

	auto options = root_mount_options();
	auto ec = remount_root(options);
	if (ec && options != MountOptions{})
	{
		util::log::warn("remount / with the fstab options: {}, retrying without them", ec.message());
		options = {};
		ec = remount_root(options);
	}
	if (ec)
	{
		util::log::error("remount /: {}", ec.message());
		return false;
	}

	util::log::success(
		"remounted / {}{}",
		options.flags & MS_RDONLY ? "read-only" : "read-write",
		options.data.empty() ? std::string() : std::format(" ({})", options.data)
	);
	return true;
}

}
