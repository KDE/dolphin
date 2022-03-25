/*
 * SPDX-FileCopyrightText: 2008 Konstantin Heil <konst.heil@stud.uni-heidelberg.de>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "tooltipmanager.h"

#include "dolphinfilemetadatawidget.h"

#include <KIO/JobUiDelegate>
#include <KIO/PreviewJob>
#include <KConfigGroup>
#include <KJobWidgets>
#include <KSharedConfig>
#include <KToolTipWidget>
#include <KIconLoader>

#include <QApplication>
#include <QDesktopWidget>
#include <QIcon>
#include <QLayout>
#include <QStyle>
#include <QTimer>
#include <QWindow>

class IconLoaderSingleton {
public:
    IconLoaderSingleton() = default;

    KIconLoader self;
};

Q_GLOBAL_STATIC(IconLoaderSingleton, iconLoader)

ToolTipManager::ToolTipManager(QWidget* parent) :
    QObject(parent),
    m_showToolTipTimer(nullptr),
    m_contentRetrievalTimer(nullptr),
    m_transientParent(nullptr),
    m_fileMetaDataWidget(nullptr),
    m_toolTipRequested(false),
    m_metaDataRequested(false),
    m_appliedWaitCursor(false),
    m_margin(4),
    m_item(),
    m_itemRect()
{
    if (parent) {
        m_margin = qMax(m_margin, parent->style()->pixelMetric(QStyle::PM_ToolTipLabelFrameWidth));
    }

    m_showToolTipTimer = new QTimer(this);
    m_showToolTipTimer->setSingleShot(true);
    m_showToolTipTimer->setInterval(500);
    connect(m_showToolTipTimer, &QTimer::timeout, this, QOverload<>::of(&ToolTipManager::showToolTip));

    m_contentRetrievalTimer = new QTimer(this);
    m_contentRetrievalTimer->setSingleShot(true);
    m_contentRetrievalTimer->setInterval(200);
    connect(m_contentRetrievalTimer, &QTimer::timeout, this, &ToolTipManager::startContentRetrieval);

    Q_ASSERT(m_contentRetrievalTimer->interval() < m_showToolTipTimer->interval());

    // Only start the retrieving of the content, when the mouse has been over this
    // item for 200 milliseconds. This prevents a lot of useless preview jobs and
    // meta data retrieval, when passing rapidly over a lot of items.
    m_fileMetaDataWidget = new DolphinFileMetaDataWidget(parent);
    connect(m_fileMetaDataWidget, &DolphinFileMetaDataWidget::metaDataRequestFinished, this, &ToolTipManager::slotMetaDataRequestFinished);
    connect(m_fileMetaDataWidget, &DolphinFileMetaDataWidget::urlActivated, this, &ToolTipManager::urlActivated);
}

ToolTipManager::~ToolTipManager()
{
}

void ToolTipManager::showToolTip(const KFileItem& item, const QRectF& itemRect, QWindow *transientParent)
{
    hideToolTip(HideBehavior::Instantly);

    m_itemRect = itemRect.toRect();

    m_itemRect.adjust(-m_margin, -m_margin, m_margin, m_margin);
    m_item = item;

    m_transientParent = transientParent;

    m_contentRetrievalTimer->start();
    m_showToolTipTimer->start();
    m_toolTipRequested = true;
    Q_ASSERT(!m_metaDataRequested);
}

void ToolTipManager::hideToolTip(const HideBehavior behavior)
{
    if (m_appliedWaitCursor) {
        QApplication::restoreOverrideCursor();
        m_appliedWaitCursor = false;
    }

    m_toolTipRequested = false;
    m_metaDataRequested = false;
    m_showToolTipTimer->stop();
    m_contentRetrievalTimer->stop();
    if (m_tooltipWidget) {
        switch (behavior) {
        case HideBehavior::Instantly:
            m_tooltipWidget->hide();
            break;
        case HideBehavior::Later:
            m_tooltipWidget->hideLater();
            break;
        }
    }
}

void ToolTipManager::startContentRetrieval()
{
    if (!m_toolTipRequested) {
        return;
    }

    m_fileMetaDataWidget->setName(m_item.text());

    // Request the retrieval of meta-data. The slot
    // slotMetaDataRequestFinished() is invoked after the
    // meta-data have been received.
    m_metaDataRequested = true;
    m_fileMetaDataWidget->setItems(KFileItemList() << m_item);
    m_fileMetaDataWidget->adjustSize();

    // Request a preview of the item
    m_fileMetaDataWidget->setPreview(QPixmap());

    const KConfigGroup globalConfig(KSharedConfig::openConfig(), "PreviewSettings");
    const QStringList plugins = globalConfig.readEntry("Plugins", KIO::PreviewJob::defaultPlugins());
    KIO::PreviewJob* job = new KIO::PreviewJob(KFileItemList() << m_item,
                                               QSize(256, 256),
                                               &plugins);
    job->setIgnoreMaximumSize(m_item.isLocalFile() && !m_item.isSlow());
    if (job->uiDelegate()) {
        KJobWidgets::setWindow(job, qApp->activeWindow());
    }

    connect(job, &KIO::PreviewJob::gotPreview,
            this, &ToolTipManager::setPreviewPix);
    connect(job, &KIO::PreviewJob::failed,
            this, &ToolTipManager::previewFailed);
}


void ToolTipManager::setPreviewPix(const KFileItem& item,
                                   const QPixmap& pixmap)
{
    if (!m_toolTipRequested || (m_item.url() != item.url())) {
        // No tooltip is requested anymore or an old preview has been received
        return;
    }

    if (pixmap.isNull()) {
        previewFailed();
    } else {
        m_fileMetaDataWidget->setPreview(pixmap);
        if (!m_showToolTipTimer->isActive()) {
            showToolTip();
        }
    }
}

void ToolTipManager::previewFailed()
{
    if (!m_toolTipRequested) {
        return;
    }
    QPalette pal;
    for (auto state : { QPalette::Active, QPalette::Inactive, QPalette::Disabled }) {
        pal.setBrush(state, QPalette::WindowText, pal.toolTipText());
        pal.setBrush(state, QPalette::Window, pal.toolTipBase());
    }
    iconLoader->self.setCustomPalette(pal);
    const QPixmap pixmap = KDE::icon(m_item.iconName(), &iconLoader->self).pixmap(128, 128);
    m_fileMetaDataWidget->setPreview(pixmap);
    if (!m_showToolTipTimer->isActive()) {
        showToolTip();
    }
}

void ToolTipManager::slotMetaDataRequestFinished()
{
    if (!m_toolTipRequested) {
        return;
    }

    m_metaDataRequested = false;

    if (!m_showToolTipTimer->isActive()) {
        showToolTip();
    }
}

void ToolTipManager::showToolTip()
{
    Q_ASSERT(m_toolTipRequested);
    if (m_appliedWaitCursor) {
        QApplication::restoreOverrideCursor();
        m_appliedWaitCursor = false;
    }

    if (m_fileMetaDataWidget->preview().isNull() || m_metaDataRequested) {
        Q_ASSERT(!m_appliedWaitCursor);
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        m_appliedWaitCursor = true;
        return;
    }

    // Adjust the size to get a proper sizeHint()
    m_fileMetaDataWidget->adjustSize();
    if (!m_tooltipWidget) {
        m_tooltipWidget.reset(new KToolTipWidget());
    }
    m_tooltipWidget->showBelow(m_itemRect, m_fileMetaDataWidget, m_transientParent);
    m_toolTipRequested = false;
}

