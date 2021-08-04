/*
 * SPDX-FileCopyrightText: 2006-2010 Peter Penz <peter.penz19@gmail.com>
 * SPDX-FileCopyrightText: 2006 Gregor Kališnik <gregor@podnapisi.net>
 * SPDX-FileCopyrightText: 2012 Stuart Citrin <ctrn3e8@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "filterbar.h"

#include <KLocalizedString>

#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QToolButton>
#include <QStylePainter>
#include <QStyleOption>

FilterBar::FilterBar(QWidget* parent) :
    QWidget(parent)
{
    // Create button to lock text when changing folders
    m_lockButton = new QToolButton(this);
    m_lockButton->setAutoRaise(true);
    m_lockButton->setCheckable(true);
    m_lockButton->setIcon(QIcon::fromTheme(QStringLiteral("object-unlocked")));
    m_lockButton->setToolTip(i18nc("@info:tooltip", "Keep Filter When Changing Folders"));
    connect(m_lockButton, &QToolButton::toggled, this, &FilterBar::slotToggleLockButton);

    setProperty("_breeze_borders_sides", QVariant::fromValue(Qt::LeftEdge | Qt::TopEdge | Qt::RightEdge));
    setContentsMargins(0, 4, 0, 4);

    // Create filter editor
    m_filterInput = new QLineEdit(this);
    m_filterInput->setLayoutDirection(Qt::LeftToRight);
    m_filterInput->setClearButtonEnabled(true);
    m_filterInput->setPlaceholderText(i18n("Filter..."));
    connect(m_filterInput, &QLineEdit::textChanged,
            this, &FilterBar::filterChanged);
    setFocusProxy(m_filterInput);

    // Create close button
    QToolButton *closeButton = new QToolButton(this);
    closeButton->setAutoRaise(true);
    closeButton->setIcon(QIcon::fromTheme(QStringLiteral("dialog-close")));
    closeButton->setToolTip(i18nc("@info:tooltip", "Hide Filter Bar"));
    connect(closeButton, &QToolButton::clicked, this, &FilterBar::closeRequest);

    // Apply layout
    QHBoxLayout* hLayout = new QHBoxLayout(this);
    hLayout->setContentsMargins(0, 0, 0, 0);
    hLayout->addWidget(m_lockButton);
    hLayout->addWidget(m_filterInput);
    hLayout->addWidget(closeButton);
}

FilterBar::~FilterBar()
{
}

void FilterBar::closeFilterBar()
{
    hide();
    clear();
    if (m_lockButton) {
        m_lockButton->setChecked(false);
    }
}

void FilterBar::selectAll()
{
    m_filterInput->selectAll();
}

void FilterBar::clear()
{
    m_filterInput->clear();
}

void FilterBar::slotUrlChanged()
{
    if (!m_lockButton || !(m_lockButton->isChecked())) {
        clear();
    }
}

void FilterBar::slotToggleLockButton(bool checked)
{
    if (checked) {
        m_lockButton->setIcon(QIcon::fromTheme(QStringLiteral("object-locked")));
    } else {
        m_lockButton->setIcon(QIcon::fromTheme(QStringLiteral("object-unlocked")));
        clear();
    }
}

void FilterBar::showEvent(QShowEvent* event)
{
    if (!event->spontaneous()) {
        m_filterInput->setFocus();
    }

    Q_EMIT shown();
}

void FilterBar::keyReleaseEvent(QKeyEvent* event)
{
    QWidget::keyReleaseEvent(event);

    switch (event->key()) {
    case Qt::Key_Escape:
        if (m_filterInput->text().isEmpty()) {
            Q_EMIT closeRequest();
        } else {
            m_filterInput->clear();
        }
        break;

    case Qt::Key_Enter:
    case Qt::Key_Return:
        Q_EMIT focusViewRequest();
        break;

    default:
        break;
    }
}

void FilterBar::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)

    QStylePainter painter(this);

    QStyleOptionFrame option;
    option.initFrom(this);
    option.rect = rect();
    option.state = QStyle::State_Sunken;
    option.palette.setBrush(QPalette::Base, option.palette.brush(QPalette::Window));

    painter.style()->drawPrimitive(QStyle::PE_Frame, &option, &painter, this);
}

void FilterBar::hideEvent(QHideEvent* event)
{
    Q_UNUSED(event)

    Q_EMIT hidden();
}
