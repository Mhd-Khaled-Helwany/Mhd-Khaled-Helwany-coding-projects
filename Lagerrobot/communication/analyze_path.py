import nav_algorithm

# Analyze the directions more carefully
result = nav_algorithm.findPath([(10,11)], (11,12))
coords = result[0]
dirs = result[1]

print("Path analysis:")
print(f"Total steps: {len(coords)}")
print()

for i in range(len(coords)-1):
    curr = coords[i]
    next_pos = coords[i+1]
    direction = dirs[i]

    # Determine actual movement
    dx = next_pos[0] - curr[0]
    dy = next_pos[1] - curr[1]

    if dx == 1:
        actual_dir = "RIGHT (2)"
    elif dx == -1:
        actual_dir = "LEFT (-2)"
    elif dy == 1:
        actual_dir = "DOWN (-1)"
    elif dy == -1:
        actual_dir = "UP (1)"
    else:
        actual_dir = "UNKNOWN"

    # Check for turns
    if i > 0:
        prev_dir = dirs[i-1]
        if direction == prev_dir:
            turn = "STRAIGHT"
        elif direction == -prev_dir:
            turn = "180° TURN"
        else:
            turn = "90° TURN"
    else:
        turn = "START"

    pickup_marker = " [PICKUP]" if curr == (10,11) else ""

    print(f"{i}: {curr} -> {next_pos}  |  direction={direction}  |  movement={actual_dir}  |  {turn}{pickup_marker}")

print(f"\nFinal: {coords[-1]}")
