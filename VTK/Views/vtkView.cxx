/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkView.cxx,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "vtkView.h"

#include "vtkAlgorithmOutput.h"
#include "vtkCollection.h"
#include "vtkCommand.h"
#include "vtkDataObject.h"
#include "vtkDataRepresentation.h"
#include "vtkObjectFactory.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkStringArray.h"
#include "vtkViewTheme.h"
#include "vtkSmartPointer.h"

#include <vtkstd/map>
#include <vtkstd/string>
#include <vtkstd/vector>

#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

//----------------------------------------------------------------------------
class vtkView::Command : public vtkCommand
{
public:
  static Command* New() {  return new Command(); }
  virtual void Execute(vtkObject *caller, unsigned long eventId,
                       void *callData)
    {
    if (this->Target)
      {
      this->Target->ProcessEvents(caller, eventId, callData);
      }
    }
  void SetTarget(vtkView* t)
    {
    this->Target = t;
    }
private:
  Command() { this->Target = 0; }
  vtkView* Target;
};

//----------------------------------------------------------------------------
class vtkView::vtkInternal
{
public:
  vtkstd::map<vtkObject*, vtkstd::string> RegisteredProgress;
};

//----------------------------------------------------------------------------
class vtkView::vtkImplementation
{
public:
  vtkstd::vector<vtkSmartPointer<vtkDataRepresentation> > Representations;
};
  

vtkCxxRevisionMacro(vtkView, "$Revision: 1.21 $");
vtkStandardNewMacro(vtkView);
//----------------------------------------------------------------------------
vtkView::vtkView()
{
  this->Internal = new vtkView::vtkInternal();
  this->Implementation = new vtkView::vtkImplementation();
  this->Observer = vtkView::Command::New();
  this->Observer->SetTarget(this);
  this->ReuseSingleRepresentation = false;
  
  // Apply default theme
  vtkViewTheme* theme = vtkViewTheme::New();
  this->ApplyViewTheme(theme);
  theme->Delete();  
}

//----------------------------------------------------------------------------
vtkView::~vtkView()
{
  this->RemoveAllRepresentations();

  this->Observer->SetTarget(0);
  this->Observer->Delete();
  delete this->Internal;
  delete this->Implementation;
}

//----------------------------------------------------------------------------
bool vtkView::IsRepresentationPresent(vtkDataRepresentation* rep)
{
  unsigned int i;
  for( i = 0; i < this->Implementation->Representations.size(); i++ )
    {
    if( this->Implementation->Representations[i] == rep )
      {
      return true;
      }
    }      
  return false;
}

//----------------------------------------------------------------------------
vtkDataRepresentation* vtkView::AddRepresentationFromInput(vtkDataObject* input)
{
  return this->AddRepresentationFromInputConnection(input->GetProducerPort());
}

//----------------------------------------------------------------------------
vtkDataRepresentation* vtkView::SetRepresentationFromInput(vtkDataObject* input)
{
  return this->SetRepresentationFromInputConnection(input->GetProducerPort());
}

//----------------------------------------------------------------------------
vtkDataRepresentation* vtkView::CreateDefaultRepresentation(vtkAlgorithmOutput* conn)
{
  vtkDataRepresentation* rep = vtkDataRepresentation::New();
  rep->SetInputConnection(conn);
  return rep;
}

//----------------------------------------------------------------------------
vtkDataRepresentation* vtkView::AddRepresentationFromInputConnection(vtkAlgorithmOutput* conn)
{
  if (this->ReuseSingleRepresentation && this->GetNumberOfRepresentations() > 0)
    {
    this->GetRepresentation()->SetInputConnection(conn);
    return this->GetRepresentation();
    }
  vtkDataRepresentation* rep = this->CreateDefaultRepresentation(conn);
  if (!rep)
    {
    vtkErrorMacro("Could not add representation from input connection because "
      "no default representation was created for the given input connection.");
    return 0;
    }

  this->AddRepresentation(rep);
  rep->Delete();
  return rep;
}

//----------------------------------------------------------------------------
vtkDataRepresentation* vtkView::SetRepresentationFromInputConnection(vtkAlgorithmOutput* conn)
{
  if (this->ReuseSingleRepresentation && this->GetNumberOfRepresentations() > 0)
    {
    this->GetRepresentation()->SetInputConnection(conn);
    return this->GetRepresentation();
    }
  vtkDataRepresentation* rep = this->CreateDefaultRepresentation(conn);
  if (!rep)
    {
    vtkErrorMacro("Could not add representation from input connection because "
      "no default representation was created for the given input connection.");
    return 0;
    }

  this->SetRepresentation(rep);
  rep->Delete();
  return rep;
}

