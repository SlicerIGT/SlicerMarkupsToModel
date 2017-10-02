#ifndef __CreateCurveUtil_h
#define __CreateCurveUtil_h

#include "vtkMRMLMarkupsToModelNode.h"

#include <vtkCardinalSpline.h>
#include <vtkKochanekSpline.h>
#include <vtkPolyData.h>

namespace CreateCurveUtil
{
  static const int NUMBER_OF_LINE_POINTS_MIN = 2;
  static const double CLEAN_POLYDATA_TOLERANCE_MM = 0.01;

  // Generates the curve model from the markups connecting consecutive segments.
  // Each segment can be linear, cardinal or Kochanek Splines (described and implemented in UpdateOutputCurveModel, UpdateOutputLinearModel
  // and UpdateOutputHermiteSplineModel methods). Uses Tube radius and clean markups option from the module node.
  void UpdateOutputCurveModel(vtkMRMLMarkupsToModelNode* markupsToModelModuleNode, vtkPolyData* outputPolyData);
  bool UpdateOutputCurveModel(vtkMRMLMarkupsToModelNode* markupsToModelModuleNode);
  bool UpdateOutputCurveModel(vtkMRMLMarkupsFiducialNode* markupsNode, vtkMRMLModelNode* modelNode,
    int interpolationType, int pointParameterType, int tubeSegmentsBetweenControlPoints, bool tubeLoop, double tubeRadius, int tubeNumberOfSides,
    bool cleanMarkups, int polynomialOrder,
    double kochanekBias, double kochanekContinuity, double kochanekTension, bool kochanekEndsCopyNearestDerivatives);

  // Generates the linear curve model connecting linear tubes from each markup.
  void UpdateOutputLinearModel(vtkPoints* controlPoints, vtkPolyData* outputTubePolyData, int tubeSegmentsBetweenControlPoints, bool tubeLoop, double tubeRadius, int tubeNumberOfSides);
  // Generates Cardinal Spline curve model.
  void UpdateOutputCardinalSplineModel(vtkPoints* controlPoints, vtkPolyData* outputTubePolyData, double tubeRadius, int tubeNumberOfSides, int tubeSegmentsBetweenControlPoints, bool tubeLoop);
  // Generates Kochanek Spline curve model.
  void UpdateOutputKochanekSplineModel(vtkPoints* controlPoints, vtkPolyData* outputTubePolyData, int tubeSegmentsBetweenControlPoints, bool tubeLoop, double tubeRadius, int tubeNumberOfSides, double kochanekBias, double kochanekContinuity, double kochanekTension, bool kochanekEndsCopyNearestDerivatives);
  // Generates a polynomial curve model.
  void UpdateOutputPolynomialFitModel(vtkPoints* controlPoints, vtkDoubleArray* markupsPointsParameters, vtkPolyData* outputPolyData, int polynomialOrder, int tubeSegmentsBetweenControlPoints, bool tubeLoop, double tubeRadius, int tubeNumberOfSides);

  // Assign parameter values to points based on their position in the markups list (good for ordered point sets)
  void ComputePointParametersRawIndices(vtkPoints* controlPoints, vtkDoubleArray* markupsPointsParameters);
  // Assign parameter values to points based on their position in a minimum spanning tree between the two farthest points (good for unordered point sets)
  void ComputePointParametersMinimumSpanningTree(vtkPoints* controlPoints, vtkDoubleArray* markupsPointsParameters);

  
  void MarkupsToPoints(vtkMRMLMarkupsFiducialNode* markupsNode, vtkPoints* outputPoints, bool cleanMarkups);

  void AllocateCurvePoints(vtkPoints* controlPoints, vtkPoints* outputPoints, int tubeSegmentsBetweenControlPoints, bool tubeLoop);
  void CloseLoop(vtkPoints* outputPoints);
  void GetTubePolyDataFromPoints(vtkPoints* pointsToConnect, vtkPolyData* outputTube, double tubeRadius, int tubeNumberOfSides);
  void SetKochanekSplineParameters(vtkPoints* controlPoints, vtkKochanekSpline* splineX, vtkKochanekSpline* splineY, vtkKochanekSpline* splineZ, bool tubeLoop, double kochanekBias, double kochanekContinuity, double kochanekTension, bool kochanekEndsCopyNearestDerivatives);
  void SetCardinalSplineParameters(vtkPoints* controlPoints, vtkCardinalSpline* splineX, vtkCardinalSpline* splineY, vtkCardinalSpline* splineZ, bool tubeLoop);
} // namespace CreateCurveUtil

#endif
