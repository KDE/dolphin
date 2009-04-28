/*
   This file is part of the Nepomuk KDE project.
   Copyright (C) 2007 Sebastian Trueg <trueg@kde.org>

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

#include "resourcetaggingwidget.h"
#include "tagcloud.h"
#include "taggingpopup.h"
#include "nepomukmassupdatejob.h"

#include <QtGui/QVBoxLayout>
#include <QtGui/QContextMenuEvent>
#include <QtGui/QCursor>
#include <QtGui/QLabel>
#include <QtCore/QSet>

#include <KLocale>

namespace Nepomuk {
    inline uint qHash( const Tag& res )
    {
        return qHash( res.resourceUri().toString() );
    }
}


class Nepomuk::ResourceTaggingWidget::Private
{
public:
    QList<Nepomuk::Resource> resources;

    TagCloud* resourceTagCloud;
    TaggingPopup* popup;

    QList<Tag> resourceTags;

    bool tagsChanged;

    void showTaggingPopup( const QPoint& );
    void updateResources();
    void _k_slotShowTaggingPopup();
    void _k_metadataUpdateDone();
    static QList<Tag> intersectTags( const QList<Resource>& );

    ResourceTaggingWidget* q;
};


void Nepomuk::ResourceTaggingWidget::Private::showTaggingPopup( const QPoint& pos )
{
    popup->showAllTags();
    resourceTags = intersectTags( resources );
    Q_FOREACH( const Tag &tag, resourceTags ) {
        popup->setTagSelected( tag, true );
    }

    tagsChanged = false;
    popup->exec( pos );

    if( tagsChanged ) {
        updateResources();
    }

    resourceTagCloud->showTags( resourceTags );
}


void Nepomuk::ResourceTaggingWidget::Private::updateResources()
{
    MassUpdateJob* job = MassUpdateJob::tagResources( resources, resourceTags );
    connect( job, SIGNAL( result( KJob* ) ),
             q, SLOT( _k_metadataUpdateDone() ) );
    q->setEnabled( false ); // no updates during execution
    job->start();
}


void Nepomuk::ResourceTaggingWidget::Private::_k_slotShowTaggingPopup()
{
    showTaggingPopup( QCursor::pos() );
}


void Nepomuk::ResourceTaggingWidget::Private::_k_metadataUpdateDone()
{
    q->setEnabled( true );
}


QList<Nepomuk::Tag> Nepomuk::ResourceTaggingWidget::Private::intersectTags( const QList<Resource>& res )
{
    if ( res.count() == 1 ) {
        return res.first().tags();
    }
    else if ( !res.isEmpty() ) {
        // determine the tags used for all resources
        QSet<Tag> tags = QSet<Tag>::fromList( res.first().tags() );
        QList<Resource>::const_iterator it = res.begin();
        for ( ++it; it != res.end(); ++it ) {
            tags.intersect( QSet<Tag>::fromList( (*it).tags() ) );
        }
        return tags.values();
    }
    else {
        return QList<Tag>();
    }
}


Nepomuk::ResourceTaggingWidget::ResourceTaggingWidget( QWidget* parent )
    : QWidget( parent ),
      d( new Private() )
{
    d->q = this;

    QVBoxLayout* layout = new QVBoxLayout( this );
    layout->setMargin( 0 );
    d->resourceTagCloud = new TagCloud( this );
    layout->addWidget( d->resourceTagCloud );
    QLabel* changeTagsLabel = new QLabel( "<p align=center><a style=\"font-size:small;\" href=\"dummy\">" + i18nc( "@label", "Change Tags..." ) + "</a>", this );
    connect( changeTagsLabel, SIGNAL( linkActivated( const QString ) ),
             this, SLOT( _k_slotShowTaggingPopup() ) );
    layout->addWidget( changeTagsLabel );

    // the popup tag cloud
    d->popup = new TaggingPopup;
    d->popup->setSelectionEnabled( true );
    d->popup->setNewTagButtonEnabled( true );

    connect( d->popup, SIGNAL( tagToggled( const Nepomuk::Tag&, bool ) ),
             this, SLOT( slotTagToggled( const Nepomuk::Tag&, bool ) ) );
    connect( d->popup, SIGNAL( tagAdded( const Nepomuk::Tag& ) ),
             this, SLOT( slotTagAdded( const Nepomuk::Tag& ) ) );

    connect( d->resourceTagCloud, SIGNAL( tagClicked( const Nepomuk::Tag& ) ),
             this, SIGNAL( tagClicked( const Nepomuk::Tag& ) ) );
}


Nepomuk::ResourceTaggingWidget::~ResourceTaggingWidget()
{
    delete d->popup;
    delete d;
}


void Nepomuk::ResourceTaggingWidget::setResource( const Nepomuk::Resource& res )
{
    setResources( QList<Resource>() << res );
}


void Nepomuk::ResourceTaggingWidget::setResources( const QList<Nepomuk::Resource>& resList )
{
    d->resources = resList;
    d->resourceTagCloud->showTags( d->intersectTags( resList ) );
}


void Nepomuk::ResourceTaggingWidget::slotTagToggled( const Nepomuk::Tag& tag, bool enabled )
{
    if ( enabled ) {
        d->resourceTags.append( tag );
    }
    else {
        d->resourceTags.removeAll( tag );
    }
    d->tagsChanged = true;
    d->popup->hide();
}


void Nepomuk::ResourceTaggingWidget::slotTagAdded( const Nepomuk::Tag& tag )
{
    // assign it right away
    d->resourceTags.append( tag );
    d->updateResources();
}


void Nepomuk::ResourceTaggingWidget::contextMenuEvent( QContextMenuEvent* e )
{
    d->showTaggingPopup( e->globalPos() );
}


void Nepomuk::ResourceTaggingWidget::showTagPopup( const QPoint& pos )
{
    d->showTaggingPopup( pos );
}

#include "resourcetaggingwidget.moc"
