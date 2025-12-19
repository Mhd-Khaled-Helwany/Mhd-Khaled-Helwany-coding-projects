import images
import pygame
import json
import os

MAP_LIST = []


class Map:
    """ An instance of Map is a blueprint for how the game map will look. """

    def __init__(self, width, height, boxes, start_positions, flag_position):
        """ Takes as argument the size of the map (width, height), an array with the boxes type,
        the start position of tanks (start_positions) and the position of the flag (flag_position).
        """
        self.width = width
        self.height = height
        self.boxes = boxes
        self.start_positions = start_positions
        self.flag_position = flag_position

    def rect(self):
        return pygame.Rect(0, 0, images.TILE_SIZE * self.width, images.TILE_SIZE * self.height)

    def boxAt(self, x, y):
        """ Return the type of the box at coordinates (x, y). """
        return self.boxes[y][x]


def read_maps():
    map_directory = 'maps'
    for file in os.scandir(map_directory):
        if file.is_file():
            map_file = json.load(open(file))
            map_info = Map(map_file["width"], map_file["height"],
                           map_file["boxes"], map_file["start_positions"],
                           map_file["flag_position"])
            MAP_LIST.append(map_info)


read_maps()
