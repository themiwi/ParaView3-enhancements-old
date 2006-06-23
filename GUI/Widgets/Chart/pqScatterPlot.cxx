/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqScatterPlot.cxx,v $

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

/*!
 * \file pqScatterPlot.cxx
 *
 * \brief
 *   The pqScatterPlot class is used to draw a piecewise linear
 *   function.
 *
 * \author Mark Richardson
 * \date   August 22, 2005
 */

#include "pqChartAxis.h"
#include "pqChartValue.h"
#include "pqMarkerPen.h"
#include "pqScatterPlot.h"

#include <QHelpEvent>
#include <QPolygon>
#include <QToolTip>

#include <vtkType.h>

/////////////////////////////////////////////////////////////////////////
// pqScatterPlot::pqImplementation

class pqScatterPlot::pqImplementation
{
public:
  pqImplementation(pqMarkerPen* pen) :
    Pen(pen)
  {
  }
  
  ~pqImplementation()
  {
    delete Pen;
  }

  void setCoordinates(const pqChartCoordinateList& list)
  {
    this->WorldCoords = list;
    this->ScreenCoords.clear();
    
    this->WorldMin = pqChartCoordinate(VTK_DOUBLE_MAX, VTK_DOUBLE_MAX);
    this->WorldMax = pqChartCoordinate(-VTK_DOUBLE_MAX, -VTK_DOUBLE_MAX);
    
    for(pqChartCoordinateList::Iterator coords = this->WorldCoords.begin(); coords != this->WorldCoords.end(); ++coords)
      {
      this->WorldMin.X = vtkstd::min(this->WorldMin.X, coords->X);
      this->WorldMin.Y = vtkstd::min(this->WorldMin.Y, coords->Y);
      this->WorldMax.X = vtkstd::max(this->WorldMax.X, coords->X);
      this->WorldMax.Y = vtkstd::max(this->WorldMax.Y, coords->Y);
      }
  }
  
  /// Stores plot data in world coordinates
  pqChartCoordinateList WorldCoords;
  /// Caches plot data in screen coordinates
  QPolygon ScreenCoords;
  /// Stores the minimum world coordinate values
  pqChartCoordinate WorldMin;
  /// Stores the maximum world coordinate values
  pqChartCoordinate WorldMax;
  /// Stores the pen used for drawing
  pqMarkerPen* const Pen;
};

////////////////////////////////////////////////////////////////////////////
// pqScatterPlot

pqScatterPlot::pqScatterPlot(pqMarkerPen* pen, const pqChartCoordinateList& coords) :
  pqAbstractPlot(),
  Implementation(new pqImplementation(pen))
{
  this->Implementation->setCoordinates(coords);
}

pqScatterPlot::~pqScatterPlot()
{
  delete this->Implementation;
}

const pqChartCoordinate pqScatterPlot::getMinimum() const
{
  return this->Implementation->WorldMin;
}

const pqChartCoordinate pqScatterPlot::getMaximum() const
{
  return this->Implementation->WorldMax;
}

void pqScatterPlot::layoutPlot(const pqChartAxis& XAxis, const pqChartAxis& YAxis)
{
  this->Implementation->ScreenCoords.resize(this->Implementation->WorldCoords.getSize());
  for(int i = 0; i != this->Implementation->WorldCoords.getSize(); ++i)
    {
    this->Implementation->ScreenCoords[i].rx() = XAxis.getPixelFor(this->Implementation->WorldCoords[i].X);
    this->Implementation->ScreenCoords[i].ry() = YAxis.getPixelFor(this->Implementation->WorldCoords[i].Y);
    }
}

void pqScatterPlot::drawPlot(QPainter& painter, const QRect& /*area*/, const pqChartAxis& /*XAxis*/, const pqChartAxis& /*YAxis*/)
{
  this->Implementation->Pen->resetMarkers();
  this->Implementation->Pen->drawPoints(painter, this->Implementation->ScreenCoords);
}

const double pqScatterPlot::getDistance(const QPoint& coords) const
{
  double distance = VTK_DOUBLE_MAX;
  for(int i = 0; i != this->Implementation->ScreenCoords.size(); ++i)
    distance = vtkstd::min(distance, static_cast<double>((this->Implementation->ScreenCoords[i] - coords).manhattanLength()));
  return distance;
}

void pqScatterPlot::showChartTip(QHelpEvent& event) const
{
  double tip_distance = VTK_DOUBLE_MAX;
  pqChartCoordinate tip_coordinate;
  for(int i = 0; i != this->Implementation->ScreenCoords.size(); ++i)
    {
    const double distance = (this->Implementation->ScreenCoords[i] - event.pos()).manhattanLength();
    if(distance < tip_distance)
      {
      tip_distance = distance;
      tip_coordinate = this->Implementation->WorldCoords[i];
      }
    }

  if(tip_distance < VTK_DOUBLE_MAX)    
    QToolTip::showText(event.globalPos(), QString("%1, %2").arg(tip_coordinate.X.getDoubleValue()).arg(tip_coordinate.Y.getDoubleValue()));
}

