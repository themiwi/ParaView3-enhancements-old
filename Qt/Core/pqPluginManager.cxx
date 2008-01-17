/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqPluginManager.cxx,v $

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

#include "pqPluginManager.h"

#include <QLibrary>
#include <QPluginLoader>
#include <QFileInfo>
#include <QMessageBox>
#include <QCoreApplication>
#include <QDir>

#include "vtkSMXMLParser.h"
#include "vtkProcessModule.h"
#include "vtkSMObject.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxy.h"
#include "vtkPVEnvironmentInformation.h"
#include "vtkPVEnvironmentInformationHelper.h"
#include "vtksys/SystemTools.hxx"

#include "pqPlugin.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqApplicationCore.h"
#include "pqSMAdaptor.h"
#include "pqFileDialogModel.h"

pqPluginManager::pqPluginManager(QObject* p)
  : QObject(p)
{
  pqServerManagerModel* sm =
    pqApplicationCore::instance()->getServerManagerModel();
  QObject::connect(sm, SIGNAL(serverAdded(pqServer*)),
                   this, SLOT(onServerConnected(pqServer*)));
  QObject::connect(sm, SIGNAL(serverRemoved(pqServer*)),
                   this, SLOT(onServerDisconnected(pqServer*)));
}

pqPluginManager::~pqPluginManager()
{
}

QObjectList pqPluginManager::interfaces()
{
  return this->Interfaces + this->ExtraInterfaces;
}

pqPluginManager::LoadStatus pqPluginManager::loadServerPlugin(pqServer* server, const QString& lib,
                                       QString& error)
{
  pqPluginManager::LoadStatus success = NOTLOADED;

  vtkSMProxyManager* pxm = vtkSMObject::GetProxyManager();
  vtkSMProxy* pxy = pxm->NewProxy("misc", "PluginLoader");
  if(pxy && !lib.isEmpty())
    {
    vtkSMProperty* prop = pxy->GetProperty("FileName");
    pqSMAdaptor::setElementProperty(prop, lib);
    pxy->SetConnectionID(server->GetConnectionID());
    pxy->UpdateVTKObjects();
    pxy->UpdatePropertyInformation();
    prop = pxy->GetProperty("Loaded");
    success = pqSMAdaptor::getElementProperty(prop).toInt() ? LOADED : NOTLOADED;
    if(success == LOADED)
      {
      prop = pxy->GetProperty("ServerManagerXML");
      QList<QVariant> xmls = pqSMAdaptor::getMultipleElementProperty(prop);
      foreach(QVariant xml, xmls)
        {
        vtkSMXMLParser* parser = vtkSMXMLParser::New();
        parser->Parse(xml.toString().toAscii().data());
        parser->ProcessConfiguration(vtkSMObject::GetProxyManager());
        parser->Delete();
        }
      }
    else
      {
      error =
        pqSMAdaptor::getElementProperty(pxy->GetProperty("Error")).toString();
      }
    pxy->UnRegister(NULL);
    }
  
  if(success == LOADED)
    {
    this->Plugins.insert(server, lib);
    emit this->serverManagerExtensionLoaded();
    }

  return success;
}

pqPluginManager::LoadStatus pqPluginManager::loadClientPlugin(const QString& lib, QString& error)
{
  pqPluginManager::LoadStatus success = NOTLOADED;
  QPluginLoader qplugin(lib);
  if(qplugin.load())
    {
    QObject* pqpluginObject = qplugin.instance();
    pqPlugin* pqplugin = qobject_cast<pqPlugin*>(pqpluginObject);
    if(pqplugin)
      {
      pqpluginObject->setParent(this);  // take ownership to clean up later
      success = LOADED;
      this->Plugins.insert(NULL, lib);
      emit this->guiPluginLoaded();
      QObjectList ifaces = pqplugin->interfaces();
      foreach(QObject* iface, ifaces)
        {
        this->Interfaces.append(iface);
        emit this->guiInterfaceLoaded(iface);
        }
      }
    else
      {
      error = "This is not a ParaView Client Plugin.";
      qplugin.unload();
      }
    }
  else
    {
    error = qplugin.errorString();
    }
  return success;
}

static QStringList getLibraries(const QString& path, pqServer* server)
{
  QStringList libs;
  pqFileDialogModel model(server);
  model.setCurrentPath(path);
  int numfiles = model.rowCount(QModelIndex());
  for(int i=0; i<numfiles; i++)
    {
    QModelIndex idx = model.index(i, 0, QModelIndex());
    QString file = model.getFilePaths(idx)[0];
    QFileInfo fileinfo(file);
    // if file names start with known lib suffixes
    if(fileinfo.completeSuffix().indexOf(QRegExp("(so|dll|sl|dylib)$")) == 0)
      {
      libs.append(file);
      }
    }
  return libs;
}

void pqPluginManager::loadPlugins(pqServer* server)
{
  QStringList plugin_paths = this->pluginPaths(server);

  foreach(QString path, plugin_paths)
    {
    this->loadPlugins(path, server);
    }
}

//-----------------------------------------------------------------------------
void pqPluginManager::loadPlugins(const QString& path, pqServer* server)
{
  QStringList libs = ::getLibraries(path, server);
  foreach(QString lib, libs)
    {
    QString dummy;
    if(server)
      {
      pqPluginManager::loadServerPlugin(server, lib, dummy);
      }
    else
      {
      pqPluginManager::loadClientPlugin(lib, dummy);
      }
    }
}

