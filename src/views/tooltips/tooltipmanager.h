/*
 * SPDX-FileCopyrightText: 2008 Konstantin Heil <konst.heil@stud.uni-heidelberg.de>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef TOOLTIPMANAGER_H
#define TOOLTIPMANAGER_H

#include <KFileItem>

#include <QObject>
#include <QRect>

class DolphinFileMetaDataWidget;
class KToolTipWidget;
class QTimer;
class QWindow;

/**
 * @brief Manages the tooltips for an item view.
 *
 * When hovering an item, a tooltip is shown after
 * a short timeout. The tooltip is hidden again when the
 * viewport is hovered or the item view has been left.
 */
class ToolTipManager : public QObject
{
    Q_OBJECT

public:
    enum class HideBehavior { Instantly, Later };

    explicit ToolTipManager(QWidget *parent);
    ~ToolTipManager() override;

    /**
     * Triggers the showing of the tooltip for the item \p item
     * where the item has the maximum boundaries of \p itemRect.
     * The tooltip manager takes care that the tooltip is shown
     * slightly delayed and with a proper \p transientParent.
     */
    void showToolTip(const KFileItem &item, const QRectF &itemRect, QWindow *transientParent);

    /**
     * Hides the currently shown tooltip.
     */
    void hideToolTip(const HideBehavior behavior = HideBehavior::Later);

Q_SIGNALS:
    /**
     * Is emitted when the user clicks a tag or a link
     * in the metadata widget.
     */
    void urlActivated(const QUrl &url);

private Q_SLOTS:
    void startContentRetrieval();
    void setPreviewPix(const KFileItem &item, const QPixmap &pix);
    void previewFailed();
    void slotMetaDataRequestFinished();
    void showToolTip();

private:
    /// Timeout from requesting a tooltip until the tooltip
    /// should be shown
    QTimer *m_showToolTipTimer;

    /// Timeout from requesting a tooltip until the retrieving of
    /// the tooltip content like preview and meta data gets started.
    QTimer *m_contentRetrievalTimer;

    /// Transient parent of the tooltip, mandatory on Wayland.
    QWindow *m_transientParent;

    QScopedPointer<KToolTipWidget> m_tooltipWidget;
    DolphinFileMetaDataWidget *m_fileMetaDataWidget = nullptr;

    /// Whether ownership of the metadata widget was transferred
    /// over to the KToolTipWidget (i.e. we should not delete it
    /// anymore)
    bool m_fileMetaDatWidgetOwnershipTransferred = false;

    bool m_toolTipRequested;
    bool m_metaDataRequested;
    bool m_appliedWaitCursor;
    int m_margin;
    KFileItem m_item;
    QRect m_itemRect;
};

#endif
