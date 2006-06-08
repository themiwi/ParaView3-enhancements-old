/*=========================================================================

   Program:   ParaQ
   Module:    $RCSfile: pqDataInformationWidget.cxx,v $

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.1. 

   See License_v1.1.txt for the full ParaQ license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "pqDataInformationWidget.h"

// Qt includes.
#include <QHeaderView>
#include <QTableView>
#include <QVBoxLayout>
#include <QMenu>

// ParaQ includes.
#include "pqApplicationCore.h"
#include "pqDataInformationModel.h"
#include "pqSectionVisibilityContextMenu.h"
#include "pqServerManagerModel.h"
#include "pqSetName.h"

//-----------------------------------------------------------------------------
pqDataInformationWidget::pqDataInformationWidget(QWidget* _parent /*=0*/)
  : QWidget(_parent)
{
  this->Model = new pqDataInformationModel(this);
  this->View = new QTableView(this);
  this->View->setModel(this->Model);
  this->View->verticalHeader()->hide();
  this->View->installEventFilter(this);
  this->View->horizontalHeader()->setMovable(true);
  this->View->horizontalHeader()->setHighlightSections(false);
  this->View->horizontalHeader()->setStretchLastSection(true);
  this->View->setSelectionBehavior(QAbstractItemView::SelectRows);

  QVBoxLayout * layout = new QVBoxLayout(this);
  if (layout)
    {
    layout->setMargin(0);
    layout->addWidget(this->View);
    }

  pqServerManagerModel* smModel = 
    pqApplicationCore::instance()->getServerManagerModel();
  QObject::connect(smModel, SIGNAL(sourceAdded(pqPipelineSource*)),
    this->Model, SLOT(addSource(pqPipelineSource*)));
  QObject::connect(smModel, SIGNAL(sourceRemoved(pqPipelineSource*)),
    this->Model, SLOT(removeSource(pqPipelineSource*)));

  // Clicking on the header should sort the column.
  QObject::connect(this->View->horizontalHeader(),
    SIGNAL(sectionClicked(int)),
    this->View, SLOT(sortByColumn(int)));

  // Set the context menu policy for the header.
  this->View->horizontalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);

  QObject::connect(this->View->horizontalHeader(),
    SIGNAL(customContextMenuRequested(const QPoint&)),
    this, SLOT(showHeaderContextMenu(const QPoint&)));

  this->View->setContextMenuPolicy(Qt::CustomContextMenu);
  QObject::connect(this->View, 
    SIGNAL(customContextMenuRequested(const QPoint&)),
    this, SLOT(showBodyContextMenu(const QPoint&)));
}

//-----------------------------------------------------------------------------
pqDataInformationWidget::~pqDataInformationWidget()
{
  delete this->View;
  delete this->Model;
}

//-----------------------------------------------------------------------------
bool pqDataInformationWidget::eventFilter(QObject* object, QEvent *event)
{
  return QWidget::eventFilter(object, event);
}

//-----------------------------------------------------------------------------
void pqDataInformationWidget::showHeaderContextMenu(const QPoint& pos)
{
  QHeaderView* header = this->View->horizontalHeader();

  pqSectionVisibilityContextMenu menu;
  menu.setObjectName("DataInformationHeaderContextMenu");
  menu.setHeaderView(header);
  menu.exec(this->View->mapToGlobal(pos));
}
//-----------------------------------------------------------------------------
void pqDataInformationWidget::showBodyContextMenu(const QPoint& pos)
{
  QMenu menu;
  menu.setObjectName("DataInformationBodyContextMenu");
  QAction* action = menu.addAction("Column Titles") 
    << pqSetName("ColumnTitles");
  action->setCheckable(true);
  action->setChecked(this->View->horizontalHeader()->isVisible());
  if (menu.exec(this->View->mapToGlobal(pos)) == action)
    {
    this->View->horizontalHeader()->setVisible(action->isChecked());
    }
}
