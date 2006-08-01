/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqChartPlugin.cxx,v $

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

#if !defined(_DEBUG)
# if !defined(QT_NO_DEBUG)
#  define QT_NO_DEBUG
# endif
#endif

#include "pqChartPlugin.h"
#include "pqHistogramWidget.h"
#include "pqHistogramWidgetPlugin.h"
#include "pqLineChartWidget.h"
#include "pqLineChartWidgetPlugin.h"


pqHistogramWidgetPlugin::pqHistogramWidgetPlugin(QObject *p)
  : QObject(p)
{
}

QWidget *pqHistogramWidgetPlugin::createWidget(QWidget *p)
{
  return new pqHistogramWidget(p);
}

QString pqHistogramWidgetPlugin::domXml() const
{
  return QLatin1String(
      "<widget class=\"pqHistogramWidget\" name=\"pqHistogram\">\n"
      " <property name=\"geometry\">\n"
      "  <rect>\n"
      "   <x>0</x>\n"
      "   <y>0</y>\n"
      "   <width>100</width>\n"
      "   <height>100</height>\n"
      "  </rect>\n"
      " </property>\n"
      "</widget>\n");
}

QIcon pqHistogramWidgetPlugin::icon() const
{
  return QIcon(QPixmap(":/pqChart/pqHistogram22.png"));
}

QString pqHistogramWidgetPlugin::includeFile() const
{
  return QLatin1String("pqHistogramWidget.h");
}

QString pqHistogramWidgetPlugin::toolTip() const
{
  return QLatin1String("Qt Histogram");
}

QString pqHistogramWidgetPlugin::whatsThis() const
{
  return QLatin1String("Qt Histogram");
}


pqLineChartWidgetPlugin::pqLineChartWidgetPlugin(QObject *p)
  : QObject(p)
{
}

QWidget *pqLineChartWidgetPlugin::createWidget(QWidget *p)
{
  return new pqLineChartWidget(p);
}

QString pqLineChartWidgetPlugin::domXml() const
{
  return QLatin1String(
      "<widget class=\"pqLineChartWidget\" name=\"pqLineChart\">\n"
      " <property name=\"geometry\">\n"
      "  <rect>\n"
      "   <x>0</x>\n"
      "   <y>0</y>\n"
      "   <width>100</width>\n"
      "   <height>100</height>\n"
      "  </rect>\n"
      " </property>\n"
      "</widget>\n");
}

QIcon pqLineChartWidgetPlugin::icon() const
{
  return QIcon(QPixmap(":/pqChart/pqLineChart22.png"));
}

QString pqLineChartWidgetPlugin::includeFile() const
{
  return QLatin1String("pqLineChartWidget.h");
}

QString pqLineChartWidgetPlugin::toolTip() const
{
  return QLatin1String("Qt Line Chart");
}

QString pqLineChartWidgetPlugin::whatsThis() const
{
  return QLatin1String("Qt Line Chart.");
}


pqChartPlugin::pqChartPlugin(QObject *p)
  : QObject(p), QDesignerCustomWidgetCollectionInterface()
{
  this->Histogram = new pqHistogramWidgetPlugin();
  this->LineChart = new pqLineChartWidgetPlugin();
}

pqChartPlugin::~pqChartPlugin()
{
  if(this->Histogram)
    delete this->Histogram;
  if(this->LineChart)
    delete this->LineChart;
}

QList<QDesignerCustomWidgetInterface*> pqChartPlugin::customWidgets() const
{
  QList<QDesignerCustomWidgetInterface*> plugins;
  plugins.append(Histogram);
  plugins.append(LineChart);
  return plugins;
}

Q_EXPORT_PLUGIN(pqChartPlugin)
