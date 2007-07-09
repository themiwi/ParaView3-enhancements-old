/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMAnimationSceneGeometryWriter.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMAnimationSceneGeometryWriter.h"

#include "vtkObjectFactory.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMXMLPVAnimationWriterProxy.h"

vtkStandardNewMacro(vtkSMAnimationSceneGeometryWriter);
vtkCxxRevisionMacro(vtkSMAnimationSceneGeometryWriter, "$Revision: 1.2 $");
vtkCxxSetObjectMacro(vtkSMAnimationSceneGeometryWriter, ViewModule, vtkSMProxy);

//-----------------------------------------------------------------------------
vtkSMAnimationSceneGeometryWriter::vtkSMAnimationSceneGeometryWriter()
{
  this->GeometryWriter = 0;
  this->ViewModule = 0;
}

//-----------------------------------------------------------------------------
vtkSMAnimationSceneGeometryWriter::~vtkSMAnimationSceneGeometryWriter()
{
  this->SetViewModule(0);
  if (this->GeometryWriter)
    {
    this->GeometryWriter->Delete();
    this->GeometryWriter = 0;
    }
}

//-----------------------------------------------------------------------------
bool vtkSMAnimationSceneGeometryWriter::SaveInitialize()
{
  if (!this->ViewModule)
    {
    vtkErrorMacro("No view from which to save the geometry is set.");
    return false;
    }

  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  this->GeometryWriter = vtkSMXMLPVAnimationWriterProxy::SafeDownCast(
    pxm->NewProxy("writers","XMLPVAnimationWriter"));
  this->GeometryWriter->SetConnectionID(this->ViewModule->GetConnectionID());

  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(
    this->GeometryWriter->GetProperty("FileName"));
  svp->SetElement(0, this->FileName);
  this->GeometryWriter->UpdateVTKObjects();

  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    this->ViewModule->GetProperty("Displays"));
/*  for (unsigned int cc=0; cc < pp->GetNumberOfProxies(); ++cc)
    {
    vtkSMDataObjectDisplayProxy* display = 
      vtkSMDataObjectDisplayProxy::SafeDownCast(
        pp->GetProxy(cc));
    if (display && display->GetVisibilityCM())
      {
      display->SetInputAsGeometryFilter(this->GeometryWriter);
      }
      } */
  this->GeometryWriter->UpdateVTKObjects();
  this->GeometryWriter->InvokeCommand("Start");
  return true;
}

//-----------------------------------------------------------------------------
bool vtkSMAnimationSceneGeometryWriter::SaveFrame(double time)
{
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->GeometryWriter->GetProperty("WriteTime"));
  dvp->SetElement(0, time);
  this->GeometryWriter->UpdateProperty("WriteTime", 1);

  if (this->GeometryWriter->GetErrorCode())
    {
    return false;
    }
  return true;
}

//-----------------------------------------------------------------------------
bool vtkSMAnimationSceneGeometryWriter::SaveFinalize()
{
  if (!this->GeometryWriter)
    {
    return true;
    }

  this->GeometryWriter->InvokeCommand("Finish");
  this->GeometryWriter->Delete();
  this->GeometryWriter = 0;
  return true;
}

//-----------------------------------------------------------------------------
void vtkSMAnimationSceneGeometryWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ViewModule: " << this->ViewModule << endl;
}

