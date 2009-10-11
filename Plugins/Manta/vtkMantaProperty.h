/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkMantaProperty.h,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*=========================================================================

  Program:   VTK/ParaView Los Alamos National Laboratory Modules (PVLANL)
  Module:    $RCSfile: vtkMantaProperty.h,v $

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
// .NAME vtkOpenProperty - Manta property
// .SECTION Description
// vtkMantaProperty is a concrete implementation of the abstract class
// vtkProperty. vtkMantaProperty interfaces to the Manta Raytracer library.

#ifndef __vtkMantaProperty_h
#define __vtkMantaProperty_h

#include "vtkMantaConfigure.h"
#include "vtkProperty.h"
#include "Interface/Texture.h"

//BTX
namespace Manta {
  class Material;
//TODO: what should we do to deal with template+namespace?
//class Texture<Color>;
}
//ETX

class vtkMantaRenderer;

class VTK_vtkManta_EXPORT vtkMantaProperty : public vtkProperty
{
public:
  static vtkMantaProperty *New();
  vtkTypeRevisionMacro(vtkMantaProperty,vtkProperty) ;
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Implement base class method.
  void Render(vtkActor *a, vtkRenderer *ren);

  // Description:
  // Implement base class method.
  void BackfaceRender(vtkActor *a, vtkRenderer *ren);

  // functions that change parameters of various materials
  vtkSetStringMacro(MaterialType);
  vtkGetStringMacro(MaterialType);
  vtkSetMacro(Reflectance, float);
  vtkGetMacro(Reflectance, float);
  vtkSetMacro(Eta, float);
  vtkGetMacro(Eta, float);
  vtkSetMacro(Thickness, float);
  vtkGetMacro(Thickness, float);
  vtkSetMacro(N, float);
  vtkGetMacro(N, float);
  vtkSetMacro(Nt, float);
  vtkGetMacro(Nt, float);
  //BTX
  vtkSetMacro(MantaMaterial, Manta::Material*);
  vtkGetMacro(MantaMaterial, Manta::Material*);
  //ETX

protected:
 vtkMantaProperty() :
  MantaMaterial(0), MaterialType(0), diffuseTexture(0), specularTexture(0),
    Reflectance(0.0), Eta(1.52), Thickness(1.0)
    {
    // No, there is no duplication of setting MaterialType, it is the way
    // vtkSetStringMacro works
    this->SetMaterialType("default");
    };
  ~vtkMantaProperty();

private:
  vtkMantaProperty(const vtkMantaProperty&);  // Not implemented.
  void operator=(const vtkMantaProperty&);  // Not implemented.

  void CreateMantaProperty();

  // the last time MantaMaterial is modified
  vtkTimeStamp MantaMaterialMTime;
  //BTX
  Manta::Material *MantaMaterial;
  Manta::Texture<Manta::Color> *diffuseTexture;
  Manta::Texture<Manta::Color> *specularTexture;
  //ETX

  // type of material to use. possible values are: "lambertian", "phong",
  // "transparent", "thindielectric", "dielectric", "ambientocclusion",
  // "metal", "orennayer"
  char * MaterialType;

  // amount of reflection to use. should be between 0.0 and 1.0
  float Reflectance;

  // the index of refraction for a material. used with the thin dielectric
  // material
  float Eta;

  // how thick the material is. used with the thin dielectric material
  float Thickness;

  // index of refraction for outside material. used in dielectric material
  float N;

  // index of refraction for inside material (transmissive). used in
  // dielectric material
  float Nt;
};

#endif
