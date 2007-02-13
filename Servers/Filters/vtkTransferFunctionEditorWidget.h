/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkTransferFunctionEditorWidget.h,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTransferFunctionEditorWidget - a 3D widget for manipulating a transfer function
// .SECTION Description
// vtkTransferFunctionEditorWidget is a superclass for 3D widgets that
// manipulate a transfer function. It allows you to interactively change the
// transfer function used for determining the colors used in rendering an actor
// or volume.
//
// .SECTION See Also
// vtkTransferFunctionEditorWidgetSimple1D
// vtkTransferFunctionEditorWidgetShapes1D
// vtkTransferFunctionEditorWidgetShapes2D

#ifndef __vtkTransferFunctionEditorWidget_h
#define __vtkTransferFunctionEditorWidget_h

#include "vtkAbstractWidget.h"

class vtkDataSet;
class vtkPiecewiseFunction;

class VTK_EXPORT vtkTransferFunctionEditorWidget : public vtkAbstractWidget
{
public:
  vtkTypeRevisionMacro(vtkTransferFunctionEditorWidget, vtkAbstractWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the input data set containing the array used for computing the
  // histogram and the scalar range.
  void SetInput(vtkDataSet *input);

  // Description:
  // Set the name of the array in the input data set to be processed.
  void SetArrayName(const char *name);

  // Description:
  // Set the field association (points = 0, cells = 1) of the array to process.
  void SetFieldAssociation(int assoc);

  // Description:
  // Set the scalar range to show in the transfer function editor.
  virtual void SetVisibleScalarRange(double range[2])
    { this->SetVisibleScalarRange(range[0], range[1]); }
  virtual void SetVisibleScalarRange(double min, double max);
  vtkGetVector2Macro(VisibleScalarRange, double);

  // Description:
  // Set the whole range of possible scalar values to show in the transfer
  // function editor. This is used for showing the whole scalar range when
  // the input is not set.
  vtkSetVector2Macro(WholeScalarRange, double);
  vtkGetVector2Macro(WholeScalarRange, double);

  // Description:
  // Reset the widget so that it shows the whole scalar range of the input
  // data set.
  void ShowWholeScalarRange();

  // Description:
  // Reconfigure the widget based on the size of the renderer containing it.
  virtual void Configure(int size[2]);

  // Description:
  // Notify the widget that the input has been modified so the histogram
  // needs to be recomputed.
  virtual void InputModified();

  // Description:
  // Methods for activating this widget. Note that the widget representation
  // must be specified or the widget will not appear.
  virtual void SetEnabled(int enable);

  // Description:
  // Set the type of function to modify.
  vtkSetClampMacro(ModificationType, int, 0, 2);
  
//BTX
  enum ModificationTypes
  {
    COLOR = 0,
    OPACITY,
    COLOR_AND_OPACITY
  };
//ETX

  vtkGetObjectMacro(OpacityFunction, vtkPiecewiseFunction);

protected:
  vtkTransferFunctionEditorWidget();
  ~vtkTransferFunctionEditorWidget();

  virtual void ComputeHistogram() = 0;

  vtkDataSet *Input;
  char *ArrayName;
  int FieldAssociation;
  double VisibleScalarRange[2];
  double WholeScalarRange[2];
  int NumberOfScalarBins; // used for float and double input images
  int ModificationType;
  vtkPiecewiseFunction *OpacityFunction;

private:
  vtkTransferFunctionEditorWidget(const vtkTransferFunctionEditorWidget&); // Not implemented.
  void operator=(const vtkTransferFunctionEditorWidget&); // Not implemented.
};

#endif
