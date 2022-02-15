import itertools

from .graph import Graph


class ProductGraph:
	def __init__(self, buchi, ts):
		super().__init__()
		self._buchi = buchi
		self._ts = ts
		self.initial = {(b, t) for b, t in itertools.product(buchi.initial, ts.initial)}

	def post(self, src):
		src_buchi_node, src_ts_node = src
		buchi_post = self._buchi.post(src_buchi_node)
		ts_post = self._ts.post(src_ts_node)
		result = set()
		for node = in itertools.product(buchi_post, ts_post):
			buchi_post_node, ts_post_node = node
			labels = self._ts.labels(ts_post_node)
			guard = self._buchi.guard(src_buchi_node, buchi_post_node)
			if guard.check(labels):
				result.add(node)
		return result

	def pre(self, dst):
		dst_buchi_node, dst_ts_node = dst
		buchi_pre = self._buchi.pre(dst_buchi_node)
		ts_pre = self._ts.pre(dst_ts_node)
		dst_labels = self._ts.labels(dst_ts_node)
		result = set()
		for node in itertools.product(buchi_pre, ts_pre):
			buchi_pre_node, ts_pre_node = node
			labels = self._ts.labels(dst_ts_node)
			guard = self._buchi.guard(buchi_pre_node, dst_buchi_node)
			if guard.check(labels):
				result.add(node)
		return result


