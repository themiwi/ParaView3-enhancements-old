/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkPVTrivialExtentTranslator.h,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVTrivialExtentTranslator - extent translator that translates any
// request to match the extent of the dataset set on the translator.
// .SECTION Description
// vtkPVTrivialExtentTranslator is an extent translator that translates any
// request to match the extent of the dataset set on the translator.
// .SECTION See Also
// vtkPVTrivialProducer.

#ifndef __vtkPVTrivialExtentTranslator_h
#define __vtkPVTrivialExtentTranslator_h

#include "vtkExtentTranslator.h"

class vtkDataSet;

class VTK_EXPORT vtkPVTrivialExtentTranslator : public vtkExtentTranslator
{
public:
  static vtkPVTrivialExtentTranslator* New();
  vtkTypeRevisionMacro(vtkPVTrivialExtentTranslator, vtkExtentTranslator);
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual void Register(vtkObjectBase* o)
    {
    this->RegisterInternal(o, 1);
    }
  virtual void UnRegister(vtkObjectBase* o)
    {
    this->UnRegisterInternal(o, 1);
    }

  void SetDataSet(vtkDataSet*);
  vtkGetObjectMacro(DataSet, vtkDataSet);

//BTX
protected:
  vtkPVTrivialExtentTranslator();
  ~vtkPVTrivialExtentTranslator();
  virtual void ReportReferences(vtkGarbageCollector*);
  virtual int PieceToExtentThreadSafe(int vtkNotUsed(piece), 
                                      int vtkNotUsed(numPieces), 
                                      int vtkNotUsed(ghostLevel), 
                                      int *wholeExtent, int *resultExtent, 
                                      int vtkNotUsed(splitMode), 
                                      int vtkNotUsed(byPoints));

  vtkDataSet* DataSet;
private:
  vtkPVTrivialExtentTranslator(const vtkPVTrivialExtentTranslator&); // Not implemented
  void operator=(const vtkPVTrivialExtentTranslator&); // Not implemented
//ETX
};

#endif

