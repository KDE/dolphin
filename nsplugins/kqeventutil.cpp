// -*- c++ -*-
/* This file is part of the KDE project
 *
 * Copyright (C) 2000 Wynn Wilkes <wynnw@caldera.com>
 *               2002 Till Krech <till@snafu.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include <qevent.h>
#include "kqeventutil.h"

QString KQEventUtil::getQtEventName( QEvent* e )
{
    QString s;

    switch( e->type() )
    {
        case QEvent::None:
            s = "None";
            break;
        case QEvent::Timer:
            s = "Timer";
            break;
        case QEvent::MouseButtonPress:
            s = "MouseButtonPress";
            break;
        case QEvent::MouseButtonRelease:
            s = "MouseButtonRelease";
            break;
        case QEvent::MouseButtonDblClick:
            s = "MouseButtonClick";
            break;
        case QEvent::MouseMove:
            s = "MouseMove";
            break;
        case QEvent::KeyPress:
            s = "KeyPress";
            break;
        case QEvent::KeyRelease:
            s = "KeyRelease";
            break;
        case QEvent::FocusIn:
            s = "FocusIn";
            break;
        case QEvent::FocusOut:
            s = "FocusOut";
            break;
        case QEvent::Enter:
            s = "Enter";
            break;
        case QEvent::Leave:
            s = "Leave";
            break;
        case QEvent::Paint:
            s = "Paint";
            break;
        case QEvent::Move:
            s = "Move";
            break;
        case QEvent::Resize:
            s = "Resize";
            break;
        case QEvent::Create:
            s = "Create";
            break;
        case QEvent::Destroy:
            s = "Destroy";
            break;
        case QEvent::Show:
            s = "Show";
            break;
        case QEvent::Hide:
            s = "Hide";
            break;
        case QEvent::Close:
            s = "Close";
            break;
        case QEvent::Quit:
            s = "Quit";
            break;
        case QEvent::Reparent:
            s = "Reparent";
            break;
        case QEvent::ShowMinimized:
            s = "ShowMinimized";
            break;
        case QEvent::ShowNormal:
            s = "ShowNormal";
            break;
        case QEvent::WindowActivate:
            s = "WindowActivate";
            break;
        case QEvent::WindowDeactivate:
            s = "WindowDeactivate";
            break;
        case QEvent::ShowToParent:
            s = "ShowToParent";
            break;
        case QEvent::HideToParent:
            s = "HideToParent";
            break;
        case QEvent::ShowMaximized:
            s = "ShowMaximized";
            break;
        case QEvent::Accel:
            s = "Accel";
            break;
        case QEvent::Wheel:
            s = "Wheel";
            break;
        case QEvent::AccelAvailable:
            s = "AccelAvailable";
            break;
        case QEvent::CaptionChange:
            s = "CaptionChange";
            break;
        case QEvent::IconChange:
            s = "IconChange";
            break;
        case QEvent::ParentFontChange:
            s = "ParentFontChange";
            break;
        case QEvent::ApplicationFontChange:
            s = "ApplicationFontChange";
            break;
        case QEvent::ParentPaletteChange:
            s = "ParentPaletteChange";
            break;
        case QEvent::ApplicationPaletteChange:
            s = "ApplicationPaletteChange";
            break;
        case QEvent::Clipboard:
            s = "Clipboard";
            break;
        case QEvent::Speech:
            s = "Speech";
            break;
        case QEvent::SockAct:
            s = "SockAct";
            break;
        case QEvent::AccelOverride:
            s = "AccelOverride";
            break;
        case QEvent::DragEnter:
            s = "DragEnter";
            break;
        case QEvent::DragMove:
            s = "DragMove";
            break;
        case QEvent::DragLeave:
            s = "DragLeave";
            break;
        case QEvent::Drop:
            s = "Drop";
            break;
        case QEvent::DragResponse:
            s = "DragResponse";
            break;
        case QEvent::ChildInserted:
            s = "ChildInserted";
            break;
        case QEvent::ChildRemoved:
            s = "ChildRemoved";
            break;
        case QEvent::LayoutHint:
            s = "LayoutHint";
            break;
        case QEvent::ShowWindowRequest:
            s = "ShowWindowRequest";
            break;
        case QEvent::ActivateControl:
            s = "ActivateControl";
            break;
        case QEvent::DeactivateControl:
            s = "DeactivateControl";
            break;
        case QEvent::User:
            s = "User Event";
            break;

        default:
            s = "Undefined Event, value = " + QString::number( e->type() );
            break;
    }

    return s;
}
