/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkNetDmfReader.h,v $
  Language:  C++
  Date:      $Date: 2009-09-16 22:46:30 $
  Version:   $Revision: 1.5 $

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

#include "vtkDirectedGraphAlgorithm.h"
#include "vtkStdString.h" // needed for vtkStdString.

class vtkDataObject;
class vtkDataArraySelection;
class vtkCallbackCommand;
class vtkMultiProcessController;
class vtkMutableDirectedGraph;
class vtkNetDmfReaderInternal;
class vtkNetDmfReaderGrid;

//BTX
class XdmfDsmBuffer;
class NetDMFDOM;
class NetDMFElement;
//ETX

class VTK_EXPORT vtkNetDmfReader : public vtkDirectedGraphAlgorithm
{
public:
  static vtkNetDmfReader* New();
  vtkTypeRevisionMacro(vtkNetDmfReader, vtkDirectedGraphAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  void SetFileName(const vtkStdString& fileName);
  const char* GetFileName()const;
  
  // Description:
  // Determine if the file can be readed with this reader.
  virtual int CanReadFile(const char* fname);

protected:
  vtkNetDmfReader();
  ~vtkNetDmfReader();

  // Description:
  // This methods parses the XML. Returns true on success. This method can be
  // called repeatedly. It has checks to ensure that the XML parsing is done
  // only if needed.
  bool ParseXML();

  vtkStdString GetElementName(NetDMFElement* element);

  virtual int RequestData(vtkInformation *, vtkInformationVector **,
                          vtkInformationVector *);
  virtual int RequestInformation(vtkInformation *, vtkInformationVector **,
                                 vtkInformationVector *);

  vtkNetDmfReaderInternal* Internal;

  int            CurrentTimeStep;
  vtkTimeStamp   ParseTime;
  vtkStdString   FileName;
  long int       FileParseTime;

private:
  vtkNetDmfReader(const vtkNetDmfReader&); // Not implemented
  void operator=(const vtkNetDmfReader&); // Not implemented
};

#endif //__vtkNetDmfReader_h
