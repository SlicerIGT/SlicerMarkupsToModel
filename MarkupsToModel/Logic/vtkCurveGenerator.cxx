#include "vtkCurveGenerator.h"
#include "vtkLinearSpline.h"

#include <vtkCardinalSpline.h>
#include <vtkDoubleArray.h>
#include <vtkKochanekSpline.h>
#include <vtkParametricSpline.h>
#include <vtkParametricFunction.h>
#include "vtkParametricPolynomialApproximation.h"
#include <vtkPoints.h>

#include <list>

//------------------------------------------------------------------------------
vtkStandardNewMacro( vtkCurveGenerator );

//------------------------------------------------------------------------------
vtkCurveGenerator::vtkCurveGenerator()
{
  this->InputPoints = vtkSmartPointer< vtkPoints >::New();
  this->InputParameters = vtkSmartPointer< vtkDoubleArray >::New();
  this->SetCurveTypeToLinearSpline();
  this->CurveIsLoop = false;
  this->NumberOfPointsPerInterpolatingSegment = 5;
  this->KochanekBias = 0.0;
  this->KochanekContinuity = 0.0;
  this->KochanekTension = 0.0;
  this->KochanekEndsCopyNearestDerivatives = false;
  this->PolynomialOrder = 1; // linear
  this->PolynomialPointSortingMethod = vtkCurveGenerator::SORTING_METHOD_INDEX;
  this->OutputPoints = vtkSmartPointer< vtkPoints >::New();
  this->OutputChangedTime.Modified();

  // timestamps for input and output are the same, initially
  this->Modified();
  this->OutputChangedTime.Modified();

  // local storage variables
  this->ParametricFunction = NULL;
}

//------------------------------------------------------------------------------
vtkCurveGenerator::~vtkCurveGenerator()
{
}

//------------------------------------------------------------------------------
void vtkCurveGenerator::PrintSelf( std::ostream &os, vtkIndent indent )
{
  Superclass::PrintSelf( os, indent );
  os << indent << "InputPoints size: " << ( InputPoints != NULL ? this->InputPoints->GetNumberOfPoints() : 0 ) << std::endl;
  os << indent << "InputParameters size: " << ( InputParameters != NULL ? this->InputParameters->GetNumberOfTuples() : 0 ) << std::endl;
  os << indent << "CurveType: " << this->GetCurveTypeAsString() << std::endl;
  os << indent << "CurveIsLoop: " << this->CurveIsLoop << std::endl;
  os << indent << "KochanekBias: " << this->KochanekBias << std::endl;
  os << indent << "KochanekContinuity: " << this->KochanekContinuity << std::endl;
  os << indent << "KochanekTension: " << this->KochanekTension << std::endl;
  os << indent << "KochanekEndsCopyNearestDerivatives: " << this->KochanekEndsCopyNearestDerivatives << std::endl;
  os << indent << "PolynomialOrder: " << this->PolynomialOrder << std::endl;
  os << indent << "OutputPoints size: " << ( OutputPoints != NULL ? this->OutputPoints->GetNumberOfPoints() : 0 ) << std::endl;
}

//------------------------------------------------------------------------------
// INPUT ACCESSORS/MUTATORS
//------------------------------------------------------------------------------
std::string vtkCurveGenerator::GetCurveTypeAsString()
{
  switch ( this->CurveType )
  {
    case vtkCurveGenerator::CURVE_TYPE_LINEAR_SPLINE:
    {
      return "linear_spline";
    }
    case vtkCurveGenerator::CURVE_TYPE_CARDINAL_SPLINE:
    {
      return "cardinal_spline";
    }
    case vtkCurveGenerator::CURVE_TYPE_KOCHANEK_SPLINE:
    {
      return "kochanek_spline";
    }
    case vtkCurveGenerator::CURVE_TYPE_POLYNOMIAL_GLOBAL_LEAST_SQUARES:
    {
      return "polynomial_global_least_squares";
    }
    default:
    {
      return "unknown_curve_type";
    }
  }
}

//------------------------------------------------------------------------------
bool vtkCurveGenerator::IsCurveTypeInterpolating()
{
  bool isInterpolating = ( this->CurveType == CURVE_TYPE_LINEAR_SPLINE ) ||
                         ( this->CurveType == CURVE_TYPE_CARDINAL_SPLINE ) ||
                         ( this->CurveType == CURVE_TYPE_KOCHANEK_SPLINE );
  return isInterpolating;
}

