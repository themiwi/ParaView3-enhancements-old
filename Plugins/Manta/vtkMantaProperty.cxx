/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkMantaProperty.cxx,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*=========================================================================

  Program:   VTK/ParaView Los Alamos National Laboratory Modules (PVLANL)
  Module:    $RCSfile: vtkMantaProperty.cxx,v $

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
// .NAME vtkMantaProperty - 
// .SECTION Description
//
#include "vtkManta.h"
#include "vtkMantaRenderer.h"
#include "vtkMantaProperty.h"

#include "vtkObjectFactory.h"

#include <Model/Materials/Flat.h>
#include <Model/Materials/Lambertian.h>
#include <Model/Materials/Transparent.h>
#include <Model/Materials/Phong.h>
#include <Model/Materials/ThinDielectric.h>
#include <Model/Materials/AmbientOcclusion.h>
#include <Model/Materials/MetalMaterial.h>
#include <Model/Materials/OrenNayar.h>
#include <Model/Materials/Dielectric.h>

#include <Model/Textures/Constant.h>

#include <cstring>

vtkCxxRevisionMacro(vtkMantaProperty, "$Revision: 1.1 $");
vtkStandardNewMacro(vtkMantaProperty);

//----------------------------------------------------------------------------
void vtkMantaProperty::CreateMantaProperty()
{
  double * diffuse  = this->GetDiffuseColor();
  double * specular = this->GetSpecularColor();

  this->diffuseTexture  = new Manta::Constant<Manta::Color>
    (  Manta::Color( Manta::RGB( diffuse[0],  diffuse[1],  diffuse[2]  ) )  );

  this->specularTexture = new Manta::Constant<Manta::Color>
    (  Manta::Color( Manta::RGB( specular[0], specular[1], specular[2] ) )  );

  diffuse  = NULL;
  specular = NULL;

  // A note on Manta Materials and shading model:
  // 1. Surface normal is computed at each hit point, if the primitive
  // has curvature (like sphere or triangle with vertex normal), it will
  // be different at each hit point.
  // 2. The Flat material takes account only the surface normal and the
  // color texture of the material. The color texture acts like the emissive
  // color in OpenGL. It is multiplied by the dot product of surface normal
  // and ray direction to get the result color. Since the dot product is
  // different at each point on the primitive the result looks more like
  // Gouraud shading without specular highlight in OpenGL.
  // 3. To get "real" FLAT shading, we have to not to supply vertex normal when
  // we create the mesh.
  // TODO: replace this whole thing with a factory
  if ( strcmp( this->MaterialType, "default" ) == 0 )
    {
    // if lighting is disabled, use EmitMaterial
    if ( this->Interpolation == VTK_FLAT )
      {
      this->MantaMaterial = new Manta::Flat( this->diffuseTexture );
      }
    else
    if ( this->GetOpacity() < 1.0 )
      {
      this->MantaMaterial =
        new Manta::Transparent( this->diffuseTexture, this->GetOpacity() );
      }
    else
    if ( this->GetSpecular() == 0 )
      {
      this->MantaMaterial = new Manta::Lambertian( this->diffuseTexture );
      }
    else
      {
      this->MantaMaterial =
        new Manta::Phong( this->diffuseTexture,
                          this->specularTexture,
                          static_cast<int> ( this->GetSpecularPower() ),
                          NULL );
      }
    }
  else
    {
    // TODO: memory leak, textures created but never deleted
    if ( strcmp( this->MaterialType,  "lambertian" ) == 0 )
      {
      this->MantaMaterial = new Manta::Lambertian( this->diffuseTexture );
      }
    else
    if ( strcmp( this->MaterialType, "phong" ) == 0 )
      {
      this->MantaMaterial =
        new Manta::Phong( this->diffuseTexture,
                          this->specularTexture,
                          static_cast<int> ( this->GetSpecularPower() ),
                          new Manta::Constant<Manta::ColorComponent>
                          ( this->Reflectance ) );
      }
    else
    if ( strcmp( this->MaterialType, "transparent" ) == 0 )
      {
      this->MantaMaterial
        = new Manta::Transparent( diffuseTexture, this->GetOpacity() );
      }
    else
    if ( strcmp( this->MaterialType, "thindielectric" ) == 0 )
      {
      this->MantaMaterial = new Manta::ThinDielectric(
        new Manta::Constant<Manta::Real>( this->Eta ),
        diffuseTexture, this->Thickness, 1 );
      }
    else
    if ( strcmp( this->MaterialType, "dielectric" ) == 0 )
      {
      this->MantaMaterial
        = new Manta::Dielectric( new Manta::Constant<Manta::Real>( this->N  ),
                                 new Manta::Constant<Manta::Real>( this->Nt ),
                                 diffuseTexture );
      }
    else
    if ( strcmp( this->MaterialType, "ambientocclusion" ) == 0 )
      {
      this->MantaMaterial =
        new Manta::AmbientOcclusion( diffuseTexture, 20, 20 );
      }
    else
    if (strcmp( this->MaterialType, "metal" ) == 0 )
      {
      this->MantaMaterial = new Manta::MetalMaterial( diffuseTexture );
      }
    else
    if ( strcmp( this->MaterialType, "orennayer" ) == 0 )
      {
      this->MantaMaterial = new Manta::OrenNayar( diffuseTexture );
      }
    else
      {
      // just default to phong
      this->MantaMaterial
        = new Manta::Phong( this->diffuseTexture,
                            this->specularTexture,
                            static_cast<int> ( this->GetSpecularPower() ),
                            new Manta::Constant<Manta::ColorComponent>(
                            this->Reflectance) );
      }
    }

  // update MantaMaterialMTime
  this->MantaMaterialMTime.Modified();
}

