// MarkupsToModel includes
#include "vtkMRMLMarkupsToModelNode.h"

// slicer includes
#include "vtkMRMLModelDisplayNode.h"
#include "vtkMRMLMarkupsDisplayNode.h"
#include "vtkMRMLDisplayNode.h"
#include "vtkMRMLModelNode.h"

// Other MRML includes
#include "vtkMRMLNode.h"
#include "vtkMRMLMarkupsFiducialNode.h"

// VTK includes
#include <vtkNew.h>

// Other includes
#include <sstream>

static const char* INPUT_ROLE = "InputMarkups";
static const char* OUTPUT_MODEL_ROLE = "OutputModel";

static const char* OUTPUT_CURVE_LENGTH_ATTRIBUTE_NAME = "MarkupsToModel_OutputCurveLength";

vtkMRMLNodeNewMacro(vtkMRMLMarkupsToModelNode);

//-----------------------------------------------------------------
vtkMRMLMarkupsToModelNode::vtkMRMLMarkupsToModelNode()
{
  this->HideFromEditorsOff();
  this->SetSaveWithScene( true );

  vtkNew<vtkIntArray> events;
  events->InsertNextValue( vtkCommand::ModifiedEvent );
  events->InsertNextValue(vtkMRMLMarkupsNode::PointAddedEvent);
  events->InsertNextValue(vtkMRMLMarkupsNode::PointRemovedEvent);
  events->InsertNextValue( vtkMRMLMarkupsNode::PointModifiedEvent );
  events->InsertNextValue( vtkMRMLModelNode::MeshModifiedEvent );

  this->AddNodeReferenceRole( INPUT_ROLE, NULL, events.GetPointer() );
  this->AddNodeReferenceRole( OUTPUT_MODEL_ROLE );

  this->AutoUpdateOutput = true;
  this->CleanMarkups = true;
  this->ConvexHull = true;
  this->ButterflySubdivision = true;
  // DelaunayAlpha = 50 would work well most of the cases but in case if not then the user would not
  // know why no model is drawn around the points. It is better to use a safe and simple setting
  // by default (alpha = 0 => use convex hull).
  this->DelaunayAlpha = 0.0;
  this->TubeRadius = 1.0;
  this->TubeSegmentsBetweenControlPoints = 5;
  this->TubeNumberOfSides = 8;
  this->TubeLoop = false;
  this->TubeCapping = true;
  this->ModelType = vtkMRMLMarkupsToModelNode::ClosedSurface;
  this->CurveType = vtkMRMLMarkupsToModelNode::Linear;
  this->PointParameterType = vtkMRMLMarkupsToModelNode::RawIndices;

  this->KochanekTension = 0;
  this->KochanekBias = 0;
  this->KochanekContinuity = 0;

  this->KochanekEndsCopyNearestDerivatives = false;

  this->PolynomialOrder = 3;
  this->PolynomialFitType = vtkMRMLMarkupsToModelNode::GlobalLeastSquares;
  this->PolynomialSampleWidth = 0.5;
  this->PolynomialWeightType = vtkMRMLMarkupsToModelNode::Gaussian;
}

//-----------------------------------------------------------------
vtkMRMLMarkupsToModelNode::~vtkMRMLMarkupsToModelNode()
{
}

