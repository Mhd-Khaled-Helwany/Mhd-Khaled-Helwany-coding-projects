""" This file contains function and classes for the Artificial Intelligence used in the game.
"""
import math
from collections import defaultdict, deque

import pymunk
from pymunk import Vec2d
import gameobjects

# NOTE: use only 'map0' during development!

MIN_ANGLE_DIF = math.radians(3)   # 3 degrees, a bit more than we can turn each tick


def angle_between_vectors(vec1, vec2):
    """ Since Vec2d operates in a cartesian coordinate space we have to
        convert the resulting vector to get the correct angle for our space.
    """
    vec = vec1 - vec2
    vec = vec.perpendicular()
    return vec.angle


def periodic_difference_of_angles(angle1, angle2):
    """ Compute the difference between two angles.
    """
    return (angle1 % (2 * math.pi)) - (angle2 % (2 * math.pi))


class Ai:
    """ A simple ai that finds the shortest path to the target using
    a breadth first search. Also capable of shooting other tanks and or wooden
    boxes. """

    def __init__(self, tank, game_objects_list, tanks_list, space, currentmap):
        self.tank = tank
        self.game_objects_list = game_objects_list
        self.tanks_list = tanks_list
        self.space = space
        self.currentmap = currentmap
        self.flag = None
        self.max_x = currentmap.width - 1
        self.max_y = currentmap.height - 1
        self.path = deque()
        self.move_cycle = self.move_cycle_gen()
        self.update_grid_pos()

    def update_grid_pos(self):
        """ This should only be called in the beginning, or at the end of a move_cycle. """
        self.grid_pos = self.get_tile_of_position(self.tank.body.position)

    def decide(self):
        """ Main decision function that gets called on every tick of the game.
        """
        self.maybe_shoot()
        next(self.move_cycle)

    def raycast(self, start, end, radius=0):
        """ Perform raycasting and return the first object encountered. """
        angle = self.tank.body.angle + math.pi / 2
        start_x = start * math.cos(angle) + self.tank.body.position.x
        start_y = start * math.sin(angle) + self.tank.body.position.y
        end_x = end * math.cos(angle) + self.tank.body.position.x
        end_y = end * math.sin(angle) + self.tank.body.position.y

        result = self.space.segment_query_first((start_x, start_y), (end_x, end_y), radius, pymunk.ShapeFilter())
        return result

    def maybe_shoot(self):
        """ Makes a raycast query in front of the tank. If another tank
            or a wooden box is found, then we shoot.
        """
        start_distance = 0.5
        end_distance = math.sqrt(self.currentmap.width**2 + self.currentmap.height**2)

        # Perform raycasting
        result = self.raycast(start_distance, end_distance)

        # Check if a target is found and hasattr(result, 'shape.parent')
        if not (result is None):
            if hasattr(result, 'shape') and hasattr(result.shape, 'parent'):

                if isinstance(result.shape.parent, gameobjects.Tank):
                    # A tank is in front, shoot it!
                    self.tank.shoot(self.space, self.game_objects_list)

                elif isinstance(result.shape.parent, gameobjects.Box) and result.shape.parent.destructable:
                    # A box is in front, shoot it!
                    self.tank.shoot(self.space, self.game_objects_list)

    def move_cycle_gen(self):

        """ A generator that iteratively goes through all the required steps
            to move to our goal.
        """
        while True:
            # find correct path
            path = self.find_shortest_path(self.filter_tile_neighbors)
            if not path:
                self.update_grid_pos()
                yield

                path = self.find_shortest_path(self.filter_tile_neighbors_metal)

            if path:
                next_coord = path.popleft()
                self.next_center = next_coord + Vec2d(0.5, 0.5)
            # Turn
            self.turn()
            while not (-MIN_ANGLE_DIF < self.find_angle() < MIN_ANGLE_DIF):
                yield
            self.tank.stop_turning()

            # Accelerate to next tile
            self.tank.accelerate()
            # Check if self.next_center is defined
            if hasattr(self, 'next_center') and self.next_center is not None:
                current_dist = self.tank.body.position.get_distance(self.next_center)
                while current_dist >= self.tank.body.position.get_distance(self.next_center):
                    current_dist = self.tank.body.position.get_distance(self.next_center)
                    yield
            self.tank.stop_moving()
            self.update_grid_pos()

    def stop_generator(self):
        # Stop the generator
        self.move_cycle.close()

    def find_angle(self):
        """Finds the turning angle between the tank and the center of the next tile"""
        # Check if self.next_center is defined
        if hasattr(self, 'next_center') and self.next_center is not None:
            angle_between_tiles = angle_between_vectors(self.tank.body.position, self.next_center)
            turning_angle = periodic_difference_of_angles(self.tank.body.angle, angle_between_tiles)
            return turning_angle
        else:
            return 0  # Return a default value if self.next_center is not defined

    def turn(self):
        """Calculates the shortest way to turn and turns"""
        # Check if self.next_center is defined
        if hasattr(self, 'next_center') and self.next_center is not None:
            angle = angle_between_vectors(self.tank.body.position, self.next_center)
            diff = periodic_difference_of_angles(self.tank.body.angle, angle)

            if diff <= -math.pi or diff >= math.pi:
                diff = -diff
            if diff > 0:
                self.tank.turn_left()
            else:
                self.tank.turn_right()

    def find_shortest_path(self, func):
        """ A simple Breadth First Search using integer coordinates as our nodes.
            Edges are calculated as we go, using an external function.
        """
        # initialize variables
        start_node = self.grid_pos
        target_node = self.get_target_tile().int_tuple
        shortest_path = []
        path = {start_node: []}
        queue = deque([start_node])
        visited = set([start_node])

        while queue:
            current_node = queue[0]
            queue.popleft()

            # Returns the shortest path
            if current_node == target_node:
                shortest_path = path[current_node]

                return deque(shortest_path)

            # Adds new tiles to the queue and visited tiles
            for neighbor in self.get_tile_neighbors(current_node, func):
                if neighbor not in visited:
                    queue.append(neighbor.int_tuple)
                    visited.add(neighbor.int_tuple)
                    path[neighbor] = path[current_node] + [neighbor]
        return deque([])  # Return an empty path

    def get_target_tile(self):
        """ Returns position of the flag if we don't have it. If we do have the flag,
            return the position of our home base.
        """
        if self.tank.flag is not None:
            x, y = self.tank.start_position
        else:
            self.get_flag()  # Ensure that we have initialized it.
            x, y = self.flag.x, self.flag.y
        return Vec2d(int(x), int(y))

    def get_flag(self):
        """ This has to be called to get the flag, since we don't know
            where it is when the Ai object is initialized.
        """
        if self.flag is None:
            # Find the flag in the game objects list
            for obj in self.game_objects_list:
                if isinstance(obj, gameobjects.Flag):
                    self.flag = obj
                    break
        return self.flag

    def get_tile_of_position(self, position_vector):
        """ Converts and returns the float position of our tank to an integer position. """
        x, y = position_vector
        return Vec2d(int(x), int(y))

    def get_tile_neighbors(self, coord_vec, func):
        """ Returns all bordering grid squares of the input coordinate.
            A bordering square is only considered accessible if it is grass
            or a wooden box.
        """
        neighbors = [coord_vec + Vec2d(0, 1), coord_vec + Vec2d(0, -1), coord_vec + Vec2d(1, 0), coord_vec + Vec2d(-1, 0)]  # Find the coordinates of the tiles' four neighbors
        return filter(func, neighbors)

    def filter_tile_neighbors(self, coord):
        """ Used to filter the tile to check if it is a neighbor of the tank.
        """
        if 0 <= coord[0] <= self.max_x and 0 <= coord[1] <= self.max_y:
            if self.currentmap.boxAt(coord[0], coord[1]) == 0 or\
                    self.currentmap.boxAt(coord[0], coord[1]) == 2:
                return True
        else:
            return False

    def filter_tile_neighbors_metal(self, coord):
        """ Used to filter the tile to check if it is a neighbor of the tank with the addition of metallboxes.
        """
        if 0 <= coord[0] <= self.max_x and 0 <= coord[1] <= self.max_y:
            if self.currentmap.boxAt(coord[0], coord[1]) == 0 or\
                    self.currentmap.boxAt(coord[0], coord[1]) == 2:
                return True
            elif self.currentmap.boxAt(coord[0], coord[1]) == 3:
                return True
        else:
            return False
