// -*- c++ -*-

/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkPlanesIntersection.h,v $
  Language:  C++
  Date:      $Date: 2003-10-01 00:25:21 $
  Version:   $Revision: 1.3 $

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  Copyright (C) 2003 Sandia Corporation
  Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
  license for use of this work by or on behalf of the U.S. Government.
  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that this Notice and any statement
  of authorship are reproduced on all copies.

  Contact: Lee Ann Fisk, lafisk@sandia.gov

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkPlanesIntersection - Computes whether the convex region
//    defined by (bounded by) it's planes intersects an axis aligned box.
//
// .SECTION Description
//    A subclass of vtkPlanes, this class determines whether it
//    intersects an axis aligned box.   This is motivated by the
//    need to intersect the axis aligned region of a spacial
//    decomposition of volume data with a view frustum created by
//    a rectangular portion of the view plane.  It uses the 
//    algorithm from Graphics Gems IV, page 81.
//
// .SECTION Caveat
//    An instance of vtkPlanes can be redefined by changing the planes,
//    but this subclass then will not know if the region vertices are
//    up to date.  (Region vertices can be specified in SetRegionVertices
//    or computed by the subclass.)  So Delete and recreate if you want
//    to change the set of planes.
//

#ifndef __vtkPlanesIntersection_h
#define __vtkPlanesIntersection_h

#include "vtkPlanes.h"

class vtkPoints;
class vtkRenderer;
class vtkPointsProjectedHull;
class vtkCell;

class VTK_EXPORT vtkPlanesIntersection : public vtkPlanes
{
    vtkTypeRevisionMacro(vtkPlanesIntersection, vtkPlanes);

public:
    void PrintSelf(ostream& os, vtkIndent indent);

    static vtkPlanesIntersection *New();

    // Description:
    //   It helps if you know the vertices of the convex region.
    //   If you don't, we will calculate them.  Region vertices
    //   are 3-tuples.

    void SetRegionVertices(vtkPoints *pts);
    void SetRegionVertices(float *v, int nvertices);
    void SetRegionVertices(double *v, int nvertices);
    int GetNumRegionVertices();
    int GetRegionVertices(float *v, int nvertices);
    int GetRegionVertices(double *v, int nvertices);

    // Description:
    //   Return 1 if the axis aligned box defined by R intersects
    //   the region defined by the planes, or 0 otherwise.
    
    int IntersectsRegion(vtkPoints *R);

    // Description:
    //   A convenience function provided by this class, returns
    //   1 if the polygon defined in pts intersects the bounding
    //   box defined in bounds, 0 otherwise.
    //
    //   The points must define a planar polygon.

    static int PolygonIntersectsBBox(float bounds[6], vtkPoints *pts);

    // Description:
    //   Another convenience function provided by this class, it returns
    //   the vtkPlanesIntersection object representing the view frustum
    //   created by projecting a sub-rectangle of the view
    //   plane.  Sub-rectangle range is [-1,1],[-1,1].
    //
    //   If you want the entire view plane, you can instead use
    //   vtkCamera::GetFrustumPlanes() to get the planes, then
    //   create a vtkPlanesIntersection object and
    //   use vtkPlanes::SetFrustumPlanes() to set the planes.

    static vtkPlanesIntersection *ConvertFrustumToWorld(vtkRenderer *ren,
                      double x0, double x1, double y0, double y1);

    // Description:
    //   Another convenience function provided by this class, returns
    //   the vtkPlanesIntersection object representing a 3D
    //   cell.  The point IDs for each face must be given in
    //   counter-clockwise order from the outside of the cell.

    static vtkPlanesIntersection *Convert3DCell(vtkCell *cell);

protected:

    vtkPlanesIntersection();
    ~vtkPlanesIntersection();

private:

  int IntersectsBoundingBox(vtkPoints *R);
  int EnclosesBoundingBox(vtkPoints *R);
  int EvaluateFacePlane(int plane, vtkPoints *R);
  int IntersectsProjection(vtkPoints *R, int direction);

  void SetPlaneEquations();
  void ComputeRegionVertices();

  const void planesMatrix(int p1, int p2, int p3, double M[3][3]);
  const int duplicate(double testv[3]);
  const void planesRHS(int p1, int p2, int p3, double r[3]);
  const int outsideRegion(double v[3]);

  static double EvaluatePlaneEquation(double *x, double *p);
  static void PlaneEquation(double *n, double *x, double *p);
  static void computeNormal(float *p1, float *p2, float *p3, float normal[3]);
  static int goodNormal(float *n);
  static void pointOnPlane(float a, float b, float c, float d, float pt[3]);
  static float sensibleZcoordinate(vtkRenderer *ren);
    
  static vtkPlanesIntersection *ConvertFrustumToWorldPerspective(
                      vtkRenderer *ren,
                      double xmin, double xmax, double ymin, double ymax);
  static vtkPlanesIntersection *ConvertFrustumToWorldParallel(
                      vtkRenderer *ren,
                      double xmin, double xmax, double ymin, double ymax);
    
    
  static int Invert3x3(double M[3][3]);

  // plane equations
  double *Plane;

  // vertices of convex regions enclosed by the planes, also
  //    the ccw hull of that region projected in 3 orthog. directions
  vtkPointsProjectedHull *regionPts;
};
#endif


