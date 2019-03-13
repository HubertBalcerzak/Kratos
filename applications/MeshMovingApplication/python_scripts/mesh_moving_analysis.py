from __future__ import print_function, absolute_import, division  # makes KratosMultiphysics backward compatible with python 2.6 and 2.7

# Importing Kratos
import KratosMultiphysics
import KratosMultiphysics.MeshMovingApplication.python_solvers_wrapper_mesh_motion as mesh_mothion_solvers

# Importing the base class
from KratosMultiphysics.analysis_stage import AnalysisStage

class MeshMovingAnalysis(AnalysisStage):
    """
    This class is the main-script of the MeshMovingApplication put in a class
    It can be imported and used as "black-box"
    """

    #### Internal functions ####
    def _CreateSolver(self):
        """ Create the Solver (and create and import the ModelPart if it is not alread in the model) """
        return mesh_mothion_solvers.CreateSolver(self.model, self.project_parameters)

    def _GetSimulationName(self):
        return "::[Mesh Moving Simulation]:: "

if __name__ == "__main__":
    from sys import argv

    if len(argv) > 2:
        err_msg =  'Too many input arguments!\n'
        err_msg += 'Use this script in the following way:\n'
        err_msg += '- With default ProjectParameters (read from "ProjectParameters.json"):\n'
        err_msg += '    "python3 mesh_moving_analysis.py"\n'
        err_msg += '- With custom ProjectParameters:\n'
        err_msg += '    "python3 mesh_moving_analysis.py CustomProjectParameters.json"\n'
        raise Exception(err_msg)

    if len(argv) == 2: # ProjectParameters is being passed from outside
        project_parameters_file_name = argv[1]
    else: # using default name
        project_parameters_file_name = "ProjectParameters.json"

    with open(project_parameters_file_name,'r') as parameter_file:
        parameters = KratosMultiphysics.Parameters(parameter_file.read())

    model = KratosMultiphysics.Model()
    simulation = MeshMovingAnalysis(model, parameters)
    simulation.Run()
