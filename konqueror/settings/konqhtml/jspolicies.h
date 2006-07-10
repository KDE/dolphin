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

#ifndef __JSPOLICIES_H__
#define __JSPOLICIES_H__

#include <q3groupbox.h>
#include <QString>

#include <khtml_settings.h>

#include "policies.h"

class QRadioButton;
class Q3ButtonGroup;

// special value for inheriting a global policy
#define INHERIT_POLICY		32767

/**
 * @short Contains all the JavaScript policies and methods for their manipulation.
 *
 * This class provides access to the JavaScript policies.
 *
 * @author Leo Savernik
 */
class JSPolicies : public Policies {
public:
#if 0
  /**
   * Enumeration for all policies
   */
  enum Policies { JavaScriptEnabled = 0, WindowOpen, WindowResize,
  		WindowMove, WindowFocus, WindowStatus, NumPolicies };
#endif

  /**
   * constructor
   * @param config configuration to initialize this instance from
   * @param group config group to use if this instance contains the global
   *	policies (global == true)
   * @param global true if this instance contains the global policy settings,
   *	false if this instance contains policies specific for a domain.
   * @param domain name of the domain this instance is used to configure the
   *	policies for (case insensitive, ignored if global == true)
   */
  JSPolicies(KSharedConfig::Ptr config, const QString &group, bool global,
  		const QString &domain = QString());
		
  /**
   * dummy constructor to make QMap happy.
   *
   * Never construct an object by using this.
   * @internal
   */
  JSPolicies();

  virtual ~JSPolicies();

  /**
   * Returns whether the WindowOpen policy is inherited.
   */
  bool isWindowOpenPolicyInherited() const {
    return window_open == INHERIT_POLICY;
  }
  /**
   * Returns the current value of the WindowOpen policy.
   *
   * This will return an illegal value if isWindowOpenPolicyInherited is
   * true.
   */
  KHTMLSettings::KJSWindowOpenPolicy windowOpenPolicy() const {
    return (KHTMLSettings::KJSWindowOpenPolicy)window_open;
  }

  /**
   * Returns whether the WindowResize policy is inherited.
   */
  bool isWindowResizePolicyInherited() const {
    return window_resize == INHERIT_POLICY;
  }
  /**
   * Returns the current value of the WindowResize policy.
   *
   * This will return an illegal value if isWindowResizePolicyInherited is
   * true.
   */
  KHTMLSettings::KJSWindowResizePolicy windowResizePolicy() const {
    return (KHTMLSettings::KJSWindowResizePolicy)window_resize;
  }

  /**
   * Returns whether the WindowMove policy is inherited.
   */
  bool isWindowMovePolicyInherited() const {
    return window_move == INHERIT_POLICY;
  }
  /**
   * Returns the current value of the WindowMove policy.
   *
   * This will return an illegal value if isWindowMovePolicyInherited is
   * true.
   */
  KHTMLSettings::KJSWindowMovePolicy windowMovePolicy() const {
    return (KHTMLSettings::KJSWindowMovePolicy)window_move;
  }

  /**
   * Returns whether the WindowFocus policy is inherited.
   */
  bool isWindowFocusPolicyInherited() const {
    return window_focus == INHERIT_POLICY;
  }
  /**
   * Returns the current value of the WindowFocus policy.
   *
   * This will return an illegal value if isWindowFocusPolicyInherited is
   * true.
   */
  KHTMLSettings::KJSWindowFocusPolicy windowFocusPolicy() const {
    return (KHTMLSettings::KJSWindowFocusPolicy)window_focus;
  }

  /**
   * Returns whether the WindowStatus policy is inherited.
   */
  bool isWindowStatusPolicyInherited() const {
    return window_status == INHERIT_POLICY;
  }
  /**
   * Returns the current value of the WindowStatus policy.
   *
   * This will return an illegal value if isWindowStatusPolicyInherited is
   * true.
   */
  KHTMLSettings::KJSWindowStatusPolicy windowStatusPolicy() const {
    return (KHTMLSettings::KJSWindowStatusPolicy)window_status;
  }

  /**
   * (re)loads settings from configuration file given in the constructor.
   */
  virtual void load();
  /**
   * saves current settings to the configuration file given in the constructor
   */
  virtual void save();
  /**
   * restores the default settings
   */
  virtual void defaults();

private:
  // one of KHTMLSettings::KJSWindowOpenPolicy or INHERIT_POLICY
  unsigned int window_open;
  // one of KHTMLSettings::KJSWindowResizePolicy or INHERIT_POLICY
  unsigned int window_resize;
  // one of KHTMLSettings::KJSWindowMovePolicy or INHERIT_POLICY
  unsigned int window_move;
  // one of KHTMLSettings::KJSWindowFocusPolicy or INHERIT_POLICY
  unsigned int window_focus;
  // one of KHTMLSettings::KJSWindowStatusPolicy or INHERIT_POLICY
  unsigned int window_status;

  friend class JSPoliciesFrame;	// for changing policies
};

/**
 * @short Provides a framed widget with controls for the JavaScript policy settings.
 *
 * This widget contains controls for changing all JavaScript policies
 * except the JavaScript enabled policy itself. The rationale behind this is
 * that the enabled policy be separate from the rest in a prominent
 * place.
 *
 * It is suitable for the global policy settings as well as for the
 * domain-specific settings.
 *
 * The difference between global and domain-specific is the existence of
 * a special inheritance option in the latter case. That way domain-specific
 * policies can inherit their value from the global policies.
 *
 * @author Leo Savernik
 */
class JSPoliciesFrame : public Q3GroupBox {
  Q_OBJECT
public:
  /**
   * constructor
   * @param policies associated object containing the policy values. This
   *	object will be updated accordingly as the settings are changed.
   * @param title title for group box
   * @param parent parent widget
   */
  JSPoliciesFrame(JSPolicies *policies, const QString &title,
		QWidget* parent = 0);

  virtual ~JSPoliciesFrame();

  /**
   * updates the controls to resemble the status of the underlying
   * JSPolicies object.
   */
  void refresh();
  /**
   * (re)loads settings from configuration file given in the constructor.
   */
  void load() {
    policies->load();
    refresh();
  }
  /**
   * saves current settings to the configuration file given in the constructor
   */
  void save() {
    policies->save();
  }
  /**
   * restores the default settings
   */
  void defaults() {
    policies->defaults();
    refresh();
  }

Q_SIGNALS:
  /**
   * emitted every time an option has been changed
   */
  void changed();

private Q_SLOTS:
  void setWindowOpenPolicy(int id);
  void setWindowResizePolicy(int id);
  void setWindowMovePolicy(int id);
  void setWindowFocusPolicy(int id);
  void setWindowStatusPolicy(int id);

private:

  JSPolicies *policies;
  Q3ButtonGroup *js_popup;
  Q3ButtonGroup *js_resize;
  Q3ButtonGroup *js_move;
  Q3ButtonGroup *js_focus;
  Q3ButtonGroup *js_statusbar;
};


#endif		// __JSPOLICIES_H__

