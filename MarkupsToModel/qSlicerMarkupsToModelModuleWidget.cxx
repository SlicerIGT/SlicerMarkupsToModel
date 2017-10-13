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
#include "vtkMRMLMarkupsFiducialNode.h"
#include "vtkMRMLInteractionNode.h"

// module includes
#include "vtkMRMLMarkupsToModelNode.h"
#include "vtkSlicerMarkupsToModelLogic.h"
#include "vtkCreateCurveUtil.h"

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

  d->InputMarkupsNodeWidget->tableWidget()->setHidden( true ); // we don't need to see the table of fiducials
  d->InputMarkupsNodeWidget->markupsPlaceWidget()->setPlaceMultipleMarkups( qSlicerMarkupsPlaceWidget::ForcePlaceMultipleMarkups );

  d->InputMarkupsNodeWidget->setNodeBaseName( "ModelMarkup" );
  QColor markupDefaultColor;
  markupDefaultColor.setRgbF( 1, 0.5, 0.5 );
  d->InputMarkupsNodeWidget->setDefaultNodeColor( markupDefaultColor );

  this->setMRMLScene( d->logic()->GetMRMLScene() );

  connect(d->ParameterNodeSelector, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(onMarkupsToModelNodeSelectionChanged()));
  connect(d->ModelNodeSelector, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(onOutputModelComboBoxSelectionChanged(vtkMRMLNode*)));
  connect(d->ModelNodeSelector, SIGNAL(nodeAddedByUser(vtkMRMLNode*)), this, SLOT(onOutputModelComboBoxNodeAdded(vtkMRMLNode*)));
  connect(d->InputMarkupsNodeWidget->markupsSelectorComboBox(), SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(onInputNodeComboBoxSelectionChanged(vtkMRMLNode*)));
  connect(d->InputMarkupsNodeWidget->markupsSelectorComboBox(), SIGNAL(nodeAddedByUser(vtkMRMLNode*)), this, SLOT(onInputNodeComboBoxNodeAdded(vtkMRMLNode*)));
  connect(d->InputNodeSelector, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(onInputNodeComboBoxSelectionChanged(vtkMRMLNode*)));
  connect(d->InputNodeSelector, SIGNAL(nodeAddedByUser(vtkMRMLNode*)), this, SLOT(onInputNodeComboBoxNodeAdded(vtkMRMLNode*)));
  connect(d->InputChangeNodeTypeButton, SIGNAL( clicked() ), this, SLOT(onInputChangeNodeTypeButtonClicked()) );
  connect(d->UpdateButton, SIGNAL(clicked()), this, SLOT(onUpdateButtonClicked()));
  connect(d->UpdateButton, SIGNAL(checkBoxToggled(bool)), this, SLOT(onUpdateButtonCheckboxToggled(bool)));

  connect(d->ButterflySubdivisionCheckBox, SIGNAL(toggled(bool)), this, SLOT(updateMRMLFromGUI()));
  connect(d->ConvexHullCheckBox, SIGNAL(toggled(bool)), this, SLOT(updateMRMLFromGUI()));
  connect(d->CleanMarkupsCheckBox, SIGNAL(toggled(bool)), this, SLOT(updateMRMLFromGUI()));

  connect(d->ModeClosedSurfaceRadioButton, SIGNAL(clicked()), this, SLOT(updateMRMLFromGUI()));
  connect(d->ModeCurveRadioButton, SIGNAL(clicked()), this, SLOT(updateMRMLFromGUI()));
  connect(d->DelaunayAlphaDoubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(updateMRMLFromGUI()));
  connect(d->TubeRadiusDoubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(updateMRMLFromGUI()));
  connect(d->TubeSegmentsSpinBox, SIGNAL(valueChanged(double)), this, SLOT(updateMRMLFromGUI()));
  connect(d->TubeSidesSpinBox, SIGNAL(valueChanged(double)), this, SLOT(updateMRMLFromGUI()));
  connect(d->TubeLoopCheckBox, SIGNAL(clicked()), this, SLOT(updateMRMLFromGUI()));

  connect(d->KochanekEndsCopyNearestDerivativesCheckBox, SIGNAL(clicked()), this, SLOT(updateMRMLFromGUI()));
  connect(d->KochanekBiasDoubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(updateMRMLFromGUI()));
  connect(d->KochanekContinuityDoubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(updateMRMLFromGUI()));
  connect(d->KochanekTensionDoubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(updateMRMLFromGUI()));
  connect(d->PointParameterRawIndicesRadioButton, SIGNAL(clicked()), this, SLOT(updateMRMLFromGUI()));
  connect(d->PointParameterMinimumSpanningTreeRadioButton, SIGNAL(clicked()), this, SLOT(updateMRMLFromGUI()));
  connect(d->PolynomialOrderSpinBox, SIGNAL(valueChanged(double)), this, SLOT(updateMRMLFromGUI()));

  connect(d->ModelOpacitySlider, SIGNAL(valueChanged(double)), this, SLOT(updateMRMLFromGUI()));
  connect(d->ModelColorSelector, SIGNAL(clicked()), this, SLOT(updateMRMLFromGUI()));
  connect(d->ModelVisiblityButton, SIGNAL(toggled(bool)), this, SLOT(updateMRMLFromGUI()));
  connect(d->ModelSliceIntersectionCheckbox, SIGNAL(toggled(bool)), this, SLOT(updateMRMLFromGUI()));
  connect(d->MarkupsTextScaleSlider, SIGNAL(valueChanged(double)), this, SLOT(updateMRMLFromGUI()));

  connect(d->LinearInterpolationRadioButton, SIGNAL(clicked()), this, SLOT(updateMRMLFromGUI()));
  connect(d->CardinalInterpolationRadioButton, SIGNAL(clicked()), this, SLOT(updateMRMLFromGUI()));
  connect(d->KochanekInterpolationRadioButton, SIGNAL(clicked()), this, SLOT(updateMRMLFromGUI()));
  connect(d->PolynomialInterpolationRadioButton, SIGNAL(clicked()), this, SLOT(updateMRMLFromGUI()));
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

  this->updateGUIFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerMarkupsToModelModuleWidget::onMarkupsToModelNodeSelectionChanged()
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
  Q_D(qSlicerMarkupsToModelModuleWidget);
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

  vtkMRMLModelNode* modelNode = vtkMRMLModelNode::SafeDownCast( newNode );
  markupsToModelModuleNode->SetAndObserveOutputModelNodeID( modelNode ? modelNode->GetID() : NULL );

  // Observe display node so that we can make sure the module GUI always shows up-to-date information
  vtkMRMLModelDisplayNode* displayNode = NULL; // temporary value
  if (modelNode)
  {
    modelNode->CreateDefaultDisplayNodes();
    displayNode = vtkMRMLModelDisplayNode::SafeDownCast(modelNode->GetDisplayNode());
  }
  qvtkReconnect(d->ModelDisplayNode, displayNode, vtkCommand::ModifiedEvent, this, SLOT(updateGUIFromMRML()));
  d->ModelDisplayNode = displayNode;

  this->updateGUIFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerMarkupsToModelModuleWidget::onOutputModelComboBoxNodeAdded( vtkMRMLNode* addedNode )
{
  Q_D(qSlicerMarkupsToModelModuleWidget);

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
    displayNode->SliceIntersectionVisibilityOn();
    displayNode->SetSliceIntersectionThickness(2);
  }
}

