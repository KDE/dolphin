/*
 * SPDX-FileCopyrightText: 2010 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef DOLPHINSEARCHBOX_H
#define DOLPHINSEARCHBOX_H

#include <QUrl>
#include <QWidget>

class DolphinFacetsWidget;
class DolphinQuery;
class QLineEdit;
class KSeparator;
class QToolButton;
class QScrollArea;
class QLabel;
class QVBoxLayout;
class KMoreToolsMenuFactory;

/**
 * @brief Input box for searching files with or without Baloo.
 *
 * The widget allows to specify:
 * - Where to search: Everywhere or below the current directory
 * - What to search: Filenames or content
 *
 * If Baloo is available and the current folder is indexed, further
 * options are offered.
 */
class DolphinSearchBox : public QWidget
{
    Q_OBJECT

public:
    explicit DolphinSearchBox(QWidget *parent = nullptr);
    ~DolphinSearchBox() override;

    /**
     * Sets the text that should be used as input for
     * searching.
     */
    void setText(const QString &text);

    /**
     * Returns the text that should be used as input
     * for searching.
     */
    QString text() const;

    /**
     * Sets the current path that is used as root for searching files.
     * If @url is the Home dir, "From Here" is selected instead.
     */
    void setSearchPath(const QUrl &url);
    QUrl searchPath() const;

    /** @return URL that will start the searching of files. */
    QUrl urlForSearching() const;

    /**
     * Extracts information from the given search \a url to
     * initialize the search box properly.
     */
    void fromSearchUrl(const QUrl &url);

    /**
     * Selects the whole text of the search box.
     */
    void selectAll();

    /**
     * Set the search box to the active mode, if \a active
     * is true. The active mode is default. The inactive mode only differs
     * visually from the active mode, no change of the behavior is given.
     *
     * Using the search box in the inactive mode is useful when having split views,
     * where the inactive view is indicated by an search box visually.
     */
    void setActive(bool active);

    /**
     * @return True, if the search box is in the active mode.
     * @see    DolphinSearchBox::setActive()
     */
    bool isActive() const;

protected:
    bool event(QEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

Q_SIGNALS:
    /**
     * Is emitted when a searching should be triggered.
     */
    void searchRequest();

    /**
     * Emitted as soon as the search box should get closed.
     */
    void closeRequest();

    /**
     * Is emitted, if the searchbox has been activated by
     * an user interaction
     * @see DolphinSearchBox::setActive()
     */
    void activated();
    void focusViewRequest();
    void clearRequest();

private Q_SLOTS:
    void emitSearchRequest();
    void emitCloseRequest();
    void slotConfigurationChanged();
    void slotSearchTextChanged(const QString &text);
    void slotReturnPressed();
    void slotFacetChanged();
    void slotSearchSaved();

private:
    void initButton(QToolButton *button);
    void loadSettings();
    void saveSettings();
    void init();

    /**
     * @return URL that represents the Baloo query for starting the search.
     */
    QUrl balooUrlForSearching() const;

    /**
     * Sets the searchbox UI with the parameters established by the \a query
     */
    void updateFromQuery(const DolphinQuery &query);

    void updateFacetsVisible();

    bool isIndexingEnabled() const;

private:
    QString queryTitle(const QString &text) const;

    bool m_startedSearching;
    bool m_active;

    QVBoxLayout *m_topLayout;

    QLineEdit *m_searchInput;
    QAction *m_saveSearchAction;
    QScrollArea *m_optionsScrollArea;
    QToolButton *m_fileNameButton;
    QToolButton *m_contentButton;
    KSeparator *m_separator;
    QToolButton *m_fromHereButton;
    QToolButton *m_everywhereButton;
    DolphinFacetsWidget *m_facetsWidget;

    QUrl m_searchPath;
    QScopedPointer<KMoreToolsMenuFactory> m_menuFactory;

    QTimer *m_startSearchTimer;
};

#endif
