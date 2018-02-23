/*==============================================================================

  Program: 3D Slicer

  Portions (c) Copyright Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  ==============================================================================*/

// MarkupsToModel Logic includes
#include "vtkSlicerMarkupsToModelLogic.h"
#include "vtkSlicerMarkupsToModelClosedSurfaceGeneration.h"
#include "vtkSlicerMarkupsToModelCurveGeneration.h"

// MRML includes
#include "vtkMRMLMarkupsFiducialNode.h"
#include "vtkMRMLModelNode.h"
#include "vtkMRMLSelectionNode.h"
#include <vtkMRMLModelDisplayNode.h>
#include <vtkMRMLModelNode.h>
#include <vtkMRMLScene.h>

// VTK includes
#include <vtkAppendPolyData.h>
#include <vtkCellArray.h>
#include <vtkCleanPolyData.h>
#include <vtkCollection.h>
#include <vtkCollectionIterator.h>
#include <vtkDoubleArray.h>
#include <vtkDataSetSurfaceFilter.h>
#include <vtkIntArray.h>
#include <vtkMath.h>
#include <vtkMatrix4x4.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPoints.h>
#include <vtkPolyDataNormals.h>
#include <vtkTransform.h>
#include <vtkTransformFilter.h>
#include <vtkUnstructuredGrid.h>

// STD includes
#include <cassert>
#include <cmath>
#include <vector>
#include <set>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerMarkupsToModelLogic);

//----------------------------------------------------------------------------
vtkSlicerMarkupsToModelLogic::vtkSlicerMarkupsToModelLogic()
{
}

//----------------------------------------------------------------------------
vtkSlicerMarkupsToModelLogic::~vtkSlicerMarkupsToModelLogic()
{
}

//----------------------------------------------------------------------------
void vtkSlicerMarkupsToModelLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
void vtkSlicerMarkupsToModelLogic::SetMRMLSceneInternal(vtkMRMLScene * newScene)
{
  vtkNew<vtkIntArray> events;
  events->InsertNextValue(vtkMRMLScene::NodeAddedEvent);
  events->InsertNextValue(vtkMRMLScene::NodeRemovedEvent);
  events->InsertNextValue(vtkMRMLScene::StartBatchProcessEvent);
  events->InsertNextValue(vtkMRMLScene::EndBatchProcessEvent);
  events->InsertNextValue(vtkMRMLScene::StartImportEvent);
  events->InsertNextValue(vtkMRMLScene::EndImportEvent);
  this->SetAndObserveMRMLSceneEventsInternal(newScene, events.GetPointer());
}

//-----------------------------------------------------------------------------
void vtkSlicerMarkupsToModelLogic::RegisterNodes()
{
  if (!this->GetMRMLScene())
  {
    vtkWarningMacro("MRML scene not yet created");
    return;
  }

  this->GetMRMLScene()->RegisterNodeClass(vtkSmartPointer< vtkMRMLMarkupsToModelNode >::New());
}

//---------------------------------------------------------------------------
void vtkSlicerMarkupsToModelLogic::UpdateFromMRMLScene()
{
  assert(this->GetMRMLScene() != 0);
}

//---------------------------------------------------------------------------
void vtkSlicerMarkupsToModelLogic::OnMRMLSceneEndImport()
{
  vtkSmartPointer<vtkCollection> markupsToModelNodes = vtkSmartPointer<vtkCollection>::Take(
    this->GetMRMLScene()->GetNodesByClass("vtkMRMLMarkupsToModelNode"));
  vtkNew<vtkCollectionIterator> markupsToModelNodeIt;
  markupsToModelNodeIt->SetCollection(markupsToModelNodes);
  for (markupsToModelNodeIt->InitTraversal(); !markupsToModelNodeIt->IsDoneWithTraversal(); markupsToModelNodeIt->GoToNextItem())
  {
    vtkMRMLMarkupsToModelNode* markupsToModelNode = vtkMRMLMarkupsToModelNode::SafeDownCast(markupsToModelNodeIt->GetCurrentObject());
    if (markupsToModelNode != NULL)
    {
      this->UpdateOutputModel(markupsToModelNode);
    }
  }
}

