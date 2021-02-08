/*
 * SPDX-FileCopyrightText: 2012 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef DOLPHINFACETSWIDGET_H
#define DOLPHINFACETSWIDGET_H

#include <QWidget>
#include <KCoreDirLister>

class QComboBox;
class QDate;
class QEvent;
class QToolButton;

/**
 * @brief Allows to filter search-queries by facets.
 *
 * TODO: The current implementation is a temporary
 * workaround for the 4.9 release and represents no
 * real facets-implementation yet: There have been
 * some Dolphin specific user-interface and interaction
 * issues since 4.6 by embedding the Nepomuk facet-widget
 * into a QDockWidget (this is unrelated to the
 * Nepomuk facet-widget itself). Now in combination
 * with the search-shortcuts in the Places Panel some
 * existing issues turned into real showstoppers.
 *
 * So the longterm plan is to use the Nepomuk facets
 * again as soon as possible.
 */
class DolphinFacetsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DolphinFacetsWidget(QWidget* parent = nullptr);
    ~DolphinFacetsWidget() override;

    QStringList searchTerms() const;
    QString facetType() const;

    bool isSearchTerm(const QString& term) const;
    void setSearchTerm(const QString& term);
    void resetSearchTerms();

    void setFacetType(const QString& type);

Q_SIGNALS:
    void facetChanged();

protected:
    void changeEvent(QEvent* event) override;

private Q_SLOTS:
    void updateTagsMenu();
    void updateTagsMenuItems(const QUrl&, const KFileItemList& items);

private:
    void setRating(const int stars);
    void setTimespan(const QDate& date);
    void addSearchTag(const QString& tag);
    void removeSearchTag(const QString& tag);

    void initComboBox(QComboBox* combo);
    void updateTagsSelector();

private:
    QComboBox* m_typeSelector;
    QComboBox* m_dateSelector;
    QComboBox* m_ratingSelector;
    QToolButton* m_tagsSelector;

    QStringList m_searchTags;
    KCoreDirLister m_tagsLister;
};

#endif
