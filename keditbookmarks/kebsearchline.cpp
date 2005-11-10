/* This file is part of the KDE project
   Copyright (C) 2005 Daniel Teske <teske@squorn.de>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License version 2 as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "kebsearchline.h"

#include <QHeaderView>
#include <QHBoxLayout>



KEBSearchLine::KEBSearchLine(QWidget *parent, KListView *listView)
    : KListViewSearchLine(parent, listView)
{
    mmode = AND;
}

KEBSearchLine::KEBSearchLine(QWidget *parent)
    :KListViewSearchLine(parent)
{
    mmode = AND;
}

KEBSearchLine::~KEBSearchLine()
{
}

bool KEBSearchLine::itemMatches(const Q3ListViewItem *item, const QString &s) const
{
    if(mmode == EXACTLY)
       return KListViewSearchLine::itemMatches(item, s);

    if(lastpattern != s)
    {
       splitted = QStringList::split(QChar(' '), s);
       lastpattern = s;
    }

    QStringList::const_iterator it = splitted.begin();
    QStringList::const_iterator end = splitted.end();

    if(mmode == OR)
    {
       if(it == end) //Nothing to match
           return true;
       for( ; it != end; ++it)
           if(KListViewSearchLine::itemMatches(item, *it))
               return true;
    }
    else if(mmode == AND)
       for( ; it != end; ++it)
           if(! KListViewSearchLine::itemMatches(item, *it))
               return false;

    return (mmode == AND);
}

KEBSearchLine::modes KEBSearchLine::mode()
{
    return mmode;
}

void KEBSearchLine::setMode(modes m)
{
    mmode = m;
}

////////////////////////////////////////////////////////////////////////////////
// KViewSearchLine
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// public methods
////////////////////////////////////////////////////////////////////////////////
class KViewSearchLine::KViewSearchLinePrivate
{
public:
    KViewSearchLinePrivate() :
        listView(0),
        treeView(0),
        caseSensitive(false),
        activeSearch(false),
        keepParentsVisible(true),
        queuedSearches(0) {}

    QListView * listView;
    QTreeView * treeView;
    bool caseSensitive;
    bool activeSearch;
    bool keepParentsVisible;
    QString search;
    int queuedSearches;
    QLinkedList<int> searchColumns;
};


KViewSearchLine::KViewSearchLine(QWidget *parent, QAbstractItemView *v) :
    KLineEdit(parent)
{
    d = new KViewSearchLinePrivate;

    d->treeView = dynamic_cast<QTreeView *>(v);
    d->listView = dynamic_cast<QListView *>(v);

    connect(this, SIGNAL(textChanged(const QString &)),
            this, SLOT(queueSearch(const QString &)));

    if(view()) {
        connect(view(), SIGNAL(destroyed()),
                this, SLOT(listViewDeleted()));
        connect(model(), SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)),
                this, SLOT(slotDataChanged(const QModelIndex &, const QModelIndex &)));
        connect(model(), SIGNAL(rowsInserted(const QModelIndex &, int , int )),
                this, SLOT(slotRowsInserted(const QModelIndex &, int, int)));
        connect(model(), SIGNAL(rowsRemoved(const QModelIndex &, int , int )),
                this, SLOT(slotRowsRemoved(const QModelIndex &, int, int)));
        connect(model(), SIGNAL(columnsInserted(const QModelIndex &, int, int )),
                this, SLOT(slotColumnsInserted(const QModelIndex &, int, int )));
        connect(model(), SIGNAL(columnsRemoved(const QModelIndex &, int, int)),
                this, SLOT(slotColumnsRemoved(const QModelIndex &, int, int)));
        connect(model(), SIGNAL(modelReset()), this, SLOT(slotModelReset()));
    }
    else
        setEnabled(false);
}

KViewSearchLine::KViewSearchLine(QWidget *parent) :
    KLineEdit(parent)
{
    d = new KViewSearchLinePrivate;

    d->treeView = 0;
    d->listView = 0;

    connect(this, SIGNAL(textChanged(const QString &)),
            this, SLOT(queueSearch(const QString &)));

    setEnabled(false);
}

KViewSearchLine::~KViewSearchLine()
{
    delete d;
}

QAbstractItemView * KViewSearchLine::view() const
{
    if(d->treeView)
        return d->treeView;
    else
        return d->listView;
}

bool KViewSearchLine::caseSensitive() const
{
    return d->caseSensitive;
}

bool KViewSearchLine::keepParentsVisible() const
{
    return d->keepParentsVisible;
}

////////////////////////////////////////////////////////////////////////////////
// public slots
////////////////////////////////////////////////////////////////////////////////

void KViewSearchLine::updateSearch(const QString &s)
{
    if(!view())
        return;

    d->search = s.isNull() ? text() : s;

    // If there's a selected item that is visible, make sure that it's visible
    // when the search changes too (assuming that it still matches).
    //FIXME reimplement

    if(d->keepParentsVisible)
        checkItemParentsVisible(model()->index(0,0, QModelIndex()));
    else
        checkItemParentsNotVisible();
}

void KViewSearchLine::setCaseSensitive(bool cs)
{
    d->caseSensitive = cs;
}

void KViewSearchLine::setKeepParentsVisible(bool v)
{
    d->keepParentsVisible = v;
}

void KViewSearchLine::setSearchColumns(const QLinkedList<int> &columns)
{
    d->searchColumns = columns;
}

void KViewSearchLine::setView(QAbstractItemView *v)
{
    if(view()) {
        disconnect(view(), SIGNAL(destroyed()),
                   this, SLOT(listViewDeleted()));
        disconnect(model(), SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)),
                this, SLOT(slotDataChanged(const QModelIndex &, const QModelIndex &)));
        disconnect(model(), SIGNAL(rowsInserted(const QModelIndex &, int , int )),
                this, SLOT(slotRowsInserted(const QModelIndex &, int, int)));
        disconnect(model(), SIGNAL(rowsRemoved(const QModelIndex &, int , int )),
                this, SLOT(slotRowsRemoved(const QModelIndex &, int, int)));
        disconnect(model(), SIGNAL(columnsInserted(const QModelIndex &, int, int )),
                this, SLOT(slotColumnsInserted(const QModelIndex &, int, int )));
        disconnect(model(), SIGNAL(columnsRemoved(const QModelIndex &, int, int)),
                this, SLOT(slotColumnsRemoved(const QModelIndex &, int, int)));
        disconnect(model(), SIGNAL(modelReset()), this, SLOT(slotModelReset()));
    }

    d->treeView = dynamic_cast<QTreeView *>(v);
    d->listView = dynamic_cast<QListView *>(v);

    if(view()) {
        connect(view(), SIGNAL(destroyed()),
                this, SLOT(listViewDeleted()));

        connect(model(), SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)),
                this, SLOT(slotDataChanged(const QModelIndex &, const QModelIndex &)));
        connect(model(), SIGNAL(rowsInserted(const QModelIndex &, int , int )),
                this, SLOT(slotRowsInserted(const QModelIndex &, int, int)));
        connect(model(), SIGNAL(rowsRemoved(const QModelIndex &, int , int )),
                this, SLOT(slotRowsRemoved(const QModelIndex &, int, int)));
        connect(model(), SIGNAL(columnsInserted(const QModelIndex &, int, int )),
                this, SLOT(slotColumnsInserted(const QModelIndex &, int, int )));
        connect(model(), SIGNAL(columnsRemoved(const QModelIndex &, int, int)),
                this, SLOT(slotColumnsRemoved(const QModelIndex &, int, int)));
        connect(model(), SIGNAL(modelReset()), this, SLOT(slotModelReset()));

    }

    setEnabled(bool(view()));
}

////////////////////////////////////////////////////////////////////////////////
// protected members
////////////////////////////////////////////////////////////////////////////////

bool KViewSearchLine::itemMatches(const QModelIndex & item, const QString &s) const
{
    if(s.isEmpty())
        return true;

    // If the search column list is populated, search just the columns
    // specifified.  If it is empty default to searching all of the columns.
    if(d->treeView)
    {
        int columnCount = d->treeView->header()->count();
        int row = item.row();
        QModelIndex parent = item.parent();
        if(!d->searchColumns.isEmpty()) {
            QLinkedList<int>::const_iterator it, end;
            end = d->searchColumns.constEnd();
            for(it = d->searchColumns.constBegin(); it != end; ++it)
            {
                if(*it < columnCount)
                {
                    const QString & text = model()->data(parent.child(row, *it)).toString();
                    if(text.find(s, 0, d->caseSensitive) >= 0)
                        return true;
                }
            }
        }
        else {
            for(int i = 0; i < columnCount; i++) 
            {
                if(d->treeView->isColumnHidden(i) == false)
                {
                    const QString & text = model()->data(parent.child(row, i)).toString();
                    if(text.find(s, 0, d->caseSensitive) >= 0)
                        return true;
                }
            }
        }
        return false;
    }
    else
    {
        QString text = model()->data(item).toString();
        if(text.find(s, 0, d->caseSensitive) >= 0)
            return true;
        else
            return false;
    }
}

void KViewSearchLine::contextMenuEvent( QContextMenuEvent*e )
{
    qDeleteAll(actions);
    QMenu *popup = KLineEdit::createStandardContextMenu();
    if(d->treeView)
    {
        int columnCount = d->treeView->header()->count();
        actions.resize(columnCount + 1);
        if(columnCount)
        {
            QMenu *submenu = new QMenu(i18n("Search Columns"), popup);
            popup->addMenu(submenu);
            bool allVisibleColumsCheked = true;
            QAction * allVisibleAct = new QAction(i18n("All Visible Columns"), 0);
            allVisibleAct->setCheckable(true);
            submenu->addAction(allVisibleAct);
            submenu->addSeparator();
            for(int i=0; i<columnCount; ++i)
            {
                int logicalIndex = d->treeView->header()->logicalIndex(i);
                QString columnText = model()->headerData(logicalIndex, Qt::Horizontal).toString();
                if(columnText.isEmpty())
                    columnText = i18n("Column number %1","Column No. %1").arg(i);
                QAction * act = new QAction(columnText, 0); 
                act->setCheckable(true);
                if( d->searchColumns.isEmpty() || d->searchColumns.find(logicalIndex) != d->searchColumns.constEnd())
                    act->setChecked(true);
    
                actions[logicalIndex] = act;
                if( !d->treeView || (d->treeView->isColumnHidden(i) == false) )
                {
                    submenu->addAction(act);
                    allVisibleColumsCheked = allVisibleColumsCheked && act->isChecked();
                }
            }
            actions[columnCount] = allVisibleAct;
            if(d->searchColumns.isEmpty() || allVisibleColumsCheked)
            {
                allVisibleAct->setChecked(true);
                d->searchColumns.clear();
            }
            connect(submenu, SIGNAL(triggered ( QAction * )), this, SLOT(searchColumnsMenuActivated(QAction *)));
        }
    }
    popup->exec( e->globalPos() );
    delete popup;
}

////////////////////////////////////////////////////////////////////////////////
// protected slots
////////////////////////////////////////////////////////////////////////////////

void KViewSearchLine::queueSearch(const QString &search)
{
    d->queuedSearches++;
    d->search = search;
    QTimer::singleShot(200, this, SLOT(activateSearch()));
}

void KViewSearchLine::activateSearch()
{
    --(d->queuedSearches);

    if(d->queuedSearches == 0)
        updateSearch(d->search);
}

////////////////////////////////////////////////////////////////////////////////
// private slots
////////////////////////////////////////////////////////////////////////////////

void KViewSearchLine::listViewDeleted()
{
    d->treeView = 0;
    d->listView = 0;
    setEnabled(false);
}

void KViewSearchLine::searchColumnsMenuActivated(QAction * action)
{
    int index;
    int count = actions.count();
    for(int i=0; i<count; ++i)
    {
        if(action == actions[i])
        {
            index = i;
            break;
        }
    }
    int columnCount = d->treeView->header()->count();
    if(index == columnCount)
    {
        if(d->searchColumns.isEmpty()) //all columns was checked
            d->searchColumns.append(0);
        else
            d->searchColumns.clear();
    }
    else
    {
        QLinkedList<int>::iterator it = d->searchColumns.find(index);
        if(it != d->searchColumns.end())
            d->searchColumns.remove(it);
        else
        {
            if(d->searchColumns.isEmpty()) //all columns was checked
            {
                for(int i=0; i<columnCount; ++i)
                    if(i != index)
                        d->searchColumns.append(i);
            }
            else
                d->searchColumns.append(index);
        }
    }
    updateSearch();
}


void KViewSearchLine::slotRowsRemoved(const QModelIndex &parent, int, int)
{
    if(!d->keepParentsVisible)
        return;

    QModelIndex p = parent;
    while(p.isValid())
    {
        int count = model()->rowCount(p);
        if(count && anyVisible( model()->index(0,0, p), model()->index( count-1, 0, p)))
            return;
        if(itemMatches(p, d->search))
            return;
        setVisible(p, false);
        p = p.parent();
    }
}

void KViewSearchLine::slotColumnsInserted(const QModelIndex &, int, int )
{
    updateSearch();
}

void KViewSearchLine::slotColumnsRemoved(const QModelIndex &, int first, int last)
{
    if(d->treeView)
        updateSearch();
    else
    {
        if(d->listView->modelColumn() >= first && d->listView->modelColumn()<= last)
        {
            if(d->listView->modelColumn()>last)
                kdFatal()<<"Columns were removed, the modelColumn() doesn't exist anymore. "
                           "K4listViewSearchLine can't cope with that."<<endl;
            updateSearch();
        }
    }
}

void KViewSearchLine::slotModelReset()
{
    //FIXME Is there a way to ensure that the view
    // has already responded to the reset signal?
    updateSearch();
}

void KViewSearchLine::slotDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
    QModelIndex parent = topLeft.parent();
    int column = 0;
    if(d->listView)
        column = d->listView->modelColumn();
    bool match = recheck( model()->index(topLeft.row(), column, parent), model()->index(bottomRight.row(), column, parent));
    if(!d->keepParentsVisible)
        return;
    if(!parent.isValid()) // includes listview
        return;
    if(match)
    {
        QModelIndex p = parent;
        while(p.isValid())
        {
            setVisible(p, true);
            p = p.parent();
        }
    }
    else //no match => might need to hide parents (this is ugly)
    {
        if(isVisible(parent) == false) // parent is already hidden
                return;
        //parent is visible => implies all parents visible

        // first check if all of the unchanged rows are hidden
        match = false;
        if(topLeft.row() >= 1)
            match |= anyVisible( model()->index(0,0, parent), model()->index(topLeft.row()-1, 0, parent));
        int rowCount = model()->rowCount(parent);
        if(bottomRight.row() + 1 <= rowCount - 1)
            match |= anyVisible( model()->index(bottomRight.row()+1, 0, parent), model()->index(rowCount-1, 0, parent));
        if(!match) //all child rows hidden
        {
            if(itemMatches(parent, d->search))
                return;
            // and parent didn't match, hide it
            setVisible(parent, false);

            // need to check all the way up to root
            QModelIndex p = parent.parent();
            while(p.isValid())
            {
                //hide p if no children of p isVisible and it doesn't match
                int count = model()->rowCount(p);
                if(anyVisible( model()->index(0, 0, p), model()->index(count-1, 0, p)))
                    return;

                if(itemMatches(p, d->search))
                    return;
                setVisible(p, false);
                p = p.parent();
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
// private methods
////////////////////////////////////////////////////////////////////////////////
QAbstractItemModel * KViewSearchLine::model() const
{
    if(d->treeView)
        return d->treeView->model();
    else
        return d->listView->model();
}


bool KViewSearchLine::anyVisible(const QModelIndex & first, const QModelIndex & last)
{
    Q_ASSERT(d->treeView);
    QModelIndex parent = first.parent();
    QModelIndex index = first;
    while(true)
    {
        if( isVisible(index))
            return true;
        if(index == last)
            break;
        index = nextRow(index);
    }
    return false;
}

bool KViewSearchLine::isVisible(const QModelIndex & index)
{
    if(d->treeView)
        return !d->treeView->isRowHidden(index.row(), index.parent());
    else
        return d->listView->isRowHidden(index.row());
}

QModelIndex KViewSearchLine::nextRow(const QModelIndex & index)
{
    return model()->index(index.row()+1, index.column(), index.parent());
}

bool KViewSearchLine::recheck(const QModelIndex & first, const QModelIndex & last)
{
    bool visible = false;
    QModelIndex index = first;
    while(true)
    {
        int rowCount = model()->rowCount(index);
        if(d->keepParentsVisible && rowCount && anyVisible( index.child(0,0), index.child( rowCount-1, 0)))
        {
            visible = true;
        }
        else // no children visible
        {
            bool match = itemMatches(index, d->search);
            setVisible(index, match);
            visible = visible || match;
        }
        if(index == last)
            break;
        index = nextRow(index);
    }
    return visible;
}

void KViewSearchLine::slotRowsInserted(const QModelIndex &parent, int first, int last)
{
    bool visible = false;
    int column = 0;
    if(d->listView)
        column = d->listView->modelColumn();

    QModelIndex index = model()->index(first, column, parent);
    QModelIndex end = model()->index(last, column, parent);
    while(true)
    {
        if(itemMatches(index, d->search))
        {
            visible = true;
            setVisible(index, true);
        }
        else
            setVisible(index, false);
        if(index == end)
            break;
        index = nextRow(index);
    }

    if(!d->keepParentsVisible)
        return;
    if(visible)
    {
        QModelIndex p = parent;
        while(p.isValid())
        {
            setVisible(p, true);
            p = p.parent();
        }
    }
}

void KViewSearchLine::setVisible(QModelIndex index, bool v)
{
    if(d->treeView)
        d->treeView->setRowHidden(index.row(), index.parent(), !v);
    else
        d->listView->setRowHidden(index.row(), !v);
}

void KViewSearchLine::checkItemParentsNotVisible()
{
    int rowCount = model()->rowCount( QModelIndex() );
    int column = 0;
    if(d->listView)
        column = d->listView->modelColumn();
    for(int i = 0; i < rowCount; ++i) 
    {
        QModelIndex it = model()->index(i, column, QModelIndex());
        if(itemMatches(it, d->search))
            setVisible(it, true);
        else
            setVisible(it, false);
    }
}

/** Check whether \p index, its siblings and their descendents should be shown. Show or hide the items as necessary.
 *
 *  \p index  The list view item to start showing / hiding items at. Typically, this is the first child of another item, or the
 *              the first child of the list view.
 *  \return \c true if an item which should be visible is found, \c false if all items found should be hidden.
 */
