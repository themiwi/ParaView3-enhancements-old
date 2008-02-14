/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMMultiProcessRenderView.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMMultiProcessRenderView.h"

#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVDisplayInformation.h"
#include "vtkPVServerInformation.h"
#include "vtkSMProxyManager.h"
#include "vtkSMRepresentationStrategy.h"

vtkCxxRevisionMacro(vtkSMMultiProcessRenderView, "$Revision: 1.6 $");
//----------------------------------------------------------------------------
vtkSMMultiProcessRenderView::vtkSMMultiProcessRenderView()
{
  this->RemoteRenderThreshold = 20.0;
  this->LastCompositingDecision = false;
  this->RemoteRenderAvailable = false;
}

//----------------------------------------------------------------------------
vtkSMMultiProcessRenderView::~vtkSMMultiProcessRenderView()
{
}

//-----------------------------------------------------------------------------
vtkSMRepresentationStrategy* vtkSMMultiProcessRenderView::NewStrategyInternal(
  int dataType)
{
  vtkSMProxyManager* pxm = vtkSMObject::GetProxyManager();
  vtkSMRepresentationStrategy* strategy = 0;

  if (dataType == VTK_POLY_DATA)
    {
    strategy = vtkSMRepresentationStrategy::SafeDownCast(
      pxm->NewProxy("strategies", "PolyDataParallelStrategy"));
    }
  else if (dataType == VTK_UNIFORM_GRID || dataType == VTK_IMAGE_DATA)
    {
    strategy = vtkSMRepresentationStrategy::SafeDownCast(
      pxm->NewProxy("strategies", "UniformGridParallelStrategy"));
    }
  else if (dataType == VTK_UNSTRUCTURED_GRID)
    {
    strategy = vtkSMRepresentationStrategy::SafeDownCast(
      pxm->NewProxy("strategies", "UnstructuredGridParallelStrategy"));
    }
  else
    {
    vtkWarningMacro("This view does not provide a suitable strategy for "
      << dataType);
    }

  return strategy;
}

//----------------------------------------------------------------------------
bool vtkSMMultiProcessRenderView::GetCompositingDecision(
  unsigned long totalMemory, int vtkNotUsed(stillRender))
{
  if (!this->RemoteRenderAvailable)
    {
    // Cannot remote render due to setup issues.
    return false;
    }

  if (static_cast<float>(totalMemory)/1000.0 < this->RemoteRenderThreshold)
    {
    return false; // Local render.
    }

  return true;

}

//----------------------------------------------------------------------------
void vtkSMMultiProcessRenderView::BeginStillRender()
{
  // When BeginStillRender() is called, none of the representations have been
  // updated. However, if any of the visible representations need an update, 
  // then when we call GetVisibileFullResDataSize() we are assured that the
  // representations will atleast be partially updated (until before they start
  // moving the data around) to ensure that correct data sizes are obtained.

  // Find out whether we are going to render with or without compositing.
  // We use the full res data size for this decision.
  this->LastCompositingDecision = 
    this->GetCompositingDecision(this->GetVisibileFullResDataSize(), 1);

  this->SetUseCompositing(this->LastCompositingDecision);

  this->Superclass::BeginStillRender();
}

//----------------------------------------------------------------------------
void vtkSMMultiProcessRenderView::BeginInteractiveRender()
{
  // Give the superclass a chance to decide if it wants to use LOD or not.
  this->Superclass::BeginInteractiveRender();

  // If LOD decision changed, then the superclass would have correctly marked
  // the this->DisplayedDataSizeValid flag so that the next call to
  // GetVisibleDisplayedDataSize() will update the LOD pipeline (atleast
  // partially) if required to obtain correct data sizes.

  this->LastCompositingDecision = 
    this->GetCompositingDecision(this->GetVisibleDisplayedDataSize(), 0);

  this->SetUseCompositing(this->LastCompositingDecision);
}

//----------------------------------------------------------------------------
void vtkSMMultiProcessRenderView::SetUseCompositing(bool usecompositing)
{
  // Update the view information so that all representations/strategies will be
  // made aware of the new UseCompositing state.
  this->Information->Set(USE_COMPOSITING(), usecompositing? 1: 0);
}

//-----------------------------------------------------------------------------
void vtkSMMultiProcessRenderView::EndCreateVTKObjects()
{
  this->Superclass::EndCreateVTKObjects();

  // Check if it's possible to access display on the server side.
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();

  vtkPVServerInformation* serverInfo = pm->GetServerInformation(this->ConnectionID);
  if (serverInfo && !serverInfo->GetRemoteRendering())
    {
    this->RemoteRenderAvailable = false;
    }
  else
    {
    vtkPVDisplayInformation* di = vtkPVDisplayInformation::New();
    pm->GatherInformation(this->ConnectionID, 
      vtkProcessModule::RENDER_SERVER, di, pm->GetProcessModuleID());
    this->RemoteRenderAvailable = (di->GetCanOpenDisplay() == 1);
    di->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkSMMultiProcessRenderView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "RemoteRenderThreshold: " 
    << this->RemoteRenderThreshold << endl;
  os << indent << "RemoteRenderAvailable: " 
    << this->RemoteRenderAvailable << endl;
}


