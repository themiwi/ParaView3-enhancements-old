/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMTesting.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMTesting.h"

#include "vtkObjectFactory.h"
#include "vtkTesting.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMRenderViewProxy.h"

vtkStandardNewMacro(vtkSMTesting);
vtkCxxRevisionMacro(vtkSMTesting, "$Revision: 1.2 $");
vtkCxxSetObjectMacro(vtkSMTesting, RenderViewProxy, vtkSMRenderViewProxy);
//-----------------------------------------------------------------------------
vtkSMTesting::vtkSMTesting()
{
  this->RenderViewProxy = 0;
  this->Testing = vtkTesting::New();
}

//-----------------------------------------------------------------------------
vtkSMTesting::~vtkSMTesting()
{
  this->SetRenderViewProxy(0);
  this->Testing->Delete();
}

//-----------------------------------------------------------------------------
void vtkSMTesting::AddArgument(const char* arg)
{
  this->Testing->AddArgument(arg);
}

//-----------------------------------------------------------------------------
int vtkSMTesting::RegressionTest(float thresh)
{
  int res = vtkTesting::FAILED;
  if (this->RenderViewProxy)
    {
    this->Testing->SetRenderWindow(this->RenderViewProxy->GetRenderWindow());
    res = this->Testing->RegressionTest(thresh);
    }
  return res;
}

//-----------------------------------------------------------------------------
void vtkSMTesting::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << "RenderViewProxy: " << this->RenderViewProxy << endl;
}
