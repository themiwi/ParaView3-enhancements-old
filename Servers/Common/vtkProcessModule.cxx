/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkProcessModule.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkProcessModule.h"

#include "vtkAlgorithm.h"
#include "vtkCallbackCommand.h"
#include "vtkClientServerID.h"
#include "vtkClientServerInterpreter.h"
#include "vtkClientServerStream.h"
#include "vtkCommand.h"
#include "vtkConnectionID.h"
#include "vtkConnectionIterator.h"
#include "vtkDataObject.h"
#include "vtkInstantiator.h"
#include "vtkKWProcessStatistics.h"
#include "vtkObjectFactory.h"
#include "vtkMultiProcessController.h"
#include "vtkMultiThreader.h"
#include "vtkProcessModuleConnectionManager.h"
#include "vtkProcessModuleGUIHelper.h"
#include "vtkPVConfig.h"
#include "vtkPVInformation.h"
#include "vtkPVOptions.h"
#include "vtkPVPaths.h"
#include "vtkPVProgressHandler.h"
#include "vtkPVServerInformation.h"
#include "vtkPVServerOptions.h"
#include "vtkServerConnection.h"
#include "vtkStdString.h"
#include "vtkSmartPointer.h"
#include "vtkStringList.h"
#include "vtkTimerLog.h"

#include <vtkstd/map>
#include <vtksys/SystemTools.hxx>

vtkProcessModule* vtkProcessModule::ProcessModule = 0;

//*****************************************************************************
class vtkProcessModuleInternals
{
public:
  typedef 
  vtkstd::map<vtkStdString, vtkSmartPointer<vtkDataObject> > DataTypesType;
  typedef vtkstd::map<vtkStdString, vtkStdString> MapStringToString;
  
  DataTypesType DataTypes;
  MapStringToString Paths;
};

//*****************************************************************************
class vtkProcessModuleObserver : public vtkCommand
{
public:
  static vtkProcessModuleObserver* New()
    { return new vtkProcessModuleObserver; }

  virtual void Execute(vtkObject* wdg, unsigned long event, void* calldata)
    {
    if (this->ProcessModule)
      {
      this->ProcessModule->ExecuteEvent(wdg, event, calldata);
      }
    this->AbortFlagOn();
    }

  void SetProcessModule(vtkProcessModule* pm)
    {
    this->ProcessModule = pm;
    }

protected:
  vtkProcessModuleObserver()
    {
    this->ProcessModule = 0;
    }
  vtkProcessModule* ProcessModule;
};
//*****************************************************************************


vtkStandardNewMacro(vtkProcessModule);
vtkCxxRevisionMacro(vtkProcessModule, "$Revision: 1.32 $");
vtkCxxSetObjectMacro(vtkProcessModule, ActiveRemoteConnection, vtkRemoteConnection);
vtkCxxSetObjectMacro(vtkProcessModule, GUIHelper, vtkProcessModuleGUIHelper);
//-----------------------------------------------------------------------------
vtkProcessModule::vtkProcessModule()
{
  this->Internals = new vtkProcessModuleInternals;
  this->ConnectionManager = 0;
  this->Interpreter = 0;
  
  this->Observer = vtkProcessModuleObserver::New();
  this->Observer->SetProcessModule(this);

  this->InterpreterObserver = 0;
  this->ReportInterpreterErrors = 1;

  this->UniqueID.ID = 3;

  this->ProgressHandler = vtkPVProgressHandler::New();
  this->ProgressRequests = 0;

  this->Options = 0;
  this->GUIHelper = 0;

  this->LogFile = 0;
  this->LogThreshold = 0;
  this->Timer = vtkTimerLog::New();

  this->ActiveRemoteConnection = 0 ;
  
  this->MemoryInformation = vtkKWProcessStatistics::New();
  this->ServerInformation = vtkPVServerInformation::New();
}

//-----------------------------------------------------------------------------
vtkProcessModule::~vtkProcessModule()
{
  this->Observer->SetProcessModule(0);
  this->Observer->Delete();

  if (this->ConnectionManager)
    {
    this->ConnectionManager->Delete();
    this->ConnectionManager = 0;
    }
  this->FinalizeInterpreter();
  delete this->Internals;

  if (this->InterpreterObserver)
    {
    this->InterpreterObserver->Delete();
    this->InterpreterObserver = 0;
    }

  this->ProgressHandler->Delete();
  this->SetOptions(0);
  this->SetGUIHelper(0);

  if (this->LogFile)
    {
    this->LogFile->close();
    delete this->LogFile;
    this->LogFile = 0;
    }

  this->SetActiveRemoteConnection(0);
  this->Timer->Delete();
  this->MemoryInformation->Delete();
  this->ServerInformation->Delete();
}

//-----------------------------------------------------------------------------
void vtkProcessModule::SetOptions(vtkPVOptions* op)
{
  this->Options = op;
  if (this->Options)
    {
    if (this->Options->GetServerMode())
      {
      this->ProgressHandler->SetServerMode(1);
      }
    if (this->Options->GetClientMode())
      {
      this->ProgressHandler->SetClientMode(1);
      }
    }
}

