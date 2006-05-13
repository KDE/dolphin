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

#include <q3buttongroup.h>
#include <QLabel>
#include <QLayout>
#include <QRadioButton>

//Added by qt3to4:
#include <QGridLayout>

#include <kconfig.h>
#include <kdebug.h>
#include <klocale.h>

#include "jspolicies.h"

// == class JSPolicies ==

JSPolicies::JSPolicies(KConfig* config,const QString &group,
		bool global,const QString &domain) :
	Policies(config,group,global,domain,"javascript.","EnableJavaScript") {
}

JSPolicies::JSPolicies() : Policies(0,QString(),false,
	QString(),QString(),QString()) {
}

JSPolicies::~JSPolicies() {
}

void JSPolicies::load() {
  Policies::load();

  QString key;

//  enableJavaScriptDebugCB->setChecked( m_pConfig->readEntry("EnableJavaScriptDebug", QVariant(false)).toBool());
//  enableDebugOutputCB->setChecked( m_pConfig->readEntry("EnableJSDebugOutput", QVariant(false)).toBool() );
  key = prefix + "WindowOpenPolicy";
  window_open = config->readEntry(key,
  	is_global ? KHTMLSettings::KJSWindowOpenSmart : INHERIT_POLICY);

  key = prefix + "WindowResizePolicy";
  window_resize = config->readEntry(key,
  	is_global ? KHTMLSettings::KJSWindowResizeAllow : INHERIT_POLICY);

  key = prefix + "WindowMovePolicy";
  window_move = config->readEntry(key,
  	is_global ? KHTMLSettings::KJSWindowMoveAllow : INHERIT_POLICY);

  key = prefix + "WindowFocusPolicy";
  window_focus = config->readEntry(key,
  	is_global ? KHTMLSettings::KJSWindowFocusAllow : INHERIT_POLICY);

  key = prefix + "WindowStatusPolicy";
  window_status = config->readEntry(key,
  	is_global ? KHTMLSettings::KJSWindowStatusAllow : INHERIT_POLICY);
}

void JSPolicies::defaults() {
  Policies::defaults();
//  enableJavaScriptGloballyCB->setChecked( true );
//  enableJavaScriptDebugCB->setChecked( false );
//  js_popup->setButton(0);
 // enableDebugOutputCB->setChecked( false );
  window_open = is_global ? KHTMLSettings::KJSWindowOpenSmart : INHERIT_POLICY;
  window_resize = is_global ? KHTMLSettings::KJSWindowResizeAllow : INHERIT_POLICY;
  window_move = is_global ? KHTMLSettings::KJSWindowMoveAllow : INHERIT_POLICY;
  window_focus = is_global ? KHTMLSettings::KJSWindowFocusAllow : INHERIT_POLICY;
  window_status = is_global ? KHTMLSettings::KJSWindowStatusAllow : INHERIT_POLICY;
}

void JSPolicies::save() {
  Policies::save();

  QString key;
  key = prefix + "WindowOpenPolicy";
  if (window_open != INHERIT_POLICY)
    config->writeEntry(key, window_open);
  else
    config->deleteEntry(key);

  key = prefix + "WindowResizePolicy";
  if (window_resize != INHERIT_POLICY)
    config->writeEntry(key, window_resize);
  else
    config->deleteEntry(key);

  key = prefix + "WindowMovePolicy";
  if (window_move != INHERIT_POLICY)
    config->writeEntry(key, window_move);
  else
    config->deleteEntry(key);

  key = prefix + "WindowFocusPolicy";
  if (window_focus != INHERIT_POLICY)
    config->writeEntry(key, window_focus);
  else
    config->deleteEntry(key);

  key = prefix + "WindowStatusPolicy";
  if (window_status != INHERIT_POLICY)
    config->writeEntry(key, window_status);
  else
    config->deleteEntry(key);

  // don't do a config->sync() here for sake of efficiency
}

// == class JSPoliciesFrame ==

