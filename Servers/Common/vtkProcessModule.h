/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkProcessModule.h,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkProcessModule
// .SECTION Description
// This is the class that encapsulates all the process initialization.
// The processes module creates a vtkProcessModuleConnectionManager which
// is used to setup connections to self, and remote machines (either client or 
// servers). The Self connection manages MPI process communication, if any.
// Each connection is assigned a Unique ID. SendStream etc are expected to
// provide the ID of the connection on which the stream is to be sent. 

#ifndef __vtkProcessModule_h
#define __vtkProcessModule_h

#include "vtkObject.h"
#include "vtkClientServerID.h" // needed for UniqueID.
#include "vtkConnectionID.h" // needed for ConnectionID.

class vtkCallbackCommand;
class vtkClientServerInterpreter;
class vtkClientServerStream;
class vtkDataObject;
class vtkKWProcessStatistics;
class vtkMultiProcessController;
class vtkProcessModuleInternals;
class vtkProcessModuleObserver;
class vtkProcessModuleConnectionManager;
class vtkProcessModuleGUIHelper;
class vtkPVInformation;
class vtkPVOptions;
class vtkPVProgressHandler;
class vtkPVServerInformation;
class vtkRemoteConnection;
class vtkSocketController;
class vtkStringList;
class vtkTimerLog;