//------------------------------------------------------------------------------
bool vtkCurveGenerator::IsCurveTypeApproximating()
{
  bool isApproximating = ( this->CurveType == CURVE_TYPE_POLYNOMIAL_GLOBAL_LEAST_SQUARES );
  return isApproximating;
}

//------------------------------------------------------------------------------
vtkPoints* vtkCurveGenerator::GetInputPoints()
{
  return this->InputPoints;
}

//------------------------------------------------------------------------------
void vtkCurveGenerator::SetInputPoints( vtkPoints* points )
{
  this->InputPoints->DeepCopy( points );
  this->Modified();
}

//------------------------------------------------------------------------------
// OUTPUT ACCESSORS
//------------------------------------------------------------------------------
vtkPoints* vtkCurveGenerator::GetOutputPoints()
{
  if ( this->UpdateNeeded() )
  {
    this->Update();
  }

  return this->OutputPoints;
}

//------------------------------------------------------------------------------
// LOGIC
//------------------------------------------------------------------------------
void vtkCurveGenerator::Update()
{
  if ( this->InputPoints == NULL )
  {
    vtkWarningMacro( "No input points. No curve generation possible." );
    this->OutputChangedTime.Modified();
    return;
  }

  int numberOfInputPoints = this->InputPoints->GetNumberOfPoints();
  if ( numberOfInputPoints <= 1 )
  {
    vtkWarningMacro( "Not enough input points, need 2 but got " << numberOfInputPoints << ". No curve generation possible." );
    return;
  }

  if ( !this->UpdateNeeded() )
  {
    return;
  }

  switch ( this->CurveType )
  {
    case vtkCurveGenerator::CURVE_TYPE_LINEAR_SPLINE:
    {
      this->SetParametricFunctionToLinearSpline();
      break;
    }
    case vtkCurveGenerator::CURVE_TYPE_CARDINAL_SPLINE:
    {
      this->SetParametricFunctionToCardinalSpline();
      break;
    }
    case vtkCurveGenerator::CURVE_TYPE_KOCHANEK_SPLINE:
    {
      this->SetParametricFunctionToKochanekSpline();
      break;
    }
    case vtkCurveGenerator::CURVE_TYPE_POLYNOMIAL_GLOBAL_LEAST_SQUARES:
    {
      this->SetParametricFunctionToPolynomial();
      break;
    }
    default:
    {
      vtkErrorMacro( "Error: Unrecognized curve type." );
      break;
    }
  }

  this->GeneratePoints();

  this->OutputChangedTime.Modified();
}

//------------------------------------------------------------------------------
bool vtkCurveGenerator::UpdateNeeded()
{
  return ( this->GetMTime() > this->OutputChangedTime );
}

//------------------------------------------------------------------------------
void vtkCurveGenerator::SetParametricFunctionToSpline( vtkSpline* xSpline, vtkSpline* ySpline, vtkSpline* zSpline )
{
  vtkSmartPointer< vtkParametricSpline > parametricSpline = vtkSmartPointer< vtkParametricSpline >::New();
  parametricSpline->SetXSpline( xSpline );
  parametricSpline->SetYSpline( ySpline );
  parametricSpline->SetZSpline( zSpline );
  parametricSpline->SetPoints( this->InputPoints );
  parametricSpline->SetClosed( this->CurveIsLoop );
  parametricSpline->SetParameterizeByLength( false );
  this->ParametricFunction = parametricSpline;
}

//------------------------------------------------------------------------------
void vtkCurveGenerator::SetParametricFunctionToLinearSpline()
{
  vtkSmartPointer< vtkLinearSpline > xSpline = vtkSmartPointer< vtkLinearSpline >::New();
  vtkSmartPointer< vtkLinearSpline > ySpline = vtkSmartPointer< vtkLinearSpline >::New();
  vtkSmartPointer< vtkLinearSpline > zSpline = vtkSmartPointer< vtkLinearSpline >::New();
  this->SetParametricFunctionToSpline( xSpline, ySpline, zSpline );
}

//------------------------------------------------------------------------------
void vtkCurveGenerator::SetParametricFunctionToCardinalSpline()
{
  vtkSmartPointer< vtkCardinalSpline > xSpline = vtkSmartPointer< vtkCardinalSpline >::New();
  vtkSmartPointer< vtkCardinalSpline > ySpline = vtkSmartPointer< vtkCardinalSpline >::New();
  vtkSmartPointer< vtkCardinalSpline > zSpline = vtkSmartPointer< vtkCardinalSpline >::New();
  this->SetParametricFunctionToSpline( xSpline, ySpline, zSpline );
}

