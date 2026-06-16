#SPDX-FileCopyrightText: 2026 Felix Ernst <felixernst@kde.org>
#SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL

import os
import sys
from pathlib import Path
import xml.etree.ElementTree as ET

use_tab_setting_detected = False

XDG_CONFIG_HOME = Path(os.getenv("XDG_CONFIG_HOME") or (Path.home() / ".config"))
custom_settings_file = XDG_CONFIG_HOME / "dolphinrc"
print(custom_settings_file)
if custom_settings_file.exists():
    with open(custom_settings_file, "r+") as f:
        lines = [line for line in f if line.find("UseTabForSwitchingSplitView=true") >= 0]
        if len(lines) > 0:
            use_tab_setting_detected = True

XDG_DATA_HOME = Path(os.getenv("XDG_DATA_HOME") or (Path.home() / ".local/share"))
custom_ui_file = XDG_DATA_HOME / "kxmlgui5/dolphin/dolphinui.rc"
if use_tab_setting_detected and custom_ui_file.exists():
    tree = ET.parse(custom_ui_file)
    root = tree.getroot()
    for actionProperties in root.iter("ActionProperties"):
        ET.SubElement(actionProperties, "Action", {"name":"focus_inactive_split_view", "shortcut":"Ctrl+F3; Tab"})
    tree.write(custom_ui_file, encoding="utf-8")