//-----------------------------------------------------------------------------
vtkDataObject* vtkProcessModule::GetDataObjectOfType(const char* classname)
{
  if (!classname)
    {
    return 0;
    }

  // Since we can not instantiate these classes, we'll replace
  // them with a subclass
  if (strcmp(classname, "vtkDataSet") == 0)
    {
    classname = "vtkImageData";
    }
  else if (strcmp(classname, "vtkPointSet") == 0)
    {
    classname = "vtkPolyData";
    }
  else if (strcmp(classname, "vtkCompositeDataSet") == 0)
    {
    classname = "vtkHierarchicalDataSet";
    }

  vtkProcessModuleInternals::DataTypesType::iterator it =
    this->Internals->DataTypes.find(classname);
  if (it != this->Internals->DataTypes.end())
    {
    return it->second.GetPointer();
    }

  vtkObject* object = vtkInstantiator::CreateInstance(classname);
  vtkDataObject* dobj = vtkDataObject::SafeDownCast(object);
  if (!dobj)
    {
    if (object)
      {
      object->Delete();
      }
    return 0;
    }

  this->Internals->DataTypes[classname] = dobj;
  dobj->Delete();
  return dobj;
}

//-----------------------------------------------------------------------------
void vtkProcessModule::GatherInformation(vtkConnectionID connectionID,
  vtkTypeUInt32 serverFlags, vtkPVInformation* info, vtkClientServerID id)
{
  vtkConnectionID rootId = 
    vtkProcessModuleConnectionManager::GetRootConnection(connectionID);
  this->ConnectionManager->GatherInformation(rootId,
    serverFlags, info, id);
}

//-----------------------------------------------------------------------------
int vtkProcessModule::Start(int argc, char** argv)
{
  // This is the place where we set up the ConnectionManager.
  if (this->ConnectionManager)
    {
    vtkErrorMacro("Duplicate call to Start.");
    return 1;
    }

  this->ConnectionManager = vtkProcessModuleConnectionManager::New();

  // This call blocks on the Satellite nodes (never on root node).
  if (this->ConnectionManager->Initialize(argc, argv, 
      this->Options->GetClientMode()) != 0)
    {
    return 1;
    }

  
  int myId = this->GetPartitionId();
  if (myId != 0)
    {
    // Satellite node. The control reaches here on a satellite node only on 
    // exit.
    return 0;
    }

  // Should only be called on root nodes.
  int ret = this->InitializeConnections();
  if (!ret)
    {
    // Failed.
    this->Exit();
    return 0;
    }

  if (this->Options->GetClientMode() || 
    (!this->Options->GetServerMode() && !this->Options->GetRenderServerMode()))
    {
    if (!this->GUIHelper)
      {
      vtkErrorMacro("GUIHelper must be set on the client.");
      this->Exit();
      return 1;
      } 
    
    if (this->Options->GetClientMode() && this->ShouldWaitForConnection())
      {
      if (!this->ClientWaitForConnection())
        {
        vtkErrorMacro("Could not connect to server(s). Exiting.");
        this->Exit();
        return 1;
        }
      }
    // Since ParaView closes server sockets, we close them for now.
    this->ConnectionManager->StopAcceptingAllConnections();
    return this->GUIHelper->RunGUIStart(argc, argv, 1, 0);
    }

  // Running in server mode.
  ret = 0;
  if (this->ShouldWaitForConnection())
    {
    cout << "Waiting for client..." << endl;
    }
  
  while  ((ret = this->ConnectionManager->MonitorConnections(0)) >= 0)
    {
    if (ret == 2)
      {
      cout << "Client connected." << endl;
      // Since ParaView closes server sockets, we close them for now.
      this->ConnectionManager->StopAcceptingAllConnections();
      }
    
    else if (ret == 3)
      {
      // Connection dropped.
      // In ParaView, we exit.
      cout << "Client connection closed." << endl;
      ret = 0;
      break;
      }
    }
  // We have to call exit explicitly on the Server since there is no
  // GUIHelper that would call it (as is the case with the client).
  this->Exit(); 
  return (ret==-1)? 1 : 0;
}

//-----------------------------------------------------------------------------
void vtkProcessModule::Exit()
{
  // Tell the connection manager to close all connections.
  this->ConnectionManager->Finalize();
}

//-----------------------------------------------------------------------------
int vtkProcessModule::InitializeConnections()
{
  // Detemine if this process supports connections.
  switch (this->Options->GetProcessType())
    {
  case vtkPVOptions::PARAVIEW:
  case vtkPVOptions::PVBATCH:
  case vtkPVOptions::XMLONLY:
  case vtkPVOptions::ALLPROCESS:
    return 1; // nothing to do here.
    }
  
  if (this->ShouldWaitForConnection())
    {
    return this->SetupWaitForConnection();
    }
  return this->ConnectToRemote();
}

//-----------------------------------------------------------------------------
int vtkProcessModule::ShouldWaitForConnection()
{
  // if client mode then return reverse connection
  if(this->Options->GetClientMode())
    {
    // if in client mode, it should not wait for a connection
    // unless reverse is 1, so just return reverse connection value
    return this->Options->GetReverseConnection();
    }
  // if server mode, then by default wait for the connection
  // so return not getreverseconnection
  return !this->Options->GetReverseConnection();
}

