/***********************************************************************
 *
 *  kfmenu.h
 *
 ***********************************************************************/

#ifndef KFMENU_H
#define KFMENU_H

#include <qwidget.h>
#include <qmenubar.h>
#include <qlabel.h>


class KfindMenu : public QWidget
{
    Q_OBJECT
public:
    KfindMenu( QWidget *parent=0, const char *name=0 );

public slots:
    void enableSaveResults(bool);
    void enableMenuItems(bool);
    void kfind_help();

signals:
    //File Menu
    void open();
    void addToArchive();
    void deleteFile();
    void renameFile();
    void properties();
    void openFolder();
    void saveResults();

    //Edit Menu
    void undo();
    void cut();
    void copy();
    void selectAll();
    void invertSelection();

    //Options Menu
    void prefs();

  /*    void filetypes();
    void archives();
    void saving();*/

    //Help Menu
    void about();

private:
    QMenuBar   *menu;
    QPopupMenu *file;
    int        openWithM;
    int        toArchM;
    int        deleteM;
    int        renameM;
    int        propsM;
    int        openFldrM;
    int        saveSearchM;
    int        quitM;
    QLabel     *label;
};


#endif 
