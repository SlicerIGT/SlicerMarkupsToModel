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
#include "vtkCurveGenerator.h"

// MRML includes
#include "vtkMRMLMarkupsNode.h"
#include "vtkMRMLModelNode.h"
#include "vtkMRMLSelectionNode.h"
#include <vtkMRMLModelDisplayNode.h>
#include <vtkMRMLModelNode.h>
#include <vtkMRMLScene.h>

// VTK includes
#include <vtkCellArray.h>
#include <vtkCleanPolyData.h>
#include <vtkCollection.h>
#include <vtkCollectionIterator.h>
#include <vtkDoubleArray.h>
#include <vtkIntArray.h>
#include <vtkMath.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPoints.h>
#include <vtkSphereSource.h>
#include <vtkTubeFilter.h>

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
  this->CurveGenerator = vtkSmartPointer< vtkCurveGenerator >::New();
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
  vtkMRMLMarkupsNode* markupsNode = vtkMRMLMarkupsNode::SafeDownCast( inputNode );
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
  vtkMRMLMarkupsNode* inputMarkupsNode = vtkMRMLMarkupsNode::SafeDownCast( inputNode );
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
  vtkSmartPointer< vtkPolyData > outputPolyData = vtkSmartPointer< vtkPolyData >::New();
  bool cleanMarkups = markupsToModelModuleNode->GetCleanMarkups();
  bool success = false;
  int modelType = markupsToModelModuleNode->GetModelType();
  switch ( modelType )
  {
    case vtkMRMLMarkupsToModelNode::ClosedSurface:
    {
      double delaunayAlpha = markupsToModelModuleNode->GetDelaunayAlpha();
      bool smoothing = markupsToModelModuleNode->GetButterflySubdivision();
      bool forceConvex = markupsToModelModuleNode->GetConvexHull();
      success = vtkSlicerMarkupsToModelLogic::UpdateClosedSurfaceModel( controlPoints, outputPolyData, smoothing, forceConvex, delaunayAlpha, cleanMarkups );
      break;
    }
    case vtkMRMLMarkupsToModelNode::Curve:
    {
      if ( this->CurveGenerator == NULL )
      {
        vtkErrorMacro( "Curve generator is null. No operation performed." );
        return;
      }
      int tubeSegmentsBetweenControlPoints = markupsToModelModuleNode->GetTubeSegmentsBetweenControlPoints();
      bool tubeLoop = markupsToModelModuleNode->GetTubeLoop();
      bool tubeCapping = markupsToModelModuleNode->GetTubeCapping();
      double tubeRadius = markupsToModelModuleNode->GetTubeRadius();
      int tubeNumberOfSides = markupsToModelModuleNode->GetTubeNumberOfSides();
      int curveType = markupsToModelModuleNode->GetCurveType();
      int polynomialOrder = markupsToModelModuleNode->GetPolynomialOrder();
      int pointParameterType = markupsToModelModuleNode->GetPointParameterType();
      bool kochanekEndsCopyNearestDerivatives = markupsToModelModuleNode->GetKochanekEndsCopyNearestDerivatives();
      double kochanekBias = markupsToModelModuleNode->GetKochanekBias();
      double kochanekContinuity = markupsToModelModuleNode->GetKochanekContinuity();
      double kochanekTension = markupsToModelModuleNode->GetKochanekTension();
      int polynomialFitType = markupsToModelModuleNode->GetPolynomialFitType();
      double polynomialSampleWidth = markupsToModelModuleNode->GetPolynomialSampleWidth();
      int polynomialWeightType = markupsToModelModuleNode->GetPolynomialWeightType();
      success = vtkSlicerMarkupsToModelLogic::UpdateOutputCurveModel( controlPoints, outputPolyData, curveType, tubeLoop, tubeRadius, tubeNumberOfSides, tubeSegmentsBetweenControlPoints, cleanMarkups, polynomialOrder, pointParameterType, kochanekEndsCopyNearestDerivatives, kochanekBias, kochanekContinuity, kochanekTension, this->CurveGenerator, polynomialFitType, polynomialSampleWidth, polynomialWeightType, tubeCapping );
      if ( success && controlPoints->GetNumberOfPoints() > 1 )
      {
        double outputCurveLength = this->CurveGenerator->GetOutputCurveLength();
        markupsToModelModuleNode->SetOutputCurveLength( outputCurveLength );
      }
      else
      {
        markupsToModelModuleNode->SetOutputCurveLength( 0.0 );
      }
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
void vtkSlicerMarkupsToModelLogic::SetMarkupsNode( vtkMRMLMarkupsNode* newMarkups, vtkMRMLMarkupsToModelNode* moduleNode )
{
  vtkWarningMacro( "vtkSlicerMarkupsToModelLogic::SetMarkupsNode() is deprecated. Use vtkMRMLMarkupsToModelNode::SetAndObserveInputNodeID() instead." );

  if (moduleNode == NULL)
  {
    vtkWarningMacro("SetMarkupsNode: Module node is invalid");
    return;
  }

  vtkMRMLMarkupsNode* previousMarkups = vtkMRMLMarkupsNode::SafeDownCast( moduleNode->GetInputNode() );
  if ( previousMarkups == newMarkups )
  {
    // no change
    return;
  }
  // Switch to the new model node
  moduleNode->SetAndObserveInputNodeID((newMarkups != NULL) ? newMarkups->GetID() : NULL);
}

//------------------------------------------------------------------------------
bool vtkSlicerMarkupsToModelLogic::UpdateOutputCurveModel( vtkMRMLMarkupsNode* markupsNode, vtkMRMLModelNode* outputModelNode,
  int curveType, bool tubeLoop, double tubeRadius, int tubeNumberOfSides, int tubeSegmentsBetweenControlPoints,
  bool cleanMarkups, int polynomialOrder, int pointParameterType, vtkCurveGenerator* curveGenerator,
  int polynomialFitType, double polynomialSampleWidth, int polynomialWeightType, bool tubeCapping)
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
  const double defaultKochanekEndsCopyNearestDerivative = false;
  const double defaultKochanekBias = 0.0;
  const double defaultKochanekContinuity = 0.0;
  const double defaultKochanekTension = 0.0;
  bool success = vtkSlicerMarkupsToModelLogic::UpdateOutputCurveModel( controlPoints, outputPolyData, curveType, tubeLoop, tubeRadius, tubeNumberOfSides, tubeSegmentsBetweenControlPoints, cleanMarkups, polynomialOrder, pointParameterType, defaultKochanekEndsCopyNearestDerivative, defaultKochanekBias, defaultKochanekContinuity, defaultKochanekTension, curveGenerator, polynomialFitType, polynomialSampleWidth, polynomialWeightType, tubeCapping );
  if ( !success )
  {
    return false;
  }

  outputModelNode->SetAndObservePolyData( outputPolyData );

  return true;
}

//------------------------------------------------------------------------------
bool vtkSlicerMarkupsToModelLogic::UpdateOutputCurveModel( vtkPoints* controlPoints, vtkPolyData* outputPolyData,
  int curveType, bool tubeLoop, double tubeRadius, int tubeNumberOfSides, int tubeSegmentsBetweenControlPoints,
  bool cleanMarkups, int polynomialOrder, int pointParameterType,
  bool kochanekEndsCopyNearestDerivatives, double kochanekBias, double kochanekContinuity, double kochanekTension,
  vtkCurveGenerator* curveGenerator,
  int polynomialFitType, double polynomialSampleWidth, int polynomialWeightType, bool tubeCapping )
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
    vtkSlicerMarkupsToModelLogic::GenerateSphereModel( controlPoints->GetPoint( 0 ), outputPolyData, tubeRadius, tubeNumberOfSides );
    return true;
  }

  vtkSmartPointer< vtkCurveGenerator > temporaryCurveGenerator = NULL; // needed in case curveGenerator is null
  if ( curveGenerator == NULL )
  {
    temporaryCurveGenerator = vtkSmartPointer< vtkCurveGenerator >::New();
    curveGenerator = temporaryCurveGenerator;
  }
  curveGenerator->SetInputPoints( controlPoints );
  curveGenerator->SetNumberOfPointsPerInterpolatingSegment( tubeSegmentsBetweenControlPoints );
  vtkPoints* curvePoints = NULL; // temporary value

  // special case
  if ( controlPoints->GetNumberOfPoints() == 2 )
  {
    curveGenerator->SetCurveIsClosed( false ); // can't loop 2 points
    curveGenerator->SetCurveTypeToLinearSpline();
    curveGenerator->Update();
    curvePoints = curveGenerator->GetOutputPoints();
    vtkSlicerMarkupsToModelLogic::GenerateTubeModel( curvePoints, outputPolyData, tubeRadius, tubeNumberOfSides, tubeCapping );
    return true;
  }

  curveGenerator->SetCurveIsClosed( tubeLoop );
  switch ( curveType )
  {
    // Generates a polynomial curve model.
    case vtkMRMLMarkupsToModelNode::Linear:
    {
      curveGenerator->SetCurveTypeToLinearSpline();
      curveGenerator->Update();
      curvePoints = curveGenerator->GetOutputPoints();
      break;
    }
    case vtkMRMLMarkupsToModelNode::CardinalSpline:
    {
      curveGenerator->SetCurveTypeToCardinalSpline();
      curveGenerator->Update();
      curvePoints = curveGenerator->GetOutputPoints();
      break;
    }
    case vtkMRMLMarkupsToModelNode::KochanekSpline:
    {
      curveGenerator->SetCurveTypeToKochanekSpline();
      curveGenerator->SetKochanekBias( kochanekBias );
      curveGenerator->SetKochanekContinuity( kochanekContinuity );
      curveGenerator->SetKochanekTension( kochanekTension );
      curveGenerator->SetKochanekEndsCopyNearestDerivatives( kochanekEndsCopyNearestDerivatives );
      curveGenerator->Update();
      curvePoints = curveGenerator->GetOutputPoints();
      break;
    }
    case vtkMRMLMarkupsToModelNode::Polynomial:
    {
      curveGenerator->SetCurveTypeToPolynomial();
      curveGenerator->SetPolynomialOrder( polynomialOrder );
      curveGenerator->SetPolynomialSampleWidth( polynomialSampleWidth );
      switch ( pointParameterType )
      {
        case vtkMRMLMarkupsToModelNode::RawIndices:
        {
          curveGenerator->SetPolynomialPointSortingMethodToIndex();
          break;
        }
        case vtkMRMLMarkupsToModelNode::MinimumSpanningTree:
        {
          curveGenerator->SetPolynomialPointSortingMethodToMinimumSpanningTreePosition();
          break;
        }
        default:
        {
          vtkGenericWarningMacro( "Unrecognized method for generating parameters from points " << pointParameterType << ". Will use default." );
          break;
        }
      }
      switch ( polynomialFitType )
      {
        case vtkMRMLMarkupsToModelNode::GlobalLeastSquares:
        {
          curveGenerator->SetPolynomialFitMethodToGlobalLeastSquares();
          break;
        }
        case vtkMRMLMarkupsToModelNode::MovingLeastSquares:
        {
          curveGenerator->SetPolynomialFitMethodToMovingLeastSquares();
          break;
        }
        default:
        {
          vtkGenericWarningMacro( "Unrecognized method for fitting polynomial " << polynomialFitType << ". Will use default." );
          break;
        }
      }
      switch ( polynomialWeightType )
      {
        case vtkMRMLMarkupsToModelNode::Rectangular:
        {
          curveGenerator->SetPolynomialWeightFunctionToRectangular();
          break;
        }
        case vtkMRMLMarkupsToModelNode::Triangular:
        {
          curveGenerator->SetPolynomialWeightFunctionToTriangular();
          break;
        }
        case vtkMRMLMarkupsToModelNode::Cosine:
        {
          curveGenerator->SetPolynomialWeightFunctionToCosine();
          break;
        }
        case vtkMRMLMarkupsToModelNode::Gaussian:
        {
          curveGenerator->SetPolynomialWeightFunctionToGaussian();
          break;
        }
        default:
        {
          vtkGenericWarningMacro( "Unrecognized weight function " << polynomialWeightType << ". Will use default." );
          break;
        }
      }
      curveGenerator->Update();
      curvePoints = curveGenerator->GetOutputPoints();
      break;
    }
    default:
    {
      vtkGenericWarningMacro( "Unknown curve type. Aborting." );
      return false;
    }
  }

  if ( curvePoints == NULL )
  {
    vtkGenericWarningMacro( "No curve points generated. No model can be generated." );
    return false;
  }

  if ( tubeLoop && curveType != vtkMRMLMarkupsToModelNode::Polynomial ) // looping not supported for polynomials
  {
    vtkSlicerMarkupsToModelLogic::MakeLoopContinuous( curvePoints );
  }
  vtkSlicerMarkupsToModelLogic::GenerateTubeModel( curvePoints, outputPolyData, tubeRadius, tubeNumberOfSides, tubeCapping );
  return true;
}

