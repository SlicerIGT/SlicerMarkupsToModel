/*=auto=========================================================================

  Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer
  Module:    $RCSfile: vtkMRMLMarkupsToModelNode.h,v $
  Date:      $Date: 2006/03/19 17:12:28 $
  Version:   $Revision: 1.6 $

=========================================================================auto=*/

#ifndef __vtkMRMLMarkupsToModelNode_h
#define __vtkMRMLMarkupsToModelNode_h

// std includes
#include <iostream>
#include <list>

// vtk includes
#include <vtkCommand.h>
#include <vtkObject.h>
#include <vtkObjectBase.h>
#include <vtkObjectFactory.h>

// Slicer includes
#include "vtkMRMLNode.h"
#include "vtkMRMLScene.h"

// MarkupsToModel includes
#include "vtkSlicerMarkupsToModelModuleMRMLExport.h"

class vtkMRMLModelNode;
class vtkMRMLMarkupsFiducialNode;

struct MarkupsTool{
  //vtkMRMLDisplayableNode* tool;
  int status;
  int playSound;
  unsigned long lastTimeStamp;
  unsigned long lastElapsedTimeStamp;
  std::string label;
  std::string id;

  MarkupsTool()
  {
    //tool=NULL;
    status=0;
    lastTimeStamp=0;
    playSound=0;
    label = "label";
    id = "";
    lastElapsedTimeStamp=0;
  }
};

