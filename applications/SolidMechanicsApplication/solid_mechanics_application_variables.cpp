//
//   Project Name:        KratosSolidMechanicsApplication $
//   Created by:          $Author:            JMCarbonell $
//   Last modified by:    $Co-Author:                     $
//   Date:                $Date:            February 2016 $
//   Revision:            $Revision:                  0.0 $
//
//

#include "solid_mechanics_application_variables.h"

namespace Kratos
{
  ///@name Type Definitions
  ///@{
  typedef array_1d<double,3> Vector3;
  typedef array_1d<double,6> Vector6;
  ///@}

  ///@name Kratos Globals
  ///@{

  //Create Variables

  // Generalized eigenvalue problem
  KRATOS_CREATE_VARIABLE( int, SOLID_BUILD_LEVEL )
  KRATOS_CREATE_VARIABLE( Vector, SOLID_EIGENVALUE_VECTOR)
  KRATOS_CREATE_VARIABLE( Matrix , SOLID_EIGENVECTOR_MATRIX )

  //explicit schemes
  KRATOS_CREATE_3D_VARIABLE_WITH_COMPONENTS( MIDDLE_VELOCITY )

  //solution
  KRATOS_CREATE_VARIABLE( int, WRITE_ID )
  KRATOS_CREATE_VARIABLE( double, RAYLEIGH_ALPHA )
  KRATOS_CREATE_VARIABLE( double, RAYLEIGH_BETA )
  
  //geometrical
  KRATOS_CREATE_VARIABLE( double, AREA )
  KRATOS_CREATE_VARIABLE( double, IX )
  KRATOS_CREATE_VARIABLE( double, IY )
  KRATOS_CREATE_VARIABLE( double, IZ )
  KRATOS_CREATE_VARIABLE( double, CROSS_AREA )
  KRATOS_CREATE_VARIABLE( double, MEAN_RADIUS )  
  KRATOS_CREATE_VARIABLE( int,    SECTION_SIDES )
  KRATOS_CREATE_VARIABLE( Matrix ,GEOMETRIC_STIFFNESS )

  //cross section
  KRATOS_CREATE_VARIABLE( ShellCrossSection::Pointer, SHELL_CROSS_SECTION )
  KRATOS_CREATE_VARIABLE( int, SHELL_CROSS_SECTION_OUTPUT_PLY_ID )
  KRATOS_CREATE_VARIABLE( double, SHELL_CROSS_SECTION_OUTPUT_PLY_LOCATION )

  //shell generalized variables
  KRATOS_CREATE_VARIABLE( Matrix, SHELL_STRAIN )
  KRATOS_CREATE_VARIABLE( Matrix, SHELL_STRAIN_GLOBAL )
  KRATOS_CREATE_VARIABLE( Matrix, SHELL_CURVATURE )
  KRATOS_CREATE_VARIABLE( Matrix, SHELL_CURVATURE_GLOBAL )
  KRATOS_CREATE_VARIABLE( Matrix, SHELL_FORCE )
  KRATOS_CREATE_VARIABLE( Matrix, SHELL_FORCE_GLOBAL )
  KRATOS_CREATE_VARIABLE( Matrix, SHELL_MOMENT )
  KRATOS_CREATE_VARIABLE( Matrix, SHELL_MOMENT_GLOBAL )
  
  //nodal load variables
  KRATOS_CREATE_3D_VARIABLE_WITH_COMPONENTS( POINT_LOAD )
  KRATOS_CREATE_3D_VARIABLE_WITH_COMPONENTS( POINT_MOMENT )
  KRATOS_CREATE_3D_VARIABLE_WITH_COMPONENTS( LINE_LOAD )
  KRATOS_CREATE_3D_VARIABLE_WITH_COMPONENTS( SURFACE_LOAD )
  
  KRATOS_CREATE_3D_VARIABLE_WITH_COMPONENTS( FOLLOWER_POINT_LOAD )
  KRATOS_CREATE_3D_VARIABLE_WITH_COMPONENTS( FOLLOWER_POINT_MOMENT )
  KRATOS_CREATE_3D_VARIABLE_WITH_COMPONENTS( FOLLOWER_LINE_LOAD )
  KRATOS_CREATE_3D_VARIABLE_WITH_COMPONENTS( FOLLOWER_SURFACE_LOAD )
  
  //condition load variables
  KRATOS_CREATE_VARIABLE( Vector, POINT_LOADS_VECTOR )
  KRATOS_CREATE_VARIABLE( Vector, POINT_MOMENTS_VECTOR )
  KRATOS_CREATE_VARIABLE( Vector, LINE_LOADS_VECTOR )
  KRATOS_CREATE_VARIABLE( Vector, SURFACE_LOADS_VECTOR )
  KRATOS_CREATE_VARIABLE( Vector, POSITIVE_FACE_PRESSURES_VECTOR )
  KRATOS_CREATE_VARIABLE( Vector, NEGATIVE_FACE_PRESSURES_VECTOR )
  
  //element
  KRATOS_CREATE_VARIABLE( double, VON_MISES_STRESS )

  //nodal dofs
  KRATOS_CREATE_VARIABLE( double, PRESSURE_REACTION )


  ///@}

}
