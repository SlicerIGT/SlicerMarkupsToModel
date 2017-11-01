# Overview

**Slicer Markups to Model** is an extension of [3D Slicer](https://www.slicer.org/) for creating 3D surface models. The user first specifies a series of input points (as fiducial markups), then the module creates a model from those points. Currently there are two types of surface models that can be created: closed surfaces and curves.

![Overview](https://raw.githubusercontent.com/SlicerIGT/SlicerMarkupsToModel/master/Screenshots/Overview.png?raw=true)
> Example models created using SlicerMarkupsToModel. Left: Closed Surface. Right: Curves.

![UseCaseClosedSurface](https://raw.githubusercontent.com/SlicerIGT/SlicerMarkupsToModel/master/Screenshots/UseCaseIntraopTumorSegmentation.PNG)
> A tumor (in phantom) is segmented on ultrasound as a closed surface. [Link](http://imno.ca/sites/default/files/ImNO%202017%20Full%20Program.pdf) (See "Electromagnetically-generated catheter paths for breast brachytherapy")

![UseCaseCurve](https://raw.githubusercontent.com/SlicerIGT/SlicerMarkupsToModel/master/Screenshots/UseCaseCatheterReconstruction.PNG)
> Curves are used to reconstruct catheter paths in a phantom. [Link](https://dx.doi.org/10.1109/TBME.2015.2466591)

# GUI Features

![GUI](https://raw.githubusercontent.com/SlicerIGT/SlicerMarkupsToModel/master/Screenshots/GUI.png)
> The main GUI for this module

The **parameter node** is used to store all options settings for the module. The parameter node can also be saved along with a Slicer scene. When the scene is later re-opened, all options and settings should be preserved.

The two radio buttons along the top indicate whether the model should be a **closed surface** or a **curve**.

The **Input Node** is used to store input points for the model. There is an option to create a new Markup Fiducials from the drop-down selector. Fiducial markups can be added or deleted using buttons beside the selector. (*Note that advanced users can also take as input a "Model" and work with points from polygonal data).

The **Output Model Node** stores the model created by this module.

The **Update Button** can be set either to manual mode (updates only happen when the button is clicked), or to automatic mode (updates happen whenever the parameters are changed or when the input points are changed). Click on the checkbox to toggle between these two modes.

The **Display Panel** allows convenient access to change basic rendering properties of the model and input markups.

![DisplayPanel](https://raw.githubusercontent.com/SlicerIGT/SlicerMarkupsToModel/master/Screenshots/DisplayPanel.png)
> The Display Panel as seen in the GUI.

- **Model Visibility**: Toggle visibility in all views.

- **Model Opacity**: Change opacity. (Larger = more opaque)

- **Model Color**: Change the color of the model in all views.

- **Model Slice Intersections**: Toggle visibility in the red, yellow, and green slice views. Model Visibility (above) must also be enabled.

- **Markups Text Scale**: Change the size of text beside each input markup fiducial.

# Closed Surfaces

![ClosedSurfaceExample](https://raw.githubusercontent.com/SlicerIGT/SlicerMarkupsToModel/master/Screenshots/ClosedSurfaceExample.png)
> Left: Example of a closed surface. Right: Example of how a closed surface might look in slice view.

Closed surfaces are [convex models](https://en.wikipedia.org/wiki/Convex_hull) that enclose the input points. Every time a new input point is added, the closed surface expands to contain the new input point. There are a variety of parameters that can be tweaked through the **Advanced Panel**.

![AdvancedPanelClosedSurface](https://raw.githubusercontent.com/SlicerIGT/SlicerMarkupsToModel/master/Screenshots/AdvancedPanelClosedSurface.png)
> Advanced panel when working with a closed surface.

- **Clean Duplicated Markups**: Remove duplicates from the input points

- **Smoothing**: Make the closed surface smoother by using the "Butterfly Subdivision". Note that sometimes concavities and self-intersections will occur after applying this smoothing. See "Force Convex Output" below.

- **Convexity**: Only tetrahedral 'cells' contained in a sphere with this radius will be used to construct the model. Concavities can be introduced by changing this value. (*Note that 0.0 means that this feature is ignored, 0.0 is recommended for most applications.*)

- **Force Convex Output**: The model will become fully convex after all other operations. Used to correct self-intersections introduced by butterfly subdivision.

# Curves

![CurveExample](https://raw.githubusercontent.com/SlicerIGT/SlicerMarkupsToModel/master/Screenshots/CurveExample.png)
> Left: Example of a curve. Right: Example of how a curve might look in slice view.

Curves are models shaped like a tube that either [interpolate](https://en.wikipedia.org/wiki/Spline_interpolation) the input points, or [approximate](https://en.wikipedia.org/wiki/Polynomial_regression) them in a best fit. There are in fact four types of curve in this module: Piecewise Linear, Cardinal Spline, Kochanek Spline, and Polynomial. All curves share the following parameters that can be seen on the **Advanced Panel**.

![AdvancedPanelCurve](https://raw.githubusercontent.com/SlicerIGT/SlicerMarkupsToModel/master/Screenshots/AdvancedPanelCurve.png)
> Advanced panel when working with a curve.

- **Radius**: Changes the radius of the tube model (larger = wider tube)

- **Number of Sides**: Changes the outer smoothness of a tube model (larger = smoother appearance)

- **Segments Per Point**: Changes the number of points used to interpolate/approximate a curve. (larger = smoother appearance)

- **Clean Duplicated Markups**: Remove duplicates from the input points

- **Curve is a Loop**: Indicate if the Curve should loop from the last point back to the first point.

**Piecewise linear** curves are the simplest type of curve that can be created. A tube model is created that passes from one input point to the next in the original order specified from the fiducial list.

**Cardinal spline** curves appear smooth. A tube model is created that passes through each input point in the order specified from the fiducial list. Between each pair of points, there will be some curvature in the model. See [Wikipedia](https://en.wikipedia.org/wiki/Cubic_Hermite_spline#Cardinal_spline) to learn more about cardinal splines.

**Kochanek spline** curves appear smooth. A tube model is created that passes through each input point in the order specified from the fiducial list. Between each pair of points, there will be some curvature in the model. See [Wikipedia](https://en.wikipedia.org/wiki/Kochanek%E2%80%93Bartels_spline) to learn more about Kochanek splines.

There are a few paramters specific to Kochanek Splines, as seen in the **Advanced Panel**.

![AdvancedPanelCurveKochanek](https://raw.githubusercontent.com/SlicerIGT/SlicerMarkupsToModel/master/Screenshots/AdvancedPanelCurveKochanek.png)
> Additional parameters shown on the Advanced Panel for Kochanek splines.

- **Ends copy nearest derivative**: The first and last points should use the tangent from their nearest neighboring points.

- **Kochanek Bias**, **Kochanek Tension**, and **Kochanek Continuity**: As indicated on [Wikipedia](https://en.wikipedia.org/wiki/Kochanek%E2%80%93Bartels_spline).

**Polynomial** curves appear smooth. Unlike the other types of curve, polynomials approximate the input points instead of interpolating them. The curve is represented using a tube-shaped model.

There are a few parameters specific to Polynomial curves, as seen in the **Advanced Panel**.

![AdvancedPanelCurvePolynomial](https://raw.githubusercontent.com/SlicerIGT/SlicerMarkupsToModel/master/Screenshots/AdvancedPanelCurvePolynomial.png)
> Additional parameters shown on the Advanced Panel for polynomials.

- **Polynomial Order**: How closely the polynomial should follow the input points. (Larger = closer fit, but also increased risk of [overfitting](https://en.wikipedia.org/wiki/Overfitting))

- **Point Parameters**: This tells the module how to determine the order of the input points. If the input points are already in order, use "*Indices*". If the point order is unknown *but* the polynomial should connect the farthest two points, use "*Minimum Spanning Tree*".
