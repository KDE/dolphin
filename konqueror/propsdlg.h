/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>
 
   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.
 
   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.
 
   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/     

/*
 * This file holds the definitions for all classes used to
 * display a properties dialog.
 */

#ifndef __propsdlg_h
#define __propsdlg_h

#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>

#include <qtabdialog.h>
#include <qbuttongroup.h>
#include <qradiobutton.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qstring.h>
#include <qcheckbox.h>
#include <qlineedit.h>
#include <qlist.h>
#include <qcombobox.h>
#include <qgroupbox.h>
#include <qpushbutton.h>
#include <qlistbox.h>
#include <qtooltip.h>

#include <kurl.h>
#include <kiconloaderdialog.h>
#include <kiconloader.h>

class PropsPage;

/**
 * The main class.
 * This one is visible to the one who created the dialog.
 * It brings up a QTabDialog.
 * This class must be created with (void)new Properties(...)
 * It will take care of deleting itself.
 */
class Properties : public QObject
{
    Q_OBJECT
public:
    /// Bring up a dialog for the given URL.
    Properties( const char *_url, mode_t _mode );
    ~Properties();

    /// Returns a parsed URL
    KURL* getKURL() { return kurl; }
    /// Returns the URL text
    const char* getURL() { return url.data(); }
    /// Returns the mode
    mode_t getMode() { return mode; }
    /// Returns a pointer to the dialog
    QTabDialog* getTab() { return tab; }

    /// Usually called from a PropsPage in order of emitting the signal.
    void emitPropertiesChanged( const QString& _new_name );
    
public slots:
    /// Called when the user presses 'Ok'.
    void slotApply();      // Deletes the Properties instance
    void slotCancel();     // Deletes the Properties instance

signals:
    /// Notify about changes in properties and rnameings.
   /**
     For example the root widget needs to be informed about renameing. Otherwise
     it would consider the renamed icon a new icon and would move it to the upper left
     corner or something like that.
     In most cases you wont need this signal because KIOManager is informed about changes.
     This causes KFileWindow for example to reload its contents if necessary.
     If the name did not change, _name is 0L.
     */
    void propertiesChanged( const QString& _url, const QString& _new_name );
    void propertiesCancel();
    
protected:
    /// Inserts all pages in the dialog.
    void insertPages();

    /// The URL of the file
    QString url;
    /// The parsed URL
    KURL *kurl;
    /// The mode of the file
    mode_t mode;
    /// List of all pages inserted ( first one first )
    QList<PropsPage> pageList;
    /// The dialog
    QTabDialog *tab;
};

/**
 * A Page in the Properties dialog
 * This is an abstract class. You must inherit from this class
 * to build a new kind of page.
 */
class PropsPage : public QWidget
{
    Q_OBJECT
public:
    /// Constructor
    PropsPage( Properties *_props );

    /// Returns the name that should appear in the tab.
    virtual const char* getTabName() { return ""; }
    /// Apply all changes to the file.
    /**
      This function is called when the user presses 'Ok'. The last page inserted
      is called first.
      */
    virtual void applyChanges() { }
    
protected:
    /// Pointer to the dialog
    Properties *properties;

    int fontHeight;
};

/**
 * 'General' page
 *  This page displays the name of the file, its size and access times.
 */
class FilePropsPage : public PropsPage
{
    Q_OBJECT
public:
    /// Constructor
    FilePropsPage( Properties *_props );

    virtual const char* getTabName() { return i18n("&General"); }
    /// Applies all changes made
    /** 'General' must be always the first page in the dialog, since this
      function may rename the file which may confuse other applyChanges
      functions. When this page is the first one this means that this applyChanges
      function is the last one called.
      */
    virtual void applyChanges();
    
    /// Tests whether the file specified by _kurl needs a 'General' page.
    static bool supports( KURL *_kurl, mode_t _mode );
    
protected:
    QLineEdit *name;

    QBoxLayout *layout;		// BL: layout mngt

    /// The initial filename
    QString oldName;
};

/**
 * 'Permissions' page
 * In this page you can modify permissions and change
 * the owner of a file.
 */
class FilePermissionsPropsPage : public PropsPage
{
    Q_OBJECT
public:
    /// Constructor
    FilePermissionsPropsPage( Properties *_props );

    virtual const char* getTabName() { return i18n("&Permissions"); }
    virtual void applyChanges();

    /// Tests whether the file specified by _kurl needs a 'Permissions' page.
    static bool supports( KURL *_kurl, mode_t _mode );
    
protected:
    QCheckBox *permBox[3][4];
    QComboBox *grp;
    QLineEdit *owner;

    /// Old permissions
    mode_t permissions;
    /// Old group
    QString strGroup;
    /// Old owner
    QString strOwner;

    /// Changeable Permissions
    static mode_t fperm[3][4];
};

/**
  Used to edit the files containing
  [KDE Desktop Entry]
  Type=Application

  Such files are used to represent a program in kpanel and kfm.
  */
