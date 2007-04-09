/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz (peter.penz@gmx.at) and              *
 *   and Patrice Tremblay                                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA          *
 ***************************************************************************/

#include "statusbarspaceinfo.h"

#include <QPainter>
#include <QPaintEvent>
#include <QTimer>

#include <kglobalsettings.h>
#include <kdiskfreesp.h>
#include <klocale.h>
#include <kio/job.h>

StatusBarSpaceInfo::StatusBarSpaceInfo(QWidget* parent) :
        QWidget(parent),
        m_gettingSize(false),
        m_kBSize(0),
        m_kBAvailable(0)
{
    setMinimumWidth(200);

    // Update the space information each 10 seconds. Polling is useful
    // here, as files can be deleted/added outside the scope of Dolphin.
    QTimer* timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(refresh()));
    timer->start(10000);
}

StatusBarSpaceInfo::~StatusBarSpaceInfo()
{}

void StatusBarSpaceInfo::setUrl(const KUrl& url)
{
    m_url = url;
    refresh();
    QTimer::singleShot(300, this, SLOT(update()));
}

void StatusBarSpaceInfo::paintEvent(QPaintEvent* /* event */)
{
    QPainter painter(this);
    const int barWidth = width();
    const int barTop = 1;
    const int barHeight = height() - 5;

    QString text;

    const int widthDec = 3;  // visual decrement for the available width

    const QColor c1 = palette().brush(QPalette::Background).color();
    const QColor c2 = KGlobalSettings::buttonTextColor();
    const QColor frameColor((c1.red()   + c2.red())   / 2,
                            (c1.green() + c2.green()) / 2,
                            (c1.blue()  + c2.blue())  / 2);
    painter.setPen(frameColor);

    const QColor backgrColor = KGlobalSettings::baseColor();
    painter.setBrush(backgrColor);

    painter.drawRect(QRect(0, barTop + 1 , barWidth - widthDec, barHeight));

    if ((m_kBSize > 0) && (m_kBAvailable > 0)) {
        // draw 'used size' bar
        painter.setPen(Qt::NoPen);
        painter.setBrush(progressColor(backgrColor));
        int usedWidth = barWidth - static_cast<int>((m_kBAvailable *
                        static_cast<float>(barWidth)) / m_kBSize);
        const int left = 1;
        int right = usedWidth - widthDec;
        if (right < left) {
            right = left;
        }
        painter.drawRect(QRect(left, barTop + 2, right, barHeight - 1));

        text = i18n("%1% of %2 used", 100 - (int)(100.0 * m_kBAvailable / m_kBSize), KIO::convertSizeFromKiB(m_kBSize));
    } else {
        if (m_gettingSize) {
            text = i18n("Getting size...");
        } else {
            text = QString();
            QTimer::singleShot(0, this, SLOT(hide()));
        }
    }

    // draw text (usually 'X% of Y GB used')
    painter.setPen(KGlobalSettings::textColor());
    painter.drawText(QRect(1, 1, barWidth - 2, barHeight + 6),
                     Qt::AlignCenter | Qt::TextWordWrap,
                     text);
}


void StatusBarSpaceInfo::slotFoundMountPoint(const unsigned long& kBSize,
        const unsigned long& /* kBUsed */,
        const unsigned long& kBAvailable,
        const QString& /* mountPoint */)
{
    m_gettingSize = false;
    m_kBSize = kBSize;
    m_kBAvailable = kBAvailable;

    // Bypass a the issue (?) of KDiskFreeSp that for protocols like
    // FTP, SMB the size of root partition is returned.
    // TODO: check whether KDiskFreeSp is buggy or Dolphin uses it in a wrong way
    const QString protocol(m_url.protocol());
    if (!protocol.isEmpty() && (protocol != "file")) {
        m_kBSize = 0;
        m_kBAvailable = 0;
    }

    update();
}

void StatusBarSpaceInfo::showResult()
{
    m_gettingSize = false;
    update();
}

void StatusBarSpaceInfo::refresh()
{
    m_gettingSize = true;
    m_kBSize = 0;
    m_kBAvailable = 0;

    const QString mountPoint(KIO::findPathMountPoint(m_url.path()));

    KDiskFreeSp* job = new KDiskFreeSp(this);
    connect(job, SIGNAL(foundMountPoint(const unsigned long&,
                                        const unsigned long&,
                                        const unsigned long&,
                                        const QString&)),
            this, SLOT(slotFoundMountPoint(const unsigned long&,
                                           const unsigned long&,
                                           const unsigned long&,
                                           const QString&)));
    connect(job, SIGNAL(done()),
            this, SLOT(showResult()));

    job->readDF(mountPoint);
}

QColor StatusBarSpaceInfo::progressColor(const QColor& bgColor) const
{
    QColor color = KGlobalSettings::buttonBackground();

    // assure that enough contrast is given between the background color
    // and the progressbar color
    int bgRed   = bgColor.red();
    int bgGreen = bgColor.green();
    int bgBlue  = bgColor.blue();

    const int backgrBrightness = qGray(bgRed, bgGreen, bgBlue);
    const int progressBrightness = qGray(color.red(), color.green(), color.blue());

    const int limit = 32;
    const int diff = backgrBrightness - progressBrightness;
    bool adjustColor = ((diff >= 0) && (diff <  limit)) ||
                       ((diff  < 0) && (diff > -limit));
    if (adjustColor) {
        const int inc = (backgrBrightness < 2 * limit) ? (2 * limit) : -limit;
        color = QColor(bgRed + inc, bgGreen + inc, bgBlue + inc);
    }

    return color;
}

#include "statusbarspaceinfo.moc"
