#ifndef konq_mainwindow_p_h
#define konq_mainwindow_p_h

class KonqExtendedBookmarkOwner : public KExtendedBookmarkOwner
{
  Q_OBJECT
public:
  KonqExtendedBookmarkOwner(KonqMainWindow *);
  // from KBookmarkOwner
  virtual void openBookmarkURL( const QString & _url );
  virtual QString currentTitle() const;
  virtual QString currentURL() const;
public slots:
  // for KExtendedBookmarkOwner
  void slotFillBookmarksList( KExtendedBookmarkOwner::QStringPairList & list );
private:
  KonqMainWindow *m_pKonqMainWindow;
};

#endif