//-----------------------------------------------------------------------------
int vtkProcessModule::SetupWaitForConnection()
{
  int port = 0;
  switch (this->Options->GetProcessType())
    {
  case vtkPVOptions::PVCLIENT:
    // Check if we wait 2 separate connections (only client in render server mode
    // waits for 2 connections).
    if (this->Options->GetRenderServerMode())
      {
      int ret = this->ConnectionManager->AcceptConnectionsOnPort(
        this->Options->GetDataServerPort(),
        vtkProcessModuleConnectionManager::DATA_SERVER);
      if (ret == -1)
        {
        return 0;
        }
      ret = this->ConnectionManager->AcceptConnectionsOnPort(
        this->Options->GetRenderServerPort(),
        vtkProcessModuleConnectionManager::RENDER_SERVER);
      if (ret == -1)
        {
        return 0;
        }
      cout << "Listen on render server port:" << 
        this->Options->GetRenderServerPort() << endl;
      cout << "Listen on data server port:" <<
        this->Options->GetDataServerPort() << endl;
      return 1; // success.
      }
    else
      {
      port = this->Options->GetServerPort();
      }
    break;
    
  case vtkPVOptions::PVSERVER:
    port = this->Options->GetServerPort();
    break;

  case vtkPVOptions::PVRENDER_SERVER:
    port = this->Options->GetRenderServerPort();
    break;

  case vtkPVOptions::PVDATA_SERVER:
    port = this->Options->GetDataServerPort();
    break;

  default:
    return 0;
    }
  
  cout << "Listen on port: " << port << endl;
  int ret = this->ConnectionManager->AcceptConnectionsOnPort(
    port, vtkProcessModuleConnectionManager::RENDER_AND_DATA_SERVER);
  if (this->Options->GetRenderServerMode())
    {
    cout << "RenderServer: ";
    }
  return (ret == -1)? 0 : 1;
}

//-----------------------------------------------------------------------------
int vtkProcessModule::ConnectToRemote()
{
  const char* message = "client";
  while (1)
    {
    vtkConnectionID id = {0};

    switch (this->Options->GetProcessType())
      {
    case vtkPVOptions::PVCLIENT:
      if (this->Options->GetRenderServerMode())
        {
        id = this->ConnectionManager->OpenConnection(
          this->Options->GetDataServerHostName(),
          this->Options->GetDataServerPort(),
          this->Options->GetRenderServerHostName(),
          this->Options->GetRenderServerPort());
        message = "servers";
        }
      else
        {
        id = this->ConnectionManager->OpenConnection(
          this->Options->GetServerHostName(), this->Options->GetServerPort());
        message = "server";
        }
      break;

    case vtkPVOptions::PVSERVER:
      id = this->ConnectionManager->OpenConnection(
        this->Options->GetClientHostName(),
        this->Options->GetServerPort());
      break;

    case vtkPVOptions::PVRENDER_SERVER:
      id = this->ConnectionManager->OpenConnection(
        this->Options->GetClientHostName(),
        this->Options->GetRenderServerPort());
      cout << "RenderServer: ";
      break;

    case vtkPVOptions::PVDATA_SERVER:
      id = this->ConnectionManager->OpenConnection(
        this->Options->GetClientHostName(),
        this->Options->GetDataServerPort());
      break;

    default:
      vtkErrorMacro("Invalid mode!");
      return 0;
      }
    
    if (id.ID)
      {
      // connection successful.
      cout << "Connected to " << message << endl;
      return 1;
      }
    
    if (!this->GUIHelper)
      {
      // Probably a server. Just flag error and exit.
      vtkErrorMacro("Server Error: Could not connect to client.");
      return 0;
      }
    int start = 0;
    if (!this->GUIHelper->OpenConnectionDialog(&start))
      {
      vtkErrorMacro("Client error: Could not connect to the server. If you are trying "
        "to connect a client to data and render servers, you must use "
        "the --client-render-server (-crs) argument.");
      this->GUIHelper->ExitApplication();
      return 0;
      }
    }
  return 0;
}

//-----------------------------------------------------------------------------
// Called on client in reverse connect mode.
// Will establish server connections.
int vtkProcessModule::ClientWaitForConnection()
{
  
  cout << "Waiting for server..." << endl;
  this->GUIHelper->PopupDialog("Waiting for server",
    "Waiting for server to connect to this client via the reverse connection.");

  int not_abort_connection = 1;
  int res;
  while (not_abort_connection)
    {
    // Wait for 1/100 th of a second.
    res = this->ConnectionManager->MonitorConnections(10);
    if (res != 0 && res != 1)
      {
      this->GUIHelper->ClosePopup();
      }
    
    if (res < 0)
      {
      // Error !
      return 0;
      }
    if (res == 2)
      {
      // Connection created successfully.
      cout << "Server connected." << endl;
      return 1;
      }
    if (res == 1)
      {
      // Processed 1 of the 2 required connections....wait on...
      continue;
      }
    // Timeout.
    not_abort_connection = this->GUIHelper->UpdatePopup();
    }
  return 0;
}

//-----------------------------------------------------------------------------
vtkClientServerID vtkProcessModule::NewStreamObject(
  const char* type, vtkClientServerStream& stream)
{
  vtkClientServerID id = this->GetUniqueID();
  stream << vtkClientServerStream::New << type
         << id <<  vtkClientServerStream::End;
  return id;
}

//-----------------------------------------------------------------------------
vtkObjectBase* vtkProcessModule::GetObjectFromID(vtkClientServerID id)
{
  return this->Interpreter->GetObjectFromID(id);
}

//----------------------------------------------------------------------------
void vtkProcessModule::DeleteStreamObject(
  vtkClientServerID id, vtkClientServerStream& stream)
{
  stream << vtkClientServerStream::Delete << id
         <<  vtkClientServerStream::End;
}


//-----------------------------------------------------------------------------
const vtkClientServerStream& vtkProcessModule::GetLastResult(
  vtkConnectionID connectionID, vtkTypeUInt32 server)
{
  return this->ConnectionManager->GetLastResult(connectionID, server);
}

