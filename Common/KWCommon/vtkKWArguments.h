/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkKWArguments.h,v $
  Language:  C++
  Date:      $Date: 2003-04-09 12:18:03 $
  Version:   $Revision: 1.2 $

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
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
// .NAME vtkKWArguments - simple wrapper for icons
// .SECTION Description
// A simple icon wrapper. It can either be used with file icons.h to 
// provide a unified interface for internal icons or a wrapper for 
// custom icons. The icons are defined with width, height, pixel_size, and array
// of unsigned char values.

#ifndef __vtkKWArguments_h
#define __vtkKWArguments_h

#include "vtkObject.h"

class vtkKWArgumentsInternal;

class VTK_EXPORT vtkKWArguments : public vtkObject
{
public:
  static vtkKWArguments* New();
  vtkTypeRevisionMacro(vtkKWArguments,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  //BTX
  // These are prototypes for callbacks.
  typedef int(*CallbackType)(const char* argument, const char* value, 
    void* call_data);
  typedef int(*ErrorCallbackType)(const char* argument, void* client_data);

  struct CallbackStructure
    {
    const char* Argument;
    int ArgumentType;
    CallbackType Callback;
    void* CallData;
    const char* Help;
    };
  
  // Description:
  // These are different argument types.
  enum { 
    NO_ARGUMENT,    // The option takes no argument             --foo
    CONCAT_ARGUMENT,// The option takes argument after no space --foobar
    SPACE_ARGUMENT, // The option takes argument after space    --foo bar
    EQUAL_ARGUMENT  // The option takes argument after equal    --foo=bar
  };
  //ETX
  
  // Description:
  // Initialize internal data structures. This should be called before parsing.
  void Initialize(int argc, char* argv[]);

  // Description:
  // This method will parse arguments and call apropriate methods. 
  int Parse();

  // Description:
  // This method will add a callback for a specific argument. The arguments to
  // it are argument, argument type, callback method, and call data. The
  // argument help specifies the help string used with this option.
  void AddCallback(const char* argument, int type, CallbackType callback, 
                   void* call_data, const char* help);

  // Description:
  // This method registers callbacks for argument types from array of
  // structures. It stops when an entry has all zeros. 
  void AddCallbacks(CallbackStructure* callbacks);

  // Description:
  // Set the callbacks for error handling.
  void SetClientData(void* client_data);
  void SetUnknownArgumentCallback(ErrorCallbackType callback);

  // Description:
  // Get remaining arguments. It allocates space for argv, so you have to call
  // delete[] on it.
  void GetRemainingArguments(int* argc, char*** argv);

  // Description:
  // Return string containing help.
  vtkGetStringMacro(Help);

protected:
  vtkKWArguments();
  ~vtkKWArguments();

  vtkSetStringMacro(Help);
  void GenerateHelp();

  vtkKWArgumentsInternal* Internals;
  char* Help;

private:
  vtkKWArguments(const vtkKWArguments&); // Not implemented
  void operator=(const vtkKWArguments&); // Not implemented
};


#endif


