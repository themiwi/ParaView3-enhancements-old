/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqExodusPanel.cxx,v $

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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

#include "pqExodusPanel.h"

// Qt includes
#include <QTreeWidget>
#include <QVariant>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QAction>
#include <QTimer>
#include <QLabel>

// VTK includes

// ParaView Server Manager includes
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMProperty.h"

// ParaView includes
#include "pqPropertyManager.h"
#include "pqProxy.h"
#include "pqSMAdaptor.h"
#include "pqTreeWidgetCheckHelper.h"
#include "pqTreeWidgetItemObject.h"
#include "ui_pqExodusPanel.h"

class pqExodusPanel::pqUI : public QObject, public Ui::ExodusPanel 
{
public:
  pqUI(pqExodusPanel* p) : QObject(p) {}
};

pqExodusPanel::pqExodusPanel(pqProxy* object_proxy, QWidget* p) :
  pqNamedObjectPanel(object_proxy, p)
{
  this->UI = new pqUI(this);
  this->UI->setupUi(this);
  
  QObject::connect(this->UI->DisplayType, SIGNAL(currentChanged(int)), 
                   this, SIGNAL(displayTypeChanged()));
  
  this->DisplItem = 0;
  QObject::connect(this->propertyManager(), 
                   SIGNAL(canAcceptOrReject(bool)),
                   this, SLOT(propertyChanged()));

  this->UI->XMLFileName->setServer(this->proxy()->getServer());
  
#if QT_VERSION < 0x040200
  // workaround for Qt bug in plastique style, where painter state wasn't being
  // restored after a save.
  // we always want a height greater than 2 anyway
  this->UI->BlockArrayStatus->setMinimumHeight(2);
  this->UI->HierarchyArrayStatus->setMinimumHeight(2);
  this->UI->MaterialArrayStatus->setMinimumHeight(2);
#endif
 
  this->DataUpdateInProgress = false;
  
  this->linkServerManagerProperties();
}

pqExodusPanel::~pqExodusPanel()
{
}
  
void pqExodusPanel::accept()
{
  this->pqNamedObjectPanel::accept();
 
  // setting block/material/hierarchy status 
  // may affect each other
  // query the reader for the new values 
  this->proxy()->getProxy()->UpdatePropertyInformation();

  vtkSMProxy* pxy = this->proxy()->getProxy();
  vtkSMProperty* prop = NULL;

  prop = pxy->GetProperty("BlockArrayStatus");
  prop->Copy(pxy->GetProperty("BlockArrayInfo"));
  
  prop = pxy->GetProperty("MaterialArrayStatus");
  prop->Copy(pxy->GetProperty("MaterialArrayInfo"));
  
  prop = pxy->GetProperty("HierarchyArrayStatus");
  prop->Copy(pxy->GetProperty("HierarchyArrayInfo"));
  
}

int pqExodusPanel::displayType() const
{
  return this->UI->DisplayType->currentIndex() + 1;
}

void pqExodusPanel::setDisplayType(int d)
{
  this->UI->DisplayType->setCurrentIndex(d-1);
}

