/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkPVLODRenderModule.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVLODRenderModule.h"
#include "vtkObjectFactory.h"
#include "vtkTimerLog.h"
#include "vtkCallbackCommand.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkPVProcessModule.h"
#include "vtkSMLODPartDisplay.h"
#include "vtkPVLODPartDisplayInformation.h"
#include "vtkSMPart.h"
#include "vtkPVDataInformation.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVLODRenderModule);
vtkCxxRevisionMacro(vtkPVLODRenderModule, "$Revision: 1.5 $");

//----------------------------------------------------------------------------
vtkPVLODRenderModule::vtkPVLODRenderModule()
{
  this->RenderInterruptsEnabled = 1;
  this->LODThreshold = 5.0;
  this->LODResolution = 50;

  this->TotalVisibleGeometryMemorySize = 0;
  this->TotalVisibleLODMemorySize = 0;

  this->AbortCheckTag = 0;
}

//----------------------------------------------------------------------------
vtkPVLODRenderModule::~vtkPVLODRenderModule()
{
}

//----------------------------------------------------------------------------
void PVLODRenderModuleAbortCheck(vtkObject*, unsigned long, void* arg, void*)
{
  vtkPVRenderModule *me = (vtkPVRenderModule*)arg;

  if (me->GetRenderInterruptsEnabled())
    {
    // Just forward the event along.
    me->InvokeEvent(vtkCommand::AbortCheckEvent, NULL);  
    }
}


//----------------------------------------------------------------------------

void vtkPVLODRenderModule::SetProcessModule(vtkProcessModule *pm)
{
  if (this->RenderWindow)
    {
    this->RenderWindow->RemoveObservers(vtkCommand::AbortCheckEvent);
    }

  this->Superclass::SetProcessModule(pm);

  if (this->RenderWindow)
    {
    vtkCallbackCommand* abc = vtkCallbackCommand::New();
    abc->SetCallback(PVLODRenderModuleAbortCheck);
    abc->SetClientData(this);
    this->RenderWindow->AddObserver(vtkCommand::AbortCheckEvent, abc);
    abc->Delete();
    }
}



//----------------------------------------------------------------------------
vtkSMPartDisplay* vtkPVLODRenderModule::CreatePartDisplay()
{
  vtkSMLODPartDisplay* pDisp;

  pDisp = vtkSMLODPartDisplay::New();
  pDisp->SetProcessModule(vtkPVProcessModule::SafeDownCast(this->GetProcessModule()));
  pDisp->SetLODResolution(this->LODResolution);
  return pDisp;
}

//----------------------------------------------------------------------------
void vtkPVLODRenderModule::InteractiveRender()
{
  this->UpdateAllDisplays();

  // Used in subclass for window subsampling, but not really necessary here.
  //this->RenderWindow->SetDesiredUpdateRate(this->InteractiveUpdateRate);
  this->RenderWindow->SetDesiredUpdateRate(5.0);

  // We need to decide globally whether to use decimated geometry.  
  if (this->GetTotalVisibleGeometryMemorySize() > this->LODThreshold*1000)
    {
    this->ProcessModule->SetGlobalLODFlag(1);
    }
  else
    {
    this->ProcessModule->SetGlobalLODFlag(0);
    }  

  vtkTimerLog::MarkStartEvent("Interactive Render");
  this->RenderWindow->Render();
  vtkTimerLog::MarkEndEvent("Interactive Render");
}

//----------------------------------------------------------------------------
void vtkPVLODRenderModule::SetLODResolution(int resolution)
{
  if( this->LODResolution != resolution )
    {
    vtkObject* object;
    vtkSMLODPartDisplay* partDisp;

    this->LODResolution = resolution;

    this->Displays->InitTraversal();
    while ( (object = this->Displays->GetNextItemAsObject()) )
      {
      partDisp = vtkSMLODPartDisplay::SafeDownCast(object);
      if (partDisp)
        {
        partDisp->SetLODResolution(resolution);
        }
      }
    }
}



//-----------------------------------------------------------------------------
void vtkPVLODRenderModule::ComputeTotalVisibleMemorySize()
{
  vtkObject* object;
  vtkSMLODPartDisplay* pDisp;
  vtkPVLODPartDisplayInformation* info;

  this->TotalVisibleGeometryMemorySize = 0;
  this->TotalVisibleLODMemorySize = 0;
  this->Displays->InitTraversal();
  while ( (object=this->Displays->GetNextItemAsObject()) )
    {
    pDisp = vtkSMLODPartDisplay::SafeDownCast(object);
    if (pDisp && pDisp->GetVisibility())
      {
      info = pDisp->GetLODInformation();
      this->TotalVisibleGeometryMemorySize += info->GetGeometryMemorySize();
      this->TotalVisibleLODMemorySize += info->GetLODGeometryMemorySize();
      }
    }
}


//-----------------------------------------------------------------------------
unsigned long vtkPVLODRenderModule::GetTotalVisibleGeometryMemorySize()
{
  if (this->GetTotalVisibleMemorySizeValid())
    {
    return this->TotalVisibleGeometryMemorySize;
    }

  this->ComputeTotalVisibleMemorySize();
  return this->TotalVisibleGeometryMemorySize;
}


//----------------------------------------------------------------------------
void vtkPVLODRenderModule::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "LODThreshold: " << this->LODThreshold << endl;
  os << indent << "LODResolution: " << this->LODResolution << endl;
}

