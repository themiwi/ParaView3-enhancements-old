/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMUnstructuredGridVolumeRepresentationProxy.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMUnstructuredGridVolumeRepresentationProxy.h"

#include "vtkCollection.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVDataInformation.h"
#include "vtkSmartPointer.h"
#include "vtkSMDataTypeDomain.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMRepresentationStrategyVector.h"
#include "vtkPVOpenGLExtensionsInformation.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMRepresentationStrategy.h"
#include "vtkSMSourceProxy.h"

vtkStandardNewMacro(vtkSMUnstructuredGridVolumeRepresentationProxy);
vtkCxxRevisionMacro(vtkSMUnstructuredGridVolumeRepresentationProxy, "$Revision: 1.4 $");
//----------------------------------------------------------------------------
vtkSMUnstructuredGridVolumeRepresentationProxy::vtkSMUnstructuredGridVolumeRepresentationProxy()
{
  this->VolumeFilter = 0;
  this->VolumePTMapper = 0;
  this->VolumeHAVSMapper = 0;
  this->VolumeBunykMapper = 0;
  this->VolumeZSweepMapper = 0;
  this->VolumeActor = 0;
  this->VolumeProperty = 0;

  this->SupportsBunykMapper  = 0;
  this->SupportsZSweepMapper = 0;
  this->SupportsHAVSMapper   = 0;
  this->RenderViewExtensionsTested = 0;

  // This representation supports selection.
  this->SetSelectionSupported(true);
}

//----------------------------------------------------------------------------
vtkSMUnstructuredGridVolumeRepresentationProxy::~vtkSMUnstructuredGridVolumeRepresentationProxy()
{
  this->VolumeFilter = 0;
  this->VolumePTMapper = 0;
  this->VolumeHAVSMapper = 0;
  this->VolumeBunykMapper = 0;
  this->VolumeZSweepMapper = 0;
  this->VolumeActor = 0;
  this->VolumeProperty = 0;

}

//----------------------------------------------------------------------------
void vtkSMUnstructuredGridVolumeRepresentationProxy::Update(vtkSMViewProxy* view)
{
  if (!this->RenderViewExtensionsTested)
    {
    this->UpdateRenderViewExtensions(view);
    }

  this->DetermineVolumeSupport();
  this->SetupVolumePipeline();

  this->Superclass::Update(view);
}

//----------------------------------------------------------------------------
bool vtkSMUnstructuredGridVolumeRepresentationProxy::AddToView(vtkSMViewProxy* view)
{
  vtkSMRenderViewProxy* renderView = vtkSMRenderViewProxy::SafeDownCast(view);
  if (!renderView)
    {
    vtkErrorMacro("View must be a vtkSMRenderViewProxy.");
    return false;
    }

  if (!this->Superclass::AddToView(view))
    {
    return false;
    }

  renderView->AddPropToRenderer(this->VolumeActor);

  // This will ensure that on update we'll check if the
  // view supports certain extensions.
  this->RenderViewExtensionsTested = 0;

  // We don't support HAVS unless we've verified that we do.
  this->SupportsHAVSMapper = 0;

  return true;
}

//----------------------------------------------------------------------------
bool vtkSMUnstructuredGridVolumeRepresentationProxy::RemoveFromView(vtkSMViewProxy* view)
{
  vtkSMRenderViewProxy* renderView = vtkSMRenderViewProxy::SafeDownCast(view);
  if (!renderView)
    {
    vtkErrorMacro("View must be a vtkSMRenderViewProxy.");
    return false;
    }

  renderView->RemovePropFromRenderer(this->VolumeActor);
  return this->Superclass::RemoveFromView(view);
}

