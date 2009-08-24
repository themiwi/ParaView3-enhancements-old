/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkNetDmfReader.h,v $
  Language:  C++
  Date:      $Date: 2009-08-20 21:27:20 $
  Version:   $Revision: 1.1 $

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkNetDmfReader - read eXtensible Data Model and Format files
// .SECTION Description
// vtkNetDmfReader is a source object that reads XDMF data.
// The output of this reader is a vtkMultiGroupDataSet with one group for
// every enabled grid in the domain.
// The superclass of this class, vtkDataReader, provides many methods for
// controlling the reading of the data file, see vtkDataReader for more
// information.
// .SECTION Caveats
// uses the XDMF API
// .SECTION See Also
// vtkGraphReader

#ifndef __vtkNetDmfReader_h
#define __vtkNetDmfReader_h

#include "vtkGraphReader.h"

class vtkDataObject;
class vtkDataArraySelection;
class vtkCallbackCommand;
class vtkMultiProcessController;
class vtkNetDmfReaderInternal;
class vtkNetDmfReaderGrid;

//BTX
class XdmfDsmBuffer;
class XdmfDOM;
//ETX

class VTK_EXPORT vtkNetDmfReader : public vtkGraphReader
{
public:
  static vtkNetDmfReader* New();
  vtkTypeRevisionMacro(vtkNetDmfReader, vtkGraphReader);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Determine if the file can be readed with this reader.
  virtual int CanReadFile(const char* fname);

  // Set the Timestep to be read. This is provided for compatibility
  // reasons only and should not be used. The correct way to
  // request time is using the UPDATE_TIME_STEPS information key
  // passed from downstream.
  vtkSetMacro(TimeStep, int);
  vtkGetMacro(TimeStep, int);

  // Description:
  // Save the range of valid timestep index values. This can be used by the PAraView GUI

  vtkGetVector2Macro(TimeStepRange, int);

protected:
  vtkNetDmfReader();
  ~vtkNetDmfReader();

  // Description:
  // This methods parses the XML. Returns true on success. This method can be
  // called repeatedly. It has checks to ensure that the XML parsing is done
  // only if needed.
  bool ParseXML();

  virtual int RequestDataObject(vtkInformation *request,
                                vtkInformationVector **inputVector,
                                vtkInformationVector *outputVector);

  virtual int RequestData(vtkInformation *, vtkInformationVector **,
                          vtkInformationVector *);
  virtual int RequestInformation(vtkInformation *, vtkInformationVector **,
                                 vtkInformationVector *);
  virtual int FillOutputPortInformation(int port, vtkInformation *info);

  vtkNetDmfReaderInternal* Internals;
  XdmfDOM         *DOM;

  unsigned int   ActualTimeStep;
  int            TimeStep;
  int TimeStepRange[2];

private:
  vtkNetDmfReader(const vtkNetDmfReader&); // Not implemented
  void operator=(const vtkNetDmfReader&); // Not implemented
};

#endif //__vtkNetDmfReader_h
