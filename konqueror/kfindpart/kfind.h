/***********************************************************************
 *
 *  Kfind.h
 *
 ***********************************************************************/

#ifndef KFIND_H
#define KFIND_H

#include <qwidget.h>
#include <kfileitem.h>

class QString;

class KQuery;
class KURL;
class KfindTabWidget;

class Kfind: public QWidget
{
    Q_OBJECT

public:
    Kfind(QWidget * parent = 0, const char * name = 0);
    ~Kfind();

    void setURL( const KURL &url );

    void setQuery(KQuery * q) { query = q; }
    void searchFinished();

    void saveState( QDataStream *stream );
    void restoreState( QDataStream *stream );

public slots:
    void startSearch();
    void stopSearch();
    //void newSearch();
    void saveResults();

signals:
    void haveResults(bool);
    void resultSelected(bool);

    void started();
    void destroyMe();

private:
    void setFocus();
    KfindTabWidget *tabWidget;

    QPushButton *mSearch;
    QPushButton *mStop;
    QPushButton *mSave;
    KQuery *query;
};

#endif


