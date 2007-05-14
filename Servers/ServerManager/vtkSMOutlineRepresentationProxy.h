/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMOutlineRepresentationProxy.h,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMOutlineRepresentationProxy - representation that can be used to
// show a 3D outline in a render view.
// .SECTION Description
// vtkSMOutlineRepresentationProxy is a concrete representation that can be used
// to render the outline in a vtkSMRenderViewProxy. It uses a
// vtkOutlineFilter to convert non-polydata input to polydata that can be
// rendered. 

#ifndef __vtkSMOutlineRepresentationProxy_h
#define __vtkSMOutlineRepresentationProxy_h

#include "vtkSMPropRepresentationProxy.h"

class VTK_EXPORT vtkSMOutlineRepresentationProxy : 
  public vtkSMPropRepresentationProxy
{
public:
  static vtkSMOutlineRepresentationProxy* New();
  vtkTypeRevisionMacro(vtkSMOutlineRepresentationProxy, vtkSMPropRepresentationProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Returns whether this representation shows selection.
  // Overridden to turn off selection visibility if no "Selection" object is
  // set.
  virtual bool GetSelectionVisibility();

  // Description:
  // Called to update the Representation. 
  // Overridden to ensure that UpdateSelectionPropVisibility() is called.
  virtual void Update(vtkSMViewProxy* view);
  virtual void Update() { this->Superclass::Update(); };

  // Description:
  // Adds to the passed in collection the props that representation has which
  // are selectable. The passed in collection object must be empty. There is
  // no selectable Prop in this representation.
  virtual void GetSelectableProps(vtkCollection*) { return;};

  // Description:
  // Given a surface selection, this function returns a new
  // vtkSelection for the selected cells/points in the input of this
  // representation. Since there is no surface selection in this 
  // representation, this will just return;
  virtual void ConvertSurfaceSelectionToVolumeSelection(
    vtkSelection* input, vtkSelection* output) { return; };

//BTX
protected:
  vtkSMOutlineRepresentationProxy();
  ~vtkSMOutlineRepresentationProxy();

  // Description:
  // This representation needs a surface compositing strategy.
  // Overridden to request the correct type of strategy from the view.
  virtual bool InitializeStrategy(vtkSMViewProxy* view);

    // Description:
  // This method is called at the beginning of CreateVTKObjects().
  // This gives the subclasses an opportunity to set the servers flags
  // on the subproxies.
  // If this method returns false, CreateVTKObjects() is aborted.
  virtual bool BeginCreateVTKObjects(int numObjects);

  // Description:
  // This method is called after CreateVTKObjects(). 
  // This gives subclasses an opportunity to do some post-creation
  // initialization.
  virtual bool EndCreateVTKObjects(int numObjects);

  // Description:
  // Called when a representation is added to a view. 
  // Returns true on success.
  // Currently a representation can be added to only one view.
  virtual bool AddToView(vtkSMViewProxy* view);

  // Description:
  // Called to remove a representation from a view.
  // Returns true on success.
  // Currently a representation can be added to only one view.
  virtual bool RemoveFromView(vtkSMViewProxy* view);

  // Description:
  // Updates selection prop visibility based on whether selection can actually
  // be shown.
  void UpdateSelectionPropVisibility();

  vtkSMSourceProxy* OutlineFilter;
  vtkSMProxy* Mapper;
  vtkSMProxy* Prop3D;
  vtkSMProxy* Property;

  // TODO: provide mechanism to share ExtractSelection and
  // SelectionGeometryFilter among representations.

  // Proxies for the selection pipeline.
  vtkSMSourceProxy* ExtractSelection;
  vtkSMSourceProxy* SelectionGeometryFilter;
  vtkSMProxy* SelectionMapper;
  vtkSMProxy* SelectionLODMapper;
  vtkSMProxy* SelectionProp3D;
  vtkSMProxy* SelectionProperty;

private:
  vtkSMOutlineRepresentationProxy(const vtkSMOutlineRepresentationProxy&); // Not implemented
  void operator=(const vtkSMOutlineRepresentationProxy&); // Not implemented
//ETX
};

#endif

