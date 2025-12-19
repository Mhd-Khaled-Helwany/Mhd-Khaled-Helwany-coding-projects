import nav_algorithm

# Testing
if __name__ == '__main__':
    # One item
    # price = nav_algorithm.findPath([(3, 5)])
    # print(price)
    
    # Straight Line
    # price = nav_algorithm.findPath([(3, 5), (3, 3)])
    # print(price)

    # Turn
    # price = nav_algorithm.findPath([(3, 5), (3, 3), (1,3)])
    # print(price)

    # Test: Should we see a 180 turn?
    # YES WE SHOULD!!!
    print("Test 1: Single pickup")
    price = nav_algorithm.findPath([(10,11)], (11,12))
    print(price)
    print()



    print("Test 2: Single pickup")
    price = nav_algorithm.findPath([(11,10)], (11,12))
    print(price)
    print()


    print("Test 3: Two pickups")
    price = nav_algorithm.findPath([(11,10), (10,11)], (11,12))
    print(price)
