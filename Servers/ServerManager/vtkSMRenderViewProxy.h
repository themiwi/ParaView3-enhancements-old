/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMRenderViewProxy.h,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMRenderViewProxy - implementation for View that includes
// render window and renderers.
// .SECTION Description
// vtkSMRenderViewProxy is a 3D view consisting for a render window and two
// renderers: 1 for 3D geometry and 1 for overlayed 2D geometry.

#ifndef __vtkSMRenderViewProxy_h
#define __vtkSMRenderViewProxy_h

#include "vtkSMViewProxy.h"

class vtkCamera;
class vtkCollection;
class vtkImageData;
class vtkPVClientServerIdCollectionInformation;
class vtkPVGenericRenderWindowInteractor;
class vtkPVRenderModuleHelper;
class vtkRenderer;
class vtkRenderWindow;
class vtkSelection;
class vtkSMRepresentationProxy;
class vtkTimerLog;

class VTK_EXPORT vtkSMRenderViewProxy : public vtkSMViewProxy
{
public:
  static vtkSMRenderViewProxy* New();
  vtkTypeRevisionMacro(vtkSMRenderViewProxy, vtkSMViewProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Callback for the triangle strips check button
  void SetUseTriangleStrips(int val);
  
  // Description:
  // Callback for the immediate mode rendering check button
  void SetUseImmediateMode(int val);
   
  // Description:
  // Access to the rendering-related objects for the GUI.
  vtkGetObjectMacro(Renderer, vtkRenderer);
  vtkGetObjectMacro(Renderer2D, vtkRenderer);
  vtkGetObjectMacro(RenderWindow, vtkRenderWindow);
  vtkGetObjectMacro(Interactor, vtkPVGenericRenderWindowInteractor);

  // Description:
  // Convenience method to set the background color.
  void SetBackgroundColorCM(double rgb[3]);

  // Description:
  // Controls whether the render module invokes abort check events.
  vtkSetMacro(RenderInterruptsEnabled,int);
  vtkGetMacro(RenderInterruptsEnabled,int);
  vtkBooleanMacro(RenderInterruptsEnabled,int);

  // Description:
  // Compute the bounding box of all visible props.
  void ComputeVisiblePropBounds(double bounds[6]);

  // Description:
  // Get the value of the z buffer at a position. 
  // This is necessary for picking the center of rotation.
  virtual double GetZBufferValue(int x, int y);

  // Description:
  // Performs a pick in the selected screen area and returns a list
  // of ClientServerIds for the representation proxies hit.
  vtkPVClientServerIdCollectionInformation* 
    Pick(int xs, int ys, int xe, int ye);

  // Description:
  // Reset camera to the given bounds.
  void ResetCamera(double bds[6]);

  // Description:
  // Reset camera. Interally calls ComputeVisiblePropBounds
  // to obtain the bounds.
  void ResetCamera();
  
  // Description:
  // This method calls UpdateInformation on the Camera Proxy
  // and sets the Camera properties according to the info
  // properties.
  void SynchronizeCameraProperties();

  // Description:
  // Generate a screenshot from the render window.
  virtual int WriteImage(const char* filename, const char* writerName);

  // Description:
  // Enable/Disable the LightKit.
  void SetUseLight(int enable);

  // Description:
  // Returns the dimensions of the first node of the render server (in
  // the argument size). Returns 1 on success, 0 on failure.
  int GetServerRenderWindowSize(int size[2]);

  // Description:
  // Called when saving server manager state.
  // Overridden to SynchronizeCameraProperties before saving the properties.
  virtual vtkPVXMLElement* SaveState(vtkPVXMLElement* root);

  // Description:
  // Returns an image data that contains a "screenshot" of the window.
  // It is the responsibility of the caller to delete the image data.
  virtual vtkImageData* CaptureWindow(int magnification);

  // Description:
  // Enable measurement of Polygons Per Second
  vtkSetClampMacro(MeasurePolygonsPerSecond, int, 0, 1);
  vtkBooleanMacro(MeasurePolygonsPerSecond, int);
  vtkGetMacro(MeasurePolygonsPerSecond, int);
  
  // Description:
  // Reset the tracking of polygons per second
  void ResetPolygonsPerSecondResults();

  // Description:
  // Get the last/maximum/average number of polygons per second
  vtkGetMacro(LastPolygonsPerSecond, double);
  vtkGetMacro(MaximumPolygonsPerSecond, double);
  vtkGetMacro(AveragePolygonsPerSecond, double);

  //BTX
  enum ProxyType
    {
    GEOMETRY,
    INPUT,
    DISPLAY
    };
  //ETX

  // Description:
  // This will look in the RenderModule's Displays and find the one
  // that corresponds to the given id (obtained by Pick()).
  // Which proxy is returned depends on the second argument (proxyType).
  // If DISPLAY, the corresponding display proxy is returned.
  // If INPUT, the input of the display proxy is returned.
  // If GEOMETRY, the geometry filter proxy is returned
  vtkSMProxy *GetProxyFromPropID(vtkClientServerID *id, int proxyType);

  // Decription:
  // Like GetProxyFromPropIds, but this lets you iterate over the displays when
  // you don't have an id to request specifically.
  vtkSMProxy *GetProxyForDisplay(int number, int proxyType);

  // Description:
  // Checks if color depth is sufficient to support selection.
  // If not, will return 0 and any calls to SelectVisibleCells will 
  // quietly return an empty selection.
  int IsSelectionAvailable();

  // Description:
  // Return a list of visible cells within the provided screen area.
  vtkSelection *SelectVisibleCells(unsigned int x0, unsigned int y0, 
    unsigned int x1, unsigned int y1);

  // Description:
  // Get/Set the cache limit (in kilobytes) for each process. If cache size
  // grows beyond the limit, no caching is done on any of the processes.
  vtkGetMacro(CacheLimit, int);
  vtkSetMacro(CacheLimit, int);

  // Description:
  // Methods called by Representation proxies to add/remove the
  // actor proxies to appropriate renderer.
  // Avoid calling these methods directly outside representation proxies.
  virtual void AddPropToRenderer(vtkSMProxy* proxy);
  virtual void AddPropToRenderer2D(vtkSMProxy* proxy);
  virtual void RemovePropFromRenderer(vtkSMProxy* proxy);
  virtual void RemovePropFromRenderer2D(vtkSMProxy* proxy);

  //BTX
  enum Strategies
    {
    SURFACE,
    VOLUME
    };
  //ETX

//BTX
protected:
  vtkSMRenderViewProxy();
  ~vtkSMRenderViewProxy();

  // Description:
  // Given the number of objects (numObjects), class name (VTKClassName)
  // and server ids ( this->GetServerIDs()), this methods instantiates
  // the objects on the server(s)
  virtual void CreateVTKObjects(int numObjects);

  // Description:
  // Called by AddRepresentation(). Subclasses can override to add 
  // observers etc.
  virtual void AddRepresentationInternal(vtkSMRepresentationProxy* rep);

  // Description:
  // Creates a new vtkSMRepresentationStrategy subclass based on the type
  // requested. \c type can be either SURFACE or VOLUME.
  virtual vtkSMRepresentationStrategy* NewStrategyInternal(
    int dataType, int type);

  // Description:
  // Set the LOD decision.
  void SetLODFlag(int val);

  // Description:
  // Called to process events.
  virtual void ProcessEvents(vtkObject* caller, unsigned long eventId, 
    void* callData);

  // Description:
  // Synchornizes the properties of the 2D and 3D renders before
  // each render.
  void SynchronizeRenderers();

  // Description:
  // Reset the camera clipping range.
  void ResetCameraClippingRange();

  // Collection of props added to the renderer.
  vtkCollection* RendererProps; 

  // Collection of props added to the renderer2D.
  vtkCollection* Renderer2DProps;
 
  vtkSMProxy* RendererProxy;
  vtkSMProxy* Renderer2DProxy;
  vtkSMProxy* ActiveCameraProxy;
  vtkSMProxy* RenderWindowProxy;
  vtkSMProxy* InteractorProxy;
  vtkSMProxy* LightKitProxy;
  vtkSMProxy* LightProxy;
  vtkSMProxy* HelperProxy;

  // Pointer to client side objects,
  // for convienience.
  vtkRenderer* Renderer2D;
  vtkRenderer* Renderer;
  vtkRenderWindow* RenderWindow;
  vtkPVGenericRenderWindowInteractor* Interactor;
  vtkCamera* ActiveCamera;
  vtkPVRenderModuleHelper* Helper;
  
  int RenderInterruptsEnabled;

  int UseTriangleStrips;
  int ForceTriStripUpdate;
  int UseImmediateMode;
  
  // Description:
  // Method called before/after Still Render is called.
  // Can be used to set GlobalLODFlag.
  virtual void BeginStillRender();
  virtual void EndStillRender();

  virtual void BeginInteractiveRender();
  virtual void EndInteractiveRender();

  virtual void PerformRender();

  vtkTimerLog *RenderTimer;
  double LastPolygonsPerSecond;
  double MaximumPolygonsPerSecond;
  double AveragePolygonsPerSecond;
  double AveragePolygonsPerSecondAccumulated;
  vtkIdType AveragePolygonsPerSecondCount;
  void CalculatePolygonsPerSecond(double time);
  int MeasurePolygonsPerSecond;
  int CacheLimit; // in KiloBytes.

  // Description:
  // Get the number of polygons this render module is rendering
  vtkIdType GetTotalNumberOfPolygons();

private:
  vtkSMRenderViewProxy(const vtkSMRenderViewProxy&); // Not implemented.
  void operator=(const vtkSMRenderViewProxy&); // Not implemented.
//ETX
};


#endif

