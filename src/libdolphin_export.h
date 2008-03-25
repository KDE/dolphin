/*
   This file is part of the KDE project
   Copyright (C) 2007 Montel Laurent <montel@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef LIBDOLPHIN_EXPORT_H
#define LIBDOLPHIN_EXPORT_H

/* needed for KDE_EXPORT macros */
#include <kdemacros.h>

/* needed, because e.g. Q_OS_UNIX is so frequently used */
#include <QtCore/QBool>

#ifndef LIBDOLPHINPRIVATE_EXPORT
# if defined(MAKE_DOLPHINPRIVATE_LIB)
   /* We are building this library */
#  define LIBDOLPHINPRIVATE_EXPORT KDE_EXPORT
# else
   /* We are using this library */
#  define LIBDOLPHINPRIVATE_EXPORT KDE_IMPORT
# endif
#endif

#endif
