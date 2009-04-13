/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqLineChartView.cxx,v $

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2. 

   See License_v1.2.txt for the full ParaView license.
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

========================================================================*/
#include "pqLineChartView.h"

// Server Manager Includes.
#include "vtkSMProperty.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMLineChartViewProxy.h"
#include "vtkSMChartRepresentationProxy.h"
#include "vtkPVDataInformation.h"

#include "vtkEventQtSlotConnect.h"
#include "vtkTable.h"
#include "vtkSmartPointer.h"
#include "vtkQtLineChartView.h"
#include "vtkQtChartArea.h"
#include "vtkQtChartAxis.h"
#include "vtkQtChartAxisLayer.h"
#include "vtkQtChartAxisModel.h"
#include "vtkQtChartContentsSpace.h"
#include "vtkQtChartInteractorSetup.h"
#include "vtkQtChartWidget.h"
#include "vtkQtChartSeriesModelCollection.h"
#include "vtkQtChartRepresentation.h"

// ParaView Includes.
#include "pqLineChartRepresentation.h"
#include "pqOutputPort.h"
#include "pqPipelineSource.h"
#include "pqServer.h"
#include "pqSMAdaptor.h"

// Qt Includes.
#include <QDebug>

//-----------------------------------------------------------------------------
class pqLineChartView::pqInternal
{
public:
  pqInternal()
    {
    }

  ~pqInternal()
    {

    }

  vtkSmartPointer<vtkQtLineChartView>                   LineChartView;
};

//-----------------------------------------------------------------------------
pqLineChartView::pqLineChartView(const QString& group,
                               const QString& name, 
                               vtkSMViewProxy* viewModule,
                               pqServer* server, 
                               QObject* parent/*=NULL*/):
  pqView(lineChartViewType(), group, name, viewModule, server, parent)
{
  viewModule->GetID(); // this results in calling CreateVTKObjects().
  this->Internal = new pqInternal();
  this->Internal->LineChartView = vtkSMLineChartViewProxy::SafeDownCast(
    viewModule)->GetLineChartView();

  // Set up the view undo/redo.
  vtkQtChartContentsSpace *contents =
    this->getVtkLineChartView()->GetChartArea()->getContentsSpace();
  this->connect(contents, SIGNAL(historyPreviousAvailabilityChanged(bool)),
    this, SIGNAL(canUndoChanged(bool)));
  this->connect(contents, SIGNAL(historyNextAvailabilityChanged(bool)),
    this, SIGNAL(canRedoChanged(bool)));
}

//-----------------------------------------------------------------------------
pqLineChartView::~pqLineChartView()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
QWidget* pqLineChartView::getWidget()
{
  return this->getVtkLineChartView()->GetWidget();
}

//-----------------------------------------------------------------------------
vtkQtLineChartView* pqLineChartView::getVtkLineChartView() const
{
  return this->Internal->LineChartView;
}

//-----------------------------------------------------------------------------
void pqLineChartView::setDefaultPropertyValues()
{
  pqView::setDefaultPropertyValues();
}

//-----------------------------------------------------------------------------
void pqLineChartView::undo()
{
  vtkQtChartArea* area = this->getVtkLineChartView()->GetChartArea();
  area->getContentsSpace()->historyPrevious();
}

//-----------------------------------------------------------------------------
void pqLineChartView::redo()
{
  vtkQtChartArea* area = this->getVtkLineChartView()->GetChartArea();
  area->getContentsSpace()->historyNext();
}

//-----------------------------------------------------------------------------
bool pqLineChartView::canUndo() const
{
  vtkQtChartArea* area = this->getVtkLineChartView()->GetChartArea();
  return area->getContentsSpace()->isHistoryPreviousAvailable();
}

//-----------------------------------------------------------------------------
bool pqLineChartView::canRedo() const
{
  vtkQtChartArea* area = this->getVtkLineChartView()->GetChartArea();
  return area->getContentsSpace()->isHistoryNextAvailable();
}

//-----------------------------------------------------------------------------
void pqLineChartView::resetDisplay()
{
  vtkQtChartArea* area = this->getVtkLineChartView()->GetChartArea();
  area->getContentsSpace()->resetZoom();
}

//-----------------------------------------------------------------------------
bool pqLineChartView::canDisplay(pqOutputPort* opPort) const
{
  pqPipelineSource* source = opPort? opPort->getSource() :0;
  vtkSMSourceProxy* sourceProxy = source ? 
    vtkSMSourceProxy::SafeDownCast(source->getProxy()) : 0;
  if(!opPort || !source ||
     opPort->getServer()->GetConnectionID() !=
     this->getServer()->GetConnectionID() || !sourceProxy ||
     sourceProxy->GetOutputPortsCreated()==0)
    {
    return false;
    }

  vtkPVDataInformation* dataInfo = opPort->getDataInformation(true);
  return (dataInfo && dataInfo->DataSetTypeIsA("vtkDataObject"));
}

