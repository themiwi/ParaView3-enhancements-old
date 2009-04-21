/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: OverView.cxx,v $

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

#include "OverView.h"
#include "ProcessModuleGUIHelper.h"
#include "pqComponentsInit.h"
#include "pqMain.h"
#include "vtkSmartPointer.h"

#include <QApplication>
#include <QTimer>

#ifdef Q_WS_X11
#include <QPlastiqueStyle>
#endif

static QString g_BrandedApplicationTitle;
static QString g_BrandedSplashTextColor;
static QString g_BrandedVersion;
static QString g_BrandedFullVersion;
static ProcessModuleGUIHelper* g_ProcessModuleGUIHelper = 0;

int OverView::main(int argc, char* argv[],
  const QString& WindowType,
  const QStringList& ConfiguredPlugins,
  const QString& BrandedApplicationTitle,
  const QString& BrandedSplashTextColor,
  const QString& BrandedVersion,
  const QString& BrandedFullVersion
  )
{
  g_BrandedApplicationTitle = BrandedApplicationTitle;
  g_BrandedSplashTextColor = BrandedSplashTextColor;
  g_BrandedVersion = BrandedVersion;
  g_BrandedFullVersion = BrandedFullVersion;

  QApplication app(argc, argv);

#ifdef Q_WS_X11
  // Using motif style gives us test failures (and its ugly).
  // Using cleanlooks style gives us errors when using valgrind (Trolltech's bug #179200)
  // let's just use plastique for now
  QApplication::setStyle(new QPlastiqueStyle);
#endif

  pqComponentsInit();

  vtkSmartPointer<ProcessModuleGUIHelper> helper = vtkSmartPointer<ProcessModuleGUIHelper>::New();
  g_ProcessModuleGUIHelper = helper;
  helper->SetWindowType(WindowType);
  helper->SetConfiguredPlugins(ConfiguredPlugins);

  return pqMain::Run(app, helper);
}

const QString OverView::GetBrandedApplicationTitle()
{
  return g_BrandedApplicationTitle;
}

const QString OverView::GetBrandedSplashTextColor()
{
  return g_BrandedSplashTextColor;
}

const QString OverView::GetBrandedVersion()
{
  return g_BrandedVersion;
}

const QString OverView::GetBrandedFullVersion()
{
  return g_BrandedFullVersion;
}

QWidget* OverView::GetUserInterface()
{
  return g_ProcessModuleGUIHelper ? g_ProcessModuleGUIHelper->GetUserInterface() : 0;
}

void OverView::ExitApplication()
{
  QTimer::singleShot(0, QApplication::instance(), SLOT(quit()));
}

