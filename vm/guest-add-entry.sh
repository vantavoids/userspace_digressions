#!/bin/sh
set -eu

init="$1"
dir=/boot/loader/entries
out="$dir/systemfeur.conf"

rm -f "$out"
src=$(ls "$dir"/*.conf | grep -v rescue | sort -V | tail -n 1)

# no initramfs: the kernel mounts root itself, so it needs the partition GUID
# (it cannot resolve filesystem UUIDs) and the fs type up front.
fs_uuid=$(sed -n 's/^options .*root=UUID=\([^ ]*\).*/\1/p' "$src")
root_dev=$(blkid -U "$fs_uuid")
part_uuid=$(blkid -s PARTUUID -o value "$root_dev")
fs_type=$(blkid -s TYPE -o value "$root_dev")

sed \
    -e 's|^title .*|title Fedora (systemfeur :p)|' \
    -e '/^initrd /d' \
    -e "/^options /s|root=UUID=[^ ]*|root=PARTUUID=$part_uuid rootfstype=$fs_type rootwait|" \
    -e '/^options /s/ rhgb//; /^options /s/ quiet//' \
    -e "/^options /s|\$| init=$init console=tty0 console=ttyS0,115200|" \
    "$src" > "$out"

grub2-editenv - unset menu_auto_hide

echo "created $out from $src:"
cat "$out"
