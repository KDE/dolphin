#ifndef __KCOOKIESMANAGEMENT_H
#define __KCOOKIESMANAGEMENT_H

#include <kcmodule.h>

class KCookiesManagement : public KCModule
{
Q_OBJECT
  public:
    KCookiesManagement(QWidget *parent = 0L, const char *name = 0L);
    ~KCookiesManagement();

    virtual void load();
    virtual void save();
    virtual void defaults();
    virtual QString quickHelp();

  private slots:
    void changed();
};

#endif // __KCOOKIESMANAGEMENT_H
