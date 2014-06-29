/***************************************************************************
 *   Copyright (C) 2010 by Peter Penz <peter.penz19@gmail.com>             *
 *   Copyright (C) 2008 by Fredrik Höglund <fredrik@kde.org>               *
 *   Copyright (C) 2012 by Mark Gaiser <markg85@gmail.com>                 *
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

#include "filemetadatatooltip.h"

#include <KColorScheme>
#include <KSeparator>
#include <KWindowSystem>
// For the blurred tooltip background
#include <KWindowEffects>
#include <KStringHandler>

#include <QLabel>
#include <QStyleOptionFrame>
#include <QStylePainter>
#include <QVBoxLayout>
#include <QTextDocument>
#include <QTextLayout>
#include <QTextLine>

#ifndef HAVE_BALOO
#include <KFileMetaDataWidget>
#else
#include <Baloo/FileMetaDataWidget>
#endif

FileMetaDataToolTip::FileMetaDataToolTip(QWidget* parent) :
    QWidget(parent),
    m_preview(0),
    m_name(0),
    m_fileMetaDataWidget(0)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowFlags(Qt::ToolTip | Qt::FramelessWindowHint);

    // Create widget for file preview
    m_preview = new QLabel(this);
    m_preview->setAlignment(Qt::AlignTop);

    // Create widget for file name
    m_name = new QLabel(this);
    m_name->setForegroundRole(QPalette::ToolTipText);
    m_name->setTextFormat(Qt::PlainText);
    m_name->setAlignment(Qt::AlignHCenter);

    QFont font = m_name->font();
    font.setBold(true);
    m_name->setFont(font);

    QFontMetrics fontMetrics(font);
    m_name->setMaximumWidth(fontMetrics.averageCharWidth() * 40);

    // Create widget for the meta data
#ifndef HAVE_BALOO
    m_fileMetaDataWidget = new KFileMetaDataWidget(this);
    connect(m_fileMetaDataWidget, &KFileMetaDataWidget::metaDataRequestFinished,
            this, &FileMetaDataToolTip::metaDataRequestFinished);
#else
    m_fileMetaDataWidget = new Baloo::FileMetaDataWidget(this);
    connect(m_fileMetaDataWidget, &Baloo::FileMetaDataWidget::metaDataRequestFinished,
            this, &FileMetaDataToolTip::metaDataRequestFinished);
#endif
    m_fileMetaDataWidget->setForegroundRole(QPalette::ToolTipText);
    m_fileMetaDataWidget->setReadOnly(true);

    QVBoxLayout* textLayout = new QVBoxLayout();
    textLayout->addWidget(m_name);
    textLayout->addWidget(new KSeparator());
    textLayout->addWidget(m_fileMetaDataWidget);
    textLayout->setAlignment(m_name, Qt::AlignCenter);
    textLayout->setAlignment(m_fileMetaDataWidget, Qt::AlignLeft);
    // Assure that the text-layout gets top-aligned by adding a stretch.
    // Don't use textLayout->setAlignment(Qt::AlignTop) instead, as this does
    // not work with the heightForWidth()-size-hint of m_fileMetaDataWidget
    // (see bug #241608)
    textLayout->addStretch();

    QHBoxLayout* tipLayout = new QHBoxLayout(this);
    tipLayout->addWidget(m_preview);
    tipLayout->addSpacing(tipLayout->margin());
    tipLayout->addLayout(textLayout);
}

FileMetaDataToolTip::~FileMetaDataToolTip()
{
}

void FileMetaDataToolTip::setPreview(const QPixmap& pixmap)
{
    m_preview->setPixmap(pixmap);
}

QPixmap FileMetaDataToolTip::preview() const
{
    if (m_preview->pixmap()) {
        return *m_preview->pixmap();
    }
    return QPixmap();
}

void FileMetaDataToolTip::setName(const QString& name)
{
    QTextOption textOption;
    textOption.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);

    const QString processedName = Qt::mightBeRichText(name) ? name : KStringHandler::preProcessWrap(name);

    QTextLayout textLayout(processedName);
    textLayout.setFont(m_name->font());
    textLayout.setTextOption(textOption);

    QString wrappedText;
    wrappedText.reserve(processedName.length());

    // wrap the text to fit into the maximum width of m_name
    textLayout.beginLayout();
    QTextLine line = textLayout.createLine();
    while (line.isValid()) {
        line.setLineWidth(m_name->maximumWidth());
        wrappedText += processedName.mid(line.textStart(), line.textLength());

        line = textLayout.createLine();
        if (line.isValid()) {
            wrappedText += QChar::LineSeparator;
        }
    }
    textLayout.endLayout();

    m_name->setText(wrappedText);
}

QString FileMetaDataToolTip::name() const
{
    return m_name->text();
}

void FileMetaDataToolTip::setItems(const KFileItemList& items)
{
    m_fileMetaDataWidget->setItems(items);
}

KFileItemList FileMetaDataToolTip::items() const
{
    return m_fileMetaDataWidget->items();
}

void FileMetaDataToolTip::paintEvent(QPaintEvent* event)
{
    QStylePainter painter(this);
    QStyleOptionFrame option;
    option.init(this);
    painter.drawPrimitive(QStyle::PE_PanelTipLabel, option);
    painter.end();

    QWidget::paintEvent(event);
}

void FileMetaDataToolTip::showEvent(QShowEvent *)
{
#pragma message("TODO: port Plasma::WindowEffects::overrideShadow")
    //Plasma::WindowEffects::overrideShadow(winId(), true);
    KWindowEffects::enableBlurBehind(winId(), true, mask());
}

