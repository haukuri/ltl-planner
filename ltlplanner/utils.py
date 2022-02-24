from collections import defaultdict

def get_item_or_initialize(collection, key, default_factory):
    """
    Tries to get an item from `collection`.

    If it is not present, initializes it with value obtained from `default_factory`
    and returns the initialized value.
    """
    try:
        item = collection[key]
    except KeyError:
        item = default_factory()
        collection[key] = item
    return item

class BidirectionalEdgeMap:
    def __init__(self):
        self.__post = defaultdict(set)
        self.__pre = defaultdict(set)

    def add(self, a, b):
        self.post(a).add(b)
        self.pre(b).add(a)

    def remove(self, a, b):
        self.post(a).remove(b)
        self.pre(b).remove(a)

    def post(self, a):
        return self.__post[a]

    def pre(self, b):
        return self.__pre[b]

    def edges(self):
        for a, post in self.__post.items():
            for b in post:
                yield (a, b)