//-----------------------------------------------------------------------------
void qSlicerMarkupsToModelModuleWidget::onInputNodeComboBoxSelectionChanged( vtkMRMLNode* newNode )
{
  Q_D(qSlicerMarkupsToModelModuleWidget);

  vtkMRMLMarkupsToModelNode* markupsToModelModuleNode = vtkMRMLMarkupsToModelNode::SafeDownCast(d->ParameterNodeSelector->currentNode());
  if (markupsToModelModuleNode == NULL)
  {
    qCritical() << Q_FUNC_INFO << ": invalid markupsToModelModuleNode";
    return;
  }

  if ( newNode == NULL )
  {
    markupsToModelModuleNode->SetAndObserveInputNodeID( NULL );
  }
  else if ( vtkMRMLMarkupsFiducialNode::SafeDownCast( newNode ) )
  {
    vtkMRMLMarkupsFiducialNode* markupsNode = vtkMRMLMarkupsFiducialNode::SafeDownCast( newNode );
    markupsToModelModuleNode->SetAndObserveInputNodeID( markupsNode->GetID() );
  
    // Observe display node so that we can make sure the module GUI always shows up-to-date information
    // (applies specifically to markups)
    vtkMRMLMarkupsDisplayNode* displayNode = NULL;
    if ( markupsNode )
    {
      markupsNode->CreateDefaultDisplayNodes();
      displayNode = vtkMRMLMarkupsDisplayNode::SafeDownCast( markupsNode->GetDisplayNode() );
    }
    qvtkReconnect( d->MarkupsDisplayNode, displayNode, vtkCommand::ModifiedEvent, this, SLOT( updateGUIFromMRML() ) );
    d->MarkupsDisplayNode = displayNode;
  }
  else if ( vtkMRMLModelNode::SafeDownCast( newNode ) )
  {
    vtkMRMLModelNode* inputModelNode = vtkMRMLModelNode::SafeDownCast( newNode );
    markupsToModelModuleNode->SetAndObserveInputNodeID( inputModelNode->GetID() );
  }
  else
  {
    qCritical() << Q_FUNC_INFO << ": unexpected input node type";
  }

  this->updateGUIFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerMarkupsToModelModuleWidget::onInputNodeComboBoxNodeAdded(vtkMRMLNode* addedNode)
{
  Q_D(qSlicerMarkupsToModelModuleWidget);

  vtkMRMLMarkupsNode* markupsNode = vtkMRMLMarkupsNode::SafeDownCast(addedNode);
  if (markupsNode == NULL)
  {
    // if not a markups node, then no action is necessary
    return;
  }

  markupsNode->CreateDefaultDisplayNodes();
  vtkMRMLMarkupsDisplayNode* displayNode = vtkMRMLMarkupsDisplayNode::SafeDownCast(markupsNode->GetDisplayNode());
  if (displayNode)
  {
    displayNode->SetTextScale(0.0);
  }
}

//-----------------------------------------------------------------------------
void qSlicerMarkupsToModelModuleWidget::onInputChangeNodeTypeButtonClicked()
{
  Q_D(qSlicerMarkupsToModelModuleWidget);

  vtkMRMLMarkupsToModelNode* markupsToModelModuleNode = vtkMRMLMarkupsToModelNode::SafeDownCast( d->ParameterNodeSelector->currentNode() );
  if ( markupsToModelModuleNode == NULL )
  {
    qCritical() << Q_FUNC_INFO << ": invalid markupsToModelModuleNode";
    return;
  }

  // If the user sees the change node type button, then
  // the input markups node widget is visible, but the
  // input node selector is not visible.
  // This is because the current input node is a markups node.
  // Set the input node to null, then the widget will once again
  // show all the normal input node selector options
  markupsToModelModuleNode->SetAndObserveInputNodeID( NULL );
  this->updateGUIFromMRML();
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

  markupsToModelModuleNode->SetCleanMarkups(d->CleanMarkupsCheckBox->isChecked());
  markupsToModelModuleNode->SetDelaunayAlpha(d->DelaunayAlphaDoubleSpinBox->value());
  markupsToModelModuleNode->SetConvexHull(d->ConvexHullCheckBox->isChecked());
  markupsToModelModuleNode->SetButterflySubdivision(d->ButterflySubdivisionCheckBox->isChecked());

  markupsToModelModuleNode->SetTubeRadius(d->TubeRadiusDoubleSpinBox->value());
  markupsToModelModuleNode->SetTubeSegmentsBetweenControlPoints(d->TubeSegmentsSpinBox->value());
  markupsToModelModuleNode->SetTubeNumberOfSides(d->TubeSidesSpinBox->value());
  markupsToModelModuleNode->SetTubeLoop(d->TubeLoopCheckBox->isChecked());
  if (d->LinearInterpolationRadioButton->isChecked())
  {
    markupsToModelModuleNode->SetInterpolationType(vtkMRMLMarkupsToModelNode::Linear);
  }
  else if (d->CardinalInterpolationRadioButton->isChecked())
  {
    markupsToModelModuleNode->SetInterpolationType(vtkMRMLMarkupsToModelNode::CardinalSpline);
  }
  else if (d->KochanekInterpolationRadioButton->isChecked())
  {
    markupsToModelModuleNode->SetInterpolationType(vtkMRMLMarkupsToModelNode::KochanekSpline);
  }
  else if (d->PolynomialInterpolationRadioButton->isChecked())
  {
    markupsToModelModuleNode->SetInterpolationType(vtkMRMLMarkupsToModelNode::Polynomial);
  }
  markupsToModelModuleNode->SetKochanekEndsCopyNearestDerivatives(d->KochanekEndsCopyNearestDerivativesCheckBox->isChecked());
  markupsToModelModuleNode->SetKochanekBias(d->KochanekBiasDoubleSpinBox->value());
  markupsToModelModuleNode->SetKochanekContinuity(d->KochanekContinuityDoubleSpinBox->value());
  markupsToModelModuleNode->SetKochanekTension(d->KochanekTensionDoubleSpinBox->value());
  if (d->PointParameterRawIndicesRadioButton->isChecked())
  {
    markupsToModelModuleNode->SetPointParameterType(vtkMRMLMarkupsToModelNode::RawIndices);
  }
  else if (d->PointParameterMinimumSpanningTreeRadioButton->isChecked())
  {
    markupsToModelModuleNode->SetPointParameterType(vtkMRMLMarkupsToModelNode::MinimumSpanningTree);
  }
  markupsToModelModuleNode->SetPolynomialOrder(d->PolynomialOrderSpinBox->value());

  markupsToModelModuleNode->EndModify(markupsToModelModuleNodeWasModified);

  vtkMRMLModelDisplayNode* modelDisplayNode = vtkMRMLModelDisplayNode::SafeDownCast(
    this->GetOutputModelNode() ? this->GetOutputModelNode()->GetDisplayNode() : NULL);
  if (modelDisplayNode != NULL)
  {
    int modelDisplayNodeWasModified = modelDisplayNode->StartModify();
    modelDisplayNode->SetVisibility(d->ModelVisiblityButton->isChecked());
    modelDisplayNode->SetOpacity(d->ModelOpacitySlider->value());
    modelDisplayNode->SetSliceIntersectionVisibility(d->ModelSliceIntersectionCheckbox->isChecked());
    modelDisplayNode->SetColor(d->ModelColorSelector->color().redF(), d->ModelColorSelector->color().greenF(), d->ModelColorSelector->color().blueF());
    modelDisplayNode->EndModify(modelDisplayNodeWasModified);
  }

  vtkMRMLMarkupsFiducialNode* inputMarkupsNode = vtkMRMLMarkupsFiducialNode::SafeDownCast( this->GetInputNode() );
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
  vtkMRMLNode* currentNode = d->ParameterNodeSelector->currentNode();
  if (currentNode == NULL)
  {
    this->enableAllWidgets(false);
    return;
  }
  vtkMRMLMarkupsToModelNode* markupsToModelNode = vtkMRMLMarkupsToModelNode::SafeDownCast(currentNode);
  if (markupsToModelNode == NULL)
  {
    qCritical("Selected node not a valid module node");
    this->enableAllWidgets(false);
    return;
  }
  this->enableAllWidgets(true);

  // Node selectors
  vtkMRMLNode* inputNode = markupsToModelNode->GetInputNode();
  if ( vtkMRMLMarkupsFiducialNode::SafeDownCast( inputNode ) )
  {
    d->InputMarkupsNodeWidget->setCurrentNode( inputNode );
    d->InputNodeSelector->setCurrentNode( inputNode );
  }
  else
  {
    d->InputMarkupsNodeWidget->setCurrentNode( NULL ); // not a markups node
    d->InputNodeSelector->setCurrentNode( inputNode );
  }
  d->ModelNodeSelector->setCurrentNode(markupsToModelNode->GetOutputModelNode());

  // block ALL signals until the function returns
  // if a return is called after this line, then unblockAllSignals should also be called.
  this->blockAllSignals(true);

  // Model type
  switch (markupsToModelNode->GetModelType())
  {
  case vtkMRMLMarkupsToModelNode::ClosedSurface: d->ModeClosedSurfaceRadioButton->setChecked(1); break;
  case vtkMRMLMarkupsToModelNode::Curve: d->ModeCurveRadioButton->setChecked(1); break;
  }

  // Update button
  d->UpdateButton->setEnabled( markupsToModelNode->GetInputNode() != NULL
                            && markupsToModelNode->GetOutputModelNode() != NULL);
  if (markupsToModelNode->GetAutoUpdateOutput())
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
  d->CleanMarkupsCheckBox->setChecked(markupsToModelNode->GetCleanMarkups());
  // closed surface
  d->ButterflySubdivisionCheckBox->setChecked(markupsToModelNode->GetButterflySubdivision());
  d->DelaunayAlphaDoubleSpinBox->setValue(markupsToModelNode->GetDelaunayAlpha());
  d->ConvexHullCheckBox->setChecked(markupsToModelNode->GetConvexHull());
  // curve
  d->TubeRadiusDoubleSpinBox->setValue(markupsToModelNode->GetTubeRadius());
  d->TubeSidesSpinBox->setValue(markupsToModelNode->GetTubeNumberOfSides());
  d->TubeSegmentsSpinBox->setValue(markupsToModelNode->GetTubeSegmentsBetweenControlPoints());
  d->TubeLoopCheckBox->setChecked(markupsToModelNode->GetTubeLoop());
  switch (markupsToModelNode->GetInterpolationType())
  {
  case vtkMRMLMarkupsToModelNode::Linear: d->LinearInterpolationRadioButton->setChecked(1); break;
  case vtkMRMLMarkupsToModelNode::CardinalSpline: d->CardinalInterpolationRadioButton->setChecked(1); break;
  case vtkMRMLMarkupsToModelNode::KochanekSpline: d->KochanekInterpolationRadioButton->setChecked(1); break;
  case vtkMRMLMarkupsToModelNode::Polynomial: d->PolynomialInterpolationRadioButton->setChecked(1); break;
  }
  d->KochanekEndsCopyNearestDerivativesCheckBox->setChecked(markupsToModelNode->GetKochanekEndsCopyNearestDerivatives());
  d->KochanekBiasDoubleSpinBox->setValue(markupsToModelNode->GetKochanekBias());
  d->KochanekContinuityDoubleSpinBox->setValue(markupsToModelNode->GetKochanekContinuity());
  d->KochanekTensionDoubleSpinBox->setValue(markupsToModelNode->GetKochanekTension());
  switch (markupsToModelNode->GetPointParameterType())
  {
  case vtkMRMLMarkupsToModelNode::RawIndices: d->PointParameterRawIndicesRadioButton->setChecked(1); break;
  case vtkMRMLMarkupsToModelNode::MinimumSpanningTree: d->PointParameterMinimumSpanningTreeRadioButton->setChecked(1); break;
  }
  d->PolynomialOrderSpinBox->setValue(markupsToModelNode->GetPolynomialOrder());

  // Model display options
  vtkMRMLModelDisplayNode* modelDisplayNode = vtkMRMLModelDisplayNode::SafeDownCast( 
    this->GetOutputModelNode() ? this->GetOutputModelNode()->GetDisplayNode() : NULL );
  if (modelDisplayNode != NULL)
  {
    d->ModelVisiblityButton->setChecked(modelDisplayNode->GetVisibility());
    d->ModelOpacitySlider->setValue(modelDisplayNode->GetOpacity());
    double* outputColor = modelDisplayNode->GetColor();
    QColor nodeOutputColor;
    nodeOutputColor.setRgbF(outputColor[0], outputColor[1], outputColor[2]);
    d->ModelColorSelector->setColor(nodeOutputColor);
    d->ModelSliceIntersectionCheckbox->setChecked(modelDisplayNode->GetSliceIntersectionVisibility());
  }
  else
  {
    d->ModelVisiblityButton->setChecked(false);
    d->ModelOpacitySlider->setValue(1.0);
    QColor nodeOutputColor;
    nodeOutputColor.setRgbF(0, 0, 0);
    d->ModelColorSelector->setColor(nodeOutputColor);
    d->ModelSliceIntersectionCheckbox->setChecked(false);
  }
  d->ModelVisiblityButton->setEnabled(modelDisplayNode != NULL);
  d->ModelOpacitySlider->setEnabled(modelDisplayNode != NULL);
  d->ModelColorSelector->setEnabled(modelDisplayNode != NULL);
  d->ModelSliceIntersectionCheckbox->setEnabled(modelDisplayNode != NULL);

  // Markups display options
  vtkMRMLMarkupsFiducialNode* inputMarkupsNode = vtkMRMLMarkupsFiducialNode::SafeDownCast( this->GetInputNode() );
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
  bool isSurface = d->ModeClosedSurfaceRadioButton->isChecked();
  bool isCurve = d->ModeCurveRadioButton->isChecked();
  bool isPolynomial = d->PolynomialInterpolationRadioButton->isChecked();
  bool isKochanek = d->KochanekInterpolationRadioButton->isChecked();
  bool isInputMarkups = ( vtkMRMLMarkupsFiducialNode::SafeDownCast( inputNode ) != NULL );
  
  d->InputNodeSelector->setVisible(!isInputMarkups);
  d->InputMarkupsNodeWidget->setVisible(isInputMarkups);
  d->InputChangeNodeTypeButton->setVisible(isInputMarkups);
  d->MarkupsTextScaleSlider->setVisible(isInputMarkups);

  d->ButterflySubdivisionLabel->setVisible(isSurface);
  d->ButterflySubdivisionCheckBox->setVisible(isSurface);
  d->DelaunayAlphaLabel->setVisible(isSurface);
  d->DelaunayAlphaDoubleSpinBox->setVisible(isSurface);
  d->ConvexHullLabel->setVisible(isSurface);
  d->ConvexHullCheckBox->setVisible(isSurface);

  d->InterpolationGroupBox->setVisible(isCurve);
  d->InterpolationLabel->setVisible(isCurve);
  d->TubeRadiusLabel->setVisible(isCurve);
  d->TubeRadiusDoubleSpinBox->setVisible(isCurve);
  d->TubeSidesLabel->setVisible(isCurve);
  d->TubeSidesSpinBox->setVisible(isCurve);
  d->TubeSegmentsLabel->setVisible(isCurve);
  d->TubeSegmentsSpinBox->setVisible(isCurve);

  d->TubeLoopLabel->setVisible(isCurve && !isPolynomial);
  d->TubeLoopCheckBox->setVisible(isCurve && !isPolynomial);

  d->KochanekEndsCopyNearestDerivativesLabel->setVisible(isCurve && isKochanek);
  d->KochanekEndsCopyNearestDerivativesCheckBox->setVisible(isCurve && isKochanek);
  d->KochanekBiasLabel->setVisible(isCurve && isKochanek);
  d->KochanekBiasDoubleSpinBox->setVisible(isCurve && isKochanek);
  d->KochanekTensionLabel->setVisible(isCurve && isKochanek);
  d->KochanekTensionDoubleSpinBox->setVisible(isCurve && isKochanek);
  d->KochanekContinuityLabel->setVisible(isCurve && isKochanek);
  d->KochanekContinuityDoubleSpinBox->setVisible(isCurve && isKochanek);

  d->PointParameterLabel->setVisible(isCurve && isPolynomial);
  d->PointParameterGroupBox->setVisible(isCurve && isPolynomial);
  d->PointParameterRawIndicesRadioButton->setVisible(isCurve && isPolynomial);
  d->PointParameterMinimumSpanningTreeRadioButton->setVisible(isCurve && isPolynomial);
  d->PolynomialOrderLabel->setVisible(isCurve && isPolynomial);
  d->PolynomialOrderSpinBox->setVisible(isCurve && isPolynomial);

  this->blockAllSignals(false);
}

//-----------------------------------------------------------------------------
void qSlicerMarkupsToModelModuleWidget::updateFromRenderedNodes()
{
  Q_D(qSlicerMarkupsToModelModuleWidget);

}

//-----------------------------------------------------------------------------
void qSlicerMarkupsToModelModuleWidget::blockAllSignals(bool block)
{
  Q_D(qSlicerMarkupsToModelModuleWidget);

  d->ParameterNodeSelector->blockSignals(block);
  d->ModeClosedSurfaceRadioButton->blockSignals(block);
  d->ModeCurveRadioButton->blockSignals(block);
  d->InputMarkupsNodeWidget->blockSignals(block);
  d->InputNodeSelector->blockSignals(block);
  d->InputChangeNodeTypeButton->blockSignals(block);
  d->ModelNodeSelector->blockSignals(block);
  d->UpdateButton->blockSignals(block);

  // advanced options
  d->CleanMarkupsCheckBox->blockSignals(block);
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
  d->PolynomialInterpolationRadioButton->blockSignals(block);
  d->KochanekBiasDoubleSpinBox->blockSignals(block);
  d->KochanekContinuityDoubleSpinBox->blockSignals(block);
  d->KochanekTensionDoubleSpinBox->blockSignals(block);
  d->PointParameterRawIndicesRadioButton->blockSignals(block);
  d->PointParameterMinimumSpanningTreeRadioButton->blockSignals(block);
  d->PolynomialOrderSpinBox->blockSignals(block);

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
  d->InputMarkupsNodeWidget->setEnabled(enable);
  d->InputNodeSelector->setEnabled(enable);
  d->InputChangeNodeTypeButton->setEnabled(enable);
  d->ModelNodeSelector->setEnabled(enable);
  d->UpdateButton->setEnabled(enable);
  d->AdvancedGroupBox->setEnabled(enable);
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
  d->logic()->UpdateOutputModel(markupsToModelModuleNode);
}

//-----------------------------------------------------------------------------
vtkMRMLModelNode* qSlicerMarkupsToModelModuleWidget::GetOutputModelNode()
{
  Q_D(qSlicerMarkupsToModelModuleWidget);
  vtkMRMLMarkupsToModelNode* markupsToModelNode = vtkMRMLMarkupsToModelNode::SafeDownCast(d->ParameterNodeSelector->currentNode());
  if (markupsToModelNode == NULL)
  {
    qCritical("Selected node not a valid module node");
    return NULL;
  }
  return markupsToModelNode->GetOutputModelNode();
}

//-----------------------------------------------------------------------------
vtkMRMLNode* qSlicerMarkupsToModelModuleWidget::GetInputNode()
{
  Q_D(qSlicerMarkupsToModelModuleWidget);
  vtkMRMLMarkupsToModelNode* markupsToModelNode = vtkMRMLMarkupsToModelNode::SafeDownCast(d->ParameterNodeSelector->currentNode());
  if (markupsToModelNode == NULL)
  {
    qCritical("Selected node not a valid module node");
    return NULL;
  }
  return markupsToModelNode->GetInputNode();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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
