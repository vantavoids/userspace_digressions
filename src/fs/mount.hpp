#pragma once

#include <span>
#include <string>

namespace fs
{

bool is_mount_point(const char *path);

struct MountOptions
{
	unsigned long flags = 0;
	std::string data;

	bool operator==(const MountOptions &) const = default;

	// splits mount(8) options into MS_* flags and the filesystem specific data string.
	static MountOptions parse(std::span<const std::string> options);
};

}
