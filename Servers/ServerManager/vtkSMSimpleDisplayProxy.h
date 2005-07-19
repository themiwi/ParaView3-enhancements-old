/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMSimpleDisplayProxy.h,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMSimpleDisplayProxy - a simple display proxy.
// .SECTION Description
// vtkSMSimpleDisplayProxy is a sink for display objects. 
// This is the superclass for objects that display PVParts.
// Not meant to be used directly, although this implements the simplest
// serial display with no LOD.

#ifndef __vtkSMSimpleDisplayProxy_h
#define __vtkSMSimpleDisplayProxy_h

#include "vtkSMDisplayProxy.h"
class vtkSMProxy;
class vtkPVDataInformation;
class vtkPVArrayInformation;
class vtkSMSourceProxy;

class VTK_EXPORT vtkSMSimpleDisplayProxy : public vtkSMDisplayProxy
{
public:
  static vtkSMSimpleDisplayProxy* New();
  vtkTypeRevisionMacro(vtkSMSimpleDisplayProxy, vtkSMDisplayProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // For flip books.
  virtual void CacheUpdate(int idx, int total);

  // Description:
  // Method gets called to set input when using Input property.
  // Internally leads to a call to SetInput.
  virtual void AddInput(vtkSMSourceProxy* input, const char*, int, int);

  // Description:
  // Connect the VTK data object to display pipeline.
  virtual void SetInput(vtkSMProxy* input);
  
  // Description:
  // This method calls a ForceUpdate on the UpdateSuppressor
  // if the Geometry is not valid. 
  virtual void Update();
  
  // Description:
  // Invalidates Geometry. Results in removal of any cached geometry. Also,
  // marks the current geometry as invalid, thus a subsequent call to Update
  // will result in call to ForceUpdate on the UpdateSuppressor(s), if any.
  virtual void InvalidateGeometry();

  // Description:
  // Called when the display is added/removed to/from a RenderModule.
  virtual void AddToRenderModule(vtkSMRenderModuleProxy*);
  virtual void RemoveFromRenderModule(vtkSMRenderModuleProxy*);

  // Description:
  // Set the representation for this display.
  virtual void SetRepresentation(int representation);

  // Description:
  // Set Visibility of the display.
  virtual void SetVisibility(int visible);

  // Description:
  // Flag indicating if the display supports a volume rendering 
  // representation.
  vtkGetMacro(HasVolumePipeline,    int);
  vtkGetMacro(SupportsBunykMapper,  int);
  vtkGetMacro(SupportsZSweepMapper, int);
  
  // Description:
  // Flag indicating if the display is currently rendered
  // as a volume. Typically, one would not use this flag,
  // instead check status of the property "Representation".
  // Here, only for RenderModuleProxy.
  vtkGetMacro(VolumeRenderMode, int);

  // Description:
  // Method to initlaize the Volume Transfer functions 
  // (ie. Opacity Function & Color Transfer fuction).
  void ResetTransferFunctions();
  
  // Description:
  // Convenience methods for switching between volume
  // mappers.
  void SetVolumeMapperToBunyk();
  void SetVolumeMapperToPT();
  void SetVolumeMapperToZSweep();
  
  // Description:
  // Convenience method for determining which
  // volume mapper is in use
  virtual int GetVolumeMapperType();
  
  // Description:
  // This method will set the subproxy GeometryFilterProxy
  // as the input (using property "Input") on the argument
  // onProxy. 
  void SetInputAsGeometryFilter(vtkSMProxy* onProxy);

  // Description:
  // Calls MarkConsumersAsModified() on all consumers. Sub-classes
  // should add their functionality and call this.
  // Overridden to clean up cached geometry as well. 
  virtual void MarkConsumersAsModified(); 

protected:
  vtkSMSimpleDisplayProxy();
  ~vtkSMSimpleDisplayProxy();

  virtual void SetInputInternal(vtkSMSourceProxy* input);

  void ResetTransferFunctions(vtkPVDataInformation* dataInfo,
    vtkPVArrayInformation* arrayInfo);

  // Description:
  // Set up the PolyData rendering pipeline.
  virtual void SetupPipeline();
  virtual void SetupDefaults();

  // Description:
  // Set up the vtkUnstructuredGrid (Volume) rendering pipeline.
  virtual void SetupVolumePipeline();
  virtual void SetupVolumeDefaults();

 
  virtual void CreateVTKObjects(int numObjects);

  virtual void GatherGeometryInformation();

//BTX
  // This is the least intrusive way of giving vtkPVComparativeVisManager
  // access to the MapperProxy. It needs this proxy to extract and cache
  // the input geometry
  friend class vtkPVComparativeVisManager;
  friend class vtkPVComparativeVis;
  friend class vtkSMComparativeVisProxy;
  vtkGetObjectMacro(MapperProxy, vtkSMProxy);
  vtkGetObjectMacro(ActorProxy, vtkSMProxy);
  vtkGetObjectMacro(GeometryFilterProxy, vtkSMProxy);
//ETX

  vtkSMProxy *GeometryFilterProxy;
  vtkSMProxy *UpdateSuppressorProxy;
  vtkSMProxy *MapperProxy; 
  vtkSMProxy *PropertyProxy;
  vtkSMProxy *ActorProxy; 

  vtkSMProxy* VolumeFilterProxy;
  vtkSMProxy* VolumeUpdateSuppressorProxy;
  vtkSMProxy* VolumePTMapperProxy;
  vtkSMProxy* VolumeBunykMapperProxy;
  vtkSMProxy* VolumeZSweepMapperProxy;
  vtkSMProxy* VolumeActorProxy;
  vtkSMProxy* VolumePropertyProxy;
  vtkSMProxy* OpacityFunctionProxy;
  vtkSMProxy* ColorTransferFunctionProxy;

  int HasVolumePipeline; 
  int SupportsZSweepMapper;
  int SupportsBunykMapper;
  
  // Flag to avoid setting up the volume pipeline
  // if the input is not vtkUnstructuredGrid. I go as far as
  // removing all the volume subproxies as well (and not creating them at all!)
  // just to save the effort when not needed.
  int VolumeRenderMode; // Flag to tell if we are using the volume rendering.

  // Description:
  // Called when Volume rendering is turned on/off.
  virtual void VolumeRenderModeOn();
  virtual void VolumeRenderModeOff();

  // IVars to avoid unnecessary setting of values.
  int Visibility;
  int Representation;
  
  int GeometryIsValid;
  int VolumeGeometryIsValid;
  int CanCreateProxy;

  void InvalidateGeometryInternal();
private:
  vtkSMSimpleDisplayProxy(const vtkSMSimpleDisplayProxy&); // Not implemented.
  void operator=(const vtkSMSimpleDisplayProxy&); // Not implemented.
};


#endif

