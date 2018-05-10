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

  this->Points = vtkSmartPointer< vtkPoints >::New();
  this->Parameters = vtkSmartPointer< vtkDoubleArray >::New();
  this->PolynomialOrder = 1;
  
  this->Coefficients = vtkSmartPointer< vtkDoubleArray >::New();

  this->FitTime = 0;
}

//----------------------------------------------------------------------------
vtkParametricPolynomialApproximation::~vtkParametricPolynomialApproximation() {}

//----------------------------------------------------------------------------
void vtkParametricPolynomialApproximation::SetPoints( vtkPoints* points )
{
  if ( points == NULL )
  {
    vtkWarningMacro( "Points are null. Setting points to empty." );
    this->Points->Reset();
    return;
  }

  this->Points->DeepCopy( points );
}

//----------------------------------------------------------------------------
void vtkParametricPolynomialApproximation::SetParameters( vtkDoubleArray* array )
{
  if ( array == NULL )
  {
    vtkWarningMacro( "Parameters are null. Setting parameters to empty." );
    this->Parameters->Reset();
    return;
  }

  this->Parameters->DeepCopy( array );
}

//----------------------------------------------------------------------------
void vtkParametricPolynomialApproximation::Evaluate( double u[ 3 ], double outputPoint[ 3 ], double* )
{
  // Set default value
  outputPoint[ 0 ] = outputPoint[ 1 ] = outputPoint[ 2 ] = 0;

  // make sure everything has been set up
  if ( this->FitTime < this->GetMTime() )
  {
    if ( !this->Fit() )
    {
      return;
    }
  }

  double sampleParameter = u[ 0 ];
  sampleParameter = vtkMath::ClampValue< double >( sampleParameter, 0.0, 1.0 );

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
bool vtkParametricPolynomialApproximation::Fit()
{
  if ( !this->Points || this->Points->GetNumberOfPoints() == 0 )
  {
    vtkErrorMacro( "Points are missing. Aborting." );
    return false;
  }

  if ( !this->Parameters || this->Parameters->GetNumberOfTuples() == 0 )
  {
    vtkErrorMacro( "Parameters are missing. Aborting." );
    return false;
  }

  int numberOfPoints = this->Points->GetNumberOfPoints();
  int numberOfParameters = this->Parameters->GetNumberOfTuples();
  if ( numberOfPoints != numberOfParameters )
  {
    vtkErrorMacro( "Need equal number of parameters and points. Got " << numberOfParameters << " and " << numberOfPoints << ", respectively. Aborting." );
    return false;
  }

  this->FitLeastSquaresPolynomials( this->Parameters, this->Points, this->PolynomialOrder, this->Coefficients );

  this->FitTime = this->GetMTime();
  return true;
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
  //    https://app.assembla.com/spaces/plus/subversion/source/HEAD/trunk/PlusLib/src/PlusCommon/PlusMath.cxx#ln111
  // 2. Mathematica uses different basis functions for polynomial fitting (shifted Chebyshev polynomials) instead 
  //    of basis functions that are simple powers of a variable to make the fitting more robust (the source code
  //    is available here: http://library.wolfram.com/infocenter/MathSource/6780/).
  const int maximumPolynomialOrder = 6;
  if ( polynomialOrder > maximumPolynomialOrder )
  {
    vtkGenericWarningMacro( "Desired polynomial order " << polynomialOrder << " is not supported. "
      << "Maximum polynomial order is " << maximumPolynomialOrder << ". "
      << "Will attempt to create polynomial order " << maximumPolynomialOrder << " instead." );
    polynomialOrder = maximumPolynomialOrder;
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
  std::vector< double > independentValues( numIndependentValues );
  independentValues.assign( numIndependentValues, 0.0 );
  for ( int coefficientIndex = 0; coefficientIndex < numberOfCoefficients; coefficientIndex++ )
  {
    for ( int pointIndex = 0; pointIndex < numberOfPoints; pointIndex++ )
    {
      double parameterValue = parameters->GetValue( pointIndex );
      double independentValue = std::pow( parameterValue, coefficientIndex );
      independentValues[ pointIndex * numberOfCoefficients + coefficientIndex ] = independentValue;
    }
  }
  std::vector< double* > independentMatrix( numberOfPoints );
  independentMatrix.assign( numberOfPoints, NULL );
  for ( int pointIndex = 0; pointIndex < numberOfPoints; pointIndex++ )
  {
    independentMatrix[ pointIndex ] = &( independentValues[ pointIndex * numberOfCoefficients ] );
  }
  double** independentMatrixPtr = &( independentMatrix[ 0 ] );

  // dependent values
  const int numberOfDimensions = 3;
  int numDependentValues = numberOfPoints * numberOfDimensions;
  std::vector< double > dependentValues( numDependentValues );
  dependentValues.assign( numDependentValues, 0.0 );
  for ( int pointIndex = 0; pointIndex < numberOfPoints; pointIndex++ )
  {
    double* currentPoint = points->GetPoint( pointIndex );
    for ( int dimensionIndex = 0; dimensionIndex < numberOfDimensions; dimensionIndex++ )
    {
      double value = currentPoint[ dimensionIndex ];
      dependentValues[ pointIndex * numberOfDimensions + dimensionIndex ] = value;
    }
  }
  std::vector< double* > dependentMatrix( numberOfPoints );
  dependentMatrix.assign( numberOfPoints, NULL );
  for ( int pointIndex = 0; pointIndex < numberOfPoints; pointIndex++ )
  {
    dependentMatrix[ pointIndex ] = &( dependentValues[ pointIndex * numberOfDimensions ] );
  }
  double** dependentMatrixPtr = &( dependentMatrix[ 0 ] );

  // solution to least squares
  std::vector< double > coefficientValues( numberOfDimensions * numberOfCoefficients );
  std::vector< double* > coefficientMatrix( numberOfCoefficients );
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
  if ( this->Parameters )
  {
    os << this->Parameters << "\n";
  }
  else
  {
    os << "(none)\n";
  }

  os << indent << "Points: ";
  if ( this->Points )
  {
    os << this->Points << "\n";
  }
  else
  {
    os << "(none)\n";
  }

  os << indent << this->PolynomialOrder << "\n";

  os << indent << "Coefficients: ";
  if ( this->Coefficients )
  {
    os << this->Coefficients << "\n";
  }
  else
  {
    os << "(none)\n";
  }
}
