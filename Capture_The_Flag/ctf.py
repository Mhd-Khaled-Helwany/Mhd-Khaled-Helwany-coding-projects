
""" Main file for the game.
"""
# -- Import from the ctf framework
# The framework needs to be imported after initialisation of pygame
import pygame
from pygame.locals import *
from pygame.color import *
import pymunk
import math
import sys
from pygame import mixer

# ----- Initialisation ----- #

# -- Initialise the display
pygame.init()
pygame.display.set_mode()

# -- Initialise the clock
clock = pygame.time.Clock()

# -- Initialise the physics engine
space = pymunk.Space()
space.gravity = (0.0, 0.0)
space.damping = 0.1  # Adds friction to the ground for all objects

import ai
import images
import gameobjects
import maps
# -- Constants
FRAMERATE = 50

# -- Variables
#   Define the  current level

current_map = maps.MAP_LIST[2]
#   List of all game objects
game_objects_list = []
tanks_list = []
ai_list = []
# -- Gamemodes
# UNFAIR AI
unfair_ai = True       # gives the ai a 25% movement speed buff and a 40% bullet speed buff

# BACKGROUND MUSIC
mixer.music.load('8bit-background-music.mp3')
mixer.music.play(-1)

# COUNTING SCORE
# Inizilize player id and scores for score point
player_id = ["Player 1", "Player 2", "Player 3", "Player 4", "Player 5", "Player 6"]
scores = {}
# Hit-points
hit_points = 3

# Map selector
if "--map0" in sys.argv:
    current_map = maps.MAP_LIST[2]
elif "--map1" in sys.argv:
    current_map = maps.MAP_LIST[1]
elif "--map2" in sys.argv:
    current_map = maps.MAP_LIST[0]

# Hot Seat Multiplayer
if "--hot-multiplayer" in sys.argv:
    hot_seat_multiplayer = True
else:
    hot_seat_multiplayer = False

# -- Resize the screen to the size of the current level
screen = pygame.display.set_mode(current_map.rect().size)

# -- Generate the background
background = pygame.Surface(screen.get_size())

# -- Collision types for the collision handler
collision_types = {"bullet": 1, "tank": 2, "box": 3}


def score_counter(tank):
    """ Prints scores of players when called. """

    player_id = tank.player_id
    scores[player_id] += 1
    print("Player Scores:")
    for player_id, score in scores.items():
        print(f"{player_id}: {score}")


def tank_respawn(tank):
    # Finds which tank got hit and respawn it
    for i in range(len(tanks_list)):
        if tank == tanks_list[i]:

            respawn_point = current_map.start_positions[i]

            # Drops flag
            if flag.is_on_tank:
                flag.is_on_tank = False
                tank.flag = None
                flag.x = tank.body.position.x
                flag.y = tank.body.position.y

            tank.body.position = respawn_point[0], respawn_point[1]
            tank.body.angle = math.radians(respawn_point[2])
            tank.acceleration = 0
            tank.body.velocity = pymunk.Vec2d.zero()
            # Resetar next_cycle_gen
            for ai_t in ai_list:
                if ai_t.tank == tank:
                    ai_t.stop_generator()
                    ai_list.remove(ai_t)
                    ai_tank = ai.Ai(tank, game_objects_list, tanks_list, space, current_map)
                    ai_list.append(ai_tank)


def restart(tanky):
    """ Restarts the game without deleting scores """
    for tank in tanks_list:
        tank_respawn(tank)

    # Reset the flag state and position
    tanky.flag = None
    flag.is_on_tank = False
    flag.x = current_map.flag_position[0]
    flag.y = current_map.flag_position[1]
    flag.orientation = 0

    new_obj_list = []
    for obj in game_objects_list:
        if not isinstance(obj, gameobjects.Box):
            new_obj_list.append(obj)
    # Remove shape
    for obj in game_objects_list:
        if isinstance(obj, gameobjects.Box):
            space.remove(obj.shape, obj.shape.body)

    game_objects_list.clear()
    create_boxes()
    game_objects_list.extend(new_obj_list)

    return


def second_player(event):
    if event.type == KEYDOWN and event.key == K_w:
        tanks_list[1].accelerate()
        return True

    elif event.type == KEYDOWN and event.key == K_s:
        tanks_list[1].decelerate()
        return True

    elif event.type == KEYDOWN and event.key == K_a:
        tanks_list[1].turn_left()
        return True

    elif event.type == KEYDOWN and event.key == K_d:
        tanks_list[1].turn_right()
        return True

    elif event.type == KEYDOWN and event.key == K_SPACE:
        tanks_list[1].shoot(space, game_objects_list)
        return True

    elif event.type == KEYUP and (event.key == K_d or event.key == K_a):
        tanks_list[1].stop_turning()
        return True

    elif event.type == KEYUP and (event.key == K_w or event.key == K_s):
        tanks_list[1].stop_moving()
        return True

    return False


