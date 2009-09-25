/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: PrismSurfacePanel.h,v $

=========================================================================*/
#ifndef _PrismSurfacePanel_h
#define _PrismSurfacePanel_h

#include <QWidget>
#include <QVariant>
#include "pqPipelineRepresentation.h"
#include "vtkSMProxy.h"
#include "pqNamedObjectPanel.h"

class QItemSelection;



/// Widget which provides an editor for the properties of a display.
class  PrismSurfacePanel : public pqNamedObjectPanel
{
  Q_OBJECT
  
public:
  /// constructor
  PrismSurfacePanel(pqProxy* proxy, QWidget* p = NULL);
  /// destructor
  ~PrismSurfacePanel();


public slots:
  
  /// reset changes made by this panel
  virtual void accept();
  virtual void reset();

protected:
  /// populate widgets with properties from the server manager
  virtual void linkServerManagerProperties();

  // fill the parameters part of the GUI
  void setupVariables();
  void setupTableWidget();
  void updateVariables();
 // void setupLogScaling();
  void updateXThresholds();
  void setupXThresholds();

  void updateYThresholds();
  void setupYThresholds();

  class pqUI;
  pqUI* UI;

protected slots:
  void setTableId(QString);
  void setXVariable(QString);
  void setYVariable(QString);
  void setZVariable(QString);
  void setContourVariable(QString);
  void lowerXChanged(double);
  void upperXChanged(double);
  void lowerYChanged(double);
  void upperYChanged(double);

  void useXLogScaling(bool);
  void useYLogScaling(bool);
  void useZLogScaling(bool);

  void onSamplesChanged();

  void onSelectionChanged(const QItemSelection&, const QItemSelection&);
  void onRangeChanged();

  void onDelete();
  void onDeleteAll();
  void onNewValue();
  void onNewRange();
  void onSelectAll();
  void onScientificNotation(bool);

private:
      bool eventFilter(QObject *object, QEvent *e);
  bool getRange(double& range_min, double& range_max);



};

#endif
