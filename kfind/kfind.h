/***********************************************************************
 *
 *  Kfind.h
 *
 ***********************************************************************/

#ifndef KFIND_H
#define KFIND_H

#include <QWidget>
#include <kfileitem.h>
#include <kdirlister.h>

class QString;
class KPushButton;

class KQuery;
class KUrl;
class KfindTabWidget;

class Kfind: public QWidget
{
    Q_OBJECT

public:
    Kfind(QWidget * parent = 0, const char * name = 0);
    ~Kfind();

    void setURL( const KUrl &url );

    void setQuery(KQuery * q) { query = q; }
    void searchFinished();

    void saveState( QDataStream *stream );
    void restoreState( QDataStream *stream );

public Q_SLOTS:
    void startSearch();
    void stopSearch();
    //void newSearch();
    void saveResults();

Q_SIGNALS:
    void haveResults(bool);
    void resultSelected(bool);

    void started();
    void destroyMe();

private:
    void setFocus();
    KfindTabWidget *tabWidget;
    KPushButton *mSearch;
    KPushButton *mStop;
    KPushButton *mSave;
    KQuery *query;

public:
    KDirLister *dirlister;
};

#endif


