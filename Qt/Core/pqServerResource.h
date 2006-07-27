/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqServerResource.h,v $

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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

#ifndef _pqServerResource_h
#define _pqServerResource_h

#include "pqCoreExport.h"

#include <QString>

/**
Encapsulates a resource (file) located on a specific server.

Resources are represented externally using a URI-like syntax that can specify
all of the types of connection supported by ParaView:

builtin:/path/to/resource
cs://host:port/path/to/resource
csrc://host:port/path/to/resource
cdsrs://dshost:dsport//rshost:rsport/path/to/resource
cdsrsrc://dshost:dsport//rshost:rsport/path/to/resource

Resource paths may also use Win32 syntax with reverse slashes:

builtin:/c:\path\to\resource
cs://host:port/c:\path\to\resource
csrc://host:port/c:\path\to\resource
cdsrs://dshost:dsport//rshost:rsport/c:\path\to\resource
cdsrsrc://dshost:dsport//rshost:rsport/c:\path\to\resource

(Note that all paths begin with a forward-slash, regardless of platform)

All port numbers are optional, e.g:

cs://host/path/to/resource
csrc://host/path/to/resource
cdsrs://dshost//rshost/path/to/resource
cdsrsrc://dshost//rshost/path/to/resource

... in this case, default port numbers will be used.

All paths are optional, e.g:

builtin:/path/to/resource
cs://host:port
csrc://host:port
cdsrs://dshost:dsport//rshost:rsport
cdsrsrc://dshost:dsport//rshost:rsport

... in this case, the resource represents a connection to a specific server
without opening a file.

\sa pqServerResources, pqServer
*/
class PQCORE_EXPORT pqServerResource
{
public:
  pqServerResource();
  pqServerResource(const QString&);
  pqServerResource(const pqServerResource&);
  pqServerResource& operator=(const pqServerResource&);
  ~pqServerResource();

  const QString toString() const;
  
  const QString scheme() const;
  void setScheme(const QString&);
  
  const QString host() const;
  void setHost(const QString&);
  
  const int port() const;
  const int port(int default_port) const;
  void setPort(int);

  const QString dataServerHost() const;
  void setDataServerHost(const QString&);
  
  const int dataServerPort() const;
  const int dataServerPort(int default_port) const;
  void setDataServerPort(int);
  
  const QString renderServerHost() const;
  void setRenderServerHost(const QString&);
    
  const int renderServerPort() const;
  const int renderServerPort(int default_port) const;
  void setRenderServerPort(int);
    
  const QString path() const;
  void setPath(const QString&);

  const pqServerResource server() const;

  const bool operator==(const pqServerResource&) const;
  const bool operator!=(const pqServerResource&) const;
  const bool operator<(const pqServerResource&) const;

private:
  class pqImplementation;
  pqImplementation* const Implementation;
};

#endif
