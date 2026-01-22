/*
 * SPDX-FileCopyrightText: 2006 Peter Penz (peter.penz@gmx.at) and Patrice Tremblay
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "statusbarspaceinfo.h"

#include "config-dolphin.h"
#include "diskspaceusagemenu.h"
#include "spaceinfoobserver.h"

#include <KCapacityBar>
#include <KIO/Global>
#include <KLocalizedString>

#include <QHBoxLayout>
#include <QToolButton>

StatusBarSpaceInfo::StatusBarSpaceInfo(QWidget *parent)
    : QWidget(parent)
    , m_observer(nullptr)
    , m_hasSpaceInfo{false}
    , m_shown{false}
{
    hide(); // Only become visible when we have space info to show. @see StatusBarSpaceInfo::setShown().

    m_capacityBar = new KCapacityBar(KCapacityBar::DrawTextInline, this);
    m_textInfoButton = new QToolButton(this);
    m_textInfoButton->setAutoRaise(true);
    m_textInfoButton->setPopupMode(QToolButton::InstantPopup);
    auto menu = new DiskSpaceUsageMenu{this};
    connect(menu, &DiskSpaceUsageMenu::showMessage, this, &StatusBarSpaceInfo::showMessage);
    connect(menu, &DiskSpaceUsageMenu::showInstallationProgress, this, &StatusBarSpaceInfo::showInstallationProgress);
    m_textInfoButton->setMenu(menu);

    auto layout = new QHBoxLayout(this);
    // We reduce the outside margin of the flat button so it visually has the same margin as the status bar text label on the other end of the bar.
    layout->setContentsMargins(2, -1, 0, -1); // "-1" makes it so the fixed height won't be ignored.
    layout->addWidget(m_capacityBar);
    layout->addWidget(m_textInfoButton);
}

StatusBarSpaceInfo::~StatusBarSpaceInfo() = default;

void StatusBarSpaceInfo::setShown(bool shown)
{
    m_shown = shown;
    if (!m_shown) {
        hide();
        return;
    }

    // We only show() this widget in slotValueChanged() when it m_hasSpaceInfo.
    if (m_observer.isNull()) {
        m_observer.reset(new SpaceInfoObserver(m_url, this));
        connect(m_observer.data(), &SpaceInfoObserver::valuesChanged, this, &StatusBarSpaceInfo::slotValuesChanged);
    }

    if (m_hasSpaceInfo) {
        slotValuesChanged();
    }
}

void StatusBarSpaceInfo::setUrl(const QUrl &url)
{
    if (m_url != url) {
        m_url = url;
        static_cast<DiskSpaceUsageMenu *>(m_textInfoButton->menu())->setUrl(url);
        m_hasSpaceInfo = false;
        if (m_observer) {
            m_observer.reset(new SpaceInfoObserver(m_url, this));
            connect(m_observer.data(), &SpaceInfoObserver::valuesChanged, this, &StatusBarSpaceInfo::slotValuesChanged);
        }
    }
}

QUrl StatusBarSpaceInfo::url() const
{
    return m_url;
}

void StatusBarSpaceInfo::update()
{
    if (m_observer) {
        m_observer->update();
    }
}

void StatusBarSpaceInfo::showEvent(QShowEvent *event)
{
    if (m_shown && m_observer.isNull()) {
        m_observer.reset(new SpaceInfoObserver(m_url, this));
        connect(m_observer.data(), &SpaceInfoObserver::valuesChanged, this, &StatusBarSpaceInfo::slotValuesChanged);
    }
    QWidget::showEvent(event);
}

void StatusBarSpaceInfo::hideEvent(QHideEvent *event)
{
    if (m_hasSpaceInfo) {
        m_observer.reset();
        m_hasSpaceInfo = false;
    }
    QWidget::hideEvent(event);
}

QSize StatusBarSpaceInfo::minimumSizeHint() const
{
    return QSize();
}

void StatusBarSpaceInfo::slotValuesChanged()
{
    Q_ASSERT(m_observer);
    const quint64 size = m_observer->size();

    if (!m_shown || size == 0) {
        hide();
        return;
    }

    m_hasSpaceInfo = true;

    const quint64 available = m_observer->available();
    const quint64 used = size - available;
    const int percentUsed = qRound(100.0 * qreal(used) / qreal(size));

    m_textInfoButton->setText(i18nc("@info:status Free disk space", "%1 free", KIO::convertSize(available)));
    setToolTip(i18nc("tooltip:status Free disk space", "%1 free out of %2 (%3% used)", KIO::convertSize(available), KIO::convertSize(size), percentUsed));
    m_textInfoButton->setToolTip(i18nc("@info:tooltip for the free disk space button",
                                       "%1 free out of %2 (%3% used)\nPress to manage disk space usage.",
                                       KIO::convertSize(available),
                                       KIO::convertSize(size),
                                       percentUsed));
    setUpdatesEnabled(false);
    m_capacityBar->setValue(percentUsed);
    setUpdatesEnabled(true);

    if (!isVisible()) {
        show();
    } else {
        update();
    }
}

#include "moc_statusbarspaceinfo.cpp"
