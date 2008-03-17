/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqSignalAdaptorCompositeTreeWidget.h,v $

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

========================================================================*/
#ifndef __pqSignalAdaptorCompositeTreeWidget_h 
#define __pqSignalAdaptorCompositeTreeWidget_h

#include <QObject>
#include <QVariant>
#include "pqComponentsExport.h"


class pqTreeWidgetItemObject;
class QTreeWidget;
class vtkPVDataInformation;
class vtkSMIntVectorProperty;
class vtkSMOutputPort;

/// pqSignalAdaptorCompositeTreeWidget is used to connect a property with
/// vtkSMCompositeTreeDomain as its domain to a Tree widget. It updates the tree
/// to show composite data tree.
class PQCOMPONENTS_EXPORT pqSignalAdaptorCompositeTreeWidget : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

  Q_PROPERTY(QList<QVariant> values READ values WRITE setValues)
public:
  enum IndexModes
    {
    INDEX_MODE_FLAT,
    INDEX_MODE_LEVEL_INDEX, // this mode works only for 1 level deep trees.
    INDEX_MODE_LEVEL,       // this mode works only for 1 level deep trees. 
    };


  /// Constructor. \c domain is used to build the tree layout to show in the
  /// widget.
  pqSignalAdaptorCompositeTreeWidget(QTreeWidget*, vtkSMIntVectorProperty* smproperty,
    bool autoUpdateWidgetVisibility=false);

  /// Alternate constructor. 
  /// \c outputport - the output port producing the composite dataset to show. 
  /// \c domainMode - vtkSMCompositeTreeDomain::ALL|LEAVES|NON_LEAVES|NONE. 
  ///                 Indicates what types of nodes in the composite tree 
  ///                 are selectable.
  /// \c indexMode  - indicates how the values are set/get (using
  ///                 composite-flat-index, using level-dataset-index or using 
  ///                 only the level number.
  /// \c selectMultiple - true if possible to select multiple nodes.
  /// \c autoUpdateWidgetVisibility -  is true, the tree widget is hidden if the
  ///                                  data information says that the data is
  ///                                  not composite.
  pqSignalAdaptorCompositeTreeWidget(QTreeWidget*, vtkSMOutputPort* outputport,
    int domainMode,
    IndexModes indexMode=INDEX_MODE_FLAT,
    bool selectMultiple=false,
    bool autoUpdateWidgetVisibility=false);

  /// Destructor
  virtual ~pqSignalAdaptorCompositeTreeWidget();

  /// Returns the current value.
  /// This is a QList of unsigned ints.
  QList<QVariant> values() const;

  /// When set, the adaptor will update the visibility of the widget depending
  /// on whether the data is composite or not.
  void setAutoUpdateWidgetVisibility(bool val)
    { this->AutoUpdateWidgetVisibility = val; }
  bool autoUpdateWidgetVisibility() const
    { return this->AutoUpdateWidgetVisibility; }

  /// Select the item with the given flat index.
  void select(unsigned int flatIndex);

  /// Returns the flat index for the current item.
  unsigned int getCurrentFlatIndex(bool* valid=NULL);

public slots:
  /// Set the values.
  void setValues(const QList<QVariant>& values);

  /// Called when domain changes.
  void domainChanged();

  /// Called when the output port says that the data information has been
  /// updated.
  void portInformationChanged();

signals:
  /// Fired when the widget value changes.
  void valuesChanged();

private slots:
  /// Called when an item check state is toggled. This is used only when
  /// this->CheckMode == SINGLE_ITEM. We uncheck all other items.
  void updateCheckState(bool checked);

private:
  pqSignalAdaptorCompositeTreeWidget(const pqSignalAdaptorCompositeTreeWidget&); // Not implemented.
  void operator=(const pqSignalAdaptorCompositeTreeWidget&); // Not implemented.


  void buildTree(pqTreeWidgetItemObject* item, 
    vtkPVDataInformation* info);

  /// updates the check flags for all the items.
  void updateItemFlags();

  class pqInternal;
  pqInternal* Internal;

  enum MetaData
    {
    FLAT_INDEX = Qt::UserRole,
    LEVEL_NUMBER = Qt::UserRole+1,
    DATASET_INDEX = Qt::UserRole+2,
    NODE_TYPE = Qt::UserRole+3,
    };

  enum NodeTypes
    {
    LEAF = 21,
    NON_LEAF = 22,
    };

  enum CheckModes
    {
    SINGLE_ITEM,
    MULTIPLE_ITEMS
    };

  IndexModes IndexMode;

  // Determines if the widget should allow checking only 1 item at a time or
  // multiple items should be check-able.
  CheckModes CheckMode;

  // These are used by buildTree() to determin indices for the nodes.
  unsigned int FlatIndex;
  unsigned int LevelNo;

  bool AutoUpdateWidgetVisibility;

  bool ShowIndex;

  // When set to true, all pieces within a  multipiece are shown.
  bool ShowDatasetsInMultiPiece;

  /// Code common to both variants of the constructor.
  void constructor(QTreeWidget* tree, bool autoUpdateVisibility);
};

#endif


