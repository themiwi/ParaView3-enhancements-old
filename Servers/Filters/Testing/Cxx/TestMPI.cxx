/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: TestMPI.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMazeSource.h"
#include "vtkPickFilter.h"
#include "vtkMPIDuplicatePolyData.h"
#include "vtkPVConnectivityFilter.h"
#include "vtkMPIDuplicateUnstructuredGrid.h"
#include "vtkMPIMoveData.h"
#include "vtkPolyData.h"
#include "vtkUnstructuredGrid.h"

int main(int , char* [])
{
  vtkMazeSource *maze = vtkMazeSource::New();
  maze->Update(); //For GetCenter
  
  vtkPickFilter *pick = vtkPickFilter::New();
  
  //law int fixme;  // why is this a problem.
  //pick->SetInput( maze->GetOutput() );
  pick->SetWorldPoint ( maze->GetOutput()->GetCenter() );
  pick->GetWorldPoint ();
  pick->SetPickCell ( 1 );
  pick->Update();

  vtkMPIDuplicatePolyData *duplicate = vtkMPIDuplicatePolyData::New();
  duplicate->SetInput( maze->GetOutput() );
  duplicate->PassThroughOn ();

  vtkPVConnectivityFilter *connect = vtkPVConnectivityFilter::New();
  connect->SetInput( duplicate->GetOutput() );
  
  vtkMPIDuplicateUnstructuredGrid *dupUns = vtkMPIDuplicateUnstructuredGrid::New();
  dupUns->SetInput( connect->GetOutput() );
  dupUns->PassThroughOn ();
  
  vtkMPIMoveData *move = vtkMPIMoveData::New();
  move->SetInput( dupUns->GetOutput() );
  move->SetMoveModeToPassThrough ();
  move->Update();

  maze->Delete();
  pick->Delete();
  duplicate->Delete();
  connect->Delete();
  dupUns->Delete();
  move->Delete();

  return 0;
}
