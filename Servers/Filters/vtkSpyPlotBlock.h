/*=========================================================================

Program:   Visualization Toolkit
Module:    $RCSfile: vtkSpyPlotBlock.h,v $

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSpyPlotBlock - Represents a SpyPlot Block Grid
// .SECTION Description
// vtkSpyPlotBlock is a regular hexahedral grid stored in a SpyPlot file.
// The grid can be part of an Adaptive Mesh Refinement (AMR) dataset or
// part 
// The class was extracted from vtkSpyPlotUniReader and expanded upon by 
// transisitioning functionality from vtkSpyPlotUniReader and adding it to 
// this class.  Note that this helper class is not derived from vtkObject 
// and can be allocated on the static

#ifndef __vtkSpyPlotBlock_h
#define __vtkSpyPlotBlock_h

class vtkDataArray;
class vtkFloatArray;
class vtkSpyPlotIStream;
class VTK_EXPORT vtkSpyPlotBlock {
public:
  vtkSpyPlotBlock();
  ~vtkSpyPlotBlock();

  // Description:
  // 
  int GetLevel() const;
  void GetDimensions(int dims[3]) const;
  int GetDimension(int i) const;
  void GetBounds(double bounds[6])const;
  void GetVectors(vtkDataArray *coordinates[3]) const;
  void GetVectors(vtkFloatArray *coordinates[3]) const;
  vtkFloatArray *GetVectors(int i) const;
  void GetExtents(int extents[6]) const;
  int IsAllocated() const;
  int IsFixed() const;
  int IsActive() const;
  void MarkedAsFixed();
  void GetRealBounds(double realBounds[6],
                     int assumeAMR) const;
  
  int GetAMRInformation(double globalBounds[6],
                        int *level, 
                        double spacing[3],
                        double origin[3], 
                        int extents[6],
                        int realExtents[6], 
                        int realDimensions[3])const;
  int HadBadGhostCell(int i) const;
  int Read(vtkSpyPlotIStream *stream);
  int GetTotalSize() const;
  int FixInformation(double globalBounds[6],
                     int extents[6],
                     int realExtents[6], 
                     int realDims[3],
                     vtkDataArray *ca[3]);
  // Dummy functions so we can use vtk macros
  void SetDebug(unsigned char i);
  unsigned char GetDebug() const;
  const char *GetClassName() const;
  int HasObserver(const char *) const;
  int InvokeEvent(const char *, void *) const;
protected:
  int RunLengthDeltaDecode(const unsigned char* in, int inSize, float* out, 
                           int outSize);
  int Dimensions[3];
  int Allocated;
  int Active;
  int Level;
  unsigned char DebugMode;
  vtkFloatArray* XYZArrays[3];
  int VectorsFixedForGhostCells;
  int RemovedBadGhostCells[6];
};

inline int vtkSpyPlotBlock::GetLevel() const 
{
  return this->Level;
}

inline void vtkSpyPlotBlock::GetDimensions(int dims[3]) const
{
  dims[0] = this->Dimensions[0];
  dims[1] = this->Dimensions[1];
  dims[2] = this->Dimensions[2];
}

inline int vtkSpyPlotBlock::IsActive() const
{
  return this->Active;
}

inline int vtkSpyPlotBlock::IsAllocated() const
{
  return this->Allocated;
}

inline int vtkSpyPlotBlock::IsFixed() const
{
  return this->VectorsFixedForGhostCells;
}

inline void vtkSpyPlotBlock::MarkedAsFixed()
{
  this->VectorsFixedForGhostCells = 1;
}

inline int vtkSpyPlotBlock::HadBadGhostCell(int i) const
{
  return (this->RemovedBadGhostCells[i]);
}

inline int vtkSpyPlotBlock::GetDimension(int i) const
{
  return this->Dimensions[i];
}

inline void vtkSpyPlotBlock::GetVectors(vtkFloatArray  *fa[3]) const
{
  fa[0] = this->XYZArrays[0];
  fa[1] = this->XYZArrays[1];
  fa[2] = this->XYZArrays[2];
}

inline vtkFloatArray *vtkSpyPlotBlock::GetVectors(int i) const
{
  return this->XYZArrays[i];
}


inline void vtkSpyPlotBlock::GetExtents(int extents[6]) const
{
  extents[0] = extents[2] = extents[4] = 0;
  extents[1] = (this->Dimensions[0] == 1) ? 0 : this->Dimensions[0];
  extents[3] = (this->Dimensions[1] == 1) ? 0 : this->Dimensions[1];
  extents[5] = (this->Dimensions[2] == 1) ? 0 : this->Dimensions[2];
}

inline int vtkSpyPlotBlock::GetTotalSize() const
{
  return (this->Dimensions[0]*
          this->Dimensions[1]*
          this->Dimensions[2]);
}

#endif
