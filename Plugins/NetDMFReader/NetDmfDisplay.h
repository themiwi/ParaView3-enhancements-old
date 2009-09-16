#ifndef __NetDmfDisplay_h
#define __NetDmfDisplay_h

#include "pqDisplayPanel.h"

/// a simple display panel widget for GlyphRepresentation representations
class NetDmfDisplay : public pqDisplayPanel
{
  Q_OBJECT
public:
  /// constructor
  NetDmfDisplay(pqRepresentation* display, QWidget* p = NULL);
  ~NetDmfDisplay();

public slots:
  void zoomToData();
  void cubeAxesVisibilityChanged();
  void editCubeAxes();
  void updateGlyphMode();

protected:
  class pqInternal;
  pqInternal* Internal;
};

#endif // __NetDmfDisplay_h
