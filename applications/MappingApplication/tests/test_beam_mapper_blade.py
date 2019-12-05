from __future__ import print_function, absolute_import, division
import KratosMultiphysics as KM

import KratosMultiphysics.MappingApplication as KratosMapping
import KratosMultiphysics.KratosUnittest as KratosUnittest

import numpy as np
import math

#mdpa_file_name_beam    = "mdpa_files/NREL_Phase_6_Blade_Different_Meshes_2/NREL_line"
#mdpa_file_name_surface = "mdpa_files/NREL_Phase_6_Blade_Different_Meshes_2/NREL_Phase_6_Blade_Tri_005"

mdpa_file_name_beam    = "mdpa_files/blade_line"
mdpa_file_name_surface = "mdpa_files/blade_quad"

def WriteGiDOutput(model_part):
    from gid_output_process import GiDOutputProcess
    gid_output = GiDOutputProcess(model_part,
        "gid_output_"+model_part.Name,
        KM.Parameters("""
            {
                "result_file_configuration" : {
                    "gidpost_flags": {
                        "GiDPostMode"           : "GiD_PostAscii",
                        "WriteDeformedMeshFlag" : "WriteUndeformed",
                        "WriteConditionsFlag"   : "WriteConditions",
                        "MultiFileFlag"         : "SingleFile"
                    },
                    "nodal_results"       : ["DISPLACEMENT"],
                    "gauss_point_results" : []
                }
            }
            """)
        )

    gid_output.ExecuteInitialize()
    gid_output.ExecuteBeforeSolutionLoop()
    gid_output.ExecuteInitializeSolutionStep()
    gid_output.PrintOutput()
    gid_output.ExecuteFinalizeSolutionStep()
    gid_output.ExecuteFinalize()

def WriteVtkOutput(model_part):
    default_parameters = KM.Parameters("""{
        "file_format"                        : "binary",
        "output_precision"                   : 7,
        "output_control_type"                : "step",
        "output_sub_model_parts"             : false,
        "save_output_files_in_folder"        : false,
        "nodal_solution_step_data_variables" : ["DISPLACEMENT"]
    }""")

    vtk_io = KM.VtkOutput(model_part, default_parameters)
    vtk_io.PrintOutput()

def CalculateRotationMatrixWithAngle( Axis, Angle ):
    rotationMatrix = np.zeros((3, 3))

    rotationMatrix[0][0] = math.cos( Angle ) + Axis[0]**2 * (1 - math.cos( Angle ))
    rotationMatrix[0][1] = Axis[0] * Axis[1] * (1 - math.cos( Angle )) - Axis[2] * math.sin( Angle )
    rotationMatrix[0][2] = Axis[0] * Axis[2] * (1 - math.cos( Angle )) + Axis[1] * math.sin( Angle )

    rotationMatrix[1][0] = Axis[0] * Axis[1] * (1 - math.cos( Angle )) + Axis[2] * math.sin( Angle )
    rotationMatrix[1][1] = math.cos( Angle ) + Axis[1]**2 * (1 - math.cos( Angle ))
    rotationMatrix[1][2] = Axis[1] * Axis[2] * (1 - math.cos( Angle )) - Axis[0] * math.sin( Angle )

    rotationMatrix[2][0] = Axis[0] * Axis[2] * (1 - math.cos( Angle )) - Axis[1] * math.sin( Angle )
    rotationMatrix[2][1] = Axis[1] * Axis[2] * (1 - math.cos( Angle )) + Axis[0] * math.sin( Angle )
    rotationMatrix[2][2] = math.cos( Angle ) + Axis[2]**2 * (1 - math.cos( Angle ))

    return rotationMatrix

