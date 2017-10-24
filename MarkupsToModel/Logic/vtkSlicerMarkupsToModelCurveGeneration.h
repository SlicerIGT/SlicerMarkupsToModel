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

    // Generates the linear curve model connecting linear tubes from each markup.
    static void GenerateLinearCurveModel( vtkPoints* controlPoints, vtkPolyData* outputTubePolyData,
      double tubeRadius=vtkSlicerMarkupsToModelCurveGeneration::TUBE_RADIUS_DEFAULT,
      int tubeNumberOfSides=vtkSlicerMarkupsToModelCurveGeneration::TUBE_NUMBER_OF_SIDES_DEFAULT,
      int tubeSegmentsBetweenControlPoints=vtkSlicerMarkupsToModelCurveGeneration::TUBE_SEGMENTS_BETWEEN_CONTROL_POINTS_DEFAULT,
      bool tubeLoop=vtkSlicerMarkupsToModelCurveGeneration::TUBE_LOOP_DEFAULT );

    // Generates Cardinal Spline curve model.
    static void GenerateCardinalCurveModel( vtkPoints* controlPoints, vtkPolyData* outputTubePolyData,
      double tubeRadius=vtkSlicerMarkupsToModelCurveGeneration::TUBE_RADIUS_DEFAULT, 
      int tubeNumberOfSides=vtkSlicerMarkupsToModelCurveGeneration::TUBE_NUMBER_OF_SIDES_DEFAULT,
      int tubeSegmentsBetweenControlPoints=vtkSlicerMarkupsToModelCurveGeneration::TUBE_SEGMENTS_BETWEEN_CONTROL_POINTS_DEFAULT,
      bool tubeLoop=vtkSlicerMarkupsToModelCurveGeneration::TUBE_LOOP_DEFAULT );

    // Generates Kochanek Spline curve model.
    static void GenerateKochanekCurveModel( vtkPoints* controlPoints, vtkPolyData* outputTubePolyData,
      double tubeRadius=vtkSlicerMarkupsToModelCurveGeneration::TUBE_RADIUS_DEFAULT,
      int tubeNumberOfSides=vtkSlicerMarkupsToModelCurveGeneration::TUBE_NUMBER_OF_SIDES_DEFAULT,
      int tubeSegmentsBetweenControlPoints=vtkSlicerMarkupsToModelCurveGeneration::TUBE_SEGMENTS_BETWEEN_CONTROL_POINTS_DEFAULT,
      bool tubeLoop=vtkSlicerMarkupsToModelCurveGeneration::TUBE_LOOP_DEFAULT,
      double kochanekBias=vtkSlicerMarkupsToModelCurveGeneration::KOCHANEK_BIAS_DEFAULT,
      double kochanekContinuity=vtkSlicerMarkupsToModelCurveGeneration::KOCHANEK_CONTINUITY_DEFAULT,
      double kochanekTension=vtkSlicerMarkupsToModelCurveGeneration::KOCHANEK_TENSION_DEFAULT,
      bool kochanekEndsCopyNearestDerivatives=vtkSlicerMarkupsToModelCurveGeneration::KOCHANEK_ENDS_COPY_NEAREST_DERIVATIVE_DEFAULT );

    // Generates a polynomial curve model.
    static void GeneratePolynomialCurveModel( vtkPoints* controlPoints, vtkPolyData* outputPolyData,
      double tubeRadius=vtkSlicerMarkupsToModelCurveGeneration::TUBE_RADIUS_DEFAULT,
      int tubeNumberOfSides=vtkSlicerMarkupsToModelCurveGeneration::TUBE_NUMBER_OF_SIDES_DEFAULT,
      int tubeSegmentsBetweenControlPoints=vtkSlicerMarkupsToModelCurveGeneration::TUBE_SEGMENTS_BETWEEN_CONTROL_POINTS_DEFAULT,
      bool tubeLoop=vtkSlicerMarkupsToModelCurveGeneration::TUBE_LOOP_DEFAULT,
      int polynomialOrder=vtkSlicerMarkupsToModelCurveGeneration::POLYNOMIAL_ORDER_DEFAULT,
      vtkDoubleArray* markupsPointsParameters=NULL );

    // Assign parameter values to points based on their position in the markups list (good for ordered point sets)
    static void ComputePointParametersRawIndices(vtkPoints* controlPoints, vtkDoubleArray* outputPointParameters);

    // Assign parameter values to points based on their position in a minimum spanning tree between the two farthest points (good for unordered point sets)
    static void ComputePointParametersMinimumSpanningTree(vtkPoints* controlPoints, vtkDoubleArray* outputPointParameters);

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
