#ifndef __vtkSlicerMarkupsToModelCurveGeneration_h
#define __vtkSlicerMarkupsToModelCurveGeneration_h

#include "vtkMRMLMarkupsToModelNode.h"

// vtk includes
#include <vtkCardinalSpline.h>
#include <vtkKochanekSpline.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>

#include "vtkSlicerMarkupsToModelModuleLogicExport.h"

class VTK_SLICER_MARKUPSTOMODEL_MODULE_LOGIC_EXPORT vtkSlicerMarkupsToModelCurveGeneration : public vtkObject
{
  public:
    // standard vtk object methods
    vtkTypeMacro( vtkSlicerMarkupsToModelCurveGeneration, vtkObject );
    void PrintSelf( ostream& os, vtkIndent indent ) VTK_OVERRIDE;
    static vtkSlicerMarkupsToModelCurveGeneration *New();

    // constant default values
    // defined in implementation file due to VS2013 compile issue C2864
    static const bool TUBE_LOOP_DEFAULT;
    static const double TUBE_RADIUS_DEFAULT;
    static const int TUBE_NUMBER_OF_SIDES_DEFAULT;
    static const int TUBE_SEGMENTS_BETWEEN_CONTROL_POINTS_DEFAULT;
    static const int POLYNOMIAL_ORDER_DEFAULT;
    static const double KOCHANEK_BIAS_DEFAULT;
    static const double KOCHANEK_CONTINUITY_DEFAULT;
    static const double KOCHANEK_TENSION_DEFAULT;
    static const double KOCHANEK_ENDS_COPY_NEAREST_DERIVATIVE_DEFAULT;

    // note: each default value below needs to be preceded explicitly with "vtkSlicerMarkupsToModelCurveGeneration::"
    //       otherwise python wrapping breaks...

    // Generates the piecewise linear curve model connecting linear tubes from each markup.
    //   controlPoints - the curve will pass through each point defined here.
    //   outputTubePolyData - the curve will be stored as a tube mesh in this poly data.
    //   tubeRadius - the radius of the tube in outputTubePolyData.
    //   tubeNumberOfSides - The number of sides of the tube in outputTubePolyData (higher = smoother).
    //   tubeSegmentsBetweenControlPoints - The number of points sampled between each control point (higher = smoother).
    //   tubeLoop - Indicates whether the tube will loop back to the first point or not in outputTubePolyData.
    static void GeneratePiecewiseLinearCurveModel( vtkPoints* controlPoints, vtkPolyData* outputTubePolyData,
      double tubeRadius=vtkSlicerMarkupsToModelCurveGeneration::TUBE_RADIUS_DEFAULT,
      int tubeNumberOfSides=vtkSlicerMarkupsToModelCurveGeneration::TUBE_NUMBER_OF_SIDES_DEFAULT,
      int tubeSegmentsBetweenControlPoints=vtkSlicerMarkupsToModelCurveGeneration::TUBE_SEGMENTS_BETWEEN_CONTROL_POINTS_DEFAULT,
      bool tubeLoop=vtkSlicerMarkupsToModelCurveGeneration::TUBE_LOOP_DEFAULT );

    // Generates Cardinal Spline curve model.
    //   controlPoints - the curve will pass through each point defined here.
    //   outputTubePolyData - the curve will be stored as a tube mesh in this poly data.
    //   tubeRadius - the radius of the tube in outputTubePolyData.
    //   tubeNumberOfSides - The number of sides of the tube in outputTubePolyData (higher = smoother).
    //   tubeSegmentsBetweenControlPoints - The number of points sampled between each control point (higher = smoother).
    //   tubeLoop - Indicates whether the tube will loop back to the first point or not in outputTubePolyData.
    static void GenerateCardinalSplineCurveModel( vtkPoints* controlPoints, vtkPolyData* outputTubePolyData,
      double tubeRadius=vtkSlicerMarkupsToModelCurveGeneration::TUBE_RADIUS_DEFAULT, 
      int tubeNumberOfSides=vtkSlicerMarkupsToModelCurveGeneration::TUBE_NUMBER_OF_SIDES_DEFAULT,
      int tubeSegmentsBetweenControlPoints=vtkSlicerMarkupsToModelCurveGeneration::TUBE_SEGMENTS_BETWEEN_CONTROL_POINTS_DEFAULT,
      bool tubeLoop=vtkSlicerMarkupsToModelCurveGeneration::TUBE_LOOP_DEFAULT );