void pqExodusPanel::linkServerManagerProperties()
{
  // parent class hooks up some of our widgets in the ui
  pqNamedObjectPanel::linkServerManagerProperties();
  
  this->propertyManager()->registerLink(
    this,
    "displayType", 
    SIGNAL(displayTypeChanged()),
    this->proxy()->getProxy(), 
    this->proxy()->getProxy()->GetProperty("DisplayType"));

  this->DisplItem = 0;

  QPixmap cellPixmap(":/pqWidgets/Icons/pqCellData16.png");
  QPixmap pointPixmap(":/pqWidgets/Icons/pqPointData16.png");
  QPixmap sideSetPixmap(":/pqWidgets/Icons/pqSideSet16.png");
  QPixmap nodeSetPixmap(":/pqWidgets/Icons/pqNodeSet16.png");

  // we hook up the node/element variables
  QTreeWidget* VariablesTree = this->UI->Variables;
  new pqTreeWidgetCheckHelper(VariablesTree, 0, this);
  pqTreeWidgetItemObject* item;
  QList<QString> strs;
  QString varName;
  
  // do block id, global element id
  varName = "Block Ids";
  strs.append(varName);
  item = new pqTreeWidgetItemObject(VariablesTree, strs);
  item->setData(0, Qt::ToolTipRole, varName);
  item->setData(0, Qt::DecorationRole, cellPixmap);
  this->propertyManager()->registerLink(item, 
                      "checked", 
                      SIGNAL(checkedStateChanged(bool)),
                      this->proxy()->getProxy(), 
                      this->proxy()->getProxy()->GetProperty("GenerateBlockIdCellArray"));
  
  varName = "Global Element Ids";
  strs.clear();
  strs.append(varName);
  item = new pqTreeWidgetItemObject(VariablesTree, strs);
  item->setData(0, Qt::ToolTipRole, varName);
  item->setData(0, Qt::DecorationRole, cellPixmap);
  this->propertyManager()->registerLink(item, 
                    "checked", 
                    SIGNAL(checkedStateChanged(bool)),
                    this->proxy()->getProxy(), 
                    this->proxy()->getProxy()->GetProperty("GenerateGlobalElementIdArray"));
  
  // do the cell variables
  vtkSMProperty* CellProperty = this->proxy()->getProxy()->GetProperty("CellArrayStatus");
  QList<QVariant> CellDomain;
  CellDomain = pqSMAdaptor::getSelectionPropertyDomain(CellProperty);
  int j;
  for(j=0; j<CellDomain.size(); j++)
    {
    varName = CellDomain[j].toString();
    strs.clear();
    strs.append(varName);
    item = new pqTreeWidgetItemObject(VariablesTree, strs);
    item->setData(0, Qt::ToolTipRole, varName);
    item->setData(0, Qt::DecorationRole, cellPixmap);
    this->propertyManager()->registerLink(item, 
                                      "checked", 
                                      SIGNAL(checkedStateChanged(bool)),
                                      this->proxy()->getProxy(), CellProperty, j);
    }
  
  
  // do global node ids
  varName = "Global Node Ids";
  strs.clear();
  strs.append(varName);
  item = new pqTreeWidgetItemObject(VariablesTree, strs);
  item->setData(0, Qt::ToolTipRole, varName);
  item->setData(0, Qt::DecorationRole, pointPixmap);
  this->propertyManager()->registerLink(item, 
                    "checked", 
                    SIGNAL(checkedStateChanged(bool)),
                    this->proxy()->getProxy(), 
                    this->proxy()->getProxy()->GetProperty("GenerateGlobalNodeIdArray"));

  // do the node variables
  vtkSMProperty* NodeProperty = this->proxy()->getProxy()->GetProperty("PointArrayStatus");
  QList<QVariant> PointDomain;
  PointDomain = pqSMAdaptor::getSelectionPropertyDomain(NodeProperty);

  for(j=0; j<PointDomain.size(); j++)
    {
    varName = PointDomain[j].toString();
    strs.clear();
    strs.append(varName);
    item = new pqTreeWidgetItemObject(VariablesTree, strs);
    item->setData(0, Qt::ToolTipRole, varName);
    item->setData(0, Qt::DecorationRole, pointPixmap);
    this->propertyManager()->registerLink(item, 
                                      "checked", 
                                      SIGNAL(checkedStateChanged(bool)),
                                      this->proxy()->getProxy(), NodeProperty, j);

    if(PointDomain[j].toString() == "DISPL")
      {
      this->DisplItem = item;
      }
    }




  if(this->DisplItem)
    {
    QObject::connect(this->DisplItem, SIGNAL(checkedStateChanged(bool)),
                     this, SLOT(displChanged(bool)));

    // connect the apply displacements check box with the "DISPL" node variable
    QCheckBox* ApplyDisp = this->UI->ApplyDisplacements;
    QObject::connect(ApplyDisp, SIGNAL(stateChanged(int)),
                     this, SLOT(applyDisplacements(int)));
    this->applyDisplacements(Qt::Checked);
    ApplyDisp->setEnabled(true);
    }
  else
    {
    QCheckBox* ApplyDisp = this->UI->ApplyDisplacements;
    this->applyDisplacements(Qt::Unchecked);
    ApplyDisp->setEnabled(false);
    }

  // we hook up the sideset/nodeset 
  QTreeWidget* SetsTree = this->UI->Sets;
  new pqTreeWidgetCheckHelper(SetsTree, 0, this);

  // do the sidesets
  vtkSMProperty* SideProperty = this->proxy()->getProxy()->GetProperty("SideSetArrayStatus");
  QList<QVariant> SideDomain;
  SideDomain = pqSMAdaptor::getSelectionPropertyDomain(SideProperty);
  for(j=0; j<SideDomain.size(); j++)
    {
    varName = SideDomain[j].toString();
    strs.clear();
    strs.append(varName);
    item = new pqTreeWidgetItemObject(SetsTree, strs);
    item->setData(0, Qt::ToolTipRole, varName);
    item->setData(0, Qt::DecorationRole, sideSetPixmap);
    this->propertyManager()->registerLink(item, 
                                      "checked", 
                                      SIGNAL(checkedStateChanged(bool)),
                                      this->proxy()->getProxy(), SideProperty, j);
    }
  
  // do the nodesets
  vtkSMProperty* NSProperty = this->proxy()->getProxy()->GetProperty("NodeSetArrayStatus");
  QList<QVariant> NSDomain;
  NSDomain = pqSMAdaptor::getSelectionPropertyDomain(NSProperty);
  for(j=0; j<NSDomain.size(); j++)
    {
    varName = NSDomain[j].toString();
    strs.clear();
    strs.append(varName);
    item = new pqTreeWidgetItemObject(SetsTree, strs);
    item->setData(0, Qt::ToolTipRole, varName);
    item->setData(0, Qt::DecorationRole, nodeSetPixmap);
    this->propertyManager()->registerLink(item, 
                                      "checked", 
                                      SIGNAL(checkedStateChanged(bool)),
                                      this->proxy()->getProxy(), NSProperty, j);
    }

  // update ranges to begin with
  this->updateDataRanges();

  QAction* a;
  
  QTreeWidget* BlockTree = this->UI->BlockArrayStatus;
  new pqTreeWidgetCheckHelper(BlockTree, 0, this);
  a = new QAction("All Blocks On", BlockTree);
  a->setObjectName("BlocksOn");
  QObject::connect(a, SIGNAL(triggered(bool)), BlockTree, SLOT(allOn()));
  BlockTree->addAction(a);
  a = new QAction("All Blocks Off", BlockTree);
  a->setObjectName("BlocksOff");
  QObject::connect(a, SIGNAL(triggered(bool)), BlockTree, SLOT(allOff()));
  BlockTree->addAction(a);
  BlockTree->setContextMenuPolicy(Qt::ActionsContextMenu);
  
  a = new QAction("All Sets On", SetsTree);
  a->setObjectName("SetsOn");
  QObject::connect(a, SIGNAL(triggered(bool)), SetsTree, SLOT(allOn()));
  SetsTree->addAction(a);
  a = new QAction("All Sets Off", SetsTree);
  a->setObjectName("SetsOff");
  QObject::connect(a, SIGNAL(triggered(bool)), SetsTree, SLOT(allOff()));
  SetsTree->addAction(a);
  SetsTree->setContextMenuPolicy(Qt::ActionsContextMenu);

  a = new QAction("All Variables On", VariablesTree);
  a->setObjectName("VariablesOn");
  QObject::connect(a, SIGNAL(triggered(bool)), VariablesTree, SLOT(allOn()));
  VariablesTree->addAction(a);
  a = new QAction("All Variables Off", VariablesTree);
  a->setObjectName("VariablesOff");
  QObject::connect(a, SIGNAL(triggered(bool)), VariablesTree, SLOT(allOff()));
  VariablesTree->addAction(a);
  VariablesTree->setContextMenuPolicy(Qt::ActionsContextMenu);
}
  
