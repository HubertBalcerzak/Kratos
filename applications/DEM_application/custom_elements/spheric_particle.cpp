//
//   Project Name:        Kratos
//   Last Modified by:    $Author: M.Santasusana $
//   Date:                $Date: 2006-11-27 16:07:33 $
//   Revision:            $Revision: 1.1.1.1 $
//
//

// System includes
#include <string>
#include <iostream>
#include <cmath>

// External includes

// Project includes
#include "includes/define.h"
#include "spheric_particle.h"
#include "custom_utilities/GeometryFunctions.h"
#include "custom_utilities/AuxiliaryFunctions.h"
#include "DEM_application.h"
#include "includes/kratos_flags.h"

namespace Kratos
{
// using namespace GeometryFunctions;

SphericParticle::SphericParticle()
    : DiscreteElement(), mRealMass(0)
{
    mRadius = 0;
    mRealMass = 0;
}

SphericParticle::SphericParticle(IndexType NewId, GeometryType::Pointer pGeometry)
    : DiscreteElement(NewId, pGeometry), mRealMass(0){
    mRadius = 0;
    mRealMass = 0;
}

SphericParticle::SphericParticle(IndexType NewId, GeometryType::Pointer pGeometry,  PropertiesType::Pointer pProperties)
    : DiscreteElement(NewId, pGeometry, pProperties), mRealMass(0)
{
    mRadius = 0;
    mRealMass = 0;
}

SphericParticle::SphericParticle(IndexType NewId, NodesArrayType const& ThisNodes)
    : DiscreteElement(NewId, ThisNodes), mRealMass(0)
{
    mRadius = 0;
    mRealMass = 0;
}

Element::Pointer SphericParticle::Create(IndexType NewId, NodesArrayType const& ThisNodes, PropertiesType::Pointer pProperties) const
{
    return Element::Pointer(new SphericParticle(NewId, GetGeometry().Create(ThisNodes), pProperties));
}

/// Destructor.
SphericParticle::~SphericParticle(){}

//**************************************************************************************************************************************************
//**************************************************************************************************************************************************

void SphericParticle::Initialize()
{
    KRATOS_TRY

    mDimension                = 3;
    mRadius                   = GetGeometry()[0].FastGetSolutionStepValue(RADIUS);
    double density            = GetDensity();
    double& mass              = GetGeometry()[0].FastGetSolutionStepValue(NODAL_MASS);
    mass                      = 4 *KRATOS_M_PI_3 * density * mRadius * mRadius * mRadius;
    mRealMass                 = mass;
    
    if (this->IsNot(BLOCKED)) GetGeometry()[0].FastGetSolutionStepValue(PARTICLE_MATERIAL) = GetParticleMaterial();
    
    mClusterId = -1;

    if (this->Is(DEMFlags::HAS_ROTATION) ){
        double moment_of_inertia = 0.4 * mass * mRadius * mRadius;
        GetGeometry()[0].FastGetSolutionStepValue(PARTICLE_MOMENT_OF_INERTIA) = moment_of_inertia;
    }

    else {
        array_1d<double, 3>& angular_velocity = GetGeometry()[0].FastGetSolutionStepValue(ANGULAR_VELOCITY);
        angular_velocity = ZeroVector(3);
    }

    if (GetGeometry()[0].GetDof(VELOCITY_X).IsFixed())         GetGeometry()[0].Set(DEMFlags::FIXED_VEL_X,true);
    else                                                          GetGeometry()[0].Set(DEMFlags::FIXED_VEL_X,false);
    if (GetGeometry()[0].GetDof(VELOCITY_Y).IsFixed())         GetGeometry()[0].Set(DEMFlags::FIXED_VEL_Y,true);
    else                                                          GetGeometry()[0].Set(DEMFlags::FIXED_VEL_Y,false);
    if (GetGeometry()[0].GetDof(VELOCITY_Z).IsFixed())         GetGeometry()[0].Set(DEMFlags::FIXED_VEL_Z,true);
    else                                                          GetGeometry()[0].Set(DEMFlags::FIXED_VEL_Z,false);
    if (GetGeometry()[0].GetDof(ANGULAR_VELOCITY_X).IsFixed()) GetGeometry()[0].Set(DEMFlags::FIXED_ANG_VEL_X,true);
    else                                                          GetGeometry()[0].Set(DEMFlags::FIXED_ANG_VEL_X,false);
    if (GetGeometry()[0].GetDof(ANGULAR_VELOCITY_Y).IsFixed()) GetGeometry()[0].Set(DEMFlags::FIXED_ANG_VEL_Y,true);
    else                                                          GetGeometry()[0].Set(DEMFlags::FIXED_ANG_VEL_Y,false);
    if (GetGeometry()[0].GetDof(ANGULAR_VELOCITY_Z).IsFixed()) GetGeometry()[0].Set(DEMFlags::FIXED_ANG_VEL_Z,true);
    else                                                          GetGeometry()[0].Set(DEMFlags::FIXED_ANG_VEL_Z,false);

    CustomInitialize();

    KRATOS_CATCH( "" )
}

void SphericParticle::FullInitialize(const ProcessInfo& r_process_info)
{
    KRATOS_TRY
    MemberDeclarationFirstStep(r_process_info);
    Initialize();   
    CreateDiscontinuumConstitutiveLaws(r_process_info);
    KRATOS_CATCH( "" )
}

//**************************************************************************************************************************************************
//**************************************************************************************************************************************************

void SphericParticle::CalculateRightHandSide(VectorType& r_right_hand_side_vector, ProcessInfo& r_current_process_info,
                                             double dt, const array_1d<double,3>& gravity, int search_control)
{
    KRATOS_TRY           

    array_1d<double, 3> additional_forces;
    array_1d<double, 3> additionally_applied_moment;
    array_1d<double, 3> initial_rotation_moment;
    array_1d<double, 3>& elastic_force       = this->GetGeometry()[0].FastGetSolutionStepValue(ELASTIC_FORCES);
    array_1d<double, 3>& contact_force       = this->GetGeometry()[0].FastGetSolutionStepValue(CONTACT_FORCES);
    array_1d<double, 3>& rigid_element_force = this->GetGeometry()[0].FastGetSolutionStepValue(RIGID_ELEMENT_FORCE);

    mContactForce.clear();
    mContactMoment.clear();
    additional_forces.clear();
    additionally_applied_moment.clear();
    initial_rotation_moment.clear();
    elastic_force.clear();
    contact_force.clear();
    rigid_element_force.clear();

    bool multi_stage_RHS = false;

    ComputeBallToBallContactForce(elastic_force, contact_force, initial_rotation_moment, r_current_process_info, dt, multi_stage_RHS);

    if (mFemOldNeighbourIds.size() > 0){  //MSI: needed??
        ComputeBallToRigidFaceContactForce(elastic_force, contact_force, initial_rotation_moment, rigid_element_force, r_current_process_info, dt, search_control);
    }
    
    if (this->IsNot(DEMFlags::BELONGS_TO_A_CLUSTER)){
        ComputeAdditionalForces(additional_forces, additionally_applied_moment, r_current_process_info, gravity);
    }

    r_right_hand_side_vector[0] =  mContactForce[0] + additional_forces[0];
    r_right_hand_side_vector[1] =  mContactForce[1] + additional_forces[1];
    r_right_hand_side_vector[2] =  mContactForce[2] + additional_forces[2];
    r_right_hand_side_vector[3] = mContactMoment[0] + additionally_applied_moment[0];
    r_right_hand_side_vector[4] = mContactMoment[1] + additionally_applied_moment[1];
    r_right_hand_side_vector[5] = mContactMoment[2] + additionally_applied_moment[2];

    array_1d<double,3>& total_forces = this->GetGeometry()[0].FastGetSolutionStepValue(TOTAL_FORCES);
    array_1d<double,3>& total_moment = this->GetGeometry()[0].FastGetSolutionStepValue(PARTICLE_MOMENT);

    for (int i = 0; i < 3; i++){
        total_forces[i] = r_right_hand_side_vector[i];
        total_moment[i] = r_right_hand_side_vector[3 + i];
    }

    KRATOS_CATCH( "" )
}

//**************************************************************************************************************************************************
//**************************************************************************************************************************************************

void SphericParticle::FirstCalculateRightHandSide(ProcessInfo& r_current_process_info, double dt, int search_control)
{
    KRATOS_TRY

    array_1d<double, 3> initial_rotation_moment;
    array_1d<double, 3>& elastic_force       = this->GetGeometry()[0].FastGetSolutionStepValue(ELASTIC_FORCES);
    array_1d<double, 3>& contact_force       = this->GetGeometry()[0].FastGetSolutionStepValue(CONTACT_FORCES);
    array_1d<double, 3>& rigid_element_force = this->GetGeometry()[0].FastGetSolutionStepValue(RIGID_ELEMENT_FORCE);

    mContactForce.clear();
    mContactMoment.clear();
    initial_rotation_moment.clear();
    elastic_force.clear();
    contact_force.clear();
    rigid_element_force.clear();

    ComputeBallToBallContactForce(elastic_force, contact_force, initial_rotation_moment, r_current_process_info, dt, true); // The preset argument 'true' should be removed

    std::vector<double>& neighbour_rigid_faces_elastic_contact_force = this->mNeighbourRigidFacesElasticContactForce;
    std::vector<double>& neighbour_rigid_faces_total_contact_force = this->mNeighbourRigidFacesTotalContactForce;
    std::fill(neighbour_rigid_faces_elastic_contact_force.begin(), neighbour_rigid_faces_elastic_contact_force.end(), 0.0);
    std::fill(neighbour_rigid_faces_total_contact_force.begin(), neighbour_rigid_faces_total_contact_force.end(), 0.0);

    if (mFemOldNeighbourIds.size() > 0){
        ComputeBallToRigidFaceContactForce(elastic_force, contact_force, initial_rotation_moment, rigid_element_force, r_current_process_info, dt, search_control);
    }

    KRATOS_CATCH( "" )
}

//**************************************************************************************************************************************************
//**************************************************************************************************************************************************

void SphericParticle::CollectCalculateRightHandSide(ProcessInfo& r_current_process_info)
{
    KRATOS_TRY

    for (unsigned int i = 0; i < mNeighbourElements.size(); i++){
        SphericParticle* ineighbour = mNeighbourElements[i];

        if (this->Is(NEW_ENTITY) && ineighbour->Is(NEW_ENTITY)) continue;
        if (this->Id() < ineighbour->Id())                      continue;

        for (unsigned int j = 0; j < ineighbour->mNeighbourElements.size(); j++){  //loop to find the neighbour of the neighbours which is me
            SphericParticle* is_that_me = ineighbour->mNeighbourElements[j];

            if (is_that_me->Id() == this->Id()){
                noalias(mNeighbourElasticContactForces[i]) = - ineighbour->mNeighbourElasticContactForces[j];
                noalias(mNeighbourTotalContactForces[i])  = - ineighbour->mNeighbourTotalContactForces[j];
                noalias(mContactForce) += mNeighbourTotalContactForces[i];
                array_1d<double, 3>& r_elastic_force = this->GetGeometry()[0].FastGetSolutionStepValue(ELASTIC_FORCES);
                noalias(r_elastic_force) += mNeighbourElasticContactForces[i];
                array_1d<double, 3>& r_contact_force = this->GetGeometry()[0].FastGetSolutionStepValue(CONTACT_FORCES);
                noalias(r_contact_force) += mNeighbourTotalContactForces[i];
                break;
            }
        }
    }

    KRATOS_CATCH( "" )
}

//**************************************************************************************************************************************************
//**************************************************************************************************************************************************

void SphericParticle::FinalCalculateRightHandSide(ProcessInfo& r_current_process_info, double dt, const array_1d<double,3>& gravity)
{
    KRATOS_TRY

    /*if (this->Is(DEMFlags::HAS_ROTATION)){
        const array_1d<double, 3> ang_vel = this->GetGeometry()[0].FastGetSolutionStepValue(ANGULAR_VELOCITY);
        const double coeff_acc            = this->GetGeometry()[0].FastGetSolutionStepValue(PARTICLE_MOMENT_OF_INERTIA) / dt;
        array_1d<double, 3> initial_rotation_moment;
        noalias(initial_rotation_moment) = coeff_acc * ang_vel; // the moment needed to stop the spin in one time step

        for (unsigned int i = 0; i < mNeighbourElements.size(); i++){
            SphericParticle* ineighbour = mNeighbourElements[i];

            if (this->Is(NEW_ENTITY) && ineighbour->Is(NEW_ENTITY)) continue;

            array_1d<double, 3> other_to_me_vect;
            noalias(other_to_me_vect) = this->GetGeometry()[0].Coordinates() - ineighbour->GetGeometry()[0].Coordinates();
            double inv_distance                  = 1.0 / DEM_MODULUS_3(other_to_me_vect);
            double other_to_me_vect_unitary[3]   = {other_to_me_vect[0] * inv_distance, other_to_me_vect[1] * inv_distance, other_to_me_vect[2] * inv_distance};
            double projection_to_local_axis2     = DEM_INNER_PRODUCT_3(mNeighbourElasticContactForces[i], other_to_me_vect_unitary);

            if (this->Is(DEMFlags::HAS_ROTATION)){
                ComputeMoments(projection_to_local_axis2, mNeighbourElasticContactForces[i], initial_rotation_moment, other_to_me_vect_unitary, ineighbour, indentation);
            }
        }

    } //if( this->Is(DEMFlags::HAS_ROTATION) )

    array_1d<double, 3> additional_forces;
    array_1d<double, 3> additionally_applied_moment;
    additional_forces.clear();
    additionally_applied_moment.clear();

    ComputeAdditionalForces(additional_forces, additionally_applied_moment, r_current_process_info, gravity);

    array_1d<double,3>& total_forces = this->GetGeometry()[0].FastGetSolutionStepValue(TOTAL_FORCES);
    array_1d<double,3>& total_moment = this->GetGeometry()[0].FastGetSolutionStepValue(PARTICLE_MOMENT);

    noalias(total_forces) = mContactForce  + additional_forces;
    noalias(total_moment) = mContactMoment + additionally_applied_moment;
*/
    KRATOS_CATCH( "" )
}

//**************************************************************************************************************************************************
//**************************************************************************************************************************************************

void SphericParticle::CalculateMaxBallToBallIndentation(double& r_current_max_indentation)
{
    r_current_max_indentation = - std::numeric_limits<double>::max();

    for (unsigned int i = 0; i < mNeighbourElements.size(); i++){
        SphericParticle* ineighbour = mNeighbourElements[i];

        array_1d<double, 3> other_to_me_vect;
        noalias(other_to_me_vect) = this->GetGeometry()[0].Coordinates() - ineighbour->GetGeometry()[0].Coordinates();
        double other_radius                  = ineighbour->GetRadius();
        double distance                      = DEM_MODULUS_3(other_to_me_vect);
        double radius_sum                    = mRadius + other_radius;
        double indentation                   = radius_sum - distance;

        r_current_max_indentation = (indentation > r_current_max_indentation) ? indentation : r_current_max_indentation;
    }
}

//**************************************************************************************************************************************************
//**************************************************************************************************************************************************

void SphericParticle::CalculateMaxBallToFaceIndentation(double& r_current_max_indentation)
{
    r_current_max_indentation = - std::numeric_limits<double>::max();

    std::vector<double>& RF_Pram = this->mNeighbourRigidFacesPram;

    for (unsigned int i = 0; i < mNeighbourRigidFaces.size(); i++){
        
        int ino1                = i * 16;
        double DistPToB         = RF_Pram[ino1 +  9];
        int ContactType         = RF_Pram[ino1 + 15];
        
        if(ContactType > 0){
            double indentation = mRadius - DistPToB;
            r_current_max_indentation = (indentation > r_current_max_indentation) ? indentation : r_current_max_indentation;    
            
        }
                
    } //for every rigidface neighbor
}

//**************************************************************************************************************************************************
//**************************************************************************************************************************************************

void SphericParticle::CalculateKineticEnergy(double& r_kinetic_energy)
{
    const array_1d<double, 3>& vel    = this->GetGeometry()[0].FastGetSolutionStepValue(VELOCITY);
    const array_1d<double, 3> ang_vel = this->GetGeometry()[0].FastGetSolutionStepValue(ANGULAR_VELOCITY);
    const double moment_of_inertia    = this->GetGeometry()[0].FastGetSolutionStepValue(PARTICLE_MOMENT_OF_INERTIA);
    double square_of_celerity         = vel[0] * vel[0] + vel[1] * vel[1] + vel[2] * vel[2];
    double square_of_angular_celerity = ang_vel[0] * ang_vel[0] + ang_vel[1] * ang_vel[1] + ang_vel[2] * ang_vel[2];

    r_kinetic_energy = 0.5 * (mRealMass * square_of_celerity + moment_of_inertia * square_of_angular_celerity);
}

//**************************************************************************************************************************************************
//**************************************************************************************************************************************************

void SphericParticle::CalculateElasticEnergyOfContacts(double& r_elastic_energy) // Calculates the elastic energy stored in the sum of all the contacts shared by the particle and all its neighbours
{
    double added_potential_energy_of_contacts   = 0.0;
    size_t i_neighbour_count                    = 0;
    double myYoung = GetYoung();
    double myPoisson = GetPoisson();

    std::vector<int> mTempNeighboursIds;
    std::vector<array_1d<double, 3> > mTempNeighbourElasticContactForces;
    std::vector<array_1d<double, 3> > mTempNeighbourTotalContactForces;
    ComputeNewNeighboursHistoricalData(mTempNeighboursIds, mTempNeighbourElasticContactForces, mTempNeighbourTotalContactForces);

    for (unsigned int i = 0; i < mNeighbourElements.size(); i++) {
        SphericParticle* ineighbour = mNeighbourElements[i];

        const double &other_radius          = ineighbour->GetRadius();
        double radius_sum                   = mRadius + other_radius;
        double radius_sum_i                 = 1.0 / radius_sum;
        double equiv_radius                 = 2.0 * mRadius * other_radius * radius_sum_i;
        double equiv_area                   = 0.25 * KRATOS_M_PI * equiv_radius * equiv_radius; // 0.25 is because we take only the half of the equivalent radius, corresponding to the case of one ball with radius Requivalent and other = radius 0.
        double equiv_young;
        double equiv_poisson;
        double kn;
        double kt;

        const double other_young            = ineighbour->GetYoung();
        const double other_poisson          = ineighbour->GetPoisson();

        equiv_young                         = 2.0 * myYoung * other_young / (myYoung + other_young);
        
        if((myPoisson + other_poisson)!= 0.0) {
            equiv_poisson                     = 2.0 * myPoisson * other_poisson / (myPoisson + other_poisson);
        } else {
            equiv_poisson = 0.0;
        }

        kn                                  = equiv_young * equiv_area * radius_sum_i; //KRATOS_M_PI * 0.5 * equiv_young * equiv_radius; //M: CANET FORMULA
        kt                                  = kn / (2.0 + equiv_poisson + equiv_poisson);

        // Normal contribution

        double aux_power_of_contact_i_normal_force;

        switch (mElasticityType){ //  0 ---linear compression & tension ; 1 --- Hertzian (non-linear compression, linear tension)
            case 0:
                aux_power_of_contact_i_normal_force = mNeighbourElasticContactForces[i_neighbour_count][2] * mNeighbourElasticContactForces[i_neighbour_count][2];
                added_potential_energy_of_contacts  += 0.5 * aux_power_of_contact_i_normal_force / kn;
                break;

            case 1:
                aux_power_of_contact_i_normal_force = pow(fabs(mNeighbourElasticContactForces[i_neighbour_count][2]), 5 / 3); //error: substitute divisions by known result!!!
                added_potential_energy_of_contacts  += 0.4 * aux_power_of_contact_i_normal_force / pow(kn, 2 / 3); //error: substitute divisions by known result!!!
                break;

            default:
                aux_power_of_contact_i_normal_force = mNeighbourElasticContactForces[i_neighbour_count][2] * mNeighbourElasticContactForces[i_neighbour_count][2];
                added_potential_energy_of_contacts  += 0.5 * aux_power_of_contact_i_normal_force / kn;
                break;
        }

        // Tangential Contribution

        double aux_power_of_contact_i_tang_force = mNeighbourElasticContactForces[i_neighbour_count][0] * mNeighbourElasticContactForces[i_neighbour_count][0] + mNeighbourElasticContactForces[i_neighbour_count][1] * mNeighbourElasticContactForces[i_neighbour_count][1];
        added_potential_energy_of_contacts       += 0.5 * aux_power_of_contact_i_tang_force / kt;

        i_neighbour_count ++;
    }

    r_elastic_energy = added_potential_energy_of_contacts;
}

//**************************************************************************************************************************************************
//**************************************************************************************************************************************************

void SphericParticle::CalculateMomentum(array_1d<double, 3>& r_momentum)
{
    const array_1d<double, 3>& vel = this->GetGeometry()[0].FastGetSolutionStepValue(VELOCITY);
    noalias(r_momentum) = mRealMass * vel;
}

//**************************************************************************************************************************************************
//**************************************************************************************************************************************************

void SphericParticle::CalculateLocalAngularMomentum(array_1d<double, 3>& r_angular_momentum)
{
    const array_1d<double, 3> ang_vel  = this->GetGeometry()[0].FastGetSolutionStepValue(ANGULAR_VELOCITY);
    const double moment_of_inertia     = this->GetGeometry()[0].FastGetSolutionStepValue(PARTICLE_MOMENT_OF_INERTIA);
    noalias(r_angular_momentum) = moment_of_inertia * ang_vel;
}

//**************************************************************************************************************************************************
//**************************************************************************************************************************************************

void SphericParticle::ComputeNewNeighboursHistoricalData(std::vector<int>& mTempNeighboursIds, std::vector<array_1d<double, 3> >& mTempNeighbourElasticContactForces,
                                                         std::vector<array_1d<double, 3> >& mTempNeighbourTotalContactForces)
{
    unsigned int new_size = mNeighbourElements.size();
    array_1d<double, 3> vector_of_zeros = ZeroVector(3);
    mTempNeighboursIds.resize(new_size);
    mTempNeighbourElasticContactForces.resize(new_size);
    mTempNeighbourTotalContactForces.resize(new_size);

    for (unsigned int i = 0; i < new_size; i++){
        SphericParticle* i_neighbour = mNeighbourElements[i];
        mTempNeighboursIds[i] = static_cast<int>(i_neighbour->Id());
        noalias(mTempNeighbourElasticContactForces[i]) = vector_of_zeros;
        noalias(mTempNeighbourTotalContactForces[i]) = vector_of_zeros;

        for (unsigned int j = 0; j != mOldNeighbourIds.size(); j++){

            if (static_cast<int>(i_neighbour->Id()) == mOldNeighbourIds[j]){
                noalias(mTempNeighbourElasticContactForces[i]) = mNeighbourElasticContactForces[j];
                noalias(mTempNeighbourTotalContactForces[i])   = mNeighbourTotalContactForces[j];
                break;
            }
        }
    }

    mOldNeighbourIds.swap(mTempNeighboursIds);
    mNeighbourElasticContactForces.swap(mTempNeighbourElasticContactForces);
    mNeighbourTotalContactForces.swap(mTempNeighbourTotalContactForces);
}

//**************************************************************************************************************************************************
//**************************************************************************************************************************************************

void SphericParticle::ComputeNewRigidFaceNeighboursHistoricalData()
{
    array_1d<double, 3> vector_of_zeros = ZeroVector(3);
    std::vector<DEMWall*>& rNeighbours = this->mNeighbourRigidFaces;
    unsigned int new_size              = rNeighbours.size();
    std::vector<int> temp_neighbours_ids(new_size); //these two temporal vectors are very small, saving them as a member of the particle loses time (usually they consist on 1 member).
    std::vector<array_1d<double, 3> > temp_neighbours_contact_forces(new_size);

    for (unsigned int i = 0; i<rNeighbours.size(); i++){

        temp_neighbours_ids[i] = static_cast<int>(rNeighbours[i]->Id());
        noalias(temp_neighbours_contact_forces[i]) = vector_of_zeros;

        for (unsigned int j = 0; j != mFemOldNeighbourIds.size(); j++) {

            if (static_cast<int>(rNeighbours[i]->Id()) == mFemOldNeighbourIds[j]) {
                noalias(temp_neighbours_contact_forces[i]) = mFemNeighbourContactForces[j];
                break;
            }
        }
    }

    mFemOldNeighbourIds.swap(temp_neighbours_ids);
    mFemNeighbourContactForces.swap(temp_neighbours_contact_forces);
}

//**************************************************************************************************************************************************
//**************************************************************************************************************************************************

void SphericParticle::EquationIdVector(EquationIdVectorType& rResult, ProcessInfo& r_current_process_info){}

//**************************************************************************************************************************************************
//**************************************************************************************************************************************************

void SphericParticle::CalculateMassMatrix(MatrixType& rMassMatrix, ProcessInfo& r_current_process_info)
{
    rMassMatrix(0,0) = mRealMass;
}

//**************************************************************************************************************************************************
//**************************************************************************************************************************************************

void SphericParticle::EvaluateDeltaDisplacement(double RelDeltDisp[3],
                                                double RelVel[3],
                                                double LocalCoordSystem[3][3],
                                                double OldLocalCoordSystem[3][3],
                                                array_1d<double, 3>& other_to_me_vect,
                                                const array_1d<double, 3>& vel,
                                                const array_1d<double, 3>& delta_displ,
                                                SphericParticle* p_neighbour,
                                                double& distance)
{
    // FORMING LOCAL COORDINATES

    //Notes: Since we will normally inherit the mesh from GiD, we respect the global system X,Y,Z [0],[1],[2]
    //In the local coordinates we will define the normal direction of the contact as the [2] component!!!!!
    //the way the normal direction is defined (other_to_me_vect) compression is positive!!!
    GeometryFunctions::ComputeContactLocalCoordSystem(other_to_me_vect, distance, LocalCoordSystem); //new Local Coordinate System (normalizes other_to_me_vect)

    // FORMING OLD LOCAL COORDINATES
    array_1d<double,3> old_coord_target;
    noalias(old_coord_target) = this->GetGeometry()[0].Coordinates() - delta_displ;
    const array_1d<double, 3 >& other_delta_displ = p_neighbour->GetGeometry()[0].FastGetSolutionStepValue(DELTA_DISPLACEMENT);
    array_1d<double,3> old_coord_neigh;
    noalias(old_coord_neigh) = p_neighbour->GetGeometry()[0].Coordinates() - other_delta_displ;

    array_1d<double,3> old_other_to_me_vect;
    noalias(old_other_to_me_vect) = old_coord_target - old_coord_neigh;

    const double old_distance = DEM_MODULUS_3(old_other_to_me_vect);

    GeometryFunctions::ComputeContactLocalCoordSystem(old_other_to_me_vect, old_distance, OldLocalCoordSystem); //Old Local Coordinate System

    // VELOCITIES AND DISPLACEMENTS
    const array_1d<double, 3 >& other_vel = p_neighbour->GetGeometry()[0].FastGetSolutionStepValue(VELOCITY);

    RelVel[0] = (vel[0] - other_vel[0]);
    RelVel[1] = (vel[1] - other_vel[1]);
    RelVel[2] = (vel[2] - other_vel[2]);

    // DeltDisp in global coordinates
    RelDeltDisp[0] = (delta_displ[0] - other_delta_displ[0]);
    RelDeltDisp[1] = (delta_displ[1] - other_delta_displ[1]);
    RelDeltDisp[2] = (delta_displ[2] - other_delta_displ[2]);
          
    /*//SLS
    double proj = GeometryFunctions::DotProduct(LocalCoordSystem[2], DeltDisp);
    proj = fabs(proj);
    DEM_MULTIPLY_BY_SCALAR_3(LocalCoordSystem[2], proj);
    DEM_ADD_SECOND_TO_FIRST(DeltDisp, LocalCoordSystem[2]);
    //SLS*/
}

//**************************************************************************************************************************************************
//**************************************************************************************************************************************************

void SphericParticle::DisplacementDueToRotation(double DeltDisp[3],
                                                double RelVel[3],
                                                double OldLocalCoordSystem[3][3],
                                                const double& other_radius,
                                                const double& dt,
                                                const array_1d<double, 3>& ang_vel,
                                                SphericParticle* p_neighbour)
{
    double velA[3]      = {0.0};
    double velB[3]      = {0.0};
    double dRotaDisp[3] = {0.0};
    const array_1d<double, 3>& other_ang_vel = p_neighbour->GetGeometry()[0].FastGetSolutionStepValue(ANGULAR_VELOCITY);

    GeometryFunctions::CrossProduct(      ang_vel, OldLocalCoordSystem[2], velA); //it was Local Coordinate system, now we do OLD.
    GeometryFunctions::CrossProduct(other_ang_vel, OldLocalCoordSystem[2], velB);

    dRotaDisp[0] = - velA[0] * mRadius - velB[0] * other_radius;
    dRotaDisp[1] = - velA[1] * mRadius - velB[1] * other_radius;
    dRotaDisp[2] = - velA[2] * mRadius - velB[2] * other_radius;
    
    DEM_MULTIPLY_BY_SCALAR_3(velA, mRadius);
    DEM_MULTIPLY_BY_SCALAR_3(velB, other_radius);
    
    RelVel[0] += velA[0] - velB[0];
    RelVel[1] += velA[1] - velB[1];
    RelVel[2] += velA[2] - velB[2];

    // Contribution of the rotation velocity
    DeltDisp[0] += dRotaDisp[0] * dt;
    DeltDisp[1] += dRotaDisp[1] * dt;
    DeltDisp[2] += dRotaDisp[2] * dt;
}

//**************************************************************************************************************************************************
//**************************************************************************************************************************************************

void SphericParticle::DisplacementDueToRotationMatrix(double DeltDisp[3],
                                                double RelVel[3],
                                                double OldLocalCoordSystem[3][3],
                                                const double& other_radius,
                                                const double& dt,
                                                const array_1d<double, 3>& angular_vel,
                                                SphericParticle* p_neighbour)
{    
        array_1d<double, 3>& neigh_angular_vel = p_neighbour->GetGeometry()[0].FastGetSolutionStepValue(ANGULAR_VELOCITY); 
        array_1d<double, 3> temp_angular_vel = angular_vel;
        array_1d<double, 3> angular_velocity = angular_vel;
        array_1d<double, 3> other_angular_velocity = neigh_angular_vel;
        array_1d<double, 3> temp_neigh_angular_vel = neigh_angular_vel;
        
        DEM_MULTIPLY_BY_SCALAR_3(temp_angular_vel, dt);
        DEM_MULTIPLY_BY_SCALAR_3(temp_neigh_angular_vel, dt);
        
        double angle = DEM_MODULUS_3(temp_angular_vel);
        double angle_2 = DEM_MODULUS_3(temp_neigh_angular_vel);
        
        array_1d<double, 3> e1;
        
        DEM_COPY_SECOND_TO_FIRST_3(e1, OldLocalCoordSystem[2]);
        DEM_MULTIPLY_BY_SCALAR_3(e1, -mRadius)
        
        array_1d<double, 3> new_axes1 = e1;
           
        array_1d<double, 3> e2;
        
        DEM_COPY_SECOND_TO_FIRST_3(e2, OldLocalCoordSystem[2]);
        DEM_MULTIPLY_BY_SCALAR_3(e2, other_radius) 
                    
        array_1d<double, 3> new_axes2 = e2;
        
        if (angle) {
            
            array_1d<double, 3> axis;
            axis[0] = temp_angular_vel[0] / angle;
            axis[1] = temp_angular_vel[1] / angle;
            axis[2] = temp_angular_vel[2] / angle;

            double cang = cos(angle);
            double sang = sin(angle);

            new_axes1[0] = axis[0] * (axis[0] * e1[0] + axis[1] * e1[1] + axis[2] * e1[2]) * (1 - cang) + e1[0] * cang + (-axis[2] * e1[1] + axis[1] * e1[2]) * sang;
            new_axes1[1] = axis[1] * (axis[0] * e1[0] + axis[1] * e1[1] + axis[2] * e1[2]) * (1 - cang) + e1[1] * cang + (axis[2] * e1[0] - axis[0] * e1[2]) * sang;
            new_axes1[2] = axis[2] * (axis[0] * e1[0] + axis[1] * e1[1] + axis[2] * e1[2]) * (1 - cang) + e1[2] * cang + (-axis[1] * e1[0] + axis[0] * e1[1]) * sang;
            
        } //if angle
        
        if (angle_2) {
            
            array_1d<double, 3> axis_2;
            axis_2[0] = temp_neigh_angular_vel[0] / angle_2;
            axis_2[1] = temp_neigh_angular_vel[1] / angle_2;
            axis_2[2] = temp_neigh_angular_vel[2] / angle_2;

            double cang = cos(angle_2);
            double sang = sin(angle_2);

            new_axes2[0] = axis_2[0] * (axis_2[0] * e2[0] + axis_2[1] * e2[1] + axis_2[2] * e2[2]) * (1 - cang) + e2[0] * cang + (-axis_2[2] * e2[1] + axis_2[1] * e2[2]) * sang;
            new_axes2[1] = axis_2[1] * (axis_2[0] * e2[0] + axis_2[1] * e2[1] + axis_2[2] * e2[2]) * (1 - cang) + e2[1] * cang + (axis_2[2] * e2[0] - axis_2[0] * e2[2]) * sang;
            new_axes2[2] = axis_2[2] * (axis_2[0] * e2[0] + axis_2[1] * e2[1] + axis_2[2] * e2[2]) * (1 - cang) + e2[2] * cang + (-axis_2[1] * e2[0] + axis_2[0] * e2[1]) * sang;

        } //if angle_2
        
        //other_radius = p_neighbour->GetRadius();
        
        array_1d<double, 3> other_to_me_vect;
        noalias(other_to_me_vect) = this->GetGeometry()[0].Coordinates() - p_neighbour->GetGeometry()[0].Coordinates();
        double distance            = DEM_MODULUS_3(other_to_me_vect);
        double radius_sum          = mRadius + other_radius;
        double indentation         = radius_sum - distance;
    
        array_1d<double, 3> radial_vector = - other_to_me_vect;
        array_1d<double, 3> other_radial_vector = other_to_me_vect;
        
        GeometryFunctions::normalize(radial_vector);
        GeometryFunctions::normalize(other_radial_vector);
        
        double arm = mRadius - indentation * mRadius / radius_sum; //////////////////////////LINEAR FORMULATION: SHOULD BE COMPUTED BY THE CONSTITUTIVE LAW
        double other_arm = other_radius - indentation * other_radius / radius_sum;
        
        radial_vector *= arm;
        other_radial_vector *= other_arm;
        //
        array_1d<double, 3> vel = ZeroVector(3);
        array_1d<double, 3> other_vel = ZeroVector(3);
        
        GeometryFunctions::CrossProduct(angular_velocity, radial_vector, vel);
        GeometryFunctions::CrossProduct(other_angular_velocity, other_radial_vector, other_vel);
                
        RelVel[0] += vel[0] - other_vel[0];
        RelVel[1] += vel[1] - other_vel[1];
        RelVel[2] += vel[2] - other_vel[2];
        
        // Contribution of the rotation velocity
        DeltDisp[0] += (new_axes1[0] - e1[0] - new_axes2[0] + e2[0]);
        DeltDisp[1] += (new_axes1[1] - e1[1] - new_axes2[1] + e2[1]);
        DeltDisp[2] += (new_axes1[2] - e1[2] - new_axes2[2] + e2[2]);
}

//**************************************************************************************************************************************************
//**************************************************************************************************************************************************

void SphericParticle::ComputeMoments(double NormalLocalElasticContactForce,
                                     array_1d<double, 3>& GlobalElasticContactForce,
                                     array_1d<double, 3>& rInitialRotaMoment,
                                     double LocalCoordSystem2[3],
                                     SphericParticle* p_neighbour,
                                     double indentation,
                                     bool wall)
{
    double MA[3] = {0.0};

    GeometryFunctions::CrossProduct(LocalCoordSystem2, GlobalElasticContactForce, MA);
    
    double arm = mRadius - indentation;
    
    if(!wall) {
        const double other_radius = p_neighbour->GetRadius();
        double radius_sum          = mRadius + other_radius;
        arm = mRadius - indentation * mRadius / radius_sum;
    }
   
    mContactMoment[0] -= MA[0] * arm;
    mContactMoment[1] -= MA[1] * arm;
    mContactMoment[2] -= MA[2] * arm;

    // ROLLING FRICTION
    if (this->Is(DEMFlags::HAS_ROLLING_FRICTION)) {
        
        double rolling_friction_coeff       = GetRollingFriction() * mRadius;
        const double other_rolling_friction = p_neighbour->GetRollingFriction();
        double other_rolling_friction_coeff = other_rolling_friction * p_neighbour->GetRadius();
        double equiv_rolling_friction_coeff = std::min(rolling_friction_coeff, other_rolling_friction_coeff);

        if (equiv_rolling_friction_coeff != 0.0) {
            double MaxRotaMoment[3]       = {rInitialRotaMoment[0] + mContactMoment[0], rInitialRotaMoment[1] + mContactMoment[1], rInitialRotaMoment[2] + mContactMoment[2]};
            double CoordSystemMoment1[3]  = {0.0};
            double CoordSystemMoment2[3]  = {0.0};
            double MR[3]                  = {0.0};

            GeometryFunctions::CrossProduct(LocalCoordSystem2, MaxRotaMoment, CoordSystemMoment1);
            if (DEM_MODULUS_3(CoordSystemMoment1) > 0.0) {
				double det_coor_sys_moment_i_1 = 1.0 / DEM_MODULUS_3(CoordSystemMoment1);
				DEM_MULTIPLY_BY_SCALAR_3(CoordSystemMoment1, det_coor_sys_moment_i_1)
			}
                  
            GeometryFunctions::CrossProduct(MaxRotaMoment, CoordSystemMoment1, CoordSystemMoment2);
            if (DEM_MODULUS_3(CoordSystemMoment2) > 0.0) {
				double det_coor_sys_moment_i_2 = 1.0 / DEM_MODULUS_3(CoordSystemMoment2);
				DEM_MULTIPLY_BY_SCALAR_3(CoordSystemMoment2, det_coor_sys_moment_i_2)
			}
                             
            if (DEM_MODULUS_3(CoordSystemMoment1) > 0.0 && DEM_MODULUS_3(CoordSystemMoment2) > 0.0) GeometryFunctions::CrossProduct(CoordSystemMoment2, CoordSystemMoment1, MR);
            double AbsoluteNormalLocalElasticContactForce = fabs(NormalLocalElasticContactForce);
            DEM_MULTIPLY_BY_SCALAR_3(MR, AbsoluteNormalLocalElasticContactForce)
                  
            double MR_now = DEM_MODULUS_3(MR) * equiv_rolling_friction_coeff;
            double MR_max = DEM_MODULUS_3(MaxRotaMoment);
            
            if (MR_max > MR_now) {
                mContactMoment[0] += MR[0] * equiv_rolling_friction_coeff;
                mContactMoment[1] += MR[1] * equiv_rolling_friction_coeff;
                mContactMoment[2] += MR[2] * equiv_rolling_friction_coeff;
            }

            else {
                mContactMoment = - rInitialRotaMoment;
            }
        } // if (equiv_rolling_friction_coeff != 0.0)
    } // if (this->Is(DEMFlags::HAS_ROLLING_FRICTION) )
    
    
    
}

//**************************************************************************************************************************************************
//**************************************************************************************************************************************************

void SphericParticle::ComputeBallToBallContactForce(array_1d<double, 3>& r_elastic_force,
                                                    array_1d<double, 3>& r_contact_force,
                                                    array_1d<double, 3>& rInitialRotaMoment,
                                                    ProcessInfo& r_current_process_info,
                                                    double dt,
                                                    const bool multi_stage_RHS)
{
    KRATOS_TRY

    // KINEMATICS

    const array_1d<double, 3>& velocity     = this->GetGeometry()[0].FastGetSolutionStepValue(VELOCITY);
    const array_1d<double, 3>& delta_displ  = this->GetGeometry()[0].FastGetSolutionStepValue(DELTA_DISPLACEMENT);
    const array_1d<double, 3>& ang_velocity = this->GetGeometry()[0].FastGetSolutionStepValue(ANGULAR_VELOCITY);

    if (this->Is(DEMFlags::HAS_ROLLING_FRICTION) && !multi_stage_RHS) {
        const double coeff_acc      = this->GetGeometry()[0].FastGetSolutionStepValue(PARTICLE_MOMENT_OF_INERTIA) / dt;
        noalias(rInitialRotaMoment) = coeff_acc * ang_velocity; // the moment needed to stop the spin in one time step
    }

    //double kn, kt, equiv_visco_damp_coeff_normal, equiv_visco_damp_coeff_tangential, equiv_tg_of_fri_ang;
    
    double LocalCoordSystem[3][3]    = {{0.0}, {0.0}, {0.0}};
    double OldLocalCoordSystem[3][3] = {{0.0}, {0.0}, {0.0}};
    double DeltDisp[3]               = {0.0};
    double LocalDeltDisp[3]          = {0.0};
    double RelVel[3]                 = {0.0};    

    //LOOP OVER NEIGHBORS:
    for (unsigned int i = 0; i < mNeighbourElements.size(); i++) {
        SphericParticle* ineighbour = mNeighbourElements[i];

        if (this->Is(NEW_ENTITY) && ineighbour->Is(NEW_ENTITY)) continue;        
        if (multi_stage_RHS  &&  this->Id() > ineighbour->Id()) continue;
        
        // BASIC CALCULATIONS
        array_1d<double, 3> other_to_me_vect;
        noalias(other_to_me_vect)  = this->GetGeometry()[0].Coordinates() - ineighbour->GetGeometry()[0].Coordinates();
        const double& other_radius = ineighbour->GetRadius();
        double distance            = DEM_MODULUS_3(other_to_me_vect);
        double radius_sum          = mRadius + other_radius;
        double indentation         = radius_sum - distance;

        //CalculateEquivalentConstitutiveParameters(other_to_me_vect, other_radius, radius_sum, kn, kt, equiv_visco_damp_coeff_normal, equiv_visco_damp_coeff_tangential, equiv_tg_of_fri_ang, ineighbour);

        DEM_SET_COMPONENTS_TO_ZERO_3(DeltDisp)
        DEM_SET_COMPONENTS_TO_ZERO_3(LocalDeltDisp)
        DEM_SET_COMPONENTS_TO_ZERO_3(RelVel)        
        DEM_SET_COMPONENTS_TO_ZERO_3x3(LocalCoordSystem)
        DEM_SET_COMPONENTS_TO_ZERO_3x3(OldLocalCoordSystem)

        EvaluateDeltaDisplacement(DeltDisp, RelVel, LocalCoordSystem, OldLocalCoordSystem, other_to_me_vect, velocity, delta_displ, ineighbour, distance);

        if (this->Is(DEMFlags::HAS_ROTATION)) {
            DisplacementDueToRotation(DeltDisp, RelVel, OldLocalCoordSystem, other_radius, dt, ang_velocity, ineighbour);
        }

        double LocalContactForce[3]             = {0.0};
        double GlobalContactForce[3]            = {0.0};
        double LocalElasticContactForce[3]      = {0.0};
        double GlobalElasticContactForce[3]     = {0.0};
        double ViscoDampingLocalContactForce[3] = {0.0};

        DEM_COPY_SECOND_TO_FIRST_3(GlobalElasticContactForce, mNeighbourElasticContactForces[i])

        GeometryFunctions::VectorGlobal2Local(OldLocalCoordSystem, GlobalElasticContactForce, LocalElasticContactForce); // Here we recover the old local forces projected in the new coordinates in the way they were in the old ones; Now they will be increased if its the necessary
        GeometryFunctions::VectorGlobal2Local(OldLocalCoordSystem, DeltDisp, LocalDeltDisp);
        

        // TRANSLATION FORCES

        bool sliding = false;

        if (indentation > 0.0) {
            
            mDiscontinuumConstitutiveLaw->InitializeContact(this, ineighbour);   
            
            LocalElasticContactForce[2]  = mDiscontinuumConstitutiveLaw->CalculateNormalForce(indentation, this, ineighbour);   
            LocalElasticContactForce[2] -= mDiscontinuumConstitutiveLaw->CalculateCohesiveNormalForce(this, ineighbour);
                       
            // TANGENTIAL FORCE. 
            mDiscontinuumConstitutiveLaw->CalculateTangentialForce(LocalElasticContactForce[2], LocalElasticContactForce, LocalDeltDisp, sliding, this, ineighbour);

            // VISCODAMPING (applied locally)
            double LocalRelVel[3]            = {0.0};
            GeometryFunctions::VectorGlobal2Local(OldLocalCoordSystem, RelVel, LocalRelVel);
            mDiscontinuumConstitutiveLaw->CalculateViscoDampingForce(LocalRelVel, ViscoDampingLocalContactForce, sliding, this, ineighbour);                        
        }

        // Transforming to global forces and adding up
        AddUpForcesAndProject(OldLocalCoordSystem, LocalCoordSystem, LocalContactForce, LocalElasticContactForce, GlobalContactForce, GlobalElasticContactForce, ViscoDampingLocalContactForce, r_elastic_force, r_contact_force, i);
        
        // ROTATION FORCES
        if (this->Is(DEMFlags::HAS_ROTATION) && !multi_stage_RHS) {
            ComputeMoments(LocalElasticContactForce[2], mNeighbourTotalContactForces[i], rInitialRotaMoment, LocalCoordSystem[2], ineighbour, indentation);
        }
    }// for each neighbor

    KRATOS_CATCH("")
}// ComputeBallToBallContactForce


void SphericParticle::ComputeRigidFaceToMeVelocity(DEMWall* rObj_2, std::size_t ino,
                                                   double LocalCoordSystem[3][3], double& DistPToB,
                                                   double Weight[4],
                                                   array_1d<double,3>& other_to_me_vel, int& ContactType)
{
    KRATOS_TRY


    std::vector<double>& RF_Pram = this->mNeighbourRigidFacesPram;

    int ino1 = ino * 16;

    LocalCoordSystem[0][0] = RF_Pram[ino1 +  0];
    LocalCoordSystem[0][1] = RF_Pram[ino1 +  1];
    LocalCoordSystem[0][2] = RF_Pram[ino1 +  2];
    LocalCoordSystem[1][0] = RF_Pram[ino1 +  3];
    LocalCoordSystem[1][1] = RF_Pram[ino1 +  4];
    LocalCoordSystem[1][2] = RF_Pram[ino1 +  5];
    LocalCoordSystem[2][0] = RF_Pram[ino1 +  6];
    LocalCoordSystem[2][1] = RF_Pram[ino1 +  7];
    LocalCoordSystem[2][2] = RF_Pram[ino1 +  8];
    DistPToB               = RF_Pram[ino1 +  9];
    Weight[0]              = RF_Pram[ino1 + 10];
    Weight[1]              = RF_Pram[ino1 + 11];
    Weight[2]              = RF_Pram[ino1 + 12];
    Weight[3]              = RF_Pram[ino1 + 13];
    ContactType            = RF_Pram[ino1 + 15];
    
    for (std::size_t inode = 0; inode < rObj_2->GetGeometry().size(); inode++){
        noalias(other_to_me_vel) += rObj_2->GetGeometry()[inode].FastGetSolutionStepValue(VELOCITY) * Weight[inode];
    }

    KRATOS_CATCH("")
}

void SphericParticle::UpdateRF_Pram(DEMWall* rObj_2, std::size_t ino,
                                    double LocalCoordSystem[3][3], double& DistPToB,
                                    double Weight[4], int& ContactType)
{
    KRATOS_TRY


    std::vector<double>& RF_Pram = this->mNeighbourRigidFacesPram;

    int ino1 = ino * 16;

    RF_Pram[ino1 +  0] = LocalCoordSystem[0][0];
    RF_Pram[ino1 +  1] = LocalCoordSystem[0][1];
    RF_Pram[ino1 +  2] = LocalCoordSystem[0][2];
    RF_Pram[ino1 +  3] = LocalCoordSystem[1][0];
    RF_Pram[ino1 +  4] = LocalCoordSystem[1][1];
    RF_Pram[ino1 +  5] = LocalCoordSystem[1][2];
    RF_Pram[ino1 +  6] = LocalCoordSystem[2][0];
    RF_Pram[ino1 +  7] = LocalCoordSystem[2][1];
    RF_Pram[ino1 +  8] = LocalCoordSystem[2][2];
    RF_Pram[ino1 +  9] = DistPToB;
    RF_Pram[ino1 + 10] = Weight[0];
    RF_Pram[ino1 + 11] = Weight[1];
    RF_Pram[ino1 + 12] = Weight[2];
    RF_Pram[ino1 + 13] = Weight[3];
    RF_Pram[ino1 + 15] = ContactType;


    KRATOS_CATCH("")
}

//**************************************************************************************************************************************************
//**************************************************************************************************************************************************

void SphericParticle::ComputeBallToRigidFaceContactForce(array_1d<double, 3>& r_elastic_force,
                                                         array_1d<double, 3>& r_contact_force,
                                                         array_1d<double, 3>& rInitialRotaMoment,
                                                         array_1d<double, 3>& rigid_element_force,
                                                         ProcessInfo& r_current_process_info,
                                                         double mTimeStep,
                                                         int search_control)
{
    KRATOS_TRY

    std::vector<DEMWall*>& rNeighbours = this->mNeighbourRigidFaces;        
    array_1d<double, 3> vel            = GetGeometry()[0].FastGetSolutionStepValue(VELOCITY);        
    double tangential_vel[3]           = {0.0};
    
    for (unsigned int i = 0; i < rNeighbours.size(); i++) {
        
        DEMWall* wall = rNeighbours[i];        
        double LocalElasticContactForce[3]  = {0.0};
        double GlobalElasticContactForce[3] = {0.0};
        bool sliding = false;
        double ViscoDampingLocalContactForce[3] = {0.0};
        double LocalCoordSystem[3][3]       = {{0.0}, {0.0}, {0.0}};
        array_1d<double, 3> other_to_me_vel = ZeroVector(3);
        array_1d<double, 3> node_coor_array = this->GetGeometry()[0].Coordinates();
        double node_coor[3]                 = {0.0};

        DEM_COPY_SECOND_TO_FIRST_3(node_coor, node_coor_array) //MSIMSI 1 can be optimized.

        double ini_delta = GetInitialDeltaWithFEM(i);              
        double DistPToB = 0.0;

        int ContactType = -1;
        double Weight[4] = {0.0};

        ComputeRigidFaceToMeVelocity(rNeighbours[i], i, LocalCoordSystem, DistPToB, Weight, other_to_me_vel, ContactType);

        //MSI: Renew Distance for steps in between searches.
        if (ContactType == 1 || ContactType == 2 || ContactType == 3) {
        
            if (search_control == 1) { //Search active but not performed in this timestep
                
                double Coord[4][3] = { {0.0},{0.0},{0.0},{0.0} };
                double total_weight = 0.0;
                double tempWeight[2] = {0.0};
                int points = 0;
                int inode1 = 0, inode2 = 0;
                
                for (unsigned int inode = 0; inode < (rNeighbours[i]->GetGeometry().size()); inode++) {
                    
                    if (Weight[inode] > 1.0e-6){
                        DEM_COPY_SECOND_TO_FIRST_3(Coord[0+points], rNeighbours[i]->GetGeometry()[inode].Coordinates())
                        total_weight = total_weight + Weight[inode];
                        points++;
                        if (points == 1) {inode1 = inode;}
                        if (points == 2) {inode2 = inode;}
                    }

                    if (fabs(total_weight - 1.0) < 1.0e-6){
                        break;
                    }
                }
            
                bool contact_exist = true;

                if (points == 3 || points == 4) {contact_exist = GeometryFunctions::JudgeIfThisFaceIsContactWithParticle(points, Coord, node_coor, mRadius, LocalCoordSystem, Weight, DistPToB);}
                
                if (points == 2) {
                    contact_exist = GeometryFunctions::JudgeIfThisEdgeIsContactWithParticle(Coord[0], Coord[1], node_coor, mRadius, LocalCoordSystem, tempWeight, DistPToB);
                    Weight[inode1] = tempWeight[0];
                    Weight[inode2] = tempWeight[1];
                }
                
                if (points == 1) {
                    contact_exist = GeometryFunctions::JudgeIfThisPointIsContactWithParticle(Coord[0], node_coor, mRadius, LocalCoordSystem, DistPToB);
                    Weight[inode1] = 1.0;
                }
            
                if (contact_exist == false) {ContactType = -1;}
            
                UpdateRF_Pram(rNeighbours[i], i, LocalCoordSystem, DistPToB, Weight, ContactType);
            }
        
        }
        
        if (ContactType == 1 || ContactType == 2 || ContactType == 3) {
            double indentation = -(DistPToB - mRadius) - ini_delta;
            double DeltDisp[3] = {0.0};
            double DeltVel [3] = {0.0};

            DeltVel[0] = vel[0] - other_to_me_vel[0];
            DeltVel[1] = vel[1] - other_to_me_vel[1];
            DeltVel[2] = vel[2] - other_to_me_vel[2];

            // For translation movement delta displacement
            DeltDisp[0] = DeltVel[0] * mTimeStep;
            DeltDisp[1] = DeltVel[1] * mTimeStep;
            DeltDisp[2] = DeltVel[2] * mTimeStep;

            if (this->Is(DEMFlags::HAS_ROTATION)) {
            
                double unitary_angular_vel[3] = {0.0};
                //double tangential_vel[3]      = {0.0};

                array_1d<double,3> AngularVel = GetGeometry()[0].FastGetSolutionStepValue(ANGULAR_VELOCITY);
                //double Ang_Vel_Temp[3] = {AngularVel[0], AngularVel[1], AngularVel[2]};
                GeometryFunctions::CrossProduct(AngularVel, LocalCoordSystem[2], unitary_angular_vel);

                tangential_vel[0] = -unitary_angular_vel[0] * mRadius;
                tangential_vel[1] = -unitary_angular_vel[1] * mRadius;
                tangential_vel[2] = -unitary_angular_vel[2] * mRadius;
   
                // Contribution of the tangential velocity on the displacements
  
                DeltDisp[0] += tangential_vel[0] * mTimeStep;
                DeltDisp[1] += tangential_vel[1] * mTimeStep;
                DeltDisp[2] += tangential_vel[2] * mTimeStep;
            }

            double LocalDeltDisp[3] = {0.0};
    
            GeometryFunctions::VectorGlobal2Local(LocalCoordSystem, DeltDisp, LocalDeltDisp);

            DEM_COPY_SECOND_TO_FIRST_3(GlobalElasticContactForce, mFemNeighbourContactForces[i])

            GeometryFunctions::VectorGlobal2Local(LocalCoordSystem, GlobalElasticContactForce, LocalElasticContactForce);                    

            if (indentation > 0.0) {
                mDiscontinuumConstitutiveLaw->InitializeContactWithFEM(this, wall);
                
                LocalElasticContactForce[2]  = mDiscontinuumConstitutiveLaw->CalculateNormalForceWithFEM(indentation, this, wall);
                LocalElasticContactForce[2] -= mDiscontinuumConstitutiveLaw->CalculateCohesiveNormalForceWithFEM(this, wall);                                                       

                mDiscontinuumConstitutiveLaw->CalculateTangentialForceWithFEM(LocalElasticContactForce[2], LocalElasticContactForce, LocalDeltDisp, sliding, this, wall);
                
                double LocalRelVel[3] = {0.0};
                GeometryFunctions::VectorGlobal2Local(LocalCoordSystem, DeltVel, LocalRelVel);                
                mDiscontinuumConstitutiveLaw->CalculateViscoDampingForceWithFEM(LocalRelVel, ViscoDampingLocalContactForce, sliding, this, wall);
            }
            //---------------------------------------------DAMPING-FORCE-CALCULATION------------------------------------------------------------//                       

            double LocalContactForce[3]  = {0.0};
            double GlobalContactForce[3] = {0.0};

            AddUpFEMForcesAndProject(LocalCoordSystem, LocalContactForce,LocalElasticContactForce,GlobalContactForce,
                                     GlobalElasticContactForce,ViscoDampingLocalContactForce, r_elastic_force, r_contact_force, i);

            rigid_element_force[0] -= GlobalElasticContactForce[0];
            rigid_element_force[1] -= GlobalElasticContactForce[1];
            rigid_element_force[2] -= GlobalElasticContactForce[2];

            if (this->Is(DEMFlags::HAS_ROTATION)) {
                ComputeMoments(LocalElasticContactForce[2], mFemNeighbourContactForces[i], rInitialRotaMoment, LocalCoordSystem[2], this, indentation, true); //WARNING: sending itself as the neighbor!!
            }
        
            //WEAR        
            if (wall->GetProperties()[PRINT_WEAR]) {
                const double area                        = KRATOS_M_PI * mRadius * mRadius;
                const double density                     = GetDensity();
                const double inverse_of_volume           = 1.0 / (4.0 * 0.333333333333333 * area * mRadius);
                ComputeWear(LocalCoordSystem, vel, tangential_vel, mTimeStep, density, sliding, inverse_of_volume, LocalElasticContactForce[2], wall, node_coor_array);
            } //wall->GetProperties()[PRINT_WEAR] loop        
        } //ContactType loop          
    } //rNeighbours.size loop
    
    KRATOS_CATCH("")
}// ComputeBallToRigidFaceContactForce

void SphericParticle::ComputeWear(double LocalCoordSystem[3][3], array_1d<double, 3>& vel, double tangential_vel[3],
                                  double mTimeStep, double density, bool sliding, double inverse_of_volume,
                                  double LocalElasticContactForce, DEMWall* wall, array_1d<double, 3>& node_coor_array) {
        
                array_1d<double, 3> local_vel;
                GeometryFunctions::VectorGlobal2Local(LocalCoordSystem, vel, local_vel);
                double non_dim_volume_wear;
                double WallSeverityOfWear           = wall->GetProperties()[SEVERITY_OF_WEAR];
                double WallImpactSeverityOfWear     = wall->GetProperties()[IMPACT_WEAR_SEVERITY];
                double InverseOfWallBrinellHardness = 1.0 / (wall->GetProperties()[BRINELL_HARDNESS]);
                double vel_module = DEM_MODULUS_3(vel);
                double quotient_of_vels = fabs(local_vel[2]) / vel_module;
                double Sliding_0 = tangential_vel[0] * mTimeStep;
                double Sliding_1 = tangential_vel[1] * mTimeStep;
                double non_dim_impact_wear = 0.5 * WallImpactSeverityOfWear * InverseOfWallBrinellHardness * density * quotient_of_vels * quotient_of_vels
                                             * pow(1.0 - 4.0 * (1.0 - quotient_of_vels), 2) * vel_module * vel_module;
        
                if (sliding) {
                non_dim_volume_wear = WallSeverityOfWear * InverseOfWallBrinellHardness * inverse_of_volume * LocalElasticContactForce
                                    * sqrt(Sliding_0 * Sliding_0 + Sliding_1 * Sliding_1);
                } else {
                    non_dim_volume_wear = 0.0;
                }
            
                //COMPUTING THE PROJECTED POINT
            
                array_1d<double, 3> normal_to_wall;
            
                wall->CalculateNormal(normal_to_wall);
            
                array_1d<double, 3> relative_vector = wall->GetGeometry()[0].Coordinates() - node_coor_array; //We could have chosen [1] or [2], also.
            
                double dot_prod = DEM_INNER_PRODUCT_3(relative_vector, normal_to_wall);
            
                DEM_MULTIPLY_BY_SCALAR_3(normal_to_wall, dot_prod);
            
                array_1d<double, 3> inner_point = node_coor_array + normal_to_wall;
            
                array_1d<double, 3> relative_vector_0 = inner_point - wall->GetGeometry()[0].Coordinates();
                array_1d<double, 3> relative_vector_1 = inner_point - wall->GetGeometry()[1].Coordinates();
                array_1d<double, 3> relative_vector_2 = inner_point - wall->GetGeometry()[2].Coordinates();
            
                double distance_0 = DEM_MODULUS_3(relative_vector_0);
                double distance_1 = DEM_MODULUS_3(relative_vector_1);
                double distance_2 = DEM_MODULUS_3(relative_vector_2);
                double inverse_of_total_distance = 1.0 / (distance_0 + distance_1 + distance_2);
            
                double weight_0 = 1.0 - (distance_1 + distance_2) * inverse_of_total_distance;
                double weight_1 = 1.0 - (distance_2 + distance_0) * inverse_of_total_distance;
                double weight_2 = 1.0 - (distance_0 + distance_1) * inverse_of_total_distance; // It could be also: weight_2 = 1.0 - weight_0 - weight_1;
                 
                wall->GetGeometry()[0].SetLock();
                wall->GetGeometry()[0].FastGetSolutionStepValue(NON_DIMENSIONAL_VOLUME_WEAR) += weight_0 * non_dim_volume_wear;
                wall->GetGeometry()[0].FastGetSolutionStepValue(IMPACT_WEAR) += weight_0 * non_dim_impact_wear;
                wall->GetGeometry()[0].UnSetLock();
            
                wall->GetGeometry()[1].SetLock();
                wall->GetGeometry()[1].FastGetSolutionStepValue(NON_DIMENSIONAL_VOLUME_WEAR) += weight_1 * non_dim_volume_wear;
                wall->GetGeometry()[1].FastGetSolutionStepValue(IMPACT_WEAR) += weight_1 * non_dim_impact_wear;
                wall->GetGeometry()[1].UnSetLock();
            
                wall->GetGeometry()[2].SetLock();
                wall->GetGeometry()[2].FastGetSolutionStepValue(NON_DIMENSIONAL_VOLUME_WEAR) += weight_2 * non_dim_volume_wear;
                wall->GetGeometry()[2].FastGetSolutionStepValue(IMPACT_WEAR) += weight_2 * non_dim_impact_wear;
                wall->GetGeometry()[2].UnSetLock();
            
} //ComputeWear



//**************************************************************************************************************************************************
//**************************************************************************************************************************************************


void SphericParticle::CreateDiscontinuumConstitutiveLaws(const ProcessInfo& r_current_process_info)
{

    mDiscontinuumConstitutiveLaw = GetProperties()[DEM_DISCONTINUUM_CONSTITUTIVE_LAW_POINTER]->Clone();
    mDiscontinuumConstitutiveLaw->Initialize(r_current_process_info);

}




//**************************************************************************************************************************************************
//**************************************************************************************************************************************************

void SphericParticle::CalculateDampingMatrix(MatrixType& rDampingMatrix, ProcessInfo& r_current_process_info){}

//**************************************************************************************************************************************************
//**************************************************************************************************************************************************

void SphericParticle::GetDofList(DofsVectorType& ElementalDofList, ProcessInfo& CurrentProcessInfo)
{
    KRATOS_TRY

    ElementalDofList.resize(0);

    for (unsigned int i = 0; i < GetGeometry().size(); i++){
        ElementalDofList.push_back(GetGeometry()[i].pGetDof(VELOCITY_X));
        ElementalDofList.push_back(GetGeometry()[i].pGetDof(VELOCITY_Y));

        if (GetGeometry().WorkingSpaceDimension() == 3){
            ElementalDofList.push_back(GetGeometry()[i].pGetDof(VELOCITY_Z));
        }

        ElementalDofList.push_back(GetGeometry()[i].pGetDof(ANGULAR_VELOCITY_X));
        ElementalDofList.push_back(GetGeometry()[i].pGetDof(ANGULAR_VELOCITY_Y));

        if (GetGeometry().WorkingSpaceDimension() == 3){
            ElementalDofList.push_back(GetGeometry()[i].pGetDof(ANGULAR_VELOCITY_Z));
        }
    }

    KRATOS_CATCH("")
}


//**************************************************************************************************************************************************
//**************************************************************************************************************************************************

void SphericParticle::InitializeSolutionStep(ProcessInfo& r_current_process_info)
{
    KRATOS_TRY

    KRATOS_CATCH("")
}

//**************************************************************************************************************************************************
//**************************************************************************************************************************************************

void SphericParticle::FinalizeSolutionStep(ProcessInfo& r_current_process_info){}

//**************************************************************************************************************************************************
//**************************************************************************************************************************************************

void SphericParticle::CustomInitialize(){}

//**************************************************************************************************************************************************
//**************************************************************************************************************************************************

void SphericParticle::CalculateEquivalentConstitutiveParameters(array_1d<double, 3>& other_to_me_vect,
                                                                const double& other_radius,
                                                                const double& radius_sum,
                                                                double& kn,
                                                                double& kt,
                                                                double& equiv_visco_damp_coeff_normal,
                                                                double& equiv_visco_damp_coeff_tangential,
                                                                double& equiv_tg_of_fri_ang,
                                                                SphericParticle* p_neighbour)
{
    double other_real_mass = p_neighbour->GetMass();
    double radius_sum_i = 1 / radius_sum;
    double equiv_radius = 2 * mRadius * other_radius * radius_sum_i;
    double equiv_area = 0.25 * KRATOS_M_PI * equiv_radius * equiv_radius; // 0.25 is because we take only the half of the equivalent radius, corresponding to the case of one ball with radius Requivalent and other = radius 0.        
    double equiv_mass = sqrt(mRealMass * other_real_mass);

    double myYoung = GetYoung();
    double myPoisson = GetPoisson();
    double myLnOfRestitCoeff = GetLnOfRestitCoeff();
    double myTgOfFrictionAngle = GetTgOfFrictionAngle();

    double other_young;
    double other_poisson;
    double equiv_ln_of_restit_coeff;
    double equiv_young;
    double equiv_poisson;
    double other_ln_of_restit_coeff;

    if (mFastProperties == p_neighbour->GetFastProperties()){
        other_young   = myYoung;
        other_poisson = myPoisson;
        equiv_poisson = myPoisson;
        other_ln_of_restit_coeff = myLnOfRestitCoeff;
        equiv_ln_of_restit_coeff = myLnOfRestitCoeff;
        equiv_tg_of_fri_ang      = myTgOfFrictionAngle;
    }

    else {
        other_young = p_neighbour->GetYoung();
        other_poisson = p_neighbour->GetPoisson();
        if ((myPoisson + other_poisson)!= 0.0) {
            equiv_poisson                     = 2.0 * myPoisson * other_poisson / (myPoisson + other_poisson);
        } else {
            equiv_poisson = 0.0;
        }
        other_ln_of_restit_coeff = p_neighbour->GetLnOfRestitCoeff();
        equiv_ln_of_restit_coeff = 0.5 * (myLnOfRestitCoeff + other_ln_of_restit_coeff);
        const double other_tg_of_fri_angle = p_neighbour->GetTgOfFrictionAngle();
        equiv_tg_of_fri_ang      = 0.5 * (myTgOfFrictionAngle + other_tg_of_fri_angle);
    }

    switch (mElasticityType) { //  0 ---linear compression & tension ; 1 --- Hertzian (non-linear compression, linear tension)
        case 0:
            equiv_young = 2.0 * myYoung * other_young / (myYoung + other_young);
            kn          = equiv_young * equiv_area * radius_sum_i; //KRATOS_M_PI * 0.5 * equiv_young * equiv_radius; //M: CANET FORMULA
            kt          = kn / (2.0 + equiv_poisson + equiv_poisson);
            break;

        case 1:
            equiv_young = myYoung * other_young / (other_young * (1.0 - myPoisson * myPoisson) + myYoung * (1.0 - other_poisson * other_poisson));
            kn          = 1.3333333333333 * equiv_young * sqrt(0.5 * equiv_radius);
            kt          = 2.0 * kn * (1.0 - equiv_poisson * equiv_poisson) / ((2.0 - equiv_poisson) * (1.0 + equiv_poisson));
            break;

        default:
            equiv_young = 2.0 * myYoung * other_young / (myYoung + other_young);
            kn          = equiv_young * equiv_area * radius_sum_i; //KRATOS_M_PI * 0.5 * equiv_young * equiv_radius; //M: CANET FORMULA
            kt          = kn / (2.0 + equiv_poisson + equiv_poisson);
            break;
    }
    
    /*
    double RCTS = 2.0 * sqrt(mRealMass / kn);
    KRATOS_WATCH(RCTS)
    std::cout << "Rayleigh's critical time step is = " << RCTS << std::endl;
    std::cout << "1% of Rayleigh's critical time step is = " << 0.01 * RCTS << std::endl;
    */
    
    if (GetLnOfRestitCoeff() > 0.0 || other_ln_of_restit_coeff > 0.0) { // Limit expressions when the restitution coefficient tends to 0. Variable lnRestitCoeff is set to 1.0 (instead of minus infinite) by the problem type.
        equiv_visco_damp_coeff_normal = 2.0 * sqrt(equiv_mass * kn);
    }

    else {
        equiv_visco_damp_coeff_normal = - 2.0 * equiv_ln_of_restit_coeff * sqrt(equiv_mass * kn / (equiv_ln_of_restit_coeff * equiv_ln_of_restit_coeff + KRATOS_M_PI * KRATOS_M_PI));
    }

    double aux_norm_to_tang = sqrt(kt / kn);
    equiv_visco_damp_coeff_tangential = equiv_visco_damp_coeff_normal * aux_norm_to_tang;
}

//**************************************************************************************************************************************************
//**************************************************************************************************************************************************

void SphericParticle::ComputeAdditionalForces(array_1d<double, 3>& externally_applied_force,
                                              array_1d<double, 3>& externally_applied_moment,
                                              ProcessInfo& r_current_process_info, const array_1d<double,3>& gravity)
{
    externally_applied_force = mRealMass * gravity;
}

//**************************************************************************************************************************************************
//**************************************************************************************************************************************************

void SphericParticle::CalculateViscoDamping(double LocalRelVel[3],
                                            double ViscoDampingLocalContactForce[3],
                                            double indentation,
                                            double equiv_visco_damp_coeff_normal,
                                            double equiv_visco_damp_coeff_tangential,
                                            bool sliding)
{
    //*** The check is component-wise since localContactForce and RelVel have in principle no relationship.
    // The visco force can be higher than the contact force only if they go to the same direction. (in my opinion)
    // But in opposite direction the visco damping can't overpass the force...

    if (mDampType > 0) {

        if (mDampType == 11 || mDampType == 10) {
            ViscoDampingLocalContactForce[2] = - equiv_visco_damp_coeff_normal * LocalRelVel[2];
        }

        if (sliding == false && (mDampType == 1 || mDampType == 11)) { //only applied when no sliding to help to the regularized friction law or the spring convergence
            ViscoDampingLocalContactForce[0] = - equiv_visco_damp_coeff_tangential * LocalRelVel[0];
            ViscoDampingLocalContactForce[1] = - equiv_visco_damp_coeff_tangential * LocalRelVel[1];
        }
    }
}

//**************************************************************************************************************************************************
//**************************************************************************************************************************************************

void SphericParticle::AddUpForcesAndProject(double OldCoordSystem[3][3],
                                            double LocalCoordSystem[3][3],
                                            double LocalContactForce[3],
                                            double LocalElasticContactForce[3],
                                            double GlobalContactForce[3],
                                            double GlobalElasticContactForce[3],
                                            double ViscoDampingLocalContactForce[3],
                                            array_1d<double, 3> &r_elastic_force,
                                            array_1d<double, 3> &r_contact_force,
                                            const unsigned int i_neighbour_count)
{
    for (unsigned int index = 0; index < 3; index++) {
        LocalContactForce[index] = LocalElasticContactForce[index] + ViscoDampingLocalContactForce[index];
    }

    GeometryFunctions::VectorLocal2Global(OldCoordSystem, LocalElasticContactForce, GlobalElasticContactForce);
    GeometryFunctions::VectorLocal2Global(OldCoordSystem, LocalContactForce, GlobalContactForce);

    // Saving contact forces (We need to, since tangential elastic force is history-dependent)
    DEM_COPY_SECOND_TO_FIRST_3(mNeighbourElasticContactForces[i_neighbour_count], GlobalElasticContactForce)
    DEM_COPY_SECOND_TO_FIRST_3(mNeighbourTotalContactForces[i_neighbour_count], GlobalContactForce)
    DEM_ADD_SECOND_TO_FIRST(mContactForce, GlobalContactForce)
    DEM_ADD_SECOND_TO_FIRST(r_elastic_force, GlobalElasticContactForce)
    DEM_ADD_SECOND_TO_FIRST(r_contact_force, GlobalContactForce)
}

//**************************************************************************************************************************************************
//**************************************************************************************************************************************************

void SphericParticle::AddUpFEMForcesAndProject(double LocalCoordSystem[3][3],
                                               double LocalContactForce[3],
                                               double LocalElasticContactForce[3],
                                               double GlobalContactForce[3],
                                               double GlobalElasticContactForce[3],
                                               double ViscoDampingLocalContactForce[3],
                                               array_1d<double, 3>& r_elastic_force,
                                               array_1d<double, 3>& r_contact_force,
                                               const unsigned int iRigidFaceNeighbour)
{
    for (unsigned int index = 0; index < 3; index++) {
        LocalContactForce[index] = LocalElasticContactForce[index] + ViscoDampingLocalContactForce[index];
    }

    GeometryFunctions::VectorLocal2Global(LocalCoordSystem, LocalElasticContactForce, GlobalElasticContactForce);
    GeometryFunctions::VectorLocal2Global(LocalCoordSystem, LocalContactForce, GlobalContactForce);

    // Saving contact forces (We need to, since tangential elastic force is history-dependent)
    DEM_COPY_SECOND_TO_FIRST_3(mFemNeighbourContactForces[iRigidFaceNeighbour], GlobalElasticContactForce)
    DEM_ADD_SECOND_TO_FIRST(mContactForce, GlobalContactForce)
    DEM_ADD_SECOND_TO_FIRST(r_elastic_force, GlobalElasticContactForce)
    DEM_ADD_SECOND_TO_FIRST(r_contact_force, GlobalContactForce)

    // Global stored contact force between rigid face and particle, used by FEM elements
    std::vector<double>& neighbour_rigid_faces_elastic_contact_force = this->mNeighbourRigidFacesElasticContactForce;
    std::vector<double>& neighbour_rigid_faces_total_contact_force = this->mNeighbourRigidFacesTotalContactForce;

    for (unsigned int i = 0; i < 3; ++i) {
        neighbour_rigid_faces_elastic_contact_force[3 * iRigidFaceNeighbour + i] = GlobalElasticContactForce[i];
        neighbour_rigid_faces_total_contact_force[3 * iRigidFaceNeighbour + i]   = GlobalContactForce[i];
    }
}

//**************************************************************************************************************************************************
//**************************************************************************************************************************************************

void SphericParticle::MemberDeclarationFirstStep(const ProcessInfo& r_process_info)

{
    // Passing the element id to the node upon initialization    
    if (r_process_info[PRINT_EXPORT_ID] == 1) {
        this->GetGeometry()[0].FastGetSolutionStepValue(EXPORT_ID) = double(this->Id());
    }

    mDampType                                    = r_process_info[DAMP_TYPE];
    mElasticityType                              = r_process_info[FORCE_CALCULATION_TYPE];        
        
    if (r_process_info[ROTATION_OPTION])         this->Set(DEMFlags::HAS_ROTATION, true);
    else                                         this->Set(DEMFlags::HAS_ROTATION, false);
    if (r_process_info[ROLLING_FRICTION_OPTION]) this->Set(DEMFlags::HAS_ROLLING_FRICTION, true);
    else                                         this->Set(DEMFlags::HAS_ROLLING_FRICTION, false);
    if (r_process_info[CRITICAL_TIME_OPTION])    this->Set(DEMFlags::HAS_CRITICAL_TIME, true);
    else                                         this->Set(DEMFlags::HAS_CRITICAL_TIME, false);
                                                 this->Set(DEMFlags::HAS_ROTATION_SPRING, false);

    AdditionalMemberDeclarationFirstStep(r_process_info);
    
}

double SphericParticle::GetInitialDeltaWithFEM(int index) //only available in continuum_particle
{
    return 0.0;
}

void SphericParticle::Calculate(const Variable<double>& rVariable, double& Output, const ProcessInfo& r_current_process_info)
{
    KRATOS_TRY
    
    //CRITICAL DELTA CALCULATION

    if (rVariable == DELTA_TIME){
        double mass = mRealMass;
        double coeff = r_current_process_info[NODAL_MASS_COEFF];

        if (coeff > 1.0){
            KRATOS_THROW_ERROR(std::runtime_error, "The coefficient assigned for virtual mass is larger than one. Virtual_mass_coeff is ", coeff);
        }

        else if ((coeff == 1.0) && (r_current_process_info[VIRTUAL_MASS_OPTION])){
            Output = 9.0E09;
        }

        else {

            if (r_current_process_info[VIRTUAL_MASS_OPTION]){
                mass = mass / (1 - coeff);
            }

            double K = KRATOS_M_PI * GetYoung() * mRadius; //M. Error, should be the same that the local definition.

            Output = 0.34 * sqrt(mass / K);

            if (this->Is(DEMFlags::HAS_ROTATION) ){
                Output *= 0.5; //factor for critical time step when rotation is allowed.
            }
        }

        return;
    }

    if (rVariable == KINETIC_ENERGY){
        CalculateKineticEnergy(Output);

        return;
    }

    if (rVariable == ELASTIC_ENERGY_OF_CONTACTS){
        CalculateElasticEnergyOfContacts(Output);

        return;
    }

    AdditionalCalculate(rVariable, Output, r_current_process_info);

    KRATOS_CATCH("")
}// Calculate

//**************************************************************************************************************************************************
//**************************************************************************************************************************************************

void SphericParticle::Calculate(const Variable<array_1d<double, 3> >& rVariable, array_1d<double, 3>& Output,
                                const ProcessInfo& r_current_process_info)
{
    if (rVariable == MOMENTUM){
        CalculateMomentum(Output);
    }

    else if (rVariable == ANGULAR_MOMENTUM){
        CalculateLocalAngularMomentum(Output);
    }
}

//**************************************************************************************************************************************************
//**************************************************************************************************************************************************

void SphericParticle::Calculate(const Variable<Vector >& rVariable, Vector& Output, const ProcessInfo& r_current_process_info){}
void SphericParticle::Calculate(const Variable<Matrix >& rVariable, Matrix& Output, const ProcessInfo& r_current_process_info){}

void SphericParticle::AdditionalMemberDeclarationFirstStep(const ProcessInfo& r_process_info){}
void SphericParticle::AdditionalCalculate(const Variable<double>& rVariable, double& Output, const ProcessInfo& r_current_process_info){}

int    SphericParticle::GetClusterId()                                                   { return mClusterId;                                                               }
void   SphericParticle::SetClusterId(int givenId)                                        { mClusterId = givenId;                                                                 }
double SphericParticle::GetRadius()                                                      { return mRadius;                                                                  }
void   SphericParticle::SetRadius(double radius)                                         { mRadius = radius;                                                                }
double SphericParticle::GetMass()                                                    { return mRealMass;                                                                }
void   SphericParticle::SetMass(double real_mass)                                    { mRealMass = real_mass;                                                           }

double SphericParticle::GetYoung()                                                       { return GetFastProperties()->GetYoung();                                          }
double SphericParticle::GetRollingFriction()                                             { return GetFastProperties()->GetRollingFriction();                                }
double SphericParticle::GetPoisson()                                                     { return GetFastProperties()->GetPoisson();                                        }
double SphericParticle::GetTgOfFrictionAngle()                                           { return GetFastProperties()->GetTgOfFrictionAngle() ;                             }
double SphericParticle::GetLnOfRestitCoeff()                                             { return GetFastProperties()->GetLnOfRestitCoeff();                                }
double SphericParticle::GetDensity()                                                     { return GetFastProperties()->GetDensity();                                        }
int    SphericParticle::GetParticleMaterial()                                            { return GetFastProperties()->GetParticleMaterial();                               }
double SphericParticle::GetParticleCohesion()                                            { return GetFastProperties()->GetParticleCohesion();                               }

void   SphericParticle::SetYoungFromProperties(double* young)                            { GetFastProperties()->SetYoungFromProperties( young);                             }
void   SphericParticle::SetRollingFrictionFromProperties(double* rolling_friction)       { GetFastProperties()->SetRollingFrictionFromProperties( rolling_friction);        }
void   SphericParticle::SetPoissonFromProperties(double* poisson)                        { GetFastProperties()->SetPoissonFromProperties( poisson);                         }
void   SphericParticle::SetTgOfFrictionAngleFromProperties(double* tg_of_friction_angle) { GetFastProperties()->SetTgOfFrictionAngleFromProperties( tg_of_friction_angle);  }
void   SphericParticle::SetLnOfRestitCoeffFromProperties(double* ln_of_restit_coeff)     { GetFastProperties()->SetLnOfRestitCoeffFromProperties( ln_of_restit_coeff);      }
void   SphericParticle::SetDensityFromProperties(double* density)                        { GetFastProperties()->SetDensityFromProperties( density);                         }
void   SphericParticle::SetParticleMaterialFromProperties(int* particle_material)        { GetFastProperties()->SetParticleMaterialFromProperties( particle_material);      }
void   SphericParticle::SetParticleCohesionFromProperties(double* particle_cohesion)     { GetFastProperties()->SetParticleCohesionFromProperties( particle_cohesion);      }

PropertiesProxy* SphericParticle::GetFastProperties()                                    { return mFastProperties;                                                          }
void   SphericParticle::SetFastProperties(PropertiesProxy* pProps)                       { mFastProperties = pProps;                                                        }
void   SphericParticle::SetFastProperties(std::vector<PropertiesProxy>& list_of_proxies) {
    for (unsigned int j=0;j<list_of_proxies.size(); j++){
        if ( list_of_proxies[j].GetId() == GetProperties().Id() ) {
            SetFastProperties(&list_of_proxies[j]);
            return;
        }
    }
}

double SphericParticle::SlowGetYoung()                                                   { return GetProperties()[YOUNG_MODULUS];                                           }
double SphericParticle::SlowGetRollingFriction()                                         { return GetProperties()[ROLLING_FRICTION_OPTION];                                 }
double SphericParticle::SlowGetPoisson()                                                 { return GetProperties()[POISSON_RATIO];                                           }
double SphericParticle::SlowGetTgOfFrictionAngle()                                       { return GetProperties()[PARTICLE_FRICTION];                                       }
double SphericParticle::SlowGetLnOfRestitCoeff()                                         { return GetProperties()[LN_OF_RESTITUTION_COEFF];                                 }
double SphericParticle::SlowGetDensity()                                                 { return GetProperties()[PARTICLE_DENSITY];                                        }
int    SphericParticle::SlowGetParticleMaterial()                                        { return GetProperties()[PARTICLE_MATERIAL];                                       }
double SphericParticle::SlowGetParticleCohesion()                                        { return GetProperties()[PARTICLE_COHESION];                                       }

}  // namespace Kratos.

