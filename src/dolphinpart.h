/* This file is part of the KDE project
   Copyright (c) 2007 David Faure <faure@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef DOLPHINPART_H
#define DOLPHINPART_H

#include <kparts/part.h>
class KFileItem;
class DolphinPartBrowserExtension;
class DolphinSortFilterProxyModel;
class KDirModel;
class KDirLister;
class DolphinView;
class QLineEdit;
class KAboutData;
namespace KParts { class ReadOnlyPart; }

class DolphinPart : public KParts::ReadOnlyPart
{
    Q_OBJECT

public:
    explicit DolphinPart(QWidget* parentWidget, QObject* parent, const QStringList& args);
    ~DolphinPart();

    static KAboutData* createAboutData();

    virtual bool openUrl(const KUrl& url);

protected:
    virtual bool openFile() { return true; }

private Q_SLOTS:
    void slotCompleted(const KUrl& url);
    void slotCanceled(const KUrl& url);
    void slotInfoMessage(const QString& msg);
    void slotErrorMessage(const QString& msg);
    void slotRequestItemInfo(const KFileItem& item);
    void slotItemTriggered(const KFileItem& item);

private:
    DolphinView* m_view;
    KDirLister* m_dirLister;
    KDirModel* m_dirModel;
    DolphinSortFilterProxyModel* m_proxyModel;
    DolphinPartBrowserExtension* m_extension;
    Q_DISABLE_COPY(DolphinPart)
};


#endif /* DOLPHINPART_H */

