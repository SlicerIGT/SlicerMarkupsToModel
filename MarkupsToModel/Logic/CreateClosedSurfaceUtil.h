#ifndef __CreateClosedSurfaceUtil_h
#define __CreateClosedSurfaceUtil_h

#include "vtkMRMLMarkupsToModelNode.h"

#include <vtkMatrix4x4.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>

namespace CreateClosedSurfaceUtil
{
  static const double CLEAN_POLYDATA_TOLERANCE_MM = 0.01;
  static const double COMPARE_TO_ZERO_TOLERANCE = 0.0001;

  enum PointArrangement
  {
    POINT_ARRANGEMENT_SINGULAR = 0,
    POINT_ARRANGEMENT_LINEAR,
    POINT_ARRANGEMENT_PLANAR,
    POINT_ARRANGEMENT_NONPLANAR,
    POINT_ARRANGEMENT_LAST // do not set to this type, insert valid types above this line
  };

  // Generates the closed surface from the markups using vtkDelaunay3D. Uses Delanauy alpha value, subdivision filter and clean markups
  // options from the module node.
  void UpdateOutputCloseSurfaceModel(vtkMRMLMarkupsToModelNode* markupsToModelModuleNode, vtkPolyData* outputPolyData);
  bool UpdateOutputCloseSurfaceModel(vtkMRMLMarkupsToModelNode* markupsToModelModuleNode);
  bool UpdateOutputCloseSurfaceModel(vtkMRMLMarkupsFiducialNode* markupsNode, vtkMRMLModelNode* modelNode,
    bool cleanMarkups, double delaunayAlpha, bool smoothing, bool forceConvex);

  // Compute the best fit plane through the points, as well as the major and minor axes which describe variation in points.
  void ComputeTransformMatrixFromBoundingAxes(vtkPoints* points, vtkMatrix4x4* transformFromBoundingAxes);

  // Compute the range of points along the specified axes (total lengths along which points appear)
  void ComputeTransformedExtentRanges(vtkPoints* points, vtkMatrix4x4* transformMatrix, double outputExtentRanges[3]);

  // Compute the amount to extrude surfaces when closed surface is linear or planar.
  double ComputeSurfaceExtrusionAmount(const double extents[3]);

  // Find out what kind of arrangment the points are in (see PointArrangementEnum above).
  // If the arrangement is planar, stores the normal of the best fit plane in planeNormal.
  // If the arrangement is linear, stores the axis of the best fit line in lineAxis.
  PointArrangement ComputePointArrangement(const double smallestBoundingExtentRanges[3]);

  // helper utility functions
  void SetNthColumnInMatrix(vtkMatrix4x4* matrix, int n, const double axis[3]);
  void GetNthColumnInMatrix(vtkMatrix4x4* matrix, int n, double outputAxis[3]);
} // namespace CreateClosedSurfaceUtil

#endif
