/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: main.cxx,v $

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2. 

   See License_v1.2.txt for the full ParaView license.
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

#include <QApplication>
#include <QDir>
#include "Config.h"
#include "pqComponentsInit.h"
#include "pqMain.h"
#include "ProcessModuleGUIHelper.h"
#include "vtkSmartPointer.h"

#ifdef Q_WS_X11
#include <QPlastiqueStyle>
#endif

int main(int argc, char* argv[])
{
  QApplication app(argc, argv);

#ifdef Q_WS_X11

  // Using motif style gives us test failures (and its ugly).
  // Using cleanlooks style gives us errors when using valgrind (Trolltech's bug #179200)
  // let's just use plastique for now
  QApplication::setStyle(new QPlastiqueStyle);

#endif

  pqComponentsInit();

  // If building an installer, look in a special location
  // for Qt plugins.
#ifdef OVERVIEW_INSTALLER_SUPPORT
  QDir plugin_directory(QApplication::applicationDirPath());
  plugin_directory.cd("qtplugins");
  QApplication::setLibraryPaths(QStringList(plugin_directory.absolutePath()));
#endif

  return pqMain::Run(app, vtkSmartPointer<ProcessModuleGUIHelper>::New());
}
