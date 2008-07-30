/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkCTHFragmentPieceTransaction.cxx,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkCTHFragmentPieceTransaction.h"
using vtksys_ios::ostream;

//
ostream &operator<<(
        ostream &sout,
        const vtkCTHFragmentPieceTransaction &ta)
{
  sout << "(" << ta.GetType() << "," << ta.GetRemoteProc() << ")";

  return sout;
}
