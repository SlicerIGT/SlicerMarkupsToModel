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
#include "vtkCreateModelUtil.h"
#include "vtkCreateClosedSurfaceUtil.h"
#include "vtkCreateCurveUtil.h"

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
  
  // check if the input node is defined
  // (no need to worry about output. if not defined, it will be created later on)
  vtkMRMLNode* inputNode = markupsToModelModuleNode->GetInputNode();
  if ( inputNode == NULL )
  {
    return;
  }

  // extract the input points from the MRML node, according to its type
  if ( vtkCreateModelUtil::vtkMRMLNodeToVtkPointsSupported( inputNode ) == false )
  {
    vtkErrorMacro( "Input node type is not supported. No operation performed." );
    return;
  }

  vtkSmartPointer< vtkPoints > controlPoints = vtkSmartPointer< vtkPoints >::New();
  vtkCreateModelUtil::vtkMRMLNodeToVtkPoints( inputNode, controlPoints );

  // get rid of duplicate points
  if ( markupsToModelModuleNode->GetCleanMarkups() )
  {
    vtkCreateModelUtil::RemoveDuplicatePoints( controlPoints );
  }

  // Create the model from the points
  vtkSmartPointer<vtkPolyData> outputPolyData = vtkSmartPointer<vtkPolyData>::New();
  switch ( markupsToModelModuleNode->GetModelType() )
  {
    case vtkMRMLMarkupsToModelNode::ClosedSurface:
    {
      this->GenerateClosedSurfacePolyData( markupsToModelModuleNode, controlPoints, outputPolyData );
      break;
    }
    case vtkMRMLMarkupsToModelNode::Curve:
    {
      this->GenerateCurvePolyData( markupsToModelModuleNode, controlPoints, outputPolyData );
      break;
    }
  }

  vtkMRMLModelNode* outputModelNode = markupsToModelModuleNode->GetOutputModelNode();
  // set up the output model node if needed
  if ( outputModelNode == NULL )
  {
    if ( markupsToModelModuleNode->GetScene() == NULL )
    {
      vtkErrorMacro( "Output model node is not specified and markupsToModelModuleNode is not associated with any scene. No operation performed." );
      return;
    }
    outputModelNode = vtkMRMLModelNode::SafeDownCast( markupsToModelModuleNode->GetScene()->AddNewNodeByClass( "vtkMRMLModelNode" ) );
    if ( markupsToModelModuleNode->GetName() )
    {
      std::string outputModelNodeName = std::string( markupsToModelModuleNode->GetName() ).append( "Model" );
      outputModelNode->SetName( outputModelNodeName.c_str() );
    }
    markupsToModelModuleNode->SetAndObserveOutputModelNodeID( outputModelNode->GetID() );
  }
  if ( outputModelNode == NULL )
  {
    vtkErrorMacro( "Failed to create output model node. No operation performed." );
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
// This function assumes that none of the MRML node parameters are null
void vtkSlicerMarkupsToModelLogic::GenerateClosedSurfacePolyData( vtkMRMLMarkupsToModelNode* markupsToModelModuleNode,
                                                                  vtkPoints* controlPoints, vtkPolyData* outputPolyData )
{
  double delaunayAlpha = markupsToModelModuleNode->GetDelaunayAlpha();
  bool smoothing = markupsToModelModuleNode->GetButterflySubdivision();
  bool forceConvex = markupsToModelModuleNode->GetConvexHull();
  vtkCreateClosedSurfaceUtil::GenerateClosedSurfaceModel( controlPoints, outputPolyData, delaunayAlpha, smoothing, forceConvex );
}

//------------------------------------------------------------------------------
// This function assumes that none of the MRML node parameters are null
void vtkSlicerMarkupsToModelLogic::GenerateCurvePolyData( vtkMRMLMarkupsToModelNode* markupsToModelModuleNode,
                                                          vtkPoints* controlPoints, vtkPolyData* outputPolyData )
{
  int tubeSegmentsBetweenControlPoints = markupsToModelModuleNode->GetTubeSegmentsBetweenControlPoints();
  bool tubeLoop = markupsToModelModuleNode->GetTubeLoop();
  double tubeRadius = markupsToModelModuleNode->GetTubeRadius();
  int tubeNumberOfSides = markupsToModelModuleNode->GetTubeNumberOfSides();
  int interpolationType = markupsToModelModuleNode->GetInterpolationType();
  switch ( interpolationType )
  {
    case vtkMRMLMarkupsToModelNode::Linear:
    {
      vtkCreateCurveUtil::GenerateLinearCurveModel( controlPoints, outputPolyData,
        tubeRadius, tubeNumberOfSides, tubeSegmentsBetweenControlPoints, tubeLoop );
      break;
    }
    case vtkMRMLMarkupsToModelNode::CardinalSpline:
    {
      vtkCreateCurveUtil::GenerateCardinalCurveModel( controlPoints, outputPolyData, 
        tubeRadius, tubeNumberOfSides, tubeSegmentsBetweenControlPoints, tubeLoop );
      break;
    }
    case vtkMRMLMarkupsToModelNode::KochanekSpline:
    {
      double kochanekBias = markupsToModelModuleNode->GetKochanekBias();
      double kochanekContinuity = markupsToModelModuleNode->GetKochanekContinuity();
      double kochanekTension = markupsToModelModuleNode->GetKochanekTension();
      double kochanekEndsCopyNearestDerivatives = markupsToModelModuleNode->GetKochanekEndsCopyNearestDerivatives();
      vtkCreateCurveUtil::GenerateKochanekCurveModel( controlPoints, outputPolyData,
        tubeRadius, tubeNumberOfSides, tubeSegmentsBetweenControlPoints, tubeLoop,
        kochanekBias, kochanekContinuity, kochanekTension, kochanekEndsCopyNearestDerivatives );
      break;
    }
    case vtkMRMLMarkupsToModelNode::Polynomial:
    {
      int polynomialOrder = markupsToModelModuleNode->GetPolynomialOrder();
      vtkSmartPointer<vtkDoubleArray> controlPointParameters = vtkSmartPointer<vtkDoubleArray>::New();
      int pointParameterType = markupsToModelModuleNode->GetPointParameterType();
      switch ( pointParameterType )
      {
        case vtkMRMLMarkupsToModelNode::RawIndices:
        {
          vtkCreateCurveUtil::ComputePointParametersRawIndices( controlPoints, controlPointParameters );
          break;
        }
        case vtkMRMLMarkupsToModelNode::MinimumSpanningTree:
        {
          vtkCreateCurveUtil::ComputePointParametersMinimumSpanningTree( controlPoints, controlPointParameters );
          break;
        }
        default:
        {
          vtkWarningMacro("Invalid PointParameterType: " << pointParameterType << ". Using raw indices.");
          vtkCreateCurveUtil::ComputePointParametersRawIndices( controlPoints, controlPointParameters );
          break;
        }
      }
      vtkCreateCurveUtil::GeneratePolynomialCurveModel( controlPoints, outputPolyData,
        tubeRadius, tubeNumberOfSides, tubeSegmentsBetweenControlPoints, tubeLoop,
        polynomialOrder, controlPointParameters );
      break;
    }
  }

  return;
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
// DEPRECATED
bool vtkSlicerMarkupsToModelLogic::UpdateOutputCurveModel(vtkMRMLMarkupsFiducialNode* markupsNode, vtkMRMLModelNode* modelNode,
  int interpolationType, bool tubeLoop, double tubeRadius, int tubeNumberOfSides, int tubeSegmentsBetweenControlPoints,
  bool cleanMarkups, int polynomialOrder, int pointParameterType)
{
  vtkWarningMacro( "vtkSlicerMarkupsToModelLogic::UpdateOutputCurveModel() is deprecated. Use methods from class vtkCreateCurveUtil instead." );

  if ( markupsNode == NULL )
  {
    vtkErrorMacro( "No markups. Aborting." );
    return false;
  }
  
  if ( modelNode == NULL )
  {
    vtkErrorMacro( "No model. Aborting." );
    return false;
  }
  
  // extract control points
  vtkSmartPointer< vtkPoints > controlPoints = vtkSmartPointer< vtkPoints >::New();
  vtkCreateModelUtil::vtkMRMLNodeToVtkPoints( markupsNode, controlPoints );

  // get rid of duplicate points
  if ( cleanMarkups )
  {
    vtkCreateModelUtil::RemoveDuplicatePoints( controlPoints );
  }

  vtkSmartPointer<vtkPolyData> outputPolyData = vtkSmartPointer<vtkPolyData>::New();
  switch ( interpolationType )
  {
    // Generates a polynomial curve model.
    case vtkMRMLMarkupsToModelNode::Linear:
    {
      vtkCreateCurveUtil::GenerateLinearCurveModel( controlPoints, outputPolyData, tubeRadius, tubeNumberOfSides, tubeSegmentsBetweenControlPoints, tubeLoop );
      break;
    }
    case vtkMRMLMarkupsToModelNode::CardinalSpline:
    {
      vtkCreateCurveUtil::GenerateCardinalCurveModel( controlPoints, outputPolyData, tubeRadius, tubeNumberOfSides, tubeSegmentsBetweenControlPoints, tubeLoop );
      break;
    }
    case vtkMRMLMarkupsToModelNode::KochanekSpline:
    {
      // Changing default Kochanek spline node parameters is not useful
      // (it would only be useful to change per control point)
      const double kochanekBias = 0.0;
      const double kochanekContinuity = 0.0;
      const double kochanekTension = 0.0;
      bool kochanekEndsCopyNearestDerivatives = false;
      vtkCreateCurveUtil::GenerateKochanekCurveModel( controlPoints, outputPolyData, tubeRadius, tubeNumberOfSides, tubeSegmentsBetweenControlPoints, tubeLoop, kochanekBias, kochanekContinuity, kochanekTension, kochanekEndsCopyNearestDerivatives );
      break;
    }
    case vtkMRMLMarkupsToModelNode::Polynomial:
    {
      vtkSmartPointer< vtkDoubleArray > controlPointParameters = vtkSmartPointer< vtkDoubleArray >::New();
      switch ( pointParameterType )
      {
        case vtkMRMLMarkupsToModelNode::RawIndices:
        {
          vtkCreateCurveUtil::ComputePointParametersRawIndices( controlPoints, controlPointParameters );
          break;
        }
        case vtkMRMLMarkupsToModelNode::MinimumSpanningTree:
        {
          vtkCreateCurveUtil::ComputePointParametersMinimumSpanningTree( controlPoints, controlPointParameters );
          break;
        }
        default:
        {
          vtkErrorMacro( "Unknown point parameter type. Aborting." );
          return false;
        }
      }
      vtkCreateCurveUtil::GeneratePolynomialCurveModel( controlPoints, outputPolyData, tubeRadius, tubeNumberOfSides, tubeSegmentsBetweenControlPoints, tubeLoop, polynomialOrder );
      break;
    }
    default:
    {
      vtkErrorMacro( "Unknown interpolation type. Aborting." );
      return false;
    }
  }
  modelNode->SetAndObservePolyData( outputPolyData );

  return true;
}

//------------------------------------------------------------------------------
// DEPRECATED
bool vtkSlicerMarkupsToModelLogic::UpdateClosedSurfaceModel(
  vtkMRMLMarkupsFiducialNode* markupsNode, vtkMRMLModelNode* modelNode,
  bool smoothing, bool forceConvex, double delaunayAlpha, bool cleanMarkups )
{
  vtkWarningMacro( "vtkSlicerMarkupsToModelLogic::UpdateClosedSurfaceModel() is deprecated. Use methods from class vtkCreateClosedSurfaceUtil instead." );

  if ( markupsNode == NULL )
  {
    vtkErrorMacro( "No markups. Aborting." );
    return false;
  }
  
  if ( modelNode == NULL )
  {
    vtkErrorMacro( "No model. Aborting." );
    return false;
  }
  
  // extract control points
  vtkSmartPointer< vtkPoints > controlPoints = vtkSmartPointer< vtkPoints >::New();
  vtkCreateModelUtil::vtkMRMLNodeToVtkPoints( markupsNode, controlPoints );

  // get rid of duplicate points
  if ( cleanMarkups )
  {
    vtkCreateModelUtil::RemoveDuplicatePoints( controlPoints );
  }

  vtkSmartPointer<vtkPolyData> outputPolyData = vtkSmartPointer<vtkPolyData>::New();
  vtkCreateClosedSurfaceUtil::GenerateClosedSurfaceModel( controlPoints, outputPolyData, delaunayAlpha, smoothing, forceConvex );
  modelNode->SetAndObservePolyData( outputPolyData );

  return true;
}
