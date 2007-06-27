/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqPipelineFilter.h,v $

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

/// \file pqPipelineFilter.h
/// \date 4/17/2006

#ifndef _pqPipelineFilter_h
#define _pqPipelineFilter_h


#include "pqCoreExport.h"
#include "pqPipelineSource.h"

class pqPipelineFilterInternal;

class PQCORE_EXPORT pqPipelineFilter : public pqPipelineSource
{
  Q_OBJECT
public:
  pqPipelineFilter(QString name, vtkSMProxy *proxy, pqServer* server, 
    QObject* parent=NULL);
  virtual ~pqPipelineFilter();

  // Get number of inputs.
  int getInputCount() const;


  // Get a list of all inputs.
  QList<pqPipelineSource*> getInputs() const;

  // Get input at given index.
  pqPipelineSource *getInput(int index) const;

  // get index for a given input.
  int getInputIndexFor(pqPipelineSource *input) const;

  // check if the input exists.
  bool hasInput(pqPipelineSource *input) const;

  /// Returns if this proxy replaces input on creation.
  /// This checks the "Hints" for the proxy, if any. If a <Visibility>
  /// element is present with replace_input="0", then this method
  /// returns false, otherwise true.
  int replaceInput() const;

protected slots:
  // process some change in the input property for the proxy.
  virtual void inputChanged();

protected:  

  // builds a set from the current value of the input property.
  void buildInputList(QSet<pqPipelineSource*>&);

  // Use this method to initialize the pqObject state using the
  // underlying vtkSMProxy. This needs to be done only once,
  // after the object has been created. 
  virtual void initialize() {this->inputChanged();};

private:
  pqPipelineFilterInternal *Internal; ///< Stores the input connections.
};

#endif
