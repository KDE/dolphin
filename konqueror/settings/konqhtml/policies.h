/*
  Copyright (c) 2002 Leo Savernik <l.savernik@aon.at>
  Derived from jsopt.h, code copied from there is copyrighted to its
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

#ifndef __POLICIES_H__
#define __POLICIES_H__

#include <QString>
#include <kconfig.h>

// special value for inheriting a global policy
#define INHERIT_POLICY		32767

/**
 * @short Contains the basic policies and methods for their manipulation.
 *
 * This class provides access to the basic policies that are common
 * to all features.
 *
 * @author Leo Savernik
 */
class Policies {
public:
  /**
   * constructor
   * @param config configuration to initialize this instance from
   * @param group config group to use if this instance contains the global
   *	policies (global == true)
   * @param global true if this instance contains the global policy settings,
   *	false if it contains policies specific to a domain.
   * @param domain name of the domain this instance is used to configure the
   *	policies for (case insensitive, ignored if global == true)
   * @param prefix prefix to use for configuration keys. The domain-specific
   *	policies use of the format "&lt;feature&gt;." (note the trailing dot).
   *	Global policies have no prefix, it is ignored if global == true.
   * @param feature_key key of the "feature enabled" policy. The final
   *	key the policy is stored under will be prefix + featureKey.
   */
  Policies(KSharedConfig::Ptr config, const QString &group, bool global,
  		const QString &domain, const QString &prefix,
		const QString &feature_key);

  virtual ~Policies();

  /**
   * Returns true if this is the global policies object
   */
  bool isGlobal() const {
    return is_global;
  }

  /** sets a new domain for this policy
   * @param domain domain name, will be converted to lowercase
   */
  void setDomain(const QString &domain);

  /**
   * Returns whether the "feature enabled" policy is inherited.
   */
  bool isFeatureEnabledPolicyInherited() const {
    return feature_enabled == INHERIT_POLICY;
  }
  /** inherits "feature enabled" policy */
  void inheritFeatureEnabledPolicy() {
    feature_enabled = INHERIT_POLICY;
  }
  /**
   * Returns whether this feature is enabled.
   *
   * This will return an illegal value if isFeatureEnabledPolicyInherited
   * is true.
   */
  bool isFeatureEnabled() const {
    return (bool)feature_enabled;
  }
  /**
   * Enables/disables this feature
   * @param on true will enable it, false disable it
   */
  void setFeatureEnabled(int on) {
    feature_enabled = on;
  }

  /**
   * (re)loads settings from configuration file given in the constructor.
   *
   * Implicitely sets the group given in the constructor. Don't forget to
   * call this method from derived methods.
   */
  virtual void load();
  /**
   * saves current settings to the configuration file given in the constructor
   *
   * Implicitely sets the group given in the constructor. Don't forget to
   * call this method from derived methods.
   */
  virtual void save();
  /**
   * restores the default settings
   */
  virtual void defaults();

protected:
  // true or false or INHERIT_POLICY
  unsigned int feature_enabled;

  bool is_global;
  KSharedConfig::Ptr config;
  QString groupname;
  QString domain;
  QString prefix;
  QString feature_key;
};

#endif		// __POLICIES_H__

