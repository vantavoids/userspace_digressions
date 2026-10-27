#include "fs/kernel_fs.hpp"
#include "fs/mount.hpp"
#include "util/log.hpp"

#include <array>
#include <cerrno>
#include <expected>
#include <filesystem>
#include <string_view>
#include <sys/mount.h>
#include <system_error>

namespace fs
{

namespace
{

struct MountError
{
	std::string_view step;
	std::error_code code;
};

struct KernelFs
{
	const char *source;
	const char *target;
	const char *type;
	unsigned long flags;
	const char *options = nullptr;

	[[nodiscard]] std::expected<void, MountError> mount() const
	{
		std::error_code ec;
		std::filesystem::create_directories(target, ec);
		if (ec)
			return std::unexpected(MountError{.step = "mkdir", .code = ec});
		if (::mount(source, target, type, flags, options) != 0)
			return std::unexpected(MountError{
				.step = "mount",
				.code = std::error_code(errno, std::system_category()),
			});
		return {};
	}
};

constexpr std::array kernel_filesystems{
	KernelFs{
		.source = "proc",
		.target = "/proc",
		.type = "proc",
		.flags = MS_NOSUID | MS_NODEV | MS_NOEXEC,
	},
	KernelFs{
		.source = "sysfs",
		.target = "/sys",
		.type = "sysfs",
		.flags = MS_NOSUID | MS_NODEV | MS_NOEXEC,
	},
	KernelFs{
		.source = "devtmpfs",
		.target = "/dev",
		.type = "devtmpfs",
		.flags = MS_NOSUID,
		.options = "mode=0755",
	},
	KernelFs{
		.source = "devpts",
		.target = "/dev/pts",
		.type = "devpts",
		.flags = MS_NOSUID | MS_NOEXEC,
		.options = "gid=5,mode=0620,ptmxmode=0666",
	},
	KernelFs{
		.source = "tmpfs",
		.target = "/dev/shm",
		.type = "tmpfs",
		.flags = MS_NOSUID | MS_NODEV,
		.options = "mode=1777",
	},
	KernelFs{
		.source = "tmpfs",
		.target = "/run",
		.type = "tmpfs",
		.flags = MS_NOSUID | MS_NODEV,
		.options = "mode=0755",
	},
};

}

bool mount_kernel_filesystems()
{
	bool ok = true;
	for (const auto &filesystem : kernel_filesystems)
	{
		if (is_mount_point(filesystem.target))
		{
			util::log::info("{} already mounted", filesystem.target);
			continue;
		}
		if (auto result = filesystem.mount(); !result)
		{
			const auto &[step, code] = result.error();
			util::log::error("{} {} on {}: {}", step, filesystem.type, filesystem.target, code.message());
			ok = false;
			continue;
		}
		util::log::success("mounted {} on {}", filesystem.type, filesystem.target);
	}
	return ok;
}

}
