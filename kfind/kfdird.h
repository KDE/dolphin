/***********************************************************************
 *
 *  KfDirD.h
 *
 **********************************************************************/

#ifndef KFDIRDLG_H
#define KFDIRDLG_H

#include <qdir.h>
#include <qdialog.h>

class QListBox;
class QLineEdit;
class QLabel;
class QPushButton;

class KfDirDialog : public QDialog
{
    Q_OBJECT
public:
    KfDirDialog( const QString& dirName,
                 QWidget *parent=0, const char *name=0, bool modal=FALSE );
   ~KfDirDialog();

    QString     selectedDir()  const;

signals:

private slots:
    void        dirSelected( int );
    void        pathSelected();

protected:
      
private:
    void        init();
    void        checkDir( const QString&, bool );
    void        rereadDir();

    QDir        d;
    QString     dirName;

    QLabel     *pathL;
    QLineEdit  *path;
    QLabel     *dirL;
    QListBox   *dirs;

    QPushButton *okB;
    QPushButton *cancelB;

private:        // Disabled copy constructor and operator=
    KfDirDialog( const KfDirDialog & );
    KfDirDialog &operator=( const KfDirDialog & );
};

#endif
