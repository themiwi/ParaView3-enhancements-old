/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkPVContourFilter.cxx,v $
  Language:  C++
  Date:      $Date: 2000-07-11 19:57:30 $
  Version:   $Revision: 1.1 $

Copyright (c) 1998-2000 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.

All rights reserved. No part of this software may be reproduced, distributed,
or modified, in any form or by any means, without permission in writing from
Kitware Inc.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

=========================================================================*/

#include "vtkPVContourFilter.h"
#include "vtkKWApplication.h"
#include "vtkKWView.h"
#include "vtkKWRenderView.h"
#include "vtkPVComposite.h"

int vtkPVContourFilterCommand(ClientData cd, Tcl_Interp *interp,
			      int argc, char *argv[]);

vtkPVContourFilter::vtkPVContourFilter()
{
  this->CommandFunction = vtkPVContourFilterCommand;
  
  this->Label = vtkKWLabel::New();
  this->Label->SetParent(this);
  this->Accept = vtkKWWidget::New();
  this->Accept->SetParent(this);
  this->ContourValueEntry = vtkKWEntry::New();
  this->ContourValueEntry->SetParent(this);
  this->ContourValueLabel = vtkKWLabel::New();
  this->ContourValueLabel->SetParent(this);
  
  this->Contour = vtkContourFilter::New();
}

vtkPVContourFilter::~vtkPVContourFilter()
{
  this->Label->Delete();
  this->Label = NULL;
  
  this->Accept->Delete();
  this->Accept = NULL;
  
  this->ContourValueEntry->Delete();
  this->ContourValueEntry = NULL;
  this->ContourValueLabel->Delete();
  this->ContourValueLabel = NULL;
  
  this->Contour->Delete();
  this->Contour = NULL;
}

vtkPVContourFilter* vtkPVContourFilter::New()
{
  return new vtkPVContourFilter();
}

void vtkPVContourFilter::Create(vtkKWApplication *app, char *args)
{
  // must set the application
  if (this->Application)
    {
    vtkErrorMacro("vtkPVContourFilter already created");
    return;
    }
  this->SetApplication(app);
  
  // create the top level
  this->Script("frame %s %s", this->GetWidgetName(), args);
  
  this->Label->Create(this->Application, "");
  this->Label->SetLabel("vtkContourFilter label");
  
  this->Script("pack %s", this->Label->GetWidgetName());
  
  this->ContourValueLabel->Create(this->Application, "");
  this->ContourValueLabel->SetLabel("Contour Value:");
  this->ContourValueEntry->Create(this->Application, "");
  this->ContourValueEntry->SetValue(this->GetContour()->GetValue(0), 2);
  
  this->Accept->Create(this->Application, "button", "-text Accept");
  this->Accept->SetCommand(this, "ContourValueChanged");
  this->Script("pack %s", this->Accept->GetWidgetName());
  this->Script("pack %s %s -side left -anchor w",
	       this->ContourValueLabel->GetWidgetName(),
	       this->ContourValueEntry->GetWidgetName());
}

void vtkPVContourFilter::ContourValueChanged()
{  
  this->Contour->SetValue(0, this->ContourValueEntry->GetValueAsFloat());
  this->Contour->Modified();
  this->Contour->Update();
  
  this->Composite->GetView()->Render();
}


