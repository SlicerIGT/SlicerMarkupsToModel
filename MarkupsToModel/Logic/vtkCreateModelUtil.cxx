#include "vtkCreateModelUtil.h"

// slicer includes
#include "vtkMRMLMarkupsFiducialNode.h"
#include "vtkMRMLModelNode.h"
#include "vtkMRMLNode.h"

// vtk includes
#include <vtkCleanPolyData.h>
#include <vtkPolyData.h>
#include <vtkPoints.h>
#include <vtkSmartPointer.h>

//------------------------------------------------------------------------------
vtkStandardNewMacro( vtkCreateModelUtil );

//------------------------------------------------------------------------------
vtkCreateModelUtil::vtkCreateModelUtil()
{
}

//------------------------------------------------------------------------------
vtkCreateModelUtil::~vtkCreateModelUtil()
{
}

//------------------------------------------------------------------------------
bool vtkCreateModelUtil::vtkMRMLNodeToVtkPointsSupported( vtkMRMLNode* inputNode )
{
  bool inputIsModelNode = vtkMRMLModelNode::SafeDownCast( inputNode ) != NULL;
  bool inputIsMarkupsNode = vtkMRMLMarkupsFiducialNode::SafeDownCast( inputNode ) != NULL;
  return ( inputIsModelNode || inputIsMarkupsNode );
}

//------------------------------------------------------------------------------
void vtkCreateModelUtil::vtkMRMLNodeToVtkPoints( vtkMRMLNode* inputNode, vtkPoints* outputPoints )
{
  if ( inputNode == NULL )
  {
    vtkGenericWarningMacro( "Input node is null. No points will be obtained." );
    return;
  }
  
  if ( outputPoints == NULL )
  {
    vtkGenericWarningMacro( "Output vtkPoints is null. No points will be obtained." );
    return;
  }

  vtkMRMLMarkupsFiducialNode* inputMarkupsNode = vtkMRMLMarkupsFiducialNode::SafeDownCast( inputNode );
  vtkMRMLModelNode* inputModelNode = vtkMRMLModelNode::SafeDownCast( inputNode );
  if ( inputModelNode != NULL)
  {
    vtkCreateModelUtil::vtkMRMLNodeToVtkPoints( inputModelNode, outputPoints );
  }
  else if ( inputMarkupsNode != NULL )
  {
    vtkCreateModelUtil::vtkMRMLNodeToVtkPoints( inputMarkupsNode, outputPoints );
  }
  else
  {
    vtkGenericWarningMacro( "Unsupported input node type. No points will be obtained." );
  }
}

//------------------------------------------------------------------------------
void vtkCreateModelUtil::vtkMRMLNodeToVtkPoints( vtkMRMLModelNode* inputModelNode, vtkPoints* outputPoints )
{
  if ( inputModelNode == NULL )
  {
    vtkGenericWarningMacro( "Input node is null. No points will be obtained." );
    return;
  }

  if ( outputPoints == NULL )
  {
    vtkGenericWarningMacro( "Output vtkPoints is null. No points will be obtained." );
    return;
  }

  vtkPolyData* inputPolyData = inputModelNode->GetPolyData();
  if ( inputPolyData == NULL )
  {
    return;
  }
  vtkPoints* inputPoints = inputPolyData->GetPoints();
  if ( inputPoints == NULL )
  {
    return;
  }
  outputPoints->DeepCopy( inputPoints );
}

//------------------------------------------------------------------------------
void vtkCreateModelUtil::vtkMRMLNodeToVtkPoints( vtkMRMLMarkupsFiducialNode* inputMarkupsNode, vtkPoints* outputPoints )
{
  if ( inputMarkupsNode == NULL )
  {
    vtkGenericWarningMacro( "Input node is null. No points will be obtained." );
    return;
  }

  if ( outputPoints == NULL )
  {
    vtkGenericWarningMacro( "Output vtkPoints is null. No points will be obtained." );
    return;
  }

  int numberOfInputMarkups = inputMarkupsNode->GetNumberOfFiducials();
  outputPoints->SetNumberOfPoints( numberOfInputMarkups );
  double inputMarkupPoint[ 3 ] = { 0.0, 0.0, 0.0 }; // values temporary
  for ( int i = 0; i < numberOfInputMarkups; i++ )
  {
    inputMarkupsNode->GetNthFiducialPosition( i, inputMarkupPoint );
    outputPoints->SetPoint( i, inputMarkupPoint );
  }
}

//------------------------------------------------------------------------------
void vtkCreateModelUtil::RemoveDuplicatePoints( vtkPoints* points )
{
  vtkSmartPointer< vtkPolyData > polyData = vtkSmartPointer< vtkPolyData >::New();
  polyData->Initialize();
  polyData->SetPoints( points );

  vtkSmartPointer< vtkCleanPolyData > cleanPointPolyData = vtkSmartPointer< vtkCleanPolyData >::New();
  cleanPointPolyData->SetInputData( polyData );
  const double CLEAN_POLYDATA_TOLERANCE_MM = 0.01;
  cleanPointPolyData->SetTolerance( CLEAN_POLYDATA_TOLERANCE_MM );
  cleanPointPolyData->Update();
}

//------------------------------------------------------------------------------
void vtkCreateModelUtil::PrintSelf( ostream &os, vtkIndent indent )
{
  Superclass::PrintSelf( os, indent );
}
