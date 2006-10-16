#include <QCheckBox>
#include <QLayout>
#include <QRadioButton>

//Added by qt3to4:
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <Q3ButtonGroup>

#include <kapplication.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kicondialog.h>
#include <klineedit.h>
#include <kinputdialog.h>
#include <klocale.h>

#include "kservicelistwidget.h"
#include "filetypedetails.h"
#include "typeslistitem.h"

FileTypeDetails::FileTypeDetails( QWidget * parent )
  : QTabWidget( parent ), m_item( 0L )
{
  QString wtstr;
  // First tab - General
  QWidget * firstWidget = new QWidget(this);
  QVBoxLayout *firstLayout = new QVBoxLayout(firstWidget);
  firstLayout->setMargin(KDialog::marginHint());
  firstLayout->setSpacing(KDialog::spacingHint());

  QHBoxLayout *hBox = new QHBoxLayout((QWidget*)0);
  hBox->setSpacing(KDialog::spacingHint());
  firstLayout->addLayout(hBox, 1);

  iconButton = new KIconButton(firstWidget);
  iconButton->setIconType(K3Icon::Desktop, K3Icon::MimeType);
  connect(iconButton, SIGNAL(iconChanged(QString)), SLOT(updateIcon(QString)));

  iconButton->setFixedSize(70, 70);
  hBox->addWidget(iconButton);

  iconButton->setWhatsThis( i18n("This button displays the icon associated"
    " with the selected file type. Click on it to choose a different icon.") );

  Q3GroupBox *gb = new Q3GroupBox(i18n("Filename Patterns"), firstWidget);
  hBox->addWidget(gb);

  QGridLayout *grid = new QGridLayout(gb);
  grid->setMargin(KDialog::marginHint());
  grid->setSpacing(KDialog::spacingHint());
  grid->addItem(new QSpacerItem(0,fontMetrics().lineSpacing()), 0, 0);

  extensionLB = new Q3ListBox(gb);
  connect(extensionLB, SIGNAL(highlighted(int)), SLOT(enableExtButtons(int)));
  grid->addWidget(extensionLB, 1, 0, 2, 1);
  grid->setRowStretch(0, 0);
  grid->setRowStretch(1, 1);
  grid->setRowStretch(2, 0);

  extensionLB->setWhatsThis( i18n("This box contains a list of patterns that can be"
    " used to identify files of the selected type. For example, the pattern *.txt is"
    " associated with the file type 'text/plain'; all files ending in '.txt' are recognized"
    " as plain text files.") );

  addExtButton = new QPushButton(i18n("Add..."), gb);
  addExtButton->setEnabled(false);
  connect(addExtButton, SIGNAL(clicked()),
          this, SLOT(addExtension()));
  grid->addWidget(addExtButton, 1, 1);

  addExtButton->setWhatsThis( i18n("Add a new pattern for the selected file type.") );

  removeExtButton = new QPushButton(i18n("Remove"), gb);
  removeExtButton->setEnabled(false);
  connect(removeExtButton, SIGNAL(clicked()),
          this, SLOT(removeExtension()));
  grid->addWidget(removeExtButton, 2, 1);

  removeExtButton->setWhatsThis( i18n("Remove the selected filename pattern.") );

  gb = new Q3GroupBox(i18n("Description"), firstWidget);
  firstLayout->addWidget(gb);

  gb->setColumnLayout(1, Qt::Horizontal);
  description = new KLineEdit(gb);
  connect(description, SIGNAL(textChanged(const QString &)),
          SLOT(updateDescription(const QString &)));

  wtstr = i18n("You can enter a short description for files of the selected"
    " file type (e.g. 'HTML Page'). This description will be used by applications"
    " like Konqueror to display directory content.");
  gb->setWhatsThis( wtstr );
  description->setWhatsThis( wtstr );

  serviceListWidget = new KServiceListWidget( KServiceListWidget::SERVICELIST_APPLICATIONS, firstWidget );
  connect( serviceListWidget, SIGNAL(changed(bool)), this, SIGNAL(changed(bool)));
  firstLayout->addWidget(serviceListWidget, 5);

  // Second tab - Embedding
  QWidget * secondWidget = new QWidget(this);
  QVBoxLayout *secondLayout = new QVBoxLayout(secondWidget);
  secondLayout->setMargin(KDialog::marginHint());
  secondLayout->setSpacing(KDialog::spacingHint());

  m_autoEmbed = new Q3ButtonGroup( i18n("Left Click Action"), secondWidget );
  secondLayout->setSpacing( KDialog::spacingHint() );
  secondLayout->addWidget( m_autoEmbed, 1 );

  m_autoEmbed->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Fixed );
  m_autoEmbed->sizePolicy().setHeightForWidth(m_autoEmbed->sizePolicy().hasHeightForWidth());

  // The order of those three items is very important. If you change it, fix typeslistitem.cpp !
  new QRadioButton( i18n("Show file in embedded viewer"), m_autoEmbed );
  new QRadioButton( i18n("Show file in separate viewer"), m_autoEmbed );
  m_rbGroupSettings = new QRadioButton( QString("Use settings for '%1' group"), m_autoEmbed );
  connect(m_autoEmbed, SIGNAL( clicked( int ) ), SLOT( slotAutoEmbedClicked( int ) ));

  m_chkAskSave = new QCheckBox( i18n("Ask whether to save to disk instead"), m_autoEmbed);
  connect(m_chkAskSave, SIGNAL( toggled(bool) ), SLOT( slotAskSaveToggled(bool) ));

  m_autoEmbed->setWhatsThis( i18n("Here you can configure what the Konqueror file manager"
    " will do when you click on a file of this type. Konqueror can display the file in"
    " an embedded viewer or start up a separate application. If set to 'Use settings for G group',"
    " Konqueror will behave according to the settings of the group G this type belongs to,"
    " for instance 'image' if the current file type is image/png.") );

  secondLayout->addSpacing(10);

  embedServiceListWidget = new KServiceListWidget( KServiceListWidget::SERVICELIST_SERVICES, secondWidget );
  embedServiceListWidget->setMinimumHeight( serviceListWidget->sizeHint().height() );
  connect( embedServiceListWidget, SIGNAL(changed(bool)), this, SIGNAL(changed(bool)));
  secondLayout->addWidget(embedServiceListWidget, 3);

  addTab( firstWidget, i18n("&General") );
  addTab( secondWidget, i18n("&Embedding") );
}

