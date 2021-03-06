/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMImageDataParallelStrategy.h,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMImageDataParallelStrategy
// .SECTION Description
// This is parallel strategy used for vtkImageData. This strategy (unlike
// vtkSMUniformGridParallelStrategy), has logic to move the input image data
// over the client or the render server. Hence, this should never be used for
// representations that show the whole image data (such as volume
// representations). It is designed for applications such as slice
// representation, which simply delivers a single slab from the image data.

#ifndef __vtkSMImageDataParallelStrategy_h
#define __vtkSMImageDataParallelStrategy_h

#include "vtkSMSimpleParallelStrategy.h"

class VTK_EXPORT vtkSMImageDataParallelStrategy : 
  public vtkSMSimpleParallelStrategy
{
public:
  static vtkSMImageDataParallelStrategy* New();
  vtkTypeRevisionMacro(vtkSMImageDataParallelStrategy, 
    vtkSMSimpleParallelStrategy);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkSMImageDataParallelStrategy();
  ~vtkSMImageDataParallelStrategy();

  // Description:
  // Create and initialize the data pipeline.
  virtual void CreatePipeline(vtkSMSourceProxy* input, int outputport);

  // Description:
  // Create and initialize the LOD data pipeline.
  // Note that this method is called irrespective of EnableLOD
  // flag.
  virtual void CreateLODPipeline(vtkSMSourceProxy* input, int outputport);

private:
  vtkSMImageDataParallelStrategy(const vtkSMImageDataParallelStrategy&); // Not implemented
  void operator=(const vtkSMImageDataParallelStrategy&); // Not implemented
//ETX
};

#endif