class VTK_EXPORT vtkProcessModule : public vtkObject
{
public:
  static vtkProcessModule* New();
  vtkTypeRevisionMacro(vtkProcessModule, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
  // Description: 
  // These flags are used to specify destination servers for the
  // SendStream function. Note that the new interface no longer
  // support the server flag CLIENT.
  enum ServerFlags
    {
    DATA_SERVER = 0x01,
    DATA_SERVER_ROOT = 0x02,
    RENDER_SERVER = 0x04,
    RENDER_SERVER_ROOT = 0x08,
    SERVERS = DATA_SERVER | RENDER_SERVER,
    // CLIENT is only here for the time being.
    // Step 2 of conversion should relinquish the notion of Client,
    // instead use SelfConnection.
    CLIENT = 0x10,
    CLIENT_AND_SERVERS = DATA_SERVER | CLIENT | RENDER_SERVER
    };

  enum ProgressEventEnum
    {
    PROGRESS_EVENT_TAG = 31415
    };

  static inline int GetRootId(int serverId)
    {
    if (serverId & CLIENT)
      {
      return CLIENT;
      }
    
    if (serverId == DATA_SERVER_ROOT || serverId == RENDER_SERVER_ROOT)
      {
      return serverId;
      }
    
    if (serverId == (DATA_SERVER | RENDER_SERVER) )
      {
      return DATA_SERVER_ROOT;
      }
    return serverId << 1;
    }
//ETX

//BTX
  // Description:
  // Get a directory listing for the given directory.  Returns 1 for
  // success, and 0 for failure (when the directory does not exist).
  virtual int GetDirectoryListing(vtkConnectionID connectionID, const char* dir, 
    vtkStringList* dirs, vtkStringList* files, int save);
//ETX

  // Description:
  // Legacy Method.
  virtual int GetDirectoryListing(const char* dir, 
    vtkStringList* dirs, vtkStringList* files, int save);

//BTX
  // Description:
  // Load a ClientServer wrapper module dynamically in the server
  // processes.  Returns 1 if all server nodes loaded the module and 0
  // otherwise.  connectionID identifies the connection. 
  // serverFlags can be used to indicate if this is a data server module
  // or a render server module. The directory argument may be used 
  // to specify a directory in which to look for the module. 
  virtual int LoadModule(vtkConnectionID connectionID,
    vtkTypeUInt32 serverFlags, const char* name, const char* directory);

  // Description:
  // Legacy Method.
  int LoadModule(vtkTypeUInt32 serverFlags, const char* name, 
    const char* directory);
//ETX
  
  // Description:
  // Used internally.  Do not call.  Use LoadModule instead.
  virtual int LoadModuleInternal(const char* name, const char* directory);

  // Description:
  // Returns a data object of the given type. This is a utility
  // method used to increase performance. The first time the
  // data object of a given type is requested, it is instantiated
  // and put in map. The following calls do not cause instantiation.
  // Used while comparing data types for input matching.
  vtkDataObject* GetDataObjectOfType(const char* classname);

//BTX
  // Description:
  // This is the method to be called to gather information.
  // This method require the ID of the connection from which
  // this information must be collected.
  virtual void GatherInformation(vtkConnectionID connectionID,
    vtkTypeUInt32 serverFlags, vtkPVInformation* info, vtkClientServerID id);

  // Description:
  // Legacy Methods.
  void GatherInformation(vtkPVInformation* info, vtkClientServerID id);
  void GatherInformationRenderServer(vtkPVInformation* info, vtkClientServerID id);
//ETX

  // Description:
  // Start the process modules. It will create the application
  // and start the event loop. Returns 0 on success.
  virtual int Start(int argc, char** argv);

  // Description:
  // Breaks the event loops and cleans up.
  virtual void Exit();

//BTX
  // Description:
  // These methods append commands to the given vtkClientServerStream
  // to construct or delete a vtk object.  For construction, the type
  // of the object is specified by string name and the new unique
  // object id is returned.  For deletion the object is specified by
  // its id.  These methods do not send the stream anywhere, so the
  // caller must use SendStream() to actually perform the operation.
  vtkClientServerID NewStreamObject(const char*, vtkClientServerStream& stream);
  void DeleteStreamObject(vtkClientServerID, vtkClientServerStream& stream);
  
  // Description:
  // Return the vtk object associated with the given id for the
  // client.  If the id is for an object on another node then 0 is
  // returned.
  virtual vtkObjectBase* GetObjectFromID(vtkClientServerID);

  // Description:
  // Return the last result for the specified server.  In this case,
  // the server should be exactly one of the ServerFlags, and not a
  // combination of servers.  For an MPI server the result from the
  // root node is returned.  There is no connection to the individual
  // nodes of a server.
  virtual const vtkClientServerStream& GetLastResult(vtkConnectionID connectionID,
    vtkTypeUInt32 server);

  // Description:
  // Legacy Method.
  const vtkClientServerStream& GetLastResult(vtkTypeUInt32 server);

  // Description:
  // Send a vtkClientServerStream to the specified servers.  Servers
  // are specified with a bit vector.  To send to more than one server
  // use the bitwise or operator to combine servers.  The resetStream
  // flag determines if Reset is called to clear the stream after it
  // is sent.
  int SendStream(vtkConnectionID connectionID, vtkTypeUInt32 server, 
    vtkClientServerStream& stream, int resetStream=1);

  // Description:
  // Legacy Method.
  int SendStream(vtkTypeUInt32 server, 
    vtkClientServerStream& stream, int resetStream=1);

  // Description:
  // Get the interpreter used on the local process.
  vtkGetObjectMacro(Interpreter, vtkClientServerInterpreter);

  // Description:
  // Initialize/Finalize the process module's
  // vtkClientServerInterpreter.
  virtual void InitializeInterpreter();
  virtual void FinalizeInterpreter();
//ETX

  // Description:
  // Initialize and finalize process module.
  void Initialize();
  void Finalize();
  
  // Description:
  // Set/Get whether to report errors from the Interpreter.
  vtkGetMacro(ReportInterpreterErrors, int);
  vtkSetMacro(ReportInterpreterErrors, int);
  vtkBooleanMacro(ReportInterpreterErrors, int);

  // Description:
  // This is the controller used to communicate with the MPI nodes
  // by the SelfConnection. 
  vtkMultiProcessController* GetController();

  // Description:
  // Get the partition piece.  -1 means no assigned piece.
  virtual int GetPartitionId();

  // Description:
  // Get the number of processes participating in sharing the data.
  virtual int GetNumberOfPartitions();
//BTX 
  // Description:
  // Get a unique vtkClientServerID for this process module.
  vtkClientServerID GetUniqueID();

  // Description:
  // Get the vtkClientServerID used for the ProcessModule.
  vtkClientServerID GetProcessModuleID();
//ETX
  // Description:
  // Get/Set the global process module.
  static vtkProcessModule* GetProcessModule();
  static void SetProcessModule(vtkProcessModule* pm);

  // Description:
  // Register object with progress handler.
  void RegisterProgressEvent(vtkObject* po, int id);
 
  // Description:
  // Internal method--called when a progress event is received.
  void ProgressEvent(vtkObject *o, int val, const char* filter);

  // Description:
  virtual void SendPrepareProgress();
  virtual void SendCleanupPendingProgress();

  // Description:
  // This method is called before progress reports start comming.
  void PrepareProgress();

  // Description:
  // This method is called after force update to clenaup all the pending
  // progresses.
  void CleanupPendingProgress();

  // Description:
  // Set the local progress. This simply forwards the call to GUIHelper,
  // if any.
  void SetLocalProgress(const char* filter, int progress);
  vtkGetMacro(ProgressRequests, int);
  vtkSetMacro(ProgressRequests, int);
  vtkGetObjectMacro(ProgressHandler, vtkPVProgressHandler);

  // Description:
  // Set and get the application options
  vtkGetObjectMacro(Options, vtkPVOptions);
  virtual void SetOptions(vtkPVOptions* op);

  // Description:
  // Set the gui helper
  void SetGUIHelper(vtkProcessModuleGUIHelper*);

 // Description:
  // Get a pointer to the log file.
  ofstream* GetLogFile();
  virtual void CreateLogFile();

  // Description:
  // For loggin from Tcl start and end execute events.  We do not have c
  // pointers to all filters.
  void LogStartEvent(const char* str);
  void LogEndEvent(const char* str);

  // Description:
  // More timer log access methods.  Static methods are not accessible 
  // from tcl.  We need a timer object on all procs.
  void SetLogBufferLength(int length);
  void ResetLog();
  void SetEnableLog(int flag);

  // Description:
  // Time threshold for event (start-end) when getting the log with indents.
  // We do not have a timer object on all procs.  Statics do not work with Tcl.
  vtkSetMacro(LogThreshold, double);
  vtkGetMacro(LogThreshold, double);

//BTX
  // Description:
  // I am experimenting with streaming. This turns streaming on and off.
  // When this value is zero, pipelines do not update.
  // When the flag is turned on, then the pipeline streams.
  void SetStreamBlock(vtkConnectionID id, int val);
  int GetStreamBlock(vtkConnectionID id);
//ETX
  void SetStreamBlockInternal(int val);

  // Description:
  // We need to get the data path for the demo on the server.
  const char* GetPath(const char* tag, const char* relativePath, const char* file);
  
  // Description:  
  // This method leaks memory.  It is a quick and dirty way to set different 
  // DISPLAY environment variables on the render server.  I think the string 
  // cannot be deleted until paraview exits.  The var should have the form:
  // "DISPLAY=amber1"
  virtual void SetProcessEnvironmentVariable(int processId, const char* var);

  // Description:
  // Propagate from the options so that it is available in CS
  int GetRenderNodePort();
  char* GetMachinesFileName();
  int GetClientMode();
  unsigned int GetNumberOfMachines();
  const char* GetMachineName(unsigned int idx);

//BTX
  // Description:
  // Earlier, the ServerInformation was synchronized with the ClientOptions.
  // This no longer is appropriate. Hence, we provide access to the server information
  // on each connection.
  vtkPVServerInformation* GetServerInformation(vtkConnectionID id);

  // Description:
  // Legacy Method
  vtkPVServerInformation* GetServerInformation();

  // Description:
  // Get the ID used for MPIMToNSocketConnection for the given connection.
  vtkClientServerID GetMPIMToNSocketConnectionID(vtkConnectionID id);
  
  // Description:
  // Legacy Method.
  vtkClientServerID GetMPIMToNSocketConnectionID(); 
//ETX

  // Description:
  // Synchronizes the Client options with the specified server.
  // Not sure this is applicable in anything but legacy ParaView.
  // Synchronizes with the the first server connection.
  void SynchronizeServerClientOptions();
  
  // Description:
  // Legacy Methods. These methods returns the controller for the first
  // server connection for now. Need to figure out how to get rid of them.
  vtkSocketController* GetSocketController();
  vtkSocketController* GetRenderServerSocketController();
  

  // Description:
  // Get the active remote connection.
  vtkGetObjectMacro(ActiveRemoteConnection, vtkRemoteConnection);

  // Description:
  // Get the socket controller associated with the ActiveRemoteConnection;
  vtkSocketController* GetActiveSocketController();

  // Description:
  // Get and Set the application installation directory
//BTX
  enum CommunicationIds
  {
    MultiDisplayDummy=948346,
    MultiDisplayRootRender,
    MultiDisplaySatelliteRender,
    MultiDisplayInfo,
    PickBestProc,
    PickBestDist2,
    IceTWinInfo,
    IceTNumTilesX,
    IceTNumTilesY,
    IceTTileRanks,
    IceTRenInfo,
    GlyphNPointsGather,
    GlyphNPointsScatter,
    TreeCompositeDataFlag,
    TreeCompositeStatus,
    DuplicatePDNProcs,
    DuplicatePDNRecLen,
    DuplicatePDNAllBuffers,
    IntegrateAttrInfo,
    IntegrateAttrData,
    PickMakeGIDs,
    TemporalPickHasData,
    TemporalPicksData
  };

//ETX
protected:
  vtkProcessModule();
  ~vtkProcessModule();

  static vtkProcessModule* ProcessModule;

  static void InterpreterCallbackFunction(vtkObject* caller,
    unsigned long eid, void* cd, void* d);
  virtual void InterpreterCallback(unsigned long eid, void*);

  // Description:
  // Detemines the log file prefix depending upon node type.
  virtual const char* DetermineLogFilePrefix();


  // Description:
  // Execute event on callback.
  void ExecuteEvent(vtkObject* o, unsigned long event, void* calldata);

  // Description:
  // Called to set up the connections, if any. 
  // Returns 0 on error, 1 on success.
  int InitializeConnections();

  // Description:
  // Connect to a remote server or client already waiting for us.
  // Returns 0 on error, 1 on success.
  int ConnectToRemote();

  // Description:
  // Setup a wait connection that is waiting for a remote process to
  // connect to it.  This can be either the client or the server.
  // Returns 0 on error, 1 on success.
  int SetupWaitForConnection();

  // Description:
  // Return 1 if the connection should wait, and 0 if the connet
  int ShouldWaitForConnection();
  

  // Description:
  // Called on client in reverse connection mode. Returns after the
  // client has connected to a RenderServer/DataServer (in case of 
  // client-render-server) or Server.
  int ClientWaitForConnection();
  
  // Description:
  // ProcessModule runs on a SingleThread. Hence, we have the notion 
  // of which connection has some activity at a given time. This "active"
  // connection is identified by ActiveRemoteConnection. This only applies 
  // to remote connections. A remote connection is active
  // when it is processing some stream on the SelfConnection.
  // It is the responsibility of vtkRemoteConnection (and subclasses)
  // to set this pointer appropriately. Note that this is reference counted.
  vtkRemoteConnection* ActiveRemoteConnection;
  void SetActiveRemoteConnection(vtkRemoteConnection*);
  //BTX
  // so that this class can access SetActiveConnection.
  friend class vtkRemoteConnection;
  //ETX

  
  vtkClientServerID UniqueID;
  
  vtkClientServerInterpreter* Interpreter;
  vtkCallbackCommand* InterpreterObserver;
  int ReportInterpreterErrors;
  //BTX
  friend class vtkProcessModuleObserver;
  //ETX

  vtkProcessModuleInternals* Internals;
  vtkProcessModuleObserver* Observer;
  vtkProcessModuleConnectionManager* ConnectionManager;

  vtkPVProgressHandler* ProgressHandler;
  int ProgressRequests;

  vtkPVOptions* Options;
  vtkProcessModuleGUIHelper* GUIHelper;

  // Description:
  // A system wide server information object.
  vtkPVServerInformation* ServerInformation;
  double LogThreshold;
  ofstream *LogFile;
  vtkTimerLog* Timer;
  vtkKWProcessStatistics* MemoryInformation;

private:
  vtkProcessModule(const vtkProcessModule&); // Not implemented.
  void operator=(const vtkProcessModule&); // Not implemented.
};


#endif