//------------------------------------------------------------------------------
void vtkCurveGenerator::SetParametricFunctionToKochanekSpline()
{
  vtkSmartPointer< vtkKochanekSpline > xSpline = vtkSmartPointer< vtkKochanekSpline >::New();
  xSpline->SetDefaultBias( this->KochanekBias );
  xSpline->SetDefaultTension( this->KochanekTension );
  xSpline->SetDefaultContinuity( this->KochanekContinuity );

  vtkSmartPointer< vtkKochanekSpline > ySpline = vtkSmartPointer< vtkKochanekSpline >::New();
  ySpline->SetDefaultBias( this->KochanekBias );
  ySpline->SetDefaultTension( this->KochanekTension );
  ySpline->SetDefaultContinuity( this->KochanekContinuity );

  vtkSmartPointer< vtkKochanekSpline > zSpline = vtkSmartPointer< vtkKochanekSpline >::New();
  zSpline->SetDefaultBias( this->KochanekBias );
  zSpline->SetDefaultTension( this->KochanekTension );
  zSpline->SetDefaultContinuity( this->KochanekContinuity );

  if ( this->KochanekEndsCopyNearestDerivatives )
  {
    // manually set the derivative to the nearest value
    // (difference between the two nearest points). The
    // constraint mode is set to 1, this tells the spline
    // class to use our manual definition.
    // left derivative
    xSpline->SetLeftConstraint( 1 );
    ySpline->SetLeftConstraint( 1 );
    zSpline->SetLeftConstraint( 1 );
    double point0[ 3 ];
    this->InputPoints->GetPoint( 0, point0 );
    double point1[ 3 ];
    this->InputPoints->GetPoint( 1, point1 );
    xSpline->SetLeftValue( point1[ 0 ] - point0[ 0 ] );
    ySpline->SetLeftValue( point1[ 1 ] - point0[ 1 ] );
    zSpline->SetLeftValue( point1[ 2 ] - point0[ 2 ] );
    // right derivative
    xSpline->SetRightConstraint( 1 );
    ySpline->SetRightConstraint( 1 );
    zSpline->SetRightConstraint( 1 );
    int numberOfInputPoints = this->InputPoints->GetNumberOfPoints();
    double pointNMinus2[ 3 ];
    this->InputPoints->GetPoint( numberOfInputPoints - 2, pointNMinus2 );
    double pointNMinus1[ 3 ];
    this->InputPoints->GetPoint( numberOfInputPoints - 1, pointNMinus1 );
    xSpline->SetRightValue( pointNMinus1[ 0 ] - pointNMinus2[ 0 ] );
    ySpline->SetRightValue( pointNMinus1[ 1 ] - pointNMinus2[ 1 ] );
    zSpline->SetRightValue( pointNMinus1[ 2 ] - pointNMinus2[ 2 ] );
  }
  else
  {
    // This ("0") is the most simple mode for end derivative computation, 
    // described by documentation as using the "first/last two points".
    // Use this as the default because others would require setting the 
    // derivatives manually
    xSpline->SetLeftConstraint( 0 );
    ySpline->SetLeftConstraint( 0 );
    zSpline->SetLeftConstraint( 0 );
    xSpline->SetRightConstraint( 0 );
    ySpline->SetRightConstraint( 0 );
    zSpline->SetRightConstraint( 0 );
  }

  this->SetParametricFunctionToSpline( xSpline, ySpline, zSpline );
}

//------------------------------------------------------------------------------
void vtkCurveGenerator::SetParametricFunctionToPolynomial()
{
  vtkSmartPointer< vtkParametricPolynomialApproximation > polynomial = vtkSmartPointer< vtkParametricPolynomialApproximation >::New();
  polynomial->SetPoints( this->InputPoints );
  polynomial->SetPolynomialOrder( this->PolynomialOrder );

  if ( this->PolynomialPointSortingMethod == vtkCurveGenerator::SORTING_METHOD_INDEX )
  {
    vtkCurveGenerator::SortByIndex( this->InputPoints, this->InputParameters );
  }
  else if ( this->PolynomialPointSortingMethod == vtkCurveGenerator::SORTING_METHOD_MINIMUM_SPANNING_TREE_POSITION  )
  {
    vtkCurveGenerator::SortByMinimumSpanningTreePosition( this->InputPoints, this->InputParameters );
  }
  else
  {
    vtkWarningMacro( "Did not recognize point sorting method. Parameters will not be generated." );
  }
  
  polynomial->SetParameters( this->InputParameters );
  this->ParametricFunction = polynomial;
}

