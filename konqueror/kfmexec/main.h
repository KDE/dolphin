#ifndef _main_h
#define _main_h

#include <qobject.h>
#include <qstring.h>
#include <qstrlist.h>

class KFMExec : public QObject
{
    Q_OBJECT
public:
    KFMExec( int argc, char **argv );

    QString shellQuote( const char *_data );    

public slots:
    void slotFinished();
    
protected:
    int counter;
    int expectedCounter;
    QString command;
    QString files;
    QStringList fileList;
    QStringList urlList;
};

#endif
