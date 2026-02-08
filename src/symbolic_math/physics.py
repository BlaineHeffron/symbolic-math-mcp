"""General relativity and field theory computations.

Computes Christoffel symbols, Riemann tensor, Ricci tensor/scalar,
and Einstein tensor from a given metric.
"""

from __future__ import annotations

from typing import Optional

import sympy
from sympy import Matrix, Symbol, simplify, zeros, latex, Rational


def _parse_metric(metric: list[list[str]], coords: list[str]) -> tuple[Matrix, list[Symbol]]:
    """Parse metric matrix and coordinate list into SymPy objects."""
    coord_syms = [Symbol(c) for c in coords]
    n = len(coords)
    g = zeros(n)
    for i in range(n):
        for j in range(n):
            g[i, j] = sympy.sympify(metric[i][j])
    return g, coord_syms


def _metric_inverse(g: Matrix) -> Matrix:
    """Compute the inverse metric tensor."""
    return g.inv()


def christoffel(metric: list[list[str]], coords: list[str]) -> dict:
    """Compute Christoffel symbols of the second kind.

    Returns Gamma^sigma_{mu nu} as a nested dict of non-zero components.
    """
    g, coord_syms = _parse_metric(metric, coords)
    g_inv = _metric_inverse(g)
    n = len(coords)

    gamma = {}
    components = {}

    for sigma in range(n):
        for mu in range(n):
            for nu in range(mu, n):  # symmetric in lower indices
                val = Rational(0)
                for rho in range(n):
                    val += Rational(1, 2) * g_inv[sigma, rho] * (
                        sympy.diff(g[nu, rho], coord_syms[mu])
                        + sympy.diff(g[mu, rho], coord_syms[nu])
                        - sympy.diff(g[mu, nu], coord_syms[rho])
                    )
                val = simplify(val)
                if val != 0:
                    key = f"Gamma^{coords[sigma]}_{{{coords[mu]}{coords[nu]}}}"
                    components[key] = {
                        "value": str(val),
                        "latex": latex(val),
                    }

    return {
        "result": components if components else "All Christoffel symbols are zero",
        "latex": _christoffel_latex(components) if components else "\\Gamma^{\\sigma}_{\\mu\\nu} = 0",
        "type": "christoffel_symbols",
    }


def _christoffel_latex(components: dict) -> str:
    """Format Christoffel symbols as LaTeX."""
    lines = []
    for key, val in components.items():
        lines.append(f"{key} = {val['latex']}")
    return " \\\\\n".join(lines)


def riemann_tensor(metric: list[list[str]], coords: list[str]) -> dict:
    """Compute the Riemann curvature tensor R^rho_{sigma mu nu}.

    Returns non-zero components.
    """
    g, coord_syms = _parse_metric(metric, coords)
    g_inv = _metric_inverse(g)
    n = len(coords)

    # First compute Christoffel symbols as a 3D array
    gamma = _christoffel_array(g, g_inv, coord_syms)

    components = {}
    for rho in range(n):
        for sigma in range(n):
            for mu in range(n):
                for nu in range(mu + 1, n):  # antisymmetric in mu,nu
                    val = (
                        sympy.diff(gamma[rho][nu][sigma], coord_syms[mu])
                        - sympy.diff(gamma[rho][mu][sigma], coord_syms[nu])
                    )
                    for lam in range(n):
                        val += (
                            gamma[rho][mu][lam] * gamma[lam][nu][sigma]
                            - gamma[rho][nu][lam] * gamma[lam][mu][sigma]
                        )
                    val = simplify(val)
                    if val != 0:
                        key = f"R^{coords[rho]}_{{{coords[sigma]}{coords[mu]}{coords[nu]}}}"
                        components[key] = {
                            "value": str(val),
                            "latex": latex(val),
                        }

    return {
        "result": components if components else "Riemann tensor is identically zero (flat space)",
        "latex": _format_tensor_latex(components, "R") if components else "R^{\\rho}_{\\sigma\\mu\\nu} = 0",
        "type": "riemann_tensor",
    }


def ricci_tensor(metric: list[list[str]], coords: list[str]) -> dict:
    """Compute the Ricci tensor R_{mu nu} by contracting the Riemann tensor."""
    g, coord_syms = _parse_metric(metric, coords)
    g_inv = _metric_inverse(g)
    n = len(coords)

    gamma = _christoffel_array(g, g_inv, coord_syms)
    riemann = _riemann_array(gamma, coord_syms, n)

    components = {}
    ricci_matrix = zeros(n)
    for mu in range(n):
        for nu in range(mu, n):  # symmetric
            val = Rational(0)
            for rho in range(n):
                val += riemann[rho][mu][rho][nu]
            val = simplify(val)
            ricci_matrix[mu, nu] = val
            ricci_matrix[nu, mu] = val
            if val != 0:
                key = f"R_{{{coords[mu]}{coords[nu]}}}"
                components[key] = {
                    "value": str(val),
                    "latex": latex(val),
                }

    return {
        "result": components if components else "Ricci tensor is identically zero",
        "latex": _format_tensor_latex(components, "R") if components else "R_{\\mu\\nu} = 0",
        "type": "ricci_tensor",
        "matrix": [[str(ricci_matrix[i, j]) for j in range(n)] for i in range(n)],
    }


