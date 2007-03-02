/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTransferFunctionEditorWidgetSimple1D.cxx,v

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTransferFunctionEditorWidgetSimple1D.h"

#include "vtkCallbackCommand.h"
#include "vtkColorTransferFunction.h"
#include "vtkHandleRepresentation.h"
#include "vtkHandleWidget.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPiecewiseFunction.h"
#include "vtkRenderer.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkTransferFunctionEditorRepresentationSimple1D.h"
#include "vtkWidgetCallbackMapper.h"
#include "vtkWidgetEvent.h"

#include <vtkstd/list>

vtkCxxRevisionMacro(vtkTransferFunctionEditorWidgetSimple1D, "$Revision: 1.14 $");
vtkStandardNewMacro(vtkTransferFunctionEditorWidgetSimple1D);

// The vtkNodeList is a PIMPLed list<T>.
class vtkNodeList : public vtkstd::list<vtkHandleWidget*> {};
typedef vtkstd::list<vtkHandleWidget*>::iterator vtkNodeListIterator;

//----------------------------------------------------------------------------
vtkTransferFunctionEditorWidgetSimple1D::vtkTransferFunctionEditorWidgetSimple1D()
{
  this->Nodes = new vtkNodeList;
  this->WidgetState = vtkTransferFunctionEditorWidgetSimple1D::Start;
  this->InitialMinimumColor[0] = this->InitialMinimumColor[1] = 0;
  this->InitialMinimumColor[2] = 1;
  this->InitialMaximumColor[0] = 1;
  this->InitialMaximumColor[1] = this->InitialMaximumColor[2] = 0;

  this->CallbackMapper->SetCallbackMethod(
    vtkCommand::LeftButtonPressEvent,
    vtkWidgetEvent::AddPoint,
    this,
    vtkTransferFunctionEditorWidgetSimple1D::AddNodeAction);
  this->CallbackMapper->SetCallbackMethod(
    vtkCommand::LeftButtonReleaseEvent,
    vtkWidgetEvent::EndSelect,
    this,
    vtkTransferFunctionEditorWidgetSimple1D::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(
    vtkCommand::MouseMoveEvent,
    vtkWidgetEvent::Move,
    this,
    vtkTransferFunctionEditorWidgetSimple1D::MoveNodeAction);
  this->CallbackMapper->SetCallbackMethod(
    vtkCommand::RightButtonPressEvent,
    vtkWidgetEvent::ModifyEvent,
    this,
    vtkTransferFunctionEditorWidgetSimple1D::ModifyAction);
}

//----------------------------------------------------------------------------
vtkTransferFunctionEditorWidgetSimple1D::~vtkTransferFunctionEditorWidgetSimple1D()
{
  this->RemoveAllNodes();
  delete this->Nodes;
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorWidgetSimple1D::RemoveAllNodes()
{
  vtkNodeListIterator niter;
  for (niter = this->Nodes->begin(); niter != this->Nodes->end();)
    {
    (*niter)->Delete();
    this->Nodes->erase(niter++);
    }
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorWidgetSimple1D::SetEnabled(int enabling)
{
  this->Superclass::SetEnabled(enabling);

  if ( ! enabling )
    {
    this->WidgetState = vtkTransferFunctionEditorWidgetSimple1D::Start;
    vtkNodeListIterator niter;
    for (niter = this->Nodes->begin(); niter != this->Nodes->end(); niter++ )
      {
      (*niter)->SetEnabled(0);
      }
    }
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorWidgetSimple1D::CreateDefaultRepresentation()
{
  if (!this->WidgetRep)
    {
    this->WidgetRep = vtkTransferFunctionEditorRepresentationSimple1D::New();
    }
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorWidgetSimple1D::AddNodeAction(
  vtkAbstractWidget *widget)
{
  vtkTransferFunctionEditorWidgetSimple1D *self =
    reinterpret_cast<vtkTransferFunctionEditorWidgetSimple1D*>(widget);

  if (self->WidgetState ==
      vtkTransferFunctionEditorWidgetSimple1D::MovingNode ||
      !self->WidgetRep)
    {
    return;
    }

  int x = self->Interactor->GetEventPosition()[0];
  int y = self->Interactor->GetEventPosition()[1];

  int state = self->WidgetRep->ComputeInteractionState(x, y);
  if (state == vtkTransferFunctionEditorRepresentationSimple1D::NearNode)
    {
    // move an existing node
    self->WidgetState = vtkTransferFunctionEditorWidgetSimple1D::MovingNode;
    self->Superclass::StartInteraction();
    self->InvokeEvent(vtkCommand::StartInteractionEvent, NULL);
    }
  else
    {
    // add a new node
    self->WidgetState = vtkTransferFunctionEditorWidgetSimple1D::PlacingNode;
    self->AddNewNode(x, y);
    }

  self->EventCallbackCommand->SetAbortFlag(1);
  self->Render();
}

//-------------------------------------------------------------------------
void vtkTransferFunctionEditorWidgetSimple1D::AddNewNode(int x, int y)
{
  double e[3]; e[2]=0.0;
  e[0] = static_cast<double>(x);
  e[1] = static_cast<double>(y);
  vtkTransferFunctionEditorRepresentationSimple1D *rep =
    reinterpret_cast<vtkTransferFunctionEditorRepresentationSimple1D*>
    (this->WidgetRep);
  if (this->ModificationType == COLOR)
    {
    int size[2];
    rep->GetDisplaySize(size);
    e[1] = size[1] / 2;
    }
  unsigned int currentHandleNumber = rep->CreateHandle(e);
  vtkHandleWidget *currentHandle = this->CreateHandleWidget(
    this, rep, currentHandleNumber);
  if (this->ModificationType != COLOR)
    {
    this->AddOpacityPoint(x, y);
    }
  if (this->ModificationType != OPACITY)
    {
    this->AddColorPoint(x);
    }

  rep->SetActiveHandle(currentHandleNumber);
  currentHandle->SetEnabled(1);
  rep->BuildRepresentation();
  this->InvokeEvent(vtkCommand::PlacePointEvent,(void*)&(currentHandleNumber));
  this->InvokeEvent(vtkCommand::InteractionEvent,NULL);
}

//-------------------------------------------------------------------------
void vtkTransferFunctionEditorWidgetSimple1D::AddNewNode(double scalar)
{
  vtkTransferFunctionEditorRepresentationSimple1D *rep =
    vtkTransferFunctionEditorRepresentationSimple1D::SafeDownCast(
      this->WidgetRep);
  if (!rep)
    {
    return;
    }

  // determine display position
  double displayPos[3];
  int displaySize[2];
  rep->GetDisplaySize(displaySize);

  displayPos[2] = 0;
  double pctX = (scalar - this->VisibleScalarRange[0]) /
    (this->VisibleScalarRange[1] - this->VisibleScalarRange[0]);
  displayPos[0] = displaySize[0] * pctX;
  if (this->ModificationType != COLOR)
    {
    double opacity = this->OpacityFunction->GetValue(scalar);
    displayPos[1] = displaySize[1] * opacity;
    }
  else
    {
    displayPos[1] = displaySize[1] / 2;
    }

  unsigned int i;
  double nodePos[3];
  double rgb[3];
  for (i = 0; i < this->Nodes->size(); i++)
    {
    rep->GetHandleDisplayPosition(i, nodePos);
    if (nodePos[0] == displayPos[0] && nodePos[1] == displayPos[1])
      {
      // A node already exists at this position; don't add another one.
      if (this->ModificationType != OPACITY)
        {
        // color the node based on the CTF
        this->ColorFunction->GetColor(scalar, rgb);
        rep->SetHandleColor(i, rgb[0], rgb[1], rgb[2]);
        }
      return;
      }
    }

  unsigned int currentHandleNumber = rep->CreateHandle(displayPos);
  if (this->ModificationType != OPACITY)
    {
    // color the node based on the CTF
    this->ColorFunction->GetColor(scalar, rgb);
    rep->SetHandleColor(currentHandleNumber, rgb[0], rgb[1], rgb[2]);
    }
  vtkHandleWidget *currentHandle = this->CreateHandleWidget(
    this, rep, currentHandleNumber);
  rep->SetActiveHandle(currentHandleNumber);
  currentHandle->SetEnabled(1);
  this->InvokeEvent(vtkCommand::PlacePointEvent,(void*)&(currentHandleNumber));
  this->InvokeEvent(vtkCommand::InteractionEvent,NULL);
}

//-------------------------------------------------------------------------
void vtkTransferFunctionEditorWidgetSimple1D::EndSelectAction(
  vtkAbstractWidget *widget)
{
  vtkTransferFunctionEditorWidgetSimple1D *self =
    reinterpret_cast<vtkTransferFunctionEditorWidgetSimple1D*>(widget);

  if (self->WidgetState == vtkTransferFunctionEditorWidgetSimple1D::MovingNode)
    {
    self->WidgetState = vtkTransferFunctionEditorWidgetSimple1D::Start;
    self->EventCallbackCommand->SetAbortFlag(1);
    self->InvokeEvent(vtkCommand::EndInteractionEvent,NULL);
    self->Superclass::EndInteraction();
    self->Render();
    }
}

//-------------------------------------------------------------------------
void vtkTransferFunctionEditorWidgetSimple1D::MoveNodeAction(
  vtkAbstractWidget *widget)
{
  vtkTransferFunctionEditorWidgetSimple1D *self =
    reinterpret_cast<vtkTransferFunctionEditorWidgetSimple1D*>(widget);

  if (self->WidgetState == vtkTransferFunctionEditorWidgetSimple1D::Start ||
      self->WidgetState == vtkTransferFunctionEditorWidgetSimple1D::PlacingNode)
    {
    return;
    }

  vtkTransferFunctionEditorRepresentationSimple1D *rep =
    reinterpret_cast<vtkTransferFunctionEditorRepresentationSimple1D*>
    (self->WidgetRep);
  if (!rep)
    {
    return;
    }

  int x = self->Interactor->GetEventPosition()[0];
  int y = self->Interactor->GetEventPosition()[1];
  unsigned int nodeId = rep->GetActiveHandle();
  double pos[3];
  pos[0] = static_cast<double>(x);
  pos[1] = static_cast<double>(y);
  pos[2] = 0.0;
  if (self->ModificationType == COLOR)
    {
    int size[2];
    rep->GetDisplaySize(size);
    pos[1] = size[1]/2;
    }
  rep->SetHandleDisplayPosition(nodeId, pos);

  if (self->ModificationType != COLOR)
    {
    self->RemoveOpacityPoint(nodeId);
    self->AddOpacityPoint(x, y);
    }

  self->EventCallbackCommand->SetAbortFlag(1);
  self->InvokeEvent(vtkCommand::InteractionEvent, NULL);
  self->Render();
}

//-------------------------------------------------------------------------
void vtkTransferFunctionEditorWidgetSimple1D::ModifyAction(
  vtkAbstractWidget *widget)
{
  vtkTransferFunctionEditorWidgetSimple1D *self =
    reinterpret_cast<vtkTransferFunctionEditorWidgetSimple1D*>(widget);
  if (!self->WidgetRep)
    {
    return;
    }

  int x = self->Interactor->GetEventPosition()[0];
  int y = self->Interactor->GetEventPosition()[1];
  int state = self->WidgetRep->ComputeInteractionState(x, y);

  if (state == vtkTransferFunctionEditorRepresentationSimple1D::NearNode &&
      self->ModificationType != OPACITY)
    {
    // Fire an event indicating that a node has been selected
    // so we can know when to display a color chooser.
    self->InvokeEvent(vtkCommand::PickEvent, NULL);
    self->EventCallbackCommand->SetAbortFlag(1);
    }
}

//-------------------------------------------------------------------------
void vtkTransferFunctionEditorWidgetSimple1D::OnChar()
{
  this->Superclass::OnChar();

  vtkTransferFunctionEditorRepresentationSimple1D *rep =
    static_cast<vtkTransferFunctionEditorRepresentationSimple1D*>
    (this->WidgetRep);

  if (!this->Interactor || !rep)
    {
    return;
    }

  char keyCode = this->Interactor->GetKeyCode();

  if (strlen(this->Interactor->GetKeySym()) == 1)
    {
    if (keyCode == 'D' || keyCode == 'd')
      {
      this->RemoveNode(rep->GetActiveHandle());
      }
    }
}

//-------------------------------------------------------------------------
void vtkTransferFunctionEditorWidgetSimple1D::RemoveNode(unsigned int id)
{
  if (id > this->Nodes->size()-1)
    {
    return;
    }

  if (this->ModificationType != COLOR)
    {
    this->RemoveOpacityPoint(id);
    }
  if (this->ModificationType != OPACITY)
    {
    this->RemoveColorPoint(id);
    }

  vtkTransferFunctionEditorRepresentationSimple1D *rep =
    static_cast<vtkTransferFunctionEditorRepresentationSimple1D*>
    (this->WidgetRep);

  vtkNodeListIterator iter;
  unsigned int i = 0;
  for (iter = this->Nodes->begin(); iter != this->Nodes->end(); iter++, i++)
    {
    if (i == id)
      {
      (*iter)->SetEnabled(0);
      (*iter)->RemoveAllObservers();
      (*iter)->Delete();
      this->Nodes->erase(iter);
      rep->RemoveHandle(id);
      return;
      }
    }
}

//-------------------------------------------------------------------------
vtkHandleWidget* vtkTransferFunctionEditorWidgetSimple1D::CreateHandleWidget(
  vtkTransferFunctionEditorWidgetSimple1D *self, 
  vtkTransferFunctionEditorRepresentationSimple1D *rep,
  unsigned int currentHandleNumber)
{
  vtkHandleRepresentation *handleRep =
    rep->GetHandleRepresentation(currentHandleNumber);
  if (!handleRep)
    {
    return NULL;
    }

  // Create the handle widget.
  vtkHandleWidget *widget = vtkHandleWidget::New();

  // Configure the handle widget
  widget->SetParent(self);
  widget->SetInteractor(self->Interactor);

  handleRep->SetRenderer(self->CurrentRenderer);
  widget->SetRepresentation(handleRep);

  // Now place the widget into the list of handle widgets.
  vtkNodeListIterator niter;
  unsigned int i = 0;
  for (niter = self->Nodes->begin(); niter != self->Nodes->end();
       niter++, i++)
    {
    if (i == currentHandleNumber)
      {
      self->Nodes->insert(niter, widget);
      return widget;
      }
    }

  if (currentHandleNumber == self->Nodes->size())
    {
    self->Nodes->insert(self->Nodes->end(), widget);
    return widget;
    }

  return NULL;
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorWidgetSimple1D::SetVisibleScalarRange(
  double min, double max)
{
  if (this->VisibleScalarRange[0] == min && this->VisibleScalarRange[1] == max)
    {
    return;
    }

  double oldRange[2];
  this->GetVisibleScalarRange(oldRange);

  this->Superclass::SetVisibleScalarRange(min, max);

  this->RecomputeNodePositions(oldRange, this->VisibleScalarRange);
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorWidgetSimple1D::UpdateFromTransferFunctions()
{
  this->RemoveAllNodes();

  vtkTransferFunctionEditorRepresentationSimple1D *rep =
    vtkTransferFunctionEditorRepresentationSimple1D::SafeDownCast(
      this->WidgetRep);
  if (rep)
    {
    rep->RemoveAllHandles();
    }

  int i;

  if (this->OpacityFunction->GetSize() == 0)
    {
    this->OpacityFunction->AddPoint(this->WholeScalarRange[0], 0);
    this->OpacityFunction->AddPoint(this->WholeScalarRange[1], 1);
    }
  if (this->ColorFunction->GetSize() == 0)
    {
    this->ColorFunction->AddRGBPoint(this->WholeScalarRange[0],
                                     this->InitialMinimumColor[0],
                                     this->InitialMinimumColor[1],
                                     this->InitialMinimumColor[2]);
    this->ColorFunction->AddRGBPoint(this->WholeScalarRange[1],
                                     this->InitialMaximumColor[0],
                                     this->InitialMaximumColor[1],
                                     this->InitialMaximumColor[2]);
    }

  if (this->ModificationType != COLOR)
    {
    double oNode[4];
    for (i = 0; i < this->OpacityFunction->GetSize(); i++)
      {
      this->OpacityFunction->GetNodeValue(i, oNode);
      if (this->ModificationType == COLOR_AND_OPACITY)
        {
        double rgb[3];
        this->ColorFunction->GetColor(oNode[0], rgb);
        this->ColorFunction->AddRGBPoint(oNode[0], rgb[0], rgb[1], rgb[2]);
        }
      this->AddNewNode(oNode[0]);
      }
    }

  if (this->ModificationType != OPACITY)
    {
    double cNode[6];
    for (i = 0; i < this->ColorFunction->GetSize(); i++)
      {
      this->ColorFunction->GetNodeValue(i, cNode);
      if (this->ModificationType == COLOR_AND_OPACITY)
        {
        double opacity = this->OpacityFunction->GetValue(cNode[0]);
        this->OpacityFunction->AddPoint(cNode[0], opacity);
        }
      this->AddNewNode(cNode[0]);
      }
    }
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorWidgetSimple1D::RecomputeNodePositions(
  double oldRange[2], double newRange[2])
{
  // recompute transfer function node positions based on a change
  // in scalar range
  vtkTransferFunctionEditorRepresentationSimple1D *rep =
    reinterpret_cast<vtkTransferFunctionEditorRepresentationSimple1D*>
    (this->WidgetRep);

  if (!rep)
    {
    return;
    }

  int displaySize[2];
  rep->GetDisplaySize(displaySize);

  double newDisplayMin =
    (oldRange[0] - newRange[0]) / (newRange[1] - newRange[0]) * displaySize[0];
  double newDisplayMax =
    (oldRange[1] - newRange[0]) / (newRange[1] - newRange[0]) * displaySize[0];
  double newWidth = newDisplayMax - newDisplayMin;

  unsigned int i;
  vtkHandleRepresentation *handle;
  double oldPos[3], newPos[3], displayPct;

  for (i = 0; i < this->Nodes->size(); i++)
    {
    handle = rep->GetHandleRepresentation(i);
    handle->GetDisplayPosition(oldPos);
    displayPct = oldPos[0] / (double)(displaySize[0]);
    newPos[0] = (displayPct * newWidth) + newDisplayMin;
    newPos[1] = oldPos[1];
    newPos[2] = oldPos[2];
    handle->SetDisplayPosition(newPos);
    }
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorWidgetSimple1D::RecomputeNodePositions(
  int oldSize[2], int newSize[2])
{
  // recompute transfer function node positions based on a change
  // in renderer size
  vtkTransferFunctionEditorRepresentationSimple1D *rep =
    reinterpret_cast<vtkTransferFunctionEditorRepresentationSimple1D*>
    (this->WidgetRep);

  if (!rep)
    {
    return;
    }

  unsigned int i;
  vtkHandleRepresentation *handle;
  double oldPos[3], newPos[3], displayPctX, displayPctY;

  for (i = 0; i < this->Nodes->size(); i++)
    {
    handle = rep->GetHandleRepresentation(i);
    handle->GetDisplayPosition(oldPos);
    displayPctX = oldPos[0] / (double)(oldSize[0]);
    displayPctY = oldPos[1] / (double)(oldSize[1]);
    newPos[0] = displayPctX * newSize[0];
    newPos[1] = displayPctY * newSize[1];
    newPos[2] = oldPos[2];
    handle->SetDisplayPosition(newPos);
    }
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorWidgetSimple1D::Configure(int size[2])
{
  vtkTransferFunctionEditorRepresentationSimple1D *rep =
    vtkTransferFunctionEditorRepresentationSimple1D::SafeDownCast(
      this->WidgetRep);
  if (!rep)
    {
    return;
    }

  int oldSize[2];
  rep->GetDisplaySize(oldSize);

  this->Superclass::Configure(size);

  this->RecomputeNodePositions(oldSize, size);
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorWidgetSimple1D::AddOpacityPoint(int x, int y)
{
  vtkTransferFunctionEditorRepresentationSimple1D *rep =
    reinterpret_cast<vtkTransferFunctionEditorRepresentationSimple1D*>
    (this->WidgetRep);

  if (!rep)
    {
    return;
    }

  int windowSize[2];
  double percent[2], newScalar;

  rep->GetDisplaySize(windowSize);
  percent[0] = x / (double)(windowSize[0]);
  percent[1] = y / (double)(windowSize[1]);
  newScalar = this->VisibleScalarRange[0] +
    percent[0] * (this->VisibleScalarRange[1]-this->VisibleScalarRange[0]);

  this->OpacityFunction->AddPoint(newScalar, percent[1]);
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorWidgetSimple1D::AddColorPoint(int x)
{
  vtkTransferFunctionEditorRepresentationSimple1D *rep =
    reinterpret_cast<vtkTransferFunctionEditorRepresentationSimple1D*>
    (this->WidgetRep);

  if (!rep)
    {
    return;
    }

  int windowSize[2];
  double percent, newScalar;

  rep->GetDisplaySize(windowSize);
  percent = x / (double)(windowSize[0]);
  newScalar = this->VisibleScalarRange[0] +
    percent * (this->VisibleScalarRange[1]-this->VisibleScalarRange[0]);

  int idx;
  double color[3];
  this->ColorFunction->GetColor(newScalar, color);
  idx = this->ColorFunction->AddRGBPoint(newScalar,
                                         color[0], color[1], color[2]);
  this->SetElementRGBColor(idx, color[0], color[1], color[2]);
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorWidgetSimple1D::RepositionColorPoint(
  unsigned int idx, double scalar)
{
  double value[6];
  this->ColorFunction->GetNodeValue(idx, value);
  this->RemoveColorPoint(idx);
  this->ColorFunction->AddRGBPoint(scalar, value[1], value[2], value[3]);
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorWidgetSimple1D::RemoveOpacityPoint(
  unsigned int id)
{
  double value[4];
  this->OpacityFunction->GetNodeValue(id, value);
  this->OpacityFunction->RemovePoint(value[0]);
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorWidgetSimple1D::RemoveColorPoint(
  unsigned int id)
{
  double value[6];
  this->ColorFunction->GetNodeValue(id, value);
  this->ColorFunction->RemovePoint(value[0]);
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorWidgetSimple1D::SetElementOpacity(
  unsigned int idx, double opacity)
{
  if (idx >= static_cast<unsigned int>(this->OpacityFunction->GetSize()))
    {
    return;
    }

  double value[4];
  this->OpacityFunction->GetNodeValue(idx, value);
  this->RemoveOpacityPoint(idx);

  this->OpacityFunction->AddPoint(value[0], opacity);

  vtkTransferFunctionEditorRepresentationSimple1D *rep =
    vtkTransferFunctionEditorRepresentationSimple1D::SafeDownCast(
      this->WidgetRep);
  if (!rep)
    {
    return;
    }

  double pos[3];
  rep->GetHandleDisplayPosition(idx, pos);
  int size[2];
  rep->GetDisplaySize(size);
  pos[1] = opacity * size[1];
  rep->SetHandleDisplayPosition(idx, pos);
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorWidgetSimple1D::SetElementRGBColor(
  unsigned int idx, double r, double g, double b)
{
  if (idx >= static_cast<unsigned int>(this->ColorFunction->GetSize()))
    {
    return;
    }

  double value[6];
  this->ColorFunction->GetNodeValue(idx, value);
  this->ColorFunction->RemovePoint(value[0]);
  this->ColorFunction->AddRGBPoint(value[0], r, g, b);

  vtkTransferFunctionEditorRepresentationSimple1D *rep =
    vtkTransferFunctionEditorRepresentationSimple1D::SafeDownCast(
      this->WidgetRep);
  if (rep)
    {
    rep->SetHandleColor(idx, r, g, b);
    this->Render();
    }
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorWidgetSimple1D::SetElementHSVColor(
  unsigned int idx, double h, double s, double v)
{
  double rgb[3];
  vtkMath::HSVToRGB(h, s, v, rgb, rgb+1, rgb+2);
  this->SetElementRGBColor(idx, rgb[0], rgb[1], rgb[2]);
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorWidgetSimple1D::SetElementScalar(
  unsigned int idx, double value)
{
  unsigned int size = this->Nodes->size();
  if (idx >= this->Nodes->size())
    {
    return;
    }

  vtkTransferFunctionEditorRepresentationSimple1D *rep =
    reinterpret_cast<vtkTransferFunctionEditorRepresentationSimple1D*>
    (this->WidgetRep);
  if (!rep)
    {
    return;
    }

  int allowSet = 0;
  double prevScalar, nextScalar, displayPos[3], pct;
  displayPos[2] = 0;
  int displaySize[2];
  prevScalar = nextScalar = 0; // initialize to avoid warnings
  if (this->ModificationType != OPACITY)
    {
    allowSet = 0;
    if (idx == 0 && size == 1)
      {
      allowSet = 1;
      }
    else
      {
      double colorNode[6];
      if (idx < size-1)
        {
        this->ColorFunction->GetNodeValue(idx+1, colorNode);
        nextScalar = colorNode[0];
        }
      if (idx > 0)
        {
        this->ColorFunction->GetNodeValue(idx-1, colorNode);
        prevScalar = colorNode[0];
        }
      if (idx == 0 && nextScalar > value)
        {
        allowSet = 1;
        }
      else if (idx == size-1 && prevScalar < value)
        {
        allowSet = 1;
        }
      else if (prevScalar < value && value < nextScalar)
        {
        allowSet = 1;
        }
      }
    if (allowSet)
      {
      this->RepositionColorPoint(idx, value);
      rep->GetDisplaySize(displaySize);
      pct = (value - this->VisibleScalarRange[0]) /
        (this->VisibleScalarRange[1] - this->VisibleScalarRange[0]);
      displayPos[0] = static_cast<int>(displaySize[0] * pct);
      displayPos[1] = 0;
      rep->SetHandleDisplayPosition(idx, displayPos);
      this->InvokeEvent(vtkCommand::InteractionEvent, NULL);
      }
    }
  if (this->ModificationType != COLOR)
    {
    double opacityNode[4];
    allowSet = 0;
    if (idx == 0 && size == 1)
      {
      allowSet = 1;
      }
    else
      {
      if (idx < size-1)
        {
        this->OpacityFunction->GetNodeValue(idx+1, opacityNode);
        nextScalar = opacityNode[0];
        }
      if (idx > 0)
        {
        this->OpacityFunction->GetNodeValue(idx-1, opacityNode);
        prevScalar = opacityNode[0];
        }
      if (idx == 0 && nextScalar > value)
        {
        allowSet = 1;
        }
      else if (idx == size-1 && prevScalar < value)
        {
        allowSet = 1;
        }
      else if (prevScalar < value && value < nextScalar)
        {
        allowSet = 1;
        }
      }
    if (allowSet)
      {
      this->OpacityFunction->GetNodeValue(idx, opacityNode);
      this->RemoveOpacityPoint(idx);
      rep->GetDisplaySize(displaySize);
      pct = (value - this->VisibleScalarRange[0]) /
        (this->VisibleScalarRange[1] - this->VisibleScalarRange[0]);
      int xPos = static_cast<int>(displaySize[0] * pct);
      int yPos = static_cast<int>(displaySize[1] * opacityNode[1]);
      this->AddOpacityPoint(xPos, yPos);
      displayPos[0] = xPos;
      displayPos[1] = yPos;
      rep->SetHandleDisplayPosition(idx, displayPos);
      this->InvokeEvent(vtkCommand::InteractionEvent, NULL);
      }
    }

  if (!allowSet)
    {
    vtkErrorMacro("Cannot move a transfer function node horizontally past the ones on either side of it.");
    }
}

//----------------------------------------------------------------------------
double vtkTransferFunctionEditorWidgetSimple1D::GetElementOpacity(
  unsigned int idx)
{
  if (idx >= static_cast<unsigned int>(this->OpacityFunction->GetSize()) ||
      this->ModificationType == COLOR)
    {
    return 0;
    }

  double value[4];
  this->OpacityFunction->GetNodeValue(idx, value);
  return value[1];
}

//----------------------------------------------------------------------------
int vtkTransferFunctionEditorWidgetSimple1D::GetElementRGBColor(
  unsigned int idx, double color[3])
{
  if (idx >= this->Nodes->size() || this->ModificationType == OPACITY)
    {
    return 0;
    }

  double value[6];
  this->ColorFunction->GetNodeValue(idx, value);
  color[0] = value[1];
  color[1] = value[2];
  color[2] = value[3];
  return 1;
}
  
//----------------------------------------------------------------------------
int vtkTransferFunctionEditorWidgetSimple1D::GetElementHSVColor(
  unsigned int idx, double color[3])
{
  if (idx >= this->Nodes->size() || this->ModificationType == OPACITY)
    {
    return 0;
    }

  double value[6];
  this->ColorFunction->GetNodeValue(idx, value);
  color[0] = value[1];
  color[1] = value[2];
  color[2] = value[3];
  vtkMath::RGBToHSV(color, color);
  return 1;
}
  
//----------------------------------------------------------------------------
double vtkTransferFunctionEditorWidgetSimple1D::GetElementScalar(
  unsigned int idx)
{
  if (idx >= this->Nodes->size())
    {
    return 0;
    }

  if (this->ModificationType != COLOR)
    {
    double opacity[4];
    this->OpacityFunction->GetNodeValue(idx, opacity);
    return opacity[0];
    }
  else
    {
    double color[6];
    this->ColorFunction->GetNodeValue(idx, color);
    return color[0];
    }
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorWidgetSimple1D::SetColorSpace(int space)
{
  if (space <= 0 || space > 2)
    {
    return;
    }

  switch (space)
    {
    case 0:
      this->ColorFunction->SetColorSpace(space);
      break;
    case 1:
      this->ColorFunction->SetColorSpace(space);
      this->ColorFunction->HSVWrapOff();
      break;
    case 2:
      this->ColorFunction->SetColorSpace(VTK_CTF_HSV);
      this->ColorFunction->HSVWrapOn();
      break;
    }
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorWidgetSimple1D::PrintSelf(ostream& os,
                                                        vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
