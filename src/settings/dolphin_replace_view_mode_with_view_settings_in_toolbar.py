#SPDX-FileCopyrightText: 2025 Jin Liu <m.liu.jin@gmail.com>
#SPDX-License-Identifier: GPL-2.0-or-later

import os
from pathlib import Path
import xml.etree.ElementTree as ET

XDG_DATA_HOME = Path(os.getenv("XDG_DATA_HOME") or (Path.home() / ".local/share"))
custom_ui_file = XDG_DATA_HOME / "kxmlgui5/dolphin/dolphinui.rc"
if custom_ui_file.exists():
    tree = ET.parse(custom_ui_file)
    root = tree.getroot()
    for toolbar in root.iter("ToolBar"):
        for item in toolbar.iter("Action"):
            if item.attrib.get("name") == "view_mode":
                item.set("name", "view_settings")
    tree.write(custom_ui_file, encoding="utf-8")