//----------------------------------------------------------------------------
void vtkView::AddRepresentation(vtkDataRepresentation* rep)
{
  if (!this->IsRepresentationPresent(rep))
    {
    if (rep->AddToView(this))
      {
      rep->AddObserver(vtkCommand::SelectionChangedEvent, this->GetObserver());
      if (rep->GetNumberOfInputPorts() > 0 &&
          rep->GetNumberOfInputConnections(0) > 0)
        {
        // TODO: Do we need to update the rep
        this->AddInputConnection(rep->GetInternalOutputPort(),
                                 rep->GetInternalSelectionOutputPort());
        }
      this->AddRepresentationInternal(rep);
      this->Implementation->Representations.push_back(rep);
      }
    }
}

//----------------------------------------------------------------------------
void vtkView::SetRepresentation(vtkDataRepresentation* rep)
{
  this->RemoveAllRepresentations();
  this->AddRepresentation(rep);
}

//----------------------------------------------------------------------------
void vtkView::RemoveRepresentation(vtkDataRepresentation* rep)
{
  if (this->IsRepresentationPresent(rep))
    {
    rep->RemoveFromView(this);
    rep->RemoveObserver(this->GetObserver());
    if (rep->GetNumberOfInputPorts() > 0 &&
        rep->GetNumberOfInputConnections(0) > 0)
      {
      // TODO: Do we need to update the rep
      this->RemoveInputConnection(rep->GetInternalOutputPort(),
                                  rep->GetInternalSelectionOutputPort());
      }
    this->RemoveRepresentationInternal(rep);
    vtkstd::vector<vtkSmartPointer<vtkDataRepresentation> >::iterator it, itEnd;
    it = this->Implementation->Representations.begin();
    itEnd = this->Implementation->Representations.end();
    for (; it != itEnd; ++it)
      {
      if (it->GetPointer() == rep)
        {
        this->Implementation->Representations.erase(it);
        break;
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkView::RemoveRepresentation(vtkAlgorithmOutput* conn)
{
  unsigned int i;
  for( i = 0; i < this->Implementation->Representations.size(); i++ )
    {
    vtkDataRepresentation* rep = this->Implementation->Representations[i];
    if (rep->GetNumberOfInputPorts() > 0 &&
        rep->GetInputConnection() == conn)
      {
      this->RemoveRepresentation(rep);
      }
    }
}

//----------------------------------------------------------------------------
void vtkView::RemoveAllRepresentations()
{
  while (this->Implementation->Representations.size())
    {
    vtkDataRepresentation* rep = this->Implementation->Representations.back();
    this->RemoveRepresentation(rep);
    }
}

//----------------------------------------------------------------------------
int vtkView::GetNumberOfRepresentations()
{
  return static_cast<int>(this->Implementation->Representations.size());
}

//----------------------------------------------------------------------------
vtkDataRepresentation* vtkView::GetRepresentation(int index)
{
  if (index >= 0 && index < this->GetNumberOfRepresentations())
    {
    return this->Implementation->Representations[index];
    }
  return 0;
}

//----------------------------------------------------------------------------
vtkCommand* vtkView::GetObserver()
{
  return this->Observer;
}

//----------------------------------------------------------------------------
void vtkView::ProcessEvents(vtkObject* caller, unsigned long eventId, 
  void* callData)
{
  vtkDataRepresentation* caller_rep = vtkDataRepresentation::SafeDownCast( caller );
  if (this->IsRepresentationPresent(caller_rep) && eventId == vtkCommand::SelectionChangedEvent)
    {
    this->InvokeEvent(vtkCommand::SelectionChangedEvent);
    }

  if (eventId == vtkCommand::ProgressEvent)
    {
    vtkstd::map<vtkObject*, vtkstd::string>::iterator iter = 
      this->Internal->RegisteredProgress.find(caller);
    if (iter != this->Internal->RegisteredProgress.end())
      {
      ViewProgressEventCallData eventdata(iter->second.c_str(),
        *(reinterpret_cast<const double*>(callData)));
      this->InvokeEvent(vtkCommand::ViewProgressEvent, &eventdata);
      }
    }
}

//----------------------------------------------------------------------------
void vtkView::RegisterProgress(vtkObject* algorithm, const char* message/*=NULL*/)
{
  if (algorithm)
    {
    const char* used_message = message? message : algorithm->GetClassName();
    this->Internal->RegisteredProgress[algorithm] = used_message;
    algorithm->AddObserver(vtkCommand::ProgressEvent, this->Observer);
    }
}

//----------------------------------------------------------------------------
void vtkView::UnRegisterProgress(vtkObject* algorithm)
{
  if (algorithm)
    {
    vtkstd::map<vtkObject*, vtkstd::string>::iterator iter = 
      this->Internal->RegisteredProgress.find(algorithm);
    if (iter != this->Internal->RegisteredProgress.end())
      {
      this->Internal->RegisteredProgress.erase(iter);
      algorithm->RemoveObservers(vtkCommand::ProgressEvent, this->Observer);
      }
    }
}

//----------------------------------------------------------------------------
void vtkView::Update()
{
  unsigned int i;
  for( i = 0; i < this->Implementation->Representations.size(); i++ )
    {
    if( this->Implementation->Representations[i] )
      {
      this->Implementation->Representations[i]->Update();
      }
    }
}

//----------------------------------------------------------------------------
void vtkView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
