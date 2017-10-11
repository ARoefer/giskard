from giskard_core import *
from giskard_ros  import GiskardControllerRunner
from whatever_math_lib import *

class PointRobot(object):
    """This is a simple point robot. It's workspace is limited to 6*6*6m around its origin"""
    def __init__(self):
        super(PointRobot, self).__init__()

        self.end_effector = vec3(InputJoint('joint_x'), InputJoint('joint_y'), InputJoint('joint_z'))
        self.controllables = [ControllableConstraint(-0.2, 0.2, 0.01, 'joint_x'),
                              ControllableConstraint(-0.2, 0.2, 0.01, 'joint_y'),
                              ControllableConstraint(-0.2, 0.2, 0.01, 'joint_z')]
        self.hard_constraints = [HardConstraint(-3 - self.end_effector.x, 3 - self.end_effector.x, self.end_effector.x),
                                 HardConstraint(-3 - self.end_effector.y, 3 - self.end_effector.y, self.end_effector.y),
                                 HardConstraint(-3 - self.end_effector.z, 3 - self.end_effector.z, self.end_effector.z)]

        self.command_topic = '/point_bot/velocity_commands'


def moveToSC(pointA, pointB, weight=1):
    """This function generates a soft constraint tries to
    drive the distance between two points down to 0
    """
    dist = norm(pointB - pointA)
    return SoftConstraint(-dist, -dist, weight, dist, 'Align points constraint')


if __name__ == '__main__':
    goalPoint = InputVec3('goal')
    pointBot = PointRobot()
    controller = QPController(pointBot.controllables, [moveToSC(pointBot.end_effector, goalPoint)], pointBot.controllables)

    print(controller.inputNames)
    # RESULT: >>> "['joint_x', 'joint_y', 'joint_z', 'goal']"

    rospy.init_node('giskard_node', anonymous=True)

    ### Somewhat fuzzy on this part. Basic idea:
    ## Runner subscribes to joint_state and whatever else
    ## Updates controller with new data and sends commands to robot
    runner = GiskardControllerRunner(pointBot.velocity_commands)

    runner.run_controller(controller)

    rospy.spin()


