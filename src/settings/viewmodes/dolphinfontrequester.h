/*
 * SPDX-FileCopyrightText: 2008 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef DOLPHINFONTREQUESTER_H
#define DOLPHINFONTREQUESTER_H

#include <QFont>
#include <QWidget>

class QComboBox;
class QPushButton;

/**
 * @brief Allows to select between using the system font or a custom font.
 */
class DolphinFontRequester : public QWidget
{
    Q_OBJECT

public:
    enum Mode
    {
        SystemFont = 0,
        CustomFont = 1
    };

    explicit DolphinFontRequester(QWidget* parent);
    ~DolphinFontRequester() override;

    void setMode(Mode mode);
    Mode mode() const;

    /**
     * Returns the custom font (see DolphinFontRequester::customFont()),
     * if the mode is \a CustomFont, otherwise the system font is
     * returned.
     */
    QFont currentFont() const;

    void setCustomFont(const QFont& font);
    QFont customFont() const;

signals:
    /** Is emitted, if the font has been changed. */
    void changed();

private slots:
    void openFontDialog();
    void changeMode(int index);

private:
    QComboBox* m_modeCombo;
    QPushButton* m_chooseFontButton;

    Mode m_mode;
    QFont m_customFont;
};

#endif