//-----------------------------------------------------------------
void vtkMRMLMarkupsToModelNode::WriteXML( ostream& of, int nIndent )
{
  Superclass::WriteXML(of, nIndent); // This will take care of referenced nodes
  vtkMRMLWriteXMLBeginMacro(of);
  vtkMRMLWriteXMLEnumMacro(ModelType, ModelType);
  vtkMRMLWriteXMLBooleanMacro(AutoUpdateOutput, AutoUpdateOutput);
  vtkMRMLWriteXMLBooleanMacro(CleanMarkups, CleanMarkups);
  vtkMRMLWriteXMLBooleanMacro(ConvexHull, ConvexHull);
  vtkMRMLWriteXMLBooleanMacro(ButterflySubdivision, ButterflySubdivision);
  vtkMRMLWriteXMLFloatMacro(DelaunayAlpha, DelaunayAlpha);
  vtkMRMLWriteXMLEnumMacro(CurveType, CurveType);
  vtkMRMLWriteXMLEnumMacro(PointParameterType, PointParameterType);
  vtkMRMLWriteXMLFloatMacro(TubeRadius, TubeRadius);
  vtkMRMLWriteXMLIntMacro(TubeNumberOfSides, TubeNumberOfSides);
  vtkMRMLWriteXMLIntMacro(TubeSegmentsBetweenControlPoints, TubeSegmentsBetweenControlPoints);
  vtkMRMLWriteXMLBooleanMacro(TubeLoop, TubeLoop);
  vtkMRMLWriteXMLBooleanMacro(TubeCapping, TubeCapping);
  vtkMRMLWriteXMLBooleanMacro(KochanekEndsCopyNearestDerivatives, KochanekEndsCopyNearestDerivatives);
  vtkMRMLWriteXMLFloatMacro(KochanekBias, KochanekBias);
  vtkMRMLWriteXMLFloatMacro(KochanekContinuity, KochanekContinuity);
  vtkMRMLWriteXMLFloatMacro(KochanekTension, KochanekTension);
  vtkMRMLWriteXMLIntMacro(PolynomialOrder, PolynomialOrder);
  vtkMRMLWriteXMLEnumMacro(PolynomialFitType, PolynomialFitType);
  vtkMRMLWriteXMLFloatMacro(PolynomialSampleWidth, PolynomialSampleWidth);
  vtkMRMLWriteXMLEnumMacro(PolynomialWeightType, PolynomialWeightType);
  vtkMRMLWriteXMLEndMacro();
}

//-----------------------------------------------------------------
void vtkMRMLMarkupsToModelNode::ReadXMLAttributes( const char** atts )
{
  int disabledModify = this->StartModify();
  Superclass::ReadXMLAttributes(atts); // This will take care of referenced nodes
  vtkMRMLReadXMLBeginMacro(atts);
  vtkMRMLReadXMLEnumMacro(ModelType, ModelType);
  vtkMRMLReadXMLBooleanMacro(AutoUpdateOutput, AutoUpdateOutput);
  vtkMRMLReadXMLBooleanMacro(CleanMarkups, CleanMarkups);
  vtkMRMLReadXMLBooleanMacro(ConvexHull, ConvexHull);
  vtkMRMLReadXMLBooleanMacro(ButterflySubdivision, ButterflySubdivision);
  vtkMRMLReadXMLFloatMacro(DelaunayAlpha, DelaunayAlpha);
  vtkMRMLReadXMLEnumMacro(InterpolationType, CurveType);
  vtkMRMLReadXMLEnumMacro(CurveType, CurveType);
  vtkMRMLReadXMLEnumMacro(PointParameterType, PointParameterType);
  vtkMRMLReadXMLFloatMacro(TubeRadius, TubeRadius);
  vtkMRMLReadXMLIntMacro(TubeNumberOfSides, TubeNumberOfSides);
  vtkMRMLReadXMLIntMacro(TubeSegmentsBetweenControlPoints, TubeSegmentsBetweenControlPoints);
  vtkMRMLReadXMLBooleanMacro(TubeLoop, TubeLoop);
  vtkMRMLReadXMLBooleanMacro(TubeCapping, TubeCapping);
  vtkMRMLReadXMLBooleanMacro(KochanekEndsCopyNearestDerivatives, KochanekEndsCopyNearestDerivatives);
  vtkMRMLReadXMLFloatMacro(KochanekBias, KochanekBias);
  vtkMRMLReadXMLFloatMacro(KochanekContinuity, KochanekContinuity);
  vtkMRMLReadXMLFloatMacro(KochanekTension, KochanekTension);
  vtkMRMLReadXMLIntMacro(PolynomialOrder, PolynomialOrder);
  vtkMRMLReadXMLEnumMacro(PolynomialFitType, PolynomialFitType);
  vtkMRMLReadXMLFloatMacro(PolynomialSampleWidth, PolynomialSampleWidth);
  vtkMRMLReadXMLEnumMacro(PolynomialWeightType, PolynomialWeightType);
  vtkMRMLReadXMLEndMacro();
  this->EndModify( disabledModify );
}

