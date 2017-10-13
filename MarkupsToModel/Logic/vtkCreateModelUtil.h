#ifndef __vtkCreateModelUtil_h
#define __vtkCreateModelUtil_h

#include "vtkMRMLMarkupsToModelNode.h"

// vtk includes
#include <vtkPoints.h>
#include <vtkPolyData.h>

class vtkCreateModelUtil : public vtkObject
{
public:
  // Determine whether there is support for getting points from this type of MRMLNode
  static bool vtkMRMLNodeToVtkPointsSupported( vtkMRMLNode* node );

  // Get the points store in a vtkMRMLNode, if the type is supported
  static void vtkMRMLNodeToVtkPoints( vtkMRMLNode* node, vtkPoints* outputPoints );

  // Get the points store in a vtkMRMLMarkupsFiducialNode
  static void vtkMRMLNodeToVtkPoints( vtkMRMLMarkupsFiducialNode* markupsNode, vtkPoints* outputPoints );

  // Get the points store in a vtkMRMLModelNode
  static void vtkMRMLNodeToVtkPoints( vtkMRMLModelNode* modelNode, vtkPoints* outputPoints );

  // Remove duplicate points from a vtkPoints object
  static void RemoveDuplicatePoints( vtkPoints* points );
};

#endif
