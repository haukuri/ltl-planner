from .lexer import get_lexer
from .expressions import (
    ORExpression,
    ANDExpression,
    NotExpression,
    SymbolExpression,
    TrueExpression,
    Expression,
)


class Parser:
    def __init__(self, formula):
        lexer = get_lexer()
        lexer.input(formula)
        self.formula = formula
        self.tokens = list(lexer)

    def symbols(self):
        syms = list()
        for token in self.tokens:
            if token.type == "SYMBOL":
                syms += [token.value]
        return list(set(syms))

    def parse(self, nnf=True):
        expr = self.orx()
        if nnf:
            expr = expr.nnf()
        expr.formula = self.formula
        return expr

    def orx(self):
        lhs = self.andx()
        if len(self.tokens) == 0 or self.tokens[0].type == "RPAREN":
            return lhs
        elif self.tokens[0].type == "OR":
            self.tokens.pop(0)
            rhs = self.andx()
            lhs = ORExpression(lhs, rhs)
            while len(self.tokens) > 0 and self.tokens[0].type == "OR":
                self.tokens.pop(0)
                rhs = self.andx()
                lhs = ORExpression(lhs, rhs)
            return lhs
        else:
            raise Exception(
                "Expected OR, RPAREN or nothing but got %s" % self.tokens[0]
            )

    def andx(self):
        lhs = self.notx()
        if len(self.tokens) == 0 or self.tokens[0].type in ["OR", "RPAREN"]:
            return lhs
        elif self.tokens[0].type == "AND":
            self.tokens.pop(0)
            rhs = self.notx()
            lhs = ANDExpression(lhs, rhs)
            while len(self.tokens) > 0 and self.tokens[0].type == "AND":
                self.tokens.pop(0)
                rhs = self.notx()
                lhs = ANDExpression(lhs, rhs)
            return lhs
        else:
            raise Exception("Expected OR, AND or nothing but got %s" % self.tokens[0])

    def notx(self):
        if self.tokens[0].type == "NOT":
            self.tokens.pop(0)
            return NotExpression(self.parx())
        else:
            return self.parx()

    def parx(self):
        if self.tokens[0].type == "LPAREN":
            self.tokens.pop(0)
            expr = self.orx()
            if self.tokens[0].type != "RPAREN":
                raise Exception("Expected RPAREN but got %s" % self.tokens[0])
            self.tokens.pop(0)
        elif self.tokens[0].type == "SYMBOL":
            expr = SymbolExpression(self.tokens[0].value)
            self.tokens.pop(0)
        elif self.tokens[0].type == "TRUE":
            expr = TrueExpression()
            self.tokens.pop(0)
        else:
            raise Exception("Expected LPAREN or SYMBOL but got %s" % self.tokens[0])
        return expr


def parse(formula, nnf=True) -> Expression:
    parser = Parser(formula)
    return parser.parse(nnf=nnf)