//----------------------------------------------------------------------------
bool vtkSMUnstructuredGridVolumeRepresentationProxy::InitializeStrategy(vtkSMViewProxy* view)
{
  // Since we use a geometry filter, the data type fed into the strategy is
  // always polydata.
  vtkSmartPointer<vtkSMRepresentationStrategy> strategy;
  strategy.TakeReference(
    view->NewStrategy(VTK_UNSTRUCTURED_GRID));
  if (!strategy.GetPointer())
    {
    vtkErrorMacro("View could not provide a strategy to use. "
      << "Cannot be rendered in this view of type " << view->GetClassName());
    return false;
    }

  this->AddStrategy(strategy);

  strategy->SetEnableLOD(false);

  // Creates the strategy objects.
  strategy->UpdateVTKObjects();

  // Now initialize the data pipelines involving this strategy.
  // Since representations are not added to views unless their input is set, we
  // can assume that the objects for this proxy have been created.
  // (Look at vtkSMPipelineRepresentationProxy::AddToView()).
  this->Connect(this->VolumeFilter, strategy, "Input");
  
  return this->Superclass::InitializeStrategy(view);
}

//----------------------------------------------------------------------------
bool vtkSMUnstructuredGridVolumeRepresentationProxy::BeginCreateVTKObjects()
{
  if (!this->Superclass::BeginCreateVTKObjects())
    {
    return false;
    }

  // Set server flags correctly on all subproxies.
  this->VolumeFilter = 
    vtkSMSourceProxy::SafeDownCast(this->GetSubProxy("VolumeFilter"));

  this->VolumePTMapper = this->GetSubProxy("VolumePTMapper");
  this->VolumeBunykMapper = this->GetSubProxy("VolumeBunykMapper");
  this->VolumeZSweepMapper = this->GetSubProxy("VolumeZSweepMapper");
  this->VolumeHAVSMapper = this->GetSubProxy("VolumeHAVSMapper");
  this->VolumeActor = this->GetSubProxy("VolumeActor");
  this->VolumeProperty = this->GetSubProxy("VolumeProperty");

  this->VolumeFilter->SetServers(vtkProcessModule::DATA_SERVER);
  this->VolumeBunykMapper->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  this->VolumeZSweepMapper->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  this->VolumePTMapper->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  this->VolumeHAVSMapper->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  this->VolumeActor->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  this->VolumeProperty->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);

  return true;
}

