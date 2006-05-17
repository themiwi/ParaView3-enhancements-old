/*=========================================================================

   Program:   ParaQ
   Module:    $RCSfile: pqPipelineSource.h,v $

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.1. 

   See License_v1.1.txt for the full ParaQ license.
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

/// \file pqPipelineSource.h
/// \date 4/17/2006

#ifndef _pqPipelineSource_h
#define _pqPipelineSource_h

#include "pqWidgetsExport.h"
#include "pqPipelineObject.h"

class pqPipelineDisplay;
class pqPipelineSourceInternal;
class vtkObject;


/// PQ representation for a vtkSMProxy that can be involved in a pipeline.
/// i.e that can have input and/or output. The public API is to observe
/// the object, changes to the pipeline structure are only through
/// protected function. These changes happen automatically as a reflection
/// of the SM state. 
class PQWIDGETS_EXPORT pqPipelineSource : public pqPipelineObject
{
  Q_OBJECT

public:
  pqPipelineSource(QString name, vtkSMProxy *proxy, pqServer* server,
    QObject* parent=NULL);
  virtual ~pqPipelineSource();

  // Returns the registration name for the proxy.
  const QString &getProxyName() const; 

  // Returns the vtkSMProxy.
  vtkSMProxy *getProxy() const; 


  // Get the number of consumers.
  int getNumberOfConsumers() const;

  // Get consumer at a particular index.
  pqPipelineSource *getConsumer(int index) const;

  // Get index for a consumer.
  int getConsumerIndexFor(pqPipelineSource *) const;

  // Check if the object exists in the consumer set.
  bool hasConsumer(pqPipelineSource *) const;

  /// Get the display at given index.
  pqPipelineDisplay *getDisplay(int index) const;

  // Get number of displays.
  int getDisplayCount() const; 

signals:
  /// fired when a connection is created between two pqPipelineSources.
  void connectionAdded(pqPipelineSource* in, pqPipelineSource* out);

  /// fired when a connection is broken between two pqPipelineSources.
  void connectionRemoved(pqPipelineSource* in, pqPipelineSource* out, int index);

protected slots:
  // process some change in the input property for the proxy--needed for subclass
  // pqPipelineFilter.
  virtual void inputChanged() { ; }

protected:
  friend class pqPipelineFilter;
  friend class pqPipelineDisplay;

  // called by pqPipelineFilter when the connections change.
  void removeConsumer(pqPipelineSource *);
  void addConsumer(pqPipelineSource*);

  // called by pqPipelineDisplay when the connections change.
  void addDisplay(pqPipelineDisplay*);
  void removeDisplay(pqPipelineDisplay*);
private:
  pqPipelineSourceInternal *Internal; ///< Stores the output connections.
};

#endif