//-----------------------------------------------------------------------------
int vtkProcessModule::SendStream(vtkConnectionID connectionID, 
  vtkTypeUInt32 server, vtkClientServerStream& stream, int resetStream/*=1*/)
{
  if (stream.GetNumberOfMessages() < 1)
    {
    return 0;
    }
  int ret = this->ConnectionManager->SendStream(connectionID,
    server, stream, resetStream);
 
  // If send failed on a Client, it means that the server connection was closed.
  // So currently, we exit the client.
 
  if (ret != 0 && this->GUIHelper)
    {
    cout << "Connection Error: Server connection closed!" << endl;
    }
  return ret;
}

//-----------------------------------------------------------------------------
void vtkProcessModule::Initialize()
{
  this->InitializeInterpreter();
}

//-----------------------------------------------------------------------------
void vtkProcessModule::Finalize()
{
  this->SetGUIHelper(0);
  this->FinalizeInterpreter();
}

//-----------------------------------------------------------------------------
void vtkProcessModule::InitializeInterpreter()
{
  if (this->Interpreter)
    {
    return;
    }
  vtkMultiThreader::SetGlobalMaximumNumberOfThreads(1);

  // Create the interpreter and supporting stream.
  this->Interpreter = vtkClientServerInterpreter::New();

  // Setup a callback for the interpreter to report errors.
  this->InterpreterObserver = vtkCallbackCommand::New();
  this->InterpreterObserver->SetCallback(
    &vtkProcessModule::InterpreterCallbackFunction);
  this->InterpreterObserver->SetClientData(this);
  this->Interpreter->AddObserver(vtkCommand::UserEvent,
    this->InterpreterObserver);



  bool needLog = false;
  if(getenv("VTK_CLIENT_SERVER_LOG"))
    {
    needLog = true;
    if(this-Options->GetClientMode())
      {
      needLog = false;
      this->GetInterpreter()->SetLogFile("paraviewClient.log");
      }
    if(this->Options->GetServerMode())
      {
      needLog = false;
      this->GetInterpreter()->SetLogFile("paraviewServer.log");
      }
    if(this->Options->GetRenderServerMode())
      {
      needLog = false;
      this->GetInterpreter()->SetLogFile("paraviewRenderServer.log");
      }
    } 
  if(needLog)
    {
    this->GetInterpreter()->SetLogFile("paraview.log");
    }

  // Assign standard IDs.
  vtkClientServerStream css;
  css << vtkClientServerStream::Assign
    << this->GetProcessModuleID() << this
    << vtkClientServerStream::End;
  this->Interpreter->ProcessStream(css);
}

//-----------------------------------------------------------------------------
void vtkProcessModule::FinalizeInterpreter()
{
  if (!this->Interpreter)
    {
    return;
    }

  // Delete the standard IDs.
  vtkClientServerStream css;
  css << vtkClientServerStream::Delete
    << this->GetProcessModuleID()
    << vtkClientServerStream::End;
  this->Interpreter->ProcessStream(css);

  // Free the interpreter and supporting stream.
  this->Interpreter->RemoveObserver(this->InterpreterObserver);
  this->InterpreterObserver->Delete();
  this->InterpreterObserver = 0;
  this->Interpreter->Delete();
  this->Interpreter = 0;
}

//-----------------------------------------------------------------------------
void vtkProcessModule::InterpreterCallbackFunction(vtkObject*,
  unsigned long eid, void* cd, void* d)
{
  reinterpret_cast<vtkProcessModule*>(cd)->InterpreterCallback(eid, d);
}

//-----------------------------------------------------------------------------
void vtkProcessModule::InterpreterCallback(unsigned long, void* pinfo)
{
  if (!this->ReportInterpreterErrors)
    {
    return;
    }
  const char* errorMessage;
  vtkClientServerInterpreterErrorCallbackInfo* info
    = static_cast<vtkClientServerInterpreterErrorCallbackInfo*>(pinfo);
  const vtkClientServerStream& last = this->Interpreter->GetLastResult();
  if(last.GetNumberOfMessages() > 0 &&
    (last.GetCommand(0) == vtkClientServerStream::Error) &&
    last.GetArgument(0, 0, &errorMessage))
    {
    ostrstream error;
    error << "\nwhile processing\n";
    info->css->PrintMessage(error, info->message);
    error << ends;
    vtkErrorMacro(<< errorMessage << error.str());
    error.rdbuf()->freeze(0);
    vtkErrorMacro("Aborting execution for debugging purposes.");
    abort();
    }
}

//-----------------------------------------------------------------------------
vtkMultiProcessController* vtkProcessModule::GetController()
{
  return vtkMultiProcessController::GetGlobalController();
}

//-----------------------------------------------------------------------------
int vtkProcessModule::GetPartitionId()
{
  if (this->Options && this->Options->GetClientMode())
    {
    return 0;
    }
  if (vtkMultiProcessController::GetGlobalController())
    {
    return vtkMultiProcessController::GetGlobalController()->GetLocalProcessId();
    }
  return 0;
}


//-----------------------------------------------------------------------------
int vtkProcessModule::GetNumberOfPartitions()
{
  if (this->Options && this->Options->GetClientMode())
    {
    // TODO: This is again legacy. 
    // How can the client know which server is the caller interested in?
    return this->ConnectionManager->GetNumberOfPartitions(
      vtkProcessModuleConnectionManager::GetRootServerConnectionID());
    }
  
  if (vtkMultiProcessController::GetGlobalController())
    {
    return vtkMultiProcessController::GetGlobalController()->
      GetNumberOfProcesses();
    }
  return 1;
}
//-----------------------------------------------------------------------------
vtkClientServerID vtkProcessModule::GetUniqueID()
{
  this->UniqueID.ID ++;
  return this->UniqueID;
}

