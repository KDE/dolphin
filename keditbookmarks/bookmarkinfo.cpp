void BookmarkInfoWidget::showBookmark(const KBookmark &bk) {
   m_title_le->setText(bk.text());
   m_url_le->setText(bk.url().url());
   m_comment_le->setText("a comment");
   m_moddate_le->setText("the modification date");
   m_credate_le->setText("the creation date");
}

BookmarkInfoWidget::BookmarkInfoWidget(
   QWidget * parent, const char * name
) : QWidget (parent, name) {
   QBoxLayout *vbox = new QVBoxLayout(this);
   QGridLayout *grid = new QGridLayout(vbox, 3, 4);

   m_title_le = new KLineEdit(this);
   m_title_le->setReadOnly(true);
   grid->addWidget(m_title_le, 0, 1);
   grid->addWidget(
            new QLabel(m_title_le, i18n("Name:"), this), 
            0, 0);

   m_url_le = new KLineEdit(this);
   m_url_le->setReadOnly(true);
   grid->addWidget(m_url_le, 1, 1);
   grid->addWidget(
            new QLabel(m_url_le, i18n("Location:"), this), 
            1, 0);

   m_comment_le = new KLineEdit(this);
   m_comment_le->setReadOnly(true);
   grid->addWidget(m_comment_le, 2, 1);
   grid->addWidget(
            new QLabel(m_comment_le, i18n("Comment:"), this), 
            2, 0);

   m_moddate_le = new KLineEdit(this);
   m_moddate_le->setReadOnly(true);
   grid->addWidget(m_moddate_le, 0, 3);
   grid->addWidget(
            new QLabel(m_moddate_le, i18n("First viewed:"), this), 
            0, 2 );

   m_credate_le = new KLineEdit(this);
   m_credate_le->setReadOnly(true);
   grid->addWidget(m_credate_le, 1, 3);
   grid->addWidget(
            new QLabel(m_credate_le, i18n("Viewed last:"), this), 
            1, 2);
}

