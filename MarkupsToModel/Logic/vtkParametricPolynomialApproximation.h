/*=========================================================================

  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkParametricPolynomialApproximation
 * @brief   parametric function for 1D polynomials
 *
 * vtkParametricPolynomialApproximation is a parametric function for 1D approximating
 * polynomials. vtkParametricPolynomialApproximation maps the single parameter u to a
 * 3D point (x,y,z). Internally a polynomial is fit to a set of input
 * points using the least squares basis.
*/

#ifndef vtkParametricPolynomialApproximation_h
#define vtkParametricPolynomialApproximation_h

class vtkPoints;
class vtkDoubleArray;

#include "vtkSlicerMarkupsToModelModuleLogicExport.h" // For export macro

#include <vtkParametricFunction.h>
#include <vtkSmartPointer.h>

class VTK_SLICER_MARKUPSTOMODEL_MODULE_LOGIC_EXPORT vtkParametricPolynomialApproximation : public vtkParametricFunction
{
public:
  vtkTypeMacro(vtkParametricPolynomialApproximation,vtkParametricFunction);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkParametricPolynomialApproximation *New();

  /**
   * Return the parametric dimension of the class.
   */
  int GetDimension() override { return 1; }

  /**
   * Evaluate the parametric function at parametric coordinate u[0] returning
   * the point coordinate Pt[3].
   */
  void Evaluate( double u[ 3 ], double Pt[ 3 ], double Du[ 9 ] ) override;

  /**
   * Evaluate a scalar value at parametric coordinate u[0] and Pt[3].
   * Simply returns the parameter u[0].
   */
  double EvaluateScalar( double u[ 3 ], double Pt[ 3 ], double Du[ 9 ] ) override;

  //@{
  /**
   * Specify the order of polynomial (maximum exponent) that should be fit.
   */
  vtkGetMacro( PolynomialOrder, int );
  vtkSetMacro( PolynomialOrder, int );
  //@}

  //@{
  /**
   * Specify the list of points that the polynomial should approximate.
   * Set the point parameters that should be used during fitting with SetParameters.
   */
  void SetPoints( vtkPoints* );
  //@}

  //@{
  /**
   * Specify the parameters for the points. Length of list should be the same,
   * and the points should be in the same order as the parameters.
   */
  void SetParameters( vtkDoubleArray* );
  //@}

protected:
  vtkParametricPolynomialApproximation();
  ~vtkParametricPolynomialApproximation() override;

private:
  // Fitting the polynomial
  vtkMTimeType FitTime;
  bool Fit();
  static void FitLeastSquaresPolynomials( vtkDoubleArray* parameters, vtkPoints* points, int polynomialOrder, vtkDoubleArray* coefficients );

  int PolynomialOrder;
  vtkSmartPointer< vtkPoints > Points;
  vtkSmartPointer< vtkDoubleArray > Parameters;

  // polynomial coefficients are computed in the fitting operation
  vtkSmartPointer< vtkDoubleArray > Coefficients;

  vtkParametricPolynomialApproximation( const vtkParametricPolynomialApproximation& ) = delete;
  void operator=( const vtkParametricPolynomialApproximation& ) = delete;
};

#endif
