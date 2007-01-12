/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkReductionFilter.h,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkReductionFilter - A generic filter that can reduce any type of
// dataset using any reduction algorithm.
// .SECTION Description
// A generic filter that can reduce any type of dataset using any reduction 
// algorithm. Actual reduction is performed by running the PreReductionHelper
// and ReductionHelper algorithms. The PreReductionHelper runs on each node
// in parallel. Next the intermediate results are gathered to the root node.
// The root node then runs the ReductionHelper algorithm to produce a single 
// result. The ReductionHelper must be an algorithm that takes multiple 
// input connections and produces a single reduced output. 
//
// In addition to doing reduction the PassThrough variable lets you choose
// to pass through the results of any one node instead of aggregating all of
// them together.

#ifndef __vtkReductionFilter_h
#define __vtkReductionFilter_h

#include "vtkDataSetAlgorithm.h"

class vtkMultiProcessController;

class VTK_EXPORT vtkReductionFilter : public vtkDataSetAlgorithm
{
public:
  static vtkReductionFilter* New();
  vtkTypeRevisionMacro(vtkReductionFilter, vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set the pre-reduction helper. Pre-Reduction helper is an algorithm 
  // that runs on each node's data before it is sent to the root.
  void SetPreReductionHelper(vtkAlgorithm*);
  vtkGetObjectMacro(PreReductionHelper, vtkAlgorithm);

  // Description:
  // Get/Set the reduction helper. Reduction helper is an algorithm with
  // multiple input connections, that produces a single output as
  // the reduced output. This is run on the root node to produce a result
  // from the gathered results of each node.
  void SetReductionHelper(vtkAlgorithm*);
  vtkGetObjectMacro(ReductionHelper, vtkAlgorithm);

  // Description:
  // Get/Set the MPI controller used for gathering.
  void SetController(vtkMultiProcessController*);

  // Description:
  //Get/Set the PassThrough flag which (when set to a nonnegative number N) 
  //tells the filter to produce results that come from node N only. The 
  //data from that node still runs through the PreReduction and 
  //ReductionHelper algorithms.
  vtkSetMacro(PassThrough, int);
  vtkGetMacro(PassThrough, int);

protected:
  vtkReductionFilter();
  ~vtkReductionFilter();

  // Overridden to mark input as optional, since input data may
  // not be available on all processes that this filter is instantiated.
  virtual int FillInputPortInformation(int port, vtkInformation *info);

  virtual int RequestData(vtkInformation* request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector);

  void MarshallData(vtkDataSet* input);
  vtkDataSet* Reconstruct(char* raw_data, int data_length);
  void Reduce(vtkDataSet* input, vtkDataSet* output);

  char* RawData;
  int DataLength;
  vtkAlgorithm* PreReductionHelper;
  vtkAlgorithm* ReductionHelper;
  vtkMultiProcessController* Controller;
  int PassThrough;

private:
  vtkReductionFilter(const vtkReductionFilter&); // Not implemented.
  void operator=(const vtkReductionFilter&); // Not implemented.
};

#endif

