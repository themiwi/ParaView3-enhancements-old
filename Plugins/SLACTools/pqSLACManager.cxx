// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: pqSLACManager.cxx,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2009 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "pqSLACManager.h"

#include "pqSLACDataLoadManager.h"

#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkSMProxy.h"

#include "pqActiveView.h"
#include "pqApplicationCore.h"
#include "pqOutputPort.h"
#include "pqPipelineSource.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"

#include <QMainWindow>
#include <QPointer>
#include <QtDebug>

#include "ui_pqSLACActionHolder.h"

//=============================================================================
class pqSLACManager::pqInternal
{
public:
  Ui::pqSLACActionHolder Actions;
  QWidget *ActionPlaceholder;
};

//=============================================================================
QPointer<pqSLACManager> pqSLACManagerInstance = NULL;

pqSLACManager *pqSLACManager::instance()
{
  if (pqSLACManagerInstance == NULL)
    {
    pqApplicationCore *core = pqApplicationCore::instance();
    if (!core)
      {
      qFatal("Cannot use the SLAC Tools without an application core instance.");
      return NULL;
      }

    pqSLACManagerInstance = new pqSLACManager(core);
    }

  return pqSLACManagerInstance;
}

//-----------------------------------------------------------------------------
pqSLACManager::pqSLACManager(QObject *p) : QObject(p)
{
  this->Internal = new pqSLACManager::pqInternal;

  // This widget serves no real purpose other than initializing the Actions
  // structure created with designer that holds the actions.
  this->Internal->ActionPlaceholder = new QWidget(NULL);
  this->Internal->Actions.setupUi(this->Internal->ActionPlaceholder);

  QObject::connect(this->actionDataLoadManager(), SIGNAL(triggered(bool)),
                   this, SLOT(showDataLoadManager()));

  this->checkFieldActionStatus();
}

pqSLACManager::~pqSLACManager()
{
  delete this->Internal->ActionPlaceholder;
  delete this->Internal;
}

//-----------------------------------------------------------------------------
QAction *pqSLACManager::actionDataLoadManager()
{
  return this->Internal->Actions.actionDataLoadManager;
}

QAction *pqSLACManager::actionShowEField()
{
  return this->Internal->Actions.actionShowEField;
}

QAction *pqSLACManager::actionShowBField()
{
  return this->Internal->Actions.actionShowBField;
}

//-----------------------------------------------------------------------------
pqServer *pqSLACManager::activeServer()
{
  pqApplicationCore *app = pqApplicationCore::instance();
  pqServerManagerModel *smModel = app->getServerManagerModel();
  pqServer *server = smModel->getItemAtIndex<pqServer*>(0);
  return server;
}

//-----------------------------------------------------------------------------
QWidget *pqSLACManager::mainWindow()
{
  foreach(QWidget *topWidget, QApplication::topLevelWidgets())
    {
    if (qobject_cast<QMainWindow*>(topWidget)) return topWidget;
    }
  return NULL;
}

//-----------------------------------------------------------------------------
pqView *pqSLACManager::view3D()
{
  pqView *view = pqActiveView::instance().current();
  // TODO: Check to make sure the active view is 3D.  If not, find one.  This
  // can be done by getting a pqServerManagerModel (from pqAppliationCore)
  // and querying pqView.  If no view is valid, create one.  Probably look
  // at pqDisplayPolicy::createPreferredRepresentation for that one.
  //
  // On second thought, since I have to be able to query for the mesh file
  // anyway, perhaps I should just find that and return a view in which that
  // is defined.  Then let pqSLACDataLoadManager create a necessary view on
  // creating the mesh reader if necessary.
  return view;
}

//-----------------------------------------------------------------------------
pqPipelineSource *pqSLACManager::findPipelineSource(const char *SMName)
{
  pqApplicationCore *core = pqApplicationCore::instance();
  pqServerManagerModel *smModel = core->getServerManagerModel();

  QList<pqPipelineSource*> sources
    = smModel->findItems<pqPipelineSource*>(this->activeServer());
  foreach(pqPipelineSource *s, sources)
    {
    if (strcmp(s->getProxy()->GetXMLName(), SMName) == 0) return s;
    }

  return NULL;
}

pqPipelineSource *pqSLACManager::meshReader()
{
  return this->findPipelineSource("SLACReader");
}

pqPipelineSource *pqSLACManager::particlesReader()
{
  return this->findPipelineSource("SLACParticleReader");
}

//-----------------------------------------------------------------------------
void pqSLACManager::showDataLoadManager()
{
  pqSLACDataLoadManager *dialog = new pqSLACDataLoadManager(this->mainWindow());
  dialog->setAttribute(Qt::WA_DeleteOnClose, true);
  QObject::connect(dialog, SIGNAL(createdPipeline()),
                   this, SLOT(checkFieldActionStatus()));
  dialog->show();
}

//-----------------------------------------------------------------------------
void pqSLACManager::checkFieldActionStatus()
{
  pqPipelineSource *reader = this->meshReader();

  if (!reader)
    {
    this->actionShowEField()->setEnabled(false);
    this->actionShowBField()->setEnabled(false);
    return;
    }

  pqOutputPort *outputPort = reader->getOutputPort(0);
  vtkPVDataInformation *dataInfo = outputPort->getDataInformation();
  vtkPVDataSetAttributesInformation *pointFields
    = dataInfo->GetPointDataInformation();

  this->actionShowEField()->setEnabled(
                            pointFields->GetArrayInformation("efield") != NULL);
  this->actionShowBField()->setEnabled(
                            pointFields->GetArrayInformation("bfield") != NULL);
}
