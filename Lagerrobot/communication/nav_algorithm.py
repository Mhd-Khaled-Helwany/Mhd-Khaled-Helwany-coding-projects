"""
This file takes in where the items are located, the startpoint of the robot and how the robot is oriented.
It then finds a valid path for the robot to take in order to pick up all items. 
"""


import math
from itertools import permutations
import priority_queue

STOP_POSITION = (11, 12)

map = [[0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
       [0, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 0],
       [0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0],
       [0, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 0],
       [0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0],
       [0, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 0],
       [0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0],
       [0, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 0],
       [0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0],
       [0, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 0],
       [0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0],
       [0, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 0],
       [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0],
       [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]] 


"""
map = [[0, 0, 0, 0, 0, 0, 0],
       [0, 2, 1, 2, 1, 2, 0],
       [0, 1, 0, 1, 0, 1, 0],
       [0, 2, 1, 2, 1, 2, 0],
       [0, 1, 0, 1, 0, 1, 0],
       [0, 2, 1, 2, 1, 2, 0],
       [0, 0, 0, 0, 0, 1, 0],
       [0, 0, 0, 0, 0, 2, 0], # <- we enter from the 2
       [0, 0, 0, 0, 0, 0, 0]] """

#TODO REMEMBER TO RESET MAP TO DEFAULT AFTER ONE ROUTE COMPLETED

def remove_position(position):
    x = position[0]
    y = position[1]
    map[y][x] = 0
    for row in  map:
        print(row)


def findPath(items, startPos,startDir=1):
    """
    Finds the best path to pick up all items

    :param items: List of item coordinates to pick up
    :param startPos: Starting position of the robot
    :param startDir: Starting direction of the robot
    """
    bestPath = None
    bestTime = math.inf
    bestDir = None
    startPosition = startPos

    permutation = list(permutations(items))
    pickup_order = [list(perm) + [STOP_POSITION] for perm in permutation]

    # Determines the order in which to pick up items
    for permutation in pickup_order:
        time = 0
        direction = startDir   #1 = up, -1 = down, 2 = right, -2 = left
        completedir = [direction]
        completePath = [startPosition]
        prevItem = startPosition

        # Runs A* for one item at a time
        for item in permutation:
            # Check if prevItem is a pickup point (not start or stop position)
            is_pickup_point = prevItem in items and prevItem != startPosition

            pathToProduct = AStar(prevItem, item, direction, is_pickup_point)

            time += pathToProduct[1]
            completePath += pathToProduct[0][1:]
            completedir += pathToProduct[2][1:]

            prevItem = item # Updates the startpoint for A*
            direction = completedir[len(completedir) - 1] # Current orientation

        if time < bestTime:
            bestTime = time
            bestPath = completePath
            bestDir = completedir

    translatedPath = translation(bestDir, bestPath)
    print(bestPath, translatedPath)
    return (bestPath, translatedPath)
    

def AStar(start, stop, direction, from_pickup_point=False):
    """
    A* pathfinding algorithm to find the best path from start to stop

    :param start: Starting coordinate
    :param stop: Destination coordinate
    :param direction: Starting direction
    :param from_pickup_point: If True, must go straight on first move (no turns at pickup)
    """
    pq = priority_queue.priorityQueue()
    pq.insert(start, [], 0, direction, [], from_pickup_point)
    iterations = 0
    max_iterations = 10000

    while len(pq.q) > 0 and iterations < max_iterations:
        iterations += 1
        current = pq.pop()
        headofPath = current[0][len(current[0])-1]  # Finds coordinates of our "current" position
        currentDir = current[2][len(current[0])-1]  # Find our "current" orientation
        must_go_straight = current[3] if len(current) > 3 else False  # Check if we must go straight

        # Check if we have reached the desired coordinate
        if headofPath == stop:
            pq.reset()
            return current[:3]  # Return only path, cost, directions (not the flag)

        # Checks cost for each neighbour
        for neighbour in neighbours(headofPath, 1):
            newHead = neighbour[0]
            newDir = neighbour[1]

            # If leaving pickup point, MUST go straight - no turns allowed at all
            # This forces any 180-turn to happen at the NEXT crossing
            if must_go_straight and newDir != currentDir:
                continue

            costToNext = current[1] + moveCost(currentDir, newDir)
            totalCost = costToNext #+ distance(headofPath, stop)

            # Determine direction the robot is traveling
            currentCopy = currentDir
            # Allow revisiting ONLY the start position (pickup point) - this enables 180-turn paths
            # where robot goes straight from pickup, turns 180 at next crossing, comes back through pickup
            is_revisit = newHead in current[0]
            is_start_position = len(current[0]) > 0 and newHead == current[0][0]

            if not is_revisit or is_start_position:
                if (newDir != currentDir):
                    currentCopy = newDir
                # After first move from pickup, all turns (including 180°) are allowed at next crossing
                next_must_go_straight = False
                pq.insert(newHead, current[0].copy(), totalCost, currentCopy, current[2].copy(), next_must_go_straight)

    print(f"A* failed: start={start}, stop={stop}, direction={direction}, from_pickup={from_pickup_point}, iterations={iterations}")
    return -1



def moveCost(currentDir, newDir): #Hardcoded costs for different actions
    if (currentDir == newDir):  # Move forward
        return 10
    elif (currentDir == -newDir):  # Turn 180 degrees
        return 90
    else:
        return 50  # Turn 90 degrees


def distance(start, stop): # Distance between start and stop
    difx = abs(start[0] - stop[0])
    dify = abs(start[1] - stop[1])
    return math.sqrt(math.pow(difx, 2) + math.pow(dify, 2))


def neighbours(tuple, dir):  # Find the corrdinates for all allowed neighbours
    directions = [1, 2, -1, -2]
    dir_ = directions.index(dir)

    neighbours_ = []
    # 1 = up, -1 = down, 2 = right, -2 = left    
    neighbours_.append(((tuple[0]-1, tuple[1]), directions[(3-dir_)%4]))
    neighbours_.append(((tuple[0]+1, tuple[1]), directions[(1-dir_)%4]))
    neighbours_.append(((tuple[0], tuple[1]-1), directions[(0-dir_)%4]))
    neighbours_.append(((tuple[0], tuple[1]+1), directions[(2-dir_)%4]))

    # Find disallowed neighbours
    neighbors_to_kill = []
    for i in range(len(neighbours_)):
        if map[neighbours_[i][0][1]][neighbours_[i][0][0]] == 0:
            neighbors_to_kill.append(i)
        
        

    # Kill disallowed neighbours
    for i in reversed(neighbors_to_kill):
        del neighbours_[i]

    return neighbours_


def translation(pathDir, path): # Translates the path into robot move directions for each coordinate
    translationPath = []

    for i in range(len(pathDir) - 1):
        index = 0
        neighbours_ = neighbours(path[i], pathDir[i])
        for j in range(len(neighbours_)):
            if (neighbours_[j][0] == path[i + 1]):
                index = j
        translationPath.append(neighbours_[index][1])

    return translationPath

