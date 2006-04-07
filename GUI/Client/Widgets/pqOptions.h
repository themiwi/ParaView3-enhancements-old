/*=========================================================================

   Program:   ParaQ
   Module:    $RCSfile: pqOptions.h,v $

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.1. 

   See License_v1.1.txt for the full ParaQ license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#ifndef __pqOptions_h
#define __pqOptions_h

#include "pqWidgetsExport.h"
#include <vtkPVOptions.h>
/*! \brief Command line options for pqClient.
 *
 * pqOptions extends vtkPVOptions to handle pqClient specific command line 
 * options.
 */
class PQWIDGETS_EXPORT pqOptions : public vtkPVOptions
{
public:
  static pqOptions *New();
  vtkTypeRevisionMacro(pqOptions, vtkPVOptions);
  void PrintSelf(ostream &os, vtkIndent indent);

  vtkGetMacro(TestUINames, int);
  vtkGetStringMacro(TestFileName);
  vtkGetStringMacro(TestDirectory);
  vtkGetStringMacro(BaselineImage);
  vtkGetMacro(ImageThreshold, int);
  vtkGetMacro(ExitBeforeEventLoop, int);
protected:
  pqOptions();
  virtual ~pqOptions();

  virtual void Initialize();
  virtual int PostProcess(int argc, const char * const *argv);

  int TestUINames;
  char* TestFileName;
  char* TestDirectory;
  char* BaselineImage;
  int ImageThreshold;
  int ExitBeforeEventLoop;
    
  vtkSetStringMacro(TestFileName);
  vtkSetStringMacro(TestDirectory);
  vtkSetStringMacro(BaselineImage);
  
private:
  pqOptions(const pqOptions &);
  void operator=(const pqOptions &);
};

#endif //__pqOptions_h

