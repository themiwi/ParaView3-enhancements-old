/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqServer.h,v $

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

#ifndef _pqServer_h
#define _pqServer_h

class vtkProcessModule;
class vtkPVOptions;
class vtkSMApplication;
class vtkSMProxyManager;
class vtkSMMultiViewRenderModuleProxy;
class vtkSMRenderModuleProxy;

#include "pqCoreExport.h"
#include "pqServerManagerModelItem.h"
#include "pqServerResource.h"
#include "vtkSmartPointer.h"

/// Abstracts the concept of a "server connection" so that ParaView clients may: 
/// have more than one connect at a time / open and close connections at-will
class PQCORE_EXPORT pqServer :
  public pqServerManagerModelItem 
{
  Q_OBJECT

public:
  /// Creates a new connection to the given server.  \sa pqServerResource, pqServerResources
  static pqServer* Create(const pqServerResource& Resource);
  
  // Use this method to disconnect a server. On calling this method,
  // the server instance will be deleted.
  static void disconnect(pqServer* server);

public:  
  pqServer(vtkIdType connectionId, vtkPVOptions*, QObject* parent = NULL);
  virtual ~pqServer();

  /// Returns the multi view manager proxy for this connection.
  vtkSMMultiViewRenderModuleProxy* GetRenderModule();

  /// create a new render module on the server and returns it.
  /// A new render module is allocated and it is the responsibility of the caller
  /// to remove it.
  vtkSMRenderModuleProxy* newRenderModule();

  const pqServerResource& getResource();
  vtkIdType GetConnectionID();

  // Return the number of data server partitions on this 
  // server connection. A convenience method.
  int getNumberOfPartitions();

signals:
  void nameChanged();

protected:
  /// Creates vtkSMMultiViewRenderModuleProxy for this connection and 
  /// initializes it to create render modules of correct type 
  /// depending upon the connection.
  void CreateRenderModule();
  
private:
  pqServer(const pqServer&);  // Not implemented.
  pqServer& operator=(const pqServer&); // Not implemented.

  void setResource(const pqServerResource &server_resource);

  pqServerResource Resource;
  vtkIdType ConnectionID;
  vtkSmartPointer<vtkSMMultiViewRenderModuleProxy> RenderModule;
  // TODO:
  // Each connection will eventually have a PVOptions object. 
  // For now, this is same as the vtkProcessModule::Options.
  vtkSmartPointer<vtkPVOptions> Options;
};

#endif // !_pqServer_h