//---------------------------------------------------------------------------
void vtkSlicerMarkupsToModelLogic::OnMRMLSceneStartImport()
{
}

//---------------------------------------------------------------------------
void vtkSlicerMarkupsToModelLogic::OnMRMLSceneNodeAdded(vtkMRMLNode* node)
{
  if (node == NULL || this->GetMRMLScene() == NULL)
  {
    vtkWarningMacro("OnMRMLSceneNodeAdded: Invalid MRML scene or node");
    return;
  }

  vtkMRMLMarkupsToModelNode* markupsToModelNode = vtkMRMLMarkupsToModelNode::SafeDownCast(node);
  if (markupsToModelNode)
  {
    vtkDebugMacro("OnMRMLSceneNodeAdded: Module node added.");
    vtkUnObserveMRMLNodeMacro(markupsToModelNode); // Remove previous observers.
    vtkNew<vtkIntArray> events;
    events->InsertNextValue(vtkCommand::ModifiedEvent);
    events->InsertNextValue(vtkMRMLMarkupsToModelNode::MarkupsPositionModifiedEvent);
    vtkObserveMRMLNodeEventsMacro(markupsToModelNode, events.GetPointer());
  }
}

//---------------------------------------------------------------------------
void vtkSlicerMarkupsToModelLogic::OnMRMLSceneNodeRemoved(vtkMRMLNode* node)
{
  if (node == NULL || this->GetMRMLScene() == NULL)
  {
    vtkWarningMacro("OnMRMLSceneNodeRemoved: Invalid MRML scene or node");
    return;
  }

  if (node->IsA("vtkSlicerMarkupsToModelNode"))
  {
    vtkDebugMacro("OnMRMLSceneNodeRemoved");
    vtkUnObserveMRMLNodeMacro(node);
  }
}

//------------------------------------------------------------------------------
void vtkSlicerMarkupsToModelLogic::UpdateSelectionNode(vtkMRMLMarkupsToModelNode* markupsToModelModuleNode)
{
  if (!markupsToModelModuleNode)
  {
    // No markups selected
    return;
  }
  vtkMRMLNode* inputNode = markupsToModelModuleNode->GetInputNode();
  vtkMRMLMarkupsFiducialNode* markupsNode = vtkMRMLMarkupsFiducialNode::SafeDownCast( inputNode );
  if (!markupsNode)
  {
    // No markups selected
    return;
  }
  if (!this->GetMRMLScene())
  {
    vtkErrorMacro("UpdateSelectionNode: no scene defined!");
    return;
  }

  // try the application logic first
  vtkMRMLApplicationLogic *mrmlAppLogic = this->GetMRMLApplicationLogic();
  vtkMRMLSelectionNode *selectionNode = NULL;
  if (mrmlAppLogic)
  {
    selectionNode = mrmlAppLogic->GetSelectionNode();
  }
  else
  {
    // try a default string
    selectionNode = vtkMRMLSelectionNode::SafeDownCast(this->GetMRMLScene()->GetNodeByID("vtkMRMLSelectionNodeSingleton"));
  }
  if (!selectionNode)
  {
    vtkErrorMacro("UpdateSelectionNode: selection node is not available");
    return;
  }

  const char *activeID = markupsNode ? markupsNode->GetID() : NULL;
  if (!activeID)
  {
    return;
  }

  const char *selectionNodeActivePlaceNodeID = selectionNode->GetActivePlaceNodeID();
  if (selectionNodeActivePlaceNodeID != NULL && activeID != NULL && !strcmp(selectionNodeActivePlaceNodeID, activeID))
  {
    // no change
    return;
  }

  selectionNode->SetReferenceActivePlaceNodeID(activeID);
}

