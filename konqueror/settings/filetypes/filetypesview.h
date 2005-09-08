#ifndef _FILETYPESVIEW_H
#define _FILETYPESVIEW_H

#include <q3ptrlist.h>
#include <qmap.h>
//Added by qt3to4:
#include <QLabel>
#include <Q3ValueList>

#include <kconfig.h>
#include <kcmodule.h>

#include "typeslistitem.h"

class QLabel;
class KListView;
class Q3ListViewItem;
class Q3ListBox;
class QPushButton;
class KIconButton;
class QLineEdit;
class QComboBox;
class FileTypeDetails;
class FileGroupDetails;
class Q3WidgetStack;

class FileTypesView : public KCModule
{
  Q_OBJECT
public:
  FileTypesView(QWidget *p = 0, const char *name = 0);
  ~FileTypesView();

  void load();
  void save();
  void defaults();

protected slots:
  /** fill in the various graphical elements, set up other stuff. */
  void init();

  void addType();
  void removeType();
  void updateDisplay(Q3ListViewItem *);
  void slotDoubleClicked(Q3ListViewItem *);
  void slotFilter(const QString &patternFilter);
  void setDirty(bool state);

  void slotDatabaseChanged();
  void slotEmbedMajor(const QString &major, bool &embed);

protected:
  void readFileTypes();
  bool sync( Q3ValueList<TypesListItem *>& itemsModified );

private:
  KListView *typesLV;
  QPushButton *m_removeTypeB;

  Q3WidgetStack * m_widgetStack;
  FileTypeDetails * m_details;
  FileGroupDetails * m_groupDetails;
  QLabel * m_emptyWidget;

  QLineEdit *patternFilterLE;
  QStringList removedList;
  bool m_dirty;
  QMap<QString,TypesListItem*> m_majorMap;
  Q3PtrList<TypesListItem> m_itemList;

  Q3ValueList<TypesListItem *> m_itemsModified;

  KSharedConfig::Ptr m_konqConfig;
};

#endif
