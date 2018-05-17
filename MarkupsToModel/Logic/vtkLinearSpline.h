/*=========================================================================

  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkLinearSpline
 * @brief   computes an interpolating spline with piecewise linear segments
 *
 *
 * vtkLinearSpline is a concrete implementation of vtkSpline using
 * piecewise linearly interpolated segments.
 *
 * @sa
 * vtkSpline vtkCardinalSpline vtkKochanekSpline
*/

#ifndef vtkLinearSpline_h
#define vtkLinearSpline_h

#include "vtkSlicerMarkupsToModelModuleLogicExport.h" // For export macro

#include <vtkSpline.h>

class VTK_SLICER_MARKUPSTOMODEL_MODULE_LOGIC_EXPORT vtkLinearSpline : public vtkSpline
{
public:
  static vtkLinearSpline *New();

  vtkTypeMacro(vtkLinearSpline,vtkSpline);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Compute linear splines for each dependent variable
   */
  void Compute () override;

  /**
   * Evaluate a 1D linear spline.
   */
  double Evaluate (double t) override;

  /**
   * Deep copy of linear spline data.
   */
  void DeepCopy(vtkSpline *s) override;

protected:
  vtkLinearSpline();
  ~vtkLinearSpline() override {}

private:
  vtkLinearSpline(const vtkLinearSpline&) = delete;
  void operator=(const vtkLinearSpline&) = delete;
};

#endif
