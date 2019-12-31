//    |  /           |
//    ' /   __| _` | __|  _ \   __|
//    . \  |   (   | |   (   |\__ `
//   _|\_\_|  \__,_|\__|\___/ ____/
//                   Multi-Physics
//
//  License:		 BSD License
//					 Kratos default license: kratos/license.txt
//
//  Main authors:    Dharmin Shah (https://github.com/sdharmin)
//                   Bence Rochlitz (https://github.com/bencerochlitz)
//
//  Supervised by:   Jordi Cotela (https://github.com/jcotela)
//                   Suneth Warnakulasuriya (https://github.com/sunethwarna)




// System includes
#include <cmath>

// External includes

// Project includes
#include "includes/define.h"

// Application includes
#include "custom_utilities/rans_check_utilities.h"
#include "rans_application_variables.h"

// Include base h
#include "rans_omega_turbulent_mixing_inlet_process.h"
namespace Kratos
{

/// Constructor
RansOmegaTurbulentMixingLengthInletProcess::RansOmegaTurbulentMixingLengthInletProcess(Model& rModel, Parameters& rParameters)
    : mrModel(rModel), mrParameters(rParameters)
{
    KRATOS_TRY

    Parameters default_parameters = Parameters(R"(
    {
        "model_part_name"         : "PLEASE_SPECIFY_MODEL_PART_NAME",
        "turbulent_mixing_length" : 0.005,
        "c_mu"                    : 0.09,
        "echo_level"              : 0,
        "is_fixed"                : true
    })");

    mrParameters.RecursivelyValidateAndAssignDefaults(default_parameters);

    mTurbulentMixingLength = mrParameters["turbulent_mixing_length"].GetDouble();
    mIsConstrained = mrParameters["is_fixed"].GetBool();
    mEchoLevel = mrParameters["echo_level"].GetInt();
    mModelPartName = mrParameters["model_part_name"].GetString();
    mCmu_75 = std::pow(mrParameters["c_mu"].GetDouble(), 0.75);

    KRATOS_ERROR_IF(mTurbulentMixingLength < std::numeric_limits<double>::epsilon())
        << "turbulent_mixing_length should be greater than zero.\n";

    KRATOS_CATCH("");
}
/// Destructor.
RansOmegaTurbulentMixingLengthInletProcess::~RansOmegaTurbulentMixingLengthInletProcess()
{
}


void RansOmegaTurbulentMixingLengthInletProcess::ExecuteInitialize()
{
    if (mIsConstrained)
    {
        ModelPart& r_model_part = mrModel.GetModelPart(mModelPartName);

        const int number_of_nodes = r_model_part.NumberOfNodes();
#pragma omp parallel for
        for (int i_node = 0; i_node < number_of_nodes; ++i_node)
        {
            NodeType& r_node = *(r_model_part.NodesBegin() + i_node);
            r_node.Fix(TURBULENT_SPECIFIC_ENERGY_DISSIPATION_RATE);
        }

        KRATOS_INFO_IF(this->Info(), mEchoLevel > 0)
            << "Fixed TURBULENT_SPECIFIC_ENERGY_DISSIPATION_RATE dofs in "
            << mModelPartName << ".\n";
    }
}

void RansOmegaTurbulentMixingLengthInletProcess::ExecuteInitializeSolutionStep()
{
    Execute();
}

void RansOmegaTurbulentMixingLengthInletProcess::Execute()
{
    KRATOS_TRY

    ModelPart::NodesContainerType& r_nodes =
        mrModel.GetModelPart(mModelPartName).Nodes();
    const int number_of_nodes = r_nodes.size();

#pragma omp parallel for
    for (int i_node = 0; i_node < number_of_nodes; ++i_node)
    {
        NodeType& r_node = *(r_nodes.begin() + i_node);
        CalculateTurbulentValues(r_node);
    }

    KRATOS_INFO_IF(this->Info(), mEchoLevel > 0)
        << "Applied omega values to " << mModelPartName << ".\n";

    KRATOS_CATCH("");
}

int RansOmegaTurbulentMixingLengthInletProcess::Check()
{
    RansCheckUtilities::CheckIfModelPartExists(mrModel, mModelPartName);

    ModelPart& r_model_part = mrModel.GetModelPart(mModelPartName);

    RansCheckUtilities::CheckIfVariableExistsInModelPart(r_model_part, TURBULENT_KINETIC_ENERGY);
    RansCheckUtilities::CheckIfVariableExistsInModelPart(
        r_model_part, TURBULENT_SPECIFIC_ENERGY_DISSIPATION_RATE);

    return 0;
}


/// Turn back information as a string.
std::string RansOmegaTurbulentMixingLengthInletProcess::Info() const
{
    return std::string("RansOmegaTurbulentMixingLengthInletProcess");
}

/// Print information about this object.
void RansOmegaTurbulentMixingLengthInletProcess::PrintInfo(std::ostream& rOStream) const
{
    rOStream << this->Info();
}

/// Print object's data.
void RansOmegaTurbulentMixingLengthInletProcess::PrintData(std::ostream& rOStream) const
{
}

void RansOmegaTurbulentMixingLengthInletProcess::CalculateTurbulentValues(NodeType& rNode)
{
    const double tke = rNode.FastGetSolutionStepValue(TURBULENT_KINETIC_ENERGY);
    rNode.FastGetSolutionStepValue(TURBULENT_SPECIFIC_ENERGY_DISSIPATION_RATE) =
        std::sqrt(std::max(tke, 0.0)) / (mCmu_75 * mTurbulentMixingLength);
}

} // namespace Kratos.
