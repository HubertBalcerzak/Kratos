//    |  /           |
//    ' /   __| _` | __|  _ \   __|
//    . \  |   (   | |   (   |\__ `
//   _|\_\_|  \__,_|\__|\___/ ____/
//                   Multi-Physics
//
//  License:		 BSD License
//					 Kratos default license: kratos/license.txt
//
//  Main authors:    Philipp Bucher
//

// System includes

// External includes

// Project includes
#include "structural_mechanics_element_utilities.h"
#include "structural_mechanics_application_variables.h"
#include "utilities/math_utils.h"

namespace Kratos {
namespace StructuralMechanicsElementUtilities {

bool ComputeLumpedMassMatrix(
    const Properties& rProperites,
    const ProcessInfo& rCurrentProcessInfo)
{
    // giving the globally defined setting (through ProcessInfo) priority
    // over the locally defined one (through Properties)
    // this is needed for the explicit solver, which requires the lumped
    // mass matrix and specifies it's computation through the ProcessInfo
    if (rCurrentProcessInfo.Has(COMPUTE_LUMPED_MASS_MATRIX)) {
        return rCurrentProcessInfo[COMPUTE_LUMPED_MASS_MATRIX];
    }
    else if (rProperites.Has(COMPUTE_LUMPED_MASS_MATRIX)) {
        return rProperites[COMPUTE_LUMPED_MASS_MATRIX];
    }

    // the default for all elements in StructuralMechanics is
    // to use the consistent-mass-matrix, hence returning false here
    return false;
}

bool HasRayleighDamping(
    const Properties& rProperites,
    const ProcessInfo& rCurrentProcessInfo)
{
    return (GetRayleighAlpha(rProperites, rCurrentProcessInfo) > 0.0 ||
            GetRayleighBeta(rProperites, rCurrentProcessInfo) > 0.0);
}

double GetRayleighAlpha(
    const Properties& rProperites,
    const ProcessInfo& rCurrentProcessInfo)
{
    // giving the locally defined setting (through Properties) priority
    // over the globally defined one (through ProcessInfo)
    if (rProperites.Has(RAYLEIGH_ALPHA)) {
        return rProperites[RAYLEIGH_ALPHA];
    }
    else if (rCurrentProcessInfo.Has(RAYLEIGH_ALPHA)) {
        return rCurrentProcessInfo[RAYLEIGH_ALPHA];
    }

    return 0.0;
}

double GetRayleighBeta(
    const Properties& rProperites,
    const ProcessInfo& rCurrentProcessInfo)
{
    // giving the locally defined setting (through Properties) priority
    // over the globally defined one (through ProcessInfo)
    if (rProperites.Has(RAYLEIGH_BETA)) {
        return rProperites[RAYLEIGH_BETA];
    }
    else if (rCurrentProcessInfo.Has(RAYLEIGH_BETA)) {
        return rCurrentProcessInfo[RAYLEIGH_BETA];
    }

    return 0.0;
}

void CalculateRayleighDampingMatrix(
    Element& rElement,
    Element::MatrixType& rDampingMatrix,
    /*const*/ProcessInfo& rCurrentProcessInfo,
    const std::size_t MatrixSize)
{
    KRATOS_TRY;

    // 1.-Resizing if needed
    if (rDampingMatrix.size1() != MatrixSize || rDampingMatrix.size2() != MatrixSize) {
        rDampingMatrix.resize( MatrixSize, MatrixSize, false );
    }
    noalias(rDampingMatrix) = ZeroMatrix(MatrixSize, MatrixSize);

    if (!HasRayleighDamping(rElement.GetProperties(), rCurrentProcessInfo)) {
        // Do nothing if no rayleigh-damping is specified
        return;
    }

    // 2.-Calculate StiffnessMatrix:
    Element::MatrixType stiffness_matrix;
    rElement.CalculateLeftHandSide(stiffness_matrix, rCurrentProcessInfo);

    // 3.-Calculate MassMatrix:
    Element::MatrixType mass_matrix;
    rElement.CalculateMassMatrix(mass_matrix, rCurrentProcessInfo);

    // 4.-Compose the Damping Matrix:
    // Rayleigh Damping Matrix: alpha*M + beta*K
    noalias(rDampingMatrix) += GetRayleighAlpha(rElement.GetProperties(), rCurrentProcessInfo) * mass_matrix;
    noalias(rDampingMatrix) += GetRayleighBeta(rElement.GetProperties(), rCurrentProcessInfo)  * stiffness_matrix;

    KRATOS_CATCH( "CalculateRayleighDampingMatrix" )
}

double CalculateReferenceLength2D2N(const Element& rElement)
{
    KRATOS_TRY;

    const array_1d<double, 3> delta_pos =
        rElement.GetGeometry()[1].GetInitialPosition().Coordinates() -
        rElement.GetGeometry()[0].GetInitialPosition().Coordinates();

    return std::sqrt((delta_pos[0] * delta_pos[0]) +
                     (delta_pos[1] * delta_pos[1]));

    KRATOS_CATCH("")
}

double CalculateCurrentLength2D2N(const Element& rElement)
{
    KRATOS_TRY;

    const array_1d<double, 3> delta_pos =
        rElement.GetGeometry()[1].GetInitialPosition().Coordinates() -
        rElement.GetGeometry()[0].GetInitialPosition().Coordinates() +
        rElement.GetGeometry()[1].FastGetSolutionStepValue(DISPLACEMENT) -
        rElement.GetGeometry()[0].FastGetSolutionStepValue(DISPLACEMENT);

    const double l = std::sqrt((delta_pos[0] * delta_pos[0]) +
                               (delta_pos[1] * delta_pos[1]));

    KRATOS_ERROR_IF(l <= std::numeric_limits<double>::epsilon())
        << "Element #" << rElement.Id() << " has a current length of zero!" << std::endl;

    return l;

    KRATOS_CATCH("")
}

double CalculateReferenceLength3D2N(const Element& rElement)
{
    KRATOS_TRY;

    const array_1d<double, 3> delta_pos =
        rElement.GetGeometry()[1].GetInitialPosition().Coordinates() -
        rElement.GetGeometry()[0].GetInitialPosition().Coordinates();

    return MathUtils<double>::Norm3(delta_pos);

    KRATOS_CATCH("")
}

double CalculateCurrentLength3D2N(const Element& rElement)
{
    KRATOS_TRY;

    const array_1d<double, 3> delta_pos =
        rElement.GetGeometry()[1].GetInitialPosition().Coordinates() -
        rElement.GetGeometry()[0].GetInitialPosition().Coordinates() +
        rElement.GetGeometry()[1].FastGetSolutionStepValue(DISPLACEMENT) -
        rElement.GetGeometry()[0].FastGetSolutionStepValue(DISPLACEMENT);

    const double l = MathUtils<double>::Norm3(delta_pos);

    KRATOS_ERROR_IF(l <= std::numeric_limits<double>::epsilon())
        << "Element #" << rElement.Id() << " has a current length of zero!" << std::endl;

    return l;

    KRATOS_CATCH("")
}

} // namespace StructuralMechanicsElementUtilities.
}  // namespace Kratos.


