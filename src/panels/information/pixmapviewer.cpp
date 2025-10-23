/*
 * SPDX-FileCopyrightText: 2006 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "pixmapviewer.h"

#include <KIconLoader>

#include <QImageReader>
#include <QMovie>
#include <QPainter>
#include <QStyle>

PixmapViewer::PixmapViewer(QWidget *parent)
    : QWidget(parent)
    , m_animatedImage(nullptr)
    , m_sizeHint()
    , m_hasAnimatedImage(false)
{
    setMinimumWidth(KIconLoader::SizeEnormous);
    setMinimumHeight(KIconLoader::SizeEnormous);
}

PixmapViewer::~PixmapViewer()
{
}

void PixmapViewer::setPixmap(const QPixmap &pixmap)
{
    if (pixmap.isNull()) {
        return;
    }

    m_pixmap = pixmap;

    // Avoid flicker with static pixmap if an animated image is running
    if (m_animatedImage && m_animatedImage->state() == QMovie::Running) {
        return;
    }

    update();

    if (m_hasAnimatedImage) {
        // If there is no transition animation but an animatedImage
        // and it is not already running, start animating now
        if (m_animatedImage->state() != QMovie::Running) {
            m_animatedImage->start();
        }
    }
}

void PixmapViewer::setSizeHint(const QSize &size)
{
    if (m_animatedImage && size != m_sizeHint) {
        m_animatedImage->stop();
    }

    m_sizeHint = size;
    updateGeometry();
}

QSize PixmapViewer::sizeHint() const
{
    return m_sizeHint;
}

void PixmapViewer::setAnimatedImageFileName(const QString &fileName)
{
    if (!m_animatedImage) {
        m_animatedImage = new QMovie(this);
        connect(m_animatedImage, &QMovie::frameChanged, this, &PixmapViewer::updateAnimatedImageFrame);
    }

    if (m_animatedImage->fileName() != fileName) {
        m_animatedImage->setFileName(fileName);
    }

    m_hasAnimatedImage = m_animatedImage->isValid() && (m_animatedImage->frameCount() > 1);
}

QString PixmapViewer::animatedImageFileName() const
{
    if (!m_hasAnimatedImage) {
        return QString();
    }
    return m_animatedImage->fileName();
}

void PixmapViewer::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);

    QPainter painter(this);

    if (!m_pixmap.isNull()) {
        style()->drawItemPixmap(&painter, rect(), Qt::AlignCenter, m_pixmap);
    }
}

void PixmapViewer::updateAnimatedImageFrame()
{
    Q_ASSERT(m_animatedImage);

    m_pixmap = m_animatedImage->currentPixmap();
    if (m_pixmap.width() > m_sizeHint.width() || m_pixmap.height() > m_sizeHint.height()) {
        m_pixmap = m_pixmap.scaled(m_sizeHint, Qt::KeepAspectRatio);
        m_animatedImage->setScaledSize(m_pixmap.size());
    }
    update();
}

void PixmapViewer::stopAnimatedImage()
{
    if (m_hasAnimatedImage) {
        m_animatedImage->stop();
        m_hasAnimatedImage = false;
        delete m_animatedImage;
        m_animatedImage = nullptr;
    }
}

bool PixmapViewer::isAnimatedMimeType(const QString &mimeType)
{
    const QList<QByteArray> imageFormats = QImageReader::imageFormatsForMimeType(mimeType.toUtf8());
    return std::any_of(imageFormats.begin(), imageFormats.end(), [](const QByteArray &format) {
        return QMovie::supportedFormats().contains(format);
    });
}

#include "moc_pixmapviewer.cpp"
