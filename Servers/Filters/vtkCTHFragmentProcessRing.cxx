/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkCTHFragmentProcessRing.cxx,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCTHFragmentProcessRing.h"
#include "vtksys/ios/iostream"
using vtksys_ios::ostream;
using vtksys_ios::cerr;
using vtksys_ios::endl;
//
void vtkCTHFragmentProcessRing::Initialize(
    vtkCTHFragmentProcessPriorityQueue &Q,
    vtkIdType upperLoadingBound)
{
  this->NextElement=0;
  this->BufferSize=0;
  this->Buffer.clear();

  // check that upper bound does not exclude
  // all processes.
  vtkCTHFragmentProcessLoading &pl=Q.Pop();
  vtkIdType minimumLoading=pl.GetLoadFactor();

  // Make sure at least one process can be used.
  if (upperLoadingBound!=-1
      && minimumLoading>upperLoadingBound)
    {
    upperLoadingBound=minimumLoading;
    cerr << "vtkCTHFragmentProcessRing "
          << "[" << __LINE__ << "] "
          << "Error: Upper loading bound excludes all processes."
          << endl;
    }

  // Build ring of process ids.
  this->Buffer.push_back(pl.GetId());
  ++this->BufferSize;
  while (!Q.Empty())
    {
    pl=Q.Pop();
    if (upperLoadingBound!=-1
        && pl.GetLoadFactor()>upperLoadingBound)
      {
      break;
      }
    this->Buffer.push_back(pl.GetId());
    ++this->BufferSize;
    }
}
//
void vtkCTHFragmentProcessRing::Print()
{
  int n=this->Buffer.size();
  if (n==0)
    {
    cerr << "{}";
    return;
    }
  cerr << "{" << this->Buffer[0];
  for (int i=1; i<n; ++i)
    {
    cerr << ", " << this->Buffer[i];
    }
  cerr << "}";
}