//-----------------------------------------------------------------------------
vtkClientServerID vtkProcessModule::GetProcessModuleID()
{
  vtkClientServerID id = { 2 };
  return id;
}

//-----------------------------------------------------------------------------
void vtkProcessModule::SetProcessModule(vtkProcessModule* pm)
{
  vtkProcessModule::ProcessModule = pm;
}

//-----------------------------------------------------------------------------
vtkProcessModule* vtkProcessModule::GetProcessModule()
{
  return vtkProcessModule::ProcessModule;
}

//----------------------------------------------------------------------------
void vtkProcessModule::RegisterProgressEvent(vtkObject* po, int id)
{
  vtkAlgorithm* alg = vtkAlgorithm::SafeDownCast(po);
  if ( !alg )
    {
    return;
    }
  alg->AddObserver(vtkCommand::ProgressEvent, this->Observer);
  this->ProgressHandler->RegisterProgressEvent(alg, id);
}

//----------------------------------------------------------------------------
void vtkProcessModule::SendPrepareProgress()
{
  if (!this->GUIHelper)
    {
    vtkErrorMacro("GUIHelper must be set for SendPrepareProgress.");
    return;
    }

  this->GUIHelper->SendPrepareProgress();
  
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke 
         << this->GetProcessModuleID() << "PrepareProgress" 
         << vtkClientServerStream::End;
  this->SendStream(
    vtkProcessModule::CLIENT|vtkProcessModule::DATA_SERVER, stream);
  this->ProgressRequests ++;
}

//----------------------------------------------------------------------------
void vtkProcessModule::SendCleanupPendingProgress()
{
  if ( this->ProgressRequests < 0 )
    {
    vtkErrorMacro("Internal ParaView Error: Progress requests went below zero");
    abort();
    }
  this->ProgressRequests --;
  if ( this->ProgressRequests > 0 )
    {
    return;
    }
  
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke 
         << this->GetProcessModuleID() << "CleanupPendingProgress" 
         << vtkClientServerStream::End;
  this->SendStream(
    vtkProcessModule::CLIENT|vtkProcessModule::DATA_SERVER, stream);
  
  if (!this->GUIHelper)
    {
    vtkErrorMacro("GUIHelper must be set for SendCleanupPendingProgress.");
    return;
    }
  this->GUIHelper->SendCleanupPendingProgress();
}

//----------------------------------------------------------------------------
void vtkProcessModule::PrepareProgress()
{
  this->ProgressHandler->PrepareProgress(this);
}

//----------------------------------------------------------------------------
void vtkProcessModule::CleanupPendingProgress()
{
  this->ProgressHandler->CleanupPendingProgress(this);
}

//-----------------------------------------------------------------------------
void vtkProcessModule::ProgressEvent(vtkObject *o, int val, const char* str)
{
  this->ProgressHandler->InvokeProgressEvent(this, o, val, str);
}

//-----------------------------------------------------------------------------
void vtkProcessModule::ExecuteEvent(
  vtkObject* o, unsigned long event, void* calldata)
{
  switch (event)
    {
  case vtkCommand::ProgressEvent:
      {
      int progress = static_cast<int>(*reinterpret_cast<double*>(calldata) *
        100.0);
      this->ProgressEvent(o, progress, 0);
      }
    break;
    }
}

//-----------------------------------------------------------------------------
void vtkProcessModule::SetLocalProgress(const char* filter, int progress)
{
  if (!this->GUIHelper)
    {
    vtkErrorMacro("GUIHelper must be set for SetLocalProgress " << filter
      << " " << progress);
    return;
    }
  this->GUIHelper->SetLocalProgress(filter, progress);
}

//-----------------------------------------------------------------------------
const char* vtkProcessModule::DetermineLogFilePrefix()
{
  if (this->Options)
    {
    switch (this->Options->GetProcessType())
      {
    case vtkPVOptions::PVCLIENT:
      return NULL; // don't need a log for client.
    case vtkPVOptions::PVSERVER:
      return "ServerNodeLog";
    case vtkPVOptions::PVRENDER_SERVER:
      return "RenderServerNodeLog";
    case vtkPVOptions::PVDATA_SERVER:
      return "DataServerNodeLog";
      }
    }
  return "NodeLog";
}

//-----------------------------------------------------------------------------
ofstream* vtkProcessModule::GetLogFile()
{
  return this->LogFile;
}

//-----------------------------------------------------------------------------
void vtkProcessModule::CreateLogFile()
{
  const char *prefix = this->DetermineLogFilePrefix();
  if (!prefix)
    {
    return;
    }
  
  ostrstream fileName;
  fileName << prefix << this->GetPartitionId() << ".txt"
    << ends;
  if (this->LogFile)
    {
    this->LogFile->close();
    delete this->LogFile;
    }
  this->LogFile = new ofstream(fileName.str(), ios::out);
  fileName.rdbuf()->freeze(0);
  if (this->LogFile->fail())
    {
    delete this->LogFile;
    this->LogFile = 0;
    }
}

