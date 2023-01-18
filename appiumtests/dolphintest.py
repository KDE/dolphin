#!/usr/bin/env python3

# SPDX-License-Identifier: MIT
# SPDX-FileCopyrightText: 2021-2022 Harald Sitter <sitter@kde.org>
# SPDX-FileCopyrightText: 2023 Marco Martin <mart@kde.org>

import unittest
from appium import webdriver
from appium.webdriver.common.appiumby import AppiumBy
from selenium.webdriver.support.ui import WebDriverWait
from dateutil.relativedelta import relativedelta
import time


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

    #def setUp(self):
        #self.driver.find_element(by=AppiumBy.NAME, value="Today").click()
        #self.assertEqual(self.compareMonthLabel(date.today()), True)

    def tearDown(self):
        if not self._outcome.result.wasSuccessful():
            self.driver.get_screenshot_as_file("failed_test_shot_{}.png".format(self.id()))

    @classmethod
    def tearDownClass(self):
        self.driver.quit()

    def assertResult(self, actual, expected):
        wait = WebDriverWait(self.driver, 20)
        wait.until(lambda x: self.getresults() == expected)
        self.assertEqual(self.getresults(), expected)

    def test_search_bar(self):
        self.driver.find_element(by=AppiumBy.NAME, value="Search").click()
        searchField = self.driver.find_element(by=AppiumBy.ACCESSIBILITY_ID, value="searchField")
        print("AAAAA {}".format(searchField))
        searchField.send_keys("bah")
        time.sleep(2)

        #wait = WebDriverWait(self.driver, 50)
        #wait.until(lambda x: self.compareMonthLabel(nextMonthDate))
        #self.assertEqual(self.compareMonthLabel(nextMonthDate), True)

if __name__ == '__main__':
    suite = unittest.TestLoader().loadTestsFromTestCase(DolphinTests)
    unittest.TextTestRunner(verbosity=2).run(suite)
