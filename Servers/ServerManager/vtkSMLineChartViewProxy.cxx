/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMLineChartViewProxy.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMLineChartViewProxy.h"

#include "vtkObjectFactory.h"

#include "vtkQtLineChartView.h"
#include "vtkQtChartWidget.h"
#include "vtkQtChartMouseSelection.h"
#include "vtkQtChartInteractorSetup.h"
#include "vtkSMChartOptionsProxy.h"

vtkStandardNewMacro(vtkSMLineChartViewProxy);
vtkCxxRevisionMacro(vtkSMLineChartViewProxy, "$Revision: 1.4 $");
//----------------------------------------------------------------------------
vtkSMLineChartViewProxy::vtkSMLineChartViewProxy()
{
  this->ChartView = 0;
}

//----------------------------------------------------------------------------
vtkSMLineChartViewProxy::~vtkSMLineChartViewProxy()
{
  if (this->ChartView)
    {
    this->ChartView->Delete();
    this->ChartView = 0;
    }
}

//----------------------------------------------------------------------------
void vtkSMLineChartViewProxy::CreateVTKObjects()
{
  if (this->ObjectsCreated)
    {
    return;
    }

  this->ChartView = vtkQtLineChartView::New();

  // Set up the paraview style interactor.
  vtkQtChartArea* area = this->ChartView->GetChartArea();
  vtkQtChartMouseSelection* selector =
    vtkQtChartInteractorSetup::createSplitZoom(area);
  this->ChartView->AddChartSelectionHandlers(selector);

  // Set default color scheme to blues
  this->ChartView->SetColorSchemeToBlues();

  vtkSMChartOptionsProxy::SafeDownCast(
    this->GetSubProxy("ChartOptions"))->SetChartView(
    this->GetLineChartView());

  this->Superclass::CreateVTKObjects();
}

//----------------------------------------------------------------------------
vtkQtChartWidget* vtkSMLineChartViewProxy::GetChartWidget()
{
  return qobject_cast<vtkQtChartWidget*>(this->ChartView->GetWidget());
}

//----------------------------------------------------------------------------
vtkQtLineChartView* vtkSMLineChartViewProxy::GetLineChartView()
{
  return this->ChartView;
}

//----------------------------------------------------------------------------
void vtkSMLineChartViewProxy::SetHelpFormat(const char* format)
{
  this->ChartView->SetHelpFormat(format);
}

//----------------------------------------------------------------------------
void vtkSMLineChartViewProxy::PerformRender()
{
  this->ChartView->Update();
  this->ChartView->Render();
}

//----------------------------------------------------------------------------
void vtkSMLineChartViewProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


