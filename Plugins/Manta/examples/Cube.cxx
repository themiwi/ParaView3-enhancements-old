/*=========================================================================

 Program:   Visualization Toolkit
 Module:    $RCSfile: Cube.cxx,v $

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
// This example shows how to manually create vtkPolyData.

#include "vtkCellArray.h"
#include "vtkFloatArray.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"

int main()
{
    int i;
    static float pointPositions[8][3]= {
      { 0, 0, 0 }, { 1, 0, 0 }, { 1, 1, 0 }, { 0, 1, 0 },
      { 0, 0, 1 }, { 1, 0, 1 }, { 1, 1, 1 }, { 0, 1, 1 } };
    static vtkIdType pointIds[6][4]= {
      { 0, 1, 2, 3 }, { 4, 5, 6, 7 }, { 0, 1, 5, 4 },
      { 1, 2, 6, 5 }, { 2, 3, 7, 6 }, { 3, 0, 4, 7 } };

    // We'll create the building blocks of polydata including data attributes.
    vtkPolyData *cube = vtkPolyData::New();
    vtkPoints *points = vtkPoints::New();
    vtkCellArray *polys = vtkCellArray::New();
    vtkFloatArray *scalars = vtkFloatArray::New();

    // Load the point, cell, and data attributes.
    for (i=0; i<8; i++)
  points->InsertPoint(i, pointPositions[i]);
    for (i=0; i<6; i++)
  polys->InsertNextCell(4, pointIds[i]);
    for (i=0; i<8; i++)
  scalars->InsertTuple1(i, i);

    // We now assign the pieces to the vtkPolyData.
    cube->SetPoints(points);
    points->Delete();
    cube->SetPolys(polys);
    polys->Delete();
    cube->GetPointData()->SetScalars(scalars);
    scalars->Delete();

    // Now we'll look at it.
    vtkPolyDataMapper *cubeMapper = vtkPolyDataMapper::New();
    cubeMapper->SetInput(cube);
    cubeMapper->SetScalarRange(0, 7);
    vtkActor *cubeActor = vtkActor::New();
    cubeActor->SetMapper(cubeMapper);
    //cubeActor->GetProperty()->SetOpacity(0.5);

    // The usual rendering stuff.
    vtkCamera *camera = vtkCamera::New();
    camera->SetPosition(1, 1, 1);
    camera->SetFocalPoint(0, 0, 0);

    vtkRenderer *renderer = vtkRenderer::New();
    renderer->AddActor(cubeActor);
    renderer->SetActiveCamera(camera);
    renderer->ResetCamera();
    renderer->SetBackground(1, 1, 1);

    vtkRenderWindow *renWin = vtkRenderWindow::New();
    renWin->AddRenderer(renderer);
    renWin->SetSize(300, 300);



    vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
    iren->SetRenderWindow(renWin);

    // interact with data
    renWin->Render();
    iren->Start();

    // Clean up
    cube->Delete();
    cubeMapper->Delete();
    cubeActor->Delete();
    camera->Delete();
    renderer->Delete();
    renWin->Delete();
    iren->Delete();

    return 0;
}
