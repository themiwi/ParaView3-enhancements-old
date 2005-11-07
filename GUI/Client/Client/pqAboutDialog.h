/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#ifndef _pqAboutDialog_h
#define _pqAboutDialog_h

#include "ui_pqAbout.h"

/// Provides an about dialog
class pqAboutDialog :
  public QDialog
{
  Q_OBJECT

public:
  pqAboutDialog(QWidget* Parent);

private:
  ~pqAboutDialog();
  pqAboutDialog(const pqAboutDialog&);
  pqAboutDialog& operator=(const pqAboutDialog&);
  
  Ui::pqAboutDialog Ui;
  
private slots:
  void accept();
  void reject();
};

#endif // !_pqAboutDialog_h

