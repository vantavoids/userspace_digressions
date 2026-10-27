#!/bin/sh
# fedora 44 test VM for systemfeur
#
#   vm.sh install     create the disk and boot the live ISO to install fedora
#   vm.sh provision   copy build/systemfeur into the guest and add a GRUB entry
#   vm.sh run         boot the installed VM (serial console on this terminal)
#
# the VM must be powered off for provision. overridable through the environment:
# VM_DIR, VM_ISO, VM_DISK_SIZE, VM_MEM, VM_CPUS.
set -eu

here=$(cd "$(dirname "$0")" && pwd)
root=$(dirname "$here")

VM_DIR=${VM_DIR:-/goinfre/$USER/systemfeur-vm}
VM_ISO=${VM_ISO:-/sgoinfre/goinfre/ISO/fedora/Fedora-Workstation-Live-44-1.7.x86_64.iso}
VM_DISK_SIZE=${VM_DISK_SIZE:-20G}
VM_MEM=${VM_MEM:-4G}
VM_CPUS=${VM_CPUS:-4}

disk="$VM_DIR/disk.qcow2"
binary="$root/build/systemfeur"
guest_init=/usr/local/sbin/systemfeur

export LIBGUESTFS_BACKEND=direct

qemu() {
    qemu-system-x86_64 \
        -name systemfeur \
        -enable-kvm -machine q35 -cpu host \
        -m "$VM_MEM" -smp "$VM_CPUS" \
        -drive file="$disk",if=virtio,format=qcow2 \
        -nic user,model=virtio-net-pci \
        -vga virtio -display gtk \
        -serial mon:stdio \
        "$@"
}

need_disk() {
    if [ ! -f "$disk" ]; then
        echo "no disk at $disk, run '$0 install' first" >&2
        exit 1
    fi
}

case "${1:-}" in
install)
    if [ -f "$disk" ]; then
        echo "$disk already exists, remove it to reinstall" >&2
        exit 1
    fi
    mkdir -p "$VM_DIR"
    qemu-img create -f qcow2 "$disk" "$VM_DISK_SIZE"
    qemu -cdrom "$VM_ISO" -boot once=d
    ;;
provision)
    need_disk
    if [ ! -x "$binary" ]; then
        echo "$binary not found, build it first (cmake -B build && cmake --build build)" >&2
        exit 1
    fi
    virt-customize -a "$disk" \
        --upload "$binary:$guest_init" \
        --chmod "0755:$guest_init" \
        --run-command "chown root:root $guest_init" \
        --upload "$here/guest-add-entry.sh:/root/guest-add-entry.sh" \
        --run-command "sh /root/guest-add-entry.sh $guest_init" \
        --delete /root/guest-add-entry.sh
    ;;
run)
    need_disk
    qemu
    ;;
*)
    sed -n '2,10s/^# \{0,1\}//p' "$0"
    exit 1
    ;;
esac
