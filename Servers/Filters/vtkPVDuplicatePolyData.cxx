/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkPVDuplicatePolyData.cxx,v $
  Language:  C++
  Date:      $Date: 2003-03-18 21:07:19 $
  Version:   $Revision: 1.2 $

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVDuplicatePolyData.h"

#include "vtkAppendPolyData.h"
#include "vtkCellData.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkSocketController.h"
#include "vtkTiledDisplaySchedule.h"

vtkCxxRevisionMacro(vtkPVDuplicatePolyData, "$Revision: 1.2 $");
vtkStandardNewMacro(vtkPVDuplicatePolyData);

vtkCxxSetObjectMacro(vtkPVDuplicatePolyData,Controller, vtkMultiProcessController);
vtkCxxSetObjectMacro(vtkPVDuplicatePolyData,SocketController, vtkSocketController);

//-----------------------------------------------------------------------------
vtkPVDuplicatePolyData::vtkPVDuplicatePolyData()
{
  // Controller keeps a reference to this object as well.
  this->Controller = NULL;
  this->SetController(vtkMultiProcessController::GetGlobalController());  

  this->Schedule = vtkTiledDisplaySchedule::New();

  this->SocketController = NULL;
  this->ClientFlag = 0;
  this->PassThrough = 0;
}

//-----------------------------------------------------------------------------
vtkPVDuplicatePolyData::~vtkPVDuplicatePolyData()
{
  if (this->Schedule)
    {
    this->Schedule->Delete();
    }

  this->SetController(0);
}


//-----------------------------------------------------------------------------
void vtkPVDuplicatePolyData::InitializeSchedule(int numProcs, int numTiles)
{
  // The +1 is for zeroEmpty.
  this->Schedule->InitializeTiles(numTiles, (numProcs+1));
}

//-----------------------------------------------------------------------------
void vtkPVDuplicatePolyData::ExecuteInformation()
{
  if (this->GetOutput() == NULL)
    {
    vtkErrorMacro("Missing output");
    return;
    }
  this->GetOutput()->SetMaximumNumberOfPieces(-1);
}

