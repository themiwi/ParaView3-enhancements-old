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


#include "pqWidgetsExport.h"
#include "pqPipelineSource.h"

class pqPipelineFilterInternal;

class PQWIDGETS_EXPORT pqPipelineFilter : public pqPipelineSource
{
public:
  pqPipelineFilter(QString name, vtkSMProxy *proxy, pqServer* server, 
    QObject* parent=NULL);
  virtual ~pqPipelineFilter();

  // Get number of inputs.
  int getInputCount() const;

  // Get input at given index.
  pqPipelineSource *getInput(int index) const;

  // get index for a given input.
  int getInputIndexFor(pqPipelineSource *input) const;

  // check if the input exists.
  bool hasInput(pqPipelineSource *input) const;

  // Use this method to initialize the pqObject state using the
  // underlying vtkSMProxy. This needs to be done only once,
  // after the object has been created. 
  virtual void initialize() {this->inputChanged();};
protected slots:
  // process some change in the input property for the proxy.
  virtual void inputChanged();

protected:  

  // builds a set from the current value of the input property.
  void buildInputList(QSet<pqPipelineSource*>&);

private:
  pqPipelineFilterInternal *Internal; ///< Stores the input connections.
};

#endif
