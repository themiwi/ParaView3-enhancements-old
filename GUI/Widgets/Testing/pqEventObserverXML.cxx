/*=========================================================================

   Program:   ParaQ
   Module:    $RCSfile: pqEventObserverXML.cxx,v $

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.1. 

   See License_v1.1.txt for the full ParaQ license.
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

#include "pqEventObserverXML.h"

/// Escapes strings so they can be embedded in an XML document
static const QString textToXML(const QString& string)
{
  QString result = string;
  result.replace("&", "&amp;");
  result.replace("<", "&lt;");
  result.replace(">", "&gt;");
  result.replace("'", "&apos;");
  result.replace("\"", "&quot;");
  
  return result;
}

////////////////////////////////////////////////////////////////////////////////////
// pqEventObserverXML

pqEventObserverXML::pqEventObserverXML(ostream& stream) :
  Stream(stream)
{
  this->Stream << "<?xml version=\"1.0\" ?>\n";
  this->Stream << "<pqevents>\n";
}

pqEventObserverXML::~pqEventObserverXML()
{
  this->Stream << "</pqevents>\n";
}

void pqEventObserverXML::onRecordEvent(const QString& Widget, const QString& Command, const QString& Arguments)
{
  this->Stream
    << "  <pqevent "
    << "object=\"" << textToXML(Widget).toAscii().data() << "\" "
    << "command=\"" << textToXML(Command).toAscii().data() << "\" "
    << "arguments=\"" << textToXML(Arguments).toAscii().data() << "\" "
    << "/>\n";
}