def quit(event):
    """
    Checks if we receive a QUIT event.
    The parameter "event" is a Pygame event object.
    Returns a bool.
    """

    if event.type == QUIT:
        return True
    elif (event.type == KEYDOWN and event.key == K_ESCAPE):
        return True
    else:
        return False


def event_handler(event):
    """
    Handles events when keys are pressed and performs corresponding actions.
    The parameter "event" is a Pygame event object.
    Returns a bool.
    """

    if event.type == KEYDOWN and event.key == K_UP:
        tanks_list[0].accelerate()
        return True

    elif event.type == KEYDOWN and event.key == K_DOWN:
        tanks_list[0].decelerate()
        return True

    elif event.type == KEYDOWN and event.key == K_LEFT:
        tanks_list[0].turn_left()
        return True

    elif event.type == KEYDOWN and event.key == K_RIGHT:
        tanks_list[0].turn_right()
        return True

    elif event.type == KEYUP and (event.key == K_RIGHT or event.key == K_LEFT):
        tanks_list[0].stop_turning()
        return True

    elif event.type == KEYUP and (event.key == K_UP or event.key == K_DOWN):
        tanks_list[0].stop_moving()
        return True

    if hot_seat_multiplayer:
        if event.type == KEYDOWN and event.key == K_RETURN:
            tanks_list[0].shoot(space, game_objects_list)
            return True

    else:
        if event.type == KEYDOWN and event.key == K_SPACE:
            tanks_list[0].shoot(space, game_objects_list)
            return True


def update_physics(skip_update):
    """
    Updates the physics of the game objects based on their acceleration.
    The parameter "skip_update" is an int.
    """
    if skip_update == 0:
        # Loop over all the game objects and update their speed in function of their
        # acceleration.
        for obj in game_objects_list:
            obj.update()
        skip_update = 2
    else:
        skip_update -= 1


def create_grass():
    """ Copys grass tiles all over the level area """

    for x in range(0, current_map.width):
        for y in range(0, current_map.height):
            # The call to the function "blit" will copy the image
            # contained in "images.grass" into the "background"
            # image at the coordinates given as the second argument
            background.blit(images.grass, (x * images.TILE_SIZE, y * images.TILE_SIZE))


create_grass()


def create_boxes():
    """ Creates boxes based on the box types specified in the current map"""

    for x in range(0, current_map.width):
        for y in range(0, current_map.height):
            # Get the type of boxes
            box_type = current_map.boxAt(x, y)
            # If the box type is not 0 (aka grass tile), create a box
            if (box_type != 0):
                # Create a "Box" using the box_type, aswell as the x,y coordinates,
                # and the pymunk space
                box = gameobjects.get_box_with_type(x, y, box_type, space)
                game_objects_list.append(box)


create_boxes()


def create_tanks():
    """ Creates the tanks and bases on the current map's start position """

    # Loop over the starting poistion
    for i in range(0, len(current_map.start_positions)):
        # Get the starting position of the tank "i"
        pos = current_map.start_positions[i]
        # Create the tank, images.tanks contains the image representing the tank
        tank = gameobjects.Tank(pos[0], pos[1], pos[2], images.tanks[i], space, collision_types, player_id[i], False, False, hit_points)
        # Create the bases, images.bases contains the images representing the base
        bases = gameobjects.GameVisibleObject(pos[0], pos[1], images.bases[i])
        # Add the bases to the game_objects_list
        game_objects_list.append(bases)
        # Add the tank to the list of tanks

        tanks_list.append(tank)
        game_objects_list.append(tank)

    for tank in tanks_list:
        scores[tank.player_id] = 0
        if hot_seat_multiplayer:
            if tank not in [tanks_list[0], tanks_list[1]]:
                ai_tank = ai.Ai(tank, game_objects_list, tanks_list, space, current_map)
                ai_list.append(ai_tank)

        else:
            if tank == tanks_list[0]:
                pass

            else:
                tank.buff = unfair_ai
                ai_tank = ai.Ai(tank, game_objects_list, tanks_list, space, current_map)
                ai_list.append(ai_tank)


create_tanks()


def create_flag():
    """ Creates a flag object and returns the flag object """

    flag = gameobjects.Flag(current_map.flag_position[0], current_map.flag_position[1])
    game_objects_list.append(flag)
    return flag


flag = create_flag()


def create_segments():
    """ Creates segments """

    wall = space.static_body
    walls = [pymunk.Segment(wall, (0, 0), (current_map.width, 0), 0.0),
             pymunk.Segment(wall, (0, current_map.height), (current_map.width, current_map.height), 0.0),
             pymunk.Segment(wall, (0, 0), (0, current_map.height), 0.0),
             pymunk.Segment(wall, (current_map.width, 0), (current_map.width, current_map.height), 0.0)
             ]

    for wall in walls:
        space.add(wall)


create_segments()


# --Create the collision handlers
bullet_tank_handler = space.add_collision_handler(collision_types["bullet"], collision_types["tank"])
bullet_destructablebox_handler = space.add_collision_handler(collision_types["bullet"], collision_types["box"])
bullet_wall_handler = space.add_collision_handler(collision_types["bullet"], 0)


