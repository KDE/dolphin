#!/usr/bin/env python3

# SPDX-License-Identifier: MIT
# SPDX-FileCopyrightText: 2021-2022 Harald Sitter <sitter@kde.org>
# SPDX-FileCopyrightText: 2023 Marco Martin <mart@kde.org>

import unittest
from appium import webdriver
from appium.webdriver.common.appiumby import AppiumBy
from appium.options.common.base import AppiumOptions
from selenium.webdriver.support.ui import WebDriverWait
from selenium.webdriver.common.keys import Keys
from selenium.webdriver.common.by import By
from selenium.webdriver.common.action_chains import ActionChains
from selenium.webdriver.support import expected_conditions as EC
import time
import os
import sys

import time

class DolphinTests(unittest.TestCase):
    @classmethod
    def setUpClass(self):
        options = AppiumOptions()
        options.set_capability("timeouts", {'implicit': 10000})
        options.set_capability("app", "org.kde.dolphin.desktop")
        options.set_capability("environ", {
            "LC_ALL": "en_US.UTF-8",
        })

        self.driver = webdriver.Remote(
            command_executor='http://127.0.0.1:4723',
            options=options)
        self.driver.implicitly_wait = 10
        filename = "{}/testDir/test1.txt".format(os.environ["HOME"])
        os.makedirs(os.path.dirname(filename), exist_ok=True)
        with open(filename, "w") as f:
            f.write("Test File 1")
        filename = "{}/testDir/test2.txt".format(os.environ["HOME"])
        with open(filename, "w") as f:
            f.write("Test File 2")

    def tearDown(self):
        if not self._outcome.result.wasSuccessful():
            self.driver.get_screenshot_as_file("failed_test_shot_{}.png".format(self.id()))

    @classmethod
    def tearDownClass(self):
        os.remove("{}/testDir/test1.txt".format(os.environ["HOME"]))
        os.remove("{}/testDir/test2.txt".format(os.environ["HOME"]))
        os.rmdir("{}/testDir".format(os.environ["HOME"]))
        self.driver.quit()

    def assertResult(self, actual, expected):
        wait = WebDriverWait(self.driver, 20)
        wait.until(lambda x: self.getresults() == expected)
        self.assertEqual(self.getresults(), expected)

    def test_1_location(self):
        editButton = self.driver.find_element(by=AppiumBy.ACCESSIBILITY_ID, value="KUrlNavigatorToggleButton")
        editButton.click()

        # clear contents
        self.driver.find_element(by=AppiumBy.ACCESSIBILITY_ID, value="DolphinUrlNavigator.KUrlComboBox.KLineEdit.QLineEditIconButton").click()

        locationBar = self.driver.find_element(by=AppiumBy.ACCESSIBILITY_ID, value="DolphinUrlNavigator.KUrlComboBox.KLineEdit")
        locationBar.send_keys("{}/testDir".format(os.environ["HOME"]))
        editButton.click()

        elements = self.driver.find_elements(by=AppiumBy.XPATH, value="//table_cell")
        self.assertEqual(len(elements), 2)
        self.assertEqual(elements[0].text, "test1.txt")
        self.assertEqual(elements[1].text, "test2.txt")

    def test_2_filter_bar(self):
        ActionChains(self.driver).send_keys(Keys.DIVIDE).perform()
        
        searchField = self.driver.find_element(by=AppiumBy.ACCESSIBILITY_ID, value="Filter.QScrollArea.qt_scrollarea_viewport.QWidget.FilterLineEdit")
        searchField.send_keys("test2.txt")
        self.assertEqual(searchField.text, "test2.txt")
        elements = self.driver.find_elements(by=AppiumBy.XPATH, value="//table_cell")
        self.assertEqual(len(elements), 1)
        self.assertEqual(elements[0].text, "test2.txt")

        # should see both files now
        buttons = self.driver.find_elements(by=AppiumBy.ACCESSIBILITY_ID, value="Filter.QScrollArea.qt_scrollarea_viewport.QWidget.QToolButton")
        self.assertEqual(len(buttons), 2)
        # close buttion is the second QToolButton
        close_button=buttons[1]
        self.assertIsNotNone(close_button)
        close_button.click()
        
        elements = self.driver.find_elements(by=AppiumBy.XPATH, value="//table_cell")
        self.assertEqual(len(elements), 2)
        self.assertEqual(elements[0].text, "test1.txt")
        self.assertEqual(elements[1].text, "test2.txt")
        

"""
    def test_search_bar(self):
        ActionChains(self.driver).key_down(Keys.CONTROL).send_keys("f").key_up(Keys.CONTROL).perform()
        
        #self.driver.pause(0.1)
        searchField = self.driver.find_element(by=AppiumBy.ACCESSIBILITY_ID, value="DolphinSearchBox.QScrollArea.qt_scrollarea_viewport.QWidget.searchField")
        searchField.send_keys("test1.txt")
        self.assertEqual(searchField.text, "test1.txt")
        
        
        # TODO the search does not work, filenamesearch:/ does not return any result
        time.sleep(1)
        elements = self.driver.find_elements(by=AppiumBy.XPATH, value="//table_cell")
        self.assertEqual(len(elements), 1)
        self.assertEqual(elements[0].text, "test1.txt")
        
        # click on leave search
        self.driver.find_element(by=AppiumBy.ACCESSIBILITY_ID, value="DolphinSearchBox.QScrollArea.qt_scrollarea_viewport.QWidget.QToolButton").click()
        # should see both files now
        elements = self.driver.find_elements(by=AppiumBy.XPATH, value="//table_cell")
        self.assertEqual(len(elements), 2)
        self.assertEqual(elements[0].text, "test2.txt")
        self.assertEqual(elements[1].text, "test1.txt")
"""

if __name__ == '__main__':
    suite = unittest.TestLoader().loadTestsFromTestCase(DolphinTests)
    unittest.TextTestRunner(verbosity=2).run(suite)