class
VTK_SLICER_MARKUPSTOMODEL_MODULE_MRML_EXPORT
vtkMRMLMarkupsToModelNode
: public vtkMRMLNode
{
public:

  enum Events
  {
    /// MarkupsPositionModifiedEvent is called when markup point positions are modified.
    /// This make it easier for logic or other classes to observe any changes in input data.
    // vtkCommand::UserEvent + 777 is just a random value that is very unlikely to be used for anything else in this class
    MarkupsPositionModifiedEvent = vtkCommand::UserEvent + 777
  };

  enum ModelType
  {
    ClosedSurface =0,
    Curve,
    ModelType_Last // insert valid types above this line
  };

  enum CurveType
  {
    Linear = 0,
    CardinalSpline,
    KochanekSpline,
    Polynomial,
    CurveType_Last // insert valid types above this line
  };

  enum PointParameterType
  {
    RawIndices = 0,
    MinimumSpanningTree,
    PointParameterType_Last // insert valid types above this line
  };

  enum PolynomialFitType
  {
    GlobalLeastSquares = 0,
    MovingLeastSquares,
    PolynomialFitType_Last // insert valid types above this line
  };

  enum PolynomialWeightType
  {
    Rectangular = 0,
    Triangular,
    Cosine,
    Gaussian,
    PolynomialWeightType_Last // insert valid types above this line
  };

  vtkTypeMacro( vtkMRMLMarkupsToModelNode, vtkMRMLNode );

  // Standard MRML node methods
  static vtkMRMLMarkupsToModelNode *New();

  virtual vtkMRMLNode* CreateNodeInstance() override;
  virtual const char* GetNodeTagName() override { return "MarkupsToModel"; };
  void PrintSelf( ostream& os, vtkIndent indent ) override;
  virtual void ReadXMLAttributes( const char** atts ) override;
  virtual void WriteXML( ostream& of, int indent ) override;
  virtual void Copy( vtkMRMLNode *node ) override;

  vtkGetMacro( KochanekTension, double );
  vtkSetClampMacro( KochanekTension, double, -1.0, 1.0 );
  vtkGetMacro( KochanekBias, double );
  vtkSetClampMacro(KochanekBias, double, -1.0, 1.0);
  vtkGetMacro( KochanekContinuity, double );
  vtkSetClampMacro(KochanekContinuity, double, -1.0, 1.0);

  vtkGetMacro( PolynomialOrder, int );
  vtkSetClampMacro( PolynomialOrder, int, 1, VTK_INT_MAX );
  vtkGetMacro( PolynomialFitType, int );
  vtkSetClampMacro(PolynomialFitType, int, 0, PolynomialFitType_Last-1);
  vtkGetMacro( PolynomialSampleWidth, double );
  vtkSetClampMacro(PolynomialSampleWidth, double, 0.0, 1.0);
  vtkGetMacro( PolynomialWeightType, int );
  vtkSetClampMacro(PolynomialWeightType, int, 0, PolynomialWeightType_Last-1);

  vtkGetMacro( ModelType, int );
  vtkSetClampMacro(ModelType, int, 0, ModelType_Last-1);
  vtkGetMacro( CurveType, int );
  vtkSetClampMacro(CurveType, int, 0, CurveType_Last-1);
  vtkGetMacro( PointParameterType, int );
  vtkSetClampMacro(PointParameterType, int, 0, PointParameterType_Last-1);
  vtkGetMacro( TubeRadius, double );
  vtkSetMacro( TubeRadius, double );
  vtkGetMacro( TubeSegmentsBetweenControlPoints, int );
  vtkSetMacro( TubeSegmentsBetweenControlPoints, int );
  vtkGetMacro( TubeNumberOfSides, int );
  vtkSetMacro( TubeNumberOfSides, int );
  vtkGetMacro( TubeLoop, bool );
  vtkSetMacro( TubeLoop, bool );
  vtkBooleanMacro( TubeLoop, bool );
  vtkGetMacro( TubeCapping, bool );
  vtkSetMacro( TubeCapping, bool );
  vtkBooleanMacro(TubeCapping, bool);
  vtkGetMacro( KochanekEndsCopyNearestDerivatives, bool );
  vtkSetMacro( KochanekEndsCopyNearestDerivatives, bool );
  vtkBooleanMacro( KochanekEndsCopyNearestDerivatives, bool );
  

  vtkGetMacro( AutoUpdateOutput, bool );
  vtkSetMacro( AutoUpdateOutput, bool );
  vtkGetMacro( CleanMarkups, bool );
  vtkSetMacro( CleanMarkups, bool );
  vtkGetMacro( ButterflySubdivision, bool );
  vtkSetMacro( ButterflySubdivision, bool );
  vtkGetMacro( DelaunayAlpha, double );
  vtkSetMacro( DelaunayAlpha, double );
  vtkGetMacro( ConvexHull, bool );
  vtkSetMacro( ConvexHull, bool );

  double GetOutputCurveLength();
  void SetOutputCurveLength( double );

protected:

  // Constructor/destructor methods
  vtkMRMLMarkupsToModelNode();
  virtual ~vtkMRMLMarkupsToModelNode();
  vtkMRMLMarkupsToModelNode ( const vtkMRMLMarkupsToModelNode& );
  void operator=( const vtkMRMLMarkupsToModelNode& );

public:

  void SetAndObserveInputNodeID( const char* inputNodeId );
  void SetAndObserveOutputModelNodeID( const char* outputModelNodeId );
  void ProcessMRMLEvents( vtkObject *caller, unsigned long event, void *callData ) override;

  vtkMRMLNode * GetInputNode( );
  vtkMRMLModelNode* GetOutputModelNode( );

  static const char* GetOutputCurveLengthAttributeName();

  // Convert between model and interpolation types IDs and names.
  static const char* GetModelTypeAsString( int id );
  static const char* GetCurveTypeAsString( int id );
  static const char* GetPointParameterTypeAsString( int id );
  static const char* GetPolynomialFitTypeAsString( int id );
  static const char* GetPolynomialWeightTypeAsString( int id );
  static int GetModelTypeFromString( const char* name );
  static int GetCurveTypeFromString( const char* name );
  static int GetPointParameterTypeFromString( const char* name );
  static int GetPolynomialFitTypeFromString( const char* name );
  static int GetPolynomialWeightTypeFromString( const char* name );

  // DEPRECATED - Get the input node
  vtkMRMLMarkupsFiducialNode* GetMarkupsNode( );

  // DEPRECATED - Get the output node
  vtkMRMLModelNode* GetModelNode( );

  // DEPRECATED - Set the input node
  void SetAndObserveMarkupsNodeID( const char* id );

  // DEPRECATED - Set the output node
  void SetAndObserveModelNodeID( const char* id );

  // DEPRECATED June 6, 2018 - Get/Set curve type
  int GetInterpolationType();
  void SetInterpolationType( int );
  static int GetInterpolationTypeFromString( const char* name );
  static const char* GetInterpolationTypeAsString( int id );

private:
  int    ModelType;
  int    CurveType;
  int    PointParameterType;
  bool   AutoUpdateOutput;
  bool   CleanMarkups;
  bool   ButterflySubdivision;
  double DelaunayAlpha;
  bool   ConvexHull;
  double TubeRadius;
  int    TubeSegmentsBetweenControlPoints;
  int    TubeNumberOfSides;
  bool   TubeLoop;
  bool   TubeCapping;
  bool   KochanekEndsCopyNearestDerivatives;
  double KochanekTension;
  double KochanekBias;
  double KochanekContinuity;
  int    PolynomialOrder;
  int    PolynomialFitType;
  double PolynomialSampleWidth;
  int    PolynomialWeightType;
  double OutputCurveLength;
};

#endif