def bullet_wall_collision(arbiter, space, data):
    """
    Handles collision between a bullet and a wall.
    Takes tre Parameters:
    - arbiter(pymunk.Arbiter): Collision between two shapes
    - space (pymunk.Space): Physical space where collision occurs
    - data: Additional data that can be associated with the collision
    Returns True.
    """

    # Checks if bullet is in gam_objetcs_list
    if arbiter.shapes[0].parent in game_objects_list:
        # Removes the bullet from game_objects_list
        game_objects_list.remove(arbiter.shapes[0].parent)
        # Removes the bullet's shape and body from the physical space
        space.remove(arbiter.shapes[0], arbiter.shapes[0].body)

    return True


bullet_wall_handler.pre_solve = bullet_wall_collision


def bullet_tank_collision(arbiter, space, data):
    """
    Handles collision between a bullet and a tank.
    Takes tre Parameters:
    - arbiter(pymunk.Arbiter): Collision between two shapes
    - space (pymunk.Space): Physical space where collision occurs
    - data: Additional data that can be associated with the collision
    Returns True.
    """
    # Checks if the shooting tank is not the same tank as the one that got hit
    if arbiter.shapes[0].parent.shooting_tank != arbiter.shapes[1].parent:
        # Checks if bullet is in game_objetcs_list
        if arbiter.shapes[0].parent in game_objects_list:
            # Removes the bullet from game_objects_list and the physical space
            game_objects_list.remove(arbiter.shapes[0].parent)
            space.remove(arbiter.shapes[0], arbiter.shapes[0].body)

        arbiter.shapes[1].parent.hit_points -= 1
        # Explosion
        expl = gameobjects.Explosion(arbiter.shapes[1].parent.body.position.x, arbiter.shapes[1].parent.body.position.y, remove_explosion)
        game_objects_list.append(expl)
        # Explosion sound
        expl_sound1 = mixer.Sound('explosion.aiff')
        expl_sound1.play()

        if arbiter.shapes[1].parent.hit_points <= 0:
            tank_respawn(arbiter.shapes[1].parent)
            arbiter.shapes[1].parent.hit_points = hit_points
    return True


bullet_tank_handler.pre_solve = bullet_tank_collision


def bullet_box_handler(arbiter, space, data):
    """
    Handles collision between a bullet and a box.
    Takes tre Parameters:
    - arbiter(pymunk.Arbiter): Collision between two shapes
    - space (pymunk.Space): Physical space where collision occurs
    - data: Additional data that can be associated with the collision
    Returns True.
    """

    if arbiter.shapes[0].parent in game_objects_list:
        game_objects_list.remove(arbiter.shapes[0].parent)
        space.remove(arbiter.shapes[0], arbiter.shapes[0].body)

    if arbiter.shapes[1].parent.destructable:
        arbiter.shapes[1].parent.hit_points -= 1
        # Explosion
        expl = gameobjects.Explosion(arbiter.shapes[1].parent.body.position.x, arbiter.shapes[1].parent.body.position.y, remove_explosion)
        game_objects_list.append(expl)
        # Explosion sound
        expl_sound1 = mixer.Sound('explosion.aiff')
        expl_sound1.play()

        if arbiter.shapes[1].parent.hit_points <= 0:
            game_objects_list.remove(arbiter.shapes[1].parent)
            space.remove(arbiter.shapes[1], arbiter.shapes[1].body)

    return True


bullet_destructablebox_handler.pre_solve = bullet_box_handler


def update():
    for obj in game_objects_list:
        obj.post_update()


def flag_update():
    for tank in tanks_list:
        tank.try_grab_flag(flag)


def victory_cond():
    for tank in tanks_list:
        if tank.has_won():
            score_counter(tank)
            restart(tank)
            # return True


def update_display():
    for obj in game_objects_list:
        obj.update_screen(screen)


def ai_decide():
    for ai_tank in ai_list:
        ai_tank.decide()


def remove_explosion(expl):
    game_objects_list.remove(expl)


def main():
    # ----- Main Loop -----#
    # -- Control whether the game run
    running = True
    skip_update = 0

    while running:

        for event in pygame.event.get():
            # -- Handle the events
            if quit(event):
                running = False
            else:
                event_handler(event)
                if hot_seat_multiplayer:
                    second_player(event)

        # -- Update physics
        update_physics(skip_update)
        # Check collisions and update the objects position
        space.step(1 / FRAMERATE)
        # Update object that depends on an other object position (for instance a flag)
        update()
        ai_decide()
        # Grab the flag and update flag position
        flag_update()

        # Victory condition
        victory_cond()
        # -- Update Display
        update_display()
        # Display the background on the screen
        screen.blit(background, (0, 0))
        # Update the display of the game objects on the screen
        update_display()
        #   Redisplay the entire screen (see double buffer technique)
        pygame.display.flip()
        #   Control the game framerate
        clock.tick(FRAMERATE)


main()
