/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 2020 Felix Ernst <fe.a.ernst@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "dolphinurlnavigator.h"

#include "dolphin_generalsettings.h"
#include "dolphinplacesmodelsingleton.h"
#include "global.h"

#include <KUrlComboBox>
#include <KLocalizedString>

#include <QAbstractButton>
#include <QLayout>
#include <QLineEdit>

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
    setWhatsThis(xi18nc("@info:whatsthis location bar",
        "<para>This describes the location of the files and folders "
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

QSize DolphinUrlNavigator::sizeHint() const
{
    // Change sizeHint() in KUrlNavigator instead.
    if (isUrlEditable()) {
        return editor()->lineEdit()->sizeHint();
    }
    int widthHint = 0;
    for (int i = 0; i < layout()->count(); ++i) {
        QWidget *widget = layout()->itemAt(i)->widget();
        const QAbstractButton *button = qobject_cast<QAbstractButton *>(widget);
        if (button && button->icon().isNull()) {
            widthHint += widget->minimumSizeHint().width();
        }
    }
    return QSize(widthHint, KUrlNavigator::sizeHint().height());
}

std::unique_ptr<DolphinUrlNavigator::VisualState> DolphinUrlNavigator::visualState() const
{
    std::unique_ptr<VisualState> visualState{new VisualState};
    visualState->isUrlEditable = (isUrlEditable());
    const QLineEdit *lineEdit = editor()->lineEdit();
    visualState->hasFocus = lineEdit->hasFocus();
    visualState->text = lineEdit->text();
    visualState->cursorPosition = lineEdit->cursorPosition();
    visualState->selectionStart = lineEdit->selectionStart();
    visualState->selectionLength = lineEdit->selectionLength();
    return visualState;
}

void DolphinUrlNavigator::setVisualState(const VisualState& visualState)
{
    setUrlEditable(visualState.isUrlEditable);
    if (!visualState.isUrlEditable) {
        return;
    }
    editor()->lineEdit()->setText(visualState.text);
    if (visualState.hasFocus) {
        editor()->lineEdit()->setFocus();
        editor()->lineEdit()->setCursorPosition(visualState.cursorPosition);
        if (visualState.selectionStart != -1) {
            editor()->lineEdit()->setSelection(visualState.selectionStart, visualState.selectionLength);
        }
    }
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
