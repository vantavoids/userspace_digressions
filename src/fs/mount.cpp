#include "fs/mount.hpp"

#include <algorithm>
#include <array>
#include <fcntl.h>
#include <linux/stat.h>
#include <span>
#include <string>
#include <string_view>
#include <sys/mount.h>
#include <sys/stat.h>

namespace fs
{

namespace
{

struct MountFlag
{
	std::string_view name;
	unsigned long flag;
	bool set;
};

constexpr std::array mount_flags{
	MountFlag{.name = "ro", .flag = MS_RDONLY, .set = true},
	MountFlag{.name = "rw", .flag = MS_RDONLY, .set = false},
	MountFlag{.name = "nosuid", .flag = MS_NOSUID, .set = true},
	MountFlag{.name = "suid", .flag = MS_NOSUID, .set = false},
	MountFlag{.name = "nodev", .flag = MS_NODEV, .set = true},
	MountFlag{.name = "dev", .flag = MS_NODEV, .set = false},
	MountFlag{.name = "noexec", .flag = MS_NOEXEC, .set = true},
	MountFlag{.name = "exec", .flag = MS_NOEXEC, .set = false},
	MountFlag{.name = "sync", .flag = MS_SYNCHRONOUS, .set = true},
	MountFlag{.name = "async", .flag = MS_SYNCHRONOUS, .set = false},
	MountFlag{.name = "dirsync", .flag = MS_DIRSYNC, .set = true},
	MountFlag{.name = "mand", .flag = MS_MANDLOCK, .set = true},
	MountFlag{.name = "nomand", .flag = MS_MANDLOCK, .set = false},
	MountFlag{.name = "noatime", .flag = MS_NOATIME, .set = true},
	MountFlag{.name = "atime", .flag = MS_NOATIME, .set = false},
	MountFlag{.name = "nodiratime", .flag = MS_NODIRATIME, .set = true},
	MountFlag{.name = "diratime", .flag = MS_NODIRATIME, .set = false},
	MountFlag{.name = "relatime", .flag = MS_RELATIME, .set = true},
	MountFlag{.name = "norelatime", .flag = MS_RELATIME, .set = false},
	MountFlag{.name = "strictatime", .flag = MS_STRICTATIME, .set = true},
	MountFlag{.name = "nostrictatime", .flag = MS_STRICTATIME, .set = false},
	MountFlag{.name = "lazytime", .flag = MS_LAZYTIME, .set = true},
	MountFlag{.name = "nolazytime", .flag = MS_LAZYTIME, .set = false},
	MountFlag{.name = "bind", .flag = MS_BIND, .set = true},
	MountFlag{.name = "rbind", .flag = MS_BIND | MS_REC, .set = true},
	MountFlag{.name = "silent", .flag = MS_SILENT, .set = true},
	MountFlag{.name = "loud", .flag = MS_SILENT, .set = false},
};

// understood by mount(8) or the init system, never passed to the kernel
constexpr auto userspace_options = std::to_array<std::string_view>({
	"defaults",
	"auto",
	"noauto",
	"user",
	"nouser",
	"users",
	"owner",
	"group",
	"nofail",
	"_netdev",
});

bool is_userspace_option(std::string_view option)
{
	return std::ranges::contains(userspace_options, option)
		|| option.starts_with("x-")
		|| option.starts_with("comment=");
}

}

bool is_mount_point(const char *path)
{
	struct statx st{};
	if (statx(AT_FDCWD, path, AT_NO_AUTOMOUNT, 0, &st) != 0)
		return false;
	return (st.stx_attributes_mask & STATX_ATTR_MOUNT_ROOT)
		&& (st.stx_attributes & STATX_ATTR_MOUNT_ROOT);
}

MountOptions MountOptions::parse(std::span<const std::string> options)
{
	MountOptions result;
	for (const auto &option : options)
	{
		if (const auto it = std::ranges::find(mount_flags, option, &MountFlag::name); it != mount_flags.end())
		{
			if (it->set)
				result.flags |= it->flag;
			else
				result.flags &= ~it->flag;
		}
		else if (!is_userspace_option(option))
		{
			if (!result.data.empty())
				result.data += ',';
			result.data += option;
		}
	}
	return result;
}

}
