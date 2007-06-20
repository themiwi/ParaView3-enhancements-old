/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMXYPlotActorProxy.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSMXYPlotActorProxy.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkClientServerStream.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkClientServerID.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMPart.h"
#include "vtkMath.h"

#include <vtkstd/vector>
#include <vtkstd/string>

vtkStandardNewMacro(vtkSMXYPlotActorProxy);
vtkCxxRevisionMacro(vtkSMXYPlotActorProxy, "$Revision: 1.11 $");
vtkCxxSetObjectMacro(vtkSMXYPlotActorProxy, Input, vtkSMSourceProxy);

class vtkSMXYPlotActorProxyInternals
{
public:
  typedef vtkstd::vector<vtkstd::string> VectorOfStrings;
  VectorOfStrings ArrayNames;
};

//-----------------------------------------------------------------------------
vtkSMXYPlotActorProxy::vtkSMXYPlotActorProxy()
{
  this->Input = 0;
  this->OutputPort = 0;
  this->Internals = new vtkSMXYPlotActorProxyInternals;
  this->SetExecutiveName(0);
  this->Smart = 1;
}

//-----------------------------------------------------------------------------
vtkSMXYPlotActorProxy::~vtkSMXYPlotActorProxy()
{
  this->SetInput(0);
  delete this->Internals;
}

//-----------------------------------------------------------------------------
void vtkSMXYPlotActorProxy::RemoveAllArrayNames()
{
  this->Internals->ArrayNames.clear();
  this->ArrayNamesModified = 1;
}

//-----------------------------------------------------------------------------
void vtkSMXYPlotActorProxy::AddArrayName(const char* arrayname)
{
  this->Internals->ArrayNames.push_back(vtkstd::string(arrayname));
  this->ArrayNamesModified = 1;
}

//-----------------------------------------------------------------------------
void vtkSMXYPlotActorProxy::SetPosition(double x, double y)
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream stream;
  vtkClientServerID sourceID = this->GetID();
  stream << vtkClientServerStream::Invoke
         << sourceID  << "GetPositionCoordinate"
         << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke
         << vtkClientServerStream::LastResult << "SetValue"
         << x << y << 0.0
         << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, this->GetServers(), stream);
}

//-----------------------------------------------------------------------------
void vtkSMXYPlotActorProxy::SetPosition2(double x, double y)
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream stream;
  vtkClientServerID sourceID = this->GetID(); 
  stream << vtkClientServerStream::Invoke
         << sourceID  << "GetPosition2Coordinate"
         << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke
         << vtkClientServerStream::LastResult << "SetValue"
         << x << y << 0.0
         << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, this->GetServers(), stream);
}

//-----------------------------------------------------------------------------
void vtkSMXYPlotActorProxy::UpdateVTKObjects()
{
  this->Superclass::UpdateVTKObjects();
  if (this->ArrayNamesModified && this->Input)
    {
    this->ArrayNamesModified = 0;
    this->SetupInputs(); 
    }
}
  
//-----------------------------------------------------------------------------
void vtkSMXYPlotActorProxy::SetupInputs()
{
  if (!this->Input)
    {
    return;
    }

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream stream;
  vtkClientServerID sourceID = this->GetID();

  stream << vtkClientServerStream::Invoke
         << sourceID << "RemoveAllInputs"
         << vtkClientServerStream::End;


  int total_numArrays = this->Internals->ArrayNames.size();
  const char* arrayname = 0;
  if (total_numArrays == 0)
    {
    pm->SendStream(this->ConnectionID, this->GetServers(), stream);
    return;
    }

  // To assign unique plot color to each array.
  double color_step = 1.0 / total_numArrays;
  double color = 0;

  int arrayCount = 0;
  
  vtkSMXYPlotActorProxyInternals::VectorOfStrings::iterator iter;
  for (iter = this->Internals->ArrayNames.begin(); 
    iter != this->Internals->ArrayNames.end(); ++iter)
    {
    arrayname = (*iter).c_str();
    vtkSMPart* part = this->Input->GetPart(this->OutputPort);
    stream << vtkClientServerStream::Invoke
           << part->GetProducerID()
           << "GetOutputDataObject"
           << part->GetPortIndex()
           << vtkClientServerStream::End
           << vtkClientServerStream::Invoke
           << sourceID << "AddInput"
           << vtkClientServerStream::LastResult
           << arrayname << 0 /*component no*/
           << vtkClientServerStream::End;

    stream << vtkClientServerStream::Invoke
           << sourceID << "SetPlotLabel" << arrayCount << arrayname
           << vtkClientServerStream::End;
    
    if (this->Smart)
      {
      double r, g , b;
      vtkMath::HSVToRGB(color, 1.0, 1.0, &r, &g, &b);

      stream << vtkClientServerStream::Invoke
        << sourceID << "SetPlotColor"
        << arrayCount << r << g << b 
        << vtkClientServerStream::End;
      }

    color += color_step;
    arrayCount++;
    }

  if (this->Smart)
    {
    vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
      this->GetProperty("LegendVisibility"));
    if (ivp)
      {
      ivp->SetElement(0, ( arrayCount > 1 ? 1 : 0));
      }
    else
      {
      vtkErrorMacro("Failed to find property LegendVisibility.");
      }
    }
  if (arrayCount == 1)
    {
    stream << vtkClientServerStream::Invoke
      << sourceID << "SetYTitle" << arrayname 
      << vtkClientServerStream::End;
    if (this->Smart)
      {
      stream << vtkClientServerStream::Invoke
        << sourceID << "SetPlotColor" << 0 << 1 << 1 << 1
        << vtkClientServerStream::End;
      }
    }
  pm->SendStream(this->ConnectionID, this->GetServers(), stream);
  this->UpdateVTKObjects(); // this is required for LegendVisibility. 
}

//-----------------------------------------------------------------------------
void vtkSMXYPlotActorProxy::AddInput(unsigned int,
                                     vtkSMSourceProxy* input,
                                     unsigned int outputPort,
                                     const char*)
{
  if (!input)
    {
    return;
    }
  input->CreateParts();
  this->SetInput(input);
  this->OutputPort = outputPort;
  this->CreateVTKObjects();
  this->ArrayNamesModified = 1;
}

//-----------------------------------------------------------------------------
void vtkSMXYPlotActorProxy::CleanInputs(const char* command)
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream stream;
  vtkClientServerID sourceID = this->GetID();

  stream << vtkClientServerStream::Invoke
    << sourceID << command << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, this->GetServers(), stream);
  this->ArrayNamesModified = 1;
  this->SetInput(0);
  
}

//-----------------------------------------------------------------------------
void vtkSMXYPlotActorProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ArrayNamesModified: " << this->ArrayNamesModified << endl;
  os << indent << "Input: " << this->Input << endl;
  os << indent << "Smart: " << this->Smart << endl;
}
