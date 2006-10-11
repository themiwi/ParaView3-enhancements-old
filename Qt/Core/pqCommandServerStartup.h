/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqCommandServerStartup.h,v $

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

#ifndef _pqCommandServerStartup_h
#define _pqCommandServerStartup_h

#include "pqCoreExport.h"
#include "pqServerStartup.h"

#include <QProcess>

/////////////////////////////////////////////////////////////////////////////
// pqCommandServerStartup

/// Concrete implementation of pqServerStartup that runs an external
/// command to start a remote server.
class PQCORE_EXPORT pqCommandServerStartup :
  public pqServerStartup
{
public:
  pqCommandServerStartup(
    const QString& name,
    const pqServerResource& server,
    const QString& owner,
    const QDomDocument& configuration);

  const QString getName();
  const pqServerResource getServer();  
  const QString getOwner();
  const QDomDocument getConfiguration();
  
  void execute(const OptionsT& options, pqServerStartupContext& context);

  const QString getExecutable();
  double getTimeout();
  double getDelay();
  const QStringList getArguments();

private:
  const QString Name;
  const pqServerResource Server;
  const QString Owner;
  const QDomDocument Configuration;
};

/// Private implementation detail ... pretend you didn't see this
class pqCommandServerStartupContextHelper :
  public QObject
{
  Q_OBJECT
  
signals:
  void succeeded();
  void failed();

private slots:
  void onReadyReadStandardOutput();
  void onReadyReadStandardError();
  void onFinished(int exitCode, QProcess::ExitStatus exitStatus);
  void onError(QProcess::ProcessError error);
  void onDelayComplete();
  
private:
  pqCommandServerStartupContextHelper(QProcess* process, double delay, QObject* parent);
  
  friend class pqCommandServerStartup;
  
  QProcess* const Process;
  const double Delay;
};

#endif // !_pqCommandServerStartup
