// -*- c++ -*-

/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkExtractCells.h,v $
  Language:  C++
  Date:      $Date: 2003-09-22 20:12:25 $
  Version:   $Revision: 1.4 $

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  Copyright (C) 2003 Sandia Corporation
  Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
  license for use of this work by or on behalf of the U.S. Government.
  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that this Notice and any statement
  of authorship are reproduced on all copies.

  Contact: Lee Ann Fisk, lafisk@sandia.gov

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkExtractCells - subset a vtkDataSet to create a vtkUnstructuredGrid
//
// .SECTION Description
//    Given a vtkDataSet and a list of cell Ids, create a vtkUnstructuredGrid
//    composed of these cells.  If the cell list is empty when vtkExtractCells 
//    executes, it will set up the ugrid, point and cell arrays, with no points, 
//    cells or data.

#include <vtkDataSetToUnstructuredGridFilter.h>
#include <vtkstd/set>     // for the internal cell ID list

class vtkIdList;
class vtkUnstructuredGrid;

class VTK_EXPORT vtkExtractCells : 
      public vtkDataSetToUnstructuredGridFilter
{
public:
  vtkTypeRevisionMacro(vtkExtractCells, vtkDataSetToUnstructuredGridFilter);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  static vtkExtractCells *New();

  // Description:
  // Set the list of cell IDs that the output vtkUnstructuredGrid
  // will be composed of.  Replaces any other cell ID list supplied
  // so far.

  void SetCellList(vtkIdList *l); 

  // Description:
  // Add the supplied list of cell IDs to those that will be included
  // in the output vtkUnstructuredGrid.

  void AddCellList(vtkIdList *l);

  // Description:
  // Add this range of cell IDs to those that will be included
  // in the output vtkUnstructuredGrid.

  void AddCellRange(vtkIdType from, vtkIdType to);

  // Description:
  //   Release the memory allocated to hold the cell ID list.

  void FreeCellList(); 

protected:

  virtual void Execute();

  vtkExtractCells();
  ~vtkExtractCells();

private:

  void Copy();
  static vtkIdType findInSortedList(vtkIdList *idList, vtkIdType id);
  vtkIdList *reMapPointIds(vtkDataSet *grid);

  void CopyCellsDataSet(vtkIdList *ptMap);
  void CopyCellsUnstructuredGrid(vtkIdList *ptMap);

//BTX
  vtkstd::set<vtkIdType> CellList;
//ETX

  int SubSetUGridCellArraySize;
  char InputIsUgrid;
};
