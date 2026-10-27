#include "fs/blockdev.hpp"
#include "util/file.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <filesystem>
#include <format>
#include <fstream>
#include <ios>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <sys/sysmacros.h>
#include <sys/types.h>
#include <system_error>
#include <unistd.h>
#include <utility>

namespace fs::blockdev
{

namespace
{

bool iequals(std::string_view a, std::string_view b)
{
	return std::ranges::equal(a, b, [](unsigned char x, unsigned char y) {
		return std::tolower(x) == std::tolower(y);
	});
}

std::optional<std::string> uevent_field(const std::filesystem::path &uevent, std::string_view key)
{
	auto content = util::read_file(uevent);
	if (!content)
		return std::nullopt;
	for (auto range : *content | std::views::split('\n'))
	{
		std::string_view line(range);
		if (line.size() > key.size() && line.starts_with(key) && line[key.size()] == '=')
			return std::string(line.substr(key.size() + 1));
	}
	return std::nullopt;
}

class Superblock
{
public:
	static std::optional<Superblock> read(const std::string &device)
	{
		std::ifstream file(device, std::ios::binary);
		if (!file)
			return std::nullopt;
		std::string data(read_size, '\0');
		file.read(data.data(), static_cast<std::streamsize>(data.size()));
		data.resize(static_cast<std::size_t>(file.gcount()));
		return Superblock(std::move(data));
	}

	[[nodiscard]] std::string_view at(std::size_t offset, std::size_t length) const
	{
		if (offset + length > data.size())
			return {};
		return std::string_view(data).substr(offset, length);
	}

	[[nodiscard]] bool has_magic(std::size_t offset, std::string_view magic) const
	{
		return at(offset, magic.size()) == magic;
	}

private:
	// enough to reach the btrfs superblock, which lives at 64 KiB
	static constexpr std::size_t read_size = 0x11000;

	explicit Superblock(std::string bytes) : data(std::move(bytes)) {}

	std::string data;
};

std::string format_uuid(std::string_view raw)
{
	std::string uuid;
	for (std::size_t i = 0; i < raw.size(); ++i)
	{
		if (i == 4 || i == 6 || i == 8 || i == 10)
			uuid += '-';
		uuid += std::format("{:02x}", static_cast<unsigned char>(raw[i]));
	}
	return uuid;
}

std::string format_label(std::string_view raw)
{
	raw = raw.substr(0, raw.find('\0'));
	while (raw.ends_with(' '))
		raw.remove_suffix(1);
	return std::string(raw);
}

std::optional<FsInfo> probe_ext(const Superblock &sb)
{
	if (!sb.has_magic(0x438, "\x53\xef"))
		return std::nullopt;
	// the ext4 driver also mounts ext2 and ext3
	return FsInfo{
		.type = "ext4",
		.uuid = format_uuid(sb.at(0x468, 16)),
		.label = format_label(sb.at(0x478, 16)),
	};
}

std::optional<FsInfo> probe_btrfs(const Superblock &sb)
{
	if (!sb.has_magic(0x10040, "_BHRfS_M"))
		return std::nullopt;
	return FsInfo{
		.type = "btrfs",
		.uuid = format_uuid(sb.at(0x10020, 16)),
		.label = format_label(sb.at(0x1012b, 256)),
	};
}

std::optional<FsInfo> probe_xfs(const Superblock &sb)
{
	if (!sb.has_magic(0, "XFSB"))
		return std::nullopt;
	return FsInfo{
		.type = "xfs",
		.uuid = format_uuid(sb.at(32, 16)),
		.label = format_label(sb.at(108, 12)),
	};
}

std::optional<FsInfo> probe_swap(const Superblock &sb)
{
	const auto page_size = static_cast<std::size_t>(sysconf(_SC_PAGESIZE));
	if (!sb.has_magic(page_size - 10, "SWAPSPACE2"))
		return std::nullopt;
	return FsInfo{
		.type = "swap",
		.uuid = format_uuid(sb.at(0x40c, 16)),
		.label = format_label(sb.at(0x41c, 16)),
	};
}

std::optional<FsInfo> probe_vfat(const Superblock &sb)
{
	if (!sb.has_magic(0x1fe, "\x55\xaa"))
		return std::nullopt;

	std::size_t id_offset = 0;
	if (sb.has_magic(0x52, "FAT32   "))
		id_offset = 0x43;
	else if (sb.has_magic(0x36, "FAT1"))
		id_offset = 0x27;
	else
		return std::nullopt;

	const auto id = sb.at(id_offset, 4);
	auto label = format_label(sb.at(id_offset + 4, 11));
	if (label == "NO NAME")
		label.clear();
	return FsInfo{
		.type = "vfat",
		.uuid = std::format(
			"{:02X}{:02X}-{:02X}{:02X}",
			static_cast<unsigned char>(id[3]),
			static_cast<unsigned char>(id[2]),
			static_cast<unsigned char>(id[1]),
			static_cast<unsigned char>(id[0])
		),
		.label = std::move(label),
	};
}

constexpr std::array probers{&probe_ext, &probe_btrfs, &probe_xfs, &probe_swap, &probe_vfat};

struct Device
{
	std::string path;
	std::filesystem::path uevent;
};

template <typename Predicate>
std::optional<std::string> find_device(Predicate matches)
{
	std::error_code ec;
	for (const auto &entry : std::filesystem::directory_iterator("/sys/class/block", ec))
	{
		auto uevent = entry.path() / "uevent";
		auto name = uevent_field(uevent, "DEVNAME");
		if (!name)
			continue;
		Device device{.path = "/dev/" + *name, .uevent = std::move(uevent)};
		if (matches(device))
			return std::move(device.path);
	}
	return std::nullopt;
}

}

std::optional<FsInfo> probe(const std::string &device)
{
	const auto sb = Superblock::read(device);
	if (!sb)
		return std::nullopt;
	for (const auto prober : probers)
		if (auto info = prober(*sb))
			return info;
	return std::nullopt;
}

std::optional<std::string> resolve(std::string_view spec)
{
	const auto tag = [spec](std::string_view prefix) -> std::optional<std::string_view> {
		if (!spec.starts_with(prefix))
			return std::nullopt;
		auto value = spec.substr(prefix.size());
		if (value.size() >= 2 && value.front() == '"' && value.back() == '"')
			value = value.substr(1, value.size() - 2);
		return value;
	};

	if (const auto uuid = tag("UUID="))
		return find_device([&](const Device &device) {
			const auto info = probe(device.path);
			return info && iequals(info->uuid, *uuid);
		});
	if (const auto label = tag("LABEL="))
		return find_device([&](const Device &device) {
			const auto info = probe(device.path);
			return info && info->label == *label;
		});
	if (const auto partuuid = tag("PARTUUID="))
		return find_device([&](const Device &device) {
			const auto id = uevent_field(device.uevent, "PARTUUID");
			return id && iequals(*id, *partuuid);
		});
	if (const auto partlabel = tag("PARTLABEL="))
		return find_device([&](const Device &device) {
			return uevent_field(device.uevent, "PARTNAME") == *partlabel;
		});
	return std::string(spec);
}

std::optional<std::string> from_device_number(dev_t device)
{
	const auto uevent = std::format("/sys/dev/block/{}:{}/uevent", major(device), minor(device));
	if (auto name = uevent_field(uevent, "DEVNAME"))
		return "/dev/" + *name;
	return std::nullopt;
}

}