//------------------------------------------------------------------------------
void vtkSlicerMarkupsToModelLogic::UpdateOutputModel(vtkMRMLMarkupsToModelNode* markupsToModelModuleNode)
{
  if ( markupsToModelModuleNode == NULL )
  {
    vtkErrorMacro( "No markupsToModelModuleNode provided to UpdateOutputModel. No operation performed." );
    return;
  }

  vtkMRMLNode* inputNode = markupsToModelModuleNode->GetInputNode();
  if (inputNode == NULL)
  {
    return;
  }

  if (markupsToModelModuleNode->GetOutputModelNode() == NULL)
  {
    vtkErrorMacro("No output model node provided to UpdateOutputModel. No operation performed.");
    return;
  }

  // extract the input points from the MRML node, according to its type
  vtkSmartPointer< vtkPoints > controlPoints = vtkSmartPointer< vtkPoints >::New();
  vtkMRMLMarkupsFiducialNode* inputMarkupsNode = vtkMRMLMarkupsFiducialNode::SafeDownCast( inputNode );
  vtkMRMLModelNode* inputModelNode = vtkMRMLModelNode::SafeDownCast( inputNode );
  if ( inputMarkupsNode != NULL )
  {
    vtkSlicerMarkupsToModelLogic::MarkupsToPoints( inputMarkupsNode, controlPoints );
  }
  else if ( inputModelNode != NULL )
  {
    vtkSlicerMarkupsToModelLogic::ModelToPoints( inputModelNode, controlPoints );
  }
  else
  {
    vtkErrorMacro( "Input node type is not supported. No operation performed." );
    return;
  }

  // Create the model from the points
  vtkSmartPointer<vtkPolyData> outputPolyData = vtkSmartPointer<vtkPolyData>::New();
  bool cleanMarkups = markupsToModelModuleNode->GetCleanMarkups();
  switch ( markupsToModelModuleNode->GetModelType() )
  {
    case vtkMRMLMarkupsToModelNode::ClosedSurface:
    {
      double delaunayAlpha = markupsToModelModuleNode->GetDelaunayAlpha();
      bool smoothing = markupsToModelModuleNode->GetButterflySubdivision();
      bool forceConvex = markupsToModelModuleNode->GetConvexHull();
      vtkSlicerMarkupsToModelLogic::UpdateClosedSurfaceModel( controlPoints, outputPolyData, smoothing, forceConvex, delaunayAlpha, cleanMarkups );
      break;
    }
    case vtkMRMLMarkupsToModelNode::Curve:
    {
      int tubeSegmentsBetweenControlPoints = markupsToModelModuleNode->GetTubeSegmentsBetweenControlPoints();
      bool tubeLoop = markupsToModelModuleNode->GetTubeLoop();
      double tubeRadius = markupsToModelModuleNode->GetTubeRadius();
      int tubeNumberOfSides = markupsToModelModuleNode->GetTubeNumberOfSides();
      int interpolationType = markupsToModelModuleNode->GetInterpolationType();
      int polynomialOrder = markupsToModelModuleNode->GetPolynomialOrder();
      int pointParameterType = markupsToModelModuleNode->GetPointParameterType();
      bool kochanekEndsCopyNearestDerivatives = markupsToModelModuleNode->GetKochanekEndsCopyNearestDerivatives();
      double kochanekBias = markupsToModelModuleNode->GetKochanekBias();
      double kochanekContinuity = markupsToModelModuleNode->GetKochanekContinuity();
      double kochanekTension = markupsToModelModuleNode->GetKochanekTension();
      vtkSlicerMarkupsToModelLogic::UpdateOutputCurveModel( controlPoints, outputPolyData, interpolationType, tubeLoop, tubeRadius, tubeNumberOfSides, tubeSegmentsBetweenControlPoints, cleanMarkups, polynomialOrder, pointParameterType, kochanekEndsCopyNearestDerivatives, kochanekBias, kochanekContinuity, kochanekTension );
      break;
    }
  }

  vtkSlicerMarkupsToModelLogic::AssignPolyDataToOutput( markupsToModelModuleNode, outputPolyData );
}

