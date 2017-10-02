#ifndef __vtkCreateClosedSurfaceUtil_h
#define __vtkCreateClosedSurfaceUtil_h

#include "vtkMRMLMarkupsToModelNode.h"

// vtk includes
#include <vtkMatrix4x4.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>

class vtkCreateClosedSurfaceUtil : public vtkObject
{
  public:
    enum PointArrangement
    {
      POINT_ARRANGEMENT_SINGULAR = 0,
      POINT_ARRANGEMENT_LINEAR,
      POINT_ARRANGEMENT_PLANAR,
      POINT_ARRANGEMENT_NONPLANAR,
      POINT_ARRANGEMENT_LAST // do not set to this type, insert valid types above this line
    };

    // Generates the closed surface from the points using vtkDelaunay3D.
    static bool GenerateCloseSurfaceModel(vtkPoints* points, vtkPolyData* outputPolyData, double delaunayAlpha, bool smoothing, bool forceConvex);

  private:
    // Prevent declarations of this class
    vtkCreateClosedSurfaceUtil() {};
    ~vtkCreateClosedSurfaceUtil() {};

    static const double COMPARE_TO_ZERO_TOLERANCE = 0.0001;

    // Compute the best fit plane through the points, as well as the major and minor axes which describe variation in points.
    static void ComputeTransformMatrixFromBoundingAxes(vtkPoints* points, vtkMatrix4x4* transformFromBoundingAxes);

    // Compute the range of points along the specified axes (total lengths along which points appear)
    static void ComputeTransformedExtentRanges(vtkPoints* points, vtkMatrix4x4* transformMatrix, double outputExtentRanges[3]);

    // Compute the amount to extrude surfaces when closed surface is linear or planar.
    static double ComputeSurfaceExtrusionAmount(const double extents[3]);

    // Find out what kind of arrangment the points are in (see PointArrangementEnum above).
    // If the arrangement is planar, stores the normal of the best fit plane in planeNormal.
    // If the arrangement is linear, stores the axis of the best fit line in lineAxis.
    static PointArrangement ComputePointArrangement(const double smallestBoundingExtentRanges[3]);

    // helper utility functions
    static void SetNthColumnInMatrix(vtkMatrix4x4* matrix, int n, const double axis[3]);
    static void GetNthColumnInMatrix(vtkMatrix4x4* matrix, int n, double outputAxis[3]);
};

#endif
