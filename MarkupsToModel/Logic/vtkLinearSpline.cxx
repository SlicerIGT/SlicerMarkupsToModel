/*=========================================================================

  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkLinearSpline.h"

#include <vtkObjectFactory.h>
#include <vtkPiecewiseFunction.h>
#include <cassert>
#include <vector>

vtkStandardNewMacro( vtkLinearSpline );

//----------------------------------------------------------------------------
// Construct a Linear Spline.
vtkLinearSpline::vtkLinearSpline() = default;

//----------------------------------------------------------------------------
// Evaluate a 1D Spline
// Note that this function is essentially a clone of implementation in
// vtkCardinalSpline
double vtkLinearSpline::Evaluate( double t )
{
  // check to see if we need to recompute the spline
  if ( this->ComputeTime < this->GetMTime() )
  {
    this->Compute();
  }

  // make sure we have at least 2 points
  int size = this->PiecewiseFunction->GetSize();
  if ( size < 2 )
  {
    return 0.0;
  }

  if ( this->Closed )
  {
    size = size + 1;
  }

  // clamp the function at both ends
  if ( t < this->Intervals[ 0 ] )
  {
    t = this->Intervals[ 0 ];
  }
  if ( t > this->Intervals[ size - 1 ] )
  {
    t = this->Intervals[ size - 1 ];
  }

  // find pointer to cubic spline coefficient using bisection method
  int index = this->FindIndex( size, t );

  // calculate offset within interval
  t = ( t - this->Intervals[ index ] );

  // evaluate function value
  double t1Coefficient = this->Coefficients[ index * 2 ];
  double t0Coefficient = this->Coefficients[ index * 2 + 1 ];
  return ( t * t1Coefficient + t0Coefficient );
}

//----------------------------------------------------------------------------
// Compute linear splines for each dependent variable
// Note that in linear splines the derivatives at each sample (t,x) is ignored
// LeftConstraint, RightConstraint, LeftValue, and RightValue have no effect
void vtkLinearSpline::Compute()
{
  // how many input points?
  int numberOfInputPoints = this->PiecewiseFunction->GetSize();

  if ( numberOfInputPoints < 2 )
  {
    vtkErrorMacro( "Cannot compute a spline with less than 2 points. # of points is: " << numberOfInputPoints );
    return;
  }

  // how many points to interpolate between?
  int numberOfInterpolatingPoints = 0; // temporary value
  if ( this->Closed )
  {
    numberOfInterpolatingPoints = numberOfInputPoints + 1;
  }
  else
  {
    numberOfInterpolatingPoints = numberOfInputPoints;
  }

  // independent values
  delete [] this->Intervals;
  this->Intervals = new double[ numberOfInterpolatingPoints ];
  double* intervalsStartPtr = this->PiecewiseFunction->GetDataPointer();
  for ( int pointIndex = 0; pointIndex < numberOfInputPoints; pointIndex++ )
  {
    this->Intervals[ pointIndex ] = *( intervalsStartPtr + 2 * pointIndex );
  }
  if ( this->Closed ) // there is still one more point
  {
    if ( this->ParametricRange[ 0 ] != this->ParametricRange[ 1 ] ) // has user specified last range?
    {
      this->Intervals[ numberOfInputPoints ] = this->ParametricRange[ 1 ];
    }
    else // use default behaviour for vtkSpline by adding 1.0 to last value
    {
      this->Intervals[ numberOfInputPoints ] = this->Intervals[ numberOfInputPoints - 1 ] + 1.0;
    }
  }

  // dependent values
  std::vector< double > values = std::vector< double >( numberOfInterpolatingPoints );
  double* valuesStartPtr = this->PiecewiseFunction->GetDataPointer() + 1;
  for ( int pointIndex = 0; pointIndex < numberOfInputPoints; pointIndex++ )
  {
    double nextValue = *( valuesStartPtr + 2 * pointIndex );
    values[ pointIndex ] = nextValue;
  }
  if ( this->Closed ) // there is still one more point, just repeat the first
  {
    double nextValue = values[ 0 ];
    values[ numberOfInputPoints ] = nextValue;
  }

  // compute coefficients
  delete [] this->Coefficients;
  int numberOfSegments = numberOfInterpolatingPoints - 1;
  this->Coefficients = new double [ 2 * numberOfSegments ];
  for ( int segmentIndex = 0; segmentIndex < numberOfSegments; segmentIndex++ )
  {
    double intervalWidth = this->Intervals[ segmentIndex + 1 ] - this->Intervals[ segmentIndex ];
    double changeInValue = values[ segmentIndex + 1 ] - values[ segmentIndex ];
    this->Coefficients[ segmentIndex * 2 ] = changeInValue / intervalWidth;
    this->Coefficients[ segmentIndex * 2 + 1 ] = values[ segmentIndex ];
  }

  // update compute time
  this->ComputeTime = this->GetMTime();
}

//----------------------------------------------------------------------------
void vtkLinearSpline::DeepCopy( vtkSpline *s )
{
  vtkLinearSpline *spline = vtkLinearSpline::SafeDownCast( s );
  if ( spline == NULL )
  {
    vtkWarningMacro( "Cannot deep copy contents into spline - not of matching type." );
    return;
  }

  this->vtkSpline::DeepCopy( s );
}

//----------------------------------------------------------------------------
void vtkLinearSpline::PrintSelf( ostream& os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
}