//-----------------------------------------------------------------------------
pqPluginManager::LoadStatus pqPluginManager::loadPlugin(
  pqServer* server, const QString& lib, QString* errorReturn)
{
  LoadStatus success = NOTLOADED;
  QString error;
  
  if(this->Plugins.values(server).contains(lib))
    {
    return ALREADYLOADED;
    }
  
  if(server)
    {
    // look for SM stuff in the plugin
    success = pqPluginManager::loadServerPlugin(server, lib, error);
    }

  if(!server)
    {
    // check if this plugin has gui stuff in it
    success = loadClientPlugin(lib, error);
    }
  
  if(!success)
    {
    if(!errorReturn)
      {
      QMessageBox::information(NULL, "Plugin Load Failed", error);
      }
    else
      {
      *errorReturn = error;
      }
    }

  return success;
}

QStringList pqPluginManager::loadedPlugins(pqServer* server)
{
  return this->Plugins.values(server);
}

void pqPluginManager::addInterface(QObject* iface)
{
  if(!this->ExtraInterfaces.contains(iface))
    {
    this->ExtraInterfaces.append(iface);
    }
}

void pqPluginManager::removeInterface(QObject* iface)
{
  int idx = this->ExtraInterfaces.indexOf(iface);
  if(idx != -1)
    {
    this->ExtraInterfaces.removeAt(idx);
    }
}

void pqPluginManager::onServerConnected(pqServer* server)
{
  this->loadPlugins(server);
}

void pqPluginManager::onServerDisconnected(pqServer* server)
{
  // remove referenced plugins
  this->Plugins.remove(server);
}

QStringList pqPluginManager::pluginPaths(pqServer* server)
{
  QString pv_plugin_path;

  if(!server)
    {
    pv_plugin_path = vtksys::SystemTools::GetEnv("PV_PLUGIN_PATH");
    if(!pv_plugin_path.isEmpty())
      {
      pv_plugin_path += ";";
      }
    pv_plugin_path += QCoreApplication::applicationDirPath() + QDir::separator()
      + "plugins";
    }
  else
    {
    vtkSMProxyManager* pxm = vtkSMObject::GetProxyManager();
    vtkSMProxy* pxy = pxm->NewProxy("misc", "PluginLoader");
    pxy->SetConnectionID(server->GetConnectionID());
    pxy->UpdateVTKObjects();
    pxy->UpdatePropertyInformation();
    pv_plugin_path =
      pqSMAdaptor::getElementProperty(pxy->GetProperty("SearchPaths")).toString();
    pxy->UnRegister(NULL);
    }

    // add $APPDATA/<organization>/<appname>/Plugins  or
    // $HOME/.config/<organization>/<appname>/Plugins

  QString settingsRoot;
  if(server)
    {
    vtkSMProxyManager* pxm = vtkSMObject::GetProxyManager();
    vtkSMProxy* helper = pxm->NewProxy("misc", "EnvironmentInformationHelper");
    helper->SetConnectionID(server->GetConnectionID());
    helper->SetServers(vtkProcessModule::DATA_SERVER_ROOT);
#if defined(Q_OS_WIN)
    pqSMAdaptor::setElementProperty(helper->GetProperty("Variable"), "APPDATA");
#else
    pqSMAdaptor::setElementProperty(helper->GetProperty("Variable"), "HOME");
#endif
    helper->UpdateVTKObjects();
    vtkPVEnvironmentInformation* info = vtkPVEnvironmentInformation::New();
    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    pm->GatherInformation(helper->GetConnectionID(), 
      vtkProcessModule::DATA_SERVER, info, helper->GetID());
    settingsRoot = info->GetVariable();
    info->Delete();
    helper->UnRegister(NULL);
    }
  else
    {
#if defined(Q_OS_WIN)
    settingsRoot = QString::fromLocal8Bit(getenv("APPDATA"));
#else
    settingsRoot = QString::fromLocal8Bit(getenv("HOME"));
#endif
    }

#if !defined(Q_OS_WIN)
  if(!settingsRoot.isEmpty())
    {
    settingsRoot += "/.config";
    }
#endif

  if(!settingsRoot.isEmpty())
    {
    QString homePluginPath = QString("%1/%2/%3/Plugins");
    homePluginPath = homePluginPath.arg(settingsRoot);
    homePluginPath = homePluginPath.arg(QCoreApplication::organizationName());
    homePluginPath = homePluginPath.arg(QCoreApplication::applicationName());
    if(!pv_plugin_path.isEmpty())
      {
      pv_plugin_path += ";";
      }
    pv_plugin_path += homePluginPath;
    }

  // trim any whitespace before or after the path delimiters
  // note, shouldn't be a problem with drive letters on Windows "c:\"
  pv_plugin_path = pv_plugin_path.trimmed();
  pv_plugin_path = pv_plugin_path.replace(QRegExp("(\\;|\\:)\\s+"), ";");
  pv_plugin_path = pv_plugin_path.replace(QRegExp("\\s+(\\;|\\:)"), ";");
  
  // pre-parse the string replacing ':' with ';', watching out for windows drive letters
  // assumes ';' is not used as part of a directory name
  for(int index=0; index < pv_plugin_path.size(); index++)
    {
    QChar c = pv_plugin_path.at(index);
    if(c == ':')
      {
      bool prevIsChar = index > 0 && pv_plugin_path.at(index-1).isLetter();
      bool prevPrevIsDelim = index == 1 || (index > 1 && pv_plugin_path.at(index-2) == ';');
      if(!(prevIsChar && prevPrevIsDelim))
        {
        pv_plugin_path.replace(index, 1, ';');
        }
      }
    }

  QStringList plugin_paths = pv_plugin_path.split(';', QString::SkipEmptyParts);
  return plugin_paths;
}

