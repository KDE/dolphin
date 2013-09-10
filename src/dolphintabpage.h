/***************************************************************************
 * Copyright (C) 2013 by Emmanuel Pescosta <emmanuelpescosta099@gmail.com> *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA            *
 ***************************************************************************/

#ifndef DOLPHIN_TAB_PAGE_H
#define DOLPHIN_TAB_PAGE_H

#include <QWidget>
#include <KUrl>

class DolphinViewContainer;
class QVBoxLayout;
class QSplitter;
class KConfigGroup;
class KIcon;

class DolphinTabPage : public QWidget
{
    Q_OBJECT

public:
    enum ViewType
    {
        NoView = -1,
        PrimaryView,
        SecondaryView
    };

    DolphinTabPage(const KUrl& url, QWidget* parent = 0);

    KIcon tabIcon() const;
    QString tabText() const;

    /**
     * Sets the view with the view type \type as ative view. If split view isn't
     * active, it always sets the primary view as active view.
     */
    void setActiveView(const ViewType type);

    /**
     * Returns the ViewType of the active view. If split view isn't active, it always
     * returns PrimaryView.
     */
    ViewType activeView() const;

    /**
     * Enables or disables the split view mode.
     * If \a enabled is true, it creates a secondary view with the url of the primary view
     * or with the given \a secondaryUrl, if the \a secondary is valid.
     * If \a enabled is false, it closes the active view.
     */
    void setSplitViewEnabled(const bool enabled, const KUrl& secondaryUrl = KUrl());

    /**
     * Returns true, if the split view is enabled. The active view constainer can be
     * accessed by DolphinTabPage::activeViewContainer().
     */
    bool isSplitViewEnabled() const;

    /**
     * Switches between one and two views:
     * If only one view is visible, it creates a new secondary url, with the url
     * of the primary view. If there are already two views visible,
     * the active view gets closed.
     */
    void toggleSplitViewEnabled();

    /**
     * Returns the DolphinViewContainer for the given ViewType \a type. If split view isn't
     * active, it always returns the PrimaryView view container.
     */
    DolphinViewContainer* viewContainer(const ViewType type) const;

    /**
     * Returns the DolphinViewContainer of the active view.
     */
    DolphinViewContainer* activeViewContainer() const;

    /**
     * Reloads the primary view + the secondary view if split view is enabled.
     */
    void reload() const;

    /**
     * Refresh the primary view + the secondary view if split view is enabled.
     */
    void refresh() const;

    void saveProperties(const int tabIndex, KConfigGroup& group) const;
    void readProperties(const int tabIndex, const KConfigGroup& group);

signals:
    void activeViewChanged();
    void tabPropertiesChanged(DolphinTabPage* tabPage);

private slots:
    /**
     * Handles the view activated event.
     * It sets the previous active view to inactive, updates the current
     * active view type and triggers the activeViewChanged event.
     */
    void slotViewActivated();

    /**
     * Handles the urlChanged() signal from the primary and secondary view. If the
     * url of the active view has changed, it triggers the tabUrlChanged() signal.
     */
    void slotViewUrlChanged(const KUrl& url);

private:
    /**
     * Creates a view container and does a default initialization.
     */
    DolphinViewContainer* createViewContainer(const KUrl& url);

    /**
     * Helper method for saveProperties() and readProperties(): Returns
     * the property string for a tab with the index \a tabIndex and
     * the property \a property.
     */
    static QString tabProperty(const QString& property, const int tabIndex);

    QString squeezedText(const QString& text) const;

private:
    QVBoxLayout* m_layout;
    QSplitter* m_splitter;
    
    ViewType m_activeView;
    bool m_splitViewEnabled;
};

#endif