#ifndef __kpopupmenu_h
#define __kpopupmenu_h

#include <sys/types.h>

#include <qobject.h>
#include <qmap.h>
#include <qstringlist.h>

#include <kmimetypes.h>

class KService;
class OPMenu;

enum { KPOPUPMENU_BACK_ID, KPOPUPMENU_FORWARD_ID, KPOPUPMENU_UP_ID };

/** TODO : docu ! */
class KonqPopupMenu : public QObject
{
  Q_OBJECT
public:

  /**
   * Constructor
   * @param urls the list of urls the popupmenu should be shown for
   * @param mode mode of urls, if common, 0 otherwise (e.g. S_IFDIR for two dirs,
   * but 0 for a dir and a file).
   * @param viewURL the URL shown in the view, to test for RMB click on view background
   * @param canGoBack set to true if the view can go back
   * @param canGoForward set to true if the view can go forward
   * @param canGoUp set to true if the view can go up
   */
  KonqPopupMenu( QStringList urls,
                 mode_t mode,
                 QString viewURL,
                 bool canGoBack, 
                 bool canGoForward, 
                 bool canGoUp );
  /**
   * Don't forget to destroy the object
   */
  ~KonqPopupMenu();

  /**
   * Execute this popup synchronously.
   * @param p menu position
   * @return ID of the selected item in the popup menu
   * The up, back, and forward values should be tested, because they are NOT implemented here.
   */
  int exec( QPoint p );

public slots:
  void slotFileNewActivated( CORBA::Long id );
  void slotFileNewAboutToShow();
  void slotPopupNewView();
  void slotPopupEmptyTrashBin();
  void slotPopupCopy();
  void slotPopupPaste();
  void slotPopupTrash();
  void slotPopupDelete();
  void slotPopupOpenWith();
  void slotPopupAddToBookmark();
  void slotPopup( int id );
  void slotPopupProperties();

protected:
  OPMenu *m_popupMenu;
  KNewMenu *m_pMenuNew;
  QString m_sViewURL;
  QStringList m_lstPopupURLs;
  mode_t m_popupMode;
  QMap<int,const KService *> m_mapPopup;
  QMap<int,KDELnkMimeType::Service> m_mapPopup2;
};

#endif
