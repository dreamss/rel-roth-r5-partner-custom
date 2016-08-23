#!/usr/bin/python
#
# Copyright (c) 2011 NVIDIA Corporation.  All rights reserved.
#
# NVIDIA Corporation and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA Corporation is strictly prohibited.
#

import common

def FullOTA_InstallEnd(info):
    RunDatabaseUpdateScript(info)
    try:
        info.input_zip.getinfo("RADIO/blob")
    except KeyError:
        return;
    else:
        # copy the data into the package.
        blob = info.input_zip.read("RADIO/blob")
        common.ZipWriteStr(info.output_zip, "blob", blob)
        # emit the script code to install this data on the device
        info.script.AppendExtra(
                """nv_copy_blob_file("blob", "/staging");""")


def IncrementalOTA_InstallEnd(info):
    RunDatabaseUpdateScript(info)
    try:
        info.target_zip.getinfo("RADIO/blob")
    except KeyError:
        return;
    else:
        target_blob = info.target_zip.read("RADIO/blob")
        try:
            info.source_zip.getinfo("RADIO/blob")
            # copy the data into the package.
            source_blob = info.source_zip.read("RADIO/blob")
            if source_blob == target_blob:
                # blob is unchanged from previous build; no
                # need to reprogram it
                return;
            else:
                # include the new blob in the OTA package
                common.ZipWriteStr(info.output_zip, "blob", target_blob)
                # emit the script code to install this data on the device
                info.script.AppendExtra(
                        """nv_copy_blob_file("blob", "/staging");""")
        except KeyError:
            # include the new blob in the OTA package
            common.ZipWriteStr(info.output_zip, "blob", target_blob)
            # emit the script code to install this data on the device
            info.script.AppendExtra(
                    """nv_copy_blob_file("blob", "/staging");""")


def RunDatabaseUpdateScript(info):
    # emit the script code to update the launcher database on the device
    info.script.AppendExtra("""mount("ext4", "EMMC", "/dev/block/platform/sdhci-tegra.3/by-name/UDA", "/data");""")
    info.script.AppendExtra("""run_program("/system/bin/sh", "-c", "[ -f /data/data/com.android.launcher/databases/launcher.db ] && /system/xbin/sqlite3 /data/data/com.android.launcher/databases/launcher.db 'UPDATE favorites SET intent=\\"#Intent;action=android.intent.action.MAIN;category=android.intent.category.LAUNCHER;launchFlags=0x10200000;component=com.nvidia.tegrazone3/.LaunchActivity;end\\" WHERE intent=\\"#Intent;action=android.intent.action.MAIN;category=android.intent.category.LAUNCHER;launchFlags=0x10200000;component=com.nvidia.roth.dashboard/.DashboardActivity;end\\";'");""")
    info.script.AppendExtra("""run_program("/system/bin/sh", "-c", "[ -f /data/data/com.android.launcher/databases/launcher.db ] && /system/xbin/sqlite3 /data/data/com.android.launcher/databases/launcher.db 'UPDATE favorites SET intent=\\"#Intent;action=android.intent.action.MAIN;category=android.intent.category.LAUNCHER;launchFlags=0x10200000;component=com.nvidia.shield.welcome/.FragmentViewer;end\\" WHERE intent=\\"#Intent;action=android.intent.action.MAIN;category=android.intent.category.LAUNCHER;launchFlags=0x10200000;component=com.nvidia.roth.welcomeapp/.HelpActivityLauncher;end\\";'");""")
    info.script.AppendExtra("""run_program("/system/bin/sh", "-c", "[ -f /data/data/com.android.providers.settings/databases/settings.db ] && /system/xbin/sqlite3 /data/data/com.android.providers.settings/databases/settings.db \\"update global set value=9 where name='network_preference';\\"");""")
    info.script.AppendExtra("""unmount("/data");""")