//------------------------------------------------------------------------------
void vtkCurveGenerator::GeneratePoints()
{
  this->OutputPoints->Reset();

  if ( this->ParametricFunction == NULL )
  {
    vtkErrorMacro( "Parametric function is null, so curve points cannot be generated." );
    return;
  }

  int numberOfInputPoints = this->InputPoints->GetNumberOfPoints();
  int numberOfSegments = 0; // temporary value
  if ( this->CurveIsLoop && this->CurveType != vtkCurveGenerator::CURVE_TYPE_POLYNOMIAL_GLOBAL_LEAST_SQUARES )
  {
    numberOfSegments = numberOfInputPoints;
  }
  else
  {
    numberOfSegments = ( numberOfInputPoints - 1 );
  }

  int totalNumberOfPoints = this->NumberOfPointsPerInterpolatingSegment * numberOfSegments + 1;
  for ( int pointIndex = 0; pointIndex < totalNumberOfPoints; pointIndex++ )
  {
    double sampleParameter = pointIndex / (double) ( totalNumberOfPoints - 1 );
    double curvePoint[ 3 ];
    this->ParametricFunction->Evaluate( &sampleParameter, curvePoint, NULL );
    this->OutputPoints->InsertNextPoint( curvePoint );
  }
}

//------------------------------------------------------------------------------
void vtkCurveGenerator::SortByIndex( vtkPoints* points, vtkDoubleArray* parameters )
{
  if ( points == NULL )
  {
    vtkGenericWarningMacro( "Input control points are null. Returning." );
    return;
  }
  
  if ( parameters == NULL )
  {
    vtkGenericWarningMacro( "Output control point parameters are null. Returning." );
    return;
  }

  int numberOfPoints = points->GetNumberOfPoints();
  // redundant error checking, to be safe
  if ( numberOfPoints < 2 )
  {
    vtkGenericWarningMacro( "Not enough points to compute polynomial parameters. Need at least 2 points but " << numberOfPoints << " are provided." );
    return;
  }

  parameters->Reset();
  for ( int v = 0; v < numberOfPoints; v++ )
  {
    parameters->InsertNextTuple1( v / double( numberOfPoints - 1 ) );
    // division to clamp all values to range 0.0 - 1.0
  }
}

