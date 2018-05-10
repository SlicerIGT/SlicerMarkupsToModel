#ifndef __vtkCurveGenerator_h
#define __vtkCurveGenerator_h

// vtk includes
#include <vtkSetGet.h>
#include <vtkObject.h>
#include <vtkTimeStamp.h>
#include <vtkSmartPointer.h>

class vtkPoints;
class vtkDoubleArray;
class vtkParametricFunction;
class vtkParametricSpline;
class vtkSpline;

// export
#include "vtkSlicerMarkupsToModelModuleLogicExport.h"

// A class to generate curves from input polydata
class VTK_SLICER_MARKUPSTOMODEL_MODULE_LOGIC_EXPORT vtkCurveGenerator : public vtkObject
{
  public:
    vtkTypeMacro( vtkCurveGenerator, vtkObject );
    static vtkCurveGenerator* New();

    void PrintSelf( ostream &os, vtkIndent indent ) VTK_OVERRIDE;

    // This indicates whether the curve should loop back in on itself,
    // connecting the last point back to the first point (disabled by default).
    vtkSetMacro( CurveIsLoop, bool );
    vtkGetMacro( CurveIsLoop, bool );

    // type of curve to generate
    enum
    {
      CURVE_TYPE_LINEAR_SPLINE = 0, // Curve interpolates between input points with straight lines
      CURVE_TYPE_CARDINAL_SPLINE, // Curve interpolates between input points smoothly
      CURVE_TYPE_KOCHANEK_SPLINE, // Curve interpolates between input points smoothly, generalized
      CURVE_TYPE_POLYNOMIAL_GLOBAL_LEAST_SQUARES, // Curve approximates the input points with a global least-squares polynomial fit
      CURVE_TYPE_LAST // Valid types go above this line
    };
    vtkGetMacro( CurveType, int );
    vtkSetMacro( CurveType, int );
    std::string GetCurveTypeAsString();
    void SetCurveTypeToLinearSpline() { this->SetCurveType( CURVE_TYPE_LINEAR_SPLINE ); }
    void SetCurveTypeToCardinalSpline() { this->SetCurveType( CURVE_TYPE_CARDINAL_SPLINE ); }
    void SetCurveTypeToKochanekSpline() { this->SetCurveType( CURVE_TYPE_KOCHANEK_SPLINE ); }
    void SetCurveTypeToPolynomialGlobalLeastSquares() { this->SetCurveType( CURVE_TYPE_POLYNOMIAL_GLOBAL_LEAST_SQUARES ); }
    bool IsCurveTypeApproximating();
    bool IsCurveTypeInterpolating();

    // Sample an *interpolating* curve this many times per segment (pair of points in sequence). Range 1 and up. Default 5.
    vtkSetMacro( NumberOfPointsPerInterpolatingSegment, int );
    vtkGetMacro( NumberOfPointsPerInterpolatingSegment, int );

    // Bias of derivative toward previous point (negative value) or next point. Range -1 to 1. Default 0.
    vtkGetMacro( KochanekBias, double );
    vtkSetMacro( KochanekBias, double );

    // Make the curve sharper( negative value) or smoother (positive value). Range -1 to 1. Default 0.
    vtkGetMacro( KochanekContinuity, double );
    vtkSetMacro( KochanekContinuity, double );

    // How quickly the curve turns, higher values like tightening an elastic. Range -1 to 1. Default 0.
    vtkGetMacro( KochanekTension, double );
    vtkSetMacro( KochanekTension, double );

    // Make the ends of the curve 'straighter' by copying derivative of the nearest point. Default false.
    vtkGetMacro( KochanekEndsCopyNearestDerivatives, bool );
    vtkSetMacro( KochanekEndsCopyNearestDerivatives, bool );

    // Set the order of the polynomials for fitting. Range 1 to 9 (equation becomes unstable from 9 upward). Default 1.
    vtkGetMacro( PolynomialOrder, int );
    vtkSetMacro( PolynomialOrder, int );

    // Set the sorting method for points in a polynomial.
    enum {
      SORTING_METHOD_INDEX = 0,
      SORTING_METHOD_MINIMUM_SPANNING_TREE_POSITION,
      SORTING_METHOD_LAST // valid types should be written above this line
    };
    vtkGetMacro( PolynomialPointSortingMethod, int );
    vtkSetMacro( PolynomialPointSortingMethod, int );
    void SetPolynomialPointSortingMethodToIndex() { this->SetPolynomialPointSortingMethod( vtkCurveGenerator::SORTING_METHOD_INDEX ); }
    void SetPolynomialPointSortingMethodToMinimumSpanningTreePosition() { this->SetPolynomialPointSortingMethod( vtkCurveGenerator::SORTING_METHOD_MINIMUM_SPANNING_TREE_POSITION ); }

    // The points used to fit the curve
    void SetInputPoints( vtkPoints* );
    vtkPoints* GetInputPoints();

    // output sampled points
    vtkPoints* GetOutputPoints();

    // logic
    void Update();

  protected:
    vtkCurveGenerator();
    ~vtkCurveGenerator();

  private:
    // inputs
    vtkSmartPointer< vtkPoints > InputPoints;
    vtkSmartPointer< vtkDoubleArray > InputParameters;
    // Wednesday May 9, 2018 TODO
    // InputParameters is currently computed by this class depending on the 
    // value of PolynomialPointSortingMethod, and is only supported for polynomials.
    // In the future this could be expanded to support splines, and to allow 
    // the user to specify their own parameters (make a SetInputParameters function)

    // input parameters
    int NumberOfPointsPerInterpolatingSegment;
    int CurveType;
    bool CurveIsLoop;
    double KochanekBias;
    double KochanekContinuity;
    double KochanekTension;
    bool KochanekEndsCopyNearestDerivatives;
    int PolynomialOrder;
    int PolynomialPointSortingMethod;

    // internal storage
    vtkSmartPointer< vtkParametricFunction > ParametricFunction;

    // output
    vtkSmartPointer< vtkPoints > OutputPoints;
    vtkTimeStamp OutputChangedTime;

    // logic
    void SetParametricFunctionToSpline( vtkSpline* xSpline, vtkSpline* ySpline, vtkSpline* zSpline );
    void SetParametricFunctionToLinearSpline();
    void SetParametricFunctionToCardinalSpline();
    void SetParametricFunctionToKochanekSpline();
    void SetParametricFunctionToPolynomial();
    bool UpdateNeeded();
    void GeneratePoints();

    static void SortByIndex( vtkPoints*, vtkDoubleArray* );
    static void SortByMinimumSpanningTreePosition( vtkPoints*, vtkDoubleArray* );

    vtkCurveGenerator( const vtkCurveGenerator& ); // Not implemented.
    void operator=( const vtkCurveGenerator& ); // Not implemented.
};

#endif