//----------------------------------------------------------------------------
int vtkProcessModule::GetDirectoryListing(vtkConnectionID connectionID,
  const char* dir, vtkStringList* dirs, vtkStringList* files, int save)
{
  // Get the listing from the server.
  vtkClientServerStream stream;
  vtkClientServerID lid = 
    this->NewStreamObject("vtkPVServerFileListing", stream);
  stream << vtkClientServerStream::Invoke
    << lid << "GetFileListing" << dir << save
    << vtkClientServerStream::End;
  this->SendStream(connectionID, vtkProcessModule::DATA_SERVER_ROOT, stream);
  
  vtkClientServerStream result;
  if(!this->GetLastResult(connectionID, 
      vtkProcessModule::DATA_SERVER_ROOT).GetArgument(0, 0, &result))
    {
    vtkErrorMacro("Error getting file list result from server.");
    this->DeleteStreamObject(lid, stream);
    this->SendStream(connectionID, vtkProcessModule::DATA_SERVER_ROOT, stream);
    return 0;
    }
  this->DeleteStreamObject(lid, stream);
  this->SendStream(connectionID, vtkProcessModule::DATA_SERVER_ROOT, stream);

  // Parse the listing.
  if ( dirs )
    {
    dirs->RemoveAllItems();
    }
  if ( files )
    {
    files->RemoveAllItems();
    }
  if(result.GetNumberOfMessages() == 2)
    {
    int i;
    // The first message lists directories.
    if ( dirs )
      {
      for(i=0; i < result.GetNumberOfArguments(0); ++i)
        {
        const char* d;
        if(result.GetArgument(0, i, &d))
          {
          dirs->AddString(d);
          }
        else
          {
          vtkErrorMacro("Error getting directory name from listing.");
          }
        }
      }

    // The second message lists files.
    if ( files )
      {
      for(i=0; i < result.GetNumberOfArguments(1); ++i)
        {
        const char* f;
        if(result.GetArgument(1, i, &f))
          {
          files->AddString(f);
          }
        else
          {
          vtkErrorMacro("Error getting file name from listing.");
          }
        }
      }
    return 1;
    }
  else
    {
    return 0;
    }
}


//-----------------------------------------------------------------------------
int vtkProcessModule::LoadModule(vtkConnectionID connectionID,
  vtkTypeUInt32 serverFlags, const char* name, const char* directory)
{
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
    << this->GetProcessModuleID()
    << "LoadModuleInternal" << name << directory
    << vtkClientServerStream::End;
  
  this->SendStream(connectionID, serverFlags, stream);
  
  int result = 0;
  if(!this->GetLastResult(connectionID,
      this->GetRootId(serverFlags)).GetArgument(0, 0, &result))
    {
    vtkErrorMacro("LoadModule could not get result from server.");
    return 0;
    }

  return result;
}

//-----------------------------------------------------------------------------
// This method (similar to GatherInformationInternal, simply forwards the
// call to the SelfConnection.
int vtkProcessModule::LoadModuleInternal(const char* name, const char* dir)
{
  return this->ConnectionManager->LoadModule(
    vtkProcessModuleConnectionManager::GetSelfConnectionID(), name, dir);
}

//-----------------------------------------------------------------------------
void vtkProcessModule::LogStartEvent(const char* str)
{
  vtkTimerLog::MarkStartEvent(str);
  this->Timer->StartTimer();
}
  
//-----------------------------------------------------------------------------
void vtkProcessModule::LogEndEvent(const char* str)
{
  this->Timer->StopTimer();
  vtkTimerLog::MarkEndEvent(str);
  if (strstr(str, "id:") && this->LogFile)
    {
    *this->LogFile << str << ", " << this->Timer->GetElapsedTime()
      << " seconds" << endl;
    *this->LogFile << "--- Virtual memory available: "
      << this->MemoryInformation->GetAvailableVirtualMemory()
      << " KB" << endl;
    *this->LogFile << "--- Physical memory available: "
      << this->MemoryInformation->GetAvailablePhysicalMemory()
      << " KB" << endl;
    }
}
//----------------------------------------------------------------------------
void vtkProcessModule::SetLogBufferLength(int length)
{
  vtkTimerLog::SetMaxEntries(length);
}

//----------------------------------------------------------------------------
void vtkProcessModule::ResetLog()
{
  vtkTimerLog::ResetLog();
}
//----------------------------------------------------------------------------
void vtkProcessModule::SetEnableLog(int flag)
{
  vtkTimerLog::SetLogging(flag);
}


//-----------------------------------------------------------------------------
void vtkProcessModule::SetStreamBlock(vtkConnectionID id, int val)
{
  if (this->ConnectionManager->GetStreamBlock(id) == val)
    {
    return;
    }
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
    << this->GetProcessModuleID()
    << "SetGlobalStreamBlockInternal" << val << vtkClientServerStream::End;
  this->SendStream(id, vtkProcessModule::DATA_SERVER, stream);
}

//-----------------------------------------------------------------------------
int vtkProcessModule::GetStreamBlock(vtkConnectionID id)
{
  return this->ConnectionManager->GetStreamBlock(id);
}

//-----------------------------------------------------------------------------
void vtkProcessModule::SetStreamBlockInternal(int val)
{
  this->ConnectionManager->SetStreamBlock(
    vtkProcessModuleConnectionManager::GetSelfConnectionID(), val);
}



