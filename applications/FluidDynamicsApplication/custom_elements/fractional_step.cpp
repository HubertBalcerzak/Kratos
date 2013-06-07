#include "fractional_step.h"

namespace Kratos {

/*
 * public FractionalStep<TDim> functions
 */

template< unsigned int TDim >
void FractionalStep<TDim>::Initialize()
{
}

template< unsigned int TDim >
void FractionalStep<TDim>::InitializeSolutionStep(ProcessInfo &rCurrentProcessInfo)
{
}

template< unsigned int TDim >
void FractionalStep<TDim>::InitializeNonLinearIteration(ProcessInfo &rCurrentProcessInfo)
{

}

template< unsigned int TDim >
void FractionalStep<TDim>::CalculateLocalSystem(MatrixType& rLeftHandSideMatrix,
                                                VectorType& rRightHandSideVector,
                                                ProcessInfo& rCurrentProcessInfo)
{
    KRATOS_TRY;

    switch ( rCurrentProcessInfo[FRACTIONAL_STEP] )
    {
    case 1:
    {
        this->CalculateLocalFractionalVelocitySystem(rLeftHandSideMatrix,rRightHandSideVector,rCurrentProcessInfo);
        break;
    }
    case 5:
    {
        this->CalculateLocalPressureSystem(rLeftHandSideMatrix,rRightHandSideVector,rCurrentProcessInfo);
        break;
    }
    case 6:
    {
        KRATOS_ERROR(std::logic_error,"Full solution of end of step velocity is not implemented, see Calculate(VELOCITY)","");
        break;
    }
    default:
    {
        KRATOS_ERROR(std::logic_error,"Unexpected value for FRACTIONAL_STEP index: ",rCurrentProcessInfo[FRACTIONAL_STEP]);
    }
    }

    KRATOS_CATCH("");
}

template< unsigned int TDim >
void FractionalStep<TDim>::Calculate(const Variable<double> &rVariable,
                                     double &rOutput,
                                     const ProcessInfo &rCurrentProcessInfo)
{
    if (rVariable == DIVPROJ)
    {
        const GeometryType& rGeom = this->GetGeometry();
        const SizeType NumNodes = rGeom.PointsNumber();
        const unsigned int LocalSize = TDim * NumNodes;

        // Shape functions and integration points
        ShapeFunctionDerivativesArrayType DN_DX;
        Matrix NContainer;
        VectorType GaussWeights;
        this->CalculateGeometryData(DN_DX,NContainer,GaussWeights);
        const unsigned int NumGauss = GaussWeights.size();

        VectorType MomentumRHS = ZeroVector(LocalSize);
        VectorType MassRHS = ZeroVector(NumNodes);
        VectorType NodalArea = ZeroVector(NumNodes);

        // Loop on integration points
        for (unsigned int g = 0; g < NumGauss; g++)
        {
            const ShapeFunctionsType& N = row(NContainer,g);
            const double GaussWeight = GaussWeights[g];

            for (unsigned int i = 0; i < NumNodes; i++)
                NodalArea[i] += N[i] * GaussWeight;

            this->CalculateProjectionRHS(MomentumRHS,MassRHS,N,DN_DX[g],GaussWeight);
        }

        // Carefully write results to nodal variables, to avoid parallelism problems
        unsigned int RowIndex = 0;
        for (SizeType i = 0; i < NumNodes; ++i)
        {
            this->GetGeometry()[i].SetLock(); // So it is safe to write in the node in OpenMP
            array_1d<double,3>& rMomValue = this->GetGeometry()[i].FastGetSolutionStepValue(ADVPROJ);
            for (unsigned int d = 0; d < TDim; ++d)
                rMomValue[d] += MomentumRHS[RowIndex++];
            this->GetGeometry()[i].FastGetSolutionStepValue(DIVPROJ) += MassRHS[i];
            this->GetGeometry()[i].FastGetSolutionStepValue(NODAL_AREA) += NodalArea[i];
            this->GetGeometry()[i].UnSetLock(); // Free the node for other threads
        }
    }
}

template< unsigned int TDim >
void FractionalStep<TDim>::Calculate(const Variable<array_1d<double,3> > &rVariable,
                                     array_1d<double,3> &rOutput,
                                     const ProcessInfo &rCurrentProcessInfo)
{
    if (rVariable == ADVPROJ)
    {
        double Tmp = 0.0;
        this->Calculate(DIVPROJ,Tmp,rCurrentProcessInfo);
    }
    else if (rVariable == CONV_PROJ)
    {
        const GeometryType& rGeom = this->GetGeometry();
        const unsigned int NumNodes = rGeom.PointsNumber();
        const unsigned int LocalSize = TDim * NumNodes;

        // Shape functions and integration points
        ShapeFunctionDerivativesArrayType DN_DX;
        Matrix NContainer;
        VectorType GaussWeights;
        this->CalculateGeometryData(DN_DX,NContainer,GaussWeights);
        const unsigned int NumGauss = GaussWeights.size();

        VectorType ConvTerm = ZeroVector(LocalSize);
        VectorType PresTerm = ZeroVector(LocalSize);
        VectorType DivTerm = ZeroVector(NumNodes);
        VectorType NodalArea = ZeroVector(NumNodes);

        // Loop on integration points
        for (unsigned int g = 0; g < NumGauss; g++)
        {
            const ShapeFunctionsType& N = row(NContainer,g);
            const double GaussWeight = GaussWeights[g];

            for (unsigned int i = 0; i < NumNodes; i++)
                NodalArea[i] += N[i] * GaussWeight;

            this->CalculateProjectionRHS(ConvTerm,PresTerm,DivTerm,N,DN_DX[g],GaussWeight);
        }

        // Carefully write results to nodal variables, to avoid parallelism problems
        unsigned int RowIndex = 0;
        for (SizeType i = 0; i < NumNodes; ++i)
        {
            this->GetGeometry()[i].SetLock(); // So it is safe to write in the node in OpenMP
            array_1d<double,3>& rConvVal = this->GetGeometry()[i].FastGetSolutionStepValue(CONV_PROJ);
            array_1d<double,3>& rPresVal = this->GetGeometry()[i].FastGetSolutionStepValue(PRESS_PROJ);
            for (unsigned int d = 0; d < TDim; ++d)
            {
                rConvVal[d] += ConvTerm[RowIndex];
                rPresVal[d] += PresTerm[RowIndex];
                ++RowIndex;
            }
            this->GetGeometry()[i].FastGetSolutionStepValue(DIVPROJ) += DivTerm[i];
            this->GetGeometry()[i].FastGetSolutionStepValue(NODAL_AREA) += NodalArea[i];
            this->GetGeometry()[i].UnSetLock(); // Free the node for other threads
        }
    }
    else if (rVariable == VELOCITY)
    {
        GeometryType& rGeom = this->GetGeometry();
        const SizeType NumNodes = rGeom.PointsNumber();
        const SizeType LocalSize = TDim * NumNodes;

        // Shape functions and integration points
        ShapeFunctionDerivativesArrayType DN_DX;
        Matrix NContainer;
        VectorType GaussWeights;
        this->CalculateGeometryData(DN_DX,NContainer,GaussWeights);
        const unsigned int NumGauss = GaussWeights.size();

        VectorType NodalVelCorrection = ZeroVector(LocalSize);

        // Loop on integration points
        for (unsigned int g = 0; g < NumGauss; ++g)
        {
            const ShapeFunctionsType& N = row(NContainer,g);
            const ShapeFunctionDerivativesType& rDN_DX = DN_DX[g];

            double Density;

            this->EvaluateInPoint(Density,DENSITY,N);

            const double Coeff = GaussWeights[g] / ( Density * rCurrentProcessInfo[BDF_COEFFICIENTS][0] );

            // Calculate contribution to the gradient term (RHS)
            double DeltaPressure;
            this->EvaluateInPoint(DeltaPressure,PRESSURE_OLD_IT,N);

            SizeType RowIndex = 0;

            for (SizeType i = 0; i < NumNodes; ++i)
            {
                for (SizeType d = 0; d < TDim; ++d)
                {
                    NodalVelCorrection[RowIndex++] += Coeff * rDN_DX(i,d) * DeltaPressure;
                }
            }
        }

        SizeType Index = 0;

        for (SizeType i = 0; i < NumNodes; ++i)
        {
            rGeom[i].SetLock(); // So it is safe to write in the node in OpenMP
            array_1d<double,3>& rTemp = rGeom[i].FastGetSolutionStepValue(FRACT_VEL);
            for (SizeType d = 0; d < TDim; ++d)
            {
                rTemp[d] += NodalVelCorrection[Index++];
            }
            rGeom[i].UnSetLock(); // Free the node for other threads
        }

    }
}

template< unsigned int TDim >
void FractionalStep<TDim>::EquationIdVector(EquationIdVectorType& rResult,
                                            ProcessInfo& rCurrentProcessInfo)
{
    KRATOS_TRY;

    switch ( rCurrentProcessInfo[FRACTIONAL_STEP] )
    {
    case 1:
    {
        this->VelocityEquationIdVector(rResult,rCurrentProcessInfo);
        break;
    }
    case 5:
    {
        this->PressureEquationIdVector(rResult,rCurrentProcessInfo);
        break;
    }
    case 6:
    {
        this->VelocityEquationIdVector(rResult,rCurrentProcessInfo);
        break;
    }
    default:
    {
        KRATOS_ERROR(std::logic_error,"Unexpected value for FRACTIONAL_STEP index: ",rCurrentProcessInfo[FRACTIONAL_STEP]);
    }
    }

    KRATOS_CATCH("");
}

template< unsigned int TDim >
void FractionalStep<TDim>::GetDofList(DofsVectorType& rElementalDofList,
                                      ProcessInfo& rCurrentProcessInfo)
{
    KRATOS_TRY;

    switch ( rCurrentProcessInfo[FRACTIONAL_STEP] )
    {
    case 1:
    {
        this->GetVelocityDofList(rElementalDofList,rCurrentProcessInfo);
        break;
    }
    case 5:
    {
        this->GetPressureDofList(rElementalDofList,rCurrentProcessInfo);
        break;
    }
    case 6:
    {
        this->GetVelocityDofList(rElementalDofList,rCurrentProcessInfo);
        break;
    }
    default:
    {
        KRATOS_ERROR(std::logic_error,"Unexpected value for FRACTIONAL_STEP index: ",rCurrentProcessInfo[FRACTIONAL_STEP]);
    }
    }

    KRATOS_CATCH("");
}

template< unsigned int TDim >
void FractionalStep<TDim>::CalculateLocalFractionalVelocitySystem(MatrixType& rLeftHandSideMatrix,
                                                                  VectorType& rRightHandSideVector,
                                                                  ProcessInfo& rCurrentProcessInfo)
{
    const GeometryType& rGeom = this->GetGeometry();
    const SizeType NumNodes = rGeom.PointsNumber();
    const SizeType LocalSize = TDim * NumNodes;

    // Check sizes and initialize
    if( rLeftHandSideMatrix.size1() != LocalSize )
        rLeftHandSideMatrix.resize(LocalSize,LocalSize);

    rLeftHandSideMatrix = ZeroMatrix(LocalSize,LocalSize);

    if( rRightHandSideVector.size() != LocalSize )
        rRightHandSideVector.resize(LocalSize);

    rRightHandSideVector = ZeroVector(LocalSize);

    // Shape functions and integration points
    ShapeFunctionDerivativesArrayType DN_DX;
    Matrix NContainer;
    VectorType GaussWeights;
    this->CalculateGeometryData(DN_DX,NContainer,GaussWeights);
    const unsigned int NumGauss = GaussWeights.size();

    MatrixType MassMatrix = ZeroMatrix(LocalSize,LocalSize);

    // Stabilization parameters
    double ElemSize = this->ElementSize();
    double TauOne;
    double TauTwo;

    // Loop on integration points
    for (unsigned int g = 0; g < NumGauss; g++)
    {
        const double GaussWeight = GaussWeights[g];
        const ShapeFunctionsType& N = row(NContainer,g);
        const ShapeFunctionDerivativesType& rDN_DX = DN_DX[g];

        // Evaluate required variables at the integration point
        double Density;
        double MassProjection;
        array_1d<double,3> BodyForce(3,0.0);
        array_1d<double,3> MomentumProjection(3,0.0);

        this->EvaluateInPoint(Density,DENSITY,N);
        this->EvaluateInPoint(MassProjection,DIVPROJ,N);
        this->EvaluateInPoint(BodyForce,BODY_FORCE,N);
//        this->EvaluateInPoint(MomentumProjection,ADVPROJ,N);
        this->EvaluateInPoint(MomentumProjection,CONV_PROJ,N);

        // Evaluate the pressure and pressure gradient at this point (for the G * P_n term)
        double OldPressure;
        this->EvaluateInPoint(OldPressure,PRESSURE,N,0);

        // For ALE: convective velocity
        array_1d<double,3> ConvVel(3,0.0);
        this->EvaluateConvVelocity(ConvVel,N);

        double Viscosity = this->EffectiveViscosity(N,rDN_DX,ElemSize,rCurrentProcessInfo);
        this->CalculateTau(TauOne,TauTwo,ElemSize,ConvVel,Density,Viscosity,rCurrentProcessInfo);

        // Evaluate convection operator Velocity * Grad(N)
        Vector UGradN(NumNodes);
        this->ConvectionOperator(UGradN,ConvVel,rDN_DX);

        // Add integration point contribution to the local mass matrix
        this->AddMomentumMassTerm(MassMatrix,N,GaussWeight*Density);

        // Add convection, stabilization and RHS contributions to the local system equation
        this->AddMomentumSystemTerms(rLeftHandSideMatrix,rRightHandSideVector,Density,UGradN,BodyForce,OldPressure,
                                     TauOne,TauTwo,MomentumProjection,MassProjection,N,rDN_DX,GaussWeight);

        // Add viscous term
        const double ViscousCoeff = Density * Viscosity * GaussWeight;
        this->AddViscousTerm(rLeftHandSideMatrix,rDN_DX,ViscousCoeff);
    }

    // Add residual of previous iteration to RHS
    VectorType LastValues = ZeroVector(LocalSize);
    this->GetVelocityValues(LastValues,0);
    noalias(rRightHandSideVector) -= prod(rLeftHandSideMatrix,LastValues);

    // Add dynamic term
    const Vector& rBDFCoeffs = rCurrentProcessInfo[BDF_COEFFICIENTS];
    noalias( rLeftHandSideMatrix ) += rBDFCoeffs[0] * MassMatrix;

    VectorType TimeTerm = rBDFCoeffs[0] * LastValues;
    for(SizeType i = 1; i < rBDFCoeffs.size(); i++)
    {
        this->GetVelocityValues(LastValues,i);
        noalias( TimeTerm ) += rBDFCoeffs[i] * LastValues;
    }

    noalias( rRightHandSideVector ) -= prod(MassMatrix,TimeTerm);
}

template< unsigned int TDim >
void FractionalStep<TDim>::CalculateLocalPressureSystem(MatrixType& rLeftHandSideMatrix,
                                                        VectorType& rRightHandSideVector,
                                                        ProcessInfo& rCurrentProcessInfo)
{
    GeometryType& rGeom = this->GetGeometry();
    const SizeType NumNodes = rGeom.PointsNumber();

    // Check sizes and initialize
    if( rLeftHandSideMatrix.size1() != NumNodes )
        rLeftHandSideMatrix.resize(NumNodes,NumNodes);

    rLeftHandSideMatrix = ZeroMatrix(NumNodes,NumNodes);

    if( rRightHandSideVector.size() != NumNodes )
        rRightHandSideVector.resize(NumNodes);

    rRightHandSideVector = ZeroVector(NumNodes);

    // Shape functions and integration points
    ShapeFunctionDerivativesArrayType DN_DX;
    Matrix NContainer;
    VectorType GaussWeights;
    this->CalculateGeometryData(DN_DX,NContainer,GaussWeights);
    const unsigned int NumGauss = GaussWeights.size();

    // Stabilization parameters
    double ElemSize = this->ElementSize();
    double TauOne;
    double TauTwo;

    // Loop on integration points
    for (unsigned int g = 0; g < NumGauss; g++)
    {
        const double GaussWeight = GaussWeights[g];
        const ShapeFunctionsType& N = row(NContainer,g);
        const ShapeFunctionDerivativesType& rDN_DX = DN_DX[g];

        // Evaluate required variables at the integration point
        double Density;
//        double Viscosity;
//        array_1d<double,3> Velocity(3,0.0);
//        array_1d<double,3> MeshVelocity(3,0.0);
        array_1d<double,3> BodyForce(3,0.0);
        array_1d<double,3> MomentumProjection(3,0.0);

        this->EvaluateInPoint(Density,DENSITY,N);
//        this->EvaluateInPoint(Viscosity,VISCOSITY,N);
//        this->EvaluateInPoint(Velocity,VELOCITY,N);
//        this->EvaluateInPoint(MeshVelocity,MESH_VELOCITY,N);
        this->EvaluateInPoint(BodyForce,BODY_FORCE,N);
//        this->EvaluateInPoint(MomentumProjection,ADVPROJ,N);
        this->EvaluateInPoint(MomentumProjection,PRESS_PROJ,N);

//        // Evaluate the pressure and pressure gradient at this point (for the G * P_n term)
//        double OldPressure;
//        this->EvaluateInPoint(OldPressure,PRESSURE,N,0);

        array_1d<double,TDim> OldPressureGradient(TDim,0.0);
        this->EvaluateGradientInPoint(OldPressureGradient,PRESSURE,rDN_DX);

//        // For ALE: convective velocity
//        array_1d<double,3> ConvVel = Velocity - MeshVelocity;

        // Stabilization parameters
        array_1d<double,3> ConvVel(3,0.0);
        this->EvaluateConvVelocity(ConvVel,N);
        double Viscosity = this->EffectiveViscosity(N,rDN_DX,ElemSize,rCurrentProcessInfo);
        this->CalculateTau(TauOne,TauTwo,ElemSize,ConvVel,Density,Viscosity,rCurrentProcessInfo);

//        // Evaluate convection operator Velocity * Grad(N)
//        Vector UGradN(NumNodes);
//        this->EvaluateConvection(UGradN,ConvVel,mDN_DX);

        double DivU;
        this->EvaluateDivergenceInPoint(DivU,VELOCITY,rDN_DX);

        // constant coefficient multiplying the pressure Laplacian (See Codina, Badia 2006 paper for details in case of a BDF2 time scheme)
        const double LaplacianCoeff = 1.0 / (Density * rCurrentProcessInfo[BDF_COEFFICIENTS][0]) ;

        // Add convection, stabilization and RHS contributions to the local system equation
        for (SizeType i = 0; i < NumNodes; ++i)
        {
            // LHS contribution
            for (SizeType j = 0; j < NumNodes; ++j)
            {
                double Lij = 0.0;
                for (SizeType d = 0; d < TDim; ++d)
                    Lij += rDN_DX(i,d) * rDN_DX(j,d);
                Lij *= (LaplacianCoeff + TauOne);

                rLeftHandSideMatrix(i,j) += GaussWeight * Lij;
            }

            // RHS contribution

            // Velocity divergence
            double RHSi = - N[i] * DivU;

            for (SizeType d = 0; d < TDim; ++d)
            {
//                double Conv = UGradN[0] * rGeom[0].FastGetSolutionStepValue(VELOCITY)[d];
//                for (SizeType j = 1; j < NumNodes; ++j) Conv += UGradN[i] * rGeom[i].FastGetSolutionStepValue(VELOCITY)[d];
                // Momentum stabilization
                RHSi += rDN_DX(i,d) * TauOne * ( Density  * ( BodyForce[d]/* - Conv*/ ) - OldPressureGradient[d] - MomentumProjection[d] );
            }

            rRightHandSideVector[i] += GaussWeight * RHSi;
        }
    }
}

template< unsigned int TDim >
int FractionalStep<TDim>::Check(const ProcessInfo &rCurrentProcessInfo)
{
    KRATOS_TRY;

    // Base class checks for positive Jacobian and Id > 0
    int ierr = Element::Check(rCurrentProcessInfo);
    if(ierr != 0) return ierr;

    // Check that all required variables have been registered
    if(VELOCITY.Key() == 0)
        KRATOS_ERROR(std::invalid_argument,"VELOCITY Key is 0. Check that the application was correctly registered.","");
    if(PRESSURE.Key() == 0)
        KRATOS_ERROR(std::invalid_argument,"PRESSURE Key is 0. Check that the application was correctly registered.","");
    if(BODY_FORCE.Key() == 0)
        KRATOS_ERROR(std::invalid_argument,"BODY_FORCE Key is 0. Check that the application was correctly registered.","");
    if(DENSITY.Key() == 0)
        KRATOS_ERROR(std::invalid_argument,"DENSITY Key is 0. Check that the application was correctly registered.","");
    if(VISCOSITY.Key() == 0)
        KRATOS_ERROR(std::invalid_argument,"VISCOSITY Key is 0. Check that the application was correctly registered.","");
    if(MESH_VELOCITY.Key() == 0)
        KRATOS_ERROR(std::invalid_argument,"MESH_VELOCITY Key is 0. Check that the application was correctly registered.","");
    if(FRACT_VEL.Key() == 0)
        KRATOS_ERROR(std::invalid_argument,"FRACT_VEL Key is 0. Check that the application was correctly registered.","");
    if(PRESSURE_OLD_IT.Key() == 0)
        KRATOS_ERROR(std::invalid_argument,"PRESSURE_OLD_IT Key is 0. Check that the application was correctly registered.","");
    if(NODAL_AREA.Key() == 0)
        KRATOS_ERROR(std::invalid_argument,"NODAL_AREA Key is 0. Check that the application was correctly registered.","");
    if(CONV_PROJ.Key() == 0)
        KRATOS_ERROR(std::invalid_argument,"CONV_PROJ Key is 0. Check that the application was correctly registered.","");
    if(PRESS_PROJ.Key() == 0)
        KRATOS_ERROR(std::invalid_argument,"PRESS_PROJ Key is 0. Check that the application was correctly registered.","");
    if(DIVPROJ.Key() == 0)
        KRATOS_ERROR(std::invalid_argument,"DIVPROJ Key is 0. Check that the application was correctly registered.","");
    if(BDF_COEFFICIENTS.Key() == 0)
        KRATOS_ERROR(std::invalid_argument,"BDF_COEFFICIENTS Key is 0. Check that the application was correctly registered.","");
    if(DELTA_TIME.Key() == 0)
        KRATOS_ERROR(std::invalid_argument,"DELTA_TIME Key is 0. Check that the application was correctly registered.","");
    if(DYNAMIC_TAU.Key() == 0)
        KRATOS_ERROR(std::invalid_argument,"DYNAMIC_TAU Key is 0. Check that the application was correctly registered.","");

    // Check that the element's nodes contain all required SolutionStepData and Degrees of freedom
    for(unsigned int i=0; i<this->GetGeometry().size(); ++i)
    {
        if(this->GetGeometry()[i].SolutionStepsDataHas(VELOCITY) == false)
            KRATOS_ERROR(std::invalid_argument,"missing VELOCITY variable on solution step data for node ",this->GetGeometry()[i].Id());
        if(this->GetGeometry()[i].SolutionStepsDataHas(PRESSURE) == false)
            KRATOS_ERROR(std::invalid_argument,"missing PRESSURE variable on solution step data for node ",this->GetGeometry()[i].Id());
        if(this->GetGeometry()[i].SolutionStepsDataHas(BODY_FORCE) == false)
            KRATOS_ERROR(std::invalid_argument,"missing BODY_FORCE variable on solution step data for node ",this->GetGeometry()[i].Id());
        if(this->GetGeometry()[i].SolutionStepsDataHas(DENSITY) == false)
            KRATOS_ERROR(std::invalid_argument,"missing DENSITY variable on solution step data for node ",this->GetGeometry()[i].Id());
        if(this->GetGeometry()[i].SolutionStepsDataHas(VISCOSITY) == false)
            KRATOS_ERROR(std::invalid_argument,"missing VISCOSITY variable on solution step data for node ",this->GetGeometry()[i].Id());
        if(this->GetGeometry()[i].SolutionStepsDataHas(MESH_VELOCITY) == false)
            KRATOS_ERROR(std::invalid_argument,"missing MESH_VELOCITY variable on solution step data for node ",this->GetGeometry()[i].Id());
        if(this->GetGeometry()[i].SolutionStepsDataHas(FRACT_VEL) == false)
            KRATOS_ERROR(std::invalid_argument,"missing FRACT_VEL variable on solution step data for node ",this->GetGeometry()[i].Id());
        if(this->GetGeometry()[i].SolutionStepsDataHas(PRESSURE_OLD_IT) == false)
            KRATOS_ERROR(std::invalid_argument,"missing PRESSURE_OLD_IT variable on solution step data for node ",this->GetGeometry()[i].Id());
        if(this->GetGeometry()[i].SolutionStepsDataHas(NODAL_AREA) == false)
            KRATOS_ERROR(std::invalid_argument,"missing NODAL_AREA variable on solution step data for node ",this->GetGeometry()[i].Id());
        if(this->GetGeometry()[i].SolutionStepsDataHas(CONV_PROJ) == false)
            KRATOS_ERROR(std::invalid_argument,"missing CONV_PROJ variable on solution step data for node ",this->GetGeometry()[i].Id());
        if(this->GetGeometry()[i].SolutionStepsDataHas(PRESS_PROJ) == false)
            KRATOS_ERROR(std::invalid_argument,"missing PRESS_PROJ variable on solution step data for node ",this->GetGeometry()[i].Id());
        if(this->GetGeometry()[i].SolutionStepsDataHas(DIVPROJ) == false)
            KRATOS_ERROR(std::invalid_argument,"missing DIVPROJ variable on solution step data for node ",this->GetGeometry()[i].Id());
        if(this->GetGeometry()[i].HasDofFor(VELOCITY_X) == false ||
           this->GetGeometry()[i].HasDofFor(VELOCITY_Y) == false ||
           this->GetGeometry()[i].HasDofFor(VELOCITY_Z) == false)
            KRATOS_ERROR(std::invalid_argument,"missing VELOCITY component degree of freedom on node ",this->GetGeometry()[i].Id());
        if(this->GetGeometry()[i].HasDofFor(PRESSURE) == false)
            KRATOS_ERROR(std::invalid_argument,"missing PRESSURE component degree of freedom on node ",this->GetGeometry()[i].Id());
    }
    
    // If this is a 2D problem, check that nodes are in XY plane
    if (this->GetGeometry().WorkingSpaceDimension() == 2)
    {
        for (unsigned int i=0; i<this->GetGeometry().size(); ++i)
        {
            if (this->GetGeometry()[i].Z() != 0.0)
                KRATOS_ERROR(std::invalid_argument,"Node with non-zero Z coordinate found. Id: ",this->GetGeometry()[i].Id());
        }
    }

    return ierr;

    KRATOS_CATCH("");
}

template<>
void FractionalStep<2>::VelocityEquationIdVector(EquationIdVectorType& rResult,
                                                 ProcessInfo& rCurrentProcessInfo)
{
    GeometryType& rGeom = this->GetGeometry();
    const SizeType NumNodes = rGeom.PointsNumber();
    const SizeType LocalSize = NumNodes*2;

    SizeType LocalIndex = 0;

    if (rResult.size() != LocalSize)
        rResult.resize(LocalSize, false);

    const unsigned int xpos = this->GetGeometry()[0].GetDofPosition(VELOCITY_X);

    for (SizeType i = 0; i < NumNodes; ++i)
    {
        rResult[LocalIndex++] = rGeom[i].GetDof(VELOCITY_X,xpos).EquationId();
        rResult[LocalIndex++] = rGeom[i].GetDof(VELOCITY_Y,xpos+1).EquationId();
    }
}

template<>
void FractionalStep<3>::VelocityEquationIdVector(EquationIdVectorType& rResult,
                                                 ProcessInfo& rCurrentProcessInfo)
{
    GeometryType& rGeom = this->GetGeometry();
    const SizeType NumNodes = rGeom.PointsNumber();
    const SizeType LocalSize = 3*NumNodes;

    SizeType LocalIndex = 0;

    if (rResult.size() != LocalSize)
        rResult.resize(LocalSize, false);

    const unsigned int xpos = this->GetGeometry()[0].GetDofPosition(VELOCITY_X);

    for (SizeType i = 0; i < NumNodes; ++i)
    {
        rResult[LocalIndex++] = rGeom[i].GetDof(VELOCITY_X,xpos).EquationId();
        rResult[LocalIndex++] = rGeom[i].GetDof(VELOCITY_Y,xpos+1).EquationId();
        rResult[LocalIndex++] = rGeom[i].GetDof(VELOCITY_Z,xpos+2).EquationId();
    }
}

template< unsigned int TDim >
void FractionalStep<TDim>::PressureEquationIdVector(EquationIdVectorType& rResult,
                                                    ProcessInfo& rCurrentProcessInfo)
{
    GeometryType& rGeom = this->GetGeometry();
    const SizeType NumNodes = rGeom.PointsNumber();

    if (rResult.size() != NumNodes)
        rResult.resize(NumNodes);

    const unsigned int pos = this->GetGeometry()[0].GetDofPosition(VELOCITY_X);

    for (SizeType i = 0; i < NumNodes; ++i)
        rResult[i] = rGeom[i].GetDof(PRESSURE,pos).EquationId();
}

template<>
void FractionalStep<2>::GetVelocityDofList(DofsVectorType& rElementalDofList,
                                           ProcessInfo& rCurrentProcessInfo)
{
    GeometryType& rGeom = this->GetGeometry();
    const SizeType NumNodes = rGeom.PointsNumber();
    const SizeType LocalSize = 2*NumNodes;

    if (rElementalDofList.size() != LocalSize)
        rElementalDofList.resize(LocalSize);

    SizeType LocalIndex = 0;

    for (SizeType i = 0; i < NumNodes; ++i)
    {
        rElementalDofList[LocalIndex++] = rGeom[i].pGetDof(VELOCITY_X);
        rElementalDofList[LocalIndex++] = rGeom[i].pGetDof(VELOCITY_Y);
    }
}

template<>
void FractionalStep<3>::GetVelocityDofList(DofsVectorType& rElementalDofList,
                                           ProcessInfo& rCurrentProcessInfo)
{
    GeometryType& rGeom = this->GetGeometry();
    const SizeType NumNodes = rGeom.PointsNumber();
    const SizeType LocalSize = 3*NumNodes;

    if (rElementalDofList.size() != LocalSize)
        rElementalDofList.resize(LocalSize);

    SizeType LocalIndex = 0;

    for (SizeType i = 0; i < NumNodes; ++i)
    {
        rElementalDofList[LocalIndex++] = rGeom[i].pGetDof(VELOCITY_X);
        rElementalDofList[LocalIndex++] = rGeom[i].pGetDof(VELOCITY_Y);
        rElementalDofList[LocalIndex++] = rGeom[i].pGetDof(VELOCITY_Z);
    }
}

template< unsigned int TDim >
void FractionalStep<TDim>::GetPressureDofList(DofsVectorType& rElementalDofList,
                                              ProcessInfo& rCurrentProcessInfo)
{
    GeometryType& rGeom = this->GetGeometry();
    const SizeType NumNodes = rGeom.PointsNumber();

    if (rElementalDofList.size() != NumNodes)
        rElementalDofList.resize(NumNodes);

    SizeType LocalIndex = 0;

    for (SizeType i = 0; i < NumNodes; ++i)
    {
        rElementalDofList[LocalIndex++] = rGeom[i].pGetDof(PRESSURE);
    }
}

template< unsigned int TDim >
void FractionalStep<TDim>::GetPressureValues(Vector& rValues,
                                            const int Step)
{
    GeometryType& rGeom = this->GetGeometry();
    const SizeType NumNodes = rGeom.PointsNumber();

    if (rValues.size() != NumNodes) rValues.resize(NumNodes);

    for (SizeType i = 0; i < NumNodes; ++i)
        rValues[i] = rGeom[i].FastGetSolutionStepValue(PRESSURE,Step);
}

template<>
void FractionalStep<2>::GetVelocityValues(Vector& rValues,
                                          const int Step)
{
    GeometryType& rGeom = this->GetGeometry();
    const SizeType NumNodes = rGeom.PointsNumber();
    const SizeType LocalSize = 2*NumNodes;

    if (rValues.size() != LocalSize) rValues.resize(LocalSize);

    SizeType Index = 0;

    for (SizeType i = 0; i < NumNodes; ++i)
    {
        rValues[Index++] = rGeom[i].FastGetSolutionStepValue(VELOCITY_X,Step);
        rValues[Index++] = rGeom[i].FastGetSolutionStepValue(VELOCITY_Y,Step);
    }
}

template<>
void FractionalStep<3>::GetVelocityValues(Vector& rValues,
                                          const int Step)
{
    GeometryType& rGeom = this->GetGeometry();
    const SizeType NumNodes = rGeom.PointsNumber();
    const SizeType LocalSize = 3*NumNodes;

    if (rValues.size() != LocalSize) rValues.resize(LocalSize);

    SizeType Index = 0;

    for (SizeType i = 0; i < NumNodes; ++i)
    {
        rValues[Index++] = rGeom[i].FastGetSolutionStepValue(VELOCITY_X,Step);
        rValues[Index++] = rGeom[i].FastGetSolutionStepValue(VELOCITY_Y,Step);
        rValues[Index++] = rGeom[i].FastGetSolutionStepValue(VELOCITY_Z,Step);
    }
}


/*
 * protected FractionalStep<TDim> functions
 */

template< unsigned int TDim >
void FractionalStep<TDim>::CalculateGeometryData(ShapeFunctionDerivativesArrayType &rDN_DX,
                                                 Matrix &NContainer,
                                                 Vector &rGaussWeights)
{
    const GeometryType& rGeom = this->GetGeometry();
    Vector DetJ;
    rGeom.ShapeFunctionsIntegrationPointsGradients(rDN_DX,DetJ,GeometryData::GI_GAUSS_2);
    NContainer = rGeom.ShapeFunctionsValues(GeometryData::GI_GAUSS_2);
    const GeometryType::IntegrationPointsArrayType& IntegrationPoints = rGeom.IntegrationPoints(GeometryData::GI_GAUSS_2);

    rGaussWeights.resize(rGeom.IntegrationPointsNumber(GeometryData::GI_GAUSS_2),false);

    for (unsigned int g = 0; g < rGeom.IntegrationPointsNumber(GeometryData::GI_GAUSS_2); g++)
        rGaussWeights[g] = DetJ[g] * IntegrationPoints[g].Weight();

    
    /*
    const GeometryType& rGeom = this->GetGeometry();
    const SizeType NumNodes = rGeom.PointsNumber();
    const unsigned int NumGauss = rGeom.IntegrationPointsNumber(GeometryData::GI_GAUSS_2);

    // Initialize arrays to proper size
    rDN_DX.resize(NumGauss);
    rDetJ.resize(NumGauss);

    const GeometryType::ShapeFunctionsGradientsType& DN_De = rGeom.ShapeFunctionsLocalGradients( GeometryData::GI_GAUSS_2 );

    // Temporary container for inverse of J
    Matrix InvJ;

    GeometryType::JacobiansType J;
    rGeom.Jacobian( J, GeometryData::GI_GAUSS_2 );

    for (unsigned int g = 0; g < NumGauss; g++)
    {
        // calculate inverse of the jacobian and its determinant
        MathUtils<double>::InvertMatrix( J[g], InvJ, rDetJ[g] );

        // calculate the shape function derivatives in global coordinates
        rDN_DX[g].resize(NumNodes,TDim);
        noalias( rDN_DX[g] ) = prod( DN_De[g], InvJ );
    }*/
}

template< unsigned int TDim >
double FractionalStep<TDim>::ElementSize(/*ShapeFunctionDerivativesType &rDN_DX*/)
{
    const GeometryType& rGeom = this->GetGeometry();
    const SizeType NumNodes = rGeom.PointsNumber();

    // calculate minimum element length (used in stabilization Tau)
    array_1d<double,3> Edge(3,0.0);
    Edge = rGeom[1].Coordinates() - rGeom[0].Coordinates();
    double ElemSize = Edge[0]*Edge[0];
    for (SizeType d = 1; d < TDim; d++)
        ElemSize += Edge[d]*Edge[d];

    for (SizeType i = 2; i < NumNodes; i++)
        for(SizeType j = 0; j < i; j++)
        {
            Edge = rGeom[i].Coordinates() - rGeom[j].Coordinates();
            double Length = Edge[0]*Edge[0];
            for (SizeType d = 1; d < TDim; d++)
                Length += Edge[d]*Edge[d];
            if (Length < ElemSize) ElemSize = Length;
        }
    return sqrt(ElemSize);

//    // calculate minimum element height (for stabilization and Smagorinsky)
//    mElemSize = 0.0;
//    for ( unsigned int d = 0; d < TDim; ++d)
//    {
//        double hd = 1.0 / mDN_DX(1,d);
//        mElemSize += hd * hd;
//    }

//    for (unsigned int i = 1; i < NumNodes; ++i)
//    {
//        double Height = 0.0;
//        for ( unsigned int d = 0; d < TDim; ++d)
//        {
//            double hd = 1.0 / mDN_DX(i,d);
//            Height += hd * hd;
//        }
//        if (Height < mElemSize) mElemSize = Height;
//    }

//    mElemSize = sqrt(mElemSize);
}

template< unsigned int TDim >
double FractionalStep<TDim>::EffectiveViscosity(const ShapeFunctionsType &rN,
                                                const ShapeFunctionDerivativesType &rDN_DX,
                                                double ElemSize,
                                                const ProcessInfo &rCurrentProcessInfo)
{
    double Csmag = this->GetValue(C_SMAGORINSKY);

    double Viscosity = 0.0;
    this->EvaluateInPoint(Viscosity,VISCOSITY,rN);

    if (Csmag != 0.0 )
    {
        const unsigned int NumNodes = this->GetGeometry().PointsNumber();

        // Calculate Symetric gradient
        MatrixType S = ZeroMatrix(TDim,TDim);
        for (unsigned int n = 0; n < NumNodes; ++n)
        {
            const array_1d<double,3>& rVel = this->GetGeometry()[n].FastGetSolutionStepValue(VELOCITY);
            for (unsigned int i = 0; i < TDim; ++i)
                for (unsigned int j = 0; j < TDim; ++j)
                    S(i,j) += 0.5 * ( rDN_DX(n,j) * rVel[i] + rDN_DX(n,i) * rVel[j] );
        }

        // Norm of symetric gradient
        double NormS = 0.0;
        for (unsigned int i = 0; i < TDim; ++i)
            for (unsigned int j = 0; j < TDim; ++j)
                NormS += S(i,j) * S(i,j);
        NormS = sqrt(2.0*NormS);

        // Nu_sgs = (Csmag * Delta)^2 * (2*Sij*Sij)^(1/2)
        Viscosity += Csmag * Csmag * ElemSize * ElemSize * NormS;
    }

    return Viscosity;
}

template< unsigned int TDim >
void FractionalStep<TDim>::AddMomentumMassTerm(Matrix& rMassMatrix,
                                               const ShapeFunctionsType& rN,
                                               const double Weight)
{
    const SizeType NumNodes = this->GetGeometry().PointsNumber();

    IndexType FirstRow = 0;
    IndexType FirstCol = 0;

    for (SizeType i = 0; i < NumNodes; ++i)
    {
        for (SizeType j = 0; j < NumNodes; ++j)
        {
            const double Mij = Weight * rN[i] * rN[j];
            for (SizeType d =  0; d < TDim; ++d)
                    rMassMatrix(FirstRow+d,FirstCol+d) += Mij;
            FirstCol += TDim;
        }
        FirstRow += TDim;
        FirstCol = 0;
    }
}

template< unsigned int TDim >
void FractionalStep<TDim>::AddMomentumSystemTerms(Matrix& rLHSMatrix,
                                                  Vector& rRHSVector,
                                                  const double Density,
                                                  const Vector& rConvOperator,
                                                  const array_1d<double,3>& rBodyForce,
                                                  const double OldPressure,
                                                  const double TauOne,
                                                  const double TauTwo,
                                                  const array_1d<double,3>& rMomentumProjection,
                                                  const double MassProjection,
                                                  const ShapeFunctionsType& rN,
                                                  const ShapeFunctionDerivativesType& rDN_DX,
                                                  const double Weight)
{
    const SizeType NumNodes = this->GetGeometry().PointsNumber();

    SizeType FirstRow = 0;
    SizeType FirstCol = 0;

    for (SizeType i = 0; i < NumNodes; ++i)
    {

        // Build RHS
        for (SizeType d = 0; d < TDim; ++d)
        {
            // Body force
            double RHSi = Density * rN[i] * rBodyForce[d];
            // Pressure gradient (integrated by parts)
            RHSi += rDN_DX(i,d) * OldPressure;
            // Momentum Stabilization
            RHSi += Density * rConvOperator[i] * TauOne * ( /*rBodyForce[d] + rOldPressureGradient[d]*/ - rMomentumProjection[d] );
            // Mass Stabilization
            RHSi -= rDN_DX(i,d) * TauTwo * MassProjection;

            rRHSVector[FirstRow+d] += Weight * RHSi;
        }

        // Build LHS
        for (SizeType j = 0; j < NumNodes; ++j)
        {
            // Convective term
            double Kij = Density * rN[i] * rConvOperator[j];

            // Streamline stabilization
            Kij += Density * rConvOperator[i] * TauOne * Density * rConvOperator[j];

            Kij *= Weight;

            for (SizeType d = 0; d < TDim; ++d)
                rLHSMatrix(FirstRow + d,FirstCol + d) += Kij;

            // Mass-GLS (TauTwo) stabiliziation term
            for (SizeType m = 0; m < TDim; ++m)
                for (SizeType n = 0; n < TDim; ++n)
                    rLHSMatrix(FirstRow+m,FirstCol+n) += Weight * rDN_DX(i,m) * TauTwo * rDN_DX(j,n);

            FirstCol += TDim;
        }
        FirstRow += TDim;
        FirstCol = 0;
    }
}

template<>
void FractionalStep<2>::AddViscousTerm(MatrixType& rDampMatrix,
                                       const ShapeFunctionDerivativesType& rShapeDeriv,
                                       const double Weight)
{
    const SizeType NumNodes = this->GetGeometry().PointsNumber();

    const double FourThirds = 4.0 / 3.0;
    const double nTwoThirds = -2.0 / 3.0;

    SizeType FirstRow(0),FirstCol(0);

    for (SizeType j = 0; j < NumNodes; ++j)
    {
        for (SizeType i = 0; i < NumNodes; ++i)
        {
            // First Row
            rDampMatrix(FirstRow,FirstCol) += Weight * ( FourThirds * rShapeDeriv(i,0) * rShapeDeriv(j,0) + rShapeDeriv(i,1) * rShapeDeriv(j,1) );
            rDampMatrix(FirstRow,FirstCol+1) += Weight * ( nTwoThirds * rShapeDeriv(i,0) * rShapeDeriv(j,1) + rShapeDeriv(i,1) * rShapeDeriv(j,0) );

            // Second Row
            rDampMatrix(FirstRow+1,FirstCol) += Weight * ( nTwoThirds * rShapeDeriv(i,1) * rShapeDeriv(j,0) + rShapeDeriv(i,0) * rShapeDeriv(j,1) );
            rDampMatrix(FirstRow+1,FirstCol+1) += Weight * ( FourThirds * rShapeDeriv(i,1) * rShapeDeriv(j,1) + rShapeDeriv(i,0) * rShapeDeriv(j,0) );

            // Update Counter
            FirstRow += 2;
        }
        FirstRow = 0;
        FirstCol += 2;
    }
}

template <>
void FractionalStep<3>::AddViscousTerm(MatrixType& rDampMatrix,
                                       const ShapeFunctionDerivativesType& rShapeDeriv,
                                       const double Weight)
{
    const SizeType NumNodes = this->GetGeometry().PointsNumber();

    const double OneThird = 1.0 / 3.0;
    const double nTwoThirds = -2.0 / 3.0;

    unsigned int FirstRow(0),FirstCol(0);

    for (SizeType j = 0; j < NumNodes; ++j)
    {
        for (SizeType i = 0; i < NumNodes; ++i)
        {
            // (dN_i/dx_k dN_j/dx_k)
            const double Diag =  rShapeDeriv(i,0) * rShapeDeriv(j,0) + rShapeDeriv(i,1) * rShapeDeriv(j,1) + rShapeDeriv(i,2) * rShapeDeriv(j,2);

            // First Row
            rDampMatrix(FirstRow,FirstCol) += Weight * ( OneThird * rShapeDeriv(i,0) * rShapeDeriv(j,0) + Diag );
            rDampMatrix(FirstRow,FirstCol+1) += Weight * ( nTwoThirds * rShapeDeriv(i,0) * rShapeDeriv(j,1) + rShapeDeriv(i,1) * rShapeDeriv(j,0) );
            rDampMatrix(FirstRow,FirstCol+2) += Weight * ( nTwoThirds * rShapeDeriv(i,0) * rShapeDeriv(j,2) + rShapeDeriv(i,2) * rShapeDeriv(j,0) );

            // Second Row
            rDampMatrix(FirstRow+1,FirstCol) += Weight * ( nTwoThirds * rShapeDeriv(i,1) * rShapeDeriv(j,0) + rShapeDeriv(i,0) * rShapeDeriv(j,1) );
            rDampMatrix(FirstRow+1,FirstCol+1) += Weight * ( OneThird * rShapeDeriv(i,1) * rShapeDeriv(j,1) + Diag );
            rDampMatrix(FirstRow+1,FirstCol+2) += Weight * ( nTwoThirds * rShapeDeriv(i,1) * rShapeDeriv(j,2) + rShapeDeriv(i,2) * rShapeDeriv(j,1) );

            // Third Row
            rDampMatrix(FirstRow+2,FirstCol) += Weight * ( nTwoThirds * rShapeDeriv(i,2) * rShapeDeriv(j,0) + rShapeDeriv(i,0) * rShapeDeriv(j,2) );
            rDampMatrix(FirstRow+2,FirstCol+1) += Weight * ( nTwoThirds * rShapeDeriv(i,2) * rShapeDeriv(j,1) + rShapeDeriv(i,1) * rShapeDeriv(j,2) );
            rDampMatrix(FirstRow+2,FirstCol+2) += Weight * ( OneThird * rShapeDeriv(i,2) * rShapeDeriv(j,2) + Diag );

            // Update Counter
            FirstRow += 3;
        }
        FirstRow = 0;
        FirstCol += 3;
    }
}

//template <unsigned int TDim>
//void FractionalStep<TDim>::CalculateTau(double &TauOne,
//                                        double &TauTwo,
//                                        double ElemSize,
//                                        const ProcessInfo &rCurrentProcessInfo)
//{
//    const ShapeFunctionsType& N = row( this->GetGeometry().ShapeFunctionsValues(GeometryData::GI_GAUSS_1), 0);

//    double Density = 0.0;
//    double Viscosity = this->GetValue(VISCOSITY);
//    array_1d<double,3> ConvVelocity(3,0.0);

//    this->EvaluateInPoint(Density,DENSITY,N);
//    this->EvaluateConvVelocity(ConvVelocity,N);

//    const double DeltaTime = rCurrentProcessInfo.GetValue(DELTA_TIME);
//    const double TimeFactor = rCurrentProcessInfo.GetValue(DYNAMIC_TAU);

//    // Compute mean advective velocity norm
//    double AdvVelNorm = 0.0;
//    for (unsigned int d = 0; d < TDim; ++d)
//        AdvVelNorm += ConvVelocity[d] * ConvVelocity[d];

//    AdvVelNorm = sqrt(AdvVelNorm);

//    TauOne = 1.0 / (Density * ( TimeFactor / DeltaTime + 4.0 * Viscosity / (ElemSize * ElemSize) + 2.0 * AdvVelNorm / ElemSize) );
//    TauTwo = Density * (Viscosity + 0.5 * ElemSize * AdvVelNorm);
//}

template <unsigned int TDim>
void FractionalStep<TDim>::CalculateTau(double& TauOne,
                                        double& TauTwo,
                                        double ElemSize,
                                        const array_1d< double, 3 > & rAdvVel,
                                        const double Density,
                                        const double Viscosity,
                                        const ProcessInfo& rCurrentProcessInfo)
{
    const double DeltaTime = rCurrentProcessInfo.GetValue(DELTA_TIME);
    const double TimeFactor = rCurrentProcessInfo.GetValue(DYNAMIC_TAU);

    // Compute mean advective velocity norm
    double AdvVelNorm = 0.0;
    for (unsigned int d = 0; d < TDim; ++d)
        AdvVelNorm += rAdvVel[d] * rAdvVel[d];

    AdvVelNorm = sqrt(AdvVelNorm);

    TauOne = 1.0 / (Density * ( TimeFactor / DeltaTime + 4.0 * Viscosity / (ElemSize * ElemSize) + 2.0 * AdvVelNorm / ElemSize) );
    TauTwo = Density * (Viscosity + 0.5 * ElemSize * AdvVelNorm);

}


template <unsigned int TDim>
void FractionalStep<TDim>::CalculateProjectionRHS(VectorType& rMomentumRHS,
                                                  VectorType& rMassRHS,
                                                  const ShapeFunctionsType& rN,
                                                  const ShapeFunctionDerivativesType& rDN_DX,
                                                  const double Weight)
{
    const SizeType NumNodes = this->GetGeometry().PointsNumber();
    double Density;
    array_1d<double,3> BodyForce(3,0.0);

    this->EvaluateInPoint(Density,DENSITY,rN);
    this->EvaluateInPoint(BodyForce,BODY_FORCE,rN);
    array_1d<double,3> ConvVel(3,0.0);
    this->EvaluateConvVelocity(ConvVel,rN);
    Vector ConvOp(NumNodes);
    this->ConvectionOperator(ConvOp,ConvVel,rDN_DX);

    array_1d<double,3> Convection(3,0.0);
    for (SizeType i = 0; i < NumNodes; ++i)
        for (SizeType d = 0; d < TDim; ++d)
            Convection[d] += ConvOp[i] * this->GetGeometry()[i].FastGetSolutionStepValue(VELOCITY)[d];

    array_1d<double,TDim> PressureGradient(TDim,0.0);
    this->EvaluateGradientInPoint(PressureGradient,PRESSURE,rDN_DX);

    double Divergence;
    this->EvaluateDivergenceInPoint(Divergence,VELOCITY,rDN_DX);

    int RowIndex = 0;
    for (unsigned int j = 0; j < NumNodes; ++j)
    {
        for (unsigned int d = 0; d < TDim; ++d)
        {
            rMomentumRHS[RowIndex++] += Weight * rN[j]*( Density * ( BodyForce[d] - Convection[d]) - PressureGradient[d] );
        }
        rMassRHS[j] -= Weight * rN[j] * Divergence;
    }
}


template <unsigned int TDim>
void FractionalStep<TDim>::CalculateProjectionRHS(VectorType& rConvTerm,
                                                  VectorType& rPresTerm,
                                                  VectorType& rDivTerm,
                                                  const ShapeFunctionsType& rN,
                                                  const ShapeFunctionDerivativesType& rDN_DX,
                                                  const double Weight)
{
    const unsigned int NumNodes = this->GetGeometry().PointsNumber();
    double Density;
    array_1d<double,3> BodyForce(3,0.0);

    this->EvaluateInPoint(Density,DENSITY,rN);
    this->EvaluateInPoint(BodyForce,BODY_FORCE,rN);

    array_1d<double,3> ConvVel(3,0.0);
    this->EvaluateConvVelocity(ConvVel,rN);

    Vector ConvOp = ZeroVector(NumNodes);
    this->ConvectionOperator(ConvOp,ConvVel,rDN_DX);

    array_1d<double,3> Convection(3,0.0);
    for (unsigned int i = 0; i < NumNodes; ++i)
        for (unsigned int d = 0; d < TDim; ++d)
            Convection[d] += ConvOp[i] * this->GetGeometry()[i].FastGetSolutionStepValue(VELOCITY)[d];

    array_1d<double,TDim> PressureGradient(TDim,0.0);
    this->EvaluateGradientInPoint(PressureGradient,PRESSURE,rDN_DX);

    double Divergence;
    this->EvaluateDivergenceInPoint(Divergence,VELOCITY,rDN_DX);

    unsigned int RowIndex = 0;

    for (unsigned int j = 0; j < NumNodes; ++j)
    {
        for (unsigned int d = 0; d < TDim; ++d)
        {
            rPresTerm[RowIndex] += Weight * rN[j] * ( Density * BodyForce[d] - PressureGradient[d] );
            rConvTerm[RowIndex] -= Weight * rN[j] * Density * Convection[d];
            ++RowIndex;
        }
        rDivTerm[j] -= Weight * rN[j] * Divergence;
    }
}

template< unsigned int TDim >
void FractionalStep<TDim>::ModulatedGradientDiffusion(MatrixType& rDampMatrix,
                                                      const ShapeFunctionDerivativesType& rDN_DX,
                                                      const double Weight)
{
    const GeometryType& rGeom = this->GetGeometry();
    const unsigned int NumNodes = rGeom.PointsNumber();

    // Velocity gradient
    MatrixType GradU = ZeroMatrix(TDim,TDim);
    for (unsigned int n = 0; n < NumNodes; n++)
    {
        const array_1d<double,3>& rVel = this->GetGeometry()[n].FastGetSolutionStepValue(VELOCITY);
        for (unsigned int i = 0; i < TDim; i++)
            for (unsigned int j = 0; j < TDim; j++)
                GradU(i,j) += rDN_DX(n,j)*rVel[i];
    }

    // Element lengths
    array_1d<double,3> Delta(3,0.0);
    Delta[0] = abs(rGeom[NumNodes-1].X()-rGeom[0].X());
    Delta[1] = abs(rGeom[NumNodes-1].Y()-rGeom[0].Y());
    Delta[2] = abs(rGeom[NumNodes-1].Z()-rGeom[0].Z());

    for (unsigned int n = 1; n < NumNodes; n++)
    {
        double hx = abs(rGeom[n].X()-rGeom[n-1].X());
        if (hx > Delta[0]) Delta[0] = hx;
        double hy = abs(rGeom[n].Y()-rGeom[n-1].Y());
        if (hy > Delta[1]) Delta[1] = hy;
        double hz = abs(rGeom[n].Z()-rGeom[n-1].Z());
        if (hz > Delta[2]) Delta[2] = hz;
    }

    double AvgDeltaSq = Delta[0];
    for (unsigned int d = 1; d < TDim; d++)
        AvgDeltaSq *= Delta[d];
    AvgDeltaSq = std::pow(AvgDeltaSq,2./TDim);

    Delta[0] = Delta[0]*Delta[0]/12.0;
    Delta[1] = Delta[1]*Delta[1]/12.0;
    Delta[2] = Delta[2]*Delta[2]/12.0;

    // Gij
    MatrixType G = ZeroMatrix(TDim,TDim);
    for (unsigned int i = 0; i < TDim; i++)
        for (unsigned int j = 0; j < TDim; j++)
            for (unsigned int d = 0; d < TDim; d++)
                G(i,j) += Delta[d]*GradU(i,d)*GradU(j,d);

    // Gij:Sij
    double GijSij = 0.0;
    for (unsigned int i = 0; i < TDim; i++)
        for (unsigned int j = 0; j < TDim; j++)
            GijSij += 0.5*G(i,j)*( GradU(i,j) + GradU(j,i) );

    if (GijSij < 0.0) // Otherwise model term is clipped
    {
        // Gkk
        double Gkk = G(0,0);
        for (unsigned int d = 1; d < TDim; d++)
            Gkk += G(d,d);

        // C_epsilon
        const double Ce = 1.0;
        
        // ksgs
        double ksgs = -4*AvgDeltaSq*GijSij/(Ce*Ce*Gkk);

        // Assembly of model term
        unsigned int RowIndex = 0;
        unsigned int ColIndex = 0;

        for (unsigned int i = 0; i < NumNodes; i++)
        {
            for (unsigned int j = 0; j < NumNodes; j++)
            {
                for (unsigned int d = 0; d < TDim; d++)
                {
                    double Aux = rDN_DX(i,d) * Delta[0] * G(d,0)*rDN_DX(j,0);
                    for (unsigned int k = 1; k < TDim; k++)
                        Aux += rDN_DX(i,d) *Delta[k] * G(d,k)*rDN_DX(j,k);
                    rDampMatrix(RowIndex+d,ColIndex+d) += Weight * 2.0*ksgs *  Aux;
                }

                ColIndex += TDim;
            }
            RowIndex += TDim;
            ColIndex = 0;
        }
    }

}

/*
 * private FractionalStep<TDim> functions
 */

template< unsigned int TDim >
void FractionalStep<TDim>::ConvectionOperator(Vector& rResult,
                                              const array_1d<double,3>& rConvVel,
                                              const ShapeFunctionDerivativesType& DN_DX)
{
    const SizeType NumNodes = this->GetGeometry().PointsNumber();

    if(rResult.size() != NumNodes) rResult.resize(NumNodes);

    for (SizeType i = 0; i < NumNodes; i++)
    {
        rResult[i] = rConvVel[0]*DN_DX(i,0);
        for(SizeType k = 1; k < TDim; k++)
            rResult[i] += rConvVel[k]*DN_DX(i,k);
    }
}

template< unsigned int TDim >
void FractionalStep<TDim>::EvaluateConvVelocity(array_1d<double,3> &rConvVel, const ShapeFunctionsType &N)
{
    GeometryType& rGeom = this->GetGeometry();
    const unsigned int NumNodes = rGeom.PointsNumber();

    array_1d<double,3> NodeVel = rGeom[0].FastGetSolutionStepValue(VELOCITY);
    NodeVel -= rGeom[0].FastGetSolutionStepValue(MESH_VELOCITY);
    rConvVel = N[0] * NodeVel;

    for (unsigned int i = 1; i < NumNodes; i++)
    {
        NodeVel = rGeom[i].FastGetSolutionStepValue(VELOCITY);
        NodeVel -= rGeom[i].FastGetSolutionStepValue(MESH_VELOCITY);
        rConvVel += N[i] * NodeVel;
    }
}

//template<>
//FractionalStep<2>::ShapeFunctionsContainerType FractionalStep<2>::InitializeShapeFunctions()
//{
//    const double TwoThirds = 2.0 / 3.0;
//    const double OneSixth = 1.0 / 6.0;
//    ShapeFunctionsContainerType NContainer(3);
//    ShapeFunctionsType Temp(3,0.0);

//    // First Gauss point
//    Temp[0] = TwoThirds;
//    Temp[1] = OneSixth;
//    Temp[2] = OneSixth;
//    NContainer[0] = Temp;

//    // Second Gauss point
//    Temp[0] = OneSixth;
//    Temp[1] = TwoThirds;
//    Temp[2] = OneSixth;
//    NContainer[1] = Temp;

//    // Third Gauss point
//    Temp[0] = OneSixth;
//    Temp[1] = OneSixth;
//    Temp[2] = TwoThirds;
//    NContainer[2] = Temp;

//    return NContainer;
//}

///*
// * FractionalStep<TDim> static members
// */

//template< unsigned int TDim >
//static const FractionalStep<TDim>::ShapeFunctionsType FractionalStep<TDim>::msNg = FractionalStep<TDim>::InitializeShapeFunctions();

/*
 * Template class definition (this should allow us to compile the desired template instantiations)
 */

template class FractionalStep<2>;
template class FractionalStep<3>;

}