//----------------------------------------------------------------------------
bool vtkSMUnstructuredGridVolumeRepresentationProxy::EndCreateVTKObjects()
{
  this->Connect(this->GetInputProxy(), this->VolumeFilter, "Input");
  this->Connect(this->VolumeBunykMapper, this->VolumeActor, "Mapper");
  this->Connect(this->VolumeHAVSMapper, this->VolumeActor, "Mapper");
  this->Connect(this->VolumePTMapper, this->VolumeActor, "Mapper");
  this->Connect(this->VolumeZSweepMapper, this->VolumeActor, "Mapper");
  this->Connect(this->VolumeProperty, this->VolumeActor, "Property");

  return this->Superclass::EndCreateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkSMUnstructuredGridVolumeRepresentationProxy::DetermineVolumeSupport()
{
  vtkSMDataTypeDomain* domain = vtkSMDataTypeDomain::SafeDownCast(
    this->VolumeFilter->GetProperty("Input")->GetDomain("input_type"));
  if (domain && domain->IsInDomain(this->GetInputProxy()))
    {

    vtkPVDataInformation* datainfo = this->GetInputProxy()->GetDataInformation();
    if (datainfo->GetNumberOfCells() < 1000000)
      {
      this->SupportsZSweepMapper = 1;
      }
    if (datainfo->GetNumberOfCells() < 500000)
      {
      this->SupportsBunykMapper = 1;
      }
    
    // HAVS support is determined when the representation is added to a view
    }
}

//-----------------------------------------------------------------------------
void vtkSMUnstructuredGridVolumeRepresentationProxy::SetupVolumePipeline()
{

  vtkSMProxyProperty* pp;
  pp = vtkSMProxyProperty::SafeDownCast(
    this->VolumeActor->GetProperty("Mapper"));
  if (!pp)
    {
    vtkErrorMacro("Failed to find property Mapper on VolumeActorProxy.");
    return;
    }
  pp->RemoveAllProxies();
  if (this->SupportsHAVSMapper)
    {
    this->Connect(this->RepresentationStrategies->front()->GetOutput(), this->VolumeHAVSMapper);
    pp->AddProxy(this->VolumeHAVSMapper);
    }
  else if(this->SupportsBunykMapper)
    {
    this->Connect(this->RepresentationStrategies->front()->GetOutput(), this->VolumeBunykMapper);
    pp->AddProxy(this->VolumeBunykMapper);
    }
  else if(this->SupportsZSweepMapper)
    {
    this->Connect(this->RepresentationStrategies->front()->GetOutput(), this->VolumeZSweepMapper);
    pp->AddProxy(this->VolumeZSweepMapper);
    }
  else
    {
    this->Connect(this->RepresentationStrategies->front()->GetOutput(), this->VolumePTMapper);
    pp->AddProxy(this->VolumePTMapper);
    }
  this->VolumeActor->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkSMUnstructuredGridVolumeRepresentationProxy::UpdateRenderViewExtensions(
  vtkSMViewProxy* view)
{
  vtkSMRenderViewProxy* rvp = vtkSMRenderViewProxy::SafeDownCast(view);
  if (!rvp)
    {
    return;
    }
  vtkPVOpenGLExtensionsInformation* glinfo =
    rvp->GetOpenGLExtensionsInformation();
  if (glinfo)
    {
    // These are extensions needed for HAVS. It would be nice
    // if these was some way of asking the HAVS mapper the extensions
    // it needs rather than hardcoding it here.
    int supports_GL_EXT_texture3D =
      glinfo->ExtensionSupported( "GL_EXT_texture3D");
    int supports_GL_EXT_framebuffer_object =
      glinfo->ExtensionSupported( "GL_EXT_framebuffer_object");
    int supports_GL_ARB_fragment_program =
      glinfo->ExtensionSupported( "GL_ARB_fragment_program" );
    int supports_GL_ARB_vertex_program =
      glinfo->ExtensionSupported( "GL_ARB_vertex_program" );
    int supports_GL_ARB_texture_float =
      glinfo->ExtensionSupported( "GL_ARB_texture_float" );
    int supports_GL_ATI_texture_float =
      glinfo->ExtensionSupported( "GL_ATI_texture_float" );

    if ( !supports_GL_EXT_texture3D ||
      !supports_GL_EXT_framebuffer_object ||
      !supports_GL_ARB_fragment_program ||
      !supports_GL_ARB_vertex_program ||
      !(supports_GL_ARB_texture_float || supports_GL_ATI_texture_float))
      {
      this->SupportsHAVSMapper = 0;
      }
    else
      {
      this->SupportsHAVSMapper = 1;
      }
    }
  this->RenderViewExtensionsTested = 1;
}

//-----------------------------------------------------------------------------
void vtkSMUnstructuredGridVolumeRepresentationProxy::SetVolumeMapperToBunykCM()
{
  vtkSMProxyProperty* pp;
  pp = vtkSMProxyProperty::SafeDownCast(
    this->VolumeActor->GetProperty("Mapper"));
  if (!pp)
    {
    vtkErrorMacro("Failed to find property Mapper on VolumeActor.");
    return;
    }
  if (pp->GetNumberOfProxies() != 1)
    {
    vtkErrorMacro("Expected one proxy in Mapper's VolumeActor.");
    }
  pp->SetProxy(0, this->VolumeBunykMapper);
  this->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkSMUnstructuredGridVolumeRepresentationProxy::SetVolumeMapperToPTCM()
{
  vtkSMProxyProperty* pp;
  pp = vtkSMProxyProperty::SafeDownCast(
    this->VolumeActor->GetProperty("Mapper"));
  if (!pp)
    {
    vtkErrorMacro("Failed to find property Mapper on VolumeActor.");
    return;
    }
  if (pp->GetNumberOfProxies() != 1)
    {
    vtkErrorMacro("Expected one proxy in Mapper's VolumeActor.");
    }
  pp->SetProxy(0, this->VolumePTMapper);
  this->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkSMUnstructuredGridVolumeRepresentationProxy::SetVolumeMapperToHAVSCM()
{
  vtkSMProxyProperty* pp;
  pp = vtkSMProxyProperty::SafeDownCast(
    this->VolumeActor->GetProperty("Mapper"));
  if (!pp)
    {
    vtkErrorMacro("Failed to find property Mapper on VolumeActor.");
    return;
    }
  if (pp->GetNumberOfProxies() != 1)
    {
    vtkErrorMacro("Expected one proxy in Mapper's VolumeActor.");
    }
  pp->SetProxy(0, this->VolumeHAVSMapper);
  this->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkSMUnstructuredGridVolumeRepresentationProxy::SetVolumeMapperToZSweepCM()
{
  vtkSMProxyProperty* pp;
  pp = vtkSMProxyProperty::SafeDownCast(
    this->VolumeActor->GetProperty("Mapper"));
  if (!pp)
    {
    vtkErrorMacro("Failed to find property Mapper on VolumeActor.");
    return;
    }
  if (pp->GetNumberOfProxies() != 1)
    {
    vtkErrorMacro("Expected one proxy in Mapper's VolumeActor.");
    }
  pp->SetProxy(0, this->VolumeZSweepMapper);
  this->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
int vtkSMUnstructuredGridVolumeRepresentationProxy::GetVolumeMapperTypeCM()
{ 
  vtkSMProxyProperty* pp;
  pp = vtkSMProxyProperty::SafeDownCast(
    this->VolumeActor->GetProperty("Mapper"));
  if (!pp)
    {
    vtkErrorMacro("Failed to find property Mapper on VolumeActor.");
    return vtkSMUnstructuredGridVolumeRepresentationProxy::UNKNOWN_VOLUME_MAPPER;
    }
  
  vtkSMProxy *p = pp->GetProxy(0);
  
  if ( !p )
    {
    vtkErrorMacro("Failed to find proxy in Mapper proxy property!");
    return vtkSMUnstructuredGridVolumeRepresentationProxy::UNKNOWN_VOLUME_MAPPER;
    }
  
  if ( !strcmp(p->GetVTKClassName(), "vtkProjectedTetrahedraMapper" ) )
    {
    return vtkSMUnstructuredGridVolumeRepresentationProxy::PROJECTED_TETRA_VOLUME_MAPPER;
    }

  if ( !strcmp(p->GetVTKClassName(), "vtkHAVSVolumeMapper" ) )
    {
    return vtkSMUnstructuredGridVolumeRepresentationProxy::HAVS_VOLUME_MAPPER;
    }

  if ( !strcmp(p->GetVTKClassName(), "vtkUnstructuredGridVolumeZSweepMapper" ) )
    {
    return vtkSMUnstructuredGridVolumeRepresentationProxy::ZSWEEP_VOLUME_MAPPER;
    }
  
  if ( !strcmp(p->GetVTKClassName(), "vtkUnstructuredGridVolumeRayCastMapper" ) )
    {
    return vtkSMUnstructuredGridVolumeRepresentationProxy::BUNYK_RAY_CAST_VOLUME_MAPPER;
    }
  
  return vtkSMUnstructuredGridVolumeRepresentationProxy::UNKNOWN_VOLUME_MAPPER;
}

//----------------------------------------------------------------------------
void vtkSMUnstructuredGridVolumeRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "VolumeFilterProxy: " << this->VolumeFilter << endl;
  os << indent << "VolumePropertyProxy: " << this->VolumeProperty << endl;
  os << indent << "VolumeActorProxy: " << this->VolumeActor << endl;
  os << indent << "SupportsHAVSMapper: " << this->SupportsHAVSMapper << endl;
  os << indent << "SupportsBunykMapper: " << this->SupportsBunykMapper << endl;
  os << indent << "SupportsZSweepMapper: " << this->SupportsZSweepMapper << endl;
  os << indent << "RenderViewExtensionsTested: " << this->RenderViewExtensionsTested << endl;
}