//============================================================================
// Stuff that is a part of render-process module.
//-----------------------------------------------------------------------------
const char* vtkProcessModule::GetPath(const char* tag, 
  const char* relativePath, const char* file)
{
  if ( !tag || !relativePath || !file )
    {
    return 0;
    }
  int found=0;

  if(this->Options)
    {
    vtksys_stl::string selfPath, errorMsg;
    vtksys_stl::string oldSelfPath;
    if (vtksys::SystemTools::FindProgramPath(
        this->Options->GetArgv0(), selfPath, errorMsg))
      {
      oldSelfPath = selfPath;
      selfPath = vtksys::SystemTools::GetFilenamePath(selfPath);
      selfPath += "/../share/paraview-" PARAVIEW_VERSION;
      vtkstd::string str = selfPath;
      str += "/";
      str += relativePath;
      str += "/";
      str += file;
      if(vtksys::SystemTools::FileExists(str.c_str()))
        {
        this->Internals->Paths[tag] = selfPath.c_str();
        found = 1;
        }
      }
    if ( !found )
      {
      selfPath = oldSelfPath;
      selfPath = vtksys::SystemTools::GetFilenamePath(selfPath);
      selfPath += "/../../share/paraview-" PARAVIEW_VERSION;
      vtkstd::string str = selfPath;
      str += "/";
      str += relativePath;
      str += "/";
      str += file;
      if(vtksys::SystemTools::FileExists(str.c_str()))
        {
        this->Internals->Paths[tag] = selfPath.c_str();
        found = 1;
        }
      }
    }

  if (!found)
    {
    // Look in binary and installation directories
    const char** dir;
    for(dir=PARAVIEW_PATHS; !found && *dir; ++dir)
      {
      vtkstd::string fullFile = *dir;
      fullFile += "/";
      fullFile += relativePath;
      fullFile += "/";
      fullFile += file;
      if(vtksys::SystemTools::FileExists(fullFile.c_str()))
        {
        this->Internals->Paths[tag] = *dir;
        found = 1;
        }
      }
    }
  if ( this->Internals->Paths.find(tag) == this->Internals->Paths.end() )
    {
    return 0;
    }

  return this->Internals->Paths[tag].c_str();
}

//----------------------------------------------------------------------------
int vtkProcessModule::GetRenderNodePort()
{
  if ( !this->Options )
    {
    return 0;
    }
  return this->Options->GetRenderNodePort();
}

//----------------------------------------------------------------------------
char* vtkProcessModule::GetMachinesFileName()
{
  if ( !this->Options )
    {
    return 0;
    }
  return this->Options->GetMachinesFileName();
}

//----------------------------------------------------------------------------
int vtkProcessModule::GetClientMode()
{
  if ( !this->Options )
    {
    return 0;
    }
  return this->Options->GetClientMode();
}

//----------------------------------------------------------------------------
unsigned int vtkProcessModule::GetNumberOfMachines()
{
  vtkPVServerOptions *opt = vtkPVServerOptions::SafeDownCast(this->Options);
  if (!opt)
    {
    return 0;
    }
  return opt->GetNumberOfMachines();
}

//----------------------------------------------------------------------------
const char* vtkProcessModule::GetMachineName(unsigned int idx)
{
  vtkPVServerOptions *opt = vtkPVServerOptions::SafeDownCast(this->Options);
  if (!opt)
    {
    return NULL;
    }
  return opt->GetMachineName(idx);
}

//----------------------------------------------------------------------------
vtkPVServerInformation* vtkProcessModule::GetServerInformation(
  vtkConnectionID id)
{
  return this->ConnectionManager->GetServerInformation(id);
}

//-----------------------------------------------------------------------------
vtkClientServerID vtkProcessModule::GetMPIMToNSocketConnectionID(
  vtkConnectionID id)
{
  return this->ConnectionManager->GetMPIMToNSocketConnectionID(id);
}


//----------------------------------------------------------------------------
// This method leaks memory.  It is a quick and dirty way to set different 
// DISPLAY environment variables on the render server.  I think the string 
// cannot be deleted until paraview exits.  The var should have the form:
// "DISPLAY=amber1"
void vtkProcessModule::SetProcessEnvironmentVariable(int processId,
                                                       const char* var)
{
  (void)processId;
  char* envstr = vtksys::SystemTools::DuplicateString(var);
  putenv(envstr);
}

//-----------------------------------------------------------------------------
// LEGACY METHODS. 
static vtkConnectionID ServerFlagsToConnections(vtkTypeUInt32 inFlags, 
  vtkTypeUInt32& outFlags)
{
  int hasSelfConnection = 0;
  int hasServerConnection = 0;
    
  outFlags = 0;
  if (inFlags & vtkProcessModule::CLIENT)
    {
    hasSelfConnection = 1;
    }
  
  outFlags = inFlags & ~vtkProcessModule::CLIENT;
  if (outFlags)
    {
    hasServerConnection = 1;
    }

  if (hasServerConnection && hasSelfConnection)
    {
    return vtkProcessModuleConnectionManager::GetAllConnectionsID();
    }
  
  if (hasServerConnection)
    {
    return vtkProcessModuleConnectionManager::GetAllServerConnectionsID();
    }
  if (outFlags == 0)
    {
    outFlags = vtkProcessModule::DATA_SERVER_ROOT;
    }
    
  return vtkProcessModuleConnectionManager::GetSelfConnectionID(); 
}

int vtkProcessModule::LoadModule(vtkTypeUInt32 serverFlags, const char* name, 
    const char* directory)
{
  return this->LoadModule(vtkProcessModuleConnectionManager::GetAllConnectionsID(),
    serverFlags, name, directory);
}