class ExecPropsPage : public PropsPage
{
    Q_OBJECT
public:
    /// Constructor
    ExecPropsPage( Properties *_props );

    virtual const char* getTabName() { return i18n("E&xecute"); }
    virtual void applyChanges();

    static bool supports( KURL *_kurl, mode_t _mode );

public slots:
    void slotBrowseExec();
    
private slots:
    void enableCheckedEdit();
    
protected:
    
    QLineEdit *execEdit;
    KIconLoaderButton *iconBox;
    QCheckBox *terminalCheck;
    QLineEdit *terminalEdit;
    QLineEdit *swallowExecEdit;
    QLineEdit *swallowTitleEdit;
    QButton *execBrowse;

    QString execStr;
    QString iconStr;
    QString swallowExecStr;
    QString swallowTitleStr;
    QString termStr;
    QString termOptionsStr;
};

/**
  Used to edit the files containing
  [KDE Desktop Entry]
  URL=....

  Such files are used to represent a program in kpanel and kfm.
  */
class URLPropsPage : public PropsPage
{
    Q_OBJECT
public:
    /// Constructor
    URLPropsPage( Properties *_props );

    virtual const char* getTabName() { return i18n("U&RL"); }
    virtual void applyChanges();

    static bool supports( KURL *_kurl, mode_t _mode );

protected:
    QLineEdit *URLEdit;
    KIconLoaderButton *iconBox;

    QString URLStr;
    QString iconStr;

    QPixmap pixmap;
    QString pixmapFile;
};

class DirPropsPage : public PropsPage
{
    Q_OBJECT
public:
    /// Constructor
    DirPropsPage( Properties *_props );
    ~DirPropsPage() {};

    virtual const char* getTabName() { return i18n("&Dir"); }
    virtual void applyChanges();

    static bool supports( KURL *_kurl, mode_t _mode );

public slots:
    void slotWallPaperChanged( int );
    void slotBrowse(); 
    void slotApply();
    void slotApplyGlobal();
    
protected:
    void showSettings(QString filename);
    void drawWallPaper();
    virtual void paintEvent ( QPaintEvent *);
    virtual void resizeEvent ( QResizeEvent *);
    
    QPushButton *applyButton;
    QPushButton *globalButton;
    QPushButton *browseButton;
    
    KIconLoaderButton *iconBox;
    QComboBox *wallBox;

    QString wallStr;
    QString iconStr;

    QPixmap wallPixmap;
    QString wallFile;
    int imageX, imageW, imageH, imageY;
};

/**
  Used to edit the files containing
  [KDE Desktop Entry]
  Type=Application

  Such files are used to represent a program in kpanel and kfm.
  */
class ApplicationPropsPage : public PropsPage
{
    Q_OBJECT
public:
    /// Constructor
    ApplicationPropsPage( Properties *_props );

    virtual const char* getTabName() { return i18n("&Application"); }
    virtual void applyChanges();

    static bool supports( KURL *_kurl, mode_t _mode );

public slots:
    void slotDelExtension();
    void slotAddExtension();    

protected:

    void addMimeType( const char * name );

    QLineEdit *binaryPatternEdit;
    QLineEdit *commentEdit;
    QLineEdit *nameEdit;
    QListBox  *extensionsList;
    QListBox  *availableExtensionsList;
    QPushButton *addExtensionButton;
    QPushButton *delExtensionButton;
    
    QBoxLayout *layout, *layoutH, *layoutV;

    QString nameStr;
    QString extensionsStr;
    QString binaryPatternStr;
    QString commentStr;
};

/**
  Used to edit the files containing
  [KDE Desktop Entry]
  Type=MimeType
  */
class BindingPropsPage : public PropsPage
{
    Q_OBJECT
public:
    /// Constructor
    BindingPropsPage( Properties *_props );

    virtual const char* getTabName() { return i18n("&Binding"); }
    virtual void applyChanges();

    static bool supports( KURL *_kurl, mode_t _mode );

protected:
    
    QLineEdit *commentEdit;
    QLineEdit *patternEdit;
    QLineEdit *mimeEdit;
    KIconLoaderButton *iconBox;
    QComboBox *appBox;
    QStrList kdelnklist; // holds the kdelnk names for the combobox items

    QPixmap pixmap;
    QString pixmapFile;
};

class DevicePropsPage : public PropsPage
{
    Q_OBJECT
public:
    DevicePropsPage( Properties *_props );
    ~DevicePropsPage() { }

    virtual const char* getTabName() { return i18n("De&vice"); }
    virtual void applyChanges();

    static bool supports( KURL *_kurl, mode_t _mode );
    
protected:
    QLineEdit* device;
    QLineEdit* mountpoint;
    QCheckBox* readonly;
    QLineEdit* fstype;
    KIconLoaderButton* mounted;
    KIconLoaderButton* unmounted;

    QPixmap pixmap;
    QString pixmapFile;

    QString deviceStr;
    QString mountPointStr;
    QString mountedStr;
    QString unmountedStr;
    QString readonlyStr;
    QString fstypeStr;
};

#endif
