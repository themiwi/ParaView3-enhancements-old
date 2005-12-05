
#include "pqMultiViewFrame.h"

#include <QStyle>
#include <QPainter>
#include <QPen>
#include <QVBoxLayout>
#include <QMenu>

static int gPenWidth = 2;

pqMultiViewFrame::pqMultiViewFrame(QWidget* parent)
  : QWidget(parent), MainWidget(0), AutoHide(false), Active(false), Color(QColor("red"))
{
  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setMargin(gPenWidth);
  layout->setSpacing(gPenWidth);

  this->Menu = new QWidget(this);
  this->setupUi(Menu);
  layout->addWidget(this->Menu);

  QVBoxLayout* sublayout = new QVBoxLayout();
  layout->addLayout(sublayout);
  sublayout->addStretch();

  this->CloseButton->setIcon(QIcon(this->style()->standardPixmap(QStyle::SP_TitleBarCloseButton)));
  this->MaximizeButton->setIcon(QIcon(this->style()->standardPixmap(QStyle::SP_TitleBarMaxButton)));

  // set up actions
  QAction* a = new QAction(this->ActiveButton->icon(), tr("Active"), this->Menu);
  a->setCheckable(true);
  this->ActiveButton->setDefaultAction(a);
  a = new QAction(this->SplitHorizontalButton->icon(), this->SplitHorizontalButton->text(), this->Menu);
  this->SplitHorizontalButton->setDefaultAction(a);
  a = new QAction(this->SplitVerticalButton->icon(), this->SplitVerticalButton->text(), this->Menu);
  this->SplitVerticalButton->setDefaultAction(a);
  a = new QAction(this->MaximizeButton->icon(), this->MaximizeButton->text(), this->Menu);
  this->MaximizeButton->setDefaultAction(a);
  a = new QAction(this->CloseButton->icon(), this->CloseButton->text(), this->Menu);
  this->CloseButton->setDefaultAction(a);


  // queued connections because these signals can potentially modify/delete this object
  this->connect(this->ActiveButton->defaultAction(), SIGNAL(triggered(bool)), SLOT(setActive(bool)), Qt::QueuedConnection);
  this->connect(this->CloseButton->defaultAction(), SIGNAL(triggered(bool)), SLOT(close()), Qt::QueuedConnection);
  this->connect(this->MaximizeButton->defaultAction(), SIGNAL(triggered(bool)), SLOT(maximize()), Qt::QueuedConnection);
  this->connect(this->SplitVerticalButton->defaultAction(), SIGNAL(triggered(bool)), SLOT(splitVertical()), Qt::QueuedConnection);
  this->connect(this->SplitHorizontalButton->defaultAction(), SIGNAL(triggered(bool)), SLOT(splitHorizontal()), Qt::QueuedConnection);
  
  // setup the context menu
  this->Menu->setContextMenuPolicy(Qt::ActionsContextMenu);
  this->Menu->addAction(this->SplitHorizontalButton->defaultAction());
  this->Menu->addAction(this->SplitVerticalButton->defaultAction());
  this->Menu->addAction(this->CloseButton->defaultAction());
  
  // TODO: temporary until they can be implemented or wanted
  this->MaximizeButton->hide();
  this->ActiveButton->hide();
}

pqMultiViewFrame::~pqMultiViewFrame()
{
}

bool pqMultiViewFrame::menuAutoHide() const
{
  return this->AutoHide;
}

void pqMultiViewFrame::setMenuAutoHide(bool autohide)
{
  this->AutoHide = autohide;
}

bool pqMultiViewFrame::active() const
{
  return this->Active;
}

void pqMultiViewFrame::setActive(bool a)
{
  if(this->ActiveButton->isChecked() != a)
    {
    this->ActiveButton->blockSignals(true);
    this->ActiveButton->setChecked(a);
    this->ActiveButton->blockSignals(false);
    }

  if(this->Active != a)
    {
    this->Active = a;
    emit this->activeChanged(a);
    this->update();
    }
}

QColor pqMultiViewFrame::borderColor() const
{
  return this->Color;
}

void pqMultiViewFrame::setBorderColor(QColor c)
{
  this->Color = c;
}

void pqMultiViewFrame::setMainWidget(QWidget* w)
{
  QLayout* l = this->layout()->itemAt(1)->layout();
  l->removeItem(l->itemAt(0));
  if(w)
    l->addWidget(w);
  else
    static_cast<QBoxLayout*>(l)->addStretch();
}

QWidget* pqMultiViewFrame::mainWidget()
{
  return this->layout()->itemAt(1)->layout()->itemAt(0)->widget();
}

void pqMultiViewFrame::paintEvent(QPaintEvent* e)
{
  QWidget::paintEvent(e);
  if(this->Active)
    {
    QPainter painter(this);
    QPen pen;
    pen.setColor(this->Color);
    pen.setWidth(gPenWidth);
    painter.setPen(pen);
    QLayoutItem* i = this->layout()->itemAt(0);
    QRect r = contentsRect();
    r.adjust(-gPenWidth/2+2, i->geometry().height()+4-gPenWidth/2, gPenWidth/2-2, gPenWidth/2-2);
    painter.drawRect(r);
    }
}

void pqMultiViewFrame::close()
{
  emit this->closePressed();
}

void pqMultiViewFrame::maximize()
{
  emit this->maximizePressed();
}

void pqMultiViewFrame::splitVertical()
{
  emit this->splitVerticalPressed();
}

void pqMultiViewFrame::splitHorizontal()
{
  emit this->splitHorizontalPressed();
}


