/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkKWMath.h,v $
  Language:  C++
  Date:      $Date: 2003-09-06 01:07:28 $
  Version:   $Revision: 1.6 $

Copyright (c) 1998-2003 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkKWMath - performs more common math operations
// .SECTION Description
// vtkKWMath is provides methods to perform more common math operations.

#ifndef __vtkKWMath_h
#define __vtkKWMath_h

#include "vtkObject.h"

class vtkDataArray;

class VTK_EXPORT vtkKWMath : public vtkObject
{
public:
  static vtkKWMath *New();
  vtkTypeRevisionMacro(vtkKWMath,vtkObject);

  // Description:
  // Rounds a float to the nearest integer.
  //BTX
  static float Round(float f) {
    return (f >= 0 ? (float)floor((double)f + 0.5) : ceil((double)f - 0.5)); }
  static double Round(double f) {
    return (f >= 0 ? floor(f + 0.5) : ceil(f - 0.5)); }
  //ETX

  // Description:
  // Get the data's scalar range given a component
  // WARNING: the 'double' version does *not* cache the ranges, it 
  // iterates over the data each time (whereas the 'float' version is
  // just a proxy to vtkDataArray::GetRange)
  // Return 1 on success, 0 otherwise.
  static int GetScalarRange(vtkDataArray *array, int comp, float range[2]);
  static int GetScalarRange(vtkDataArray *array, int comp, double range[2]);
 
  // Description:
  // Get the data's scalar range given a component. This range is adjusted
  // for unsigned char and short to the whole data type range (or 4095 if
  // the real range is within that limit).
  // Return 1 on success, 0 otherwise.
  static int GetAdjustedScalarRange(
    vtkDataArray *array, int comp, float range[2]);
 
  // Description:
  // Get the scalar type that will be able to store a given range of data 
  // once it has been scaled and shifted. If any of those parameters is not
  // an integer number, the search will default to float types (float, double)
  // Return -1 on error or no scalar type found.
  static int GetScalarTypeFittingRange(
    double range_min, double range_max, 
    double scale = 1.0, double shift = 0.0);
 
protected:
  vtkKWMath() {};
  ~vtkKWMath() {};
  
private:
  vtkKWMath(const vtkKWMath&);  // Not implemented.
  void operator=(const vtkKWMath&);  // Not implemented.
};

#endif
