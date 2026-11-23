#!/usr/bin/env python3

# SPDX-License-Identifier: MIT
# SPDX-FileCopyrightText: 2021-2022 Harald Sitter <sitter@kde.org>
# SPDX-FileCopyrightText: 2023 Marco Martin <mart@kde.org>
# SPDX-FileCopyrightText: 2026 Sebastian Englbrecht

from appium.webdriver.common.appiumby import AppiumBy

from dolphinappium import DolphinTestCase, byObjectName, main


class FilterBarTests(DolphinTestCase):
    testRoot = "filterBarTest"
    testTree = ("test1.txt", "test2.txt")

    def test_0_open_folder(self):
        self.openLocation(self.root)

        self.assertEventually(self.itemNames, ["test1.txt", "test2.txt"])

    def test_1_filter_hides_the_items_that_do_not_match(self):
        # "/" opens the filter bar and moves the focus into it
        self.typeText("/")
        filterInput = self.driver.find_element(by=AppiumBy.XPATH, value=byObjectName("filterInput"))

        # The full name matches in each filter mode
        self.typeText("test2.txt")

        self.assertEventually(lambda: filterInput.text, "test2.txt")
        self.assertEventually(self.itemNames, ["test2.txt"])

    def test_2_closing_the_filter_bar_shows_all_items(self):
        self.driver.find_element(by=AppiumBy.XPATH, value=byObjectName("filterCloseButton")).click()

        self.assertEventually(self.itemNames, ["test1.txt", "test2.txt"])


if __name__ == "__main__":
    main()
