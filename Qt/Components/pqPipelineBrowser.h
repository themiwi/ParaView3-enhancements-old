/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqPipelineBrowser.h,v $

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

/// \file pqPipelineBrowser.h
/// \date 4/20/2006

#ifndef _pqPipelineBrowser_h
#define _pqPipelineBrowser_h


#include "pqComponentsExport.h"
#include <QWidget>

class pqPipelineBrowserInternal;
class pqConsumerDisplay;
class pqFlatTreeView;
class pqGenericViewModule;
class pqPipelineModel;
class pqPipelineSource;
class pqServer;
class pqServerManagerModelItem;
class pqSourceHistoryModel;
class pqSourceInfoGroupMap;
class pqSourceInfoIcons;
class pqSourceInfoModel;
class QItemSelectionModel;
class QModelIndex;
class QStringList;
class vtkPVXMLElement;
class vtkSMProxy;

// This is the pipeline browser widget. It creates the pqPipelineModel
// and the pqFlatTreeView. pqPipelineModel observes events from the
// pqServerManagerModel do keep the pipeline view in sync with the 
// the server manager. It provides slot (select()) to change the currently
// selected item, it also fires a signal selectionChanged() when the selection
// changes.
class PQCOMPONENTS_EXPORT pqPipelineBrowser : public QWidget
{
  Q_OBJECT

public:
  /// \brief
  ///   Creates a pipeline browser instance.
  /// \param parent The parent widget.
  pqPipelineBrowser(QWidget *parent=0);
  virtual ~pqPipelineBrowser();

  /// \brief
  ///   Used to monitor the key press events in the tree view.
  /// \param object The object receiving the event.
  /// \param e The event information.
  /// \return
  ///   True if the event should not be sent to the object.
  virtual bool eventFilter(QObject *object, QEvent *e);

  pqPipelineModel *getListModel() const {return this->ListModel;}
  pqFlatTreeView *getTreeView() const {return this->TreeView;}

  pqSourceInfoIcons *getIcons() const {return this->Icons;}

  /// \name Session Continuity Methods
  //@{
  void loadFilterInfo(vtkPVXMLElement *root);

  void saveState(vtkPVXMLElement *root) const;

  void restoreState(vtkPVXMLElement *root);
  //@}

  /// \name Selection Helper Methods
  //@{
  /// \brief
  ///   Gets the selection model from the tree view.
  /// \return
  ///   A pointer to the selection model.
  QItemSelectionModel *getSelectionModel() const;

  /// \brief
  ///   Gets the server manager model item for the current index.
  /// \return
  ///   A pointer to the current server manager model item, which can
  ///   be a server, source, or filter.
  pqServerManagerModelItem *getCurrentSelection() const;

  /// \brief
  ///   Gets the server for the current index.
  ///
  /// If the current index is a server, this method is equivalent to
  /// \c getCurrentSelection. If the current index is a source or
  /// filter, the server for the item is returned.
  ///
  /// \return
  ///   A pointer to the server for the current index.
  pqServer *getCurrentServer() const;
  //@}
  
  /// get the view module this pipeline browser works with
  pqGenericViewModule *getViewModule() const;

  /// Helper method to create a display for the source
  /// on the current view module.
  pqConsumerDisplay *createDisplay(pqPipelineSource *source, bool visible);

public slots:
  // Call this to select the particular item.
  void select(pqServerManagerModelItem* item);
  void select(pqPipelineSource* src);
  void select(pqServer* server);

  /// \name Model Modification Methods
  //@{
  void addSource();
  void addFilter();
  void changeInput();
  void deleteSelected();
  //@}

  /// \brief
  ///   Sets the current render module.
  /// \param rm The current render module.
  void setViewModule(pqGenericViewModule* rm);

signals:
  /// \brief
  ///   Emitted when the selection changes.
  /// \param selectedItem The newly selected item.
  void selectionChanged(pqServerManagerModelItem* selectedItem);
  
private slots:
  void changeCurrent(const QModelIndex &current, const QModelIndex &previous);
  void handleIndexClicked(const QModelIndex &index);
  void saveState(const QModelIndex &index);
  void restoreState(const QModelIndex &index);

private:
  pqSourceInfoModel *getFilterModel();
  void setupConnections(pqSourceInfoModel *model, pqSourceInfoGroupMap *map);
  void getAllowedSources(pqSourceInfoModel *model, vtkSMProxy *input,
      QStringList &list);
  void saveState(const QModelIndex &index, vtkPVXMLElement *root) const;
  void restoreState(const QModelIndex &index, vtkPVXMLElement *root);

private:
  pqPipelineBrowserInternal *Internal; ///< Stores the class data.
  pqPipelineModel *ListModel;          ///< Stores the pipeline model.
  pqFlatTreeView *TreeView;            ///< Stores the tree view.
  pqSourceInfoIcons *Icons;            ///< Stores the icons.
  pqSourceInfoGroupMap *FilterGroups;  ///< Stores the filter grouping.
  pqSourceHistoryModel *FilterHistory; ///< Stores the recent filters.
};

#endif
