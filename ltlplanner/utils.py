from collections import defaultdict

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
