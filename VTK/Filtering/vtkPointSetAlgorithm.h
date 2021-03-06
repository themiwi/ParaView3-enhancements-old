/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkPointSetAlgorithm.h,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPointSetAlgorithm - Superclass for algorithms that produce output of the same type as input
// .SECTION Description
// vtkPointSetAlgorithm is a convenience class to make writing algorithms
// easier. It is also designed to help transition old algorithms to the new
// pipeline architecture. Ther are some assumptions and defaults made by this
// class you should be aware of. This class defaults such that your filter
// will have one input port and one output port. If that is not the case
// simply change it with SetNumberOfInputPorts etc. See this classes
// contstructor for the default. This class also provides a FillInputPortInfo
// method that by default says that all inputs will be PointSet. If that
// isn't the case then please override this method in your subclass. This
// class breaks out the downstream requests into seperate functions such as
// RequestDataObject RequestData and ExecuteInformation. The default 
// implementation of RequestDataObject will create an output data of the 
// same type as the input. 


#ifndef __vtkPointSetAlgorithm_h
#define __vtkPointSetAlgorithm_h

#include "vtkAlgorithm.h"

class vtkPointSet;
class vtkPolyData;
class vtkStructuredGrid;
class vtkUnstructuredGrid;

class VTK_FILTERING_EXPORT vtkPointSetAlgorithm : public vtkAlgorithm
{
public:
  static vtkPointSetAlgorithm *New();
  vtkTypeRevisionMacro(vtkPointSetAlgorithm,vtkAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the output data object for a port on this algorithm.
  vtkPointSet* GetOutput();
  vtkPointSet* GetOutput(int);

  // Description:
  // Get the output as vtkPolyData.
  vtkPolyData *GetPolyDataOutput();

  // Description:
  // Get the output as vtkStructuredGrid.
  vtkStructuredGrid *GetStructuredGridOutput();

  // Description:
  // Get the output as vtkUnstructuredGrid.
  vtkUnstructuredGrid *GetUnstructuredGridOutput();

  // Description:
  // Set an input of this algorithm. You should not override these
  // methods because they are not the only way to connect a pipeline.
  // Note that these methods support old-style pipeline connections.
  // When writing new code you should use the more general
  // vtkAlgorithm::SetInputConnection().  These methods transform the
  // input index to the input port index, not an index of a connection
  // within a single port.
  void SetInput(vtkDataObject*);
  void SetInput(int, vtkDataObject*);
  void SetInput(vtkPointSet*);
  void SetInput(int, vtkPointSet*);

  // Description:
  // Add an input of this algorithm.  Note that these methods support
  // old-style pipeline connections.  When writing new code you should
  // use the more general vtkAlgorithm::AddInputConnection().  See
  // SetInput() for details.
  void AddInput(vtkDataObject *);
  void AddInput(vtkPointSet*);
  void AddInput(int, vtkPointSet*);
  void AddInput(int, vtkDataObject*);

  // this method is not recommended for use, but lots of old style filters
  // use it
  vtkDataObject *GetInput();

  // Description:
  // see vtkAlgorithm for details
  virtual int ProcessRequest(vtkInformation* request, 
                             vtkInformationVector** inputVector,
                             vtkInformationVector* outputVector);

protected:
  vtkPointSetAlgorithm();
  ~vtkPointSetAlgorithm() {};
  
  // Description:
  // This is called by the superclass.
  // This is the method you should override.
  virtual int RequestDataObject(vtkInformation* request, 
                                vtkInformationVector** inputVector, 
                                vtkInformationVector* outputVector);
  
  // Description:
  // This is called by the superclass.
  // This is the method you should override.
  virtual int ExecuteInformation(vtkInformation*, 
                                 vtkInformationVector**, 
                                 vtkInformationVector*) {return 1;};
  
  // Description:
  // This is called by the superclass.
  // This is the method you should override.
  virtual int RequestData(vtkInformation*, 
                          vtkInformationVector**, 
                          vtkInformationVector*) {return 1;};
  
  // Description:
  // This is called by the superclass.
  // This is the method you should override.
  virtual int ComputeInputUpdateExtent(vtkInformation*,
                                       vtkInformationVector**,
                                       vtkInformationVector*) 
    {
      return 1;
    };

  // see algorithm for more info
  virtual int FillOutputPortInformation(int port, vtkInformation* info);
  virtual int FillInputPortInformation(int port, vtkInformation* info);

private:
  vtkPointSetAlgorithm(const vtkPointSetAlgorithm&);  // Not implemented.
  void operator=(const vtkPointSetAlgorithm&);  // Not implemented.
};

#endif
