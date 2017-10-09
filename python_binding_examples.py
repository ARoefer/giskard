#####################
### Easy but ugly ###
#####################
import giskard_core as gc

scope = gc.ScopeSpec()
scope['goal']  = gc.InputScalar('goal')
scope['joint'] = gc.InputJoint('joint')
scope['ctrl']  = scope['goal'] - scope['joint']

s = gc.SoftConstraint(scope['ctrl'], scope['ctrl'], 1, scope['joint'], 'some constraint')
c = gc.ControllableConstraint(-0.2, 0.2, 0.01, 'joint')

# scope, hard constraints, soft constraints, hard constraints
controller = gc.QPController(scope, [c], [s], [])

##################################
### More complicated but nicer ###
##################################
import giskard_core as gc

goal  = gc.InputScalar('goal')
joint = gc.InputJoint('joint')
ctrl  = goal - joint

s = gc.SoftConstraint(ctrl, ctrl, 1, joint, 'some constraint')
c = gc.ControllableConstraint(-0.2, 0.2, 0.01, 'joint')

controller = gc.QPController([c], [s], [])