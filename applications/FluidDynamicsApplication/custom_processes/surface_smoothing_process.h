//    |  /           |
//    ' /   __| _` | __|  _ \   __|
//    . \  |   (   | |   (   |\__ `
//   _|\_\_|  \__,_|\__|\___/ ____/
//                   Multi-Physics
//
//  License:         BSD License
//                   Kratos default license: kratos/license.txt
//
//  Main authors:    Mohammad R. Hashemi
//

#ifndef KRATOS_SURFACE_SMOOTHING_H
#define KRATOS_SURFACE_SMOOTHING_H

// System includes
#include <string>
#include <iostream>

// External includes

// Project includes
#include "includes/define.h"
#include "processes/process.h"
#include "containers/model.h"
#include "includes/checks.h"
#include "utilities/parallel_utilities.h"
#include "spaces/ublas_space.h"
#include "linear_solvers/linear_solver.h"
#include "modeler/connectivity_preserve_modeler.h"
#include "solving_strategies/builder_and_solvers/residualbased_block_builder_and_solver.h"
#include "solving_strategies/schemes/residualbased_incrementalupdate_static_scheme.h"
#include "solving_strategies/strategies/residualbased_linear_strategy.h"
#include "utilities/variable_utils.h"
#include "utilities/element_size_calculator.h"
#include "includes/global_pointer_variables.h"

// Application includes
#include "fluid_dynamics_application_variables.h"
#include "custom_elements/surface_smoothing_element.h"

namespace Kratos
{
///@addtogroup FluidDynamicsApplication
///@{

///@name Kratos Globals
///@{

///@}
///@name Type Definitions
///@{

///@}
///@name  Enum's
///@{

///@}
///@name  Functions
///@{

///@}
///@name Kratos Classes
///@{

/// Utility for surface smoothing
/// Based on Tornberg, Anna-Karin, and Björn Engquist. "A finite element based level-set method
/// for multiphase flow applications." Computing and Visualization in Science 3, no. 1-2 (2000): 93-101.
/// The algorithm is improved by imposing a boundary condition and a correction step.
class SurfaceSmoothingProcess : public Process
{
public:
    ///@name Type Definitions
    ///@{

    /// Pointer definition of SurfaceSmoothingProcess
    KRATOS_CLASS_POINTER_DEFINITION(SurfaceSmoothingProcess);

    typedef UblasSpace<double, CompressedMatrix, Vector> TSparseSpace;
    typedef UblasSpace<double, Matrix, Vector> TDenseSpace;
    typedef LinearSolver<TSparseSpace, TDenseSpace > TLinearSolver;
    typedef Scheme< TSparseSpace,  TDenseSpace > SchemeType;
    typedef typename SchemeType::Pointer SchemePointerType;
    typedef typename BuilderAndSolver<TSparseSpace,TDenseSpace,TLinearSolver>::Pointer BuilderSolverPointerType;
    typedef SolvingStrategy< TSparseSpace, TDenseSpace, TLinearSolver > SolvingStrategyType;

    ///@}
    ///@name Life Cycle
    ///@{

    /// Constructor.
    SurfaceSmoothingProcess(
        ModelPart& rModelPart,
        TLinearSolver::Pointer);

    /// Destructor.
    ~SurfaceSmoothingProcess() override
    {
        Model& current_model = mrModelPart.GetModel();
        if(current_model.HasModelPart( mAuxModelPartName ))
            current_model.DeleteModelPart( mAuxModelPartName );
    }

    ///@}
    ///@name Operators
    ///@{

    void Execute() override;

    void ExecuteInitialize() override;

    void ExecuteBeforeSolutionLoop() override;

    void ExecuteInitializeSolutionStep() override;

    void ExecuteFinalizeSolutionStep() override;

    virtual void Clear()
    {
        Model& r_model = mrModelPart.GetModel();
        ModelPart& r_smoothing_model_part = r_model.GetModelPart( mAuxModelPartName );
        r_smoothing_model_part.Nodes().clear();
        r_smoothing_model_part.Conditions().clear();
        r_smoothing_model_part.Elements().clear();
        mp_solving_strategy->Clear();
    }

    ///@}
    ///@name Operations
    ///@{

    ///@}
    ///@name Access
    ///@{

    ///@}
    ///@name Inquiry
    ///@{

    ///@}
    ///@name Input and output
    ///@{

    /// Turn back information as a string.
    std::string Info() const override
    {
        std::stringstream buffer;
        buffer << "Surface Smoothing Process" ;
        return buffer.str();
    }

    /// Print information about this object.
    void PrintInfo(std::ostream& rOStream) const override {rOStream << "Surface Smoothing Process";}

    /// Print object's data.
    void PrintData(std::ostream& rOStream) const override {}

    ///@}
    ///@name Friends
    ///@{

    ///@}

private:
    ///@name Static Member Variables
    ///@{

    ///@}
    ///@name Member Variables
    ///@{

    ModelPart& mrModelPart;

    std::string mAuxModelPartName = "Aux_Smoothing_Model_Part";

    SolvingStrategyType::UniquePointer mp_solving_strategy;

    ///@}
    ///@name Protected Operators
    ///@{

    ///@}
    ///@name Private Operations
    ///@{

    void InitializeSolutionStrategy(
        TLinearSolver::Pointer pLinearSolver,
        BuilderSolverPointerType pBuilderAndSolver)
    {
        // Generate a linear solver strategy
        auto p_scheme = Kratos::make_shared< ResidualBasedIncrementalUpdateStaticScheme< TSparseSpace,TDenseSpace > >();

        Model& r_model = mrModelPart.GetModel();
        ModelPart& r_smoothing_model_part = r_model.GetModelPart( mAuxModelPartName );

        bool CalculateReactions = false;
        bool ReformDofAtEachIteration = false;
        bool CalculateNormDxFlag = false;

        mp_solving_strategy = Kratos::make_unique<ResidualBasedLinearStrategy<TSparseSpace, TDenseSpace, TLinearSolver> >(
            r_smoothing_model_part,
            p_scheme,
            pLinearSolver,
            pBuilderAndSolver,
            CalculateReactions,
            ReformDofAtEachIteration,
            CalculateNormDxFlag);

        mp_solving_strategy->Check();
    }

    /**
     * @brief Initialize the process
     * Create a Model Part with the SurfaceSmoothingElement
     */
    void CreateAuxModelPart();

    ///@}
    ///@name Private  Access
    ///@{


    ///@}
    ///@name Private Inquiry
    ///@{


    ///@}
    ///@name Un accessible methods
    ///@{

    /// Default constructor.
    SurfaceSmoothingProcess() = delete;

    /// Assignment operator.
    SurfaceSmoothingProcess& operator=(SurfaceSmoothingProcess const& rOther) = delete;

    /// Copy constructor.
    SurfaceSmoothingProcess(SurfaceSmoothingProcess const& rOther) = delete;

    ///@}

}; // Class SurfaceSmoothingProcess

///@}
///@name Type Definitions
///@{

///@}
///@name Input and output
///@{

///@}

///@} addtogroup block

};  // namespace Kratos.

#endif // KRATOS_SURFACE_SMOOTHING__H