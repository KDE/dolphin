# SPDX-FileCopyrightText: 2014 Christophe Giboudeaux <cgiboudeaux@gmx.com>
# SPDX-FileCopyrightText: 2015 Albert Astals Cid <aacid@kde.org>
# SPDX-FileCopyrightText: 2015 Montel Laurent <montel@kde.org>
# SPDX-FileCopyrightText: 2021 Alex Richardson <Alexander.Richardson@cl.cam.ac.uk>
# SPDX-FileCopyrightText: 2023 Dāvis Mosāns <davispuh@gmail.com>
#
# SPDX-License-Identifier: GPL-2.0-or-later

if (KF6DocTools_FOUND)
    kdoctools_create_handbook(index.docbook INSTALL_DESTINATION ${KDE_INSTALL_DOCBUNDLEDIR}/en SUBDIR dolphin)
endif()
