/*
  Copyright (c) 2002 Leo Savernik <l.savernik@aon.at>
  Derived from jsopts.h and javaopts.h, code copied from there is
  copyrighted to its respective owners.

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

#ifndef __DOMAINLISTVIEW_H__
#define __DOMAINLISTVIEW_H__

#include <q3groupbox.h>
#include <QMap>
#include <kconfig.h>

class Q3ListViewItem;
class QPushButton;
class QStringList;

class K3ListView;

class Policies;
class PolicyDialog;

/**
 * @short Provides a list view of domains which policies are attached to.
 *
 * This class resembles a list view of domain names and some buttons to
 * manipulate it. You should use this widget if you need to manage domains
 * whose policies are described by (derivatives of) Policies objects.
 *
 * The contained widgets can be accessed by respective getters for
 * fine-tuning/customizing them afterwards.
 *
 * To use this class you have to derive your own and implement most
 * (all) of the protected methods. You need these to customize this widget
 * for its special purpose.
 *
 * @author Leo Savernik
 */
class DomainListView : public Q3GroupBox {
  Q_OBJECT
public:
  /** Enumerates the available buttons.
    */
  enum PushButton {
    AddButton, ChangeButton, DeleteButton, ImportButton, ExportButton
  };

  /**
   * constructor
   * @param config configuration to read from and to write to
   * @param title title to be used for enclosing group box
   * @param parent parent widget
   * @param name internal name for debugging
   */
  DomainListView(KSharedConfig::Ptr config,const QString &title,QWidget *parent );

  virtual ~DomainListView();

  /**
   * clears the list view.
   */
//  void clear();

  /**
   * returns the list view displaying the domains
   */
  K3ListView *listView() const { return domainSpecificLV; }

  /**
   * returns the add push-button.
   *
   * Note: The add button already contains a default "what's this" text.
   */
  QPushButton *addButton() const { return addDomainPB; }

  /**
   * returns the change push-button.
   *
   * Note: The change button already contains a default "what's this" text.
   */
  QPushButton *changeButton() const { return changeDomainPB; }

  /**
   * returns the delete push-button.
   *
   * Note: The delete button already contains a default "what's this" text.
   */
  QPushButton *deleteButton() const { return deleteDomainPB; }

  /**
   * returns the import push-button.
   */
  QPushButton *importButton() const { return importDomainPB; }

  /**
   * returns the export push-button.
   */
  QPushButton *exportButton() const { return exportDomainPB; }

  /**
   * Initializes the list view with the given list of domains as well
   * as the domain policy map.
   *
   * This method may be called multiple times on a DomainListView instance.
   *
   * @param domainList given list of domains
   */
  void initialize(const QStringList &domainList);

  /**
   * saves the current state of all domains to the configuration object.
   * @param group the group the information is to be saved under
   * @param domainListKey the name of the key which the list of domains
   *	is stored under.
   */
  void save(const QString &group, const QString &domainListKey);


Q_SIGNALS:
  /**
   * indicates that a configuration has been changed within this list view.
   * @param state true if changed, false if not
   */
  void changed(bool state);

protected:
  /**
   * factory method for creating a new domain-specific policies object.
   *
   * Example:
   * <pre>
   * JavaPolicies *JavaDomainListView::createPolicies() {
   *   return new JavaPolicies(m_pConfig,m_groupname,false);
   * }
   * </pre>
   */
  virtual Policies *createPolicies() = 0;

  /**
   * factory method for copying a policies object.
   *
   * Derived classes must interpret the given object as the same type
   * as those created by createPolicies and return a copy of this very type.
   *
   * Example:
   * <pre>
   * JavaPolicies *JavaDomainListView::copyPolicies(Policies *pol) {
   *   return new JavaPolicies(*static_cast<JavaPolicies *>(pol));
   * }
   * </pre>
   * @param pol policies object to be copied
   */
  virtual Policies *copyPolicies(Policies *pol) = 0;

  /**
   * allows derived classes to customize the policy dialog.
   *
   * The default implementation does nothing.
   * @param trigger triggered by which button
   * @param pDlg reference to policy dialog
   * @param copy policies object this dialog is used for changing. Derived
   *	classes can safely cast the @p copy object to the same type they
   *	returned in their createPolicies implementation.
   */
  virtual void setupPolicyDlg(PushButton trigger,PolicyDialog &pDlg,
  		Policies *copy);

private Q_SLOTS:
  void addPressed();
  void changePressed();
  void deletePressed();
  void importPressed();
  void exportPressed();
    void updateButton();

protected:

  KSharedConfig::Ptr config;

  K3ListView *domainSpecificLV;

  QPushButton* addDomainPB;
  QPushButton* changeDomainPB;
  QPushButton* deleteDomainPB;
  QPushButton* importDomainPB;
  QPushButton* exportDomainPB;

  typedef QMap<Q3ListViewItem*, Policies *> DomainPolicyMap;
  DomainPolicyMap domainPolicies;
};

#endif		// __DOMAINLISTVIEW_H__

