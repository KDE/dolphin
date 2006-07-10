/*
  Copyright (c) 2002 Leo Savernik <l.savernik@aon.at>
  Derived from jsopt.cpp, code copied from there is copyrighted to its
  respective owners.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/

#include <kconfig.h>
#include <kdebug.h>

#include "policies.h"

// == class Policies ==

Policies::Policies(KSharedConfig::Ptr config,const QString &group,
		bool global,const QString &domain, const QString &prefix,
		const QString &feature_key) :
	is_global(global), config(config), groupname(group),
	prefix(prefix), feature_key(feature_key) {

  if (is_global) {
    this->prefix.clear();	// global keys have no prefix
  }/*end if*/
  setDomain(domain);
}

Policies::~Policies() {
}

void Policies::setDomain(const QString &domain) {
  if (is_global) return;
  this->domain = domain.toLower();
  groupname = this->domain;	// group is domain in this case
}

void Policies::load() {
  config->setGroup(groupname);

  QString key = prefix + feature_key;
  if (config->hasKey(key))
    feature_enabled = config->readEntry(key, QVariant(false)).toBool();
  else
    feature_enabled = is_global ? true : INHERIT_POLICY;
}

void Policies::defaults() {
  feature_enabled = is_global ? true : INHERIT_POLICY;
}

void Policies::save() {
  config->setGroup(groupname);

  QString key = prefix + feature_key;
  if (feature_enabled != INHERIT_POLICY)
    config->writeEntry(key, (bool)feature_enabled);
  else
    config->deleteEntry(key);

  // don't do a config->sync() here for sake of efficiency
}
