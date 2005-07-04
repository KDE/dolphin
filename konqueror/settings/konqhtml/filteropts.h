/*
  Copyright (C) 2005 Ivor Hewitt <ivor@ivor.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301, USA.
*/
#ifndef FILTEROPTS_H
#define FILTEROPTS_H

#include <kcmodule.h>

class QListBox;
class QPushButton;
class QLineEdit;
class QListBoxItem;
class QCheckBox;

class KConfig;

class KCMFilter : public KCModule
{
    Q_OBJECT
public:
    KCMFilter( KConfig* config, QString group, QWidget* parent = 0, const char* name = 0 );
    ~KCMFilter();
    
    void load();
    void save();
    void defaults();
    QString quickHelp() const;
    
public slots:

protected slots:
    void insertFilter();
    void updateFilter();
    void removeFilter();
    void slotItemSelected();
    void slotEnableChecked();
    void slotKillChecked();

    void exportFilters();
    void importFilters();

private:
    void updateButton();
    QListBox *mListBox;
    QLineEdit *mString;
    QCheckBox *mEnableCheck;
    QCheckBox *mKillCheck;
    QPushButton *mInsertButton;
    QPushButton *mUpdateButton;
    QPushButton *mRemoveButton;
    QPushButton *mImportButton;
    QPushButton *mExportButton;

    KConfig *mConfig;
    QString mGroupname;
    int mSelCount;
};

#endif
