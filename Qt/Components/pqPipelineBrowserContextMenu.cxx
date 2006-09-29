/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqPipelineBrowserContextMenu.cxx,v $

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

/// \file pqPipelineBrowserContextMenu.cxx
/// \date 4/20/2006

#include "pqPipelineBrowserContextMenu.h"

#include "pqApplicationCore.h"
#include "pqDisplayProxyEditor.h"
#include "pqFlatTreeView.h"
#include "pqPipelineBrowser.h"
#include "pqPipelineDisplay.h"
#include "pqPipelineModel.h"
#include "pqPipelineSource.h"
#include "pqRenderModule.h"
#include "pqServer.h"
#include "pqUndoStack.h"

#include <QDialog>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMenu>

#include "vtkSMProxy.h"
#include "vtkSMDataObjectDisplayProxy.h"


pqPipelineBrowserContextMenu::pqPipelineBrowserContextMenu(
    pqPipelineBrowser *browser)
  : QObject(browser)
{
  this->Browser = browser;

  this->setObjectName("ContextMenu");

  // Listen for the custom context menu signal.
  if(this->Browser)
    {
    QObject::connect(this->Browser->getTreeView(),
        SIGNAL(customContextMenuRequested(const QPoint &)),
        this, SLOT(showContextMenu(const QPoint &)));
    }
}

pqPipelineBrowserContextMenu::~pqPipelineBrowserContextMenu()
{
}

void pqPipelineBrowserContextMenu::showContextMenu(const QPoint &pos)
{
  if(!this->Browser)
    {
    return;
    }

  QMenu menu;
  menu.setObjectName("PipelineObjectMenu");
  bool menuHasItems = false;
  pqFlatTreeView *tree = this->Browser->getTreeView();
  QModelIndex current = tree->selectionModel()->currentIndex();
  pqPipelineModel *model = this->Browser->getListModel();
  pqPipelineSource *source = dynamic_cast<pqPipelineSource*>(
    model->getItemFor(current));
  pqServer* server = dynamic_cast<pqServer*>(model->getItemFor(current));
  if(source)
    {
    QAction* action;

    // Add the context menu items for a pipeline object.
    action = menu.addAction("Display Settings...", this, SLOT(showDisplayEditor()));
    action->setObjectName("Display Settings");
    // here we check just the display count. If no display present at all,
    // it implies mostly that the pending display is still pending.
    if(source->getDisplayCount() == 0)
      {
      action->setEnabled(false);
      }

    action = menu.addAction("Delete", this->Browser, SLOT(deleteSelected()));
    action->setObjectName("Delete");
    if (source->getNumberOfConsumers() > 0)
      {
      action->setEnabled(false);
      }
    menuHasItems = true;
    }
  else if(server)
    {
    // Show the context menu for a server object.
    }

  if(menuHasItems)
    {
    menu.exec(tree->mapToGlobal(pos));
    }
}

void pqPipelineBrowserContextMenu::showDisplayEditor()
{
  if(!this->Browser)
    {
    return;
    }

  // Show the display proxy editor for the selected object.
  pqFlatTreeView *tree = this->Browser->getTreeView();
  QModelIndex current = tree->selectionModel()->currentIndex();
  pqPipelineModel *model = this->Browser->getListModel();
  pqPipelineSource *source = dynamic_cast<pqPipelineSource*>(
      model->getItemFor(current));
  if(!source)
    {
    return;
    }

  // Add the dialog to the main window.
  QWidget* topParent = this->Browser;
  while(topParent->parentWidget())
    {
    topParent = topParent->parentWidget();
    }

  QDialog dialog(topParent);
  dialog.setObjectName("ObjectDisplayProperties");
  dialog.setWindowTitle("Display Settings");
  QVBoxLayout* l = new QVBoxLayout(&dialog);
  l->setMargin(0);
  l->setSpacing(6);
  pqDisplayProxyEditor* editor = new pqDisplayProxyEditor(&dialog);
  pqPipelineDisplay* display = 
    qobject_cast<pqPipelineDisplay*>(source->getDisplay(
      this->Browser->getViewModule()));
  if (!display)
    {
    // If display doesn't exist, as far as the user is concerned, it simply
    // means the source is not visible. Create a new hidden display 
    // and show its properties.
    display = qobject_cast<pqPipelineDisplay*>(
      this->Browser->createDisplay(source, false));
    }
  editor->setDisplay(display);
  l->addWidget(editor);

  QHBoxLayout* hl = new QHBoxLayout;
  hl->setMargin(6);
  l->addLayout(hl);
  hl->addStretch();

  QPushButton* closeButton = new QPushButton(&dialog);
  closeButton->setObjectName("DismissButton");
  closeButton->setText(tr("Close"));
  closeButton->setAutoDefault(true);
  hl->addWidget(closeButton);

  QObject::connect(closeButton, SIGNAL(clicked(bool)),
    &dialog, SLOT(accept()));
  
  pqApplicationCore::instance()->getUndoStack()->
    BeginUndoSet("Display Settings");
  dialog.exec();
  pqApplicationCore::instance()->getUndoStack()->
    EndUndoSet();
}

void pqPipelineBrowserContextMenu::showRenderViewEditor()
{
  // TODO: Move this code to the correct location.
  /*QWidget* widget = this->ListModel->getWidgetFor(selection);
  vtkSMRenderModuleProxy* pw = pqServerManagerObserver::instance()->getRenderModule(qobject_cast<QVTKWidget*>(widget));
  if(pw)
    {
    static const char* ViewSettingString = "View Settings...";
    QMenu menu;
    menu.addAction(ViewSettingString);
    QAction* action = menu.exec(this->TreeView->mapToGlobal(position));
    if(action && action->text() == ViewSettingString)
      {
      QDialog* dialog = new QDialog(topParent);
      dialog->setWindowTitle("View Settings");
      QHBoxLayout* l = new QHBoxLayout(dialog);
      pqRenderViewEditor* editor = new pqRenderViewEditor(dialog);
      editor->setRenderView(pw);
      l->addWidget(editor);
      dialog->show();
      }
    }*/
}