//-----------------------------------------------------------------
void vtkMRMLMarkupsToModelNode::Copy( vtkMRMLNode *anode )
{
  int disabledModify = this->StartModify();
  Superclass::Copy( anode ); // This will take care of referenced nodes
  vtkMRMLCopyBeginMacro(anode);
  vtkMRMLCopyEnumMacro(ModelType);
  vtkMRMLCopyBooleanMacro(AutoUpdateOutput);
  vtkMRMLCopyBooleanMacro(CleanMarkups);
  vtkMRMLCopyBooleanMacro(ConvexHull);
  vtkMRMLCopyBooleanMacro(ButterflySubdivision);
  vtkMRMLCopyFloatMacro(DelaunayAlpha);
  vtkMRMLCopyEnumMacro(CurveType);
  vtkMRMLCopyEnumMacro(PointParameterType);
  vtkMRMLCopyFloatMacro(TubeRadius);
  vtkMRMLCopyIntMacro(TubeNumberOfSides);
  vtkMRMLCopyIntMacro(TubeSegmentsBetweenControlPoints);
  vtkMRMLCopyBooleanMacro(TubeLoop);
  vtkMRMLCopyBooleanMacro(TubeCapping);
  vtkMRMLCopyBooleanMacro(KochanekEndsCopyNearestDerivatives);
  vtkMRMLCopyFloatMacro(KochanekBias);
  vtkMRMLCopyFloatMacro(KochanekContinuity);
  vtkMRMLCopyFloatMacro(KochanekTension);
  vtkMRMLCopyIntMacro(PolynomialOrder);
  vtkMRMLCopyEnumMacro(PolynomialFitType);
  vtkMRMLCopyFloatMacro(PolynomialSampleWidth);
  vtkMRMLCopyEnumMacro(PolynomialWeightType);
  vtkMRMLCopyEndMacro();
  this->EndModify(disabledModify);
}

//-----------------------------------------------------------------
void vtkMRMLMarkupsToModelNode::PrintSelf( ostream& os, vtkIndent indent )
{
  Superclass::PrintSelf(os,indent); // This will take care of referenced nodes
  vtkMRMLPrintBeginMacro(os, indent);
  vtkMRMLPrintEnumMacro(ModelType);
  vtkMRMLPrintBooleanMacro(AutoUpdateOutput);
  vtkMRMLPrintBooleanMacro(CleanMarkups);
  vtkMRMLPrintBooleanMacro(ConvexHull);
  vtkMRMLPrintBooleanMacro(ButterflySubdivision);
  vtkMRMLPrintFloatMacro(DelaunayAlpha);
  vtkMRMLPrintEnumMacro(CurveType);
  vtkMRMLPrintEnumMacro(PointParameterType);
  vtkMRMLPrintFloatMacro(TubeRadius);
  vtkMRMLPrintIntMacro(TubeNumberOfSides);
  vtkMRMLPrintIntMacro(TubeSegmentsBetweenControlPoints);
  vtkMRMLPrintBooleanMacro(TubeLoop);
  vtkMRMLPrintBooleanMacro(TubeCapping);
  vtkMRMLPrintBooleanMacro(KochanekEndsCopyNearestDerivatives);
  vtkMRMLPrintFloatMacro(KochanekBias);
  vtkMRMLPrintFloatMacro(KochanekContinuity);
  vtkMRMLPrintFloatMacro(KochanekTension);
  vtkMRMLPrintIntMacro(PolynomialOrder);
  vtkMRMLPrintEnumMacro(PolynomialFitType);
  vtkMRMLPrintFloatMacro(PolynomialSampleWidth);
  vtkMRMLPrintEnumMacro(PolynomialWeightType);
  vtkMRMLPrintEndMacro();
}

