/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkMantaLight.cxx,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*=========================================================================

  Program:   VTK/ParaView Los Alamos National Laboratory Modules (PVLANL)
  Module:    $RCSfile: vtkMantaLight.cxx,v $

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
// .NAME vtkMantaLight - 
// .SECTION Description
//
#include "vtkManta.h"
#include "vtkMantaLight.h"
#include "vtkMantaRenderer.h"

#include "vtkObjectFactory.h"

#include <Interface/Light.h>
#include <Interface/LightSet.h>
#include <Interface/Context.h>
#include <Engine/Control/RTRT.h>
#include <Model/Lights/PointLight.h>
#include <Model/Lights/DirectionalLight.h>

#include <math.h>

vtkCxxRevisionMacro(vtkMantaLight, "$Revision: 1.1 $");
vtkStandardNewMacro(vtkMantaLight);

void vtkMantaLight::UpdateMantaLight(vtkRenderer *ren)
{
  double *color, *position, *focal, direction[3];

  // Manta Lights only have one "color"
  color    = this->GetColor();
  position = this->GetTransformedPosition();
  focal    = this->GetTransformedFocalPoint();

  if (this->GetPositional())
    {
    Manta::PointLight * pointLight = dynamic_cast<Manta::PointLight *>(this->mantaLight);
    if ( pointLight )
        {
        pointLight->setPosition(Manta::Vector(position[0], position[1], position[2]));
        pointLight->setColor(Manta::Color(Manta::RGB(color[0],color[1],color[2])));
        }
    else
      {
      vtkWarningMacro(
        << "Changing from Directional to Positional light is not supported by vtkManta" );
      }
    }
  else
    {
    Manta::DirectionalLight * dirLight = dynamic_cast<Manta::DirectionalLight *>(this->mantaLight);
    if ( dirLight )
        {
        // "direction" in Manta means the direction toward light source rather than the
        // direction of rays originate from light source
        direction[0] = position[0] - focal[0];
        direction[1] = position[1] - focal[1];
        direction[2] = position[2] - focal[2];
        dirLight->setDirection(Manta::Vector(direction[0], direction[1], direction[2]));
        dirLight->setColor(Manta::Color(Manta::RGB(color[0],color[1],color[2])));
        }
    else
      {
      vtkWarningMacro(
              << "Changing from Positional to Directional light is not supported by vtkManta" );
      }
    }
}

//----------------------------------------------------------------------------
// called in Transaction context, it is safe to modify the engine state here
void vtkMantaLight::CreateMantaLight(vtkRenderer *ren)
{
  vtkMantaRenderer *mantaRenderer = vtkMantaRenderer::SafeDownCast(ren);

  double *color, *position, *focal, direction[3];

  // Manta Lights only have one "color"
  color    = this->GetColor();
  position = this->GetTransformedPosition();
  focal    = this->GetTransformedFocalPoint();

  if (this->GetPositional())
    {
    this->mantaLight = new Manta::PointLight(
      Manta::Vector(position[0], position[1], position[2]),
      Manta::Color(Manta::RGB(color[0],color[1],color[2])));
    }
  else
    {
    // "direction" in Manta means the direction toward light source rather than the
    // direction of rays originate from light source
    direction[0] = position[0] - focal[0];
    direction[1] = position[1] - focal[1];
    direction[2] = position[2] - focal[2];
    this->mantaLight = new Manta::DirectionalLight(
      Manta::Vector(direction[0], direction[1], direction[2]),
      Manta::Color(Manta::RGB(color[0],color[1],color[2])));
    }
  mantaRenderer->GetMantaLightSet()->add(mantaLight);
}

//----------------------------------------------------------------------------
void vtkMantaLight::Render(vtkRenderer *ren, int /* not used */)
{
  vtkMantaRenderer *mantaRenderer = vtkMantaRenderer::SafeDownCast(ren);

  if (this->mantaLight)
    {
    mantaRenderer->GetMantaEngine()->addTransaction(
        "update light",
        Manta::Callback::create(this,
            &vtkMantaLight::UpdateMantaLight,
            ren));
    }
  else
    {
    mantaRenderer->GetMantaEngine()->addTransaction(
        "create light",
        Manta::Callback::create(this,
            &vtkMantaLight::CreateMantaLight,
            ren));
    }
}

//----------------------------------------------------------------------------
vtkMantaLight::~vtkMantaLight()
{
  // TODO: do we have to remove mantaLight from MantaLightSet?
  // Ans: We should, but Manta Materials are holding pointers to
  // the mantaLight in their local lightset cache.
  delete this->mantaLight;
}

//----------------------------------------------------------------------------
void vtkMantaLight::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