void pqExodusPanel::applyDisplacements(int state)
{
  if(state == Qt::Checked && this->DisplItem)
    {
    this->DisplItem->setCheckState(0, Qt::Checked);
    }
  this->UI->DisplacementMagnitude->setEnabled(state == Qt::Checked ? 
                                                  true : false);
}

void pqExodusPanel::displChanged(bool state)
{
  if(!state)
    {
    QCheckBox* ApplyDisp = this->UI->ApplyDisplacements;
    ApplyDisp->setCheckState(Qt::Unchecked);
    }
}

QString pqExodusPanel::formatDataFor(vtkPVArrayInformation* ai)
{
  QString info;
  if(ai)
    {
    int numComponents = ai->GetNumberOfComponents();
    int dataType = ai->GetDataType();
    double range[2];
    for(int i=0; i<numComponents; i++)
      {
      ai->GetComponentRange(i, range);
      QString s;
      if(dataType != VTK_VOID && dataType != VTK_FLOAT && 
         dataType != VTK_DOUBLE)
        {
        // display as integers (capable of 64 bit ids)
        qlonglong min = qRound64(range[0]);
        qlonglong max = qRound64(range[1]);
        s = QString("%1 - %2").arg(min).arg(max);
        }
      else
        {
        // display as reals
        double min = range[0];
        double max = range[1];
        s = QString("%1 - %2").arg(min,0,'f',6).arg(max,0,'f',6);
        }
      if(i > 0)
        {
        info += ", ";
        }
      info += s;
      }
    }
  else
    {
    info = "Unavailable";
    }
  return info;
}

