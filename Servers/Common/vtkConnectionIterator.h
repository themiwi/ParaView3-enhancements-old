/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkConnectionIterator.h,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkConnectionIterator - iterates over all connections.
// .SECTION Description
// vtkConnectionIterator iterates over all the connections
// in the vtkProcessModuleConnectionManager. It MatchConnectionID is set,
// it iterates over only those connection that match the id. 

#ifndef __vtkConnectionIterator_h
#define __vtkConnectionIterator_h

#include "vtkObject.h"
//BTX
#include "vtkConnectionID.h" // Needed for MatchConnectionID.
//ETX
class vtkProcessModuleConnection;
class vtkProcessModuleConnectionManager;
class vtkConnectionIteratorInternals;

class VTK_EXPORT vtkConnectionIterator : public vtkObject
{
public:
  static vtkConnectionIterator* New();
  vtkTypeRevisionMacro(vtkConnectionIterator, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Go to the beginning.
  void Begin();

  // Description:
  // Is the iterator pointing past the last element.
  int IsAtEnd();

  // Description:
  // Move to the next connection.
  void Next();
//BTX
  // Description:
  // Get/Set the connection ID to match.
  vtkSetMacro(MatchConnectionID, vtkConnectionID);
  vtkGetMacro(MatchConnectionID, vtkConnectionID);
  // Description:
  // Get/Set the ConnectionManager.
  vtkGetObjectMacro(ConnectionManager, vtkProcessModuleConnectionManager);
  void SetConnectionManager(vtkProcessModuleConnectionManager*);
  
  // Description:
  // Get the connection at the current position.
  vtkProcessModuleConnection* GetCurrentConnection();
  vtkConnectionID GetCurrentConnectionID();
//ETX
protected:
  vtkConnectionIterator();
  ~vtkConnectionIterator();

  vtkConnectionIteratorInternals* Internals;
  vtkConnectionID MatchConnectionID;
  vtkProcessModuleConnectionManager* ConnectionManager;
  int InBegin;
private:
  vtkConnectionIterator(const vtkConnectionIterator&); // Not implemented.
  void operator=(const vtkConnectionIterator&); // Not implemented.
};

#endif