//------------------------------------------------------------------------------
void vtkSlicerMarkupsToModelLogic::ProcessMRMLNodesEvents(vtkObject* caller, unsigned long event, void* vtkNotUsed( callData ) )
{
  vtkMRMLNode* callerNode = vtkMRMLNode::SafeDownCast(caller);
  if (callerNode == NULL)
  {
    return;
  }

  vtkMRMLMarkupsToModelNode* markupsToModelModuleNode = vtkMRMLMarkupsToModelNode::SafeDownCast(callerNode);
  if (markupsToModelModuleNode == NULL || !markupsToModelModuleNode->GetAutoUpdateOutput())
  {
    return;
  }

  if (this->GetMRMLScene() &&
    (this->GetMRMLScene()->IsImporting() ||
    this->GetMRMLScene()->IsRestoring() ||
    this->GetMRMLScene()->IsClosing()))
  {
    return;
  }

  if (event == vtkMRMLMarkupsToModelNode::MarkupsPositionModifiedEvent
    || event == vtkCommand::ModifiedEvent)
  {
    this->UpdateOutputModel(markupsToModelModuleNode);
  }
}

//------------------------------------------------------------------------------
// DEPRECATED
void vtkSlicerMarkupsToModelLogic::SetMarkupsNode( vtkMRMLMarkupsFiducialNode* newMarkups, vtkMRMLMarkupsToModelNode* moduleNode )
{
  vtkWarningMacro( "vtkSlicerMarkupsToModelLogic::SetMarkupsNode() is deprecated. Use vtkMRMLMarkupsToModelNode::SetAndObserveOutputModelNodeID() instead." );

  if (moduleNode == NULL)
  {
    vtkWarningMacro("SetMarkupsNode: Module node is invalid");
    return;
  }

  vtkMRMLMarkupsFiducialNode* previousMarkups = vtkMRMLMarkupsFiducialNode::SafeDownCast( moduleNode->GetInputNode() );
  if ( previousMarkups == newMarkups )
  {
    // no change
    return;
  }
  // Switch to the new model node
  moduleNode->SetAndObserveInputNodeID((newMarkups != NULL) ? newMarkups->GetID() : NULL);
}

//------------------------------------------------------------------------------
bool vtkSlicerMarkupsToModelLogic::UpdateOutputCurveModel( vtkMRMLMarkupsFiducialNode* markupsNode, vtkMRMLModelNode* outputModelNode,
  int interpolationType, bool tubeLoop, double tubeRadius, int tubeNumberOfSides, int tubeSegmentsBetweenControlPoints,
  bool cleanMarkups, int polynomialOrder, int pointParameterType )
{
  if ( markupsNode == NULL )
  {
    vtkGenericWarningMacro( "No markups. Aborting." );
    return false;
  }
  
  if ( outputModelNode == NULL )
  {
    vtkGenericWarningMacro( "No model. Aborting." );
    return false;
  }
  
  // extract control points
  vtkSmartPointer< vtkPoints > controlPoints = vtkSmartPointer< vtkPoints >::New();
  vtkSlicerMarkupsToModelLogic::MarkupsToPoints( markupsNode, controlPoints );
  vtkSmartPointer< vtkPolyData > outputPolyData = vtkSmartPointer< vtkPolyData >::New();
  bool success = vtkSlicerMarkupsToModelLogic::UpdateOutputCurveModel( controlPoints, outputPolyData, interpolationType, tubeLoop, tubeRadius, tubeNumberOfSides, tubeSegmentsBetweenControlPoints, cleanMarkups, polynomialOrder, pointParameterType );
  if ( !success )
  {
    return false;
  }
  
  outputModelNode->SetAndObservePolyData( outputPolyData );

  return true;
}