//-----------------------------------------------------------------------------
void vtkProcessModule::GatherInformation(vtkPVInformation* info, 
  vtkClientServerID id)
{
  this->GatherInformation(
    vtkProcessModuleConnectionManager::GetAllServerConnectionsID(),
    vtkProcessModule::DATA_SERVER, info, id);
}

//-----------------------------------------------------------------------------
void vtkProcessModule::GatherInformationRenderServer(vtkPVInformation* info, 
  vtkClientServerID id)
{
  this->GatherInformation(
    vtkProcessModuleConnectionManager::GetAllServerConnectionsID(),
    vtkProcessModule::RENDER_SERVER, info, id);
}

//-----------------------------------------------------------------------------
const vtkClientServerStream& vtkProcessModule::GetLastResult(vtkTypeUInt32 server)
{
  vtkTypeUInt32 flags = 0;
  vtkConnectionID id = ::ServerFlagsToConnections(server, flags);
  return this->GetLastResult(id, flags);
}

//-----------------------------------------------------------------------------
int vtkProcessModule::SendStream(vtkTypeUInt32 server, 
    vtkClientServerStream& stream, int resetStream /*=1*/)
{
  vtkTypeUInt32 flags = 0;
  vtkConnectionID id = ::ServerFlagsToConnections(server, flags);
  return this->SendStream(id, flags, stream, resetStream); 
}

//-----------------------------------------------------------------------------
vtkClientServerID vtkProcessModule::GetMPIMToNSocketConnectionID()
{
  return this->GetMPIMToNSocketConnectionID(
    vtkProcessModuleConnectionManager::GetRootServerConnectionID());
}
//-----------------------------------------------------------------------------
vtkPVServerInformation* vtkProcessModule::GetServerInformation()
{
  vtkPVServerInformation* info = this->GetServerInformation(
    vtkProcessModuleConnectionManager::GetRootServerConnectionID());
  return (info)? info : this->ServerInformation;
}
//-----------------------------------------------------------------------------
void vtkProcessModule::SynchronizeServerClientOptions()
{
  vtkPVServerInformation* info = this->GetServerInformation();
  if (!this->Options->GetTileDimensions()[0])
    {
    this->Options->SetTileDimensions
      (info->GetTileDimensions());
    }
  if (!this->Options->GetUseOffscreenRendering())
    {
    this->Options->SetUseOffscreenRendering
      (info->GetUseOffscreenRendering());
    }
}
//-----------------------------------------------------------------------------
int vtkProcessModule::GetDirectoryListing(const char* dir, 
    vtkStringList* dirs, vtkStringList* files, int save)
{
  return this->GetDirectoryListing(
    vtkProcessModuleConnectionManager::GetRootServerConnectionID(), dir,
    dirs, files, save);
}

//-----------------------------------------------------------------------------
vtkSocketController* vtkProcessModule::GetActiveSocketController()
{
  if (!this->ActiveRemoteConnection)
    {
    return 0;
    }
  return this->ActiveRemoteConnection->GetSocketController();
}

//-----------------------------------------------------------------------------
vtkSocketController* vtkProcessModule::GetSocketController()
{
  // Get first Server Connection.
  vtkConnectionIterator  * iter = this->ConnectionManager->NewIterator();
  iter->SetMatchConnectionID(
    vtkProcessModuleConnectionManager::GetAllServerConnectionsID());
  
  vtkRemoteConnection* conn = 0;
  for (iter->Begin(); !iter->IsAtEnd(); iter->Next())
    {
    conn = vtkRemoteConnection::SafeDownCast(
      iter->GetCurrentConnection());
    if (conn)
      {
      break;
      }
    }
  iter->Delete();
  if (conn)
    {
    return conn->GetSocketController();
    }
  return 0;
}

//-----------------------------------------------------------------------------
vtkSocketController* vtkProcessModule::GetRenderServerSocketController()
{
  // Get first Server Connection.
  vtkConnectionIterator  * iter = this->ConnectionManager->NewIterator();
  iter->SetMatchConnectionID(
    vtkProcessModuleConnectionManager::GetAllServerConnectionsID());
  for (iter->Begin(); !iter->IsAtEnd(); iter->Next())
    {
    vtkServerConnection* sconn = vtkServerConnection::SafeDownCast(
      iter->GetCurrentConnection());
    if (sconn && sconn->GetRenderServerSocketController())
      {
      iter->Delete();
      return sconn->GetRenderServerSocketController();
      }
    
    vtkRemoteConnection* conn = vtkRemoteConnection::SafeDownCast(
      iter->GetCurrentConnection());
    if (conn)
      {
      iter->Delete();
      return conn->GetSocketController();
      }
    }
  iter->Delete();
  return 0;
}

//-----------------------------------------------------------------------------
void vtkProcessModule::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "LogThreshold: " << this->LogThreshold << endl;
  os << indent << "ProgressRequests: " << this->ProgressRequests << endl;
  os << indent << "ReportInterpreterErrors: " << this->ReportInterpreterErrors
    << endl;
 
  os << indent << "Interpreter: " ;
  if (this->Interpreter)
    {
    this->Interpreter->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)" << endl;
    }

  os << indent << "ProgressHandler: " ;
  if (this->ProgressHandler)
    {
    this->ProgressHandler->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)" << endl;
    }

  os << indent << "ActiveRemoteConnection: " ;
  if (this->ActiveRemoteConnection)
    {
    this->ActiveRemoteConnection->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)" << endl;
    }
  
  os << indent << "Options: ";
  if (this->Options)
    {
    this->Options->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)" << endl;
    }
    
}
