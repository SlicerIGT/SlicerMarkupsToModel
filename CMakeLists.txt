cmake_minimum_required(VERSION 3.5)

project(MarkupsToModel)

#-----------------------------------------------------------------------------
# Extension meta-information
set(EXTENSION_HOMEPAGE "https://github.com/SlicerIGT/MarkupsToModel")
set(EXTENSION_CATEGORY "Informatics")
set(EXTENSION_CONTRIBUTORS "Thomas Vaughan (PerkLab, Queen's University),\
  Andras Lasso (PerkLab, Queen's University), Adam Rankin (Robarts Research Center, Western University),\
  Aniqah Mair (PerkLab, Queen's University), Jaime Garcia Guevara (PerkLab, Queen's University)")
set(EXTENSION_DESCRIPTION "Create tube or closed surface models from points of a markup fiducial node.")
set(EXTENSION_ICONURL "https://github.com/SlicerIGT/SlicerMarkupsToModel/raw/master/MarkupsToModel.png")
set(EXTENSION_SCREENSHOTURLS "\
  https://github.com/SlicerIGT/SlicerMarkupsToModel/raw/master/ClosedSurface.png \
  https://github.com/SlicerIGT/SlicerMarkupsToModel/raw/master/Curve.png \
  ")
set(EXTENSION_DEPENDS "NA") # Specified as a space separated string, a list or 'NA' if any

#-----------------------------------------------------------------------------
# Extension dependencies
find_package(Slicer REQUIRED)
include(${Slicer_USE_FILE})

#-----------------------------------------------------------------------------
# Extension modules
add_subdirectory(MarkupsToModel)
## NEXT_MODULE

#-----------------------------------------------------------------------------
include(${Slicer_EXTENSION_GENERATE_CONFIG})
include(${Slicer_EXTENSION_CPACK})
