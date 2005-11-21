/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#ifndef _pqPythonDialog_h
#define _pqPythonDialog_h

#include <QDialog>

/// Provides an about dialog
class pqPythonDialog :
  public QDialog
{
  Q_OBJECT

public:
  pqPythonDialog(QWidget* Parent);
  
private slots:
  void accept();
  void reject();

private:
  ~pqPythonDialog();
  pqPythonDialog(const pqPythonDialog&);
  pqPythonDialog& operator=(const pqPythonDialog&);
  
  struct pqImplementation;
  pqImplementation* const Implementation;
};

#endif // !_pqPythonDialog_h

