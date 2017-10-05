#ifndef __vtkCreateCurveUtil_h
#define __vtkCreateCurveUtil_h

#include "vtkMRMLMarkupsToModelNode.h"

// vtk includes
#include <vtkCardinalSpline.h>
#include <vtkKochanekSpline.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>

class vtkCreateCurveUtil : public vtkObject
{
  public:
    static const bool TUBE_LOOP_DEFAULT = false;
    static const double TUBE_RADIUS_DEFAULT = 1.0;
    static const int TUBE_NUMBER_OF_SIDES_DEFAULT = 8;
    static const int TUBE_SEGMENTS_BETWEEN_CONTROL_POINTS_DEFAULT = 5;
    static const int POLYNOMIAL_ORDER_DEFAULT = 3;
    static const double KOCHANEK_BIAS_DEFAULT = 0.0;
    static const double KOCHANEK_CONTINUITY_DEFAULT = 0.0;
    static const double KOCHANEK_TENSION_DEFAULT = 0.0;
    static const double KOCHANEK_ENDS_COPY_NEAREST_DERIVATIVE_DEFAULT = 0.0;

    // Generates the linear curve model connecting linear tubes from each markup.
    static void GenerateLinearCurveModel(vtkPoints* controlPoints, vtkPolyData* outputTubePolyData,
      double tubeRadius=TUBE_RADIUS_DEFAULT, int tubeNumberOfSides=TUBE_NUMBER_OF_SIDES_DEFAULT,
      int tubeSegmentsBetweenControlPoints=TUBE_SEGMENTS_BETWEEN_CONTROL_POINTS_DEFAULT, bool tubeLoop=TUBE_LOOP_DEFAULT);

    // Generates Cardinal Spline curve model.
    static void GenerateCardinalCurveModel(vtkPoints* controlPoints, vtkPolyData* outputTubePolyData,
      double tubeRadius=TUBE_RADIUS_DEFAULT, int tubeNumberOfSides=TUBE_NUMBER_OF_SIDES_DEFAULT,
      int tubeSegmentsBetweenControlPoints=TUBE_SEGMENTS_BETWEEN_CONTROL_POINTS_DEFAULT, bool tubeLoop=TUBE_LOOP_DEFAULT);

    // Generates Kochanek Spline curve model.
    static void GenerateKochanekCurveModel(vtkPoints* controlPoints, vtkPolyData* outputTubePolyData,
      double tubeRadius=TUBE_RADIUS_DEFAULT, int tubeNumberOfSides=TUBE_NUMBER_OF_SIDES_DEFAULT,
      int tubeSegmentsBetweenControlPoints=TUBE_SEGMENTS_BETWEEN_CONTROL_POINTS_DEFAULT, bool tubeLoop=TUBE_LOOP_DEFAULT,
      double kochanekBias=KOCHANEK_BIAS_DEFAULT, double kochanekContinuity=KOCHANEK_CONTINUITY_DEFAULT,
      double kochanekTension=KOCHANEK_TENSION_DEFAULT, bool kochanekEndsCopyNearestDerivatives=KOCHANEK_ENDS_COPY_NEAREST_DERIVATIVE_DEFAULT);

    // Generates a polynomial curve model.
    static void GeneratePolynomialCurveModel(vtkPoints* controlPoints, vtkPolyData* outputPolyData,
      double tubeRadius=TUBE_RADIUS_DEFAULT, int tubeNumberOfSides=TUBE_NUMBER_OF_SIDES_DEFAULT,
      int tubeSegmentsBetweenControlPoints=TUBE_SEGMENTS_BETWEEN_CONTROL_POINTS_DEFAULT, bool tubeLoop=TUBE_LOOP_DEFAULT,
      int polynomialOrder=POLYNOMIAL_ORDER_DEFAULT, vtkDoubleArray* markupsPointsParameters=NULL);

    // Assign parameter values to points based on their position in the markups list (good for ordered point sets)
    static void ComputePointParametersRawIndices(vtkPoints* controlPoints, vtkDoubleArray* outputPointParameters);

    // Assign parameter values to points based on their position in a minimum spanning tree between the two farthest points (good for unordered point sets)
    static void ComputePointParametersMinimumSpanningTree(vtkPoints* controlPoints, vtkDoubleArray* outputPointParameters);

    // Convert the MRML Markups node to a vtkPoints object, with option to remove duplicates
    static void MarkupsToPoints(vtkMRMLMarkupsFiducialNode* markupsNode, vtkPoints* outputPoints, bool removeDuplicates);

  private:
    // Prevent declarations of this class
    vtkCreateCurveUtil() {};
    ~vtkCreateCurveUtil() {};
    
    static const int NUMBER_OF_LINE_POINTS_MIN = 2;
    static const double CLEAN_POLYDATA_TOLERANCE_MM = 0.01;

    static void AllocateCurvePoints(vtkPoints* controlPoints, vtkPoints* outputPoints, int tubeSegmentsBetweenControlPoints, bool tubeLoop);
    static void CloseLoop(vtkPoints* outputPoints);
    static void GetTubePolyDataFromPoints(vtkPoints* pointsToConnect, vtkPolyData* outputTube, double tubeRadius, int tubeNumberOfSides);
    static void SetKochanekSplineParameters(vtkPoints* controlPoints, vtkKochanekSpline* splineX, vtkKochanekSpline* splineY, vtkKochanekSpline* splineZ, bool tubeLoop, double kochanekBias, double kochanekContinuity, double kochanekTension, bool kochanekEndsCopyNearestDerivatives);
    static void SetCardinalSplineParameters(vtkPoints* controlPoints, vtkCardinalSpline* splineX, vtkCardinalSpline* splineY, vtkCardinalSpline* splineZ, bool tubeLoop);
};

#endif
