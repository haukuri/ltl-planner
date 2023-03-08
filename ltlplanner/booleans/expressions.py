import abc
import itertools


class Expression(abc.ABC):
    name = "Expression"

    def __iter__(self):
        raise NotImplementedError()

    def __eq__(self, o: object) -> bool:
        return isinstance(o, type(self))

    def check(self, label) -> bool:
        raise NotImplementedError()

    def distance(self, label):
        raise NotImplementedError()

    def nnf(self):
        return self

    def children(self) -> list["Expression"]:
        return []

    def symbols(self) -> set[str]:
        s = set()
        for child in self.children():
            child_symbols = child.symbols()
            s.update(child_symbols)
        return s


class SymbolExpression(Expression):
    def __init__(self, name):
        self.name = name
        self.symbol = name

    def __repr__(self):
        return "SymbolExpression(%s)" % str(self.symbol)

    def __iter__(self):
        for expr in [self]:
            yield expr

    def __eq__(self, o: object) -> bool:
        return super().__eq__(o) and self.symbol == o.symbol

    def check(self, label):
        return self.symbol in label

    def distance(self, label):
        if self.symbol in label:
            return 0
        else:
            return 1

    def symbols(self):
        return {self.symbol}


class NotSymbolExpression(Expression):
    def __init__(self, name):
        self.name = "!%s" % name
        self.symbol = name

    def __repr__(self):
        return "NotSymbolExpression(%s)" % str(self.symbol)

    def __iter__(self):
        for expr in [self]:
            yield expr

    def __eq__(self, o: object) -> bool:
        return super().__eq__(o) and self.symbol == o.symbol

    def check(self, label):
        return self.symbol not in label

    def distance(self, label):
        if self.symbol not in label:
            return 0
        else:
            return 1

    def symbols(self):
        return {self.symbol}


class TrueExpression(Expression):
    name = "TRUE"

    def __init__(self):
        pass

    def __repr__(self):
        return "TrueExpression()"

    def __iter__(self):
        for expr in [self]:
            yield expr

    def check(self, label):
        return True

    def distance(self, label):
        return 0


class NotExpression(Expression):
    name = "NOT"

    def __init__(self, inner):
        self.inner = inner

    def __repr__(self):
        return "NotExpression(%s)" % str(self.inner)

    def __iter__(self):
        for expr in itertools.chain([self], self.inner):
            yield expr

    def __eq__(self, o):
        return super().__eq__(o) and self.inner == o.inner

    def children(self):
        return [self.inner]

    def check(self, label):
        return not self.inner.check(label)

    def nnf(self):
        if isinstance(self.inner, SymbolExpression):
            s = NotSymbolExpression(self.inner.name)
            return s
        elif isinstance(self.inner, ORExpression):
            left = NotExpression(self.inner.left).nnf()
            right = NotExpression(self.inner.right).nnf()
            s = ANDExpression(left, right)
            return s
        elif isinstance(self.inner, ANDExpression):
            left = NotExpression(self.inner.left).nnf()
            right = NotExpression(self.inner.right).nnf()
            s = ORExpression(left, right)
            return s
        raise Exception("Unexpected child of NotExpression")


class BinExpression(Expression):
    def __init__(self, left, right):
        self.left = left
        self.right = right

    def __iter__(self):
        for expr in itertools.chain([self], self.left, self.right):
            yield expr

    def __eq__(self, o: object) -> bool:
        return super().__eq__(o) and self.left == o.left and self.right == o.right

    def children(self):
        return [self.left, self.right]

    def nnf(self):
        self.left = self.left.nnf()
        self.right = self.right.nnf()
        return self


class ORExpression(BinExpression):
    name = "OR"

    def __repr__(self):
        return "ORExpression(%s, %s)" % (str(self.left), str(self.right))

    def check(self, label):
        return self.left.check(label) or self.right.check(label)

    def distance(self, label):
        ldist = self.left.distance(label)
        rdist = self.right.distance(label)
        return min([ldist, rdist])


class ANDExpression(BinExpression):
    name = "AND"

    def __repr__(self):
        return "ANDExpression(%s, %s)" % (str(self.left), str(self.right))

    def check(self, label):
        return self.left.check(label) and self.right.check(label)

    def distance(self, label):
        return self.left.distance(label) + self.right.distance(label)
