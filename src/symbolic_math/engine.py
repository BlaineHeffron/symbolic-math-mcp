"""Core symbolic algebra engine using SymPy (with optional SymEngine acceleration).

All functions accept string or LaTeX expressions and return dicts with
'result' (string), 'latex' (LaTeX), and 'type' fields.
"""

from __future__ import annotations

import re
from typing import Optional

import sympy
from sympy import (
    Symbol,
    latex,
    oo,
    simplify as _simplify,
    expand as _expand,
    factor as _factor,
    diff,
    integrate as _integrate,
    series as _series,
    solve as _solve,
    limit as _limit,
    S,
)
from sympy.parsing.sympy_parser import (
    parse_expr as _sympy_parse,
    standard_transformations,
    implicit_multiplication_application,
    convert_xor,
)

# Try to use SymEngine for fast operations
try:
    import symengine
    HAS_SYMENGINE = True
except ImportError:
    HAS_SYMENGINE = False

TRANSFORMATIONS = standard_transformations + (
    implicit_multiplication_application,
    convert_xor,
)


def _latex_to_sympy_str(latex_str: str) -> str:
    """Convert common LaTeX patterns to SymPy-parseable strings."""
    s = latex_str.strip()
    # Remove display math delimiters
    s = re.sub(r'^\$\$?|\$\$?$', '', s)
    s = re.sub(r'^\\[(\[]|\\[)\]]$', '', s)
    # \frac{a}{b} -> (a)/(b)
    while r'\frac' in s:
        s = re.sub(r'\\frac\{([^{}]*)\}\{([^{}]*)\}', r'(\1)/(\2)', s)
    # \sqrt{x} -> sqrt(x), \sqrt[n]{x} -> x**(1/n)
    s = re.sub(r'\\sqrt\[([^]]*)\]\{([^{}]*)\}', r'(\2)**(1/(\1))', s)
    s = re.sub(r'\\sqrt\{([^{}]*)\}', r'sqrt(\1)', s)
    # Common functions
    for fn in ('sin', 'cos', 'tan', 'log', 'ln', 'exp', 'arcsin', 'arccos', 'arctan',
               'sinh', 'cosh', 'tanh', 'sec', 'csc', 'cot'):
        s = s.replace(f'\\{fn}', fn)
    s = s.replace('\\ln', 'log')
    # \pi, \infty, \alpha etc.
    s = s.replace('\\pi', 'pi')
    s = s.replace('\\infty', 'oo')
    s = s.replace('\\inf', 'oo')
    # Superscripts: x^{2} -> x**(2), x^2 -> x**2
    s = re.sub(r'\^{([^{}]*)}', r'**(\1)', s)
    s = re.sub(r'\^(\w)', r'**\1', s)
    # Subscripts: just remove for simple cases
    s = re.sub(r'_\{([^{}]*)\}', r'_\1', s)
    # \left, \right, \cdot
    s = s.replace('\\left', '').replace('\\right', '')
    s = s.replace('\\cdot', '*')
    s = s.replace('\\times', '*')
    # Whitespace cleanup
    s = s.replace('{', '(').replace('}', ')')
    s = re.sub(r'\s+', ' ', s).strip()
    return s


def parse_expr(input_str: str) -> sympy.Basic:
    """Parse a string or LaTeX expression into a SymPy expression."""
    s = input_str.strip()

    # Try direct SymPy parse first
    try:
        return _sympy_parse(s, transformations=TRANSFORMATIONS)
    except Exception:
        pass

    # Try as LaTeX
    converted = _latex_to_sympy_str(s)
    try:
        return _sympy_parse(converted, transformations=TRANSFORMATIONS)
    except Exception:
        pass

    # Try sympy.parsing.latex if available
    try:
        from sympy.parsing.latex import parse_latex
        return parse_latex(s)
    except Exception:
        pass

    raise ValueError(f"Could not parse expression: {input_str}")