bool KViewSearchLine::checkItemParentsVisible(QModelIndex index)
{
    bool visible = false;
    int rowCount = model()->rowCount(index.parent());
    int column = 0;
    if(d->listView)
        column = d->listView->modelColumn();
    for(int i = 0; i<rowCount; ++i)
    {
        index = model()->index(i, column, index.parent());
        if(model()->rowCount(index) && checkItemParentsVisible(index.child(0,column))
           || itemMatches(index, d->search))
        {
            visible = true;
            setVisible(index, true);
        }
        else
            setVisible(index, false);
    }
    return visible;
}


////////////////////////////////////////////////////////////////////////////////
// KViewSearchLineWidget
////////////////////////////////////////////////////////////////////////////////

class KViewSearchLineWidget::KViewSearchLineWidgetPrivate
{
public:
    KViewSearchLineWidgetPrivate() : view(0), searchLine(0), clearButton(0), layout(0) {}
    QAbstractItemView *view;
    KViewSearchLine *searchLine;
    QToolButton *clearButton;
    QHBoxLayout *layout;
};

KViewSearchLineWidget::KViewSearchLineWidget(QAbstractItemView *view,
                                             QWidget *parent) :
    QWidget(parent)
{
    d = new KViewSearchLineWidgetPrivate;
    d->view = view;

    QTimer::singleShot(0, this, SLOT(createWidgets()));
}

