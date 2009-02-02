/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqBarChartOptionsHandler.cxx,v $

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

#include "pqBarChartOptionsHandler.h"

#include "pqBarChartOptionsEditor.h"
#include "pqSMAdaptor.h"
#include "pqView.h"

#include "vtkQtBarChartOptions.h"
#include "vtkSMProxy.h"

#include <QVariant>


pqBarChartOptionsHandler::pqBarChartOptionsHandler()
{
  this->ModifiedData = 0;
  this->Options = 0;
  this->View = 0;
}

void pqBarChartOptionsHandler::setOptions(pqBarChartOptionsEditor *options)
{
  this->Options = options;
  if(this->Options)
    {
    this->Options->setApplyHandler(this);
    }
}

void pqBarChartOptionsHandler::setView(pqView *chart)
{
  this->View = chart;
  this->initializeOptions();
}

void pqBarChartOptionsHandler::setModified(
    pqBarChartOptionsHandler::ModifiedFlag flag)
{
  this->ModifiedData |= flag;
  this->Options->sendChangesAvailable();
}

void pqBarChartOptionsHandler::applyChanges()
{
  if(this->ModifiedData == 0 || !this->Options || !this->View)
    {
    return;
    }

  vtkSMProxy *proxy = this->View->getProxy();
  if(this->ModifiedData & HelpFormatModified)
    {
    QString text;
    this->Options->getHelpFormat(text);
    pqSMAdaptor::setElementProperty(proxy->GetProperty("HelpFormat"), text);
    }

  if(this->ModifiedData & OutlineStyleModified)
    {
    pqSMAdaptor::setElementProperty(proxy->GetProperty("OutlineStyle"),
        QVariant((int)this->Options->getOutlineStyle()));
    }

  if(this->ModifiedData & GroupFractionModified)
    {
    pqSMAdaptor::setElementProperty(proxy->GetProperty("GroupFraction"),
        QVariant((double)this->Options->getBarGroupFraction()));
    }

  if(this->ModifiedData & WidthFractionModified)
    {
    pqSMAdaptor::setElementProperty(proxy->GetProperty("WidthFraction"),
        QVariant((double)this->Options->getBarWidthFraction()));
    }

  this->ModifiedData = 0;
}

void pqBarChartOptionsHandler::resetChanges()
{
  if(this->ModifiedData == 0)
    {
    return;
    }

  this->initializeOptions();
  this->ModifiedData = 0;
}

void pqBarChartOptionsHandler::initializeOptions()
{
  if(!this->View || !this->Options)
    {
    return;
    }

  // Initialize the chart options. Block the signals from the editor.
  vtkSMProxy *proxy = this->View->getProxy();
  this->Options->blockSignals(true);

  this->Options->setHelpFormat(pqSMAdaptor::getElementProperty(
      proxy->GetProperty("HelpFormat")).toString());
  this->Options->setOutlineStyle(
      (vtkQtBarChartOptions::OutlineStyle)pqSMAdaptor::getElementProperty(
      proxy->GetProperty("OutlineStyle")).toInt());
  this->Options->setBarGroupFraction((float)pqSMAdaptor::getElementProperty(
      proxy->GetProperty("GroupFraction")).toDouble());
  this->Options->setBarWidthFraction((float)pqSMAdaptor::getElementProperty(
      proxy->GetProperty("WidthFraction")).toDouble());

  this->Options->blockSignals(false);
}


