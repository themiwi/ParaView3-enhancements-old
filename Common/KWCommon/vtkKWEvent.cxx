/*=========================================================================

  Module:    $RCSfile: vtkKWEvent.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWEvent.h"
#include "vtkCommand.h"

static const char *vtkKWEventStrings[] = {
  "KWWidgetEvents",
  "MessageDialogInvokeEvent",
  "FocusInEvent",
  "FocusOutEvent",
  // VV    
  "AngleVisibilityChangedEvent",
  "AnnotationColorChangedEvent",
  "ApplicationAreaChangedEvent",
  "BackgroundColorChangedEvent",
  "ContourAnnotationActiveChangedEvent",
  "ContourAnnotationAddAtPositionEvent",
  "ContourAnnotationAddEvent",
  "ContourAnnotationColorByScalarsChangedEvent",
  "ContourAnnotationColorChangedEvent",
  "ContourAnnotationComputeStatisticsEvent",
  "ContourAnnotationLineWidthChangedEvent",
  "ContourAnnotationOpacityChangedEvent",
  "ContourAnnotationRemoveAllEvent",
  "ContourAnnotationRemoveEvent",
  "ContourAnnotationSurfacePropertyChangedEvent",
  "ContourAnnotationSurfaceQualityChangedEvent",
  "ContourAnnotationSurfaceRepresentationChangedEvent",
  "ContourAnnotationVisibilityChangedEvent",
  "ControlLeftMouseOptionChangedEvent",
  "ControlMiddleMouseOptionChangedEvent",
  "ControlRightMouseOptionChangedEvent",
  "CroppingPlanesPositionChangedEvent",
  "CroppingRegionFlagsChangedEvent",
  "CroppingRegionsVisibilityChangedEvent",
  "Cursor3DInteractiveStateChangedEvent",
  "Cursor3DPositionChangedEvent",
  "Cursor3DPositionChangingEvent",
  "Cursor3DTypeChangedEvent",
  "Cursor3DVisibilityChangedEvent",
  "Cursor3DXColorChangedEvent",
  "Cursor3DYColorChangedEvent",
  "Cursor3DZColorChangedEvent",
  "DistanceVisibilityChangedEvent",
  "EnableShadingEvent",
  "GradientOpacityFunctionChangedEvent",
  "GradientOpacityFunctionPresetApplyEvent",
  "GradientOpacityStateChangedEvent",
  "HistogramChangedEvent",
  "ImageAngleVisibilityChangedEvent",
  "ImageBackgroundColorChangedEvent",
  "ImageCameraFocalPointAndPositionChangedEvent",
  "ImageCameraResetEvent",
  "ImageColorMappingEvent",
  "ImageCornerAnnotationChangedEvent",
  "ImageDistanceVisibilityChangedEvent",
  "ImageHeaderAnnotationChangedEvent",
  "ImageMouseBindingChangedEvent",
  "ImageScaleBarColorChangedEvent",
  "ImageScaleBarVisibilityChangedEvent",
  "ImageSliceChangedEvent",
  "ImageZoomFactorChangedEvent",
  "InteractiveRenderStartEvent",
  "LeftMouseOptionChangedEvent",
  "LightActiveChangedEvent",
  "LightColorChangedEvent",
  "LightIntensityChangedEvent",
  "LightPositionChangedEvent",
  "LightVisibilityChangedEvent",
  "LightboxOrientationChangedEvent",
  "LightboxResolutionChangedEvent",
  "Marker2DColorChangedEvent",
  "Marker2DPositionChangedEvent",
  "Marker2DVisibilityChangedEvent",
  "Marker3DAddMarkerEvent",
  "Marker3DColorChangedEvent",
  "Marker3DPositionChangedEvent",
  "Marker3DRemoveAllMarkersEvent",
  "Marker3DRemoveSelectedMarkerEvent",
  "Marker3DVisibilityChangedEvent",
  "MaterialPropertyChangedEvent",
  "MaterialPropertyChangingEvent",
  "MiddleMouseOptionChangedEvent",
  "MouseBindingChangedEvent",
  "MouseOperationsChangedEvent",
  "NotebookHidePageEvent",
  "NotebookPinPageEvent",
  "NotebookRaisePageEvent",
  "NotebookShowPageEvent",
  "NotebookUnpinPageEvent",
  "ObliqueProbeColorChangedEvent",
  "ObliqueProbeMovementEvent",
  "ObliqueProbeResetEvent",
  "ObliqueProbeScalarsVisibilityChangedEvent",
  "ObliqueProbeVisibilityChangedEvent",
  "PerspectiveViewAngleChangedEvent",
  "PluginFilterApplyEvent",
  "PluginFilterApplyPrepareEvent",
  "PluginFilterCancelEvent",
  "PluginFilterListEvent",
  "PluginFilterListAddedEvent",
  "PluginFilterListRemovedEvent",
  "PluginFilterRedoEvent",
  "PluginFilterRemoveMeshEvent",
  "PluginFilterSelectEvent",
  "PluginFilterUndoEvent",
  "PrinterDPIChangedEvent",
  "ProbeInformationChangedEvent",
  "ProbeInformationOffEvent",
  "ProjectionTypeChangedEvent",
  "ReceiveRemoteSessionEvent",
  "RenderEvent",
  "RenderWidgetInSelectionFrameChangedEvent",
  "RightMouseOptionChangedEvent",
  "ScalarColorFunctionChangedEvent",
  "ScalarColorFunctionPresetApplyEvent",
  "ScalarComponentChangedEvent",
  "ScalarComponentWeightChangedEvent",
  "ScalarComponentWeightChangingEvent",
  "ScalarOpacityFunctionChangedEvent",
  "ScalarOpacityFunctionPresetApplyEvent",
  "ScaleBarVisibilityChangedEvent",
  "ShiftLeftMouseOptionChangedEvent",
  "ShiftMiddleMouseOptionChangedEvent",
  "ShiftRightMouseOptionChangedEvent",
  "StandardInteractivityChangedEvent",
  "SurfacePropertyChangedEvent",
  "SurfacePropertyChangingEvent",
  "SwitchToVolumeProEvent",
  "TransferFunctionsChangedEvent",
  "TransferFunctionsChangingEvent",
  "UserInterfaceVisibilityChangedEvent",
  "ViewAnnotationChangedEvent",
  "ViewSelectedEvent",
  "VolumeBackgroundColorChangedEvent",
  "VolumeBlendModeChangedEvent",
  "VolumeBoundingBoxColorChangedEvent",
  "VolumeBoundingBoxVisibilityChangedEvent",
  "VolumeCameraResetEvent",
  "VolumeCornerAnnotationChangedEvent",
  "VolumeDistanceVisibilityChangedEvent",
  "VolumeFlySpeedChangedEvent",
  "VolumeHeaderAnnotationChangedEvent",
  "VolumeMapperComputeGradientsStartEvent",
  "VolumeMapperComputeGradientsEndEvent",
  "VolumeMapperComputeGradientsProgressEvent",
  "VolumeMapperRenderStartEvent",
  "VolumeMapperRenderEndEvent",
  "VolumeMapperRenderProgressEvent",
  "VolumeMaterialPropertyChangedEvent",
  "VolumeMaterialPropertyChangingEvent",
  "VolumeMouseBindingChangedEvent",
  "VolumeOrientationMarkerColorChangedEvent",
  "VolumeOrientationMarkerVisibilityChangedEvent",
  "VolumePropertyChangedEvent",
  "VolumePropertyChangingEvent",
  "VolumeReformatBoxVisibilityChangedEvent",
  "VolumeReformatChangedEvent",
  "VolumeReformatManipulationStyleChangedEvent",
  "VolumeReformatPlaneChangedEvent",
  "VolumeReformatThicknessChangedEvent",
  "VolumeScalarBarComponentChangedEvent",
  "VolumeScalarBarWidgetChangedEvent",
  "VolumeScaleBarColorChangedEvent",
  "VolumeScaleBarVisibilityChangedEvent",
  "VolumeStandardCameraViewEvent",
  "VolumeZSamplingChangedEvent",
  "WindowInterfaceChangedEvent",
  "WindowLayoutChangedEvent",
  "WindowLevelChangedEvent",
  "WindowLevelChangingEndEvent",
  "WindowLevelChangingEvent",
  "WindowLevelResetEvent",
  // PV
  "ErrorMessageEvent",
  "InitializeTraceEvent",
  "ManipulatorModifiedEvent",
  "WarningMessageEvent",
  "WidgetModifiedEvent",
  //
  "FinalBogusNotUsedEvent",
  0
};

unsigned long vtkKWEvent::GetEventIdFromString(const char* cevent)
{
  unsigned long event = vtkCommand::GetEventIdFromString(cevent);
  if ( event != vtkCommand::NoEvent )
    {
    return event;
    }
  
  int cc;
  for ( cc = 0; vtkKWEventStrings[cc] != 0; cc ++ )
    {
    if ( strcmp(cevent, vtkKWEventStrings[cc]) == 0 )
      {
      return cc + vtkKWEvent::KWWidgetEvents;
      }
    }
  return vtkCommand::NoEvent;
}

const char *vtkKWEvent::GetStringFromEventId(unsigned long event)
{
  static unsigned long numevents = 0;
  
  // find length of table
  if (!numevents)
    {
    while (vtkKWEventStrings[numevents] != NULL)
      {
      numevents++;
      }
    }
  if ( event < vtkKWEvent::KWWidgetEvents ) 
    {
    return vtkCommand::GetStringFromEventId(event);
    }

  event -= 2000;

  if (event < numevents)
    {
    return vtkKWEventStrings[event];
    }
  else
    {
    return "UnknownEvent";
    }
}