JSPoliciesFrame::JSPoliciesFrame(JSPolicies *policies, const QString &title,
		QWidget* parent) :
	Q3GroupBox(title, parent, "jspoliciesframe"),
	policies(policies) {

  bool is_per_domain = !policies->isGlobal();

  setColumnLayout(0, Qt::Vertical);
  layout()->setSpacing(0);
  layout()->setMargin(0);
  QGridLayout *this_layout = new QGridLayout();
  layout()->addItem( this_layout );
  this_layout->setAlignment(Qt::AlignTop);
  this_layout->setSpacing(3);
  this_layout->setMargin(11);

  QString wtstr;	// what's this description
  int colIdx;		// column index

  // === window.open ================================
  colIdx = 0;
  QLabel *label = new QLabel(i18n("Open new windows:"),this);
  this_layout->addWidget(label,0,colIdx++);

  js_popup = new Q3ButtonGroup(this);
  js_popup->setExclusive(true);
  js_popup->setHidden(true);

  QRadioButton* policy_btn;
  if (is_per_domain) {
    policy_btn = new QRadioButton(i18n("Use global"), this);
    policy_btn->setWhatsThis(i18n("Use setting from global policy."));
    js_popup->insert(policy_btn,INHERIT_POLICY);
    this_layout->addWidget(policy_btn,0,colIdx++);
    this_layout->addItem(new QSpacerItem(10,0),0,colIdx++);
  }/*end if*/

  policy_btn = new QRadioButton(i18n("Allow"), this);
  policy_btn->setWhatsThis(i18n("Accept all popup window requests."));
  js_popup->insert(policy_btn,KHTMLSettings::KJSWindowOpenAllow);
  this_layout->addWidget(policy_btn,0,colIdx++);
  this_layout->addItem(new QSpacerItem(10,0),0,colIdx++);

  policy_btn = new QRadioButton(i18n("Ask"), this);
  policy_btn->setWhatsThis(i18n("Prompt every time a popup window is requested."));
  js_popup->insert(policy_btn,KHTMLSettings::KJSWindowOpenAsk);
  this_layout->addWidget(policy_btn,0,colIdx++);
  this_layout->addItem(new QSpacerItem(10,0),0,colIdx++);

  policy_btn = new QRadioButton(i18n("Deny"), this);
  policy_btn->setWhatsThis(i18n("Reject all popup window requests."));
  js_popup->insert(policy_btn,KHTMLSettings::KJSWindowOpenDeny);
  this_layout->addWidget(policy_btn,0,colIdx++);
  this_layout->addItem(new QSpacerItem(10,0),0,colIdx++);

  policy_btn = new QRadioButton(i18n("Smart"), this);
  policy_btn->setWhatsThis( i18n("Accept popup window requests only when "
                                   "links are activated through an explicit "
                                   "mouse click or keyboard operation."));
  js_popup->insert(policy_btn,KHTMLSettings::KJSWindowOpenSmart);
  this_layout->addWidget(policy_btn,0,colIdx++);
  this_layout->addItem(new QSpacerItem(10,0),0,colIdx++);

  wtstr = i18n("If you disable this, Konqueror will stop "
               "interpreting the <i>window.open()</i> "
               "JavaScript command. This is useful if you "
               "regularly visit sites that make extensive use "
               "of this command to pop up ad banners.<br>"
               "<br><b>Note:</b> Disabling this option might "
               "also break certain sites that require <i>"
               "window.open()</i> for proper operation. Use "
               "this feature carefully.");
  label->setWhatsThis( wtstr);
  connect(js_popup, SIGNAL(clicked(int)), SLOT(setWindowOpenPolicy(int)));

  // === window.resizeBy/resizeTo ================================
  colIdx = 0;
  label = new QLabel(i18n("Resize window:"),this);
  this_layout->addWidget(label,1,colIdx++);

  js_resize = new Q3ButtonGroup(this);
  js_resize->setExclusive(true);
  js_resize->setHidden(true);

  if (is_per_domain) {
    policy_btn = new QRadioButton(i18n("Use global"), this);
    policy_btn->setWhatsThis(i18n("Use setting from global policy."));
    js_resize->insert(policy_btn,INHERIT_POLICY);
    this_layout->addWidget(policy_btn,1,colIdx++);
    this_layout->addItem(new QSpacerItem(10,0),0,colIdx++);
  }/*end if*/

  policy_btn = new QRadioButton(i18n("Allow"), this);
  policy_btn->setWhatsThis(i18n("Allow scripts to change the window size."));
  js_resize->insert(policy_btn,KHTMLSettings::KJSWindowResizeAllow);
  this_layout->addWidget(policy_btn,1,colIdx++);
  this_layout->addItem(new QSpacerItem(10,0),0,colIdx++);

  policy_btn = new QRadioButton(i18n("Ignore"), this);
  policy_btn->setWhatsThis(i18n("Ignore attempts of scripts to change the window size. "
  				"The web page will <i>think</i> it changed the "
				"size but the actual window is not affected."));
  js_resize->insert(policy_btn,KHTMLSettings::KJSWindowResizeIgnore);
  this_layout->addWidget(policy_btn,1,colIdx++);
  this_layout->addItem(new QSpacerItem(10,0),0,colIdx++);

  wtstr = i18n("Some websites change the window size on their own by using "
  		"<i>window.resizeBy()</i> or <i>window.resizeTo()</i>. "
		"This option specifies the treatment of such "
		"attempts.");
  label->setWhatsThis( wtstr);
  connect(js_resize, SIGNAL(clicked(int)), SLOT(setWindowResizePolicy(int)));

  // === window.moveBy/moveTo ================================
  colIdx = 0;
  label = new QLabel(i18n("Move window:"),this);
  this_layout->addWidget(label,2,colIdx++);

  js_move = new Q3ButtonGroup(this);
  js_move->setExclusive(true);
  js_move->setHidden(true);

  if (is_per_domain) {
    policy_btn = new QRadioButton(i18n("Use global"), this);
    policy_btn->setWhatsThis(i18n("Use setting from global policy."));
    js_move->insert(policy_btn,INHERIT_POLICY);
    this_layout->addWidget(policy_btn,2,colIdx++);
    this_layout->addItem(new QSpacerItem(10,0),0,colIdx++);
  }/*end if*/

  policy_btn = new QRadioButton(i18n("Allow"), this);
  policy_btn->setWhatsThis(i18n("Allow scripts to change the window position."));
  js_move->insert(policy_btn,KHTMLSettings::KJSWindowMoveAllow);
  this_layout->addWidget(policy_btn,2,colIdx++);
  this_layout->addItem(new QSpacerItem(10,0),0,colIdx++);

  policy_btn = new QRadioButton(i18n("Ignore"), this);
  policy_btn->setWhatsThis(i18n("Ignore attempts of scripts to change the window position. "
  				"The web page will <i>think</i> it moved the "
				"window but the actual position is not affected."));
  js_move->insert(policy_btn,KHTMLSettings::KJSWindowMoveIgnore);
  this_layout->addWidget(policy_btn,2,colIdx++);
  this_layout->addItem(new QSpacerItem(10,0),0,colIdx++);

  wtstr = i18n("Some websites change the window position on their own by using "
  		"<i>window.moveBy()</i> or <i>window.moveTo()</i>. "
		"This option specifies the treatment of such "
		"attempts.");
  label->setWhatsThis( wtstr);
  connect(js_move, SIGNAL(clicked(int)), SLOT(setWindowMovePolicy(int)));

  // === window.focus ================================
  colIdx = 0;
  label = new QLabel(i18n("Focus window:"),this);
  this_layout->addWidget(label,3,colIdx++);

  js_focus = new Q3ButtonGroup(this);
  js_focus->setExclusive(true);
  js_focus->setHidden(true);

  if (is_per_domain) {
    policy_btn = new QRadioButton(i18n("Use global"), this);
    policy_btn->setWhatsThis(i18n("Use setting from global policy."));
    js_focus->insert(policy_btn,INHERIT_POLICY);
    this_layout->addWidget(policy_btn,3,colIdx++);
    this_layout->addItem(new QSpacerItem(10,0),0,colIdx++);
  }/*end if*/

  policy_btn = new QRadioButton(i18n("Allow"), this);
  policy_btn->setWhatsThis(i18n("Allow scripts to focus the window.") );
  js_focus->insert(policy_btn,KHTMLSettings::KJSWindowFocusAllow);
  this_layout->addWidget(policy_btn,3,colIdx++);
  this_layout->addItem(new QSpacerItem(10,0),0,colIdx++);

  policy_btn = new QRadioButton(i18n("Ignore"), this);
  policy_btn->setWhatsThis(i18n("Ignore attempts of scripts to focus the window. "
  				"The web page will <i>think</i> it brought "
				"the focus to the window but the actual "
				"focus will remain unchanged.") );
  js_focus->insert(policy_btn,KHTMLSettings::KJSWindowFocusIgnore);
  this_layout->addWidget(policy_btn,3,colIdx++);
  this_layout->addItem(new QSpacerItem(10,0),0,colIdx++);

  wtstr = i18n("Some websites set the focus to their browser window on their "
  		"own by using <i>window.focus()</i>. This usually leads to "
		"the window being moved to the front interrupting whatever "
		"action the user was dedicated to at that time. "
		"This option specifies the treatment of such "
		"attempts.");
  label->setWhatsThis( wtstr);
  connect(js_focus, SIGNAL(clicked(int)), SLOT(setWindowFocusPolicy(int)));

  // === window.status ================================
  colIdx = 0;
  label = new QLabel(i18n("Modify status bar text:"),this);
  this_layout->addWidget(label,4,colIdx++);

  js_statusbar = new Q3ButtonGroup(this);
  js_statusbar->setExclusive(true);
  js_statusbar->setHidden(true);

  if (is_per_domain) {
    policy_btn = new QRadioButton(i18n("Use global"), this);
    policy_btn->setWhatsThis(i18n("Use setting from global policy."));
    js_statusbar->insert(policy_btn,INHERIT_POLICY);
    this_layout->addWidget(policy_btn,4,colIdx++);
    this_layout->addItem(new QSpacerItem(10,0),0,colIdx++);
  }/*end if*/

  policy_btn = new QRadioButton(i18n("Allow"), this);
  policy_btn->setWhatsThis(i18n("Allow scripts to change the text of the status bar."));
  js_statusbar->insert(policy_btn,KHTMLSettings::KJSWindowStatusAllow);
  this_layout->addWidget(policy_btn,4,colIdx++);
  this_layout->addItem(new QSpacerItem(10,0),0,colIdx++);

  policy_btn = new QRadioButton(i18n("Ignore"), this);
  policy_btn->setWhatsThis(i18n("Ignore attempts of scripts to change the status bar text. "
  				"The web page will <i>think</i> it changed "
				"the text but the actual text will remain "
				"unchanged.") );
  js_statusbar->insert(policy_btn,KHTMLSettings::KJSWindowStatusIgnore);
  this_layout->addWidget(policy_btn,4,colIdx++);
  this_layout->addItem(new QSpacerItem(10,0),0,colIdx++);

  wtstr = i18n("Some websites change the status bar text by setting "
  		"<i>window.status</i> or <i>window.defaultStatus</i>, "
		"thus sometimes preventing displaying the real URLs of hyperlinks. "
		"This option specifies the treatment of such "
		"attempts.");
  label->setWhatsThis( wtstr);
  connect(js_statusbar, SIGNAL(clicked(int)), SLOT(setWindowStatusPolicy(int)));
}

