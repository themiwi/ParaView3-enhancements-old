/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkMPIDuplicateUnstructuredGrid.cxx,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMPIDuplicateUnstructuredGrid.h"

#include "vtkAppendFilter.h"
#include "vtkCellData.h"
#include "vtkMultiProcessController.h"
#include "vtkUnstructuredGridWriter.h"
#include "vtkUnstructuredGridReader.h"
#include "vtkCharArray.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkUnstructuredGrid.h"
#include "vtkSocketController.h"
#include "vtkTimerLog.h"
#include "vtkToolkits.h"
#ifdef VTK_USE_MPI
#include "vtkMPICommunicator.h"
#endif

vtkCxxRevisionMacro(vtkMPIDuplicateUnstructuredGrid, "$Revision: 1.2 $");
vtkStandardNewMacro(vtkMPIDuplicateUnstructuredGrid);

vtkCxxSetObjectMacro(vtkMPIDuplicateUnstructuredGrid,Controller, vtkMultiProcessController);
vtkCxxSetObjectMacro(vtkMPIDuplicateUnstructuredGrid,SocketController, vtkSocketController);
vtkCxxSetObjectMacro(vtkMPIDuplicateUnstructuredGrid,RenderServerSocketController, vtkSocketController);

//-----------------------------------------------------------------------------
vtkMPIDuplicateUnstructuredGrid::vtkMPIDuplicateUnstructuredGrid()
{
  // Controller keeps a reference to this object as well.
  this->Controller = NULL;
  this->SetController(vtkMultiProcessController::GetGlobalController());  

  this->SocketController = NULL;
  this->ClientFlag = 0;

  this->RenderServerSocketController = NULL;
  this->RenderServerFlag = 0;

  this->PassThrough = 0;
  this->ZeroEmpty = 0;
  //this->MemorySize = 0;
  
  // Client has no inputs.
  this->NumberOfRequiredInputs = 0;
}

//-----------------------------------------------------------------------------
vtkMPIDuplicateUnstructuredGrid::~vtkMPIDuplicateUnstructuredGrid()
{
  this->SetController(0);
  this->SetSocketController(0);
  this->SetRenderServerSocketController(0);
}


#define vtkDPDPow2(j) (1 << (j))
static inline int vtkDPDLog2(int j, int& exact)
{
  int counter=0;
  exact = 1;
  while(j)
    {
    if ( ( j & 1 ) && (j >> 1) )
      {
      exact = 0;
      }
    j = j >> 1;
    counter++;
    }
  return counter-1;
}


//-----------------------------------------------------------------------------
void vtkMPIDuplicateUnstructuredGrid::ExecuteInformation()
{
  if (this->GetOutput() == NULL)
    {
    vtkErrorMacro("Missing output");
    return;
    }
  this->GetOutput()->SetMaximumNumberOfPieces(-1);
}

