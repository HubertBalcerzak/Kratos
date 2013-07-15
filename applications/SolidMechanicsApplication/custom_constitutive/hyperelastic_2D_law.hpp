//   
//   Project Name:        KratosSolidMechanicsApplication $      
//   Last modified by:    $Author:            JMCarbonell $ 
//   Date:                $Date:                July 2013 $
//   Revision:            $Revision:                  0.0 $
//
//

#if !defined (KRATOS_HYPERELASTIC_2D_LAW_H_INCLUDED)
#define  KRATOS_HYPERELASTIC_2D_LAW_H_INCLUDED

// System includes 

// External includes 

// Project includes
#include "custom_constitutive/hyperelastic_3D_law.hpp"


namespace Kratos
{
  /**
   * Defines a hyperelastic isotropic constitutive law in 2D Neohookean Model (Plane Strain)
   * This material law is defined by the parameters:
   * 1) YOUNG MODULUS 
   * 2) POISSON RATIO
   * As there are no further parameters the functionality is limited 
   * to large displacements elasticity.
   */

  class HyperElastic2DLaw : public HyperElastic3DLaw
  {
  public:
    /**
     * Type Definitions
     */
    typedef ProcessInfo      ProcessInfoType;
    typedef ConstitutiveLaw         BaseType;
    typedef std::size_t             SizeType;
    /**
     * Counted pointer of HyperElastic2DLaw
     */
    
    KRATOS_CLASS_POINTER_DEFINITION(HyperElastic2DLaw);
    
    /**
     * Life Cycle 
     */

    /**
     * Default constructor.
     */
    HyperElastic2DLaw();
			
    /**
     * Clone function (has to be implemented by any derived class)
     * @return a pointer to a new instance of this constitutive law
     */
    ConstitutiveLaw::Pointer Clone() const;
    
    /**
     * Copy constructor.
     */
    HyperElastic2DLaw (const HyperElastic2DLaw& rOther);
   

    /**
     * Assignment operator.
     */

    //HyperElastic2DLaw& operator=(const HyperElastic2DLaw& rOther);


    /**
     * Destructor.
     */
    virtual ~HyperElastic2DLaw();
			
    /**
     * Operators 
     */
    
     /**
     * Input and output
     */
    /**
     * Turn back information as a string.
     */
    //virtual String Info() const;
    /**
     * Print information about this object.
     */
    //virtual void PrintInfo(std::ostream& rOStream) const;
    /**
     * Print object's data.
     */
    //virtual void PrintData(std::ostream& rOStream) const;
		
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


    /**
     * Calculates the GreenLagrange strains
     * @param rRightCauchyGreen
     * @param rStrainVector
     */
    virtual void CalculateGreenLagrangeStrain( const Matrix & rRightCauchyGreen,
					       Vector& rStrainVector );


    /**
     * Calculates the Almansi strains
     * @param rRightCauchyGreen
     * @param rStrainVector
     */
    virtual void CalculateAlmansiStrain( const Matrix & rLeftCauchyGreen,
					 Vector& rStrainVector );


    /**
     * Calculates the constitutive matrix 
     * @param rMatrixIC can be the Identity or the RightCauchyGreen tensor
     * @param rdetF the determinant of the deformation gradient
     * @param rLameLambda lame paramenter lambda
     * @param rLameMu lame paramenter mu
     * matrix is to be generated for
     * @param rResult Matrix the result (Constitutive Matrix) will be stored in
     */
    void CalculateConstitutiveMatrix (const Matrix & rMatrixIC,
				      const double &rdetF,
				      const double &rLameLambda,
				      const double &rLameMu,
				      Matrix& rConstitutiveMatrix);


    /**
     * Calculates the constitutive matrix and makes a pull-back
     * @param rMatrixIC can be the Identity or the RightCauchyGreen tensor
     * @param rinvF the invers of the current deformation gradient
     * @param rdetF0 the determinant of the total deformation gradient
     * @param rLameLambda lame paramenter lambda
     * @param rLameMu lame paramenter mu
     * matrix is to be generated for
     * @param rConstitutiveMatrix matrix where the constitutive tensor is stored
     */
    void CalculateConstitutiveMatrix (const Matrix &rMatrixIC,
				      const Matrix &rinvF, 
				      const double &rdetF0,
				      const double &rLameLambda,
				      const double &rLameMu,
				      Matrix& rConstitutiveMatrix);

 
		
  private:

    ///@name Static Member Variables
    ///@{
    ///@}
    ///@name Member Variables
    ///@{


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

 
    ///@}
    ///@name Serialization
    ///@{
    friend class Serializer;

    virtual void save(Serializer& rSerializer) const
    {
      KRATOS_SERIALIZE_SAVE_BASE_CLASS(rSerializer, HyperElastic3DLaw);
    }

    virtual void load(Serializer& rSerializer)
    {
      KRATOS_SERIALIZE_LOAD_BASE_CLASS(rSerializer, HyperElastic3DLaw);
    }



  }; // Class HyperElastic2DLaw 
}  // namespace Kratos.
#endif // KRATOS_HYPERELASTIC_2D_LAW_H_INCLUDED  defined 