//------------------------------------------------------------------------------
bool vtkSlicerMarkupsToModelLogic::UpdateOutputCurveModel( vtkPoints* controlPoints, vtkPolyData* outputPolyData,
  int interpolationType, bool tubeLoop, double tubeRadius, int tubeNumberOfSides, int tubeSegmentsBetweenControlPoints,
  bool cleanMarkups, int polynomialOrder, int pointParameterType,
  bool kochanekEndsCopyNearestDerivatives, double kochanekBias, double kochanekContinuity, double kochanekTension )
{
  if ( controlPoints == NULL )
  {
    vtkGenericWarningMacro( "No control points. Aborting." );
    return false;
  }
  
  if ( outputPolyData == NULL )
  {
    vtkGenericWarningMacro( "No poly data. Aborting." );
    return false;
  }

  // get rid of duplicate points
  if ( cleanMarkups )
  {
    vtkSlicerMarkupsToModelLogic::RemoveDuplicatePoints( controlPoints );
  }

  // check a few special cases before handling the different types of curve
  if ( controlPoints->GetNumberOfPoints() <= 0 )
  {
    // don't need to do anything for 0 points
    return true;
  }

  if ( controlPoints->GetNumberOfPoints() == 1 )
  {
    vtkSlicerMarkupsToModelCurveGeneration::GenerateSphereModel( controlPoints->GetPoint( 0 ), outputPolyData, tubeRadius, tubeNumberOfSides );
    return true;
  }
  
  if ( controlPoints->GetNumberOfPoints() == 2 )
  {
    vtkSlicerMarkupsToModelCurveGeneration::GeneratePiecewiseLinearCurveModel( controlPoints, outputPolyData, tubeRadius, tubeNumberOfSides, tubeSegmentsBetweenControlPoints, tubeLoop );
    return true;
  }

  switch ( interpolationType )
  {
    // Generates a polynomial curve model.
    case vtkMRMLMarkupsToModelNode::Linear:
    {
      vtkSlicerMarkupsToModelCurveGeneration::GeneratePiecewiseLinearCurveModel( controlPoints, outputPolyData, tubeRadius, tubeNumberOfSides, tubeSegmentsBetweenControlPoints, tubeLoop );
      break;
    }
    case vtkMRMLMarkupsToModelNode::CardinalSpline:
    {
      vtkSlicerMarkupsToModelCurveGeneration::GenerateCardinalSplineCurveModel( controlPoints, outputPolyData, tubeRadius, tubeNumberOfSides, tubeSegmentsBetweenControlPoints, tubeLoop );
      break;
    }
    case vtkMRMLMarkupsToModelNode::KochanekSpline:
    {
      vtkSlicerMarkupsToModelCurveGeneration::GenerateKochanekSplineCurveModel( controlPoints, outputPolyData, tubeRadius, tubeNumberOfSides, tubeSegmentsBetweenControlPoints, tubeLoop, kochanekBias, kochanekContinuity, kochanekTension, kochanekEndsCopyNearestDerivatives );
      break;
    }
    case vtkMRMLMarkupsToModelNode::Polynomial:
    {
      vtkSmartPointer< vtkDoubleArray > controlPointParameters = vtkSmartPointer< vtkDoubleArray >::New();
      switch ( pointParameterType )
      {
        case vtkMRMLMarkupsToModelNode::RawIndices:
        {
          vtkSlicerMarkupsToModelCurveGeneration::ComputePointParametersFromIndices( controlPoints, controlPointParameters );
          break;
        }
        case vtkMRMLMarkupsToModelNode::MinimumSpanningTree:
        {
          vtkSlicerMarkupsToModelCurveGeneration::ComputePointParametersFromMinimumSpanningTree( controlPoints, controlPointParameters );
          break;
        }
        default:
        {
          vtkGenericWarningMacro( "Unknown point parameter type. Aborting." );
          return false;
        }
      }
      vtkSlicerMarkupsToModelCurveGeneration::GeneratePolynomialCurveModel( controlPoints, outputPolyData, tubeRadius, tubeNumberOfSides, tubeSegmentsBetweenControlPoints, tubeLoop, polynomialOrder, controlPointParameters );
      break;
    }
    default:
    {
      vtkGenericWarningMacro( "Unknown interpolation type. Aborting." );
      return false;
    }
  }
  return true;
}

