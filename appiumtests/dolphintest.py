#!/usr/bin/env python3

# SPDX-License-Identifier: MIT
# SPDX-FileCopyrightText: 2021-2022 Harald Sitter <sitter@kde.org>
# SPDX-FileCopyrightText: 2023 Marco Martin <mart@kde.org>

import unittest
from appium import webdriver
from appium.webdriver.common.appiumby import AppiumBy
from selenium.webdriver.support.ui import WebDriverWait
import time
import os
import sys

class DolphinTests(unittest.TestCase):
    @classmethod
    def setUpClass(self):
        desired_caps = {}
        desired_caps["app"] = "org.kde.dolphin.desktop"
        desired_caps["timeouts"] = {'implicit': 10000}
        self.driver = webdriver.Remote(
            command_executor='http://127.0.0.1:4723',
            desired_capabilities=desired_caps)
        self.driver.implicitly_wait = 10
        filename = "{}/testDir/test1.txt".format(os.environ["HOME"])
        os.makedirs(os.path.dirname(filename), exist_ok=True)
        with open(filename, "w") as f:
            f.write("Test File 1")
        filename = "{}/testDir/test2.txt".format(os.environ["HOME"])
        with open(filename, "w") as f:
            f.write("Test File 2")

    #def setUp(self):
        #self.driver.find_element(by=AppiumBy.NAME, value="Today").click()
        #self.assertEqual(self.compareMonthLabel(date.today()), True)

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
        self.assertEqual(elements[0].text, "test2.txt")
        self.assertEqual(elements[1].text, "test1.txt")

    def test_2_filter_bar(self):
        self.driver.find_element(by=AppiumBy.NAME, value="Filter").click()
        searchField = self.driver.find_element(by=AppiumBy.ACCESSIBILITY_ID, value="FilterBar.QLineEdit")
        searchField.send_keys("test2.txt")
        self.assertEqual(searchField.text, "test2.txt")
        elements = self.driver.find_elements(by=AppiumBy.XPATH, value="//table_cell")
        self.assertEqual(len(elements), 1)
        self.assertEqual(elements[0].text, "test2.txt")

        # should see both files now
        self.driver.find_element(by=AppiumBy.NAME, value="Filter").click()
        elements = self.driver.find_elements(by=AppiumBy.XPATH, value="//table_cell")
        self.assertEqual(len(elements), 2)
        self.assertEqual(elements[0].text, "test2.txt")
        self.assertEqual(elements[1].text, "test1.txt")

    def test_search_bar(self):
        self.driver.find_element(by=AppiumBy.NAME, value="Search").click()
        searchField = self.driver.find_element(by=AppiumBy.ACCESSIBILITY_ID, value="searchField")
        searchField.send_keys("test1.txt")
        self.assertEqual(searchField.text, "test1.txt")
        time.sleep(1)
        #FIXME: this still crashes
        #wait = WebDriverWait(self.driver, 50)
        #wait.until(lambda x: len(self.driver.find_elements(by=AppiumBy.XPATH, value="//table_cell")) == 1)
        elements = self.driver.find_elements(by=AppiumBy.XPATH, value="//table_cell")
        self.assertEqual(len(elements), 1)
        self.assertEqual(elements[0].text, "test1.txt")

        # should see both files now
        self.driver.find_element(by=AppiumBy.NAME, value="Search").click()
        elements = self.driver.find_elements(by=AppiumBy.XPATH, value="//table_cell")
        self.assertEqual(len(elements), 2)
        self.assertEqual(elements[0].text, "test2.txt")
        self.assertEqual(elements[1].text, "test1.txt")


if __name__ == '__main__':
    suite = unittest.TestLoader().loadTestsFromTestCase(DolphinTests)
    unittest.TextTestRunner(verbosity=2).run(suite)