//------------------------------------------------------------------------------
void vtkCurveGenerator::SortByMinimumSpanningTreePosition( vtkPoints* points, vtkDoubleArray* parameters )
{
  if ( points == NULL )
  {
    vtkGenericWarningMacro( "Input points are null. Returning" );
    return;
  }

  if ( parameters == NULL )
  {
    vtkGenericWarningMacro( "Output point parameters are null. Returning" );
    return;
  }

  int numberOfPoints = points->GetNumberOfPoints();
  // redundant error checking, to be safe
  if ( numberOfPoints < 2 )
  {
    vtkGenericWarningMacro( "Not enough points to compute polynomial parameters. Need at least 2 points but " << numberOfPoints << " are provided." );
    return;
  }

  // vtk boost algorithms cannot be used because they are not built with 3D Slicer
  // so this is a custom implementation of:
  // 1. constructing an undirected graph as a 2D array
  // 2. Finding the two vertices that are the farthest apart
  // 3. running prim's algorithm on the graph
  // 4. extract the "trunk" path from the last vertex to the first
  // 5. based on the distance along that path, assign each vertex a polynomial parameter value

  // in the following code, two tasks are done:
  // 1. construct an undirected graph
  std::vector< double > distances( numberOfPoints * numberOfPoints );
  distances.assign( numberOfPoints * numberOfPoints, 0.0 );
  // 2. find the two farthest-seperated vertices in the distances array
  int treeStartIndex = 0;
  int treeEndIndex = 0;
  double maximumDistance = 0;
  // iterate through all points
  for ( int v = 0; v < numberOfPoints; v++ )
  {
    for ( int u = 0; u < numberOfPoints; u++ )
    {
      double pointU[ 3 ], pointV[ 3 ];
      points->GetPoint( u, pointU );
      points->GetPoint( v, pointV );
      double distanceSquared = vtkMath::Distance2BetweenPoints( pointU, pointV );
      double distance = sqrt( distanceSquared );
      distances[ v * numberOfPoints + u ] = distance;
      if ( distance > maximumDistance )
      {
        maximumDistance = distance;
        treeStartIndex = v;
        treeEndIndex = u;
      }
    }
  }
  // use the 1D vector as a 2D vector
  std::vector< double* > graph(numberOfPoints);
  for (int v = 0; v < numberOfPoints; v++)
  {
    graph[v] = &(distances[v * numberOfPoints]);
  }

  // implementation of Prim's algorithm heavily based on:
  // http://www.geeksforgeeks.org/greedy-algorithms-set-5-prims-minimum-spanning-tree-mst-2/
  std::vector< int > parent( numberOfPoints ); // Array to store constructed MST
  std::vector< double > key( numberOfPoints );   // Key values used to pick minimum weight edge in cut
  std::vector< bool > mstSet( numberOfPoints );  // To represent set of vertices not yet included in MST

  // Initialize all keys as INFINITE (or at least as close as we can get)
  for ( int i = 0; i < numberOfPoints; i++ )
  {
    key[ i ] = VTK_DOUBLE_MAX;
    mstSet[ i ] = false;
  }

  // Always include first 1st vertex in MST.
  key[ treeStartIndex ] = 0.0; // Make key 0 so that this vertex is picked as first vertex
  parent[ treeStartIndex ] = -1; // First node is always root of MST 

  // The MST will have numberOfPoints vertices
  for ( int count = 0; count < numberOfPoints - 1; count++ )
  {
    // Pick the minimum key vertex from the set of vertices
    // not yet included in MST
    int nextPointIndex = -1;
    double minDistance = VTK_DOUBLE_MAX;
    for ( int v = 0; v < numberOfPoints; v++ )
    {
      if ( mstSet[ v ] == false && key[ v ] < minDistance )
      {
        minDistance = key[ v ];
        nextPointIndex = v;
      }
    }

    // Add the picked vertex to the MST Set
    mstSet[ nextPointIndex ] = true;

    // Update key value and parent index of the adjacent vertices of
    // the picked vertex. Consider only those vertices which are not yet
    // included in MST
    for ( int v = 0; v < numberOfPoints; v++ )
    {
      // graph[u][v] is non zero only for adjacent vertices of m
      // mstSet[v] is false for vertices not yet included in MST
      // Update the key only if graph[u][v] is smaller than key[v]
      if ( graph[ nextPointIndex ][ v ] >= 0 &&
           mstSet[ v ] == false &&
           graph[ nextPointIndex ][ v ] < key[ v ] )
      {
        parent[ v ] = nextPointIndex;
        key[ v ] = graph[nextPointIndex ][ v ];
      }
    }
  }

  // determine the "trunk" path of the tree, from first index to last index
  std::vector< int > pathIndices;
  int currentPathIndex = treeEndIndex;
  while ( currentPathIndex != -1 )
  {
    pathIndices.push_back( currentPathIndex );
    currentPathIndex = parent[ currentPathIndex ]; // go up the tree one layer
  }

  // find the sum of distances along the trunk path of the tree
  double sumOfDistances = 0.0;
  for ( unsigned int i = 0; i < pathIndices.size() - 1; i++ )
  {
    sumOfDistances += graph[ i ][ i + 1 ];
  }

  // check this to prevent a division by zero (in case all points are duplicates)
  if ( sumOfDistances == 0 )
  {
    vtkGenericWarningMacro( "Minimum spanning tree path has distance zero. No parameters will be assigned. Check inputs (are there duplicate points?)." );
    return;
  }

  // find the parameters along the trunk path of the tree
  std::vector< double > pathParameters;
  double currentDistance = 0.0;
  for ( unsigned int i = 0; i < pathIndices.size() - 1; i++ )
  {
    pathParameters.push_back( currentDistance / sumOfDistances );
    currentDistance += graph[ i ][ i + 1 ];
  }
  pathParameters.push_back( currentDistance / sumOfDistances ); // this should be 1.0

  // finally assign polynomial parameters to each point, and store in the output array
  parameters->Reset();
  for ( int i = 0; i < numberOfPoints; i++ )
  {
    int currentIndex = i;
    bool alongPath = false;
    int indexAlongPath = -1;
    for ( unsigned int j = 0; j < pathIndices.size(); j++ )
    {
      if ( pathIndices[ j ] == currentIndex )
      {
        alongPath = true;
        indexAlongPath = j;
        break;
      }
    }
    while ( !alongPath )
    {
      currentIndex = parent[ currentIndex ];
      for ( unsigned int j = 0; j < pathIndices.size(); j++ )
      {
        if ( pathIndices[ j ] == currentIndex )
        {
          alongPath = true;
          indexAlongPath = j;
          break;
        }
      }
    }
    parameters->InsertNextTuple1( pathParameters[ indexAlongPath ] );
  }
}
