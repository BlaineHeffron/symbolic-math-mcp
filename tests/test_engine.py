"""Tests for the core symbolic algebra engine."""

from symbolic_math.engine import (
    expand, factor, differentiate, integrate, series, solve,
    substitute, simplify, limit, latex_render, parse_expr,
)


def test_expand():
    result = expand("(x+1)**3")
    assert "x**3" in result["result"]
    assert "3*x**2" in result["result"]
    assert "3*x" in result["result"]
    assert "1" in result["result"]


def test_factor():
    result = factor("x**2 - 1")
    assert "(x - 1)" in result["result"]
    assert "(x + 1)" in result["result"]


def test_differentiate():
    result = differentiate("x**3 + 2*x", "x")
    assert "3*x**2 + 2" == result["result"]


def test_differentiate_higher_order():
    result = differentiate("x**4", "x", order=2)
    assert "12*x**2" == result["result"]


def test_integrate():
    result = integrate("x**2", "x")
    assert "x**3/3" == result["result"]


def test_definite_integral():
    result = integrate("exp(-x**2)", "x", "-oo", "oo")
    assert "sqrt(pi)" in result["result"]


def test_series():
    result = series("sin(x)", "x", "0", 6)
    assert "x" in result["result"]
    assert "x**3" in result["result"] or "/6" in result["result"]


def test_solve():
    result = solve("x**2 - 4", "x")
    assert "2" in result["result"]
    assert "-2" in result["result"]


def test_substitute():
    result = substitute("x**2 + y", {"x": "3", "y": "1"})
    assert result["result"] == "10"


def test_simplify_trig():
    result = simplify("sin(x)**2 + cos(x)**2")
    assert result["result"] == "1"


def test_limit():
    result = limit("sin(x)/x", "x", "0")
    assert result["result"] == "1"


def test_latex_output():
    result = expand("(x+1)**2")
    assert result["latex"]
    assert len(result["latex"]) > 0


def test_latex_render():
    result = latex_render("x**2 + 1")
    assert "x" in result["latex"]


def test_parse_latex_frac():
    expr = parse_expr(r"\frac{x^2}{2}")
    assert str(expr) == "x**2/2"
