/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "pqServer.h"
#include "pqOptions.h"
#include <QCoreApplication>

#include <vtkToolkits.h>
#include <vtkObjectFactory.h>
#include <vtkProcessModuleGUIHelper.h>
#include <vtkPVOptions.h>
#include <vtkProcessModule.h>
#include <vtkProcessModuleConnectionManager.h>
#include <vtkPVServerInformation.h>
#include <vtkSMApplication.h>
#include <vtkSMProperty.h>
#include <vtkSMProxy.h>
#include <vtkSMProxyManager.h>
#include <vtkSMRenderModuleProxy.h>
#include <vtkSMMultiViewRenderModuleProxy.h>

#include <cassert>
  
vtkSMApplication* pqServer::ServerManager = 0;

// Client / server wrapper initialization functions
extern "C" void vtkCommonCS_Initialize(vtkClientServerInterpreter*);
extern "C" void vtkFilteringCS_Initialize(vtkClientServerInterpreter*);
extern "C" void vtkGenericFilteringCS_Initialize(vtkClientServerInterpreter*);
extern "C" void vtkImagingCS_Initialize(vtkClientServerInterpreter*);
extern "C" void vtkGraphicsCS_Initialize(vtkClientServerInterpreter*);
extern "C" void vtkIOCS_Initialize(vtkClientServerInterpreter*);
extern "C" void vtkRenderingCS_Initialize(vtkClientServerInterpreter*);
extern "C" void vtkVolumeRenderingCS_Initialize(vtkClientServerInterpreter*);
extern "C" void vtkHybridCS_Initialize(vtkClientServerInterpreter*);
extern "C" void vtkWidgetsCS_Initialize(vtkClientServerInterpreter*);
extern "C" void vtkParallelCS_Initialize(vtkClientServerInterpreter*);
extern "C" void vtkPVServerCommonCS_Initialize(vtkClientServerInterpreter*);
extern "C" void vtkPVFiltersCS_Initialize(vtkClientServerInterpreter*);
extern "C" void vtkXdmfCS_Initialize(vtkClientServerInterpreter *);

namespace
{

class pqProcessModuleGUIHelper :
  public vtkProcessModuleGUIHelper
{
public:
  static pqProcessModuleGUIHelper* New()
  {
    return new pqProcessModuleGUIHelper();
  }

  virtual int OpenConnectionDialog(int *)
  {
    return 0;
  }

  virtual void SendPrepareProgress()
  {
  }
  
  virtual void SetLocalProgress(const char* /*filter*/, int /*progress*/)
  {
  }
  
  virtual void SendCleanupPendingProgress()
  {
  }

  virtual void ExitApplication()
  {
  }

  virtual int RunGUIStart(int /*argc*/, char** /*argv*/, int /*numServerProcs*/, int /*myId*/)
  {
    return 0;
  }

protected:
  pqProcessModuleGUIHelper() {}

private:
  pqProcessModuleGUIHelper(const pqProcessModuleGUIHelper&);
  void operator=(const pqProcessModuleGUIHelper&);
};

///////////////////////////////////////////////////////////////////////////////////////
// pqInitializeServer

void pqInitializeServer(pqOptions* options, vtkProcessModule*& process_module, vtkSMApplication*& server_manager, vtkSMMultiViewRenderModuleProxy*& render_module)
{
  process_module = 0;
  server_manager = 0;
  render_module = 0;

  process_module = vtkProcessModule::New();  
  if(!process_module)
    return;

  process_module->Initialize();
  process_module->SetOptions(options);
  process_module->SetGUIHelper(pqProcessModuleGUIHelper::New());

  vtkCommonCS_Initialize(process_module->GetInterpreter());
  vtkFilteringCS_Initialize(process_module->GetInterpreter());
  vtkGenericFilteringCS_Initialize(process_module->GetInterpreter());
  vtkImagingCS_Initialize(process_module->GetInterpreter());
  vtkGraphicsCS_Initialize(process_module->GetInterpreter());
  vtkIOCS_Initialize(process_module->GetInterpreter());
  vtkRenderingCS_Initialize(process_module->GetInterpreter());
  vtkVolumeRenderingCS_Initialize(process_module->GetInterpreter());
  vtkHybridCS_Initialize(process_module->GetInterpreter());
  vtkWidgetsCS_Initialize(process_module->GetInterpreter());
  vtkParallelCS_Initialize(process_module->GetInterpreter());
  vtkPVServerCommonCS_Initialize(process_module->GetInterpreter());
  vtkPVFiltersCS_Initialize(process_module->GetInterpreter());
  vtkXdmfCS_Initialize(process_module->GetInterpreter());

  vtkProcessModule::SetProcessModule(process_module);
  if(process_module->Start(0, 0))
    {
    vtkProcessModule::SetProcessModule(0);
    return; // Failed to connect!
    }
  
  // Create server manager & proxy manager
  server_manager = vtkSMApplication::New();
  server_manager->Initialize();
  vtkSMProperty::SetCheckDomains(0);
  vtkSMProxyManager* const proxy_manager = server_manager->GetProxyManager();
  
  // Create render module ...
  process_module->SynchronizeServerClientOptions(
    vtkProcessModuleConnectionManager::GetRootServerConnectionID());

  render_module = 
    vtkSMMultiViewRenderModuleProxy::SafeDownCast(
      proxy_manager->NewProxy("rendermodules", "MultiViewRenderModule"));

  const char* renderModuleName = 0;

  if (!renderModuleName)
    {
    renderModuleName = options->GetRenderModuleName();
    }
  if (!renderModuleName)
    {
    if (options->GetTileDimensions()[0])
      {
      renderModuleName = "IceTRenderModule";
      }
    else if(options->GetClientMode())
      {
      renderModuleName = "IceTDesktopRenderModule";
      }
    else
      {
      renderModuleName = "LODRenderModule";
      }
    }
  
  options->SetRenderModuleName(renderModuleName);
  render_module->SetRenderModuleName(renderModuleName);
  render_module->UpdateVTKObjects();
}

} // namespace

