/*
 * SPDX-FileCopyrightText: 2006-2010 Peter Penz <peter.penz19@gmail.com>
 * SPDX-FileCopyrightText: 2006 Gregor Kališnik <gregor@podnapisi.net>
 * SPDX-FileCopyrightText: 2012 Stuart Citrin <ctrn3e8@gmail.com>
 * SPDX-FileCopyrightText: 2026 Alessio Bonfiglio <alessio.bonfiglio@mail.polimi.it>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "filterbar.h"

#include <KConfigGroup>
#include <KLocalizedString>
#include <KSharedConfig>

#include <KColorScheme>
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLineEdit>
#include <QPalette>
#include <QToolButton>

FilterBar::FilterBar(QWidget *parent)
    : AnimatedHeightWidget{parent}
{
    QWidget *contentsContainer = prepareContentsContainer();

    // Create button to lock text when changing folders
    m_lockButton = new QToolButton(contentsContainer);
    m_lockButton->setAutoRaise(true);
    m_lockButton->setCheckable(true);
    m_lockButton->setIcon(QIcon::fromTheme(QStringLiteral("object-unlocked")));
    m_lockButton->setToolTip(i18nc("@info:tooltip", "Keep Filter When Changing Folders"));
    connect(m_lockButton, &QToolButton::toggled, this, &FilterBar::slotToggleLockButton);

    // Create filter editor
    m_filterInput = new QLineEdit(contentsContainer);
    m_filterInput->setLayoutDirection(Qt::LeftToRight);
    m_filterInput->setClearButtonEnabled(true);
    m_filterInput->setPlaceholderText(i18n("Filter…"));
    connect(m_filterInput, &QLineEdit::textChanged, this, &FilterBar::filterChanged);
    connect(m_filterInput, &QLineEdit::textChanged, this, &FilterBar::updateInvalidPatternView);
    setFocusProxy(m_filterInput);

    m_invalidPatternAction = new QAction(m_filterInput);
    m_invalidPatternAction->setCheckable(false);
    m_invalidPatternAction->setIcon(QIcon::fromTheme(QStringLiteral("error-symbolic")));
    m_invalidPatternAction->setToolTip(i18n("Invalid expression"));
    m_filterInput->addAction(m_invalidPatternAction, QLineEdit::TrailingPosition);
    m_invalidPatternAction->setVisible(false);

    // Create case sensitive button
    m_caseSensitiveButton = new QToolButton(contentsContainer);
    m_caseSensitiveButton->setAutoRaise(true);
    m_caseSensitiveButton->setCheckable(true);
    m_caseSensitiveButton->setIcon(QIcon::fromTheme(QStringLiteral("format-text-superscript"), QIcon::fromTheme(QStringLiteral("format-text-bold"))));
    m_caseSensitiveButton->setToolTip(i18nc("@info:tooltip", "Match case"));
    connect(m_caseSensitiveButton, &QToolButton::toggled, this, &FilterBar::caseSensitiveChanged);
    connect(m_caseSensitiveButton, &QToolButton::toggled, this, &FilterBar::updateInvalidPatternView);

    // Create filter mode combobox
    m_filterModeComboBox = new QComboBox(contentsContainer);
    m_filterModeComboBox->addItem(i18nc("@item:inlistbox", "Plain Text"), KFileItemModelFilter::FilterMode::PlainText);
    m_filterModeComboBox->addItem(i18nc("@item:inlistbox", "Glob Pattern"), KFileItemModelFilter::FilterMode::Glob);
    m_filterModeComboBox->addItem(i18nc("@item:inlistbox", "Regular Expression"), KFileItemModelFilter::FilterMode::Regex);
    connect(m_filterModeComboBox, &QComboBox::currentIndexChanged, this, [this](int index) {
        Q_EMIT filterModeChanged(m_filterModeComboBox->itemData(index).value<KFileItemModelFilter::FilterMode>());
    });
    connect(m_filterModeComboBox, &QComboBox::currentIndexChanged, this, &FilterBar::updateInvalidPatternView);

    // Create close button
    QToolButton *closeButton = new QToolButton(contentsContainer);
    closeButton->setAutoRaise(true);
    closeButton->setIcon(QIcon::fromTheme(QStringLiteral("dialog-close")));
    closeButton->setToolTip(i18nc("@info:tooltip", "Hide Filter Bar"));
    connect(closeButton, &QToolButton::clicked, this, &FilterBar::closeRequest);

    // Apply layout
    QHBoxLayout *hLayout = new QHBoxLayout(contentsContainer);
    hLayout->setContentsMargins(0, 0, 0, 0);
    hLayout->addWidget(m_lockButton);
    hLayout->addWidget(m_filterInput);
    hLayout->addWidget(m_caseSensitiveButton);
    hLayout->addWidget(m_filterModeComboBox);
    hLayout->addWidget(closeButton);

    setTabOrder({m_lockButton, m_caseSensitiveButton, m_filterModeComboBox, closeButton, m_filterInput});

    KConfigGroup filterBarConfig(KSharedConfig::openStateConfig(), QStringLiteral("FilterBar"));
    bool caseSensitiveEnabled = filterBarConfig.readEntry("caseSensitive", false);
    int filterModeComboBoxIndex = filterBarConfig.readEntry("filterMode", m_filterModeComboBox->findData(KFileItemModelFilter::FilterMode::Glob));
    m_caseSensitiveButton->setChecked(caseSensitiveEnabled);
    m_filterModeComboBox->setCurrentIndex(filterModeComboBoxIndex);
}

FilterBar::~FilterBar()
{
    KConfigGroup filterBarConfig(KSharedConfig::openStateConfig(), QStringLiteral("FilterBar"));
    filterBarConfig.writeEntry("caseSensitive", this->m_caseSensitiveButton->isChecked());
    filterBarConfig.writeEntry("filterMode", this->m_filterModeComboBox->currentIndex());
}

void FilterBar::closeFilterBar()
{
    setVisible(false, WithAnimation);
    clear();
    if (m_lockButton) {
        m_lockButton->setChecked(false);
    }
}

void FilterBar::selectAll()
{
    m_filterInput->selectAll();
}

KFileItemModelFilter::FilterMode FilterBar::filterMode() const
{
    return m_filterModeComboBox->itemData(m_filterModeComboBox->currentIndex()).value<KFileItemModelFilter::FilterMode>();
}

bool FilterBar::isCaseSensitive() const
{
    return m_caseSensitiveButton->isChecked();
}

void FilterBar::clear()
{
    m_filterInput->clear();
}

void FilterBar::clearIfUnlocked()
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

void FilterBar::updateInvalidPatternView()
{
    bool valid = true;

    KFileItemModelFilter::FilterMode current_filter_mode = filterMode();
    if (current_filter_mode != KFileItemModelFilter::FilterMode::PlainText) {
        QRegularExpression regExp = QRegularExpression();
        QString pattern = m_filterInput->text();

        QRegularExpression::PatternOptions options =
            m_caseSensitiveButton->isChecked() ? QRegularExpression::NoPatternOption : QRegularExpression::CaseInsensitiveOption;
        if (current_filter_mode == KFileItemModelFilter::FilterMode::Regex) {
            regExp.setPattern(pattern);
            regExp.setPatternOptions(options);
        } else if (current_filter_mode == KFileItemModelFilter::FilterMode::Glob) {
            regExp.setPattern(QRegularExpression::wildcardToRegularExpression(pattern, QRegularExpression::UnanchoredWildcardConversion));
            regExp.setPatternOptions(options);
        }

        valid = regExp.isValid();
    }

    if (valid) {
        m_filterInput->setPalette(QPalette());
        m_invalidPatternAction->setVisible(false);
    } else {
        auto pal = m_filterInput->palette();
        KColorScheme::adjustBackground(pal, KColorScheme::NegativeBackground);
        m_filterInput->setPalette(pal);
        m_invalidPatternAction->setVisible(true);
    }
}

void FilterBar::showEvent(QShowEvent *event)
{
    if (!event->spontaneous()) {
        m_filterInput->setFocus();
    }
}

void FilterBar::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Escape:
        if (m_filterInput->text().isEmpty()) {
            Q_EMIT closeRequest();
        } else {
            m_filterInput->clear();
        }
        return;

    case Qt::Key_Enter:
    case Qt::Key_Return:
        Q_EMIT focusViewRequest();
        return;

    case Qt::Key_Down:
    case Qt::Key_PageDown:
    case Qt::Key_Up:
    case Qt::Key_PageUp: {
        Q_EMIT focusViewRequest();
        QWidget *focusWidget = QApplication::focusWidget();
        if (focusWidget && focusWidget != this) {
            QApplication::sendEvent(focusWidget, event);
        }
        return;
    }

    default:
        break;
    }

    QWidget::keyPressEvent(event);
}

int FilterBar::preferredHeight() const
{
    return std::max({m_filterInput->sizeHint().height(),
                     m_lockButton->sizeHint().height(),
                     m_caseSensitiveButton->sizeHint().height(),
                     m_filterModeComboBox->sizeHint().height()});
}

#include "moc_filterbar.cpp"
