#!/usr/bin/env python3

# SPDX-License-Identifier: MIT
# SPDX-FileCopyrightText: 2023 Marco Martin <mart@kde.org>
# SPDX-FileCopyrightText: 2026 Sebastian Englbrecht

from appium.webdriver.common.appiumby import AppiumBy
from selenium.webdriver.common.keys import Keys

from dolphinappium import DolphinTestCase, byObjectName, main


class SearchBarTests(DolphinTestCase):
    """
    Without a Baloo index for the test folder, Dolphin searches with the filenamesearch KIO worker.
    """

    testRoot = "searchBarTest"
    testTree = ("test1.txt", "test2.txt", "sub/test1-copy.txt")

    def test_0_open_folder(self):
        self.openLocation(self.root)

        self.assertEventually(self.itemNames, ["sub", "test1.txt", "test2.txt"])

    def test_1_search_finds_matching_files_in_subfolders(self):
        self.pressShortcut(Keys.CONTROL, "f")
        searchField = self.driver.find_element(by=AppiumBy.XPATH, value=byObjectName("searchTermEditor"))

        self.typeText("test1")

        self.assertEventually(lambda: searchField.text, "test1")
        self.assertEventually(lambda: sorted(self.itemNames() or []), ["test1-copy.txt", "test1.txt"])

    def test_2_quitting_the_search_shows_the_folder_again(self):
        self.driver.find_element(by=AppiumBy.XPATH, value=byObjectName("searchCloseButton")).click()

        self.assertEventually(self.itemNames, ["sub", "test1.txt", "test2.txt"])


if __name__ == "__main__":
    main()