def getRotationVector(rotationMatrix):
    # see Non-linear Modeling and Analysis of Solids and Structures (Steen Krenk 2009) P52
    rotationVector = np.array([0.0, 0.0, 0.0])
    angle = rotationMatrix[0][0] + rotationMatrix[1][1] + rotationMatrix[2][2] - 1.0

    angle = angle/2.0
    if (angle > 1.0):
        angle = 1.0
    elif (angle < -1.0):
        angle = -1.0

    angle = math.acos(angle) # between 0 and pi

    EPS = 1E-6
    M_PI = math.pi
    if (angle < EPS):
        rotationVector[0] = 0.0
        rotationVector[1] = 0.0
        rotationVector[2] = 0.0
        return rotationVector
    elif ((M_PI - angle) < EPS):
        product11 = (rotationMatrix[0][0] + 1.0) / 2.0
        product22 = (rotationMatrix[1][1] + 1.0) / 2.0
        product33 = (rotationMatrix[2][2] + 1.0) / 2.0
        product12 = (rotationMatrix[0][1] + 1.0) / 2.0
        product23 = (rotationMatrix[1][2] + 1.0) / 2.0
        product13 = (rotationMatrix[0][2] + 1.0) / 2.0
        tmp1 = math.sqrt(product11)
        tmp2 = math.sqrt(product22)
        tmp3 = math.sqrt(product33)

        rotationVector[0] = tmp1
        rotationVector[1] = tmp2
        rotationVector[2] = tmp3
        tmp12 = rotationVector[0] * rotationVector[1]
        tmp13 = rotationVector[0] * rotationVector[2]
        tmp23 = rotationVector[1] * rotationVector[2]
        if (math.fabs(tmp12) < EPS or math.fabs(tmp12 - product12) < math.fabs(tmp12 + product12)):
            if (math.fabs(tmp13) < EPS or math.fabs(tmp13 - product13) < math.fabs(tmp13 + product13)):
                if (math.fabs(tmp23) < EPS or math.fabs(tmp23 - product23) < math.fabs(tmp23 + product23)):
                    rotationVector[0] *= M_PI
                    rotationVector[1] *= M_PI
                    rotationVector[2] *= M_PI
                    return rotationVector

        rotationVector[0] =  tmp1
        rotationVector[1] = -tmp2
        rotationVector[2] = -tmp3
        tmp12 = rotationVector[0] * rotationVector[1]
        tmp13 = rotationVector[0] * rotationVector[2]
        tmp23 = rotationVector[1] * rotationVector[2]
        if (math.fabs(tmp12) < EPS or math.fabs(tmp12 - product12) < math.fabs(tmp12 + product12)):
            if (math.fabs(tmp13) < EPS or math.fabs(tmp13 - product13) < math.fabs(tmp13 + product13)):
                if (math.fabs(tmp23) < EPS or math.fabs(tmp23 - product23) < math.fabs(tmp23 + product23)):
                    rotationVector[0] *= M_PI
                    rotationVector[1] *= M_PI
                    rotationVector[2] *= M_PI
                    return rotationVector

        rotationVector[0] = -tmp1
        rotationVector[1] =  tmp2
        rotationVector[2] = -tmp3
        tmp12 = rotationVector[0] * rotationVector[1]
        tmp13 = rotationVector[0] * rotationVector[2]
        tmp23 = rotationVector[1] * rotationVector[2]
        if (math.fabs(tmp12) < EPS or math.fabs(tmp12 - product12) < math.fabs(tmp12 + product12)):
            if (math.fabs(tmp13) < EPS or math.fabs(tmp13 - product13) < math.fabs(tmp13 + product13)):
                if (math.fabs(tmp23) < EPS or math.fabs(tmp23 - product23) < math.fabs(tmp23 + product23)):
                    rotationVector[0] *= M_PI
                    rotationVector[1] *= M_PI
                    rotationVector[2] *= M_PI
                    return rotationVector

        rotationVector[0] = -tmp1
        rotationVector[1] = -tmp2
        rotationVector[2] =  tmp3
        tmp12 = rotationVector[0] * rotationVector[1]
        tmp13 = rotationVector[0] * rotationVector[2]
        tmp23 = rotationVector[1] * rotationVector[2]
        if (math.fabs(tmp12) < EPS or math.fabs(tmp12 - product12) < math.fabs(tmp12 + product12)):
            if (math.fabs(tmp13) < EPS or math.fabs(tmp13 - product13) < math.fabs(tmp13 + product13)):
                if (math.fabs(tmp23) < EPS or math.fabs(tmp23 - product23) < math.fabs(tmp23 + product23)):
                    rotationVector[0] *= M_PI
                    rotationVector[1] *= M_PI
                    rotationVector[2] *= M_PI
                    return rotationVector
        assert(false)
    
    tmp = angle / 2.0 / math.sin(angle)
    rotationVector[0] = -(rotationMatrix[1][2] - rotationMatrix[2][1]) * tmp
    rotationVector[1] =  (rotationMatrix[0][2] - rotationMatrix[2][0]) * tmp
    rotationVector[2] = -(rotationMatrix[0][1] - rotationMatrix[1][0]) * tmp

    return rotationVector

