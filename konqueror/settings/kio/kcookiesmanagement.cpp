// kcookiesmanagement.cpp - Cookies management

#include <klocale.h>

#include "kcookiesmanagement.h"

KCookiesManagement::KCookiesManagement(QWidget *parent, const char *name)
                   :KCModule(parent, name)
{
}

KCookiesManagement::~KCookiesManagement()
{
}

void KCookiesManagement::load()
{
}

void KCookiesManagement::save()
{
}

void KCookiesManagement::defaults()
{
}

QString KCookiesManagement::quickHelp() const
{
  return i18n("<h1>KCookiesManagement::quickHelp()</h1>" );
}

void KCookiesManagement::changed()
{
  emit KCModule::changed(true);
}

#include "kcookiesmanagement.moc"
