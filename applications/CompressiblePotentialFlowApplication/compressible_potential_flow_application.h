//    |  /           |
//    ' /   __| _` | __|  _ \   __|
//    . \  |   (   | |   (   |\__ `
//   _|\_\_|  \__,_|\__|\___/ ____/
//                   Multi-Physics
//
//  License:		 BSD License
//					 Kratos default license: kratos/license.txt
//
//  Main authors:    Riccardo Rossi
//


#if !defined(KRATOS_COMPRESSIBLE_POTENTIAL_FLOW_APPLICATION_H_INCLUDED )
#define  KRATOS_COMPRESSIBLE_POTENTIAL_FLOW_APPLICATION_H_INCLUDED


// System includes
#include <string>
#include <iostream>


// External includes


// Project includes
#include "includes/define.h"
#include "includes/kratos_application.h"
#include "includes/variables.h"
#include "custom_elements/compressible_potential_flow_element.h"
#include "custom_elements/compressible_potential_flow_element_wake_implementation_01_cutting_no_penalty.h"
#include "custom_elements/compressible_potential_flow_element_wake_implementation_03_no_cutting_no_penalty.h"
#include "custom_elements/compressible_potential_flow_element_wake_implementation_09_no_cutting_no_penalty_half_kutta.h"
#include "custom_elements/compressible_potential_flow_element_wake_implementation_11_no_airfoil.h"
#include "custom_elements/compressible_potential_flow_element_wake_implementation_13_no_te.h"
#include "custom_elements/compressible_potential_flow_element_wake_implementation_15_no_corner.h"
#include "custom_elements/compressible_potential_flow_element_wake_implementation_16_no_corner_te_down.h"
#include "custom_elements/compressible_potential_flow_element_wake_implementation_19_no_corner_zero_velocity.h"
#include "custom_elements/compressible_potential_flow_element_wake_implementation_21_no_corner_variation.h"
#include "custom_conditions/potential_wall_condition.h"

namespace Kratos {

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

/// Short class definition.
/** Detail class definition.
*/
class KratosCompressiblePotentialFlowApplication : public KratosApplication {
public:
	///@name Type Definitions
	///@{


	/// Pointer definition of KratosCompressiblePotentialFlowApplication
	KRATOS_CLASS_POINTER_DEFINITION(KratosCompressiblePotentialFlowApplication);

	///@}
	///@name Life Cycle
	///@{

	/// Default constructor.
	KratosCompressiblePotentialFlowApplication();

	/// Destructor.
	~KratosCompressiblePotentialFlowApplication() override{}


	///@}
	///@name Operators
	///@{


	///@}
	///@name Operations
	///@{

	void Register() override;



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
	std::string Info() const override {
		return "KratosCompressiblePotentialFlowApplication";
	}

	/// Print information about this object.
	void PrintInfo(std::ostream& rOStream) const override {
		rOStream << Info();
		PrintData(rOStream);
	}

	///// Print object's data.
	void PrintData(std::ostream& rOStream) const override {
  		KRATOS_WATCH("in my application");
  		KRATOS_WATCH(KratosComponents<VariableData>::GetComponents().size() );

		rOStream << "Variables:" << std::endl;
		KratosComponents<VariableData>().PrintData(rOStream);
		rOStream << std::endl;
		rOStream << "Elements:" << std::endl;
		KratosComponents<Element>().PrintData(rOStream);
		rOStream << std::endl;
		rOStream << "Conditions:" << std::endl;
		KratosComponents<Condition>().PrintData(rOStream);
    }


	///@}
	///@name Friends
	///@{


	///@}

protected:
	///@name Protected static Member Variables
	///@{


	///@}
	///@name Protected member Variables
	///@{


	///@}
	///@name Protected Operators
	///@{


	///@}
	///@name Protected Operations
	///@{


	///@}
	///@name Protected  Access
	///@{


	///@}
	///@name Protected Inquiry
	///@{


	///@}
	///@name Protected LifeCycle
	///@{


	///@}

private:
	///@name Static Member Variables
	///@{

	// static const ApplicationCondition  msApplicationCondition;

	///@}
	///@name Member Variables
	///@{
        const CompressiblePotentialFlowElement<2,3> mCompressiblePotentialFlowElement2D3N;
        const CompressiblePotentialFlowElement<3,4> mCompressiblePotentialFlowElement3D4N;
        const PotentialWallCondition<2,2> mPotentialWallCondition2D2N;
        const PotentialWallCondition<3,3> mPotentialWallCondition3D3N;

		const CompressiblePotentialFlowElementWakeImplementation01CuttingNoPenalty<2,3>
				mCompressiblePotentialFlowElementWakeImplementation01CuttingNoPenalty2D3N;

		const CompressiblePotentialFlowElementWakeImplementation03NoCuttingNoPenalty<2,3>
				mCompressiblePotentialFlowElementWakeImplementation03NoCuttingNoPenalty2D3N;

		const CompressiblePotentialFlowElementWakeImplementation09NoCuttingNoPenaltyHalfKutta<2,3>
				mCompressiblePotentialFlowElementWakeImplementation09NoCuttingNoPenaltyHalfKutta2D3N;

		const CompressiblePotentialFlowElementWakeImplementation11NoAirfoil<2,3>
				mCompressiblePotentialFlowElementWakeImplementation11NoAirfoil2D3N;

		const CompressiblePotentialFlowElementWakeImplementation13NoTe<2,3>
				mCompressiblePotentialFlowElementWakeImplementation13NoTe2D3N;

		const CompressiblePotentialFlowElementWakeImplementation15NoCorner<2,3>
				mCompressiblePotentialFlowElementWakeImplementation15NoCorner2D3N;

		const CompressiblePotentialFlowElementWakeImplementation16NoCornerTeDown<2,3>
				mCompressiblePotentialFlowElementWakeImplementation16NoCornerTeDown2D3N;

		const CompressiblePotentialFlowElementWakeImplementation19NoCornerZeroVelocity<2,3>
				mCompressiblePotentialFlowElementWakeImplementation19NoCornerZeroVelocity2D3N;

		const CompressiblePotentialFlowElementWakeImplementation21NoCornerVariation<2,3>
				mCompressiblePotentialFlowElementWakeImplementation21NoCornerVariation2D3N;


	///@}
	///@name Private Operators
	///@{


	///@}
	///@name Private Operations
	///@{


	///@}
	///@name Private  Access
	///@{


	///@}
	///@name Private Inquiry
	///@{


	///@}
	///@name Un accessible methods
	///@{

	/// Assignment operator.
	KratosCompressiblePotentialFlowApplication& operator=(KratosCompressiblePotentialFlowApplication const& rOther);

	/// Copy constructor.
	KratosCompressiblePotentialFlowApplication(KratosCompressiblePotentialFlowApplication const& rOther);


	///@}

}; // Class KratosCompressiblePotentialFlowApplication

///@}


///@name Type Definitions
///@{


///@}
///@name Input and output
///@{

///@}


}  // namespace Kratos.

#endif // KRATOS_COMPRESSIBLE_POTENTIAL_FLOW_APPLICATION_H_INCLUDED  defined
