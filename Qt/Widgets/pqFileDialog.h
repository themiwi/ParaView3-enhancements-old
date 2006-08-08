/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqFileDialog.h,v $

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

#ifndef _pqFileDialog_h
#define _pqFileDialog_h

#include "QtWidgetsExport.h"
#include <QDialog>

class pqFileDialogModel;
class pqFileDialogFilter;
namespace Ui { class pqFileDialog; }
class QModelIndex;

/**
  Provides a standard file dialog "front-end" for the pqFileDialogModel "back-end", i.e. it can be used for both local and remote file browsing.

  When creating an instance of pqFileDialog, you must provide an instance of the back-end.  To get the file(s) selected by the user, connect
  to the filesSelected() signal:
  
  /code
  pqFileDialog* dialog = new pqFileDialog(new pqLocalFileDialogModel(), "Open Session File", this, "OpenSessionFile");
  dialog << pqConnect(SIGNAL(filesSelected(const QStringList&)), this, SLOT(onOpenSessionFile(const QStringList&)));
  /endcode
  
  \sa pqLocalFileDialogModel, pqServerFileDialogModel
*/

class QTWIDGETS_EXPORT pqFileDialog :
  public QDialog
{
  typedef QDialog base;
  
  Q_OBJECT
  
public:

  /// choose mode for selecting file/folder.
  /// AnyFile: The name of a file, whether it exists or not.  
  ///   Typically used by "Save As..."
  /// ExistingFile: The name of a single existing file.
  ///   Typically used by "Open..."
  /// ExistingFiles: The names of zero or more existing files.
  /// Directory: The name of a directory.
  enum FileMode { AnyFile, ExistingFile, ExistingFiles, Directory };
    
  /// creates a file dialog using the dialog model
  /// the title, and start directory may be specified
  /// the filter is a string of semi-colon separated filters
  pqFileDialog(pqFileDialogModel* Model, QWidget* Parent, 
               const QString& Title = QString(), 
               const QString& Directory = QString(), 
               const QString& Filter = QString());
  ~pqFileDialog();

  /// Forces the dialog to emit the filesSelected() signal and close, as if receiving user input
  void emitFilesSelected(const QStringList&);

  /// returns the current file filter
  QString currentFilter();

  void accept();
  void reject();

  FileMode fileMode();
  void setFileMode(FileMode);
  
signals:
  /// Signal emitted when the user has chosen a set of files and accepted the dialog
  void filesSelected(const QStringList&);

protected:
  pqFileDialogModel* const Model;
  Ui::pqFileDialog* const Ui;
  pqFileDialogFilter* Filter;
  FileMode Mode;
  
protected slots:
  void onDataChanged(const QModelIndex&, const QModelIndex&);
  void onActivated(const QModelIndex&);
  void onClickedFiles(const QModelIndex&);
  void onClickedFavorites(const QModelIndex&);
  void onManualEntry(const QString&);
  void onNavigate(const QString&);
  void onNavigateUp();
  void onNavigateDown(const QModelIndex&);
  void onFilterChange(const QString&);

private:
  pqFileDialog(const pqFileDialog&);
  pqFileDialog& operator=(const pqFileDialog&);
  
  void onClicked(const QModelIndex&);
};

#endif // !_pqFileDialog_h
