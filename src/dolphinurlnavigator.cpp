/*
 * Copyright 2020  Felix Ernst <fe.a.ernst@gmail.com>
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) version 3, or any
 * later version accepted by the membership of KDE e.V. (or its
 * successor approved by the membership of KDE e.V.), which shall
 * act as a proxy defined in Section 6 of version 3 of the license.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "dolphinurlnavigator.h"

#include "dolphin_generalsettings.h"
#include "dolphinplacesmodelsingleton.h"
#include "global.h"

#include <KToggleAction>
#include <KUrlComboBox>
#include <KLocalizedString>

#include <QLineEdit>
#include <QMenu>

DolphinUrlNavigator::DolphinUrlNavigator(QWidget *parent) :
    KUrlNavigator(DolphinPlacesModelSingleton::instance().placesModel(), QUrl(), parent)
{
    init();
}

DolphinUrlNavigator::DolphinUrlNavigator(const QUrl &url, QWidget *parent) :
    KUrlNavigator(DolphinPlacesModelSingleton::instance().placesModel(), url, parent)
{
    init();
}

void DolphinUrlNavigator::init()
{
    const GeneralSettings* settings = GeneralSettings::self();
    setUrlEditable(settings->editableUrl());
    setShowFullPath(settings->showFullPath());
    setHomeUrl(Dolphin::homeUrl());
    setPlacesSelectorVisible(s_placesSelectorVisible);
    editor()->setCompletionMode(KCompletion::CompletionMode(settings->urlCompletionMode()));
    editor()->lineEdit()->installEventFilter(this);
    installEventFilter(this);
    setWhatsThis(xi18nc("@info:whatsthis location bar",
        "<para>This line describes the location of the files and folders "
        "displayed below.</para><para>The name of the currently viewed "
        "folder can be read at the very right. To the left of it is the "
        "name of the folder that contains it. The whole line is called "
        "the <emphasis>path</emphasis> to the current location because "
        "following these folders from left to right leads here.</para>"
        "<para>This interactive path "
        "is more powerful than one would expect. To learn more "
        "about the basic and advanced features of the location bar "
        "<link url='help:/dolphin/location-bar.html'>click here</link>. "
        "This will open the dedicated page in the Handbook.</para>"));

    s_instances.push_front(this);

    connect(this, &DolphinUrlNavigator::returnPressed,
            this, &DolphinUrlNavigator::slotReturnPressed);
    connect(editor(), &KUrlComboBox::completionModeChanged,
            this, DolphinUrlNavigator::setCompletionMode);
}

DolphinUrlNavigator::~DolphinUrlNavigator()
{
    s_instances.remove(this);
}

bool DolphinUrlNavigator::eventFilter(QObject* watched, QEvent* event)
{
    Q_UNUSED(watched)
    if (event->type() == QEvent::ChildPolished) {
        QChildEvent *childEvent = static_cast<QChildEvent *>(event);
        QMenu *popup = qobject_cast<QMenu *>(childEvent->child());
        if (popup) {
            // The popups of the "breadcrumb mode" navigation buttons
            // should not get the action added. They can currently be
            // identified by their number of separators: 0 or 1
            // The popups we are interested in have 2 or more separators.
            int separatorCount = 0;
            for (QAction *action : popup->actions()) {
                if (action->isSeparator()) {
                    separatorCount++;
                }
            }
            if (separatorCount > 1) {
                q_check_ptr(s_ActionForContextMenu);
                popup->addAction(s_ActionForContextMenu);
            }
        }
    }
    return false;
}

void DolphinUrlNavigator::slotReadSettings()
{
    // The startup settings should (only) get applied if they have been
    // modified by the user. Otherwise keep the (possibly) different current
    // settings of the URL navigators and split view.
    if (GeneralSettings::modifiedStartupSettings()) {
        for (DolphinUrlNavigator *urlNavigator : s_instances) {
            urlNavigator->setUrlEditable(GeneralSettings::editableUrl());
            urlNavigator->setShowFullPath(GeneralSettings::showFullPath());
            urlNavigator->setHomeUrl(Dolphin::homeUrl());
        }
    }
}

void DolphinUrlNavigator::slotReturnPressed()
{
    if (!GeneralSettings::editableUrl()) {
        setUrlEditable(false);
    }
}

void DolphinUrlNavigator::addToContextMenu(QAction* action)
{
    s_ActionForContextMenu = action;
}


void DolphinUrlNavigator::slotPlacesPanelVisibilityChanged(bool visible)
{
    // The places-selector from the URL navigator should only be shown
    // if the places dock is invisible
    s_placesSelectorVisible = !visible;

    for (DolphinUrlNavigator *urlNavigator : s_instances) {
        urlNavigator->setPlacesSelectorVisible(s_placesSelectorVisible);
    }
}

void DolphinUrlNavigator::setCompletionMode(const KCompletion::CompletionMode completionMode)
{
    if (completionMode != GeneralSettings::urlCompletionMode())
    {
        GeneralSettings::setUrlCompletionMode(completionMode);
        for (const DolphinUrlNavigator *urlNavigator : s_instances)
        {
            urlNavigator->editor()->setCompletionMode(completionMode);
        }
    }
}

std::forward_list<DolphinUrlNavigator *> DolphinUrlNavigator::s_instances;
bool DolphinUrlNavigator::s_placesSelectorVisible = true;
QAction *DolphinUrlNavigator::s_ActionForContextMenu = nullptr;
