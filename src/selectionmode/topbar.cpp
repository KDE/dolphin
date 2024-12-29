/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 2022 Felix Ernst <felixernst@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "topbar.h"

#include "backgroundcolorhelper.h"

#include <KColorScheme>
#include <KContextualHelpButton>
#include <KLocalizedString>

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QStyle>

using namespace SelectionMode;

TopBar::TopBar(QWidget *parent)
    : AnimatedHeightWidget{parent}
{
    QWidget *contentsContainer = prepareContentsContainer();

    BackgroundColorHelper::instance()->controlBackgroundColor(this);

    m_fullLabelString = i18nc("@info label above the view explaining the state", "Selection Mode: Click on files or folders to select or deselect them.");
    m_shortLabelString = i18nc("@info label above the view explaining the state", "Selection Mode");
    m_label = new QLabel(contentsContainer);
    m_label->setMinimumWidth(0);
    m_label->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard | Qt::LinksAccessibleByKeyboard); // for keyboard accessibility
    BackgroundColorHelper::instance()->controlBackgroundColor(m_label);

    m_contextualHelpButton = new KContextualHelpButton{
        xi18nc("@info",
               "<title>Selection Mode</title><para>Select files or folders to manage or manipulate them."
               "<list><item>Press on a file or folder to select it.</item><item>Press on an already selected file or folder to deselect it.</item>"
               "<item>Pressing an empty area does <emphasis>not</emphasis> clear the selection.</item>"
               "<item>Selection rectangles (created by dragging from an empty area) invert the selection status of items within.</item>"
               "<item>Moving with <shortcut>arrow keys</shortcut> does <emphasis>not</emphasis> change the selection.</item>"
               "<item>Pressing <shortcut>%1</shortcut>, <shortcut>%2</shortcut>, or <shortcut>%3</shortcut> toggles the selection.</item></list></para>"
               "<para>The available action buttons at the bottom change depending on the current selection.</para>",
               QKeySequence{Qt::Key_Enter}.toString(QKeySequence::NativeText),
               QKeySequence{Qt::Key_Return}.toString(QKeySequence::NativeText),
               QKeySequence{Qt::CTRL | Qt::Key_Space}.toString(QKeySequence::NativeText)),
        nullptr,
        contentsContainer};

    m_closeButton = new QPushButton(QIcon::fromTheme(QStringLiteral("window-close-symbolic")), "", contentsContainer);
    m_closeButton->setText(i18nc("@action:button", "Exit Selection Mode"));
    m_closeButton->setFlat(true);
    connect(m_closeButton, &QAbstractButton::clicked, this, &TopBar::selectionModeLeavingRequested);

    QHBoxLayout *layout = new QHBoxLayout(contentsContainer);
    auto contentsMargins = layout->contentsMargins();
    m_preferredHeight = contentsMargins.top() + m_label->sizeHint().height() + contentsMargins.bottom();
    m_closeButton->setFixedHeight(m_preferredHeight);
    layout->setContentsMargins(0, 0, 0, 0);

    layout->addStretch();
    layout->addWidget(m_label);
    layout->addWidget(m_contextualHelpButton);
    layout->addStretch();
    layout->addWidget(m_closeButton);
}

void TopBar::resizeEvent(QResizeEvent *resizeEvent)
{
    updateLabelString();
    return AnimatedHeightWidget::resizeEvent(resizeEvent);
}

void TopBar::updateLabelString()
{
    QFontMetrics fontMetrics = m_label->fontMetrics();
    if (fontMetrics.horizontalAdvance(m_fullLabelString) + m_contextualHelpButton->sizeHint().width() + m_closeButton->sizeHint().width()
            + style()->pixelMetric(QStyle::PM_LayoutLeftMargin) * 2 + style()->pixelMetric(QStyle::PM_LayoutRightMargin) * 2
        < width()) {
        m_label->setText(m_fullLabelString);
    } else {
        m_label->setText(m_shortLabelString);
    }
}

#include "moc_topbar.cpp"
