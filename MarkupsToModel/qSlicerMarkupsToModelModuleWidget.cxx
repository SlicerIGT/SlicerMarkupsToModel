/*==============================================================================

  Program: 3D Slicer

  Portions ( c ) Copyright Brigham and Women's Hospital ( BWH ) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

==============================================================================*/

// Qt includes
#include <QtGui>
#include <QDebug>
#include <QButtonGroup>

#include "qSlicerApplication.h"

// SlicerQt includes
#include "qSlicerMarkupsToModelModuleWidget.h"
#include "ui_qSlicerMarkupsToModelModuleWidget.h"
#include "qSlicerSimpleMarkupsWidget.h"

// Slicer includes
#include "vtkMRMLModelDisplayNode.h"
#include "vtkMRMLMarkupsDisplayNode.h"
#include "vtkMRMLDisplayNode.h"
#include "vtkMRMLModelNode.h"
#include "vtkMRMLMarkupsNode.h"
#include "vtkMRMLInteractionNode.h"

// module includes
#include "vtkMRMLMarkupsToModelNode.h"
#include "vtkSlicerMarkupsToModelLogic.h"

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_ExtensionTemplate
class qSlicerMarkupsToModelModuleWidgetPrivate : public Ui_qSlicerMarkupsToModelModuleWidget
{
  Q_DECLARE_PUBLIC(qSlicerMarkupsToModelModuleWidget);

protected:
  qSlicerMarkupsToModelModuleWidget* const q_ptr;
public:
  qSlicerMarkupsToModelModuleWidgetPrivate(qSlicerMarkupsToModelModuleWidget& object);
  vtkSlicerMarkupsToModelLogic* logic() const;

  QButtonGroup modeButtonGroup;

  // Observed nodes (to keep GUI up-to-date)
  vtkWeakPointer<vtkMRMLMarkupsToModelNode> MarkupsToModelNode;
  vtkWeakPointer<vtkMRMLMarkupsDisplayNode> MarkupsDisplayNode;
  vtkWeakPointer<vtkMRMLModelDisplayNode> ModelDisplayNode;
};

//-----------------------------------------------------------------------------
// qSlicerMarkupsToModelModuleWidgetPrivate methods
//-----------------------------------------------------------------------------
qSlicerMarkupsToModelModuleWidgetPrivate::qSlicerMarkupsToModelModuleWidgetPrivate(qSlicerMarkupsToModelModuleWidget& object) : q_ptr(&object)
{
}

//-----------------------------------------------------------------------------
vtkSlicerMarkupsToModelLogic* qSlicerMarkupsToModelModuleWidgetPrivate::logic() const
{
  Q_Q(const qSlicerMarkupsToModelModuleWidget);
  return vtkSlicerMarkupsToModelLogic::SafeDownCast(q->logic());
}

//-----------------------------------------------------------------------------
// qSlicerMarkupsToModelModuleWidget methods
//-----------------------------------------------------------------------------
qSlicerMarkupsToModelModuleWidget::qSlicerMarkupsToModelModuleWidget(QWidget* _parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerMarkupsToModelModuleWidgetPrivate(*this))
{
}

//-----------------------------------------------------------------------------
qSlicerMarkupsToModelModuleWidget::~qSlicerMarkupsToModelModuleWidget()
{
}