/////////////////////////////////////////////////////////////////////////////////////////////
// pqServer

pqServer* pqServer::CreateStandalone()
{
  /** \todo Get rid of this logic once multiple connections are fully supported */
  if(vtkProcessModule::GetProcessModule())
    return 0;

  // Initialize options ...
  pqOptions* const options = pqOptions::New();
  options->SetClientMode(false);
  options->SetProcessType(vtkPVOptions::PARAVIEW);

  vtkProcessModule* process_module = 0;
  vtkSMApplication* server_manager = 0;
  vtkSMMultiViewRenderModuleProxy* render_module = 0;

  pqInitializeServer(options, process_module, server_manager, render_module);
  if(!process_module || !server_manager || !render_module)
    return 0;
    
  return new pqServer(options, process_module, server_manager, render_module);
}

pqServer* pqServer::CreateConnection(const char* const hostName, int portNumber)
{
  /** \todo Get rid of this logic once multiple connections are fully supported */
  if(vtkProcessModule::GetProcessModule())
    return 0;

  // Initialize options ...
  pqOptions* const options = pqOptions::New();
  options->SetClientMode(true);
  options->SetProcessType(vtkPVOptions::PVCLIENT);
  options->SetServerHost(hostName);
  options->SetServerPort(portNumber);

  vtkProcessModule* process_module = 0;
  vtkSMApplication* server_manager = 0;
  vtkSMMultiViewRenderModuleProxy* render_module = 0;
    
  pqInitializeServer(options, process_module, server_manager, render_module);
  if(!process_module || !server_manager || !render_module)
    return 0;
    
  return new pqServer(options, process_module, server_manager, render_module);
}

pqServer::pqServer(pqOptions* options, vtkProcessModule* process_module, vtkSMApplication* server_manager, vtkSMMultiViewRenderModuleProxy* render_module) :
  FriendlyName(),
  Options(options),
  ProcessModule(process_module),
  RenderModule(render_module)
{
  pqServer::ServerManager = server_manager;
  this->FriendlyName = this->getAddress();
}

pqServer::~pqServer()
{
  this->RenderModule->Delete();
  
  this->ServerManager->Finalize();
  this->ServerManager->Delete();
  
  this->ProcessModule->Finalize();
  this->ProcessModule->Delete();
  
  this->Options->Delete();
  
  vtkProcessModule::SetProcessModule(0);

}

QString pqServer::getAddress() const
{
  QString address;
  if(this->Options)
  {
    address.setNum(this->Options->GetServerPort());
    address.prepend(":");
    address.prepend(this->Options->GetServerHostName());
  }

  return address;
}

void pqServer::setFriendlyName(const QString& name)
{
  this->FriendlyName = name;
}

vtkProcessModule* pqServer::GetProcessModule()
{
  return ProcessModule;
}

vtkSMProxyManager* pqServer::GetProxyManager()
{
  return ServerManager->GetProxyManager();
}

vtkSMMultiViewRenderModuleProxy* pqServer::GetRenderModule()
{
  return RenderModule;
}


