# SPDX-License-Identifier: MIT
# SPDX-FileCopyrightText: 2021-2022 Harald Sitter <sitter@kde.org>
# SPDX-FileCopyrightText: 2023 Marco Martin <mart@kde.org>
# SPDX-FileCopyrightText: 2026 Méven Car <meven@kde.org>
# SPDX-FileCopyrightText: 2026 Sebastian Englbrecht

"""Shared setup and helpers for the Dolphin GUI tests that drive Dolphin through AT-SPI."""

import os
import shutil
import unittest
from xml.etree import ElementTree

from appium import webdriver
from appium.options.common.base import AppiumOptions
from appium.webdriver.common.appiumby import AppiumBy
from selenium.common.exceptions import TimeoutException
from selenium.webdriver.common.action_chains import ActionChains
from selenium.webdriver.support.ui import WebDriverWait

ITEM_LIST_VIEW = "DolphinItemListView"

URL_NAVIGATOR_TOGGLE = "KUrlNavigatorToggleButton"
URL_NAVIGATOR_EDIT = "DolphinUrlNavigator.KUrlComboBox.KLineEdit"
URL_NAVIGATOR_CLEAR = URL_NAVIGATOR_EDIT + ".QLineEditIconButton"

WAIT_TIMEOUT = 20


def byObjectName(name):
    """An XPath to the widget with the object name name, which ends its accessibility id."""
    return "//*[contains(@accessibility-id, '.{}')]".format(name)


class DolphinTestCase(unittest.TestCase):
    """
    Starts one Dolphin for all tests of a class.

    The tests of a class run in the order of their names and share the window, so a test may
    start where the previous one stopped.
    """

    # The folders and files below $HOME/<testRoot> that setUpClass creates; folders end with "/"
    testRoot = None
    testTree = ()

    @classmethod
    def setUpClass(cls):
        if cls.testRoot:
            cls.root = os.path.join(os.environ["HOME"], cls.testRoot)
            shutil.rmtree(cls.root, ignore_errors=True)
            os.makedirs(cls.root)
            for path in cls.testTree:
                fullPath = os.path.join(cls.root, path)
                if path.endswith("/"):
                    os.makedirs(fullPath, exist_ok=True)
                else:
                    os.makedirs(os.path.dirname(fullPath), exist_ok=True)
                    with open(fullPath, "w") as f:
                        f.write(path)

        cls.prepareEnvironment()

        options = AppiumOptions()
        options.set_capability("timeouts", {"implicit": 10000})
        options.set_capability("app", cls.application())
        options.set_capability("environ", {
            "LC_ALL": "en_US.UTF-8",
        })
        cls.driver = webdriver.Remote(command_executor="http://127.0.0.1:4723", options=options)

    @classmethod
    def tearDownClass(cls):
        cls.driver.quit()
        if cls.testRoot:
            shutil.rmtree(cls.root, ignore_errors=True)

    @classmethod
    def prepareEnvironment(cls):
        """Runs before Dolphin starts, for example to write its configuration."""

    @staticmethod
    def application():
        """The Dolphin of this build when ctest runs the test, the installed one otherwise."""
        binaryDir = os.environ.get("CMAKE_BINARY_DIR")
        if binaryDir and os.path.exists(os.path.join(binaryDir, "dolphin")):
            return os.path.join(binaryDir, "dolphin")
        return "org.kde.dolphin.desktop"

    def setUp(self):
        self.problemsBefore = self.problemCount()

    def tearDown(self):
        # Only a test that failed itself takes a screenshot, not every test after the first failure
        if self.problemCount() > self.problemsBefore:
            self.driver.get_screenshot_as_file("failed_test_shot_{}.png".format(self.id()))

    def problemCount(self):
        result = self._outcome.result
        return len(result.failures) + len(result.errors)

    # Waiting

    def assertEventually(self, actual, expected, message=None):
        """Waits until actual() returns expected, and fails with the last value otherwise."""
        try:
            WebDriverWait(self.driver, WAIT_TIMEOUT).until(lambda _: actual() == expected)
        except TimeoutException:
            pass
        self.assertEqual(actual(), expected, message)

    # Reading the window

    def accessibleTree(self):
        return ElementTree.fromstring(self.driver.page_source)

    def itemLists(self):
        """The accessible lists of the item views, a single one unless the columns view is shown."""
        return [element for element in self.accessibleTree().iter("list") if ITEM_LIST_VIEW in element.get("accessibility-id", "")]

    def itemNames(self):
        """The names of the items in the view, in the order of the view."""
        lists = self.itemLists()
        if len(lists) != 1:
            return None
        return [item.get("name") for item in lists[0].findall("list_item")]

    def windowTitle(self):
        return self.driver.find_element(by=AppiumBy.XPATH, value="//frame").text

    # Acting

    def typeText(self, text):
        """
        Types text with the keyboard into the widget that has the focus.

        Unlike send_keys() on an element, which inserts the text through the accessibility
        interface, this emits QLineEdit::textEdited like a user does.
        """
        ActionChains(self.driver).send_keys(text).perform()

    def pressShortcut(self, modifier, key):
        """Presses key while modifier is held, for example Keys.CONTROL and "f"."""
        ActionChains(self.driver).key_down(modifier).send_keys(key).key_up(modifier).perform()

    def openLocationInEditor(self, path):
        """Switches the location bar into the editable mode and types path, without confirming it."""
        toggle = self.driver.find_element(by=AppiumBy.ACCESSIBILITY_ID, value=URL_NAVIGATOR_TOGGLE)
        toggle.click()
        self.driver.find_element(by=AppiumBy.ACCESSIBILITY_ID, value=URL_NAVIGATOR_CLEAR).click()
        self.driver.find_element(by=AppiumBy.ACCESSIBILITY_ID, value=URL_NAVIGATOR_EDIT).send_keys(path)
        return toggle

    def openLocation(self, path):
        """Opens path through the location bar, and returns to the breadcrumb mode."""
        self.openLocationInEditor(path).click()


def main():
    unittest.main(verbosity=2)
