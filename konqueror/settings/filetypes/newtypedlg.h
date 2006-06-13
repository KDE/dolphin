#ifndef _NEWTYPEDLG_H
#define _NEWTYPEDLG_H

#include <QString>
#include <QStringList>

#include <kdialog.h>

class KLineEdit;
class QComboBox;

/**
 * A dialog for creating a new file type, with
 * a combobox for choosing the group and a line-edit
 * for entering the name of the file type
 */
class NewTypeDialog : public KDialog
{
public:
  NewTypeDialog(QStringList groups, QWidget *parent = 0, 
		const char *name = 0);
  QString group() const;
  QString text() const;
private:
  KLineEdit *typeEd;
  QComboBox *groupCombo;
};

#endif