//-----------------------------------------------------------------
vtkMRMLNode* vtkMRMLMarkupsToModelNode::GetInputNode()
{
  vtkMRMLNode* inputNode = this->GetNodeReference( INPUT_ROLE );
  return inputNode;
}

//-----------------------------------------------------------------
vtkMRMLModelNode* vtkMRMLMarkupsToModelNode::GetOutputModelNode()
{
  vtkMRMLModelNode* modelNode = vtkMRMLModelNode::SafeDownCast( this->GetNodeReference( OUTPUT_MODEL_ROLE ) );
  return modelNode;
}

//-----------------------------------------------------------------
void vtkMRMLMarkupsToModelNode::SetAndObserveInputNodeID( const char* inputId )
{
  // error check
  const char* outputId = this->GetNodeReferenceID( OUTPUT_MODEL_ROLE );
  if ( inputId != NULL && outputId != NULL && strcmp( inputId, outputId ) == 0 )
  {
    vtkErrorMacro( "Input node and output node cannot be the same." );
    return;
  }

  this->SetAndObserveNodeReferenceID( INPUT_ROLE, inputId );
}

//-----------------------------------------------------------------
void vtkMRMLMarkupsToModelNode::SetAndObserveOutputModelNodeID( const char* outputId )
{
  // error check
  const char* inputId = this->GetNodeReferenceID( INPUT_ROLE );
  if ( inputId != NULL && outputId != NULL && strcmp( inputId, outputId ) == 0 )
  {
    vtkErrorMacro( "Input node and output node cannot be the same." );
    return;
  }

  this->SetAndObserveNodeReferenceID( OUTPUT_MODEL_ROLE, outputId );
}

//-----------------------------------------------------------------
double vtkMRMLMarkupsToModelNode::GetOutputCurveLength()
{
  vtkMRMLModelNode* outputModelNode = vtkMRMLModelNode::SafeDownCast( this->GetOutputModelNode() );
  if ( outputModelNode == NULL )
  {
    vtkWarningMacro( "No output model node. Returning 0." );
    return 0;
  }

  const char* curveLengthAsConstChar = outputModelNode->GetAttribute( OUTPUT_CURVE_LENGTH_ATTRIBUTE_NAME );
  return std::atof( curveLengthAsConstChar );
}

//-----------------------------------------------------------------
void vtkMRMLMarkupsToModelNode::SetOutputCurveLength( double curveLength )
{
  vtkMRMLModelNode* outputModelNode = vtkMRMLModelNode::SafeDownCast( this->GetOutputModelNode() );
  if ( outputModelNode == NULL )
  {
    vtkErrorMacro( "No output model node." );
    return;
  }

  std::stringstream curvestream;
  curvestream << curveLength;
  outputModelNode->SetAttribute( OUTPUT_CURVE_LENGTH_ATTRIBUTE_NAME, curvestream.str().c_str());
}

//-----------------------------------------------------------------
void vtkMRMLMarkupsToModelNode::ProcessMRMLEvents( vtkObject *caller, unsigned long /*event*/, void* /*callData*/ )
{
  vtkMRMLNode* callerNode = vtkMRMLNode::SafeDownCast( caller );
  if ( callerNode == NULL ) return;

  if ( this->GetInputNode() && this->GetInputNode()==caller )
  {
    this->InvokeCustomModifiedEvent(MarkupsPositionModifiedEvent);
  }
}