JSPoliciesFrame::~JSPoliciesFrame() {
}

void JSPoliciesFrame::refresh() {
  QRadioButton *button;
  button = static_cast<QRadioButton *>(js_popup->find(
  		policies->window_open));
  if (button != 0) button->setChecked(true);
  button = static_cast<QRadioButton *>(js_resize->find(
  		policies->window_resize));
  if (button != 0) button->setChecked(true);
  button = static_cast<QRadioButton *>(js_move->find(
  		policies->window_move));
  if (button != 0) button->setChecked(true);
  button = static_cast<QRadioButton *>(js_focus->find(
  		policies->window_focus));
  if (button != 0) button->setChecked(true);
  button = static_cast<QRadioButton *>(js_statusbar->find(
  		policies->window_status));
  if (button != 0) button->setChecked(true);
}

void JSPoliciesFrame::setWindowOpenPolicy(int id) {
  policies->window_open = id;
  emit changed();
}

void JSPoliciesFrame::setWindowResizePolicy(int id) {
  policies->window_resize = id;
  emit changed();
}

void JSPoliciesFrame::setWindowMovePolicy(int id) {
  policies->window_move = id;
  emit changed();
}

void JSPoliciesFrame::setWindowFocusPolicy(int id) {
  policies->window_focus = id;
  emit changed();
}

void JSPoliciesFrame::setWindowStatusPolicy(int id) {
  policies->window_status = id;
  emit changed();
}

#include "jspolicies.moc"
