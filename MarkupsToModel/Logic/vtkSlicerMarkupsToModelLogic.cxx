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

static const double CLEAN_POLYDATA_TOLERANCE_MM = 0.01;

#include "vtkCreateClosedSurfaceUtil.h"
#include "vtkCreateCurveUtil.h"

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
void vtkSlicerMarkupsToModelLogic::SetMarkupsNode(vtkMRMLMarkupsFiducialNode* newMarkups, vtkMRMLMarkupsToModelNode* moduleNode)
{
  if (moduleNode == NULL)
  {
    vtkWarningMacro("SetWatchedModelNode: Module node is invalid");
    return;
  }

  vtkMRMLMarkupsFiducialNode* previousMarkups = moduleNode->GetMarkupsNode();
  if (previousMarkups == newMarkups)
  {
    // no change
    return;
  }
  // Switch to the new model node
  moduleNode->SetAndObserveMarkupsNodeID((newMarkups != NULL) ? newMarkups->GetID() : NULL);
}

//------------------------------------------------------------------------------
void vtkSlicerMarkupsToModelLogic::UpdateSelectionNode(vtkMRMLMarkupsToModelNode* markupsToModelModuleNode)
{
  if (!markupsToModelModuleNode)
  {
    // No markups selected
    return;
  }
  vtkMRMLMarkupsFiducialNode* markupsNode = markupsToModelModuleNode->GetMarkupsNode();
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
  if (markupsToModelModuleNode->GetModelNode() == NULL || markupsToModelModuleNode->GetMarkupsNode() == NULL)
  {
    return;
  }

  switch (markupsToModelModuleNode->GetModelType())
  {
    case vtkMRMLMarkupsToModelNode::ClosedSurface:
    {
      this->UpdateClosedSurfaceModel(markupsToModelModuleNode);
      break;
    }
    case vtkMRMLMarkupsToModelNode::Curve:
    {
      this->UpdateCurveModel(markupsToModelModuleNode);
      break;
    }
  }

  vtkMRMLModelNode* modelNode = markupsToModelModuleNode->GetModelNode();
  if (modelNode)
  {
    vtkMRMLModelDisplayNode* displayNode = vtkMRMLModelDisplayNode::SafeDownCast(modelNode->GetDisplayNode());
    if (displayNode == NULL)
    {
      modelNode->CreateDefaultDisplayNodes();
      displayNode = vtkMRMLModelDisplayNode::SafeDownCast(modelNode->GetDisplayNode());
      std::string name = std::string(modelNode->GetName()).append("ModelDisplay");
      displayNode->SetName(name.c_str());
    }
  }
}

//------------------------------------------------------------------------------
void vtkSlicerMarkupsToModelLogic::ProcessMRMLNodesEvents(vtkObject* caller, unsigned long event, void* /*callData*/ )
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
bool vtkSlicerMarkupsToModelLogic::UpdateClosedSurfaceModel(vtkMRMLMarkupsToModelNode* markupsToModelModuleNode)
{
  if (markupsToModelModuleNode == NULL)
  {
    vtkGenericWarningMacro("No markupsToModelModuleNode provided to UpdateClosedSurfaceModel. No operation performed.");
    return false;
  }

  vtkMRMLMarkupsFiducialNode* markupsNode = markupsToModelModuleNode->GetMarkupsNode();
  if (markupsNode == NULL)
  {
    vtkGenericWarningMacro("No markups node is defined in markupsToModelModuleNode.");
    return false;
  }

  vtkMRMLModelNode* modelNode = markupsToModelModuleNode->GetModelNode();
  if (modelNode == NULL)
  {
    if (markupsToModelModuleNode->GetScene() == NULL)
    {
      vtkGenericWarningMacro("Output model node is not specified and markupsToModelModuleNode is not associated with any scene.");
      return false;
    }
    modelNode = vtkMRMLModelNode::SafeDownCast(markupsToModelModuleNode->GetScene()->AddNewNodeByClass("vtkMRMLModelNode"));
    if (markupsToModelModuleNode->GetName())
    {
      std::string modelNodeName = std::string(markupsToModelModuleNode->GetName()).append("Model");
      modelNode->SetName(modelNodeName.c_str());
    }
    markupsToModelModuleNode->SetAndObserveModelNodeID(modelNode->GetID());
  }
  
  bool cleanMarkups = markupsToModelModuleNode->GetCleanMarkups();
  vtkSmartPointer<vtkPoints> controlPoints = vtkSmartPointer<vtkPoints>::New();
  vtkCreateCurveUtil::MarkupsToPoints(markupsNode, controlPoints, cleanMarkups);

  vtkSmartPointer<vtkPolyData> outputPolyData = vtkSmartPointer<vtkPolyData>::New();
  double delaunayAlpha = markupsToModelModuleNode->GetDelaunayAlpha();
  bool smoothing = markupsToModelModuleNode->GetButterflySubdivision();
  bool forceConvex = markupsToModelModuleNode->GetConvexHull();
  vtkCreateClosedSurfaceUtil::GenerateCloseSurfaceModel(controlPoints, outputPolyData, delaunayAlpha, smoothing, forceConvex);

  modelNode->SetAndObservePolyData(outputPolyData);
  return true;
}

