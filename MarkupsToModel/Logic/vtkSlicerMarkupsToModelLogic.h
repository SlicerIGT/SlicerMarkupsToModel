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

// .NAME vtkSlicerMarkupsToModelLogic - slicer logic class to convert markups to models
// .SECTION Description
// This class manages the logic associated with selecting, adding and removing markups and converting these markups to either a:
// a) closed surface using vtkDelaunay3D triangulation
// b) piece wise connected curve. The curve can be linear, Cardinal or Kochanek Splines


#ifndef __vtkSlicerMarkupsToModelLogic_h
#define __vtkSlicerMarkupsToModelLogic_h

// Slicer includes
#include "vtkSlicerModuleLogic.h"
#include "vtkSlicerMarkupsLogic.h"

// vtk includes
#include <vtkDoubleArray.h>
#include <vtkMatrix4x4.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>

// MRML includes
#include "vtkMRMLMarkupsToModelNode.h"

// STD includes
#include <cstdlib>

#include "vtkSlicerMarkupsToModelModuleLogicExport.h"

class vtkMRMLMarkupsNode;
class vtkMRMLMarkupsToModelNode;
class vtkMRMLModelNode;
class vtkPolyData;
class vtkCurveGenerator;

/// \ingroup Slicer_QtModules_ExtensionTemplate
class VTK_SLICER_MARKUPSTOMODEL_MODULE_LOGIC_EXPORT vtkSlicerMarkupsToModelLogic :
  public vtkSlicerModuleLogic
{
public:
  static vtkSlicerMarkupsToModelLogic *New();
  vtkTypeMacro(vtkSlicerMarkupsToModelLogic, vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  vtkSlicerMarkupsLogic* MarkupsLogic;
  void ProcessMRMLNodesEvents( vtkObject* caller, unsigned long event, void* callData ) override;

  // Updates the mouse selection type to create markups or to navigate the scene.
  void UpdateSelectionNode( vtkMRMLMarkupsToModelNode* markupsToModelModuleNode );

  // Updates closed surface or curve output model from markups
  void UpdateOutputModel( vtkMRMLMarkupsToModelNode* moduleNode );

  // lower-level access to functionality for making a closed surface model
  static bool UpdateClosedSurfaceModel( vtkMRMLMarkupsNode* markupsNode, vtkMRMLModelNode* modelNode,
      bool smoothing = true, bool forceConvex = false, double delaunayAlpha = 0.0, bool cleanMarkups = true );

  static bool UpdateClosedSurfaceModel( vtkPoints* controlPoints, vtkPolyData* polyData,
    bool smoothing = true, bool forceConvex = false, double delaunayAlpha = 0.0, bool cleanMarkups = true );

  // Lower-level access to functionality for making a curve model.
  // If tubeRadius<=0.0 then a line will be created instead of a tube.
  static bool UpdateOutputCurveModel( vtkMRMLMarkupsNode* markupsNode, vtkMRMLModelNode* modelNode,
      int curveType = vtkMRMLMarkupsToModelNode::Linear,
      bool tubeLoop = false, double tubeRadius = 1.0, int tubeNumberOfSides = 8, int tubeSegmentsBetweenControlPoints = 5,
      bool cleanMarkups = true, int polynomialOrder = 3, int pointParameterType = vtkMRMLMarkupsToModelNode::RawIndices,
      vtkCurveGenerator* curveGenerator = NULL,
      int polynomialFitType = vtkMRMLMarkupsToModelNode::GlobalLeastSquares, double polynomialSampleWidth = 0.5,
      int polynomialWeightType = vtkMRMLMarkupsToModelNode::Rectangular,
      bool tubeCap = true);

  static bool UpdateOutputCurveModel( vtkPoints* controlPoints, vtkPolyData* polyData,
      int curveType = vtkMRMLMarkupsToModelNode::Linear,
      bool tubeLoop = false, double tubeRadius = 1.0, int tubeNumberOfSides = 8, int tubeSegmentsBetweenControlPoints = 5,
      bool cleanMarkups = true, int polynomialOrder = 3, int pointParameterType = vtkMRMLMarkupsToModelNode::RawIndices,
      bool kochanekEndsCopyNearestDerivative = false, double kochanekBias = 0.0,
      double kochanekContinuity = 0.0, double kochanekTension = 0.0,
      vtkCurveGenerator* curveGenerator = NULL,
      int polynomialFitType = vtkMRMLMarkupsToModelNode::GlobalLeastSquares, double polynomialSampleWidth = 0.5,
      int polynomialWeightType = vtkMRMLMarkupsToModelNode::Rectangular,
      bool tubeCap = true);

  // Get the points store in a vtkMRMLMarkupsNode
  static void MarkupsToPoints( vtkMRMLMarkupsNode* markupsNode, vtkPoints* outputPoints );

  // Get the points store in a vtkMRMLModelNode
  static void ModelToPoints( vtkMRMLModelNode* modelNode, vtkPoints* outputPoints );

  // Remove duplicate points from a vtkPoints object
  static void RemoveDuplicatePoints( vtkPoints* points );

  // DEPRECATED - Sets the input node to be processed
  void SetMarkupsNode( vtkMRMLMarkupsNode* newMarkups, vtkMRMLMarkupsToModelNode* moduleNode );

protected:
  vtkSlicerMarkupsToModelLogic();
  virtual ~vtkSlicerMarkupsToModelLogic();

  virtual void SetMRMLSceneInternal(vtkMRMLScene* newScene) override;
  /// Register MRML Node classes to Scene. Gets called automatically when the MRMLScene is attached to this logic class.
  virtual void RegisterNodes() override;
  virtual void UpdateFromMRMLScene() override;
  ///When a scene has been imported it will set the markups list and model.
  virtual void OnMRMLSceneEndImport() override;
  virtual void OnMRMLSceneStartImport() override;
  virtual void OnMRMLSceneNodeAdded(vtkMRMLNode* node) override;
  virtual void OnMRMLSceneNodeRemoved(vtkMRMLNode* node) override;

private:
  vtkSmartPointer< vtkCurveGenerator > CurveGenerator;

  // Generate a sphere at the point specified. Special case to be called when only one point is input.
  //   point - center of the sphere
  //   outputSpherePolyData - the sphere will be stored in this poly data.
  //   sphereRadius - the radius of the sphere in outputSphere.
  //   sphereNumberOfSides - The phi and theta resolution for sphere tesselation (higher = smoother).
  static void GenerateSphereModel( double point[ 3 ], vtkPolyData* outputSpherePolyData, double sphereRadius, int sphereNumberOfSides );

  // Generate a tube that passes through the points specified.
  //   points - the points that the tube passes through
  //   outputTubePolyData - the tube mesh will be stored in this poly data.
  //   tubeRadius - the radius of the tube in outputTubePolyData.
  //   tubeNumberOfSides - The resolution for tube tesselation (higher = smoother).
  static void GenerateTubeModel( vtkPoints* points, vtkPolyData* outputTubePolyData, double tubeRadius, int tubeNumberOfSides, bool tubeCapping=true );

  // If looped, the first and last segment of the curve must be exactly parallel.
  // Otherwise the curve will have two caps that don't line up and the curve will
  // not appear continuous.
  static void MakeLoopContinuous( vtkPoints* curvePoints );

  static void AssignPolyDataToOutput( vtkMRMLMarkupsToModelNode* moduleNode, vtkPolyData* polyData );

  vtkSlicerMarkupsToModelLogic(const vtkSlicerMarkupsToModelLogic&); // Not implemented
  void operator=(const vtkSlicerMarkupsToModelLogic&); // Not implemented
};

#endif
