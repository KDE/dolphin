#!/usr/bin/env python3

# SPDX-License-Identifier: MIT
# SPDX-FileCopyrightText: 2021-2022 Harald Sitter <sitter@kde.org>
# SPDX-FileCopyrightText: 2023 Marco Martin <mart@kde.org>
# SPDX-FileCopyrightText: 2026 Sebastian Englbrecht

from appium.webdriver.common.appiumby import AppiumBy
from selenium.common.exceptions import StaleElementReferenceException
from selenium.webdriver.common.action_chains import ActionChains

from dolphinappium import URL_NAVIGATOR_CLEAR, DolphinTestCase, main


class LocationBarTests(DolphinTestCase):
    testRoot = "locationBarTest"
    testTree = ("first/test1.txt", "first/test2.txt", "second/other.txt")

    def test_1_toggling_back_opens_the_typed_location(self):
        self.openLocation(self.root + "/first")

        self.assertEventually(self.itemNames, ["test1.txt", "test2.txt"])

    def test_2_click_on_the_view_leaves_the_editable_mode(self):
        self.openLocationInEditor(self.root + "/second")
        clearButton = self.driver.find_element(by=AppiumBy.ACCESSIBILITY_ID, value=URL_NAVIGATOR_CLEAR)

        background = self.driver.find_element(by=AppiumBy.ACCESSIBILITY_ID, value="DolphinViewContainer.DolphinView.KItemListContainer")
        rect = background.rect
        ActionChains(self.driver).move_to_element_with_offset(background, rect["width"] - 50, rect["height"] - 50).click().perform()

        self.assertEventually(lambda: self.isShown(clearButton), False)
        self.assertEventually(self.itemNames, ["test1.txt", "test2.txt"])

    @staticmethod
    def isShown(element):
        try:
            return element.is_displayed()
        except StaleElementReferenceException:
            return False


if __name__ == "__main__":
    main()
