#ifndef __TEMPLATE_H__
#define __TEMPLATE_H__


#include <QString>
#include <QMap>

class CSSTemplate
{
public:

  CSSTemplate(QString fname) : _filename(fname) {};
  bool expand(QString destname, const QMap<QString,QString> &dict);

protected:
  QString _filename;

};


#endif