//-----------------------------------------------------------------------------
void vtkMPIDuplicateUnstructuredGrid::ComputeInputUpdateExtents(vtkDataObject *output)
{
  vtkUnstructuredGrid *input = this->GetInput();
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
void vtkMPIDuplicateUnstructuredGrid::Execute()
{
  vtkUnstructuredGrid *input = this->GetInput();
  
  vtkTimerLog::MarkStartEvent("MPIGather");
  
  if (this->PassThrough)
    {
    if (input == NULL)
      {
      return;
      }
    vtkUnstructuredGrid* output = this->GetOutput();
    output->CopyStructure(input);
    output->GetPointData()->ShallowCopy(input->GetPointData());
    output->GetCellData()->ShallowCopy(input->GetCellData());
    return;
    }
  
  // Setup a reader.
  vtkUnstructuredGridReader *reader = vtkUnstructuredGridReader::New();
  reader->ReadFromInputStringOn();
  vtkCharArray* mystring = vtkCharArray::New();
  reader->SetInputArray(mystring);
  mystring->Delete();

  if (this->SocketController && this->ClientFlag)
    {
    this->ClientExecute(reader);
    reader->Delete();
    reader = NULL;
    return;
    }
  
  // Create the writer.
  vtkUnstructuredGridWriter *writer = vtkUnstructuredGridWriter::New();
  writer->SetFileTypeToBinary();
  writer->WriteToOutputStringOn();

  // Render server just receives the data from the data server.
  if (this->RenderServerFlag)
    {
    this->RenderServerExecute(reader);
    }

  // Node 0 gathers all the the data and the broadcast it to all procs.
  this->ServerExecute(reader, writer);

  reader->Delete();
  reader = NULL;
  writer->Delete();
  writer = NULL;

  vtkTimerLog::MarkEndEvent("MPIGather");
}


//-----------------------------------------------------------------------------
#ifdef VTK_USE_MPI
void vtkMPIDuplicateUnstructuredGrid::ServerExecute(vtkUnstructuredGridReader* reader,
                                            vtkUnstructuredGridWriter* writer)
#else
void vtkMPIDuplicateUnstructuredGrid::ServerExecute(vtkUnstructuredGridReader* ,
                                            vtkUnstructuredGridWriter* writer)
#endif
{
  int numProcs;
  numProcs = this->Controller->GetNumberOfProcesses();
  vtkUnstructuredGrid* input;
  vtkUnstructuredGrid* pd;
  
  // First marshal our input.
  input = this->GetInput();
  pd = vtkUnstructuredGrid::New();
  if (input)
    {
    pd->CopyStructure(input);
    pd->GetPointData()->PassData(input->GetPointData());
    pd->GetCellData()->PassData(input->GetCellData());
    }
  writer->SetInput(pd);
  writer->Write();
  int size = writer->GetOutputStringLength();
  char* buf = writer->RegisterAndGetOutputString();
  
  pd->Delete();
  pd = NULL;
  
#ifdef VTK_USE_MPI
  int idx;
  int sum;
  int myId = this->Controller->GetLocalProcessId();

  vtkMPICommunicator *com = NULL;
  if (this->Controller)
    {
    com = vtkMPICommunicator::SafeDownCast(
      this->Controller->GetCommunicator());
    }

  if (com)
    {
    // Allocate arrays used by the AllGatherV call.
    int* recvLengths = new int[numProcs * 2];
    // Use a single array for lengths and offsets to avoid
    // another send to the client.
    int* recvOffsets = recvLengths+numProcs;
  
    // Compute the degenerate input offsets and lengths.
    // Broadcast our size to all other processes.
    com->AllGather(&size, recvLengths, 1);

    // Compute the displacements.
    sum = 0;
    for (idx = 0; idx < numProcs; ++idx)
      {
      recvOffsets[idx] = sum;
      sum += recvLengths[idx];
      }
  
    // Gather the marshaled data sets from all procs.
    char* allbuffers = new char[sum];
    com->AllGatherV(buf, allbuffers, size, recvLengths, recvOffsets);
    if (myId == 0 && this->SocketController)
      {
      // Send the string to the client.
      this->SocketController->Send(&numProcs, 1, 1, 948344);
      this->SocketController->Send(recvLengths, numProcs*2, 1, 948345);
      this->SocketController->Send(allbuffers, sum, 1, 948346);
      }
    if (this->RenderServerSocketController)
      {
      // Send the string to the render server.
      this->RenderServerSocketController->Send(&numProcs, 1, 1, 948344);
      this->RenderServerSocketController->Send(recvLengths, numProcs*2, 1, 948345);
      this->RenderServerSocketController->Send(allbuffers, sum, 1, 948346);
      }
    this->ReconstructOutput(reader, numProcs, allbuffers, 
                            recvLengths, recvOffsets);
    delete [] allbuffers;
    allbuffers = NULL;
    delete [] recvLengths;
    recvLengths = NULL;
    // recvOffsets is part of the recvLengths array.
    recvOffsets = NULL;
    delete [] buf;
    buf = NULL;
    return;
    }

#endif

  // Server must be a single process!
  if (this->RenderServerSocketController)
    {
    this->RenderServerSocketController->Send(&numProcs, 1, 1, 948344);
    int tmp[2];
    tmp[0] = size;
    tmp[1] = 0;
    this->RenderServerSocketController->Send(tmp, 2, 1, 948345);
    this->RenderServerSocketController->Send(buf, size, 1, 948346);
    }

  // Server must be a single process!
  if (this->SocketController)
    {
    this->SocketController->Send(&numProcs, 1, 1, 948344);
    int tmp[2];
    tmp[0] = size;
    tmp[1] = 0;
    this->SocketController->Send(tmp, 2, 1, 948345);
    this->SocketController->Send(buf, size, 1, 948346);
    }
  // Degenerate reconstruct output.
  if (input)
    {
    this->GetOutput()->ShallowCopy(input);
    }
  delete [] buf;
  buf = NULL;
}



//-----------------------------------------------------------------------------
void vtkMPIDuplicateUnstructuredGrid::ReconstructOutput(vtkUnstructuredGridReader* reader,
                                                int numProcs, char* recv,
                                                int* recvLengths, 
                                                int* recvOffsets)
{
  vtkUnstructuredGrid* output;
  vtkUnstructuredGrid* pd;
  int idx;

  // Reconstruct the poly datas and append them together.
  vtkAppendFilter *append = vtkAppendFilter::New();
  // First append the input from this process.
  for (idx = 0; idx < numProcs; ++idx)
    {
    // vtkCharArray should not delete the string when it's done.
    reader->Modified();
    reader->GetInputArray()->SetArray(recv+recvOffsets[idx],
                                      recvLengths[idx], 1);
    output = reader->GetOutput();
    output->Update();
    pd = vtkUnstructuredGrid::New();
    pd->CopyStructure(output);
    pd->GetPointData()->PassData(output->GetPointData());
    pd->GetCellData()->PassData(output->GetCellData());
    append->AddInput(pd);
    pd->Delete();
    pd = NULL;
    }

  // Append
  output = append->GetOutput();
  output->Update();

  // Copy results to our output;
  pd = this->GetOutput();
  pd->CopyStructure(output);
  pd->GetPointData()->PassData(output->GetPointData());
  pd->GetCellData()->PassData(output->GetCellData());

  append->Delete();
  append = NULL;
}


//-----------------------------------------------------------------------------
void vtkMPIDuplicateUnstructuredGrid::ClientExecute(vtkUnstructuredGridReader* reader)
{
  int numProcs;
  int* recvLengths;
  int* recvOffsets;
  int totalLength, idx;
  char* buffers;

  // Receive the numer of processes.
  this->SocketController->Receive(&numProcs, 1, 1, 948344);

  // Receive information about the lengths/offsets of each marshaled data set.
  recvLengths = new int[numProcs*2];
  recvOffsets = recvLengths + numProcs;
  this->SocketController->Receive(recvLengths, numProcs*2, 1, 948345);

  // Receive the actual buffers.
  totalLength = 0;
  for (idx = 0; idx < numProcs; ++idx)
    {
    totalLength += recvLengths[idx];
    }
  buffers = new char[totalLength];
  this->SocketController->Receive(buffers, totalLength, 1, 948346);

  this->ReconstructOutput(reader, numProcs, buffers, 
                          recvLengths, recvOffsets);
  delete [] recvLengths;
  recvLengths = NULL;
  delete [] buffers;
  buffers = NULL;
}


//-----------------------------------------------------------------------------
// The way this is implemented is inefficient.
// It would be better to have node zero receive the duplicate data and
// broadcast the data to all procs using MPI.
void vtkMPIDuplicateUnstructuredGrid::RenderServerExecute(vtkUnstructuredGridReader* reader)
{
  int numProcs;
  int* recvLengths;
  int* recvOffsets;
  int totalLength, idx;
  char* buffers;

  if (this->RenderServerSocketController == NULL)
    {
    vtkErrorMacro("Missing socket to data server.");
    }

  // Receive the numer of processes.
  this->RenderServerSocketController->Receive(&numProcs, 1, 1, 948344);

  // Receive information about the lengths/offsets of each marshaled data set.
  recvLengths = new int[numProcs*2];
  recvOffsets = recvLengths + numProcs;
  this->RenderServerSocketController->Receive(recvLengths, numProcs*2, 1, 948345);

  // Receive the actual buffers.
  totalLength = 0;
  for (idx = 0; idx < numProcs; ++idx)
    {
    totalLength += recvLengths[idx];
    }
  buffers = new char[totalLength];
  this->RenderServerSocketController->Receive(buffers, totalLength, 1, 948346);

  this->ReconstructOutput(reader, numProcs, buffers, 
                          recvLengths, recvOffsets);
  delete [] recvLengths;
  recvLengths = NULL;
  delete [] buffers;
  buffers = NULL;
}

//-----------------------------------------------------------------------------
void vtkMPIDuplicateUnstructuredGrid::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  
  os << indent << "Controller: (" << this->Controller << ")\n";
  if (this->SocketController)
    {
    os << indent << "SocketController: (" << this->SocketController << ")\n";
    os << indent << "ClientFlag: " << this->ClientFlag << endl;
    }
  if (this->RenderServerSocketController)
    {
    os << indent << "RenderServerSocketController: (" 
       << this->RenderServerSocketController << ")\n";
    os << indent << "RenderServerFlag: " << this->RenderServerFlag << endl;
    }
  os << indent << "PassThrough: " << this->PassThrough << endl;
  os << indent << "ZeroEmpty: " << this->ZeroEmpty << endl;
  // this->MemorySize doesn't exist (if vtkCollectUnstructuredGrid API is ever
  // to be mimicked, then this may need to be declared).
}