//-----------------------------------------------------------------------------
void qSlicerMarkupsToModelModuleWidget::setup()
{
  Q_D(qSlicerMarkupsToModelModuleWidget);
  d->setupUi( this );
  this->Superclass::setup();

  d->modeButtonGroup.addButton( d->ModeClosedSurfaceRadioButton );
  d->modeButtonGroup.addButton( d->ModeCurveRadioButton );

  this->setMRMLScene( d->logic()->GetMRMLScene() );

  connect( d->ParameterNodeSelector, SIGNAL( currentNodeChanged( vtkMRMLNode* ) ), this, SLOT( onParameterNodeSelectionChanged() ) );
  connect( d->ModelNodeSelector, SIGNAL( currentNodeChanged( vtkMRMLNode* ) ), this, SLOT( onOutputModelComboBoxSelectionChanged( vtkMRMLNode*) ) );
  connect( d->ModelNodeSelector, SIGNAL( nodeAddedByUser( vtkMRMLNode* ) ), this, SLOT( onOutputModelComboBoxNodeAdded( vtkMRMLNode* ) ) );
  connect( d->InputNodeSelector, SIGNAL( currentNodeChanged( vtkMRMLNode* ) ), this, SLOT( onInputNodeComboBoxSelectionChanged( vtkMRMLNode* ) ) );
  connect( d->InputNodeSelector, SIGNAL( nodeAddedByUser( vtkMRMLNode* ) ), this, SLOT( onInputNodeComboBoxNodeAdded( vtkMRMLNode* ) ) );
  connect( d->UpdateButton, SIGNAL( clicked() ), this, SLOT( onUpdateButtonClicked() ) );
  connect( d->UpdateButton, SIGNAL( checkBoxToggled( bool ) ), this, SLOT( onUpdateButtonCheckboxToggled( bool ) ) );

  connect(d->ButterflySubdivisionCheckBox, SIGNAL(toggled(bool)), this, SLOT(updateMRMLFromGUI()));
  connect(d->ConvexHullCheckBox, SIGNAL(toggled(bool)), this, SLOT(updateMRMLFromGUI()));
  connect(d->CleanDuplicateInputPointsCheckbox, SIGNAL(toggled(bool)), this, SLOT(updateMRMLFromGUI()));

  connect(d->ModeClosedSurfaceRadioButton, SIGNAL(clicked()), this, SLOT(updateMRMLFromGUI()));
  connect(d->ModeCurveRadioButton, SIGNAL(clicked()), this, SLOT(updateMRMLFromGUI()));
  connect(d->DelaunayAlphaDoubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(updateMRMLFromGUI()));
  connect(d->TubeRadiusDoubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(updateMRMLFromGUI()));
  connect(d->TubeSegmentsSpinBox, SIGNAL(valueChanged(double)), this, SLOT(updateMRMLFromGUI()));
  connect(d->TubeSidesSpinBox, SIGNAL(valueChanged(double)), this, SLOT(updateMRMLFromGUI()));
  connect(d->TubeLoopCheckBox, SIGNAL(clicked()), this, SLOT(updateMRMLFromGUI()));
  connect(d->TubeCappingCheckBox, SIGNAL(clicked()), this, SLOT(updateMRMLFromGUI()));

  connect(d->KochanekEndsCopyNearestDerivativesCheckBox, SIGNAL(clicked()), this, SLOT(updateMRMLFromGUI()));
  connect(d->KochanekBiasDoubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(updateMRMLFromGUI()));
  connect(d->KochanekContinuityDoubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(updateMRMLFromGUI()));
  connect(d->KochanekTensionDoubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(updateMRMLFromGUI()));

  connect(d->PointSortingIndicesRadioButton, SIGNAL(clicked()), this, SLOT(updateMRMLFromGUI()));
  connect(d->PointSortingMinimumSpanningTreeRadioButton, SIGNAL(clicked()), this, SLOT(updateMRMLFromGUI()));
  connect(d->PolynomialOrderSpinBox, SIGNAL(valueChanged(double)), this, SLOT(updateMRMLFromGUI()));
  connect(d->PolynomialSampleWidthDoubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(updateMRMLFromGUI()));
  connect(d->WeightFunctionRectangularRadioButton, SIGNAL(clicked()), this, SLOT(updateMRMLFromGUI()));
  connect(d->WeightFunctionTriangularRadioButton, SIGNAL(clicked()), this, SLOT(updateMRMLFromGUI()));
  connect(d->WeightFunctionCosineRadioButton, SIGNAL(clicked()), this, SLOT(updateMRMLFromGUI()));
  connect(d->WeightFunctionGaussianRadioButton, SIGNAL(clicked()), this, SLOT(updateMRMLFromGUI()));

  connect(d->ModelOpacitySlider, SIGNAL(valueChanged(double)), this, SLOT(updateMRMLFromGUI()));
  connect(d->ModelColorSelector, SIGNAL(clicked()), this, SLOT(updateMRMLFromGUI()));
  connect(d->ModelVisiblityButton, SIGNAL(toggled(bool)), this, SLOT(updateMRMLFromGUI()));
  connect(d->ModelSliceIntersectionCheckbox, SIGNAL(toggled(bool)), this, SLOT(updateMRMLFromGUI()));
  connect(d->MarkupsTextScaleSlider, SIGNAL(valueChanged(double)), this, SLOT(updateMRMLFromGUI()));

  connect(d->LinearInterpolationRadioButton, SIGNAL(clicked()), this, SLOT(updateMRMLFromGUI()));
  connect(d->CardinalInterpolationRadioButton, SIGNAL(clicked()), this, SLOT(updateMRMLFromGUI()));
  connect(d->KochanekInterpolationRadioButton, SIGNAL(clicked()), this, SLOT(updateMRMLFromGUI()));
  connect(d->GlobalLeastSquaresPolynomialApproximationRadioButton, SIGNAL(clicked()), this, SLOT(updateMRMLFromGUI()));
  connect(d->MovingLeastSquaresPolynomialApproximationRadioButton, SIGNAL(clicked()), this, SLOT(updateMRMLFromGUI()));
}

//-----------------------------------------------------------------------------
void qSlicerMarkupsToModelModuleWidget::enter()
{
  Q_D(qSlicerMarkupsToModelModuleWidget);
  this->Superclass::enter();

  if (this->mrmlScene() == NULL)
  {
    qCritical() << "Invalid scene!";
    return;
  }

  // For convenience, select a default parameter node.
  if (d->ParameterNodeSelector->currentNode() == NULL)
  {
    vtkMRMLNode* node = this->mrmlScene()->GetNthNodeByClass(0, "vtkMRMLMarkupsToModelNode");
    if (node == NULL)
    {
      node = this->mrmlScene()->AddNewNodeByClass("vtkMRMLMarkupsToModelNode");
    }
    // Create a new parameter node if there is none in the scene.
    if (node == NULL)
    {
      qCritical("Failed to create module node");
      return;
    }
    d->ParameterNodeSelector->setCurrentNode(node);
  }

  // Need to update the GUI so that it observes whichever parameter node is selected
  this->onParameterNodeSelectionChanged();
}

