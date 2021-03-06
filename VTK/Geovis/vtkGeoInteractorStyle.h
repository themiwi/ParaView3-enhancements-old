/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkGeoInteractorStyle.h,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
// .NAME vtkGeoInteractorStyle - Interaction for a globe
//
// .SECTION Description
// vtkGeoInteractorStyle contains interaction capabilities for a geographic
// view including orbit, zoom, and tilt. It also includes a compass widget
// for changing view parameters.
//
// .SECTION See Also
// vtkCompassWidget vtkInteractorStyle
   
#ifndef __vtkGeoInteractorStyle_h
#define __vtkGeoInteractorStyle_h

#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkSmartPointer.h" // for SP

class vtkCamera;
class vtkCommand;
class vtkCompassWidget;
class vtkGeoCamera;
class vtkUnsignedCharArray;

class VTK_GEOVIS_EXPORT vtkGeoInteractorStyle : 
  public vtkInteractorStyleTrackballCamera
{
public:
  static vtkGeoInteractorStyle *New();
  vtkTypeRevisionMacro(vtkGeoInteractorStyle, 
                       vtkInteractorStyleTrackballCamera);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Event bindings
  virtual void OnEnter();
  virtual void OnLeave();
  virtual void OnMouseMove();
  virtual void OnLeftButtonUp();
  virtual void OnMiddleButtonUp();
  virtual void OnRightButtonUp();
  virtual void OnLeftButtonDown();
  virtual void OnMiddleButtonDown();
  virtual void OnRightButtonDown();
  virtual void OnChar();
  
  virtual void RubberBandZoom();
  virtual void Pan();
  virtual void Dolly();

  // Public for render callback.
  void RedrawRectangle();

  // See cxx for description of why we need this method.
  void StartState(int newstate);

  // Used for updating the terrain.
  vtkGeoCamera* GetGeoCamera();

  // Description:
  // This can be used to set the camera to the standard view of the earth.
  void ResetCamera();
  
  //! Called when the sub widgets have an interaction
  void WidgetInteraction(vtkObject *caller);

  // Description:
  // Set/Get the Interactor wrapper being controlled by this object.
  // (Satisfy superclass API.)
  virtual void SetInteractor(vtkRenderWindowInteractor *interactor);

  int ViewportToWorld(double x, double y, 
                      double &wx, double &wy, double &wz);
  void WorldToLongLat(double wx, double wy, double wz,
                      double &lon, double &lat);
  void ViewportToLongLat(double x, double y, 
                         double &lon, double &lat);
  int GetRayIntersection(double origin[3], 
                         double direction[3], 
                         double intersection[3]);

  // Description:
  // Override to make the renderer use this camera subclass
  virtual void SetCurrentRenderer(vtkRenderer*);

  // Decription:
  // Whether to lock the heading a particular value during pan.
  vtkGetMacro(LockHeading, bool);
  vtkSetMacro(LockHeading, bool);
  vtkBooleanMacro(LockHeading, bool);
  
  // Decription:
  // Called after camera properties are modified
  void ResetCameraClippingRange();

protected:
  vtkGeoInteractorStyle();
  ~vtkGeoInteractorStyle();

  // To avoid a warning.
  // We should really inherit directy from vtkInteractorStyle
  virtual void Dolly(double);  
  
  void OnTimer();
  // Used to get a constant speed regardless of frame rate.
  double LastTime;

  // Rubberband zoom has a verification stage.
  int RubberBandExtent[4];
  int RubberBandExtentEnabled;
  int RenderCallbackTag;
  void EnableRubberBandRedraw();
  void DisableRubberBandRedraw();
  bool InRubberBandRectangle(int x, int y);
  void DrawRectangle();
    
  void KeepCameraAboveGround(vtkCamera* camera);
  void UpdateLights();  
  void GetPanCenter(double &px, double &py);

  int StartPosition[2];
  int EndPosition[2];
  int DraggingRubberBandBoxState;
  double MotionFactor;
  vtkUnsignedCharArray *PixelArray;
  int PixelDims[2];
  bool LockHeading;

//BTX
  vtkSmartPointer<vtkGeoCamera> GeoCamera;

  // widget handling members
  vtkSmartPointer<vtkCompassWidget> CompassWidget;
  vtkSmartPointer<vtkCommand> EventCommand;

//ETX
  
private:
  vtkGeoInteractorStyle(const vtkGeoInteractorStyle&);  // Not implemented.
  void operator=(const vtkGeoInteractorStyle&);  // Not implemented.
};

#endif