//------------------------------------------------------------------------------
bool vtkSlicerMarkupsToModelLogic::UpdateCurveModel(vtkMRMLMarkupsToModelNode* markupsToModelModuleNode)
{
  if (markupsToModelModuleNode == NULL)
  {
    vtkGenericWarningMacro("No markupsToModelModuleNode provided to UpdateCurveModel. No operation performed.");
    return false;
  }

  vtkMRMLMarkupsFiducialNode* markupsNode = markupsToModelModuleNode->GetMarkupsNode();
  if (markupsNode == NULL)
  {
    vtkGenericWarningMacro("No markups node is defined in markupsToModelModuleNode.");
    return false;
  }

  vtkMRMLModelNode* modelNode = markupsToModelModuleNode->GetModelNode();
  if (modelNode == NULL)
  {
    if (markupsToModelModuleNode->GetScene() == NULL)
    {
      vtkGenericWarningMacro("Output model node is not specified and markupsToModelModuleNode is not associated with any scene.");
      return false;
    }
    modelNode = vtkMRMLModelNode::SafeDownCast(markupsToModelModuleNode->GetScene()->AddNewNodeByClass("vtkMRMLModelNode"));
    if (markupsToModelModuleNode->GetName())
    {
      std::string modelNodeName = std::string(markupsToModelModuleNode->GetName()).append("Model");
      modelNode->SetName(modelNodeName.c_str());
    }
    markupsToModelModuleNode->SetAndObserveModelNodeID(modelNode->GetID());
  }

  bool cleanMarkups = markupsToModelModuleNode->GetCleanMarkups();
  vtkSmartPointer<vtkPoints> controlPoints = vtkSmartPointer<vtkPoints>::New();
  vtkCreateCurveUtil::MarkupsToPoints(markupsNode, controlPoints, cleanMarkups);

  int tubeSegmentsBetweenControlPoints = markupsToModelModuleNode->GetTubeSegmentsBetweenControlPoints();
  bool tubeLoop = markupsToModelModuleNode->GetTubeLoop();
  double tubeRadius = markupsToModelModuleNode->GetTubeRadius();
  int tubeNumberOfSides = markupsToModelModuleNode->GetTubeNumberOfSides();
  vtkSmartPointer<vtkPolyData> outputPolyData = vtkSmartPointer<vtkPolyData>::New();
  int interpolationType = markupsToModelModuleNode->GetInterpolationType();
  switch (interpolationType)
  {
    case vtkMRMLMarkupsToModelNode::Linear:
    {
      vtkCreateCurveUtil::GenerateLinearCurveModel(controlPoints, outputPolyData,
        tubeRadius, tubeNumberOfSides, tubeSegmentsBetweenControlPoints, tubeLoop);
      break;
    }
    case vtkMRMLMarkupsToModelNode::CardinalSpline:
    {
      vtkCreateCurveUtil::GenerateCardinalCurveModel(controlPoints, outputPolyData, 
        tubeRadius, tubeNumberOfSides, tubeSegmentsBetweenControlPoints, tubeLoop);
      break;
    }
    case vtkMRMLMarkupsToModelNode::KochanekSpline:
    {
      double kochanekBias = markupsToModelModuleNode->GetKochanekBias();
      double kochanekContinuity = markupsToModelModuleNode->GetKochanekContinuity();
      double kochanekTension = markupsToModelModuleNode->GetKochanekTension();
      double kochanekEndsCopyNearestDerivatives = markupsToModelModuleNode->GetKochanekEndsCopyNearestDerivatives();
      vtkCreateCurveUtil::GenerateKochanekCurveModel(controlPoints, outputPolyData,
        tubeRadius, tubeNumberOfSides, tubeSegmentsBetweenControlPoints, tubeLoop,
        kochanekBias, kochanekContinuity, kochanekTension, kochanekEndsCopyNearestDerivatives);
      break;
    }
    case vtkMRMLMarkupsToModelNode::Polynomial:
    {
      int polynomialOrder = markupsToModelModuleNode->GetPolynomialOrder();
      vtkSmartPointer<vtkDoubleArray> controlPointParameters = vtkSmartPointer<vtkDoubleArray>::New();
      int pointParameterType = markupsToModelModuleNode->GetPointParameterType();
      switch (pointParameterType)
      {
        case vtkMRMLMarkupsToModelNode::RawIndices:
        {
          vtkCreateCurveUtil::ComputePointParametersRawIndices(controlPoints, controlPointParameters);
          break;
        }
        case vtkMRMLMarkupsToModelNode::MinimumSpanningTree:
        {
          vtkCreateCurveUtil::ComputePointParametersMinimumSpanningTree(controlPoints, controlPointParameters);
          break;
        }
        default:
        {
          vtkGenericWarningMacro("Invalid PointParameterType: " << pointParameterType << ". Using raw indices.");
          vtkCreateCurveUtil::ComputePointParametersRawIndices(controlPoints, controlPointParameters);
          break;
        }
      }
      vtkCreateCurveUtil::GeneratePolynomialCurveModel(controlPoints, outputPolyData,
        tubeRadius, tubeNumberOfSides, tubeSegmentsBetweenControlPoints, tubeLoop,
        polynomialOrder, controlPointParameters);
      break;
    }
  }

  modelNode->SetAndObservePolyData(outputPolyData);
  return true;
}
