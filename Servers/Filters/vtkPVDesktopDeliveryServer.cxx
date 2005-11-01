/*=========================================================================

Program:   ParaView
Module:    $RCSfile: vtkPVDesktopDeliveryServer.cxx,v $

Copyright (c) Kitware, Inc.
All rights reserved.
See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPVDesktopDeliveryServer.h"

#include "vtkCallbackCommand.h"
#include "vtkCamera.h"
#include "vtkDoubleArray.h"
#include "vtkLight.h"
#include "vtkLightCollection.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkRendererCollection.h"
#include "vtkRenderer.h"
#include "vtkSquirtCompressor.h"
#include "vtkTimerLog.h"
#include "vtkUnsignedCharArray.h"

#include "vtkSmartPointer.h"

#include <vtkstd/map>

//-----------------------------------------------------------------------------

static void SatelliteStartRender(vtkObject *caller,
                                 unsigned long vtkNotUsed(event),
                                 void *clientData, void *);
static void SatelliteEndRender(vtkObject *caller,
                               unsigned long vtkNotUsed(event),
                               void *clientData, void *);
static void SatelliteStartParallelRender(vtkObject *caller,
                                         unsigned long vtkNotUsed(event),
                                         void *clientData, void *);
static void SatelliteEndParallelRender(vtkObject *caller,
                                       unsigned long vtkNotUsed(event),
                                       void *clientData, void *);

static void UseRendererSet(void *localArg, void *remoteArg, int, int);

//-----------------------------------------------------------------------------

typedef vtkstd::map<int, vtkSmartPointer<vtkRendererCollection> > RendererMapType;
class vtkPVDesktopDeliveryServerRendererMap
{
public:
  RendererMapType Renderers;
};

//-----------------------------------------------------------------------------

vtkCxxRevisionMacro(vtkPVDesktopDeliveryServer, "$Revision: 1.4 $");
vtkStandardNewMacro(vtkPVDesktopDeliveryServer);

//----------------------------------------------------------------------------
vtkPVDesktopDeliveryServer::vtkPVDesktopDeliveryServer()
{
  this->ParallelRenderManager = NULL;
  this->RemoteDisplay = 1;
  this->SquirtBuffer = vtkUnsignedCharArray::New();

  this->RendererMap = new vtkPVDesktopDeliveryServerRendererMap;

  this->SendImageBuffer = vtkUnsignedCharArray::New();

  this->Renderers->Delete();
  this->Renderers = NULL;
}

//----------------------------------------------------------------------------
vtkPVDesktopDeliveryServer::~vtkPVDesktopDeliveryServer()
{
  this->SetParallelRenderManager(NULL);
  this->SquirtBuffer->Delete();

  delete this->RendererMap;

  this->SendImageBuffer->Delete();

  this->Renderers = NULL;

  if (this->Controller && this->AddedRMIs)
    {
    this->Controller->RemoveFirstRMI(
                                 vtkPVDesktopDeliveryServer::WINDOW_ID_RMI_TAG);
    }
}

//----------------------------------------------------------------------------
void vtkPVDesktopDeliveryServer::SetController(
                                          vtkMultiProcessController *controller)
{
  vtkDebugMacro("SetController");

  if (controller && (controller->GetNumberOfProcesses() != 2))
    {
    vtkErrorMacro("vtkDesktopDelivery needs controller with 2 processes");
    return;
    }

  this->Superclass::SetController(controller);

  if (this->Controller)
    {
    this->RootProcessId = 1 - this->Controller->GetLocalProcessId();
    }
}

//----------------------------------------------------------------------------
void vtkPVDesktopDeliveryServer::SetParallelRenderManager(
                                                  vtkParallelRenderManager *prm)
{
  if (this->ParallelRenderManager == prm)
    {
    return;
    }
  this->Modified();

  if (this->ParallelRenderManager)
    {
    // Remove all observers.
    this->ParallelRenderManager->RemoveObserver(this->StartParallelRenderTag);
    this->ParallelRenderManager->RemoveObserver(this->EndParallelRenderTag);

    // Delete the reference.
    this->ParallelRenderManager->UnRegister(this);
    this->ParallelRenderManager = NULL;
    }

  this->ParallelRenderManager = prm;
  if (this->ParallelRenderManager)
    {
    // Create a reference.
    this->ParallelRenderManager->Register(this);

    if (this->RemoteDisplay)
      {
      // No need to write the image back on the render server.
      this->ParallelRenderManager->WriteBackImagesOff();
      }
    else
      {
      // Presumably someone is viewing the remote screen, perhaps in a
      // tile display.
      this->ParallelRenderManager->WriteBackImagesOn();
      }

    // Attach observers.
    vtkCallbackCommand *cbc;

    cbc = vtkCallbackCommand::New();
    cbc->SetCallback(::SatelliteStartParallelRender);
    cbc->SetClientData(this);
    this->StartParallelRenderTag
      = this->ParallelRenderManager->AddObserver(vtkCommand::StartEvent, cbc);
    // ParallelRenderManager will really delete cbc when observer is removed.
    cbc->Delete();

    cbc = vtkCallbackCommand::New();
    cbc->SetCallback(::SatelliteEndParallelRender);
    cbc->SetClientData(this);
    this->EndParallelRenderTag
      = this->ParallelRenderManager->AddObserver(vtkCommand::EndEvent, cbc);
    // ParallelRenderManager will really delete cbc when observer is removed.
    cbc->Delete();

    // Remove observers to RenderWindow.  We use the prm instead.
    if (this->ObservingRenderWindow)
      {
      this->RenderWindow->RemoveObserver(this->StartRenderTag);
      this->RenderWindow->RemoveObserver(this->EndRenderTag);
      this->ObservingRenderWindow = 0;
      }
    }
  else
    {
    // Apparently we added and then removed a ParallelRenderManager.
    // Restore RenderWindow observers.
    if (this->RenderWindow)
      {
      vtkCallbackCommand *cbc;
        
      vtkRendererCollection *rens = this->GetRenderers();
      if (rens)
        {
        vtkRenderer *ren;
        rens->InitTraversal();
        ren = rens->GetNextItem();
        if (ren)
          {
          this->ObservingRenderWindow = 1;
          
          cbc= vtkCallbackCommand::New();
          cbc->SetCallback(::SatelliteStartRender);
          cbc->SetClientData(this);
          this->StartRenderTag
            = ren->AddObserver(vtkCommand::StartEvent,cbc);
          cbc->Delete();
          
          cbc = vtkCallbackCommand::New();
          cbc->SetCallback(::SatelliteEndRender);
          cbc->SetClientData(this);
          this->EndRenderTag
            = ren->AddObserver(vtkCommand::EndEvent,cbc);
          cbc->Delete();
          }
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVDesktopDeliveryServer::SetRenderWindow(vtkRenderWindow *renWin)
{
  this->Superclass::SetRenderWindow(renWin);

  if (this->ObservingRenderWindow && this->ParallelRenderManager)
    {
    vtkRendererCollection *rens = this->GetRenderers();
    vtkRenderer *ren;
    rens->InitTraversal();
    ren = rens->GetNextItem();
    if (ren)
      {
      // Don't need the observers we just attached.
      ren->RemoveObserver(this->StartRenderTag);
      ren->RemoveObserver(this->EndRenderTag);
      this->ObservingRenderWindow = 0;
      }
    }
}

//-----------------------------------------------------------------------------
void vtkPVDesktopDeliveryServer::InitializeRMIs()
{
  this->Superclass::InitializeRMIs();

  this->Controller->AddRMI(::UseRendererSet, this,
                           vtkPVDesktopDeliveryServer::WINDOW_ID_RMI_TAG);
}

//----------------------------------------------------------------------------
void vtkPVDesktopDeliveryServer::SetRemoteDisplay(int flag)
{
  vtkDebugMacro(<< this->GetClassName() << " (" << this
                << "): setting RemoteDisplay to " << flag);
  if (this->RemoteDisplay != flag)
    {
    this->RemoteDisplay = flag;
    this->Modified();

    if (this->ParallelRenderManager)
      {
      if (this->RemoteDisplay)
        {
        // No need to write the image back on the render server.
        this->ParallelRenderManager->WriteBackImagesOff();
        }
      else
        {
        // Presumably someone is viewing the remote screen, perhaps in a
        // tile display.
        this->ParallelRenderManager->WriteBackImagesOn();
        }
      }
    }
}

//-----------------------------------------------------------------------------
void vtkPVDesktopDeliveryServer::UseRendererSet(int id)
{
  if (!this->SyncRenderWindowRenderers)
    {
    this->Renderers = this->RendererMap->Renderers[id];

    vtkRendererCollection *rens = this->RenderWindow->GetRenderers();
    vtkCollectionSimpleIterator cookie;
    vtkRenderer *ren;
    for (rens->InitTraversal(cookie);
         (ren = rens->GetNextRenderer(cookie)) != NULL; )
      {
      // Turn off everything that is not annotation.  The superclass will
      // later turn on any renderers that the client requested.
      if (ren->GetLayer() >= this->AnnotationLayer)
        {
        ren->DrawOn();
        }
      else
        {
        ren->DrawOff();
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVDesktopDeliveryServer::ReceiveWindowInformation()
{
  vtkPVDesktopDeliveryServer::WindowGeometry winGeoInfo;
  this->Controller->Receive(reinterpret_cast<int *>(&winGeoInfo),
                            vtkPVDesktopDeliveryServer::WINDOW_GEOMETRY_SIZE,
                            this->RootProcessId,
                            vtkPVDesktopDeliveryServer::WINDOW_GEOMETRY_TAG);

  // Correct window size.
  this->ClientWindowSize[0] = this->FullImageSize[0];
  this->ClientWindowSize[1] = this->FullImageSize[1];
  this->ClientRequestedImageSize[0] = this->ReducedImageSize[0];
  this->ClientRequestedImageSize[1] = this->ReducedImageSize[1];
  this->FullImageSize[0] = winGeoInfo.GUISize[0];
  this->FullImageSize[1] = winGeoInfo.GUISize[1];
  this->ReducedImageSize[0]
    = (int)(this->FullImageSize[0]/this->ImageReductionFactor);
  this->ReducedImageSize[1]
    = (int)(this->FullImageSize[1]/this->ImageReductionFactor);
  this->ClientWindowPosition[0] = winGeoInfo.Position[0];
  this->ClientWindowPosition[1] = winGeoInfo.Position[1];
  this->ClientGUISize[0] = winGeoInfo.GUISize[0];
  this->ClientGUISize[1] = winGeoInfo.GUISize[1];

  this->AnnotationLayer = winGeoInfo.AnnotationLayer;

  this->UseRendererSet(winGeoInfo.Id);

  vtkPVDesktopDeliveryServer::SquirtOptions squirtOptions;
  this->Controller->Receive(reinterpret_cast<int *>(&squirtOptions),
                            vtkPVDesktopDeliveryServer::SQUIRT_OPTIONS_SIZE,
                            this->RootProcessId,
                            vtkPVDesktopDeliveryServer::SQUIRT_OPTIONS_TAG);
  this->Squirt = squirtOptions.Enabled;
  this->SquirtCompressionLevel = squirtOptions.CompressLevel;
}

//-----------------------------------------------------------------------------
void vtkPVDesktopDeliveryServer::ReceiveRendererInformation(vtkRenderer *ren)
{
  // Not really receiving anything.  Just correcting the viewport.
  double viewport[4];
  ren->GetViewport(viewport);

  if (this->ParallelRenderManager && (this->ImageReductionFactor > 1.0))
    {
    // Undo the image reduction so that the other parallel render manager may
    // take care of it correctly.
    viewport[0] *= this->ImageReductionFactor;
    viewport[1] *= this->ImageReductionFactor;
    viewport[2] *= this->ImageReductionFactor;
    viewport[3] *= this->ImageReductionFactor;
    }

  double scaleX = (double)this->ClientWindowSize[0]/this->ClientGUISize[0];
  double scaleY = (double)this->ClientWindowSize[1]/this->ClientGUISize[1];
  viewport[0] *= scaleX;
  viewport[1] *= scaleY;
  viewport[2] *= scaleX;
  viewport[3] *= scaleY;

  double offsetX = (double)this->ClientWindowPosition[0]/this->ClientGUISize[0];
  double offsetY = (double)this->ClientWindowPosition[1]/this->ClientGUISize[1];
  if (!this->ParallelRenderManager && (this->ImageReductionFactor > 1.0))
    {
    offsetX /= this->ImageReductionFactor;
    offsetY /= this->ImageReductionFactor;
    }
  viewport[0] += offsetX;
  viewport[1] += offsetY;
  viewport[2] += offsetX;
  viewport[3] += offsetY;

  ren->SetViewport(viewport);
}

//----------------------------------------------------------------------------
void vtkPVDesktopDeliveryServer::PreRenderProcessing()
{
  vtkDebugMacro("PreRenderProcessing");

  // Send remote display flag.
  this->Controller->Send(&this->RemoteDisplay, 1, this->RootProcessId,
                         vtkPVDesktopDeliveryServer::REMOTE_DISPLAY_TAG);

  if (this->ParallelRenderManager)
    {
    // Make sure the prm has the correct image reduction factor.
    if (  this->ParallelRenderManager->GetMaxImageReductionFactor()
          < this->ImageReductionFactor)
      {
      this->ParallelRenderManager
        ->SetMaxImageReductionFactor(this->ImageReductionFactor);
      }
    this->ParallelRenderManager
      ->SetImageReductionFactor(this->ImageReductionFactor);
    this->ParallelRenderManager->AutoImageReductionFactorOff();

    // Pass UseCompositing flag.
    this->ParallelRenderManager->SetUseCompositing(this->UseCompositing);
    }

  this->ImageResized = 0;
}

//----------------------------------------------------------------------------
void vtkPVDesktopDeliveryServer::PostRenderProcessing()
{
  vtkDebugMacro("PostRenderProcessing");

  vtkPVDesktopDeliveryServer::ImageParams ip;
  ip.RemoteDisplay = this->RemoteDisplay;

  if (ip.RemoteDisplay)
    {
    this->ReadReducedImage();
    ip.NumberOfComponents = this->ReducedImage->GetNumberOfComponents();
    if (   (this->ClientWindowSize[0] == this->ClientGUISize[0])
        && (this->ClientWindowSize[1] == this->ClientGUISize[1]) )
      {
      ip.ImageSize[0] = this->ReducedImageSize[0];
      ip.ImageSize[1] = this->ReducedImageSize[1];
      this->SendImageBuffer->SetArray(this->ReducedImage->GetPointer(0),
                                      ip.ImageSize[0]*ip.ImageSize[1]
                                      *ip.NumberOfComponents, 1);
      this->SendImageBuffer->SetNumberOfComponents(ip.NumberOfComponents);
      this->SendImageBuffer->SetNumberOfTuples(ip.ImageSize[0]*ip.ImageSize[1]);
      }
    else
      {
      // Grab a subset of the server's window that corresponds to the window on
      // the client.
      if (   (this->ClientGUISize[0] == this->FullImageSize[0])
          && !this->ImageResized )
        {
        // Window was not resized.  Return image size requested.
        ip.ImageSize[0] = this->ClientRequestedImageSize[0];
        ip.ImageSize[1] = this->ClientRequestedImageSize[1];
        }
      else
        {
        ip.ImageSize[0]
          = (  (this->ReducedImageSize[0]*this->ClientWindowSize[0])
             / this->ClientGUISize[0] );
        ip.ImageSize[1]
          = (  (this->ReducedImageSize[1]*this->ClientWindowSize[1])
             / this->ClientGUISize[1] );
        }
      int left   = (  (this->ReducedImageSize[0]*this->ClientWindowPosition[0])
                    / this->ClientGUISize[0] );
      int bottom = (  (this->ReducedImageSize[1]*this->ClientWindowPosition[1])
                    / this->ClientGUISize[1] );

      this->SendImageBuffer->Initialize();
      this->SendImageBuffer->SetNumberOfComponents(ip.NumberOfComponents);
      this->SendImageBuffer->SetNumberOfTuples(ip.ImageSize[0]*ip.ImageSize[1]);
      for (int i = 0; i < ip.ImageSize[1]; i++)
        {
        int destPos = i*ip.ImageSize[0]*ip.NumberOfComponents;
        int srcPos
          = (left + (bottom+i)*this->ReducedImageSize[0])*ip.NumberOfComponents;
        memcpy(this->SendImageBuffer->GetPointer(destPos),
               this->ReducedImage->GetPointer(srcPos),
               ip.ImageSize[0]*ip.NumberOfComponents);
        }
      }

    ip.SquirtCompressed = this->Squirt && (ip.NumberOfComponents == 4);

    if (ip.SquirtCompressed)
      {
      this->SquirtCompress(this->SendImageBuffer, this->SquirtBuffer);
      ip.NumberOfComponents = 4;
      ip.BufferSize
        = ip.NumberOfComponents*this->SquirtBuffer->GetNumberOfTuples();
      this->Controller->Send(reinterpret_cast<int *>(&ip),
                             vtkPVDesktopDeliveryServer::IMAGE_PARAMS_SIZE,
                             this->RootProcessId,
                             vtkPVDesktopDeliveryServer::IMAGE_PARAMS_TAG);
      this->Controller->Send(this->SquirtBuffer->GetPointer(0), ip.BufferSize,
                             this->RootProcessId,
                             vtkPVDesktopDeliveryServer::IMAGE_TAG);
      }
    else
      {
      ip.BufferSize
        = ip.NumberOfComponents*this->SendImageBuffer->GetNumberOfTuples();
      this->Controller->Send(reinterpret_cast<int *>(&ip),
                             vtkPVDesktopDeliveryServer::IMAGE_PARAMS_SIZE,
                             this->RootProcessId,
                             vtkPVDesktopDeliveryServer::IMAGE_PARAMS_TAG);
      this->Controller->Send(this->SendImageBuffer->GetPointer(0), ip.BufferSize,
                             this->RootProcessId,
                             vtkPVDesktopDeliveryServer::IMAGE_TAG);
      }
    }
  else
    {
    this->Controller->Send(reinterpret_cast<int *>(&ip),
                           vtkPVDesktopDeliveryServer::IMAGE_PARAMS_SIZE,
                           this->RootProcessId,
                           vtkPVDesktopDeliveryServer::IMAGE_PARAMS_TAG);
    }

  // Send timing metrics
  vtkPVDesktopDeliveryServer::TimingMetrics tm;
  if (this->ParallelRenderManager)
    {
    tm.ImageProcessingTime
      = this->ParallelRenderManager->GetImageProcessingTime();
    }
  else
    {
    tm.ImageProcessingTime = 0.0;
    }

  this->Controller->Send(reinterpret_cast<double *>(&tm),
                         vtkPVDesktopDeliveryServer::TIMING_METRICS_SIZE,
                         this->RootProcessId,
                         vtkPVDesktopDeliveryServer::TIMING_METRICS_TAG);

  // If another parallel render manager has already made an image, don't
  // clober it.
  if (this->ParallelRenderManager)
    {
    this->RenderWindowImageUpToDate = 1;
    }
}

//-----------------------------------------------------------------------------
void vtkPVDesktopDeliveryServer::AddRenderer(int id, vtkRenderer *ren)
{
  if (this->RendererMap->Renderers[id].GetPointer() == NULL)
    {
    this->RendererMap->Renderers[id]
      = vtkSmartPointer<vtkRendererCollection>::New();
    }
  this->RendererMap->Renderers[id]->AddItem(ren);
}

//-----------------------------------------------------------------------------
void vtkPVDesktopDeliveryServer::RemoveRenderer(int id, vtkRenderer *ren)
{
  RendererMapType::iterator iter = this->RendererMap->Renderers.find(id);
  if (iter != this->RendererMap->Renderers.end())
    {
    iter->second->RemoveItem(ren);
    if (iter->second->GetNumberOfItems() < 1)
      {
      this->RendererMap->Renderers.erase(iter);
      }
    }
}

//-----------------------------------------------------------------------------
void vtkPVDesktopDeliveryServer::RemoveAllRenderers(int id)
{
  this->RendererMap->Renderers.erase(id);
}

//----------------------------------------------------------------------------
void vtkPVDesktopDeliveryServer::SetRenderWindowSize()
{
  if (this->RemoteDisplay)
    {
    this->Superclass::SetRenderWindowSize();
    }
  else
    {
    int *size = this->RenderWindow->GetSize();
    this->FullImageSize[0] = size[0];
    this->FullImageSize[1] = size[1];
    this->ReducedImageSize[0] = (int)(size[0]/this->ImageReductionFactor);
    this->ReducedImageSize[1] = (int)(size[1]/this->ImageReductionFactor);
    }
}

//----------------------------------------------------------------------------
void vtkPVDesktopDeliveryServer::ReadReducedImage()
{
  if (this->ParallelRenderManager)
    {
    int *size = this->ParallelRenderManager->GetReducedImageSize();
    if ( this->ReducedImageSize[0] != size[0]
      || this->ReducedImageSize[1] != size[1] )
      {
      vtkDebugMacro(<< "Coupled parallel render manager reports unexpected reduced image size\n"
                    << "Expected size: " << this->ReducedImageSize[0] << " "
                    << this->ReducedImageSize[1] << "\n"
                    << "Reported size: " << size[0] << " " << size[1]);
      if ( this->ReducedImageSize[0] == this->FullImageSize[0]
        && this->ReducedImageSize[1] == this->FullImageSize[1] )
        {
        vtkWarningMacro(<< "The coupled render manager has apparently resized the window.\n"
                        << "Operation will still work normally, but the client may waste many cycles\n"
                        << "resizing the resulting window.");
        }
      this->ReducedImageSize[0] = size[0];
      this->ReducedImageSize[1] = size[1];
      }
    this->ParallelRenderManager->GetReducedPixelData(this->ReducedImage);
    this->ReducedImageUpToDate = 1;
    }
  else
    {
    this->Superclass::ReadReducedImage();
    }
}

//----------------------------------------------------------------------------
void vtkPVDesktopDeliveryServer::LocalComputeVisiblePropBounds(vtkRenderer *ren,
                                                             double bounds[6])
{
  if (this->ParallelRenderManager)
    {
    this->ParallelRenderManager->ComputeVisiblePropBounds(ren, bounds);
    }
  else
    {
    this->Superclass::LocalComputeVisiblePropBounds(ren, bounds);
    }
}

//----------------------------------------------------------------------------
void vtkPVDesktopDeliveryServer::SquirtCompress(vtkUnsignedCharArray *in,
                                              vtkUnsignedCharArray *out)
{
  vtkSquirtCompressor *compressor = vtkSquirtCompressor::New();
  compressor->SetInput(in);
  compressor->SetSquirtLevel(this->SquirtCompressionLevel);
  compressor->SetOutput(out);
  compressor->Compress();
  compressor->Delete();
}

//----------------------------------------------------------------------------
void vtkPVDesktopDeliveryServer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ParallelRenderManager: "
     << this->ParallelRenderManager << endl;
  os << indent << "RemoteDisplay: "
     << (this->RemoteDisplay ? "on" : "off") << endl;
}


//----------------------------------------------------------------------------
static void SatelliteStartRender(vtkObject *caller,
                                 unsigned long vtkNotUsed(event),
                                 void *clientData, void *)
{
  vtkPVDesktopDeliveryServer *self
    = reinterpret_cast<vtkPVDesktopDeliveryServer *>(clientData);
  if (caller != self->GetRenderWindow())
    {
    vtkGenericWarningMacro("vtkPVDesktopDeliveryServer caller mismatch");
    return;
    }
  self->SatelliteStartRender();
}

//----------------------------------------------------------------------------
static void SatelliteEndRender(vtkObject *caller,
                               unsigned long vtkNotUsed(event),
                               void *clientData, void *)
{
  vtkPVDesktopDeliveryServer *self
    = reinterpret_cast<vtkPVDesktopDeliveryServer *>(clientData);
  if (caller != self->GetRenderWindow())
    {
    vtkGenericWarningMacro("vtkPVDesktopDeliveryServer caller mismatch");
    return;
    }
  self->SatelliteEndRender();
}

//----------------------------------------------------------------------------
static void SatelliteStartParallelRender(vtkObject *caller,
                                         unsigned long vtkNotUsed(event),
                                         void *clientData, void *)
{
  vtkPVDesktopDeliveryServer *self
    = reinterpret_cast<vtkPVDesktopDeliveryServer *>(clientData);
  if (caller != self->GetParallelRenderManager())
    {
    vtkGenericWarningMacro("vtkPVDesktopDeliveryServer caller mismatch");
    return;
    }
  self->SatelliteStartRender();
}

//----------------------------------------------------------------------------
static void SatelliteEndParallelRender(vtkObject *caller,
                                       unsigned long vtkNotUsed(event),
                                       void *clientData, void *)
{
  vtkPVDesktopDeliveryServer *self
    = reinterpret_cast<vtkPVDesktopDeliveryServer *>(clientData);
  if (caller != self->GetParallelRenderManager())
    {
    vtkGenericWarningMacro("vtkPVDesktopDeliveryServer caller mismatch");
    return;
    }
  self->SatelliteEndRender();
}

//-----------------------------------------------------------------------------
static void UseRendererSet(void *localArg, void *remoteArg, int, int)
{
  vtkPVDesktopDeliveryServer *self
    = reinterpret_cast<vtkPVDesktopDeliveryServer *>(localArg);
  self->UseRendererSet(*reinterpret_cast<int*>(remoteArg));
}