def ricci_scalar(metric: list[list[str]], coords: list[str]) -> dict:
    """Compute the Ricci scalar R = g^{mu nu} R_{mu nu}."""
    g, coord_syms = _parse_metric(metric, coords)
    g_inv = _metric_inverse(g)
    n = len(coords)

    gamma = _christoffel_array(g, g_inv, coord_syms)
    riemann = _riemann_array(gamma, coord_syms, n)

    # Ricci tensor
    ricci = zeros(n)
    for mu in range(n):
        for nu in range(n):
            val = Rational(0)
            for rho in range(n):
                val += riemann[rho][mu][rho][nu]
            ricci[mu, nu] = val

    # Contract with inverse metric
    scalar = Rational(0)
    for mu in range(n):
        for nu in range(n):
            scalar += g_inv[mu, nu] * ricci[mu, nu]
    scalar = simplify(scalar)

    return {
        "result": str(scalar),
        "latex": latex(scalar),
        "type": "scalar",
    }


def einstein_tensor(metric: list[list[str]], coords: list[str]) -> dict:
    """Compute the Einstein tensor G_{mu nu} = R_{mu nu} - 1/2 R g_{mu nu}."""
    g, coord_syms = _parse_metric(metric, coords)
    g_inv = _metric_inverse(g)
    n = len(coords)

    gamma = _christoffel_array(g, g_inv, coord_syms)
    riemann = _riemann_array(gamma, coord_syms, n)

    # Ricci tensor
    ricci = zeros(n)
    for mu in range(n):
        for nu in range(n):
            val = Rational(0)
            for rho in range(n):
                val += riemann[rho][mu][rho][nu]
            ricci[mu, nu] = val

    # Ricci scalar
    scalar = Rational(0)
    for mu in range(n):
        for nu in range(n):
            scalar += g_inv[mu, nu] * ricci[mu, nu]
    scalar = simplify(scalar)

    # Einstein tensor
    components = {}
    einstein_matrix = zeros(n)
    for mu in range(n):
        for nu in range(mu, n):
            val = simplify(ricci[mu, nu] - Rational(1, 2) * scalar * g[mu, nu])
            einstein_matrix[mu, nu] = val
            einstein_matrix[nu, mu] = val
            if val != 0:
                key = f"G_{{{coords[mu]}{coords[nu]}}}"
                components[key] = {
                    "value": str(val),
                    "latex": latex(val),
                }

    return {
        "result": components if components else "Einstein tensor is identically zero (vacuum solution)",
        "latex": _format_tensor_latex(components, "G") if components else "G_{\\mu\\nu} = 0",
        "type": "einstein_tensor",
        "matrix": [[str(einstein_matrix[i, j]) for j in range(n)] for i in range(n)],
    }


def covariant_derivative(expr_str: str, metric: list[list[str]],
                         coords: list[str], direction: str) -> dict:
    """Compute the covariant derivative of a scalar expression."""
    g, coord_syms = _parse_metric(metric, coords)
    g_inv = _metric_inverse(g)

    expr = sympy.sympify(expr_str)
    dir_idx = coords.index(direction)

    # For a scalar, covariant derivative = partial derivative
    result = simplify(sympy.diff(expr, coord_syms[dir_idx]))

    return {
        "result": str(result),
        "latex": latex(result),
        "type": "expression",
    }


def evaluate_component(expr_str: str, substitutions: dict) -> dict:
    """Evaluate a tensor component expression numerically."""
    expr = sympy.sympify(expr_str)
    subs = {Symbol(k): sympy.sympify(v) for k, v in substitutions.items()}
    result = simplify(expr.subs(subs))
    return {
        "result": str(result),
        "latex": latex(result),
        "type": "expression",
    }


# --- Internal helpers ---

def _christoffel_array(g: Matrix, g_inv: Matrix, coords: list[Symbol]) -> list:
    """Compute Christoffel symbols as a 3D nested list."""
    n = len(coords)
    gamma = [[[Rational(0) for _ in range(n)] for _ in range(n)] for _ in range(n)]
    for sigma in range(n):
        for mu in range(n):
            for nu in range(n):
                val = Rational(0)
                for rho in range(n):
                    val += Rational(1, 2) * g_inv[sigma, rho] * (
                        sympy.diff(g[nu, rho], coords[mu])
                        + sympy.diff(g[mu, rho], coords[nu])
                        - sympy.diff(g[mu, nu], coords[rho])
                    )
                gamma[sigma][mu][nu] = simplify(val)
    return gamma


def _riemann_array(gamma: list, coords: list[Symbol], n: int) -> list:
    """Compute Riemann tensor as a 4D nested list."""
    riemann = [[[[Rational(0) for _ in range(n)] for _ in range(n)]
                for _ in range(n)] for _ in range(n)]
    for rho in range(n):
        for sigma in range(n):
            for mu in range(n):
                for nu in range(n):
                    val = (
                        sympy.diff(gamma[rho][nu][sigma], coords[mu])
                        - sympy.diff(gamma[rho][mu][sigma], coords[nu])
                    )
                    for lam in range(n):
                        val += (
                            gamma[rho][mu][lam] * gamma[lam][nu][sigma]
                            - gamma[rho][nu][lam] * gamma[lam][mu][sigma]
                        )
                    riemann[rho][sigma][mu][nu] = simplify(val)
    return riemann


def _format_tensor_latex(components: dict, symbol: str) -> str:
    """Format tensor components as LaTeX."""
    lines = []
    for key, val in components.items():
        lines.append(f"{key} = {val['latex']}")
    return " \\\\\n".join(lines)
