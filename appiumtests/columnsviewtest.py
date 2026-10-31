#!/usr/bin/env python3

# SPDX-License-Identifier: MIT
# SPDX-FileCopyrightText: 2026 Méven Car <meven@kde.org>

import os
import shutil
import unittest
from xml.etree import ElementTree

from appium import webdriver
from appium.options.common.base import AppiumOptions
from appium.webdriver.common.appiumby import AppiumBy
from selenium.webdriver.common.keys import Keys
from selenium.webdriver.support.ui import WebDriverWait

COLUMN_LIST = "DolphinColumnPane.KItemListContainer.DolphinItemListView"


class ColumnsViewTests(unittest.TestCase):
    """
    The columns view shows one column per folder, from the opened folder to the selected one.
    The tests run in order, and each one starts where the previous one stopped.
    """

    @classmethod
    def setUpClass(self):
        self.root = "{}/columnsTest".format(os.environ["HOME"])
        shutil.rmtree(self.root, ignore_errors=True)
        os.makedirs("{}/alpha/inner".format(self.root))
        os.makedirs("{}/beta".format(self.root))
        for path in ("alpha/a1.txt", "alpha/inner/deep.txt", "beta/b1.txt", "top.txt"):
            with open("{}/{}".format(self.root, path), "w") as f:
                f.write(path)

        # Each folder keeps its own view mode, and the test folder is saved in the columns mode
        # (ViewMode 3)
        configHome = os.environ.get("XDG_CONFIG_HOME", "{}/.config".format(os.environ["HOME"]))
        os.makedirs(configHome, exist_ok=True)
        with open("{}/dolphinrc".format(configHome), "a") as f:
            f.write("[General]\nGlobalViewProps=false\n")
        with open("{}/.directory".format(self.root), "w") as f:
            f.write("[Dolphin]\nVersion=4\nViewMode=3\n")

        options = AppiumOptions()
        options.set_capability("timeouts", {'implicit': 10000})
        options.set_capability("app", "org.kde.dolphin.desktop")
        options.set_capability("environ", {
            "LC_ALL": "en_US.UTF-8",
        })

        self.driver = webdriver.Remote(
            command_executor='http://127.0.0.1:4723',
            options=options)

    def tearDown(self):
        if not self._outcome.result.wasSuccessful():
            self.driver.get_screenshot_as_file("failed_test_shot_{}.png".format(self.id()))

    @classmethod
    def tearDownClass(self):
        self.driver.quit()
        shutil.rmtree(self.root, ignore_errors=True)

    def columns(self):
        """Returns the folder name and the item names of each column, from left to right."""
        root = ElementTree.fromstring(self.driver.page_source)
        columns = [column for column in root.iter("list") if COLUMN_LIST in column.get("accessibility-id", "")]
        # The description holds the location, and each column lists a subfolder of the one on its left
        columns.sort(key=lambda column: column.get("description").count("/"))
        return [(column.get("name"), [item.get("name") for item in column.findall("list_item")]) for column in columns]

    def windowTitle(self):
        return self.driver.find_element(by=AppiumBy.XPATH, value="//frame").text

    def assertColumns(self, expected, folder):
        """Waits until the view shows the expected columns and the window title names folder."""
        try:
            WebDriverWait(self.driver, 20).until(lambda _: self.columns() == expected and self.windowTitle().startswith(folder + " "))
        except Exception:
            pass
        self.assertEqual(self.columns(), expected)
        self.assertTrue(self.windowTitle().startswith(folder + " "), self.windowTitle())

    def pressKeys(self, column, keys):
        """Types keys into the column that lists the folder called column, which has the keyboard focus."""
        self.driver.find_element(by=AppiumBy.XPATH, value="//list[@name='{}' and contains(@accessibility-id, '{}')]".format(column, COLUMN_LIST)).send_keys(keys)

    def test_1_opened_folder_is_the_first_column(self):
        editButton = self.driver.find_element(by=AppiumBy.ACCESSIBILITY_ID, value="KUrlNavigatorToggleButton")
        editButton.click()
        self.driver.find_element(by=AppiumBy.ACCESSIBILITY_ID, value="DolphinUrlNavigator.KUrlComboBox.KLineEdit.QLineEditIconButton").click()
        self.driver.find_element(by=AppiumBy.ACCESSIBILITY_ID, value="DolphinUrlNavigator.KUrlComboBox.KLineEdit").send_keys(self.root)
        editButton.click()

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


if __name__ == '__main__':
    unittest.main(verbosity=2)
