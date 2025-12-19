"""
Implementation of a priority queue
"""


class priorityQueue:

    q = []   # The queue

    def insert(self, coord:tuple, path, value:int, direction:int, pathDir, must_go_straight=False):
        path.append(coord)
        pathDir.append(direction)
        self.q.append((path, value, pathDir, must_go_straight))

    def pop(self):
        try:
            m = 0
            for i in range(len(self.q)):
                if self.q[i][1] < self.q[m][1]:
                    m = i
            item = self.q[m]

            del self.q[m]
            return item
        except IndexError:
            print("Tried to pop an empty priority queue.")
            exit()

    def is_empty(q):
        return len(q) == 0
    
    def reset(self):
        self.q.clear()
