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

//-----------------------------------------------------------------
vtkMRMLMarkupsToModelNode* vtkMRMLMarkupsToModelNode::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance( "vtkMRMLMarkupsToModelNode" );
  if( ret )
  {
    return ( vtkMRMLMarkupsToModelNode* )ret;
  }
  // If the factory was unable to create the object, then create it here.
  return new vtkMRMLMarkupsToModelNode;
}

//-----------------------------------------------------------------
vtkMRMLMarkupsToModelNode::vtkMRMLMarkupsToModelNode()
{
  this->HideFromEditorsOff();
  this->SetSaveWithScene( true );

  vtkNew<vtkIntArray> events;
  events->InsertNextValue( vtkCommand::ModifiedEvent );
  events->InsertNextValue( vtkMRMLMarkupsNode::PointModifiedEvent );//PointEndInteractionEvent
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
vtkMRMLNode* vtkMRMLMarkupsToModelNode::CreateNodeInstance()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance( "vtkMRMLMarkupsToModelNode" );
  if( ret )
    {
      return ( vtkMRMLMarkupsToModelNode* )ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkMRMLMarkupsToModelNode;
}

//-----------------------------------------------------------------
void vtkMRMLMarkupsToModelNode::WriteXML( ostream& of, int nIndent )
{
  Superclass::WriteXML(of, nIndent); // This will take care of referenced nodes
  vtkIndent indent(nIndent);

  of << indent << " ModelType =\"" << this->GetModelTypeAsString(this->ModelType) << "\"";
  of << indent << " AutoUpdateOutput =\"" << (this->AutoUpdateOutput ? "true" : "false") << "\"";
  of << indent << " CleanMarkups =\"" << (this->CleanMarkups ? "true" : "false") << "\"";
  of << indent << " ConvexHull =\"" << (this->ConvexHull ? "true" : "false") << "\"";
  of << indent << " ButterflySubdivision =\"" << (this->ButterflySubdivision ? "true" : "false") << "\"";
  of << indent << " DelaunayAlpha =\"" << this->DelaunayAlpha << "\"";
  of << indent << " CurveType=\"" << this->GetCurveTypeAsString(this->CurveType) << "\"";
  of << indent << " PointParameterType=\"" << this->GetPointParameterTypeAsString(this->PointParameterType) << "\"";
  of << indent << " TubeRadius=\"" << this->TubeRadius << "\"";
  of << indent << " TubeNumberOfSides=\"" << this->TubeNumberOfSides << "\"";
  of << indent << " TubeSegmentsBetweenControlPoints=\"" << this->TubeSegmentsBetweenControlPoints << "\"";
  of << indent << " TubeLoop=\"" << ( this->TubeLoop ? "true" : "false" ) << "\"";
  of << indent << " KochanekEndsCopyNearestDerivatives=\"" << ( this->KochanekEndsCopyNearestDerivatives ? "true" : "false" ) << "\"";
  of << indent << " KochanekBias=\"" << this->KochanekBias << "\"";
  of << indent << " KochanekContinuity=\"" << this->KochanekContinuity << "\"";
  of << indent << " KochanekTension=\"" << this->KochanekTension << "\"";
  of << indent << " PolynomialOrder=\"" << this->PolynomialOrder << "\"";
  of << indent << " PolynomialFitType=\"" << this->GetPolynomialFitTypeAsString( this->PolynomialFitType ) << "\"";
  of << indent << " PolynomialSampleWidth=\"" << this->PolynomialSampleWidth << "\"";
  of << indent << " PolynomialWeightType=\"" << this->GetPolynomialWeightTypeAsString( this->PolynomialWeightType ) << "\"";
}

//-----------------------------------------------------------------
void vtkMRMLMarkupsToModelNode::ReadXMLAttributes( const char** atts )
{
  int disabledModify = this->StartModify();

  Superclass::ReadXMLAttributes(atts); // This will take care of referenced nodes

  // Read all MRML node attributes from two arrays of names and values
  const char* attName;
  const char* attValue;

  while (*atts != NULL)
  {
    attName  = *(atts++);
    attValue = *(atts++);
    if ( strcmp( attName, "ModelType" ) == 0 )
    {
      int typeAsInt = this->GetModelTypeFromString( attValue );
      if ( typeAsInt < 0 || typeAsInt >= ModelType_Last)
      {
        vtkWarningMacro( "Unrecognized model type read from MRML node: " << attValue << ". Setting to ClosedSurface." );
        typeAsInt = this->ClosedSurface;
      }
      this->SetModelType( typeAsInt );
    }
    else if ( strcmp( attName, "AutoUpdateOutput" ) == 0 )
    {
      this->SetAutoUpdateOutput(!strcmp(attValue,"true"));
    }
    else if ( strcmp( attName, "CleanMarkups" ) == 0 )
    {
      this->SetCleanMarkups(!strcmp(attValue,"true"));
    }
    else if ( strcmp( attName, "ConvexHull" ) == 0 )
    {
      this->SetConvexHull(!strcmp(attValue,"true"));
    }
    else if ( strcmp( attName, "ButterflySubdivision" ) == 0 )
    {
      this->SetButterflySubdivision(!strcmp(attValue,"true"));
    }
    else if ( strcmp( attName, "DelaunayAlpha" ) == 0 )
    {
      double delaunayAlpha = 0.0;
      std::stringstream nameString;
      nameString << attValue;
      nameString >> delaunayAlpha;
      this->SetDelaunayAlpha( delaunayAlpha );
    }
    // CurveType is formerly InterpolationType (now deprecated)
    else if ( strcmp( attName, "InterpolationType" ) == 0 || strcmp( attName, "CurveType" ) == 0 )
    {
      int typeAsInt = this->GetCurveTypeFromString( attValue );
      if ( typeAsInt < 0 || typeAsInt >= CurveType_Last )
      {
        vtkWarningMacro( "Unrecognized curve type read from MRML node: " << attValue << ". Setting to Linear." );
        typeAsInt = this->Linear;
      }
      this->SetCurveType( typeAsInt );
    }
    else if ( strcmp( attName, "PointParameterType" ) == 0 )
    {
      int typeAsInt = this->GetPointParameterTypeFromString( attValue );
      if ( typeAsInt < 0 || typeAsInt >= PointParameterType_Last )
      {
        vtkWarningMacro( "Unrecognized point parameter type read from MRML node: " << attValue << ". Setting to RawIndices." );
        typeAsInt = this->RawIndices;
      }
      this->SetPointParameterType( typeAsInt );
    }
    else if ( strcmp( attName, "TubeRadius" ) == 0 )
    {
      double tubeRadius = 0.0;
      std::stringstream nameString;
      nameString << attValue;
      nameString >> tubeRadius;
      this->SetTubeRadius( tubeRadius );
    }
    else if ( strcmp( attName, "TubeNumberOfSides" ) == 0 )
    {
      double tubeNumberOfSides = 0.0;
      std::stringstream nameString;
      nameString << attValue;
      nameString >> tubeNumberOfSides;
      this->SetTubeNumberOfSides( tubeNumberOfSides );
    }
    else if ( strcmp( attName, "TubeSegmentsBetweenControlPoints" ) == 0 )
    {
      double tubeSegmentsBetweenControlPoints = 0.0;
      std::stringstream nameString;
      nameString << attValue;
      nameString >> tubeSegmentsBetweenControlPoints;
      this->SetTubeSegmentsBetweenControlPoints( tubeSegmentsBetweenControlPoints );
    }
    else if ( strcmp( attName, "TubeLoop" ) == 0 )
    {
      bool isTrue = !strcmp( attValue, "true" );
      this->SetTubeLoop( isTrue );
    }
    else if ( strcmp( attName, "KochanekEndsCopyNearestDerivatives" ) == 0 )
    {
      bool isTrue = !strcmp( attValue, "true" );
      this->SetKochanekEndsCopyNearestDerivatives( isTrue );
    }
    else if ( strcmp( attName, "KochanekBias" ) == 0 )
    {
      double kochanekBias = 0.0;
      std::stringstream nameString;
      nameString << attValue;
      nameString >> kochanekBias;
      if ( kochanekBias < -1.0 )
      {
        vtkWarningMacro( "Kochanek Bias " << kochanekBias << " is too small. Setting to -1.0.)" );
        kochanekBias = -1.0;
      }
      else if ( kochanekBias > 1.0 )
      {
        vtkWarningMacro( "Kochanek Bias " << kochanekBias << " is too large. Setting to 1.0.)" );
        kochanekBias = 1.0;
      }
      this->SetKochanekBias( kochanekBias );
    }
    else if ( strcmp( attName, "KochanekContinuity" ) == 0 )
    {
      double kochanekContinuity = 0.0;
      std::stringstream nameString;
      nameString << attValue;
      nameString >> kochanekContinuity;
      if ( kochanekContinuity < -1.0 )
      {
        vtkWarningMacro( "Kochanek Continuity " << kochanekContinuity << " is too small. Setting to -1.0.)" );
        kochanekContinuity = -1.0;
      }
      else if ( kochanekContinuity > 1.0 )
      {
        vtkWarningMacro( "Kochanek Continuity " << kochanekContinuity << " is too large. Setting to 1.0.)" );
        kochanekContinuity = 1.0;
      }
      this->SetKochanekContinuity( kochanekContinuity );
    }
    else if ( strcmp( attName, "KochanekTension" ) == 0 )
    {
      double kochanekTension = 0.0;
      std::stringstream nameString;
      nameString << attValue;
      nameString >> kochanekTension;
      if ( kochanekTension < -1.0 )
      {
        vtkWarningMacro( "Kochanek Tension " << kochanekTension << " is too small. Setting to -1.0.)" );
        kochanekTension = -1.0;
      }
      else if ( kochanekTension > 1.0 )
      {
        vtkWarningMacro( "Kochanek Tension " << kochanekTension << " is too large. Setting to 1.0.)" );
        kochanekTension = 1.0;
      }
      this->SetKochanekTension( kochanekTension );
    }
    else if ( strcmp( attName, "PolynomialOrder" ) == 0 )
    {
      int polynomialOrder = 1;
      std::stringstream nameString;
      nameString << attValue;
      nameString >> polynomialOrder;
      if ( polynomialOrder < 1 )
      {
        vtkWarningMacro( "Polynomial Order " << polynomialOrder << " is too small. Setting to 1.)" );
        polynomialOrder = 1;
      }
      this->SetPolynomialOrder( polynomialOrder );
    }
    else if ( strcmp( attName, "PolynomialFitType" ) == 0 )
    {
      int typeAsInt = this->GetPolynomialFitTypeFromString( attValue );
      if ( typeAsInt < 0 || typeAsInt >= PolynomialFitType_Last )
      {
        vtkWarningMacro( "Unrecognized polynomial fit type read from MRML node: " << attValue << ". Setting to GlobalLeastSquares." );
        typeAsInt = this->GlobalLeastSquares;
      }
      this->SetPolynomialFitType( typeAsInt );
    }
    else if ( strcmp( attName, "PolynomialSampleWidth" ) == 0 )
    {
      double polynomialSampleWidth = 0.0;
      std::stringstream nameString;
      nameString << attValue;
      nameString >> polynomialSampleWidth;
      if ( polynomialSampleWidth < 0.0 )
      {
        vtkWarningMacro( "Polynomial sample width " << polynomialSampleWidth << " is too small. Setting to 0.0.)" );
        polynomialSampleWidth = 0.0;
      }
      else if ( polynomialSampleWidth > 1.0 )
      {
        vtkWarningMacro( "Polynomial sample width " << polynomialSampleWidth << " is too large. Setting to 1.0.)" );
        polynomialSampleWidth = 1.0;
      }
      this->SetPolynomialSampleWidth( polynomialSampleWidth );
    }
    else if ( strcmp( attName, "PolynomialWeightType" ) == 0 )
    {
      int typeAsInt = this->GetPolynomialWeightTypeFromString( attValue );
      if ( typeAsInt < 0 || typeAsInt >= PolynomialWeightType_Last )
      {
        vtkWarningMacro("Unrecognized polynomial weight type read from MRML node: " << attValue << ". Setting to Rectangular.");
        typeAsInt = vtkMRMLMarkupsToModelNode::Rectangular;
      }
      this->SetPolynomialWeightType( typeAsInt );
    }
  }

  this->EndModify( disabledModify );
}

//-----------------------------------------------------------------
void vtkMRMLMarkupsToModelNode::Copy( vtkMRMLNode *anode )
{  
  Superclass::Copy( anode ); // This will take care of referenced nodes
  this->Modified();
}

//-----------------------------------------------------------------
void vtkMRMLMarkupsToModelNode::PrintSelf( ostream& os, vtkIndent indent )
{
  vtkMRMLNode::PrintSelf(os,indent); // This will take care of referenced nodes
  os << indent << "ModelID: ";
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
  const char* curveLengthAsConstChar = curvestream.str().c_str();
  outputModelNode->SetAttribute( OUTPUT_CURVE_LENGTH_ATTRIBUTE_NAME, curveLengthAsConstChar );
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
