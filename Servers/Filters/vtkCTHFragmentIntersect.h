/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkCTHFragmentIntersect.h,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCTHFragmentIntersect - Geometry intersection operations.
// .SECTION Description
// TODO write doc

#ifndef __vtkCTHFragmentIntersect_h
#define __vtkCTHFragmentIntersect_h

#include "vtkMultiBlockDataSetAlgorithm.h"
#include "vtkstd/vector" // using vector internally. ok for leaf classes.
#include "vtkstd/string" // ...then same is true of string
#include "iostream"

class vtkPolyData;
//class vtkMultiBlockDataSet;
class vtkPoints;
class vtkDoubleArray;
class vtkIntArray;
class vtkImplicitFunction;
class vtkMultiProcessController;
class vtkCTHFragmentCommBuffer;
class vtkCutter;

class VTK_EXPORT vtkCTHFragmentIntersect : public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkCTHFragmentIntersect *New();
  vtkTypeRevisionMacro(vtkCTHFragmentIntersect,vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  /// PARAVIEW interface stuff
  // Description
  // Specify the implicit function to perform the cutting.
  virtual void SetCutFunction(vtkImplicitFunction*);
  vtkGetObjectMacro(CutFunction,vtkImplicitFunction);
  // Description:
  // Specify the geometry Input.
  void SetGeometryInputConnection(vtkAlgorithmOutput* algOutput);
  // Description:
  // Specify the geometry Input.
  void SetStatisticsInputConnection(vtkAlgorithmOutput* algOutput);
  // Description:
  // Override GetMTime because we refer to vtkImplicitFunction.
  unsigned long GetMTime();

protected:
  vtkCTHFragmentIntersect();
  ~vtkCTHFragmentIntersect();

  //BTX
  /// pipeline
  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  virtual int FillInputPortInformation(int port, vtkInformation *info);
  virtual int FillOutputPortInformation(int port, vtkInformation *info);
  /// 
  // Make list of what we own
  int IdentifyLocalFragments();
  // Copy structure from multi block of polydata.
  int CopyInputStructureStats(
        vtkMultiBlockDataSet *dest,
        vtkMultiBlockDataSet *src);
  // Copy structure from mutli block of multi piece
  int CopyInputStructureGeom(
        vtkMultiBlockDataSet *dest,
        vtkMultiBlockDataSet *src);
  //
  int PrepareForIntersection();
  //
  int Intersect();
  // Send my geometric attribuites to a single process.
  int SendGeometricAttributes(const int recipientProcId);
  // size buffers & new containers
  int PrepareToCollectGeometricAttributes(
          vtkstd::vector<vtkCTHFragmentCommBuffer> &buffers,
          vtkstd::vector<vtkstd::vector<vtkDoubleArray *> >&centers,
          vtkstd::vector<vtkstd::vector<int *> >&ids);
  // Free resources.
  int CleanUpAfterCollectGeometricAttributes(
          vtkstd::vector<vtkCTHFragmentCommBuffer> &buffers,
          vtkstd::vector<vtkstd::vector<vtkDoubleArray *> >&centers,
          vtkstd::vector<vtkstd::vector<int *> >&ids);
  // Recieve all geometric attributes from all other
  // processes.
  int CollectGeometricAttributes(
          vtkstd::vector<vtkCTHFragmentCommBuffer> &buffers,
          vtkstd::vector<vtkstd::vector<vtkDoubleArray *> >&centers,
          vtkstd::vector<vtkstd::vector<int *> >&ids);
  // size local copy to hold all.
  int PrepareToMergeGeometricAttributes(
          vtkstd::vector<vtkCTHFragmentCommBuffer> &buffers,
          vtkstd::vector<vtkstd::vector<vtkDoubleArray *> >&centers);
  // Gather geometric attributes on a single process.
  int GatherGeometricAttributes(const int recipientProcId);
  // Copy attributes from input to output
  int CopyAttributesToStatsOutput(const int controllingProcId);
  //
  int CleanUpAfterRequest();

  /// data 
  //
  vtkMultiProcessController* Controller;
  // ids of what we own
  vtkstd::vector<vtkstd::vector<int> >FragmentIds;
  // 
  vtkstd::vector<vtkDoubleArray *>IntersectionCenters;
  vtkstd::vector<vtkstd::vector<int> >IntersectionIds;
  //
  vtkCutter *Cutter;
  // pts to what we are working on
  vtkMultiBlockDataSet *GeomIn;
  vtkMultiBlockDataSet *GeomOut;
  vtkMultiBlockDataSet *StatsIn;
  vtkMultiBlockDataSet *StatsOut;

  /// PARAVIEW interface data
  vtkImplicitFunction *CutFunction;

private:
  vtkCTHFragmentIntersect(const vtkCTHFragmentIntersect&);  // Not implemented.
  void operator=(const vtkCTHFragmentIntersect&);  // Not implemented.
  //ETX
};

#endif

