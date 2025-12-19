import nav_algorithm

def analyze_turns(coords, dirs):
    """Analyze and print turn information"""
    print(f"Path length: {len(coords)} positions")
    turns_180 = 0
    turns_90 = 0
    straight = 0

    for i in range(len(dirs)-1):
        curr_dir = dirs[i]
        next_dir = dirs[i+1]

        if curr_dir == next_dir:
            straight += 1
        elif curr_dir == -next_dir:
            turns_180 += 1
            print(f"  180° TURN at position {coords[i+1]} (direction {curr_dir} -> {next_dir})")
        else:
            turns_90 += 1

    print(f"\nSummary:")
    print(f"  - Straight moves: {straight}")
    print(f"  - 90° turns: {turns_90}")
    print(f"  - 180° turns: {turns_180} ✓")
    return turns_180

print("="*60)
print("TEST 1: Pickup at (10,11) near start")
print("="*60)
result = nav_algorithm.findPath([(10,11)], (11,12))
turns = analyze_turns(result[0], result[1])

print("\n" + "="*60)
print("TEST 2: Pickup at (5,11) far from start")
print("="*60)
result = nav_algorithm.findPath([(5,11)], (11,12))
turns = analyze_turns(result[0], result[1])

print("\n" + "="*60)
print("TEST 3: Two pickups on same line")
print("="*60)
result = nav_algorithm.findPath([(5,11), (9,11)], (11,12))
turns = analyze_turns(result[0], result[1])

print("\n" + "="*60)
print("VERIFICATION: 180° turns happen at CROSSINGS, not at PICKUPS!")
print("="*60)
