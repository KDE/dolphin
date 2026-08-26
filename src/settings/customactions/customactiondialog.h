/*
 * SPDX-FileCopyrightText: 2026 KsmBL <katzen.sind.lecker69@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef CUSTOMACTIONDIALOG_H
#define CUSTOMACTIONDIALOG_H

#include "customactions.h"

#include <QDialog>

class KIconButton;
class QLineEdit;
class QPushButton;

/**
 * @brief Edits a single custom context menu entry.
 */
class CustomActionDialog : public QDialog
{
    Q_OBJECT

public:
    CustomActionDialog(QWidget *parent, CustomActions::Target target, const CustomActions::Entry &entry = {});

    CustomActions::Entry entry() const;

private:
    void updateOkButton();

    /** Asks for an executable or .desktop file and puts it in the command field. */
    void browseForApplication();

    QLineEdit *m_name;
    QLineEdit *m_command;
    KIconButton *m_icon;
    QPushButton *m_okButton;
};

#endif
