/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkPVContourFilter.h,v $
  Language:  C++
  Date:      $Date: 2000-10-18 16:08:35 $
  Version:   $Revision: 1.13 $

Copyright (c) 1998-1999 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.

All rights reserved. No part of this software may be reproduced, distributed,
or modified, in any form or by any means, without permission in writing from
Kitware Inc.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

=========================================================================*/

#ifndef __vtkPVContourFilter_h
#define __vtkPVContourFilter_h

#include "vtkPVDataSetToPolyDataFilter.h"
class vtkSingleContourFilter;

class VTK_EXPORT vtkPVContourFilter : public vtkPVDataSetToPolyDataFilter
{
public:
  static vtkPVContourFilter* New();
  vtkTypeMacro(vtkPVContourFilter, vtkPVDataSetToPolyDataFilter);

  // Description:
  // You have to clone this object before you create its UI.
  void CreateProperties();

  // Description:
  // Returns the countour VTK filter for this object.
  // This should allow the filter to be changed from C++ easily.
  vtkSingleContourFilter *GetContour();

protected:
  vtkPVContourFilter();
  ~vtkPVContourFilter() {};
  vtkPVContourFilter(const vtkPVContourFilter&) {};
  void operator=(const vtkPVContourFilter&) {};
};

#endif
