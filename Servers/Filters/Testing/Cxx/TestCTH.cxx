/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: TestCTH.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCTHFractal.h"
#include "vtkCTHData.h"
#include "vtkPlane.h"
#include "vtkCTHExtractAMRPart.h"
#include "vtkCTHOutlineFilter.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRegressionTestImage.h"

int main(int argc, char * argv[])
{
  vtkCTHFractal *fractal = vtkCTHFractal::New();
  fractal->SetDimensions( 10 );
  fractal->SetFractalValue( 9.5 );
  fractal->SetMaximumLevel( 5 );
  fractal->SetGhostLevels( 0 );
  fractal->Update();
  
  vtkCTHData* data = fractal->GetOutput();

  // Just for coverage:
  double origin[3];
  data->GetTopLevelOrigin ( origin );
  data->Print( cout );
  
  vtkPlane *clipPlane = vtkPlane::New();
  clipPlane->SetNormal ( 1, 1, 1);
  clipPlane->SetOrigin ( origin );

  vtkCTHExtractAMRPart *extract = vtkCTHExtractAMRPart::New();
  extract->SetInput( fractal->GetOutput());
  extract->SetClipPlane (clipPlane);
  
  vtkCTHOutlineFilter *outline = vtkCTHOutlineFilter::New();
  outline->SetInput( fractal->GetOutput());
  
  vtkPolyDataMapper *outlineMapper = vtkPolyDataMapper::New();
  outlineMapper->SetInput( outline->GetOutput() );
  
  vtkActor *outlineActor = vtkActor::New();
  outlineActor->SetMapper( outlineMapper );

  vtkPolyDataMapper *mapper = vtkPolyDataMapper::New();
  mapper->SetInput( extract->GetOutput() );
  
  vtkActor *actor = vtkActor::New();
  actor->SetMapper( mapper );

  vtkRenderer *ren = vtkRenderer::New();
  ren->AddActor( actor );
  ren->AddActor( outlineActor );
  
  vtkRenderWindow *renWin = vtkRenderWindow::New();
  renWin->AddRenderer( ren );

  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

   // interact with data
  renWin->Render();

  int retVal = vtkRegressionTestImage( renWin );
 
  if ( retVal == vtkTesting::DO_INTERACTOR)
    {
    iren->Start();
    }

  // Clean up
  fractal->Delete();
  clipPlane->Delete();
  extract->Delete();
  outline->Delete();
  outlineMapper->Delete();
  outlineActor->Delete();
  mapper->Delete();
  actor->Delete();
  ren->Delete();
  renWin->Delete();
  iren->Delete();

  return 0;
}


