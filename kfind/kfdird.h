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
class QComboBox;
class QLabel;
class QPushButton;


class KfDirDialog : public QDialog
{
    Q_OBJECT
public:
    KfDirDialog( const char *dirName,
                 QWidget *parent=0, const char *name=0, bool modal=FALSE );
    KfDirDialog( QWidget *parent=0, const char *name=0, bool modal=FALSE );
   ~KfDirDialog();

    QString     selectedDir()  const;

    const char *dirPath() const;
    void        setDir( const char * );
    const QDir *dir() const;
    void        setDir( const QDir & );

    void        rereadDir();

signals:
    void        dirHighlighted( const char * );
    void        dirSelected( const char * );
    void        dirEntered( const char * );

private slots:
    void        dirHighlighted( int );
    void        dirSelected( int );
    void        pathSelected( int );

    void        okClicked();
    void        cancelClicked();

protected:
    void        resizeEvent( QResizeEvent * );
      
private:
    void        init();
    void        updatePathBox( const char * );

    QDir        d;
    QString     dirName;

    QListBox   *dirs;
    QComboBox  *pathBox;
    QLabel     *dirL;

    QPushButton *okB;
    QPushButton *cancelB;

private:        // Disabled copy constructor and operator=
    KfDirDialog( const KfDirDialog & ) {}
    KfDirDialog &operator=( const KfDirDialog & ) { return *this; }
};

#endif