    // Generates Kochanek Spline curve model.
    //   controlPoints - the curve will pass through each point defined here.
    //   outputTubePolyData - the curve will be stored as a tube mesh in this poly data.
    //   tubeRadius - the radius of the tube in outputTubePolyData.
    //   tubeNumberOfSides - The number of sides of the tube in outputTubePolyData (higher = smoother).
    //   tubeSegmentsBetweenControlPoints - The number of points sampled between each control point (higher = smoother).
    //   tubeLoop - Indicates whether the tube will loop back to the first point or not in outputTubePolyData.
    //   kochanekBias - Alters the bias parameter for the kochanek spline.
    //   kochanekContinuity - Alters the bias parameter for the kochanek spline.
    //   kochanekTension - Alters the bias parameter for the kochanek spline.
    //   kochanekEndsCopyNearestDerivative - Copy the curvature on either end of the spline from the nearest point.
    static void GenerateKochanekSplineCurveModel( vtkPoints* controlPoints, vtkPolyData* outputTubePolyData,
      double tubeRadius=vtkSlicerMarkupsToModelCurveGeneration::TUBE_RADIUS_DEFAULT,
      int tubeNumberOfSides=vtkSlicerMarkupsToModelCurveGeneration::TUBE_NUMBER_OF_SIDES_DEFAULT,
      int tubeSegmentsBetweenControlPoints=vtkSlicerMarkupsToModelCurveGeneration::TUBE_SEGMENTS_BETWEEN_CONTROL_POINTS_DEFAULT,
      bool tubeLoop=vtkSlicerMarkupsToModelCurveGeneration::TUBE_LOOP_DEFAULT,
      double kochanekBias=vtkSlicerMarkupsToModelCurveGeneration::KOCHANEK_BIAS_DEFAULT,
      double kochanekContinuity=vtkSlicerMarkupsToModelCurveGeneration::KOCHANEK_CONTINUITY_DEFAULT,
      double kochanekTension=vtkSlicerMarkupsToModelCurveGeneration::KOCHANEK_TENSION_DEFAULT,
      bool kochanekEndsCopyNearestDerivatives=vtkSlicerMarkupsToModelCurveGeneration::KOCHANEK_ENDS_COPY_NEAREST_DERIVATIVE_DEFAULT );

    // Generates a polynomial curve model.
    //   controlPoints - the curve will be fit through these points.
    //   outputTubePolyData - the curve will be stored as a tube mesh in this poly data.
    //   tubeRadius - the radius of the tube in outputTubePolyData.
    //   tubeNumberOfSides - The number of sides of the tube in outputTubePolyData (higher = smoother).
    //   tubeSegmentsBetweenControlPoints - The number of points sampled between each control point (higher = smoother).
    //   tubeLoop - Indicates whether the tube will loop back to the first point or not in outputTubePolyData.
    //   polynomial - The order of polynomial to fit. Higher = fit the points better, but slower and risk of 'overfitting'
    //   markupsPointsParameters - Indicate the parameter (independent) values for fitting each point. See also:
    //     - ComputePointParametersFromIndices
    //     - ComputePointParametersFromMinimumSpanningTree
    static void GeneratePolynomialCurveModel( vtkPoints* points, vtkPolyData* outputPolyData,
      double tubeRadius=vtkSlicerMarkupsToModelCurveGeneration::TUBE_RADIUS_DEFAULT,
      int tubeNumberOfSides=vtkSlicerMarkupsToModelCurveGeneration::TUBE_NUMBER_OF_SIDES_DEFAULT,
      int tubeSegmentsBetweenControlPoints=vtkSlicerMarkupsToModelCurveGeneration::TUBE_SEGMENTS_BETWEEN_CONTROL_POINTS_DEFAULT,
      bool tubeLoop=vtkSlicerMarkupsToModelCurveGeneration::TUBE_LOOP_DEFAULT,
      int polynomialOrder=vtkSlicerMarkupsToModelCurveGeneration::POLYNOMIAL_ORDER_DEFAULT,
      vtkDoubleArray* markupsPointsParameters=NULL );

    // Assign parameter values to points based on their position in the markups list (good for ordered point sets)
    // Either ComputePointParametersFromIndices or ComputePointParametersFromMinimumSpanningTree should be used
    // before GeneratePolynomialCurve
    static void ComputePointParametersFromIndices( vtkPoints* points, vtkDoubleArray* outputPointParameters );

    // Assign parameter values to points based on their position in a minimum spanning tree between the two
    // farthest points (good for unordered point sets)
    // Parameters are assigned based on the length along the MST path connecting the two farthest points.
    // For points that branch off this path, the parameter will be copied from the branching point.
    // Either ComputePointParametersFromIndices or ComputePointParametersFromMinimumSpanningTree should be used
    // before GeneratePolynomialCurve
    static void ComputePointParametersFromMinimumSpanningTree( vtkPoints* points, vtkDoubleArray* outputPointParameters );

  protected:
    vtkSlicerMarkupsToModelCurveGeneration();
    ~vtkSlicerMarkupsToModelCurveGeneration();

  private:
    static void AllocateCurvePoints(vtkPoints* controlPoints, vtkPoints* outputPoints, int tubeSegmentsBetweenControlPoints, bool tubeLoop);
    static void CloseLoop(vtkPoints* outputPoints);
    static void GetTubePolyDataFromPoints(vtkPoints* pointsToConnect, vtkPolyData* outputTube, double tubeRadius, int tubeNumberOfSides);
    static void SetKochanekSplineParameters(vtkPoints* controlPoints, vtkKochanekSpline* splineX, vtkKochanekSpline* splineY, vtkKochanekSpline* splineZ, bool tubeLoop, double kochanekBias, double kochanekContinuity, double kochanekTension, bool kochanekEndsCopyNearestDerivatives);
    static void SetCardinalSplineParameters(vtkPoints* controlPoints, vtkCardinalSpline* splineX, vtkCardinalSpline* splineY, vtkCardinalSpline* splineZ, bool tubeLoop);

    // not used
    vtkSlicerMarkupsToModelCurveGeneration ( const vtkSlicerMarkupsToModelCurveGeneration& ) VTK_DELETE_FUNCTION;
    void operator= ( const vtkSlicerMarkupsToModelCurveGeneration& ) VTK_DELETE_FUNCTION;
};

#endif
