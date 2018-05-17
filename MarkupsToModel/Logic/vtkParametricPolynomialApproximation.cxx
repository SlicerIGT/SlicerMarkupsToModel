/*=========================================================================

  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkParametricPolynomialApproximation.h"

#include <vtkDoubleArray.h>
#include <vtkMath.h>
#include <vtkObjectFactory.h>
#include <vtkPoints.h>
#include <vtkSmartPointer.h>

#include <vector>
#include <set>

vtkStandardNewMacro( vtkParametricPolynomialApproximation );

//----------------------------------------------------------------------------
vtkParametricPolynomialApproximation::vtkParametricPolynomialApproximation()
{
  this->MinimumU = 0;
  this->MaximumU = 1.0;
  this->JoinU = 0;

  this->Points = NULL;
  this->Parameters = NULL;
  this->PolynomialOrder = 1;
  
  this->Coefficients = NULL;
}

//----------------------------------------------------------------------------
vtkParametricPolynomialApproximation::~vtkParametricPolynomialApproximation() {}

//----------------------------------------------------------------------------
void vtkParametricPolynomialApproximation::SetPoints( vtkPoints* points )
{
  this->Points = points;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkParametricPolynomialApproximation::SetParameters( vtkDoubleArray* array )
{
  this->Parameters = array;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkParametricPolynomialApproximation::Evaluate( double u[ 3 ], double outputPoint[ 3 ], double* )
{
  // Set default value
  outputPoint[ 0 ] = outputPoint[ 1 ] = outputPoint[ 2 ] = 0;

  // make sure everything has been set up
  if ( this->ComputeCoefficientsNeeded() )
  {
    this->ComputeCoefficients();
  }

  // error cases, just return
  if ( this->Coefficients == NULL || this->Coefficients->GetNumberOfTuples() == 0 )
  {
    vtkErrorMacro( "Polynomial coefficients were not computed. Returning without evaluating." );
    return;
  }

  double sampleParameter = vtkMath::ClampValue< double >( u[ 0 ], 0.0, 1.0 );

  const int numberOfDimensions = 3;
  int numberOfCoefficients = this->Coefficients->GetNumberOfComponents();
  for ( int dimensionIndex = 0; dimensionIndex < numberOfDimensions; dimensionIndex++ )
  {
    for ( int coefficientIndex = 0; coefficientIndex < numberOfCoefficients; coefficientIndex++ )
    {
      double coefficient = this->Coefficients->GetComponent( dimensionIndex, coefficientIndex );
      double sampleParameterToExponent = std::pow( sampleParameter, coefficientIndex );
      outputPoint[ dimensionIndex ] += coefficient * sampleParameterToExponent;
    }
  }
}

//----------------------------------------------------------------------------
double vtkParametricPolynomialApproximation::EvaluateScalar( double u[ 3 ], double*, double* )
{
  return u[ 0 ];
}

//----------------------------------------------------------------------------
void vtkParametricPolynomialApproximation::ComputeCoefficients()
{
  this->Coefficients = NULL; // this indicates that the coefficients have not been computed (yet)

  if ( this->Points == NULL || this->Points->GetNumberOfPoints() == 0 )
  {
    vtkErrorMacro( "Points are missing. Cannot compute coefficients." );
    return;
  }

  if ( this->Parameters == NULL || this->Parameters->GetNumberOfTuples() == 0 )
  {
    vtkErrorMacro( "Parameters are missing. Cannot compute coefficients." );
    return;
  }

  int numberOfPoints = this->Points->GetNumberOfPoints();
  int numberOfParameters = this->Parameters->GetNumberOfTuples();
  if ( numberOfPoints != numberOfParameters )
  {
    vtkErrorMacro( "Need equal number of parameters and points. Got " << numberOfParameters << " and " << numberOfPoints << ", respectively. Cannot compute coefficients." );
    return;
  }

  this->Coefficients = vtkSmartPointer< vtkDoubleArray >::New();
  this->FitLeastSquaresPolynomials( this->Parameters, this->Points, this->PolynomialOrder, this->Coefficients );
}

//------------------------------------------------------------------------------
bool vtkParametricPolynomialApproximation::ComputeCoefficientsNeeded()
{
  // assume that if anything is null, then the user intends for everything to be computed
  // in normal use, none of these should be null
  if ( this->Coefficients == NULL || this->Points == NULL || this->Parameters == NULL )
  {
    return true;
  }

  vtkMTimeType coefficientsModifiedTime = this->Coefficients->GetMTime();
  vtkMTimeType approximatorModifiedTime = this->GetMTime();
  if ( approximatorModifiedTime > coefficientsModifiedTime )
  {
    return true;
  }

  vtkMTimeType pointsModifiedTime = this->Points->GetMTime();
  if ( pointsModifiedTime > coefficientsModifiedTime )
  {
    return true;
  }

  vtkMTimeType parametersModifiedTime = this->Parameters->GetMTime();
  if ( parametersModifiedTime > coefficientsModifiedTime )
  {
    return true;
  }
  
  return false;
}

//------------------------------------------------------------------------------
// This function formats the data so it works with vtkMath::SolveLeastSquares.
// TODO: Make a weighted version of this, take a list of weights (length same as number of points and parameters),
// then multiply each dependent and independent value by the corresponding weight for the point
void vtkParametricPolynomialApproximation::FitLeastSquaresPolynomials( vtkDoubleArray* parameters, vtkPoints* points, int polynomialOrder, vtkDoubleArray* coefficients )
{
  // The system of equations using high-order polynomials is not well-conditioned.
  // The vtkMath implementation will usually abort with polynomial orders higher than 9.
  // Since there is also numerical instability, we decide to limit the polynomial order to 6.
  // If order higher than 6 is needed on a global fit, then another algorithm should be considered anyway.
  // If at some point we want to add support for higher order polynomials, then here are two options (from Andras):
  // 1. VNL. While the VNL code is more sophisticated, and I guess also more stable, you would probably need to
  //    limit the number of samples and normalize data that you pass to the LSQR solver to be able to compute 
  //    higher-order fits (see for example this page for related discussion:
  //    http://digital.ni.com/public.nsf/allkb/45C2016C23B3B0298525645F0073B828). 
  //    See an example how VNL is used in Plus:
  //    https://github.com/PlusToolkit/PlusLib/blob/master/src/PlusCommon/PlusMath.cxx#L111
  // 2. Mathematica uses different basis functions for polynomial fitting (shifted Chebyshev polynomials) instead 
  //    of basis functions that are simple powers of a variable to make the fitting more robust (the source code
  //    is available here: http://library.wolfram.com/infocenter/MathSource/6780/).
  const int maximumPolynomialOrder = 6;
  if ( polynomialOrder > maximumPolynomialOrder )
  {
    vtkGenericWarningMacro( "Desired polynomial order " << polynomialOrder << " is not supported. "
      << "Maximum supported order is " << maximumPolynomialOrder << ". "
      << "Will attempt to create polynomial order " << maximumPolynomialOrder << " instead." );
    polynomialOrder = maximumPolynomialOrder;
  }
  const int minimumPolynomialOrder = 0; // It's a pretty weird input, but it does work. Just creates an average.
  if ( polynomialOrder < minimumPolynomialOrder )
  {
    vtkGenericWarningMacro( "Desired polynomial order " << polynomialOrder << " is not supported. "
      << "Minimum supported order is " << minimumPolynomialOrder << ". "
      << "Will attempt to create constant average instead." );
    polynomialOrder = minimumPolynomialOrder;
  }

  int numberOfPoints = points->GetNumberOfPoints();

  // determine number of coefficients for this polynomial
  std::set< double > uniqueParameters;
  for ( int pointIndex = 0; pointIndex < numberOfPoints; pointIndex++ )
  {
    double parameterValue = parameters->GetValue( pointIndex );
    uniqueParameters.insert( parameterValue ); // set cannot contain duplicates
  }
  int numberOfUniqueParameters = uniqueParameters.size();
  int numberOfCoefficients = polynomialOrder + 1;
  if ( numberOfUniqueParameters < numberOfCoefficients )
  {
    // must reduce the order of polynomial according to the amount of information is available
    numberOfCoefficients = numberOfUniqueParameters;
  }

  // independent values (parameter along the curve)
  int numIndependentValues = numberOfPoints * numberOfCoefficients;
  std::vector< double > independentValues( numIndependentValues, 0.0 );
  for ( int coefficientIndex = 0; coefficientIndex < numberOfCoefficients; coefficientIndex++ )
  {
    for ( int pointIndex = 0; pointIndex < numberOfPoints; pointIndex++ )
    {
      double parameterValue = parameters->GetValue( pointIndex );
      double independentValue = std::pow( parameterValue, coefficientIndex );
      independentValues[ pointIndex * numberOfCoefficients + coefficientIndex ] = independentValue;
    }
  }
  std::vector< double* > independentMatrix( numberOfPoints, NULL );
  for ( int pointIndex = 0; pointIndex < numberOfPoints; pointIndex++ )
  {
    independentMatrix[ pointIndex ] = &( independentValues[ pointIndex * numberOfCoefficients ] );
  }
  double** independentMatrixPtr = &( independentMatrix[ 0 ] );

  // dependent values
  const int numberOfDimensions = 3;
  int numDependentValues = numberOfPoints * numberOfDimensions;
  std::vector< double > dependentValues( numDependentValues, 0.0 );
  for ( int pointIndex = 0; pointIndex < numberOfPoints; pointIndex++ )
  {
    double* currentPoint = points->GetPoint( pointIndex );
    for ( int dimensionIndex = 0; dimensionIndex < numberOfDimensions; dimensionIndex++ )
    {
      double value = currentPoint[ dimensionIndex ];
      dependentValues[ pointIndex * numberOfDimensions + dimensionIndex ] = value;
    }
  }
  std::vector< double* > dependentMatrix( numberOfPoints, NULL );
  for ( int pointIndex = 0; pointIndex < numberOfPoints; pointIndex++ )
  {
    dependentMatrix[ pointIndex ] = &( dependentValues[ pointIndex * numberOfDimensions ] );
  }
  double** dependentMatrixPtr = &( dependentMatrix[ 0 ] );

  // solution to least squares
  std::vector< double > coefficientValues( numberOfDimensions * numberOfCoefficients, 0.0 );
  std::vector< double* > coefficientMatrix( numberOfCoefficients, NULL );
  for ( int coefficientIndex = 0; coefficientIndex < numberOfCoefficients; coefficientIndex++ )
  {
    coefficientMatrix[ coefficientIndex ] = &( coefficientValues[ coefficientIndex * numberOfDimensions ] );
  }
  double** coefficientMatrixPtr = &( coefficientMatrix[ 0 ] ); // the solution

  // Input the forumulation into vtkMath::SolveLeastSquares
  vtkMath::SolveLeastSquares( numberOfPoints, independentMatrixPtr, numberOfCoefficients, dependentMatrixPtr, numberOfDimensions, coefficientMatrixPtr );

  // Store result in the appropriate variable
  coefficients->Reset();
  coefficients->SetNumberOfComponents( numberOfCoefficients ); // must be set before number of tuples
  coefficients->SetNumberOfTuples( numberOfDimensions );
  for ( int dimensionIndex = 0; dimensionIndex < numberOfDimensions; dimensionIndex++ )
  {
    for ( int coefficientIndex = 0; coefficientIndex < numberOfCoefficients; coefficientIndex++ )
    {
      double coefficient = coefficientValues[ coefficientIndex * numberOfDimensions + dimensionIndex ];
      coefficients->SetComponent( dimensionIndex, coefficientIndex, coefficient );
    }
  }
}

//----------------------------------------------------------------------------
void vtkParametricPolynomialApproximation::PrintSelf( ostream& os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );

  os << indent << "Parameters: ";
  if ( this->Parameters != NULL )
  {
    os << this->Parameters << "\n";
  }
  else
  {
    os << "(none)\n";
  }

  os << indent << "Points: ";
  if ( this->Points != NULL )
  {
    os << this->Points << "\n";
  }
  else
  {
    os << "(none)\n";
  }

  os << indent << this->PolynomialOrder << "\n";

  os << indent << "Coefficients: ";
  if ( this->Coefficients != NULL )
  {
    os << this->Coefficients << "\n";
  }
  else
  {
    os << "(none)\n";
  }
}
