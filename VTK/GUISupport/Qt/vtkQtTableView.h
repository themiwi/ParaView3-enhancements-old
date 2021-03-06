/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkQtTableView.h,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
// .NAME vtkQtTableView - A VTK view based on a Qt Table view.
//
// .SECTION Description
// vtkQtTableView is a VTK view using an underlying QTableView. 
//
// .SECTION Thanks
// Thanks to Brian Wylie from Sandia National Laboratories for implementing
// this class

#ifndef __vtkQtTableView_h
#define __vtkQtTableView_h

#include "QVTKWin32Header.h"
#include "vtkQtView.h"

#include <QPointer>
#include <QSortFilterProxyModel>
#include "vtkQtAbstractModelAdapter.h"
#include "vtkSmartPointer.h"

class vtkAddMembershipArray;
class vtkApplyColors;
class vtkDataObjectToTable;
class QItemSelection;
class QTableView;
class vtkQtTableModelAdapter;

class QVTK_EXPORT vtkQtTableView : public vtkQtView
{
Q_OBJECT

public:
  static vtkQtTableView *New();
  vtkTypeRevisionMacro(vtkQtTableView, vtkQtView);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Get the main container of this view (a  QWidget).
  // The application typically places the view with a call
  // to GetWidget(): something like this
  // this->ui->box->layout()->addWidget(this->View->GetWidget());
  virtual QWidget* GetWidget();
  
  // Description:
  // Have the view show/hide its column headers
  void SetShowVerticalHeaders(bool);
  
  // Description:
  // Have the view show/hide its row headers
  void SetShowHorizontalHeaders(bool);

  //BTX
  enum
    {
    FIELD_DATA = 0,
    POINT_DATA = 1,
    CELL_DATA = 2,
    VERTEX_DATA = 3,
    EDGE_DATA = 4,
    ROW_DATA = 5,
    };
  //ETX
  
  // Description:
  // The field type to copy into the output table.
  // Should be one of FIELD_DATA, POINT_DATA, CELL_DATA, VERTEX_DATA, EDGE_DATA.
  vtkGetMacro(FieldType, int);
  void SetFieldType(int);

  // Description: 
  // Whether or not to display all columns from the input table or to use the 
  // ColumnName provided.
  // FIXME: This should be replaced with an Add/Remove column API.
  void SetShowAll(bool);
  vtkGetMacro(ShowAll, bool);

  // Description: 
  // The name of a single column to display.
  // FIXME: This should be replaced with an Add/Remove column API.
  vtkSetStringMacro(ColumnName);
  vtkGetStringMacro(ColumnName);

  void SetColumnVisibility(const QString &name, bool status);

  // Description:
  // Set whether or not the table view should split multi-component columns
  // into multiple single-component columns
  void SetSplitMultiComponentColumns(bool value);

  // Description:
  // Get whether or not the table view splits multi-component columns into
  // multiple single-component columns
  bool GetSplitMultiComponentColumns();

  // Description:
  // Whether or not to sort selections that the view receives to the top
  void SetSortSelectionToTop(bool value);
  vtkGetMacro(SortSelectionToTop, bool);

  // Description:
  // Whether or not to add an icon to the row header denoting the color
  // of an annotated row.
  void SetApplyRowColors(bool value);
  vtkGetMacro(ApplyRowColors, bool);

  // Description:
  // Updates the view.
  virtual void Update();

protected:
  vtkQtTableView();
  ~vtkQtTableView();

  virtual void AddRepresentationInternal(vtkDataRepresentation* rep);
  virtual void RemoveRepresentationInternal(vtkDataRepresentation* rep);

private slots:
  void slotQtSelectionChanged(const QItemSelection&,const QItemSelection&);

private:
  void SetVTKSelection();
  unsigned long LastSelectionMTime;
  unsigned long LastInputMTime;
  unsigned long LastMTime;
  
  QPointer<QTableView> TableView;
  vtkQtTableModelAdapter* TableAdapter;
  QSortFilterProxyModel* TableSorter;
  int FieldType;    
  bool ShowAll;
  char* ColumnName;
  bool InSelectionChanged;
  bool SortSelectionToTop;
  bool ApplyRowColors;

//BTX
  vtkSmartPointer<vtkAddMembershipArray> AddSelectedColumn;
  vtkSmartPointer<vtkDataObjectToTable> DataObjectToTable;
  vtkSmartPointer<vtkApplyColors> ApplyColors;
//ETX
  
  vtkQtTableView(const vtkQtTableView&);  // Not implemented.
  void operator=(const vtkQtTableView&);  // Not implemented.
  
};

#endif
