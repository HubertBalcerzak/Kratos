import KratosMultiphysics
import KratosMultiphysics.RANSApplication as KratosRANS

def Factory(settings, Model):
    if(type(settings) != KratosMultiphysics.Parameters):
        raise Exception("expected input shall be a Parameters object, encapsulating a json string")
    return ApplySlipProcess(Model, settings["Parameters"])

## All the processes python should be derived from "Process"
class ApplySlipProcess(KratosMultiphysics.Process):
    def __init__(self, Model, settings ):
        KratosMultiphysics.Process.__init__(self)

        default_parameters = KratosMultiphysics.Parameters( """
            {
                "model_part_name":"PLEASE_CHOOSE_MODEL_PART_NAME"
            }  """ )

        settings.ValidateAndAssignDefaults(default_parameters)

        self.model_part = Model[settings["model_part_name"].GetString()]

        for node in self.model_part.Nodes:
            node.Set(KratosMultiphysics.SLIP, True)

        for condition in self.model_part.Conditions:
            condition.Set(KratosMultiphysics.SLIP, True)