def _to_symengine(expr: sympy.Basic):
    """Convert SymPy expression to SymEngine if available."""
    if HAS_SYMENGINE:
        try:
            return symengine.sympify(expr)
        except Exception:
            pass
    return None


def _from_symengine(se_expr) -> sympy.Basic:
    """Convert SymEngine expression back to SymPy."""
    return sympy.sympify(str(se_expr))


def to_output(expr, expr_type: str = "expression") -> dict:
    """Convert a SymPy expression to the standard output dict."""
    if isinstance(expr, (list, tuple)):
        result_str = str(expr)
        latex_str = ", ".join(latex(e) for e in expr)
    else:
        result_str = str(expr)
        latex_str = latex(expr)
    return {
        "result": result_str,
        "latex": latex_str,
        "type": expr_type,
    }


def simplify(expr_str: str) -> dict:
    """Simplify a symbolic expression."""
    expr = parse_expr(expr_str)
    if HAS_SYMENGINE:
        se = _to_symengine(expr)
        if se is not None:
            try:
                result = _from_symengine(symengine.expand(se))
                result = _simplify(result)
                return to_output(result)
            except Exception:
                pass
    result = _simplify(expr)
    return to_output(result)


def expand(expr_str: str) -> dict:
    """Expand products and powers."""
    expr = parse_expr(expr_str)
    if HAS_SYMENGINE:
        se = _to_symengine(expr)
        if se is not None:
            try:
                return to_output(_from_symengine(symengine.expand(se)))
            except Exception:
                pass
    return to_output(_expand(expr))


def factor(expr_str: str) -> dict:
    """Factor a polynomial expression."""
    expr = parse_expr(expr_str)
    return to_output(_factor(expr))


def differentiate(expr_str: str, var: str, order: int = 1) -> dict:
    """Symbolic differentiation."""
    expr = parse_expr(expr_str)
    v = Symbol(var)
    if HAS_SYMENGINE:
        se = _to_symengine(expr)
        if se is not None:
            try:
                sv = symengine.Symbol(var)
                for _ in range(order):
                    se = symengine.diff(se, sv)
                return to_output(_from_symengine(se))
            except Exception:
                pass
    result = diff(expr, v, order)
    return to_output(result)


def integrate(expr_str: str, var: str,
              lower: Optional[str] = None, upper: Optional[str] = None) -> dict:
    """Symbolic integration (definite or indefinite). Uses SymPy."""
    expr = parse_expr(expr_str)
    v = Symbol(var)
    if lower is not None and upper is not None:
        lo = parse_expr(lower)
        hi = parse_expr(upper)
        result = _integrate(expr, (v, lo, hi))
    else:
        result = _integrate(expr, v)
    return to_output(result)


def series(expr_str: str, var: str, point: str = "0", order: int = 6) -> dict:
    """Taylor/Laurent series expansion."""
    expr = parse_expr(expr_str)
    v = Symbol(var)
    p = parse_expr(point)
    result = _series(expr, v, p, n=order)
    return to_output(result)


def solve(expr_str: str, var: str) -> dict:
    """Solve an equation or expression = 0."""
    expr = parse_expr(expr_str)
    v = Symbol(var)
    result = _solve(expr, v)
    return to_output(result, expr_type="solutions")


def substitute(expr_str: str, substitutions: dict) -> dict:
    """Substitute values/expressions into an expression."""
    expr = parse_expr(expr_str)
    subs = {}
    for k, val in substitutions.items():
        subs[Symbol(k)] = parse_expr(str(val))
    result = expr.subs(subs)
    return to_output(result)


def limit(expr_str: str, var: str, point: str) -> dict:
    """Compute a limit."""
    expr = parse_expr(expr_str)
    v = Symbol(var)
    p = parse_expr(point)
    result = _limit(expr, v, p)
    return to_output(result)


def latex_render(expr_str: str) -> dict:
    """Parse an expression and render it as LaTeX without computation."""
    expr = parse_expr(expr_str)
    return to_output(expr)
