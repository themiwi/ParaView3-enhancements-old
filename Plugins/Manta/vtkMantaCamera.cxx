/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkMantaCamera.cxx,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*=========================================================================

  Program:   VTK/ParaView Los Alamos National Laboratory Modules (PVLANL)
  Module:    $RCSfile: vtkMantaCamera.cxx,v $

Copyright (c) 2007, Los Alamos National Security, LLC

All rights reserved.

Copyright 2007. Los Alamos National Security, LLC. 
This software was produced under U.S. Government contract DE-AC52-06NA25396 
for Los Alamos National Laboratory (LANL), which is operated by 
Los Alamos National Security, LLC for the U.S. Department of Energy. 
The U.S. Government has rights to use, reproduce, and distribute this software. 
NEITHER THE GOVERNMENT NOR LOS ALAMOS NATIONAL SECURITY, LLC MAKES ANY WARRANTY,
EXPRESS OR IMPLIED, OR ASSUMES ANY LIABILITY FOR THE USE OF THIS SOFTWARE.  
If software is modified to produce derivative works, such modified software 
should be clearly marked, so as not to confuse it with the version available 
from LANL.
 
Additionally, redistribution and use in source and binary forms, with or 
without modification, are permitted provided that the following conditions 
are met:
-   Redistributions of source code must retain the above copyright notice, 
    this list of conditions and the following disclaimer. 
-   Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution. 
-   Neither the name of Los Alamos National Security, LLC, Los Alamos National
    Laboratory, LANL, the U.S. Government, nor the names of its contributors
    may be used to endorse or promote products derived from this software 
    without specific prior written permission. 

THIS SOFTWARE IS PROVIDED BY LOS ALAMOS NATIONAL SECURITY, LLC AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
ARE DISCLAIMED. IN NO EVENT SHALL LOS ALAMOS NATIONAL SECURITY, LLC OR 
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; 
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR 
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkMantaCamera - 
// .SECTION Description
//

#include "vtkManta.h"
#include "vtkMantaCamera.h"
#include "vtkMantaRenderer.h"
#include "vtkMantaRenderWindow.h"

#include "vtkObjectFactory.h"

#include <Interface/Camera.h>
#include <Engine/Control/RTRT.h>
#include <math.h>

vtkCxxRevisionMacro(vtkMantaCamera, "$Revision: 1.1 $");
vtkStandardNewMacro(vtkMantaCamera);

//----------------------------------------------------------------------------
void vtkMantaCamera::CreateMantaCamera(vtkRenderer *ren)
{
  if (vtkMantaRenderer * mantaRenderer = vtkMantaRenderer::SafeDownCast(ren))
    {
    // the mantaCamera is a singleton created by vtkMantaRenderer, all
    // vtkCameras refer to the same mantaCamera created by vtkMantaRenderer
    // (is it possible to have multiple vtkCameras associated with one
    //  vtkRenderer?)
    if (!this->mantaCamera)
      {
      this->mantaCamera = mantaRenderer->GetMantaCamera();
      }
    
    // for figuring out aspect ratio
    int lowerLeft[2];
    int usize, vsize;
    ren->GetTiledSizeAndOrigin(&usize, &vsize, lowerLeft, lowerLeft + 1);

    double *eye, *lookat, *up, vfov;
    eye    = this->Position;
    lookat = this->FocalPoint;
    up     = this->ViewUp;
    vfov   = this->ViewAngle;

    const Manta::BasicCameraData bookmark(
        Manta::Vector(eye[0], eye[1], eye[2]),
        Manta::Vector(lookat[0], lookat[1], lookat[2]),
        Manta::Vector(up[0], up[1], up[2]),
        vfov * usize / vsize, vfov);

    mantaRenderer->GetMantaEngine()->addTransaction("update camera",
        Manta::Callback::create(this->mantaCamera,
            &Manta::Camera::setBasicCameraData, bookmark));
    }
}

//----------------------------------------------------------------------------
// called by Renderer::UpdateCamera()
void vtkMantaCamera::Render(vtkRenderer *ren)
{
  if (this->GetMTime() > this->lastRenderTime)
    {
    // vtkInteractorStyle changes the basic attributes of vtkCamera, all
    // we need to do is update the mantaCamera accordingly via transaction
    CreateMantaCamera(ren);
    }

  // update lastRenderTime
  this->lastRenderTime.Modified();
}

//----------------------------------------------------------------------------
void vtkMantaCamera::UpdateViewport(vtkRenderer *ren)
{
  // Deal with change of window size
  // Not actaully called by anyone
}
