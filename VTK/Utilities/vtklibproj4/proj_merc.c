/*
** libproj -- library of cartographic projections
**
** Copyright (c) 2003, 2006   Gerald I. Evenden
*/
static const char
LIBPROJ_ID[] = "$Id: proj_merc.c,v 1.1 2008-11-07 16:41:14 jeff Exp $";
/*
** Permission is hereby granted, free of charge, to any person obtaining
** a copy of this software and associated documentation files (the
** "Software"), to deal in the Software without restriction, including
** without limitation the rights to use, copy, modify, merge, publish,
** distribute, sublicense, and/or sell copies of the Software, and to
** permit persons to whom the Software is furnished to do so, subject to
** the following conditions:
**
** The above copyright notice and this permission notice shall be
** included in all copies or substantial portions of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
** EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
** MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
** IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
** CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
** TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
** SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#define PROJ_LIB__
#include  <lib_proj.h>
PROJ_HEAD(merc, "Mercator") "\n\tCyl, Sph&Ell\n\tlat_ts=";
#define EPS10 1.e-10
FORWARD(e_forward); /* ellipsoid */
  if (fabs(fabs(lp.phi) - HALFPI) <= EPS10) F_ERROR;
  xy.x = P->k0 * lp.lam;
  xy.y = - P->k0 * log(proj_tsfn(lp.phi, sin(lp.phi), P->e));
  return (xy);
}
FORWARD(s_forward); /* spheroid */
  if (fabs(fabs(lp.phi) - HALFPI) <= EPS10) F_ERROR;
  xy.x = P->k0 * lp.lam;
  xy.y = P->k0 * log(tan(FORTPI + .5 * lp.phi));
  return (xy);
}
INVERSE(e_inverse); /* ellipsoid */
  if ((lp.phi = proj_phi2(exp(- xy.y / P->k0), P->e)) == HUGE_VAL) I_ERROR;
  lp.lam = xy.x / P->k0;
  return (lp);
}
INVERSE(s_inverse); /* spheroid */
  lp.phi = HALFPI - 2. * atan(exp(-xy.y / P->k0));
  lp.lam = xy.x / P->k0;
  return (lp);
}
FREEUP; if (P) free(P); }
ENTRY0(merc)
  double phits = 0.;
  int is_phits;

  if ((is_phits = proj_param(P->params, "tlat_ts").i)) {
    phits = fabs(proj_param(P->params, "rlat_ts").f);
    if (phits >= HALFPI) E_ERROR(-24);
  }
  if (P->es) { /* ellipsoid */
    if (is_phits)
      P->k0 = proj_msfn(sin(phits), cos(phits), P->es);
    P->inv = e_inverse;
    P->fwd = e_forward;
  } else { /* sphere */
    if (is_phits)
      P->k0 = cos(phits);
    P->inv = s_inverse;
    P->fwd = s_forward;
  }
ENDENTRY(P)
/*
** $Log: proj_merc.c,v $
** Revision 1.1  2008-11-07 16:41:14  jeff
** ENH: Adding a 2D geoview. Adding the geographic projection library libproj4
** to Utilities. Updating the architecture of the geospatial views. All
** multi-resolution sources are now subclasses of vtkGeoSource. Each source
** has its own worker thread for fetching refined images or geometry.
** On the 3D side, vtkGeoGlobeSource is an appropriate source for vtkGeoTerrain,
** and vtkGeoAlignedImageSource is an appropriate source for
** vtkGeoAlignedImageRepresentation. On the 2D side, vtkGeoProjectionSource is an
** appropriate source for vtkGeoTerrain2D, and the image source is the same.
**
** Revision 3.1  2006/01/11 01:38:18  gie
** Initial
**
*/