//-----------------------------------------------------------------
const char* vtkMRMLMarkupsToModelNode::GetOutputCurveLengthAttributeName()
{
  return OUTPUT_CURVE_LENGTH_ATTRIBUTE_NAME;
}

//-----------------------------------------------------------------
const char* vtkMRMLMarkupsToModelNode::GetModelTypeAsString( int id )
{
  switch ( id )
  {
  case ClosedSurface: return "closedSurface";
  case Curve: return "curve";
  default:
    // invalid id
    return "";
  }
}

//------------------------------------------------------------------------------
const char* vtkMRMLMarkupsToModelNode::GetCurveTypeAsString( int id )
{
  switch ( id )
  {
  case Linear: return "linear";
  case CardinalSpline: return "cardinalSpline";
  case KochanekSpline: return "kochanekSpline";
  case Polynomial: return "polynomial";
  default:
    // invalid id
    return "";
  }
}

//------------------------------------------------------------------------------
const char* vtkMRMLMarkupsToModelNode::GetPointParameterTypeAsString( int id )
{
  switch ( id )
  {
  case RawIndices: return "rawIndices";
  case MinimumSpanningTree: return "minimumSpanningTree";
  default:
    // invalid id
    return "";
  }
}

//------------------------------------------------------------------------------
const char* vtkMRMLMarkupsToModelNode::GetPolynomialFitTypeAsString( int id )
{
  switch ( id )
  {
  case GlobalLeastSquares: return "globalLeastSquares";
  case MovingLeastSquares: return "movingLeastSquares";
  default:
    // invalid id
    return "";
  }
}

//------------------------------------------------------------------------------
const char* vtkMRMLMarkupsToModelNode::GetPolynomialWeightTypeAsString( int id )
{
  switch ( id )
  {
  case Rectangular: return "rectangular";
  case Triangular: return "triangular";
  case Cosine: return "cosine";
  case Gaussian: return "gaussian";
  default:
    // invalid id
    return "";
  }
}

//------------------------------------------------------------------------------
int vtkMRMLMarkupsToModelNode::GetModelTypeFromString( const char* name )
{
  if ( name == NULL )
  {
    // invalid name
    return -1;
  }
  for ( int i = 0; i < ModelType_Last; i++ )
  {
    if ( strcmp( name, GetModelTypeAsString( i ) ) == 0 )
    {
      // found a matching name
      return i;
    }
  }
  // unknown name
  return -1;
}

//------------------------------------------------------------------------------
int vtkMRMLMarkupsToModelNode::GetCurveTypeFromString( const char* name )
{
  if ( name == NULL )
  {
    // invalid name
    return -1;
  }
  for ( int i = 0; i < CurveType_Last; i++ )
  {
    if ( strcmp( name, GetCurveTypeAsString( i ) ) == 0 )
    {
      // found a matching name
      return i;
    }
  }
  // unknown name
  return -1;
}

//------------------------------------------------------------------------------
int vtkMRMLMarkupsToModelNode::GetPointParameterTypeFromString( const char* name )
{
  if ( name == NULL )
  {
    // invalid name
    return -1;
  }
  for ( int i = 0; i < PointParameterType_Last; i++ )
  {
    if ( strcmp( name, GetPointParameterTypeAsString( i ) ) == 0 )
    {
      // found a matching name
      return i;
    }
  }
  // unknown name
  return -1;
}

//------------------------------------------------------------------------------
int vtkMRMLMarkupsToModelNode::GetPolynomialFitTypeFromString( const char* name )
{
  if ( name == NULL )
  {
    // invalid name
    return -1;
  }
  for ( int i = 0; i < PolynomialFitType_Last; i++ )
  {
    if ( strcmp( name, GetPolynomialFitTypeAsString( i ) ) == 0 )
    {
      // found a matching name
      return i;
    }
  }
  // unknown name
  return -1;
}


