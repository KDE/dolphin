#!/usr/bin/env python3

# SPDX-License-Identifier: MIT
# SPDX-FileCopyrightText: 2026 Méven Car <meven@kde.org>
# SPDX-FileCopyrightText: 2026 Sebastian Englbrecht

import os

from appium.webdriver.common.appiumby import AppiumBy
from selenium.webdriver.common.keys import Keys

from dolphinappium import ITEM_LIST_VIEW, DolphinTestCase, main

COLUMN_LIST = "DolphinColumnPane.KItemListContainer." + ITEM_LIST_VIEW


class ColumnsViewTests(DolphinTestCase):
    """
    The columns view shows one column per folder, from the opened folder to the selected one.
    The tests run in order, and each one starts where the previous one stopped.
    """

    testRoot = "columnsTest"
    testTree = ("alpha/a1.txt", "alpha/inner/deep.txt", "beta/b1.txt", "top.txt")

    @classmethod
    def prepareEnvironment(cls):
        # Each folder keeps its own view mode, and the test folder is saved in the columns mode
        # (ViewMode 3)
        configHome = os.environ.get("XDG_CONFIG_HOME", "{}/.config".format(os.environ["HOME"]))
        os.makedirs(configHome, exist_ok=True)
        with open("{}/dolphinrc".format(configHome), "a") as f:
            f.write("[General]\nGlobalViewProps=false\n")
        with open("{}/.directory".format(cls.root), "w") as f:
            f.write("[Dolphin]\nVersion=4\nViewMode=3\n")

    def columns(self):
        """Returns the folder name and the item names of each column, from left to right."""
        columns = [column for column in self.itemLists() if COLUMN_LIST in column.get("accessibility-id", "")]
        # The description holds the location, and each column lists a subfolder of the one on its left
        columns.sort(key=lambda column: column.get("description").count("/"))
        return [(column.get("name"), [item.get("name") for item in column.findall("list_item")]) for column in columns]

    def assertColumns(self, expected, folder):
        """Waits until the view shows the expected columns and the window title names folder."""
        self.assertEventually(self.columns, expected)
        self.assertEventually(lambda: self.windowTitle().split(" ")[0], folder)

    def pressKeys(self, column, keys):
        """Types keys into the column that lists the folder called column, which has the keyboard focus."""
        self.driver.find_element(by=AppiumBy.XPATH, value="//list[@name='{}' and contains(@accessibility-id, '{}')]".format(column, COLUMN_LIST)).send_keys(keys)

    def test_1_opened_folder_is_the_first_column(self):
        self.openLocation(self.root)

        self.assertColumns([("columnsTest", ["alpha", "beta", "top.txt"])], "columnsTest")

    def test_2_selected_folder_shows_its_content(self):
        self.pressKeys("columnsTest", "a")

        self.assertColumns([("columnsTest", ["alpha", "beta", "top.txt"]), ("alpha", ["inner", "a1.txt"])], "alpha")

    def test_3_right_enters_the_folder_and_shows_its_first_folder(self):
        self.pressKeys("columnsTest", Keys.RIGHT)

        self.assertColumns([("columnsTest", ["alpha", "beta", "top.txt"]), ("alpha", ["inner", "a1.txt"]), ("inner", ["deep.txt"])], "inner")

    def test_4_left_keeps_the_columns(self):
        self.pressKeys("alpha", Keys.LEFT)

        self.assertColumns([("columnsTest", ["alpha", "beta", "top.txt"]), ("alpha", ["inner", "a1.txt"]), ("inner", ["deep.txt"])], "inner")

    def test_5_another_folder_replaces_the_columns_after_it(self):
        self.pressKeys("columnsTest", Keys.DOWN)

        self.assertColumns([("columnsTest", ["alpha", "beta", "top.txt"]), ("beta", ["b1.txt"])], "beta")

    def test_6_selected_file_closes_the_columns_after_it(self):
        self.pressKeys("columnsTest", Keys.DOWN)

        self.assertColumns([("columnsTest", ["alpha", "beta", "top.txt"])], "columnsTest")


if __name__ == "__main__":
    main()
