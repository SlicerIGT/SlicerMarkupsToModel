#ifndef __vtkCreateModelUtil_h
#define __vtkCreateModelUtil_h

#include "vtkMRMLMarkupsToModelNode.h"

// vtk includes
#include <vtkPoints.h>
#include <vtkPolyData.h>

#include "vtkSlicerMarkupsToModelModuleLogicExport.h"

class VTK_SLICER_MARKUPSTOMODEL_MODULE_LOGIC_EXPORT vtkCreateModelUtil : public vtkObject
{
public:
  // standard vtk object methods
  vtkTypeMacro( vtkCreateModelUtil, vtkObject );
  void PrintSelf( ostream& os, vtkIndent indent ) VTK_OVERRIDE;
  static vtkCreateModelUtil *New();
  
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

protected:
  vtkCreateModelUtil();
  ~vtkCreateModelUtil();

private:
  // not used
  vtkCreateModelUtil ( const vtkCreateModelUtil& ) VTK_DELETE_FUNCTION;
  void operator= ( const vtkCreateModelUtil& ) VTK_DELETE_FUNCTION;
};

#endif
