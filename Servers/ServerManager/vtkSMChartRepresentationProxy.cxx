/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMChartRepresentationProxy.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMChartRepresentationProxy.h"

#include "vtkObjectFactory.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMSourceProxy.h"

vtkStandardNewMacro(vtkSMChartRepresentationProxy);
vtkCxxRevisionMacro(vtkSMChartRepresentationProxy, "$Revision: 1.2 $");
//----------------------------------------------------------------------------
vtkSMChartRepresentationProxy::vtkSMChartRepresentationProxy()
{
}

//----------------------------------------------------------------------------
vtkSMChartRepresentationProxy::~vtkSMChartRepresentationProxy()
{
}

//----------------------------------------------------------------------------
bool vtkSMChartRepresentationProxy::EndCreateVTKObjects()
{
  if (!this->Superclass::EndCreateVTKObjects())
    {
    return false;
    }

  // The reduction type for all chart representation is TABLE_MERGE since charts
  // always deliver tables.
  this->SetReductionType(vtkSMClientDeliveryRepresentationProxy::TABLE_MERGE);

  if (this->GetSubProxy("DummyConsumer"))
    {
    vtkSMPropertyHelper(this->GetSubProxy("DummyConsumer"), "Input").Set(
      this->PreProcessorProxy);
    }
  return true;
}

//----------------------------------------------------------------------------
void vtkSMChartRepresentationProxy::Update(vtkSMViewProxy* view)
{
  this->Superclass::Update(view);

  if (this->GetSubProxy("DummyConsumer"))
    {
    vtkSMProxy* subProxy = this->GetSubProxy("DummyConsumer");
    subProxy->GetProperty("Input")->UpdateDependentDomains();
    }
}

//----------------------------------------------------------------------------
void vtkSMChartRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


