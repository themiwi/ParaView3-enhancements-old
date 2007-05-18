/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkSMClientServerRenderViewProxy.h,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

// .NAME vtkSMClientServerRenderViewProxy - Render module supporting client/server connections
//
// .SECTION Description
//
// This render module is for situations where ParaView is in a client/server
// mode.  It handles the delivery of either data or images to the desktop.  It
// also synchronizes any server side rendering with the state of the client
// (render window geometry, camera position, etc.).
//

#ifndef __vtkSMClientServerRenderViewProxy_h
#define __vtkSMClientServerRenderViewProxy_h

#include "vtkSMRenderViewProxy.h"

class vtkSMCompositeDisplayProxy;

class VTK_EXPORT vtkSMClientServerRenderViewProxy : public vtkSMRenderViewProxy
{
public:
  static vtkSMClientServerRenderViewProxy *New();
  vtkTypeRevisionMacro(vtkSMClientServerRenderViewProxy, vtkSMRenderViewProxy);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Indicates if we should locally render.
  virtual int IsRenderLocal();

  // Description:
  // Set/get the threshold for local or remote rendering.  (Subclasses may
  // override behavior of local/remote rendering.)
  vtkSetMacro(RemoteRenderThreshold, double);
  vtkGetMacro(RemoteRenderThreshold, double);

  // Multi-view methods:

  // Description:
  // Sets the size of the server render window.
  // Overridden to send the value to the composite manager.
  virtual void SetGUISize(int x, int y);

  // Description:
  // Sets the position of the view associated with this module inside
  // the server render window. (0,0) corresponds to upper left corner.
  // Overridden to send the value to the composite manager.
  virtual void SetWindowPosition(int x, int y);

  // Description:
  // Squirt is a hybrid run length encoding and bit reduction compression
  // algorithm that is used to compress images for transmition from the
  // server to client.  Value of 0 disabled all compression.  Level zero is just
  // run length compression with no bit compression (lossless).
  vtkSetMacro(SquirtLevel, int);
  vtkGetMacro(SquirtLevel, int);

  // Description:
  // Set this flag to indicate whether to calculate the reduction factor for
  // use in tree composite (or client server).
  vtkSetMacro(ReductionFactor, int);
  vtkGetMacro(ReductionFactor, int);

  // Description:
  // Get the value of the z buffer at a position. 
  // This is necessary for picking the center of rotation.
  virtual double GetZBufferValue(int x, int y);

  // Description:
  // Set the Id of the render module. This is only needed for multi view
  // and has to be unique. It is used to match client/server render RMIs
  // vtkSMMultiViewRenderModuleProxy usually takes care of this.
  vtkSetMacro(RenderModuleId, int);
  vtkGetMacro(RenderModuleId, int);

  // Description:
  // Set the proxy of the server render window. This should be set immediately
  // after the render module is created. Setting this after CreateVTKObjects()
  // has been called has no effect.  vtkSMMultiViewRenderModuleProxy usually
  // takes care of this.
  virtual void SetServerRenderWindowProxy(vtkSMProxy*);
  vtkGetObjectMacro(ServerRenderWindowProxy, vtkSMProxy);

  // Description:
  // Set the proxy of the server render sync manager. This should be set
  // immediately after the render module is created. Setting this after
  // CreateVTKObjects() has been called has no effect.
  // vtkSMMultiViewRenderModuleProxy usually takes care of this.
  virtual void SetServerRenderSyncManagerProxy(vtkSMProxy*);
  vtkGetObjectMacro(ServerRenderSyncManagerProxy, vtkSMProxy);

  // Description:
  // This flag indicates if it is possible to render on the server side.
  // This flag must be set to false when this class (or subclasses) determine 
  // that it is not possible to render on the server side at all (may be due to
  // absence of X or offscreen mesa etc.).
  vtkGetMacro(CanRenderRemote, bool);

protected:
  vtkSMClientServerRenderViewProxy();
  ~vtkSMClientServerRenderViewProxy();

  int LocalRender;

  double RemoteRenderThreshold;
  int SquirtLevel;
  int ReductionFactor;

  int RenderModuleId;

  virtual void CreateVTKObjects(int numObjects);

  vtkSMProxy* RenderSyncManagerProxy;

  vtkSMProxy* ServerRenderWindowProxy;
  vtkSMProxy* ServerRenderSyncManagerProxy;

  // Description:
  // Method called before Still Render is called.
  // Used to perform some every-still-render-setup actions.
  virtual void BeginStillRender();

  // Description:
  // Method called before Interactive Render.
  // Used to perform some every-interactive-render-setup actions.
  virtual void BeginInteractiveRender();

  // Description:
  // Subclasses should override this method to initialize the render sync
  // manager. This is called in CreateVTKObjects.
  virtual void InitializeRenderSyncPipeline();

  // Description:
  // By default, creates a pair of vtkPVDesktopDeliveryClient/Server objects.
  // Subclasses may override this method to set their own render sync manager.
  virtual void CreateRenderSyncManager();

  // Computes the reduction factor to use in compositing.  This class simply
  // uses the given reduction factor, but subclasses may want to adjust the
  // value based on past rendering performance.
  virtual void ComputeReductionFactor(int inReductionFactor);

  // Indicates if we should locally render.
  // Flag stillRender is set when this decision is to be made during StillRender
  // else it's 0 (for InteractiveRender);
  virtual int GetLocalRenderDecision(unsigned long totalMemory,
                                     int stillRender);

  // Convenience method to set ImageReductionFactor on Composity Proxy.
  // Note that this message is sent only to the client.
  virtual void SetImageReductionFactor(vtkSMProxy* compositor, int factor);

  // Convenience method to set Squirt Level on Composite Proxy.
  // Note that this message is sent only to the client.
  virtual void SetSquirtLevel(vtkSMProxy* compositor, int level);
  
  // Convenience method to set Use Compositing on RenderSyncManagerProxy and the
  // ViewHelper. The compositing flag usually actual means remote rendering.  
  // It used to be the case that the two were synonymous, hence the misnomer.  
  // However, it is still usually the case that remote rendering also enables 
  // compositing. Note that this message is sent only to the client.
  virtual void SetUseCompositing(bool use_compositing);

  // Return the servers  where the PrepareProgress request
  // must be sent when rendering. By default,
  // it is RENDER_SERVER|CLIENT, however in CompositeRenderModule,
  // when rendering locally, the progress messages need not 
  // be sent to the servers.
  virtual vtkTypeUInt32 GetRenderingProgressServers();

  // Indicates if it is possible to render on the server. This must be set
  // this to false if it is not possible to render on the server side at all.
  // Set to true, by default.
  vtkSetMacro(CanRenderRemote, bool);
  bool CanRenderRemote;
private:
  vtkSMClientServerRenderViewProxy(const vtkSMClientServerRenderViewProxy &); // Not implemented.
  void operator=(const vtkSMClientServerRenderViewProxy &); // Not implemented
};

#endif //__vtkSMClientServerRenderViewProxy_h