//------------------------------------------------------------------------------
bool vtkSlicerMarkupsToModelLogic::UpdateClosedSurfaceModel(
  vtkMRMLMarkupsFiducialNode* markupsNode, vtkMRMLModelNode* outputModelNode,
  bool smoothing, bool forceConvex, double delaunayAlpha, bool cleanMarkups )
{
  if ( markupsNode == NULL )
  {
    vtkGenericWarningMacro( "No markups. Aborting." );
    return false;
  }
  
  if ( outputModelNode == NULL )
  {
    vtkGenericWarningMacro( "No model. Aborting." );
    return false;
  }

  vtkSmartPointer< vtkPoints > controlPoints = vtkSmartPointer< vtkPoints >::New();
  vtkSlicerMarkupsToModelLogic::MarkupsToPoints( markupsNode, controlPoints );
  vtkSmartPointer< vtkPolyData > outputPolyData = vtkSmartPointer< vtkPolyData >::New();
  bool success = vtkSlicerMarkupsToModelLogic::UpdateClosedSurfaceModel( controlPoints, outputPolyData, smoothing, forceConvex, delaunayAlpha, cleanMarkups );
  if ( !success )
  {
    return false;
  }

  outputModelNode->SetAndObservePolyData( outputPolyData );
  return true;
}

//------------------------------------------------------------------------------
bool vtkSlicerMarkupsToModelLogic::UpdateClosedSurfaceModel(
  vtkPoints* controlPoints, vtkPolyData* outputPolyData,
  bool smoothing, bool forceConvex, double delaunayAlpha, bool cleanMarkups )
{
  if ( controlPoints == NULL )
  {
    vtkGenericWarningMacro( "No control points. Aborting." );
    return false;
  }
  
  if ( outputPolyData == NULL )
  {
    vtkGenericWarningMacro( "No poly data. Aborting." );
    return false;
  }

  // get rid of duplicate points
  if ( cleanMarkups )
  {
    vtkSlicerMarkupsToModelLogic::RemoveDuplicatePoints( controlPoints );
  }

  vtkSlicerMarkupsToModelClosedSurfaceGeneration::GenerateClosedSurfaceModel( controlPoints, outputPolyData, delaunayAlpha, smoothing, forceConvex );
  return true;
}

//------------------------------------------------------------------------------
void vtkSlicerMarkupsToModelLogic::ModelToPoints( vtkMRMLModelNode* inputModelNode, vtkPoints* outputPoints )
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
void vtkSlicerMarkupsToModelLogic::MarkupsToPoints( vtkMRMLMarkupsFiducialNode* inputMarkupsNode, vtkPoints* outputPoints )
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
void vtkSlicerMarkupsToModelLogic::RemoveDuplicatePoints( vtkPoints* points )
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
void vtkSlicerMarkupsToModelLogic::AssignPolyDataToOutput( vtkMRMLMarkupsToModelNode* markupsToModelModuleNode, vtkPolyData* outputPolyData )
{  
  vtkMRMLModelNode* outputModelNode = markupsToModelModuleNode->GetOutputModelNode();
  if ( outputModelNode == NULL )
  {
    vtkGenericWarningMacro( "Output model node is not specified. No operation performed." );
    return;
  }
  outputModelNode->SetAndObservePolyData( outputPolyData );

  // Attach a display node if needed
  vtkMRMLModelDisplayNode* displayNode = vtkMRMLModelDisplayNode::SafeDownCast( outputModelNode->GetDisplayNode() );
  if ( displayNode == NULL )
  {
    outputModelNode->CreateDefaultDisplayNodes();
    displayNode = vtkMRMLModelDisplayNode::SafeDownCast( outputModelNode->GetDisplayNode() );
    std::string name = std::string( outputModelNode->GetName() ).append( "ModelDisplay" );
    displayNode->SetName( name.c_str() );
  }
}