//------------------------------------------------------------------------------
bool vtkSlicerMarkupsToModelLogic::UpdateClosedSurfaceModel(
  vtkMRMLMarkupsNode* markupsNode, vtkMRMLModelNode* outputModelNode,
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
void vtkSlicerMarkupsToModelLogic::MarkupsToPoints( vtkMRMLMarkupsNode* inputMarkupsNode, vtkPoints* outputPoints )
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

  int numberOfInputControlPoints = inputMarkupsNode->GetNumberOfControlPoints();
  outputPoints->SetNumberOfPoints( numberOfInputControlPoints );
  double inputMarkupPoint[ 3 ] = { 0.0, 0.0, 0.0 }; // values temporary
  for ( int i = 0; i < numberOfInputControlPoints; i++ )
  {
    inputMarkupsNode->GetNthControlPointPosition( i, inputMarkupPoint );
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
void vtkSlicerMarkupsToModelLogic::GenerateSphereModel( double point[ 3 ], vtkPolyData* outputSphere, double sphereRadius, int sphereNumberOfSides )
{
  if ( point == NULL )
  {
    vtkGenericWarningMacro( "Input point for sphere generation is null. No model generated." );
    return;
  }

  if ( outputSphere == NULL )
  {
    vtkGenericWarningMacro( "Output sphere poly data is null. No model generated." );
    return;
  }

  vtkSmartPointer< vtkSphereSource > sphereSource = vtkSmartPointer< vtkSphereSource >::New();
  sphereSource->SetRadius( sphereRadius );
  sphereSource->SetThetaResolution( sphereNumberOfSides );
  sphereSource->SetPhiResolution( sphereNumberOfSides );
  sphereSource->SetCenter( point );
  sphereSource->Update();

  outputSphere->DeepCopy( sphereSource->GetOutput() );
}

//------------------------------------------------------------------------------
void vtkSlicerMarkupsToModelLogic::GenerateTubeModel( vtkPoints* pointsToConnect, vtkPolyData* outputTubePolyData, double tubeRadius, int tubeNumberOfSides, bool tubeCapping)
{
  if ( pointsToConnect == NULL )
  {
    vtkGenericWarningMacro( "Points to connect is null. No model generated." );
    return;
  }

  if ( outputTubePolyData == NULL )
  {
    vtkGenericWarningMacro( "Output tube poly data is null. No model generated." );
    return;
  }

  int numPoints = pointsToConnect->GetNumberOfPoints();

  vtkSmartPointer< vtkCellArray > lineCellArray = vtkSmartPointer< vtkCellArray >::New();
  lineCellArray->InsertNextCell( numPoints );
  for ( int i = 0; i < numPoints; i++ )
  {
    lineCellArray->InsertCellPoint( i );
  }

  vtkSmartPointer< vtkPolyData > linePolyData = vtkSmartPointer< vtkPolyData >::New();
  linePolyData->Initialize();
  linePolyData->SetPoints( pointsToConnect );
  linePolyData->SetLines( lineCellArray );

  if (tubeRadius > 0.0)
  {
    vtkSmartPointer< vtkTubeFilter > tubeSegmentFilter = vtkSmartPointer< vtkTubeFilter >::New();
    tubeSegmentFilter->SetInputData( linePolyData );
    tubeSegmentFilter->SetRadius( tubeRadius );
    tubeSegmentFilter->SetNumberOfSides( tubeNumberOfSides );
    tubeSegmentFilter->SetCapping(tubeCapping);
    tubeSegmentFilter->Update();
    outputTubePolyData->DeepCopy( tubeSegmentFilter->GetOutput() );
  }
  else
  {
    outputTubePolyData->DeepCopy( linePolyData );
  }
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

//------------------------------------------------------------------------------
void vtkSlicerMarkupsToModelLogic::MakeLoopContinuous( vtkPoints* curvePoints )
{
  // Move the starting point a tiny bit and add an *extra* point to join the curve
  // to the new starting position.
  double point0[ 3 ];
  curvePoints->GetPoint( 0, point0 );
  double point1[ 3 ];
  curvePoints->GetPoint( 1, point1 );
  double finalPoint[ 3 ];
  finalPoint[ 0 ] = point0[ 0 ] * 0.5 + point1[ 0 ] * 0.5;
  finalPoint[ 1 ] = point0[ 1 ] * 0.5 + point1[ 1 ] * 0.5;
  finalPoint[ 2 ] = point0[ 2 ] * 0.5 + point1[ 2 ] * 0.5;
  curvePoints->SetPoint( 0, finalPoint );
  curvePoints->InsertNextPoint( finalPoint );
}
