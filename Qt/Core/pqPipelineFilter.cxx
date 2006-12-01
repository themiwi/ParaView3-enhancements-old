/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqPipelineFilter.cxx,v $

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

/// \file pqPipelineFilter.cxx
/// \date 4/17/2006

#include "pqPipelineFilter.h"

// ParaView Server Manager includes.
#include "vtkEventQtSlotConnect.h"
#include "vtkSmartPointer.h"
#include "vtkSMInputProperty.h"
#include "vtkSMProxy.h"

//Qt includes.
#include <QList>
#include <QPointer>
#include <QSet>
#include <QtDebug>

// ParaView includes.
#include "pqServerManagerModel.h"

//-----------------------------------------------------------------------------
class pqPipelineFilterInternal
{
public:
  vtkSmartPointer<vtkEventQtSlotConnect> VTKConnect;
  QList<QPointer<pqPipelineSource> > Inputs;
  pqPipelineFilterInternal()
    {
    this->VTKConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New();
    }
};


//-----------------------------------------------------------------------------
pqPipelineFilter::pqPipelineFilter(QString name, vtkSMProxy* proxy,
  pqServer* server, QObject* p/*=NULL*/) : 
  pqPipelineSource(name, proxy, server, p)
{
  this->Internal = new pqPipelineFilterInternal();
  this->setType(pqPipelineModel::Filter);
  if (proxy)
    {
    // Listen to proxy events to get input changes.
    this->Internal->VTKConnect->Connect(proxy->GetProperty("Input"),
      vtkCommand::ModifiedEvent, this, SLOT(inputChanged()));
    }
}

//-----------------------------------------------------------------------------
pqPipelineFilter::~pqPipelineFilter()
{
  foreach(pqPipelineSource* input, this->Internal->Inputs)
    {
    if (input)
      {
      input->removeConsumer(this);
      }
    }
  delete this->Internal;
}

//-----------------------------------------------------------------------------
int pqPipelineFilter::getInputCount() const
{
  return this->Internal->Inputs.size();
}

//-----------------------------------------------------------------------------
pqPipelineSource *pqPipelineFilter::getInput(int index) const
{
  if(index >= 0 && index < this->Internal->Inputs.size())
    {
    return this->Internal->Inputs[index]; 
    }
  qCritical() << "Index " << index << "out of bound.";
  return 0;
}

//-----------------------------------------------------------------------------
int pqPipelineFilter::getInputIndexFor(pqPipelineSource *input) const
{
  return this->Internal->Inputs.indexOf(input);
}

//-----------------------------------------------------------------------------
bool pqPipelineFilter::hasInput(pqPipelineSource *input) const
{
  return (this->getInputIndexFor(input) != -1);
}

//-----------------------------------------------------------------------------
QList<pqPipelineSource*> pqPipelineFilter::getInputs() const
{
  QList<pqPipelineSource*> inputs;
  foreach(pqPipelineSource* src, this->Internal->Inputs)
    {
    if (src)
      {
      inputs.push_back(src);
      }
    }
  return inputs;
}

//-----------------------------------------------------------------------------
void pqPipelineFilter::inputChanged()
{
  // We must determine what changed on the input property.
  // Remember that all proxy that are added to the input property are
  // must already have pqPipelineFilter associated with them. If not, the input proxy was not
  // registered with the SM and as far as we are concerned, does not even exist :).

  QSet<pqPipelineSource *> oldInputs;
  foreach(pqPipelineSource* obj, this->Internal->Inputs)
    {
    oldInputs.insert(obj);
    }
    
  QSet<pqPipelineSource*> currentInputs;
  this->buildInputList(currentInputs);

  QSet<pqPipelineSource*> removed = oldInputs - currentInputs;
  QSet<pqPipelineSource*> added = currentInputs - oldInputs;

  // To preserve the order, we do this funny computation to sync the inputs list.
  foreach (pqPipelineSource* obj, removed)
    {
    this->Internal->Inputs.removeAt(this->Internal->Inputs.indexOf(obj));
    }
  foreach (pqPipelineSource* obj, added)
    {
    this->Internal->Inputs.push_back(obj);
    }

  // Now tell pqPipelineSource in removed list that we are no longer their
  // descendent.
  foreach(pqPipelineSource* removedInput, removed)
    {
    removedInput->removeConsumer(this);
    }

  foreach(pqPipelineSource* addedInput, added)
    {
    addedInput->addConsumer(this);
    }
  // The pqPipelineSource whose consumer changes raises the events when the 
  // consumer is removed added, so we don't need to raise any events here to 
  // let the world know that connections were broken/created.
}

//-----------------------------------------------------------------------------
void pqPipelineFilter::buildInputList(QSet<pqPipelineSource*> &set)
{
  vtkSMInputProperty* ivp = vtkSMInputProperty::SafeDownCast(
    this->getProxy()->GetProperty("Input"));

  pqServerManagerModel* model = pqServerManagerModel::instance();

  unsigned int max = ivp->GetNumberOfProxies();
  for (unsigned int cc=0; cc <max; cc++)
    {
    vtkSMProxy* proxy = ivp->GetProxy(cc);
    if (!proxy)
      {
      continue;
      }
    pqPipelineSource* pqSrc = model->getPQSource(proxy);
    if (!pqSrc)
      {
      qCritical() << "Some proxy is added as input but was not registered with"
        <<" Proxy Manager. This is not recommended.";
      continue;
      }
    set.insert(pqSrc);
    }
}

