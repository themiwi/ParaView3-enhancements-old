/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMMultiDisplayRenderModuleProxy.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSMMultiDisplayRenderModuleProxy.h"

#include "vtkObjectFactory.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkClientServerStream.h"
#include "vtkClientServerID.h"
#include "vtkProcessModule.h"
#include "vtkPVOptions.h"

vtkStandardNewMacro(vtkSMMultiDisplayRenderModuleProxy);
vtkCxxRevisionMacro(vtkSMMultiDisplayRenderModuleProxy, "$Revision: 1.4 $");
//-----------------------------------------------------------------------------
vtkSMMultiDisplayRenderModuleProxy::vtkSMMultiDisplayRenderModuleProxy()
{
  this->SetDisplayXMLName("MultiDisplay");
}

//-----------------------------------------------------------------------------
vtkSMMultiDisplayRenderModuleProxy::~vtkSMMultiDisplayRenderModuleProxy()
{
}

//-----------------------------------------------------------------------------
void vtkSMMultiDisplayRenderModuleProxy::CreateCompositeManager()
{
  // Created in XML.
  this->GetSubProxy("CompositeManager")->SetServers(vtkProcessModule::CLIENT | 
    vtkProcessModule::RENDER_SERVER);
}

//-----------------------------------------------------------------------------
void vtkSMMultiDisplayRenderModuleProxy::InitializeCompositingPipeline()
{
  if (!this->CompositeManagerProxy)
    {
    return;
    }
  vtkSMIntVectorProperty* ivp;

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();

  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->CompositeManagerProxy->GetProperty("TileDimensions"));
  if (!ivp)
    {
    vtkErrorMacro("Failed to find proeprty TileDimensions on CompositeManagerProxy.");
    return;
    }
  int *tileDim = pm->GetOptions()->GetTileDimensions();
  unsigned int i;
  ivp->SetElements(tileDim);
  this->CompositeManagerProxy->UpdateVTKObjects();

  vtkClientServerStream stream;
  for (i=0; i < this->CompositeManagerProxy->GetNumberOfIDs(); i++)
    {
    stream << vtkClientServerStream::Invoke << pm->GetProcessModuleID()
      << "GetRenderServerSocketController" 
      << pm->GetConnectionClientServerID(this->ConnectionID)
      << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke 
      << this->CompositeManagerProxy->GetID(i)
      << "SetSocketController" << vtkClientServerStream::LastResult
      << vtkClientServerStream::End;
    }
  pm->SendStream(this->ConnectionID,
    this->CompositeManagerProxy->GetServers(), stream, 1);
  
  for (i=0; i < this->CompositeManagerProxy->GetNumberOfIDs(); i++)
    {
    if (pm->GetOptions()->GetClientMode())
      {

      // Clean up this mess !!!!!!!!!!!!!
      // Even a cast to vtkPVClientServerModule would be better than this.
      // How can we syncronize the process modules and render modules?
      stream << vtkClientServerStream::Invoke << pm->GetProcessModuleID()
        << "GetClientMode" << vtkClientServerStream::End;
      stream << vtkClientServerStream::Invoke 
        << this->CompositeManagerProxy->GetID(i) 
        << "SetClientFlag"
        << vtkClientServerStream::LastResult << vtkClientServerStream::End;

      stream << vtkClientServerStream::Invoke
        << this->CompositeManagerProxy->GetID(i) << "SetZeroEmpty" << 0
        << vtkClientServerStream::End;
      }
    else
      {
      stream << vtkClientServerStream::Invoke
        << this->CompositeManagerProxy->GetID(i) << "SetZeroEmpty" << 1
        << vtkClientServerStream::End;     
      }
    stream << vtkClientServerStream::Invoke
      << this->CompositeManagerProxy->GetID(i) << "InitializeSchedule"
      << vtkClientServerStream::End;
    }
  pm->SendStream(this->ConnectionID, 
    this->CompositeManagerProxy->GetServers(), stream);
 
  this->Superclass::InitializeCompositingPipeline();

  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->CompositeManagerProxy->GetProperty("UseCompositing"));
  if (ivp)
    {
    // In multi display mode, the server windows must be shown immediately.
    ivp->SetElement(0, 1); 
    }

  this->CompositeManagerProxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkSMMultiDisplayRenderModuleProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
