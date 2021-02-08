/*
 * SPDX-FileCopyrightText: 2006 Peter Penz <peter.penz@gmx.at>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef VIEWPROPSPROGRESSINFO_H
#define VIEWPROPSPROGRESSINFO_H

#include <kio/directorysizejob.h>

#include <QDialog>
#include <QUrl>

class ApplyViewPropsJob;
class QLabel;
class QProgressBar;
class QTimer;
class ViewProperties;

/**
 * @brief Shows the progress information when applying view properties
 *        recursively to a given directory.
 *
 * It is possible to cancel the applying. In this case the already applied
 * view properties won't get reverted.
 */
class ViewPropsProgressInfo : public QDialog
{
    Q_OBJECT

public:
    /**
     * @param parent    Parent widget of the dialog.
     * @param dir       Directory where the view properties should be applied to
     *                  (including sub directories).
     * @param viewProps View properties for the directory \a dir including its
     *                  sub directories.
     */
    ViewPropsProgressInfo(QWidget* parent,
                          const QUrl& dir,
                          const ViewProperties& viewProps);

    ~ViewPropsProgressInfo() override;

protected:
    void closeEvent(QCloseEvent* event) override;

public Q_SLOTS:
    void reject() override;

private Q_SLOTS:
    void updateProgress();
    void applyViewProperties();

private:
    QUrl m_dir;
    ViewProperties* m_viewProps;

    QLabel* m_label;
    QProgressBar* m_progressBar;

    KIO::DirectorySizeJob* m_dirSizeJob;
    ApplyViewPropsJob* m_applyViewPropsJob;
    QTimer* m_timer;
};

#endif