void pqExodusPanel::updateDataRanges()
{
  this->DataUpdateInProgress = false;

  // update data information about loaded arrays

  vtkSMSourceProxy* sp =
    vtkSMSourceProxy::SafeDownCast(this->proxy()->getProxy());
  vtkPVDataSetAttributesInformation* pdi = 0;
  vtkPVDataSetAttributesInformation* cdi = 0;
  if (sp->GetNumberOfParts() > 0)
    {
    vtkPVDataInformation* di = sp->GetDataInformation();
    pdi = di->GetPointDataInformation();
    cdi = di->GetCellDataInformation();
    }
  vtkPVArrayInformation* ai;
  
  QTreeWidget* VariablesTree = this->UI->Variables;
  pqTreeWidgetItemObject* item;
  QString dataString;
  
  // block ids
  item = static_cast<pqTreeWidgetItemObject*>(VariablesTree->topLevelItem(0));
  ai = 0;
  if (cdi)
    {
    ai = cdi->GetArrayInformation("BlockId");
    }
  dataString = this->formatDataFor(ai);
  item->setData(1, Qt::DisplayRole, dataString);
  item->setData(1, Qt::ToolTipRole, dataString);

  
  // remove global element id
  item = static_cast<pqTreeWidgetItemObject*>(VariablesTree->topLevelItem(1));
  ai = 0;
  if (cdi)
    {
    ai = cdi->GetArrayInformation("GlobalElementId");
    }
  dataString = this->formatDataFor(ai);
  item->setData(1, Qt::DisplayRole, dataString);
  item->setData(1, Qt::ToolTipRole, dataString);

  const int CellOffset = 2;
  
  // do the cell variables
  vtkSMProperty* CellProperty = this->proxy()->getProxy()->GetProperty("CellArrayStatus");
  QList<QVariant> CellDomain;
  CellDomain = pqSMAdaptor::getSelectionPropertyDomain(CellProperty);
  int j;
  for(j=0; j<CellDomain.size(); j++)
    {
    item = static_cast<pqTreeWidgetItemObject*>(
                  VariablesTree->topLevelItem(j+CellOffset));
    ai = 0;
    if (cdi)
      {
      ai = cdi->GetArrayInformation(CellDomain[j].toString().toAscii().data());
      }
    dataString = this->formatDataFor(ai);
    item->setData(1, Qt::DisplayRole, dataString);
    item->setData(1, Qt::ToolTipRole, dataString);
    }
  
  // remove global node id
  item = static_cast<pqTreeWidgetItemObject*>(
        VariablesTree->topLevelItem(CellOffset + CellDomain.size()));
  ai = 0;
  if (pdi)
    {
    ai = pdi->GetArrayInformation("GlobalNodeId");
    }
  dataString = this->formatDataFor(ai);
  item->setData(1, Qt::DisplayRole, dataString);
  item->setData(1, Qt::ToolTipRole, dataString);

  const int PointOffset = 1;
  // do the node variables
  vtkSMProperty* NodeProperty = this->proxy()->getProxy()->GetProperty("PointArrayStatus");
  QList<QVariant> PointDomain;
  PointDomain = pqSMAdaptor::getSelectionPropertyDomain(NodeProperty);

  for(j=0; j<PointDomain.size(); j++)
    {
    item = static_cast<pqTreeWidgetItemObject*>(
        VariablesTree->topLevelItem(j + CellOffset + 
                                    PointOffset + CellDomain.size()));
    ai = 0;
    if (pdi)
      {
      ai = pdi->GetArrayInformation(PointDomain[j].toString().toAscii().data());
      }
    dataString = this->formatDataFor(ai);
    item->setData(1, Qt::DisplayRole, dataString);
    item->setData(1, Qt::ToolTipRole, dataString);
    }
}


void pqExodusPanel::propertyChanged()
{
  if(this->DataUpdateInProgress == false)
    {
    this->DataUpdateInProgress = true;
    QTimer::singleShot(0, this, SLOT(updateDataRanges()));
    }
}