class TestBeamMapper(KratosUnittest.TestCase):
    def setUp(self):
        self.current_model = KM.Model()
        self.model_part_beam = self.current_model.CreateModelPart("beam")
        self.model_part_surface = self.current_model.CreateModelPart("surface")

        # list of variables involved in the Mapper-Tests
        self.model_part_beam.AddNodalSolutionStepVariable(KM.DISPLACEMENT)
        self.model_part_beam.AddNodalSolutionStepVariable(KM.ROTATION)

        self.model_part_surface.AddNodalSolutionStepVariable(KM.DISPLACEMENT)

        KM.ModelPartIO(mdpa_file_name_beam).ReadModelPart(self.model_part_beam)
        KM.ModelPartIO(mdpa_file_name_surface).ReadModelPart(self.model_part_surface)
    
    def addDofs(self):
        for node in self.model_part_beam.Nodes:
            node.AddDof(KM.DISPLACEMENT_X)
            node.AddDof(KM.DISPLACEMENT_Y)
            node.AddDof(KM.DISPLACEMENT_Z)
            node.AddDof(KM.ROTATION_X)
            node.AddDof(KM.ROTATION_Y)
            node.AddDof(KM.ROTATION_Z)
        for node in self.model_part_surface.Nodes:
            node.AddDof(KM.DISPLACEMENT_X)
            node.AddDof(KM.DISPLACEMENT_Y)
            node.AddDof(KM.DISPLACEMENT_Z)

    def test_beam_mapper(self):
        mapper_settings = KM.Parameters("""{
            "mapper_type": "beam_mapper",
            "echo_level" : 3,
            "local_coord_tolerance" : 0.25,
            "use_corotation" : true
        }""")

        self.mapper = KratosMapping.MapperFactory.CreateMapper(self.model_part_beam, self.model_part_surface, mapper_settings)

        for node in self.model_part_beam.Nodes:
            lenght_beam = 5.029
            alfa = 1.0472 # 20° = 0.3491 rad, 40° = 0.6981, 60° = 1.0472 alfa is the slope of the right end
            beta = alfa
            r = lenght_beam / alfa

            theta_X = 0.0
            theta_Z = (beta * node.Z) / lenght_beam
            theta_Y = - node.Z / r 

            e_x = np.array([1.0, 0.0, 0.0])
            e_y = np.array([0.0, 1.0, 0.0])
            e_z = np.array([0.0, 0.0, 1.0])

            Rx = CalculateRotationMatrixWithAngle(e_x, theta_X)
            Ry = CalculateRotationMatrixWithAngle(e_y, theta_Y)
            Rz = CalculateRotationMatrixWithAngle(e_z, theta_Z)

            R_temp = np.dot(Ry, Rz)
            R = np.dot(Rx, R_temp)

            _ROTATION = getRotationVector(R)

            node.SetSolutionStepValue(KM.DISPLACEMENT_X, -r + r*math.cos(-theta_Y))
            node.SetSolutionStepValue(KM.DISPLACEMENT_Y, 0.0)
            node.SetSolutionStepValue(KM.DISPLACEMENT_Z, r * math.sin(-theta_Y) - node.Z )
            node.SetSolutionStepValue(KM.ROTATION_X, _ROTATION[0] )
            node.SetSolutionStepValue(KM.ROTATION_Y, _ROTATION[1] )
            node.SetSolutionStepValue(KM.ROTATION_Z, _ROTATION[2] )
            
        self.mapper.Map(KM.DISPLACEMENT, KM.ROTATION, KM.DISPLACEMENT)

        #WriteGiDOutput(self.model_part_beam)
        #WriteGiDOutput(self.model_part_surface)

        WriteVtkOutput(self.model_part_beam)
        WriteVtkOutput(self.model_part_surface)


if __name__ == '__main__':
    KratosUnittest.main()