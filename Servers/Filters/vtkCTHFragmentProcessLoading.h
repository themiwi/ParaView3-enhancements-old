/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkCTHFragmentProcessLoading.h,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCTHFragmentProcessLoading
// .SECTION Description
// Data type to represent a node in a multiprocess job
// and its current loading state.

#ifndef __vtkCTHFragmentProcessLoading_h
#define __vtkCTHFragmentProcessLoading_h

#include <cassert>
#include "vtkType.h"
#include "vtksys/ios/iostream"

class vtkCTHFragmentProcessLoading
{
  public:
    enum {ID=0, LOADING=1, SIZE=2};
    //
    vtkCTHFragmentProcessLoading(){ this->Initialize(-1,0); }
    //
    ~vtkCTHFragmentProcessLoading(){ this->Initialize(-1,0); }
    // Description:
    // Set the id and load factor.
    void Initialize(int id, vtkIdType loadFactor)
    {
      this->Data[ID]=id;
      this->Data[LOADING]=loadFactor;
    }
    // Description:
    // Comparision of two objects loading.
    bool operator<(const vtkCTHFragmentProcessLoading &rhs) const
    {
      return this->Data[LOADING]<rhs.Data[LOADING];
    }
    // Description:
    // Comparision of two objects loading.
    bool operator>(const vtkCTHFragmentProcessLoading &rhs) const
    {
      return this->Data[LOADING]>rhs.Data[LOADING];
    }
    // Description:
    // Return the process id.
    int GetId() const{ return this->Data[ID]; }
    // Description:
    // Return the load factor.
    vtkIdType GetLoadFactor() const{ return this->Data[LOADING]; }
    // Description:
    // Add to the load factor.
    vtkIdType UpdateLoadFactor(vtkIdType loadFactor)
    {
      assert("Update would make loading negative."
             && (this->Data[LOADING]+loadFactor)>=0);
      return this->Data[LOADING]+=loadFactor;
    }
  private:
    vtkIdType Data[SIZE];
};
vtksys_ios::ostream &operator<<(vtksys_ios::ostream &sout, vtkCTHFragmentProcessLoading &fp);
#endif