//----------------------------------------------------------------------------
// called by vtkActor::RenderOpaqueGeometry() and
// vtkActor::RenderTranslucentPolygonalGeometry()
// In PV, this function will called twice per vtkRenderer::Render(), once
// in vtkPVLODActor::RenderOpaqueGeometry() and once in vtkPVLODActor::Render()
void vtkMantaProperty::Render( vtkActor * anActor, vtkRenderer * ren)
{
  // this test works for both a newly created vtkMantaProperty and
  // an updated one
  if ( this->GetMTime() > this->MantaMaterialMTime )
    {
    // if there is some change to the vtkProperty since last time
    // MantaMaterial is created, recreate mantaProperty accordingly.
    // the actor referencing this property will check if vtkProperty
    // is changed and update it's Manta primitive's material accordingly.

    // FIXME: thread safety, these deletes may be called with the engine
    // still running
//    delete this->MantaMaterial;    this->MantaMaterial   = NULL;
//    delete this->diffuseTexture;   this->diffuseTexture  = NULL;
//    delete this->specularTexture;  this->specularTexture = NULL;
    this->CreateMantaProperty();
    }
}

//----------------------------------------------------------------------------
// Implement base class method.
void vtkMantaProperty::BackfaceRender( vtkActor * vtkNotUsed( anActor ),
                                       vtkRenderer * vtkNotUsed( ren ) )
{
  // NOT supported by Manta
  cerr
    << "vtkMantaProperty::BackfaceRender(), backface rendering "
    << "is not supported by Manta"
    << endl;
}

//----------------------------------------------------------------------------
void vtkMantaProperty::PrintSelf( ostream & os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
}

//----------------------------------------------------------------------------
vtkMantaProperty::~vtkMantaProperty()
{
  // TODO: We don't have to test if these pointers are NULL if
  // we get other parts correct, this only hides memory problems
  if ( this->MantaMaterial   ) delete   this->MantaMaterial;
  if ( this->diffuseTexture  ) delete   this->diffuseTexture;
  if ( this->specularTexture ) delete   this->specularTexture;
  if ( this->MaterialType    ) delete[] this->MaterialType;
}