//------------------------------------------------------------------------------
int vtkMRMLMarkupsToModelNode::GetPolynomialWeightTypeFromString( const char* name )
{
  if ( name == NULL )
  {
    // invalid name
    return -1;
  }
  for ( int i = 0; i < PolynomialWeightType_Last; i++ )
  {
    if ( strcmp( name, GetPolynomialWeightTypeAsString( i ) ) == 0 )
    {
      // found a matching name
      return i;
    }
  }
  // unknown name
  return -1;
}

//------------------------------------------------------------------------------
vtkMRMLMarkupsFiducialNode* vtkMRMLMarkupsToModelNode::GetMarkupsNode()
{
  vtkWarningMacro( "vtkMRMLMarkupsToModelNode::GetMarkupsNode() is deprecated. Use vtkMRMLMarkupsToModelNode::GetInputNode() instead." );
  return vtkMRMLMarkupsFiducialNode::SafeDownCast( this->GetInputNode() );
}

//------------------------------------------------------------------------------
vtkMRMLModelNode* vtkMRMLMarkupsToModelNode::GetModelNode()
{
  vtkWarningMacro( "vtkMRMLMarkupsToModelNode::GetModelNode() is deprecated. Use vtkMRMLMarkupsToModelNode::GetOutputModelNode() instead." );
  return this->GetOutputModelNode();
}

//------------------------------------------------------------------------------
void vtkMRMLMarkupsToModelNode::SetAndObserveMarkupsNodeID( const char* id )
{
  vtkWarningMacro( "vtkMRMLMarkupsToModelNode::SetAndObserveMarkupsNodeID() is deprecated. Use vtkMRMLMarkupsToModelNode::SetAndObserveInputNodeID() instead." );
  this->SetAndObserveInputNodeID( id );
}

//------------------------------------------------------------------------------
void vtkMRMLMarkupsToModelNode::SetAndObserveModelNodeID( const char* id )
{
  vtkWarningMacro( "vtkMRMLMarkupsToModelNode::SetAndObserveModelNodeID() is deprecated. Use vtkMRMLMarkupsToModelNode::SetAndObserveOutputModelNodeID() instead." );
  this->SetAndObserveOutputModelNodeID( id );
}

//------------------------------------------------------------------------------
// DEPRECATED June 6, 2018
int vtkMRMLMarkupsToModelNode::GetInterpolationType()
{
  vtkGenericWarningMacro( "vtkMRMLMarkupsToModelNode::GetInterpolationType() is deprecated. Use vtkMRMLMarkupsToModelNode::GetCurveType() instead." );
  return this->GetCurveType();
}

//------------------------------------------------------------------------------
// DEPRECATED June 6, 2018
void vtkMRMLMarkupsToModelNode::SetInterpolationType( int newValue )
{
  vtkGenericWarningMacro( "vtkMRMLMarkupsToModelNode::SetInterpolationType() is deprecated. Use vtkMRMLMarkupsToModelNode::SetCurveType() instead." );
  this->SetCurveType( newValue );
}

//------------------------------------------------------------------------------
// DEPRECATED June 6, 2018
const char* vtkMRMLMarkupsToModelNode::GetInterpolationTypeAsString( int id )
{
  vtkGenericWarningMacro( "vtkMRMLMarkupsToModelNode::GetInterpolationTypeAsString() is deprecated. Use vtkMRMLMarkupsToModelNode::GetCurveTypeAsString() instead." );
  return vtkMRMLMarkupsToModelNode::GetCurveTypeAsString( id );
}

//------------------------------------------------------------------------------
// DEPRECATED June 6, 2018
int vtkMRMLMarkupsToModelNode::GetInterpolationTypeFromString( const char* name )
{
  vtkGenericWarningMacro( "vtkMRMLMarkupsToModelNode::GetInterpolationTypeFromString() is deprecated. Use vtkMRMLMarkupsToModelNode::GetCurveTypeFromString() instead." );
  return vtkMRMLMarkupsToModelNode::GetCurveTypeFromString( name );
}
