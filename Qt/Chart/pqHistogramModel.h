/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqHistogramModel.h,v $

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

/// \file pqHistogramModel.h
/// \date 8/15/2006

#ifndef _pqHistogramModel_h
#define _pqHistogramModel_h


#include "QtChartExport.h"
#include <QObject>

class pqChartValue;


class QTCHART_EXPORT pqHistogramModel : public QObject
{
  Q_OBJECT

public:
  pqHistogramModel(QObject *parent=0);
  virtual ~pqHistogramModel() {}

  virtual int getNumberOfBins() const=0;
  virtual void getBinValue(int index, pqChartValue &bin) const=0;

  virtual void getRangeX(pqChartValue &min, pqChartValue &max) const=0;

  virtual void getRangeY(pqChartValue &min, pqChartValue &max) const=0;

signals:
  void binValuesReset();
  void aboutToInsertBinValues(int first, int last);
  void binValuesInserted();
  void aboutToRemoveBinValues(int first, int last);
  void binValuesRemoved();
  void rangeChanged(const pqChartValue &min, const pqChartValue &max);

protected:
  void resetBinValues();
  void beginInsertBinValues(int first, int last);
  void endInsertBinValues();
  void beginRemoveBinValues(int first, int last);
  void endRemoveBinValues();
};

#endif
