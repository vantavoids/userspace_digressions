#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <sys/types.h>

namespace fs::blockdev
{

struct FsInfo
{
	std::string type;
	std::string uuid;
	std::string label;
};

std::optional<FsInfo> probe(const std::string &device);

// turns an fstab-style spec (UUID=, LABEL=, PARTUUID=, PARTLABEL= or a path) into a path.
std::optional<std::string> resolve(std::string_view spec);

std::optional<std::string> from_device_number(dev_t device);

}
