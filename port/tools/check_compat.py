#!/usr/bin/env python3
"""
Reports what the current machine can do with the patcher and the patched ROM.

The two questions this answers are different, and only the first one has a hard
answer here:

1. Can this machine run the patcher itself? Always. The Windows launcher is a
   static SDL2 executable with a 480x320 UI and the Android app draws the same
   screen with Canvas. Patching a 64 MiB ROM needs nothing but a CPU and a few
   hundred MiB of temporary memory, and the Android side uses the Storage
   Access Framework, so there is no storage-permission or GPU requirement.

2. Can this machine play the *game* the patcher produces? That is a Nintendo
   DS game, so it runs on a DS/3DS or under an NDS emulator. The decompilation
   percentage is irrelevant here: the 57% figure describes source reconstruction
   progress, and the game itself still comes from the ROM, so playback always
   means an emulator on this hardware. This script prints the evidence an
   emulator's requirements would be judged against and a rough tier.

No extra packages are used; everything reads the OS or well-known proc/sysctl
interfaces. Run with any Python 3.11+:

    python tools/check_compat.py        Windows / Linux / macOS
"""

import os
import platform
import shutil
import subprocess
import sys

TIERS = {
    "A": "comfortable for melonDS and DeSmuME at DS resolution",
    "B": "melonDS on lighter games, DeSmuME is the safer default",
    "C": "the patcher runs, but NDS emulation will struggle",
}


def ram_gib():
    """Physical RAM in GiB, best effort, standard library only."""
    if sys.platform == "win32":
        import ctypes

        class MEMORYSTATUSEX(ctypes.Structure):
            _fields_ = [
                ("dwLength", ctypes.c_ulong),
                ("dwMemoryLoad", ctypes.c_ulong),
                ("ullTotalPhys", ctypes.c_ulonglong),
                ("ullAvailPhys", ctypes.c_ulonglong),
                ("ullTotalPageFile", ctypes.c_ulonglong),
                ("ullAvailPageFile", ctypes.c_ulonglong),
                ("ullTotalVirtual", ctypes.c_ulonglong),
                ("ullAvailVirtual", ctypes.c_ulonglong),
                ("ullAvailExtendedVirtual", ctypes.c_ulonglong),
            ]

        stat = MEMORYSTATUSEX(dwLength=ctypes.sizeof(MEMORYSTATUSEX))
        if ctypes.windll.kernel32.GlobalMemoryStatusEx(ctypes.byref(stat)):
            return stat.ullTotalPhys / (1024 ** 3)
    elif os.path.exists("/proc/meminfo"):
        with open("/proc/meminfo", "r", encoding="ascii") as handle:
            for line in handle:
                if line.startswith("MemTotal:"):
                    kib = int(line.split()[1])
                    return kib / (1024 * 1024)
    elif sys.platform == "darwin":
        out = subprocess.run(
            ["sysctl", "-n", "hw.memsize"], capture_output=True, text=True
        ).stdout.strip()
        if out.isdigit():
            return int(out) / (1024 ** 3)
    return None


def os_label():
    if sys.platform == "win32":
        release = platform.release()
        build = platform.version()
        bitness = platform.architecture()[0]
        return "Windows %s (build %s), %s" % (release, build, bitness)
    if sys.platform == "darwin":
        return "macOS %s" % platform.mac_ver()[0]
    if sys.platform.startswith("linux"):
        name = platform.freedesktop_os_release().get("PRETTY_NAME", "Linux")
        return name
    return platform.platform()


def tier_of(cpu_count, ram_gib_value, machine):
    if ram_gib_value is None:
        ram_gib_value = 0
    cheap = machine in ("x86_64", "AMD64", "aarch64", "arm64")
    if cheap and cpu_count >= 4 and ram_gib_value >= 4:
        return "A"
    if cheap and ram_gib_value >= 2 and cpu_count >= 2:
        return "B"
    return "C"


def main():
    machine = platform.machine()
    cpu_count = os.cpu_count() or 1
    ram = ram_gib()
    disk = shutil.disk_usage(os.path.abspath(os.getcwd()))
    tier = tier_of(cpu_count, ram, machine)

    print("Compatibility report")
    print("=" * 60)
    print("OS:              %s" % os_label())
    print("Architecture:    %s" % machine)
    print("Logical CPUs:    %d" % cpu_count)
    if ram is not None:
        print("Physical RAM:    %.1f GiB" % ram)
    else:
        print("Physical RAM:    (could not be read)")
    print("Free disk:       %.1f GiB (of %.1f GiB total)" % (
        disk.free / (1024 ** 3), disk.total / (1024 ** 3)))
    print("Python:          %s" % sys.version.split()[0])
    print()
    print("Patcher:         runs anywhere; no GPU, storage grant or free "
          "disk requirement beyond the ROM and its output.")
    print()
    print("NDS emulation tier: %s" % tier)
    print("  %s" % TIERS[tier])
    print()
    print("To actually play the patched ROM you need real DS hardware or an NDS "
          "emulator such as melonDS (recommended) or DeSmuME.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