KViewSearchLineWidget::~KViewSearchLineWidget()
{
    delete d->layout;
    delete d;
}

KViewSearchLine *KViewSearchLineWidget::createSearchLine(QAbstractItemView *view)
{
    if(!d->searchLine)
        d->searchLine = new KViewSearchLine(0, view);
    return d->searchLine;
}

void KViewSearchLineWidget::createWidgets()
{
    d->layout = new QHBoxLayout(this);
    d->layout->setSpacing(5);
    positionInToolBar();

    if(!d->clearButton) {
        d->clearButton = new QToolButton();
        d->layout->addWidget(d->clearButton);
        QIcon icon = SmallIconSet(QApplication::isRightToLeft() ? "clear_left" : "locationbar_erase");
        d->clearButton->setIconSet(icon);
    }

    d->clearButton->show();

    QLabel *label = new QLabel(i18n("S&earch:"), 0, "kde toolbar widget");
    d->layout->addWidget(label);

    d->searchLine = createSearchLine(d->view);
    d->layout->addWidget(d->searchLine);
    d->searchLine->show();

    label->setBuddy(d->searchLine);
    label->show();

    connect(d->clearButton, SIGNAL(clicked()), d->searchLine, SLOT(clear()));
}

KViewSearchLine *KViewSearchLineWidget::searchLine() const
{
    return d->searchLine;
}

void KViewSearchLineWidget::positionInToolBar()
{
    KToolBar *toolBar = dynamic_cast<KToolBar *>(parent());

    if(toolBar) {

        // Here we have The Big Ugly.  Figure out how many widgets are in the
        // and do a hack-ish iteration over them to find this widget so that we
        // can insert the clear button before it.

        int widgetCount = toolBar->count();

        for(int index = 0; index < widgetCount; index++) {
            int id = toolBar->idAt(index);
            if(toolBar->getWidget(id) == this) {
                toolBar->setItemAutoSized(id);
                if(!d->clearButton) {
                    QString icon = QApplication::isRightToLeft() ? "clear_left" : "locationbar_erase";
                    d->clearButton = new KToolBarButton(icon, 2005, toolBar);
                }
                toolBar->insertWidget(2005, d->clearButton->width(), d->clearButton, index);
                break;
            }
        }
    }

    if(d->searchLine)
        d->searchLine->show();
}

#include "kebsearchline.moc"
