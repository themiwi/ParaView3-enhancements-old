/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMLODPartDisplay.h,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMLODPartDisplay - This contains all the LOD, mapper and actor stuff.
// .SECTION Description
// This is the part displays for serial execution of paraview.
// I handles all of the decimation levels of detail.


#ifndef __vtkSMLODPartDisplay_h
#define __vtkSMLODPartDisplay_h


#include "vtkSMPartDisplay.h"
class vtkDataSet;
class vtkPVLODPartDisplayInformation;
class vtkPolyDataMapper;
class vtkProp;
class vtkProperty;
class vtkPVPart;
class vtkRMScalarBarWidget;

class VTK_EXPORT vtkSMLODPartDisplay : public vtkSMPartDisplay
{
public:
  static vtkSMLODPartDisplay* New();
  vtkTypeRevisionMacro(vtkSMLODPartDisplay, vtkSMPartDisplay);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the number of bins per axes on the quadric decimation filter.
  virtual void SetLODResolution(int res);

  // Description:
  // Toggles the mappers to use immediate mode rendering or display lists.
  virtual void SetUseImmediateMode(int val);

  // Description:
  // Change the color mode to map scalars or 
  // use unsigned char arrays directly.
  // MapScalarsOff only works when coloring by an array 
  // on unsigned chars with 1 or 3 components.
  virtual void SetDirectColorFlag(int val);
  virtual void SetScalarVisibility(int val);
  virtual void ColorByArray(vtkRMScalarBarWidget *colorMap, int field);

  // Description:
  // Option to use a 1d texture map for the attribute color.
  virtual void SetInterpolateColorsFlag(int val);

  // Description:
  // This method updates the piece that has been assigned to this process.
  // It also gathers the data information.
  virtual void Update();

  // Description:
  // For flip books.
  virtual void RemoveAllCaches();
  virtual void CacheUpdate(int idx, int total);
              
  //BTX
  // Description:
  // Returns an up to data information object.
  // Do not keep a reference to this object.
  vtkPVLODPartDisplayInformation* GetLODInformation();
  //ETX

protected:
  vtkSMLODPartDisplay();
  ~vtkSMLODPartDisplay();

  int LODResolution;
  
  vtkSMProxy* LODUpdateSuppressorProxy;
  vtkSMProxy* LODMapperProxy;
  vtkSMProxy* LODDeciProxy;

  // Description:
  // This method should be called immediately after the object is constructed.
  // It create VTK objects which have to exeist on all processes.
  virtual void CreateVTKObjects(int num);

  vtkPVLODPartDisplayInformation* LODInformation;
  int LODInformationIsValid;

  vtkSMLODPartDisplay(const vtkSMLODPartDisplay&); // Not implemented
  void operator=(const vtkSMLODPartDisplay&); // Not implemented
};

#endif
