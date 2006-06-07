/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkUndoStack.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkUndoStack.h"

#include "vtkObjectFactory.h"
#include "vtkUndoStackInternal.h"


vtkStandardNewMacro(vtkUndoStack);
vtkCxxRevisionMacro(vtkUndoStack, "$Revision: 1.4 $");
//-----------------------------------------------------------------------------
vtkUndoStack::vtkUndoStack()
{
  this->Internal = new vtkUndoStackInternal;
}

//-----------------------------------------------------------------------------
vtkUndoStack::~vtkUndoStack()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void vtkUndoStack::Clear()
{
  this->Internal->UndoStack.clear();
  this->Internal->RedoStack.clear();
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkUndoStack::Push(const char* label, vtkUndoSet* changeSet)
{
  this->Internal->RedoStack.clear();
  this->Internal->UndoStack.push_back(
    vtkUndoStackInternal::Element(label, changeSet));
  this->Modified();
}

//-----------------------------------------------------------------------------
unsigned int vtkUndoStack::GetNumberOfUndoSets()
{
  return static_cast<unsigned int>(this->Internal->UndoStack.size());
}

//-----------------------------------------------------------------------------
unsigned int vtkUndoStack::GetNumberOfRedoSets()
{
  return static_cast<unsigned int>(this->Internal->RedoStack.size());
}

//-----------------------------------------------------------------------------
const char* vtkUndoStack::GetUndoSetLabel(unsigned int position)
{
  if (position >= this->Internal->UndoStack.size())
    {
    return NULL;
    }
  position = (this->Internal->UndoStack.size() - position) -1;
  return this->Internal->UndoStack[position].Label.c_str();
}

//-----------------------------------------------------------------------------
const char* vtkUndoStack::GetRedoSetLabel(unsigned int position)
{
  if (position >= this->Internal->RedoStack.size())
    {
    return NULL;
    }
  position = (this->Internal->RedoStack.size() - position) -1;
  return this->Internal->RedoStack[position].Label.c_str();
}

//-----------------------------------------------------------------------------
int vtkUndoStack::Undo()
{
  if (this->Internal->UndoStack.empty())
    {
    return 0;
    }
  int status = this->Internal->UndoStack.back().UndoSet.GetPointer()->Undo();
  if (status)
    {
    this->PopUndoStack();
    }
  return status;
}

//-----------------------------------------------------------------------------
int vtkUndoStack::Redo()
{
  if (this->Internal->RedoStack.empty())
    {
    return 0;
    }
  int status = this->Internal->RedoStack.back().UndoSet.GetPointer()->Redo();
  if (status)
    {
    this->PopRedoStack();
    }
  return status;
}

//-----------------------------------------------------------------------------
void vtkUndoStack::PopUndoStack()
{
  if (this->Internal->UndoStack.empty())
    {
    return;
    }
  this->Internal->RedoStack.push_back(this->Internal->UndoStack.back());
  this->Internal->UndoStack.pop_back();
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkUndoStack::PopRedoStack()
{
  if (this->Internal->RedoStack.empty())
    {
    return;
    }
  this->Internal->UndoStack.push_back(this->Internal->RedoStack.back());
  this->Internal->RedoStack.pop_back();
  this->Modified();
}

//-----------------------------------------------------------------------------
vtkUndoSet* vtkUndoStack::GetNextUndoSet()
{
  if (!this->CanUndo())
    {
    return NULL;
    }
  return this->Internal->UndoStack.back().UndoSet.GetPointer();
}

//-----------------------------------------------------------------------------
vtkUndoSet* vtkUndoStack::GetNextRedoSet()
{
  if (!this->CanRedo())
    {
    return NULL;
    }
  return this->Internal->RedoStack.back().UndoSet.GetPointer();
}

//-----------------------------------------------------------------------------
void vtkUndoStack::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