//-----------------------------------------------------------------------------
void qSlicerMarkupsToModelModuleWidget::onParameterNodeSelectionChanged()
{
  Q_D(qSlicerMarkupsToModelModuleWidget);
  vtkMRMLMarkupsToModelNode* selectedMarkupsToModelNode = vtkMRMLMarkupsToModelNode::SafeDownCast(d->ParameterNodeSelector->currentNode());
  qvtkReconnect(d->MarkupsToModelNode, selectedMarkupsToModelNode, vtkCommand::ModifiedEvent, this, SLOT(updateGUIFromMRML()));
  d->MarkupsToModelNode = selectedMarkupsToModelNode;
  d->logic()->UpdateSelectionNode(d->MarkupsToModelNode);
  this->updateGUIFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerMarkupsToModelModuleWidget::exit()
{
  Superclass::exit();
}

//-----------------------------------------------------------------------------
void qSlicerMarkupsToModelModuleWidget::setMRMLScene(vtkMRMLScene* scene)
{
  Q_D(qSlicerMarkupsToModelModuleWidget);
  this->Superclass::setMRMLScene(scene);
  qvtkReconnect(d->logic(), scene, vtkMRMLScene::EndImportEvent, this, SLOT(onSceneImportedEvent()));
}

//-----------------------------------------------------------------------------
void qSlicerMarkupsToModelModuleWidget::onSceneImportedEvent()
{
  this->updateGUIFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerMarkupsToModelModuleWidget::onOutputModelComboBoxSelectionChanged( vtkMRMLNode* newNode )
{
  Q_D(qSlicerMarkupsToModelModuleWidget);

  vtkMRMLMarkupsToModelNode* markupsToModelModuleNode = vtkMRMLMarkupsToModelNode::SafeDownCast(d->ParameterNodeSelector->currentNode());
  if (markupsToModelModuleNode == NULL)
  {
    qCritical() << Q_FUNC_INFO << ": invalid markupsToModelModuleNode";
    return;
  }

  vtkMRMLModelNode* outputModelNode = vtkMRMLModelNode::SafeDownCast( newNode );
  markupsToModelModuleNode->SetAndObserveOutputModelNodeID( outputModelNode ? outputModelNode->GetID() : NULL );

  // Observe display node so that we can make sure the module GUI always shows up-to-date information
  vtkMRMLModelDisplayNode* outputModelDisplayNode = NULL; // temporary value
  if ( outputModelNode != NULL )
  {
    outputModelNode->CreateDefaultDisplayNodes();
    outputModelDisplayNode = vtkMRMLModelDisplayNode::SafeDownCast(outputModelNode->GetDisplayNode());
  }
  qvtkReconnect(d->ModelDisplayNode, outputModelDisplayNode, vtkCommand::ModifiedEvent, this, SLOT(updateGUIFromMRML()));
  d->ModelDisplayNode = outputModelDisplayNode;

  this->updateGUIFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerMarkupsToModelModuleWidget::onOutputModelComboBoxNodeAdded( vtkMRMLNode* addedNode )
{
  vtkMRMLModelNode* modelNode = vtkMRMLModelNode::SafeDownCast(addedNode);
  if (modelNode == NULL)
  {
    qCritical() << Q_FUNC_INFO << "failed: invalid node";
    return;
  }

  modelNode->CreateDefaultDisplayNodes();
  vtkMRMLModelDisplayNode* displayNode = vtkMRMLModelDisplayNode::SafeDownCast(modelNode->GetDisplayNode());
  if (displayNode)
  {
    displayNode->SetColor(1, 1, 0);
    displayNode->Visibility2DOn();
    displayNode->SetSliceIntersectionThickness(2);
  }
}

//-----------------------------------------------------------------------------
void qSlicerMarkupsToModelModuleWidget::onInputNodeComboBoxSelectionChanged( vtkMRMLNode* newNode )
{
  Q_D( qSlicerMarkupsToModelModuleWidget );

  vtkMRMLMarkupsToModelNode* markupsToModelModuleNode = vtkMRMLMarkupsToModelNode::SafeDownCast( d->ParameterNodeSelector->currentNode() );
  if ( markupsToModelModuleNode == NULL )
  {
    qCritical() << Q_FUNC_INFO << ": invalid markupsToModelModuleNode";
    return;
  }

  if ( newNode == NULL )
  {
    markupsToModelModuleNode->SetAndObserveInputNodeID( NULL );
  }

  vtkMRMLMarkupsNode* inputMarkupsNode = vtkMRMLMarkupsNode::SafeDownCast( newNode );
  vtkMRMLModelNode* inputModelNode = vtkMRMLModelNode::SafeDownCast( newNode );
  if ( inputMarkupsNode != NULL )
  {
    markupsToModelModuleNode->SetAndObserveInputNodeID( inputMarkupsNode->GetID() );

    // Observe display node so that we can make sure the module GUI always shows up-to-date information
    // (applies specifically to markups)
    inputMarkupsNode->CreateDefaultDisplayNodes();
    vtkMRMLMarkupsDisplayNode* inputMarkupsDisplayNode = vtkMRMLMarkupsDisplayNode::SafeDownCast( inputMarkupsNode->GetDisplayNode() );
    qvtkReconnect( d->MarkupsDisplayNode, inputMarkupsDisplayNode, vtkCommand::ModifiedEvent, this, SLOT( updateGUIFromMRML() ) );
    d->MarkupsDisplayNode = inputMarkupsDisplayNode;
  }
  else if ( inputModelNode != NULL )
  {
    markupsToModelModuleNode->SetAndObserveInputNodeID( inputModelNode->GetID() );
  }
  else
  {
    markupsToModelModuleNode->SetAndObserveInputNodeID( NULL );
    qCritical() << Q_FUNC_INFO << ": unexpected input node type";
  }

  this->updateGUIFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerMarkupsToModelModuleWidget::onInputNodeComboBoxNodeAdded( vtkMRMLNode* addedNode )
{
  vtkMRMLMarkupsNode* inputMarkupsNode = vtkMRMLMarkupsNode::SafeDownCast( addedNode );
  vtkMRMLModelNode* inputModelNode = vtkMRMLModelNode::SafeDownCast( addedNode );
  if ( inputMarkupsNode != NULL )
  {
    inputMarkupsNode->CreateDefaultDisplayNodes();
    vtkMRMLMarkupsDisplayNode* inputMarkupsDisplayNode = vtkMRMLMarkupsDisplayNode::SafeDownCast( inputMarkupsNode->GetDisplayNode() );
    if ( inputMarkupsDisplayNode )
    {
      inputMarkupsDisplayNode->SetTextScale(0.0);
    }
  }
  else if ( inputModelNode != NULL )
  {
    inputModelNode->CreateDefaultDisplayNodes();
  }
}

//-----------------------------------------------------------------------------
void qSlicerMarkupsToModelModuleWidget::updateMRMLFromGUI()
{
  Q_D(qSlicerMarkupsToModelModuleWidget);

  vtkMRMLMarkupsToModelNode* markupsToModelModuleNode = vtkMRMLMarkupsToModelNode::SafeDownCast(d->ParameterNodeSelector->currentNode());
  if (markupsToModelModuleNode == NULL)
  {
    qCritical("Selected node not a valid module node");
    return;
  }

  int markupsToModelModuleNodeWasModified = markupsToModelModuleNode->StartModify();
  if (d->ModeClosedSurfaceRadioButton->isChecked())
  {
    markupsToModelModuleNode->SetModelType(vtkMRMLMarkupsToModelNode::ClosedSurface);
  }
  else if (d->ModeCurveRadioButton->isChecked())
  {
    markupsToModelModuleNode->SetModelType(vtkMRMLMarkupsToModelNode::Curve);
  }
  else
  {
    qCritical("Invalid markups to model mode selected.");
  }
  markupsToModelModuleNode->SetAutoUpdateOutput(d->UpdateButton->isChecked());

  markupsToModelModuleNode->SetCleanMarkups(d->CleanDuplicateInputPointsCheckbox->isChecked());
  markupsToModelModuleNode->SetDelaunayAlpha(d->DelaunayAlphaDoubleSpinBox->value());
  markupsToModelModuleNode->SetConvexHull(d->ConvexHullCheckBox->isChecked());
  markupsToModelModuleNode->SetButterflySubdivision(d->ButterflySubdivisionCheckBox->isChecked());

  markupsToModelModuleNode->SetTubeRadius(d->TubeRadiusDoubleSpinBox->value());
  markupsToModelModuleNode->SetTubeSegmentsBetweenControlPoints(d->TubeSegmentsSpinBox->value());
  markupsToModelModuleNode->SetTubeNumberOfSides(d->TubeSidesSpinBox->value());
  markupsToModelModuleNode->SetTubeLoop(d->TubeLoopCheckBox->isChecked());
  markupsToModelModuleNode->SetTubeCapping(d->TubeCappingCheckBox->isChecked());
  if (d->LinearInterpolationRadioButton->isChecked())
  {
    markupsToModelModuleNode->SetCurveType(vtkMRMLMarkupsToModelNode::Linear);
  }
  else if (d->CardinalInterpolationRadioButton->isChecked())
  {
    markupsToModelModuleNode->SetCurveType(vtkMRMLMarkupsToModelNode::CardinalSpline);
  }
  else if (d->KochanekInterpolationRadioButton->isChecked())
  {
    markupsToModelModuleNode->SetCurveType(vtkMRMLMarkupsToModelNode::KochanekSpline);
  }
  else if (d->GlobalLeastSquaresPolynomialApproximationRadioButton->isChecked())
  {
    markupsToModelModuleNode->SetCurveType(vtkMRMLMarkupsToModelNode::Polynomial);
    markupsToModelModuleNode->SetPolynomialFitType( vtkMRMLMarkupsToModelNode::GlobalLeastSquares );
  }
  else if (d->MovingLeastSquaresPolynomialApproximationRadioButton->isChecked())
  {
    markupsToModelModuleNode->SetCurveType(vtkMRMLMarkupsToModelNode::Polynomial);
    markupsToModelModuleNode->SetPolynomialFitType( vtkMRMLMarkupsToModelNode::MovingLeastSquares );
  }
  markupsToModelModuleNode->SetKochanekEndsCopyNearestDerivatives(d->KochanekEndsCopyNearestDerivativesCheckBox->isChecked());
  markupsToModelModuleNode->SetKochanekBias(d->KochanekBiasDoubleSpinBox->value());
  markupsToModelModuleNode->SetKochanekContinuity(d->KochanekContinuityDoubleSpinBox->value());
  markupsToModelModuleNode->SetKochanekTension(d->KochanekTensionDoubleSpinBox->value());

  if (d->PointSortingIndicesRadioButton->isChecked())
  {
    markupsToModelModuleNode->SetPointParameterType(vtkMRMLMarkupsToModelNode::RawIndices);
  }
  else if (d->PointSortingMinimumSpanningTreeRadioButton->isChecked() )
  {
    markupsToModelModuleNode->SetPointParameterType(vtkMRMLMarkupsToModelNode::MinimumSpanningTree);
  }
  markupsToModelModuleNode->SetPolynomialOrder(d->PolynomialOrderSpinBox->value());

  markupsToModelModuleNode->SetPolynomialSampleWidth( d->PolynomialSampleWidthDoubleSpinBox->value() );
  if ( d->WeightFunctionRectangularRadioButton->isChecked() )
  {
    markupsToModelModuleNode->SetPolynomialWeightType( vtkMRMLMarkupsToModelNode::Rectangular );
  }
  else if ( d->WeightFunctionTriangularRadioButton->isChecked() )
  {
    markupsToModelModuleNode->SetPolynomialWeightType( vtkMRMLMarkupsToModelNode::Triangular );
  }
  else if ( d->WeightFunctionCosineRadioButton->isChecked() )
  {
    markupsToModelModuleNode->SetPolynomialWeightType( vtkMRMLMarkupsToModelNode::Cosine );
  }
  else if ( d->WeightFunctionGaussianRadioButton->isChecked() )
  {
    markupsToModelModuleNode->SetPolynomialWeightType( vtkMRMLMarkupsToModelNode::Gaussian );
  }

  markupsToModelModuleNode->EndModify(markupsToModelModuleNodeWasModified);

  vtkMRMLModelDisplayNode* modelDisplayNode = vtkMRMLModelDisplayNode::SafeDownCast(
    this->GetOutputModelNode() ? this->GetOutputModelNode()->GetDisplayNode() : NULL);
  if (modelDisplayNode != NULL)
  {
    int modelDisplayNodeWasModified = modelDisplayNode->StartModify();
    modelDisplayNode->SetVisibility(d->ModelVisiblityButton->isChecked());
    modelDisplayNode->SetOpacity(d->ModelOpacitySlider->value());
    modelDisplayNode->SetVisibility2D(d->ModelSliceIntersectionCheckbox->isChecked());
    modelDisplayNode->SetColor(d->ModelColorSelector->color().redF(), d->ModelColorSelector->color().greenF(), d->ModelColorSelector->color().blueF());
    modelDisplayNode->EndModify(modelDisplayNodeWasModified);
  }

  vtkMRMLMarkupsNode* inputMarkupsNode = vtkMRMLMarkupsNode::SafeDownCast( this->GetInputNode() );
  if ( inputMarkupsNode != NULL )
  {
    vtkMRMLMarkupsDisplayNode* inputMarkupsDisplayNode = vtkMRMLMarkupsDisplayNode::SafeDownCast( inputMarkupsNode->GetDisplayNode() );
    if ( inputMarkupsDisplayNode != NULL )
    {
      inputMarkupsDisplayNode->SetTextScale( d->MarkupsTextScaleSlider->value() );
    }
  }
}

//-----------------------------------------------------------------------------
void qSlicerMarkupsToModelModuleWidget::updateGUIFromMRML()
{
  Q_D(qSlicerMarkupsToModelModuleWidget);

  vtkMRMLMarkupsToModelNode* markupsToModelModuleNode = vtkMRMLMarkupsToModelNode::SafeDownCast( d->ParameterNodeSelector->currentNode() );
  if (markupsToModelModuleNode == NULL)
  {
    qCritical("Selected node not a valid module node");
    this->enableAllWidgets(false);
    return;
  }
  this->enableAllWidgets(true); // unless otherwise specified, everything is enabled

  // Node selectors
  vtkMRMLNode* inputNode = markupsToModelModuleNode->GetInputNode();
  d->InputNodeSelector->setCurrentNode( inputNode );

  vtkMRMLMarkupsNode* inputMarkupsFiducialNode = vtkMRMLMarkupsNode::SafeDownCast( inputNode );
  if ( inputMarkupsFiducialNode != NULL )
  {
    d->InputMarkupsPlaceWidget->setCurrentNode( inputMarkupsFiducialNode );
  }
  else
  {
    d->InputMarkupsPlaceWidget->setCurrentNode( NULL ); // not a markups node
  }
  d->ModelNodeSelector->setCurrentNode( markupsToModelModuleNode->GetOutputModelNode() );

  // block ALL signals until the function returns
  // if a return is called after this line, then unblockAllSignals should also be called.
  this->blockAllSignals( true );

  // Model type
  switch (markupsToModelModuleNode->GetModelType())
  {
  case vtkMRMLMarkupsToModelNode::ClosedSurface: d->ModeClosedSurfaceRadioButton->setChecked(1); break;
  case vtkMRMLMarkupsToModelNode::Curve: d->ModeCurveRadioButton->setChecked(1); break;
  }

  // Update button
  d->UpdateButton->setEnabled( markupsToModelModuleNode->GetInputNode() != NULL
                            && markupsToModelModuleNode->GetOutputModelNode() != NULL);
  if (markupsToModelModuleNode->GetAutoUpdateOutput())
  {
    bool wasBlocked = d->UpdateButton->blockSignals(true);
    d->UpdateButton->setText(tr("Auto-update"));
    d->UpdateButton->setCheckable(true);
    d->UpdateButton->setChecked(true);
    d->UpdateButton->blockSignals(wasBlocked);
  }
  else
  {
    bool wasBlocked = d->UpdateButton->blockSignals(true);
    d->UpdateButton->setText(tr("Update"));
    d->UpdateButton->setCheckable(false);
    d->UpdateButton->blockSignals(wasBlocked);
  }

  // Advanced options
  d->CleanDuplicateInputPointsCheckbox->setChecked(markupsToModelModuleNode->GetCleanMarkups());
  // closed surface
  d->ButterflySubdivisionCheckBox->setChecked(markupsToModelModuleNode->GetButterflySubdivision());
  d->DelaunayAlphaDoubleSpinBox->setValue(markupsToModelModuleNode->GetDelaunayAlpha());
  d->ConvexHullCheckBox->setChecked(markupsToModelModuleNode->GetConvexHull());
  // curve
  d->TubeRadiusDoubleSpinBox->setValue(markupsToModelModuleNode->GetTubeRadius());
  d->TubeSidesSpinBox->setValue(markupsToModelModuleNode->GetTubeNumberOfSides());
  d->TubeSegmentsSpinBox->setValue(markupsToModelModuleNode->GetTubeSegmentsBetweenControlPoints());
  d->TubeLoopCheckBox->setChecked(markupsToModelModuleNode->GetTubeLoop());
  d->TubeCappingCheckBox->setChecked(markupsToModelModuleNode->GetTubeCapping());
  int curveType = markupsToModelModuleNode->GetCurveType();
  if ( curveType == vtkMRMLMarkupsToModelNode::Linear )
  {
    d->LinearInterpolationRadioButton->setChecked(1);
  }
  else if ( curveType == vtkMRMLMarkupsToModelNode::CardinalSpline )
  {
    d->CardinalInterpolationRadioButton->setChecked(1);
  }
  else if ( curveType == vtkMRMLMarkupsToModelNode::KochanekSpline )
  {
    d->KochanekInterpolationRadioButton->setChecked(1);
  }
  else if ( curveType == vtkMRMLMarkupsToModelNode::Polynomial )
  {
    int polynomialFitType = markupsToModelModuleNode->GetPolynomialFitType();
    if ( polynomialFitType == vtkMRMLMarkupsToModelNode::GlobalLeastSquares )
    {
      d->GlobalLeastSquaresPolynomialApproximationRadioButton->setChecked(1);
    }
    else if ( polynomialFitType == vtkMRMLMarkupsToModelNode::MovingLeastSquares )
    {
      d->MovingLeastSquaresPolynomialApproximationRadioButton->setChecked(1);
    }
    else
    {
      qCritical() << "Unexpected polynomial fit type:" << polynomialFitType << ". Cannot update GUI.";
    }
  }
  else
  {
    qCritical() << "Unexpected curve type:" << curveType << ". Cannot update GUI.";
  }

  d->KochanekEndsCopyNearestDerivativesCheckBox->setChecked(markupsToModelModuleNode->GetKochanekEndsCopyNearestDerivatives());
  d->KochanekBiasDoubleSpinBox->setValue(markupsToModelModuleNode->GetKochanekBias());
  d->KochanekContinuityDoubleSpinBox->setValue(markupsToModelModuleNode->GetKochanekContinuity());
  d->KochanekTensionDoubleSpinBox->setValue(markupsToModelModuleNode->GetKochanekTension());

  int pointSortingMethod = markupsToModelModuleNode->GetPointParameterType();
  if ( pointSortingMethod == vtkMRMLMarkupsToModelNode::RawIndices )
  {
    d->PointSortingIndicesRadioButton->setChecked(1);
  }
  else if ( pointSortingMethod == vtkMRMLMarkupsToModelNode::MinimumSpanningTree )
  {
    d->PointSortingMinimumSpanningTreeRadioButton->setChecked(1);
  }
  else
  {
    qCritical() << "Unexpected point sorting method:" << pointSortingMethod << ". Cannot update GUI.";
  }

  d->PolynomialOrderSpinBox->setValue(markupsToModelModuleNode->GetPolynomialOrder());
  d->PolynomialSampleWidthDoubleSpinBox->setValue(markupsToModelModuleNode->GetPolynomialSampleWidth());
  int polynomialWeightType = markupsToModelModuleNode->GetPolynomialWeightType();
  if ( polynomialWeightType == vtkMRMLMarkupsToModelNode::Rectangular )
  {
    d->WeightFunctionRectangularRadioButton->setChecked(1);
  }
  else if ( polynomialWeightType == vtkMRMLMarkupsToModelNode::Triangular )
  {
    d->WeightFunctionTriangularRadioButton->setChecked(1);
  }
  else if ( polynomialWeightType == vtkMRMLMarkupsToModelNode::Cosine )
  {
    d->WeightFunctionCosineRadioButton->setChecked(1);
  }
  else if ( polynomialWeightType == vtkMRMLMarkupsToModelNode::Gaussian )
  {
    d->WeightFunctionGaussianRadioButton->setChecked(1);
  }
  else
  {
    qCritical() << "Unexpected polynomial weight type:" << polynomialWeightType << ". Cannot update GUI.";
  }

  // Model display options
  vtkMRMLModelDisplayNode* modelDisplayNode = vtkMRMLModelDisplayNode::SafeDownCast(
    this->GetOutputModelNode() ? this->GetOutputModelNode()->GetDisplayNode() : NULL );
  if ( modelDisplayNode != NULL )
  {
    d->ModelVisiblityButton->setChecked( modelDisplayNode->GetVisibility() );
    d->ModelOpacitySlider->setValue( modelDisplayNode->GetOpacity() );
    double* outputColor = modelDisplayNode->GetColor();
    QColor nodeOutputColor;
    nodeOutputColor.setRgbF( outputColor[ 0 ], outputColor[ 1 ], outputColor[ 2 ] );
    d->ModelColorSelector->setColor( nodeOutputColor );
    d->ModelSliceIntersectionCheckbox->setChecked( modelDisplayNode->GetVisibility2D() );
  }
  else
  {
    d->ModelVisiblityButton->setChecked( false );
    d->ModelOpacitySlider->setValue( 1.0 );
    QColor nodeOutputColor;
    nodeOutputColor.setRgbF( 0, 0, 0 );
    d->ModelColorSelector->setColor( nodeOutputColor );
    d->ModelSliceIntersectionCheckbox->setChecked( false );
  }
  d->ModelVisiblityButton->setEnabled( modelDisplayNode != NULL );
  d->ModelOpacitySlider->setEnabled( modelDisplayNode != NULL );
  d->ModelColorSelector->setEnabled( modelDisplayNode != NULL );
  d->ModelSliceIntersectionCheckbox->setEnabled( modelDisplayNode != NULL );

  // Markups display options
  vtkMRMLMarkupsNode* inputMarkupsNode = vtkMRMLMarkupsNode::SafeDownCast( this->GetInputNode() );
  if ( inputMarkupsNode != NULL )
  {
    vtkMRMLMarkupsDisplayNode* inputMarkupsDisplayNode = vtkMRMLMarkupsDisplayNode::SafeDownCast( inputMarkupsNode->GetDisplayNode() );
    if ( inputMarkupsDisplayNode != NULL )
    {
      d->MarkupsTextScaleSlider->setValue( inputMarkupsDisplayNode->GetTextScale() );
      d->MarkupsTextScaleSlider->setEnabled( true );
    }
    else
    {
      d->MarkupsTextScaleSlider->setValue(0);
      d->MarkupsTextScaleSlider->setEnabled( false );
    }
  }
  else
  {
    d->MarkupsTextScaleSlider->setEnabled( false );
  }

  // Determine visibility of widgets
  bool isInputMarkups = ( vtkMRMLMarkupsNode::SafeDownCast( inputNode ) != NULL );

  d->InputMarkupsPlaceWidget->setVisible( isInputMarkups );
  d->MarkupsTextScaleSlider->setVisible( isInputMarkups );

  bool isClosedSurface = d->ModeClosedSurfaceRadioButton->isChecked();

  d->ClosedSurfaceModelGroupBox->setVisible( isClosedSurface );

  bool isCurve = d->ModeCurveRadioButton->isChecked();

  d->CurveModelGroupBox->setVisible( isCurve );

  bool isLinearSpline = d->LinearInterpolationRadioButton->isChecked();
  bool isCardinalSpline = d->CardinalInterpolationRadioButton->isChecked();
  bool isKochanekSpline = d->KochanekInterpolationRadioButton->isChecked();
  bool isSpline = isLinearSpline || isCardinalSpline || isKochanekSpline;

  d->TubeLoopCheckBox->setEnabled( isSpline );

  bool isGlobalLeastSquaresPolynomial = d->GlobalLeastSquaresPolynomialApproximationRadioButton->isChecked();
  bool isMovingLeastSquaresPolynomial = d->MovingLeastSquaresPolynomialApproximationRadioButton->isChecked();
  bool isPolynomial = isGlobalLeastSquaresPolynomial || isMovingLeastSquaresPolynomial;

  d->FittingGroupBox->setVisible( isKochanekSpline || isPolynomial );

  d->KochanekEndsCopyNearestDerivativesLabel->setVisible( isKochanekSpline );
  d->KochanekEndsCopyNearestDerivativesCheckBox->setVisible( isKochanekSpline );
  d->KochanekBiasLabel->setVisible( isKochanekSpline );
  d->KochanekBiasDoubleSpinBox->setVisible( isKochanekSpline );
  d->KochanekTensionLabel->setVisible( isKochanekSpline );
  d->KochanekTensionDoubleSpinBox->setVisible( isKochanekSpline );
  d->KochanekContinuityLabel->setVisible( isKochanekSpline );
  d->KochanekContinuityDoubleSpinBox->setVisible( isKochanekSpline );

  d->PointSortingLabel->setVisible( isPolynomial );
  d->PointSortingFrame->setVisible( isPolynomial );
  d->PolynomialOrderLabel->setVisible( isPolynomial );
  d->PolynomialOrderSpinBox->setVisible( isPolynomial );
  d->PolynomialSampleWidthLabel->setVisible( isMovingLeastSquaresPolynomial );
  d->PolynomialSampleWidthDoubleSpinBox->setVisible( isMovingLeastSquaresPolynomial );
  d->WeightFunctionLabel->setVisible( isMovingLeastSquaresPolynomial );
  d->WeightFunctionFrame->setVisible( isMovingLeastSquaresPolynomial );

  this->blockAllSignals( false );
}

//-----------------------------------------------------------------------------
void qSlicerMarkupsToModelModuleWidget::blockAllSignals(bool block)
{
  Q_D(qSlicerMarkupsToModelModuleWidget);

  d->ParameterNodeSelector->blockSignals(block);
  d->ModeClosedSurfaceRadioButton->blockSignals(block);
  d->ModeCurveRadioButton->blockSignals(block);
  d->InputMarkupsPlaceWidget->blockSignals(block);
  d->InputNodeSelector->blockSignals(block);
  d->ModelNodeSelector->blockSignals(block);
  d->UpdateButton->blockSignals(block);

  // advanced options
  d->CleanDuplicateInputPointsCheckbox->blockSignals(block);
  // closed surface options
  d->ButterflySubdivisionCheckBox->blockSignals(block);
  d->DelaunayAlphaDoubleSpinBox->blockSignals(block);
  d->ConvexHullCheckBox->blockSignals(block);
  // curve options
  d->TubeSidesSpinBox->blockSignals(block);
  d->TubeRadiusDoubleSpinBox->blockSignals(block);
  d->TubeSegmentsSpinBox->blockSignals(block);
  d->LinearInterpolationRadioButton->blockSignals(block);
  d->CardinalInterpolationRadioButton->blockSignals(block);
  d->KochanekInterpolationRadioButton->blockSignals(block);
  d->GlobalLeastSquaresPolynomialApproximationRadioButton->blockSignals(block);
  d->MovingLeastSquaresPolynomialApproximationRadioButton->blockSignals(block);
  d->KochanekBiasDoubleSpinBox->blockSignals(block);
  d->KochanekContinuityDoubleSpinBox->blockSignals(block);
  d->KochanekTensionDoubleSpinBox->blockSignals(block);
  d->PointSortingIndicesRadioButton->blockSignals(block);
  d->PointSortingMinimumSpanningTreeRadioButton->blockSignals(block);
  d->PolynomialOrderSpinBox->blockSignals(block);
  d->WeightFunctionRectangularRadioButton->blockSignals(block);
  d->WeightFunctionTriangularRadioButton->blockSignals(block);
  d->WeightFunctionCosineRadioButton->blockSignals(block);
  d->WeightFunctionGaussianRadioButton->blockSignals(block);

  // display options
  d->ModelVisiblityButton->blockSignals(block);
  d->ModelOpacitySlider->blockSignals(block);
  d->ModelColorSelector->blockSignals(block);
  d->ModelSliceIntersectionCheckbox->blockSignals(block);
  d->MarkupsTextScaleSlider->blockSignals(block);
}

//-----------------------------------------------------------------------------
void qSlicerMarkupsToModelModuleWidget::enableAllWidgets(bool enable)
{
  Q_D(qSlicerMarkupsToModelModuleWidget);
  d->ModeClosedSurfaceRadioButton->setEnabled(enable);
  d->ModeCurveRadioButton->setEnabled(enable);
  d->InputNodeSelector->setEnabled(enable);
  d->InputMarkupsPlaceWidget->setEnabled(enable);
  d->ModelNodeSelector->setEnabled(enable);
  d->UpdateButton->setEnabled(enable);
  d->ClosedSurfaceModelGroupBox->setEnabled(enable);
  d->CurveModelGroupBox->setEnabled(enable);
  d->DisplayGroupBox->setEnabled(enable);
}

//-----------------------------------------------------------------------------
void qSlicerMarkupsToModelModuleWidget::UpdateOutputModel()
{
  Q_D(qSlicerMarkupsToModelModuleWidget);
  vtkMRMLMarkupsToModelNode* markupsToModelModuleNode = vtkMRMLMarkupsToModelNode::SafeDownCast(d->ParameterNodeSelector->currentNode());
  if (markupsToModelModuleNode == NULL)
  {
    qCritical("Model node changed with no module node selection");
    return;
  }

  // set up the output model node if needed
  vtkMRMLModelNode* outputModelNode = markupsToModelModuleNode->GetOutputModelNode();
  if (outputModelNode == NULL)
  {
    if (markupsToModelModuleNode->GetScene() == NULL)
    {
      vtkGenericWarningMacro("Output model node is not specified and markupsToModelModuleNode is not associated with any scene. No operation performed.");
      return;
    }
    outputModelNode = vtkMRMLModelNode::SafeDownCast(markupsToModelModuleNode->GetScene()->AddNewNodeByClass("vtkMRMLModelNode"));
    if (markupsToModelModuleNode->GetName())
    {
      std::string outputModelNodeName = std::string(markupsToModelModuleNode->GetName()).append("Model");
      outputModelNode->SetName(outputModelNodeName.c_str());
    }
    markupsToModelModuleNode->SetAndObserveOutputModelNodeID(outputModelNode->GetID());
  }

  d->logic()->UpdateOutputModel(markupsToModelModuleNode);
}

//-----------------------------------------------------------------------------
vtkMRMLModelNode* qSlicerMarkupsToModelModuleWidget::GetOutputModelNode()
{
  Q_D(qSlicerMarkupsToModelModuleWidget);
  vtkMRMLMarkupsToModelNode* markupsToModelModuleNode = vtkMRMLMarkupsToModelNode::SafeDownCast(d->ParameterNodeSelector->currentNode());
  if (markupsToModelModuleNode == NULL)
  {
    qCritical("Selected node not a valid module node");
    return NULL;
  }
  return markupsToModelModuleNode->GetOutputModelNode();
}

//-----------------------------------------------------------------------------
vtkMRMLNode* qSlicerMarkupsToModelModuleWidget::GetInputNode()
{
  Q_D(qSlicerMarkupsToModelModuleWidget);
  vtkMRMLMarkupsToModelNode* markupsToModelModuleNode = vtkMRMLMarkupsToModelNode::SafeDownCast(d->ParameterNodeSelector->currentNode());
  if (markupsToModelModuleNode == NULL)
  {
    qCritical("Selected node not a valid module node");
    return NULL;
  }
  return markupsToModelModuleNode->GetInputNode();
}

//------------------------------------------------------------------------------
void qSlicerMarkupsToModelModuleWidget::onUpdateButtonClicked()
{
  Q_D(qSlicerMarkupsToModelModuleWidget);
  if (d->UpdateButton->checkState() == Qt::Checked)
  {
    // If update button is untoggled then make it unchecked, too
    d->UpdateButton->setCheckState(Qt::Unchecked);
  }
  this->UpdateOutputModel();
}

//------------------------------------------------------------------------------
void qSlicerMarkupsToModelModuleWidget::onUpdateButtonCheckboxToggled(bool checked)
{
  Q_D(qSlicerMarkupsToModelModuleWidget);
  vtkMRMLMarkupsToModelNode* markupsToModelModuleNode = vtkMRMLMarkupsToModelNode::SafeDownCast(d->ParameterNodeSelector->currentNode());
  if (markupsToModelModuleNode == NULL)
  {
    qCritical("Model node changed with no module node selection");
    return;
  }
  if (markupsToModelModuleNode == NULL)
  {
    return;
  }
  markupsToModelModuleNode->SetAutoUpdateOutput(checked);
}
