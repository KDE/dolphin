#ifndef _main_h
#define _main_h

#include <qobject.h>
#include <qstring.h>
#include <qstrlist.h>

namespace KIO { class Job; }

class KFMExec : public QObject
{
    Q_OBJECT
public:
    KFMExec();

    QString shellQuote( const QString & _data );

public slots:
    void slotResult( KIO::Job * );

protected:
    int counter;
    int expectedCounter;
    QString command;
    QString files;
    QStringList fileList;
    KURL::List urlList;
};

#endif
