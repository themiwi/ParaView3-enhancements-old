/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqServerResources.cxx,v $

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

#include "pqApplicationCore.h"
#include "pqServerResources.h"
#include "pqServer.h"
#include "pqServerStartups.h"
#include "pqSettings.h"

#include <QStringList>
#include <QtDebug>

#include <vtkPVXMLParser.h>

#include <vtkstd/algorithm>
#include <vtkstd/vector>

class pqServerResources::pqImplementation
{
public:
  typedef vtkstd::vector<pqServerResource> ResourcesT;
  ResourcesT Resources;
};

class pqServerResources::pqMatchHostPath
{
public:
  pqMatchHostPath(const pqServerResource& resource) :
    Resource(resource)
  {
  }

  bool operator()(const pqServerResource& rhs) const
  {
    return this->Resource.hostPath() == rhs.hostPath();
  }
  
private:
  const pqServerResource& Resource;
};

pqServerResources::pqServerResources() :
  Implementation(new pqImplementation())
{
}

pqServerResources::~pqServerResources()
{
  delete this->Implementation;
}

void pqServerResources::add(const pqServerResource& resource)
{
  // Remove any existing resources that match the resource we're about to add ...
  // Note: we consider a resource a "match" if it has the same host(s) and path;
  // we ignore scheme and port(s)
  this->Implementation->Resources.erase(
    vtkstd::remove_if(
      this->Implementation->Resources.begin(),
      this->Implementation->Resources.end(),
      pqMatchHostPath(resource)),
    this->Implementation->Resources.end());
    
  this->Implementation->Resources.insert(this->Implementation->Resources.begin(), resource);
  
  const unsigned long max_length = 10;
  if(this->Implementation->Resources.size() > max_length)
    {
    this->Implementation->Resources.resize(max_length);
    }
  
  emit this->changed();
}

const pqServerResources::ListT pqServerResources::list() const
{
  ListT results;

  vtkstd::copy(
    this->Implementation->Resources.begin(),
    this->Implementation->Resources.end(),
    vtkstd::back_inserter(results));
    
  return results;
}

void pqServerResources::load(pqSettings& settings)
{
  const QStringList resources = settings.value("ServerResources").toStringList();
  for(int i = resources.size() - 1; i >= 0; --i)
    {
    this->add(pqServerResource(resources[i]));
    }
}

void pqServerResources::save(pqSettings& settings)
{
  QStringList resources;
  for(
      pqImplementation::ResourcesT::const_iterator resource = this->Implementation->Resources.begin();
      resource != this->Implementation->Resources.end();
      ++resource)
    {
    resources.push_back(resource->toString());
    }
  settings.setValue("ServerResources", resources);
}

void pqServerResources::open(const pqServerResource& resource)
{
  if(resource.scheme() == "session")
    {
    pqServer* const server = pqApplicationCore::instance()->createServer(
      resource.sessionServer());
    if(!server)
      {
      qCritical() << "Error creating server " << resource.sessionServer().toString() << "\n";
      return;
      }
    
    if(resource.path().isEmpty())
      {
      return;
      }

    // Read in the xml file to restore.
    vtkSmartPointer<vtkPVXMLParser> xmlParser = vtkSmartPointer<vtkPVXMLParser>::New();
    xmlParser->SetFileName(resource.path().toAscii().data());
    xmlParser->Parse();

    // Get the root element from the parser.
    if(vtkPVXMLElement* const root = xmlParser->GetRootElement())
      {
      pqApplicationCore::instance()->loadState(root, server, 0/*this->getActiveRenderModule()*/);
      }
    else
      {
      qCritical("Root does not exist. Either state file could not be opened "
                "or it does not contain valid xml");
      return;
      }
    }
  else
    {
    pqServer* const server = pqApplicationCore::instance()->createServer(resource);
    if(!server)
      {
      qCritical() << "Error creating server " << resource.toString() << "\n";
      return;
      }
    
    if(resource.path().isEmpty())
      {
      return;
      }
    
    if(!pqApplicationCore::instance()->createReaderOnServer(resource.path(), server))
      {
      qCritical() << "Error opening file " << resource.path() << "\n";
      return;
      }
    }
}
