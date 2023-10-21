import itertools
from .buchi import Buchi
from .ts import TransitionSystem

type ProductNode = tuple[str, str]


class ProductGraph:
    _buchi: Buchi
    _ts: TransitionSystem
    initial: set[ProductNode]

    def __init__(self, buchi: Buchi, ts: TransitionSystem):
        super().__init__()
        self._buchi = buchi
        self._ts = ts
        self.initial = {(b, t) for b, t in itertools.product(buchi.initial, ts.initial)}

    def post(self, src: ProductNode) -> set[ProductNode]:
        src_buchi_node, src_ts_node = src
        buchi_post = self._buchi.post(src_buchi_node)
        ts_post = self._ts.post(src_ts_node)
        result = set()
        for node in itertools.product(buchi_post, ts_post):
            buchi_post_node, ts_post_node = node
            labels = self._ts.labels(ts_post_node)
            guard = self._buchi.guard(src_buchi_node, buchi_post_node)
            if guard.check(labels):
                result.add(node)
        return result

    def pre(self, dst: ProductNode) -> set[ProductNode]:
        dst_buchi_node, dst_ts_node = dst
        buchi_pre = self._buchi.pre(dst_buchi_node)
        ts_pre = self._ts.pre(dst_ts_node)
        dst_labels = self._ts.labels(dst_ts_node)
        result = set()
        for node in itertools.product(buchi_pre, ts_pre):
            buchi_pre_node, ts_pre_node = node
            guard = self._buchi.guard(buchi_pre_node, dst_buchi_node)
            if guard.check(dst_labels):
                result.add(node)
        return result