//-----------------------------------------------------------------------------
void vtkPVDuplicatePolyData::ComputeInputUpdateExtents(vtkDataObject *output)
{
  vtkPolyData *input = this->GetInput();
  int piece = output->GetUpdatePiece();
  int numPieces = output->GetUpdateNumberOfPieces();
  int ghostLevel = output->GetUpdateGhostLevel();

  if (input == NULL)
    {
    return;
    }
  input->SetUpdatePiece(piece);
  input->SetUpdateNumberOfPieces(numPieces);
  input->SetUpdateGhostLevel(ghostLevel);
}

  
//-----------------------------------------------------------------------------
void vtkPVDuplicatePolyData::Execute()
{
  vtkPolyData *input = this->GetInput();
  vtkPolyData *output = this->GetOutput();
  int myId;
  int idx, tileId, otherProcessId;
  int numElements;
  // A list of appends (not all are used by all processes.
  vtkAppendPolyData** appendFilters;
  vtkPolyData* tmp;

  if (this->SocketController && this->ClientFlag)
    {
    this->ClientExecute();
    return;
    }

  if (input == NULL)
    {
    vtkErrorMacro("Input has not been set.");
    return;
    }

  // Take this path if memory size is too large.
  if (this->Controller == NULL || this->PassThrough)
    {
    output->CopyStructure(input);
    output->GetPointData()->PassData(input->GetPointData());
    output->GetCellData()->PassData(input->GetCellData());
    if (this->SocketController && ! this->ClientFlag)
      {
      this->SocketController->Send(output, 1, 18732);
      }
    return;
    }
  
  myId = this->Controller->GetLocalProcessId();
  if (myId == 0)
    {
    // Copy to zero also (eventhough zeroEmpty). (it is small).
    tmp = vtkPolyData::New();
    this->Controller->Receive(tmp, 1, 12333);
    output->CopyStructure(tmp);
    output->GetPointData()->PassData(tmp->GetPointData());
    output->GetCellData()->PassData(tmp->GetCellData());
    tmp->Delete();
    tmp = NULL;
    return;
    }

  appendFilters = new vtkAppendPolyData* [this->Schedule->GetNumberOfTiles()];
  for (idx = 0; idx < this->Schedule->GetNumberOfTiles(); ++idx)
    {
    appendFilters[idx] = NULL;
    }

  // For zeroEmpty condition.
  myId = myId -1;
  numElements = this->Schedule->GetNumberOfProcessElements(myId);
  for (idx = 0; idx < numElements; ++idx)
    {
    otherProcessId = this->Schedule->GetElementOtherProcessId(myId, idx);
    if (this->Schedule->GetElementReceiveFlag(myId, idx))
      {
      tileId = this->Schedule->GetElementTileId(myId, idx);
      if (appendFilters[tileId] == NULL)
        {
        appendFilters[tileId] = vtkAppendPolyData::New();
        tmp = vtkPolyData::New();
        tmp->CopyStructure(input);
        tmp->GetPointData()->PassData(input->GetPointData());
        tmp->GetCellData()->PassData(input->GetCellData());
        appendFilters[tileId]->AddInput(tmp);
        tmp->Delete();
        tmp = NULL;
        }
      tmp = vtkPolyData::New();
      this->Controller->Receive(tmp, otherProcessId, 12329);
      appendFilters[tileId]->AddInput(tmp);
      tmp->Delete();
      tmp = NULL;
      }
    else
      {
      tileId = this->Schedule->GetElementTileId(myId, idx);
      if (appendFilters[tileId] == NULL)
        {
        this->Controller->Send(input, otherProcessId, 12329);
        }
      else
        {
        tmp = appendFilters[tileId]->GetOutput();
        tmp->Update();
        this->Controller->Send(tmp, otherProcessId, 12329);
        // No longer need this filter.
        appendFilters[tileId]->Delete();
        appendFilters[tileId] = NULL;
        }
      }
    }

  // If we are a tile, copy to output.
  tileId = this->Schedule->GetProcessTileId(myId);
  if (tileId > -1)
    {
    if (appendFilters[tileId])
      {
      tmp = appendFilters[tileId]->GetOutput();
      tmp->Update();
      }
    else
      {
      tmp = input;
      }
    output->CopyStructure(tmp);
    output->GetPointData()->PassData(tmp->GetPointData());
    output->GetCellData()->PassData(tmp->GetCellData());
    }

  // Clean up temporary objects.
  for (idx = 0; idx < this->Schedule->GetNumberOfTiles(); ++idx)
    {
    if (appendFilters[idx])
      {
      appendFilters[idx]->Delete();
      appendFilters[idx] = NULL;
      }
    }
  delete [] appendFilters;

  // zeroEmpty case.
  // Remember: myId was decremented.
  if (myId == 0)
    {
    this->Controller->Send(output, 0, 12333);
    }

  // Not worrying about client server yet. .......
  if (this->SocketController && ! this->ClientFlag)
    {
    this->SocketController->Send(output, 1, 18732);
    }
}


//-----------------------------------------------------------------------------
void vtkPVDuplicatePolyData::ClientExecute()
{
  vtkPolyData *output = this->GetOutput();
  vtkPolyData *tmp = vtkPolyData::New();

  // No data is on the client, so we just have to get the data
  // from node 0 of the server.
  this->SocketController->Receive(tmp, 1, 18732);
  output->CopyStructure(tmp);
  output->GetPointData()->PassData(tmp->GetPointData());
  output->GetCellData()->PassData(tmp->GetCellData());
}


//-----------------------------------------------------------------------------
void vtkPVDuplicatePolyData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  
  os << indent << "Controller: (" << this->Controller << ")\n";
  if (this->SocketController)
    {
    os << indent << "SocketController: (" << this->SocketController << ")\n";
    os << indent << "ClientFlag: " << this->ClientFlag << endl;
    }

  if (this->Schedule)
    {
    this->Schedule->PrintSelf(os, indent);
    }

  os << indent << "PassThrough: " << this->PassThrough << endl;
}

