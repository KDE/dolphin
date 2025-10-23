/*
 * SPDX-FileCopyrightText: 2006 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef PIXMAPVIEWER_H
#define PIXMAPVIEWER_H

#include <QPixmap>
#include <QQueue>
#include <QTimeLine>
#include <QWidget>

class QPaintEvent;
class QMovie;

/**
 * @brief Widget which shows a pixmap centered inside the boundaries.
 *
 * When the pixmap is changed, a smooth transition is done from the old pixmap
 * to the new pixmap.
 */
class PixmapViewer : public QWidget
{
    Q_OBJECT

public:
    explicit PixmapViewer(QWidget *parent);

    ~PixmapViewer() override;
    void setPixmap(const QPixmap &pixmap);
    QPixmap pixmap() const;

    /**
     * Sets the size hint to \a size and triggers a relayout
     * of the parent widget. Per default no size hint is given.
     */
    void setSizeHint(const QSize &size);
    QSize sizeHint() const override;

    void setAnimatedImageFileName(const QString &fileName);
    QString animatedImageFileName() const;

    void stopAnimatedImage();

    /**
     * Checks if \a mimeType has a format supported by QMovie.
     */
    static bool isAnimatedMimeType(const QString &mimeType);

protected:
    void paintEvent(QPaintEvent *event) override;

private Q_SLOTS:
    void updateAnimatedImageFrame();

private:
    QPixmap m_pixmap;
    QMovie *m_animatedImage;
    QSize m_sizeHint;
    bool m_hasAnimatedImage;
};

inline QPixmap PixmapViewer::pixmap() const
{
    return m_pixmap;
}

#endif