void FileTypeDetails::updateRemoveButton()
{
    removeExtButton->setEnabled(extensionLB->count()>0);
}

void FileTypeDetails::updateIcon(QString icon)
{
  if (!m_item)
    return;

  m_item->setIcon(icon);

  emit changed(true);
}

void FileTypeDetails::updateDescription(const QString &desc)
{
  if (!m_item)
    return;

  m_item->setComment(desc);

  emit changed(true);
}

void FileTypeDetails::addExtension()
{
  if ( !m_item )
    return;

  bool ok;
  QString ext = KInputDialog::getText( i18n( "Add New Extension" ),
    i18n( "Extension:" ), "*.", &ok, this );
  if (ok) {
    extensionLB->insertItem(ext);
    QStringList patt = m_item->patterns();
    patt += ext;
    m_item->setPatterns(patt);
    updateRemoveButton();
    emit changed(true);
  }
}

void FileTypeDetails::removeExtension()
{
  if (extensionLB->currentItem() == -1)
    return;
  if ( !m_item )
    return;
  QStringList patt = m_item->patterns();
  patt.removeAll(extensionLB->text(extensionLB->currentItem()));
  m_item->setPatterns(patt);
  extensionLB->removeItem(extensionLB->currentItem());
  updateRemoveButton();
  emit changed(true);
}

void FileTypeDetails::slotAutoEmbedClicked( int button )
{
  if ( !m_item || (button > 2))
    return;

  m_item->setAutoEmbed( button );

  updateAskSave();

  emit changed(true);
}

void FileTypeDetails::updateAskSave()
{
  if ( !m_item )
    return;

  int button = m_item->autoEmbed();
  if (button == 2)
  {
    bool embedParent = TypesListItem::defaultEmbeddingSetting(m_item->majorType());
    emit embedMajor(m_item->majorType(), embedParent);
    button = embedParent ? 0 : 1;
  }

  QString mimeType = m_item->name();

  QString dontAskAgainName;

  if (button == 0) // Embedded
    dontAskAgainName = "askEmbedOrSave"+mimeType;
  else
    dontAskAgainName = "askSave"+mimeType;

  KSharedConfig::Ptr config = KSharedConfig::openConfig("konquerorrc", false, false);
  config->setGroup("Notification Messages");
  bool ask = config->readEntry(dontAskAgainName, QString()).isEmpty();
  m_item->getAskSave(ask);

  bool neverAsk = false;

  if (button == 0)
  {
    KMimeType::Ptr mime = KMimeType::mimeType( mimeType );
    // Don't ask for:
    // - html (even new tabs would ask, due to about:blank!)
    // - dirs obviously (though not common over HTTP :),
    // - images (reasoning: no need to save, most of the time, because fast to see)
    // e.g. postscript is different, because takes longer to read, so
    // it's more likely that the user might want to save it.
    // - multipart/* ("server push", see kmultipart)
    // - other strange 'internal' mimetypes like print/manager...
    if ( mime->is( "text/html" ) ||
         mime->is( "text/xml" ) ||
         mime->is( "inode/directory" ) ||
         mimeType.startsWith( "image" ) ||
         mime->is( "multipart/x-mixed-replace" ) ||
         mime->is( "multipart/replace" ) ||
         mimeType.startsWith( "print" ) )
    {
        neverAsk = true;
    }
  }

  m_chkAskSave->blockSignals(true);
  m_chkAskSave->setChecked(ask && !neverAsk);
  m_chkAskSave->setEnabled(!neverAsk);
  m_chkAskSave->blockSignals(false);
}

void FileTypeDetails::slotAskSaveToggled(bool askSave)
{
  if (!m_item)
    return;

  m_item->setAskSave(askSave);
  emit changed(true);
}

void FileTypeDetails::setTypeItem( TypesListItem * tlitem )
{
  m_item = tlitem;
  Q_ASSERT(tlitem);
  iconButton->setIcon(tlitem->icon());
  description->setText(tlitem->comment());
  m_rbGroupSettings->setText( i18n("Use settings for '%1' group", tlitem->majorType() ) );
  extensionLB->clear();
  addExtButton->setEnabled(true);
  removeExtButton->setEnabled(false);

  serviceListWidget->setTypeItem( tlitem );
  embedServiceListWidget->setTypeItem( tlitem );
  m_autoEmbed->setButton( tlitem->autoEmbed() );
  m_rbGroupSettings->setEnabled( tlitem->canUseGroupSetting() );

  extensionLB->insertStringList(tlitem->patterns());

  updateAskSave();
}

void FileTypeDetails::enableExtButtons(int /*index*/)
{
  removeExtButton->setEnabled(true);
}

#include "filetypedetails.moc"
