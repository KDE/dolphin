// Necessary for the moc stuff in qiconview.cpp
// since am_edit doesn't handle running moc on .cpp files

#include <qmultilineedit.h>
#include <qstring.h>
class QWidget;

#include "qiconview.h"

class QIconViewItemLineEdit : public QMultiLineEdit
{
    friend class QIconViewItem;
 
    Q_OBJECT
 
public:
    QIconViewItemLineEdit( const QString &text, QWidget *parent, QIconViewItem *theItem, const char
*name = 0 );
 
signals:
    void escapePressed();
 
protected:
    void keyPressEvent( QKeyEvent *e );
    void focusOutEvent( QFocusEvent *e );
 
protected:
    QIconViewItem *item;
    QString startText;
 
};

#include "qiconview.moc.cpp"

