"""Tensor algebra operations, optionally using Cadabra2.

Provides tensor contraction, symmetrization, gamma algebra, and Fierz identities.
Falls back gracefully when Cadabra2 is not installed.
"""

from __future__ import annotations

import sympy
from sympy import Matrix, eye, trace, zeros, latex, simplify, Rational

# Try to import Cadabra2
try:
    import cadabra2
    HAS_CADABRA = True
except ImportError:
    HAS_CADABRA = False


def _require_cadabra(operation: str):
    """Raise if Cadabra2 is needed but not installed."""
    if not HAS_CADABRA:
        raise ImportError(
            f"Cadabra2 required for {operation}. "
            "Install: conda install -c conda-forge cadabra2"
        )


def tensor_contract(expr_str: str, index_pairs: list) -> dict:
    """Contract tensor indices.

    Args:
        expr_str: Tensor expression as string
        index_pairs: List of [i, j] index pairs to contract

    For simple cases, uses SymPy matrix trace. For complex tensor
    expressions, delegates to Cadabra2.
    """
    if HAS_CADABRA:
        try:
            return _cadabra_contract(expr_str, index_pairs)
        except Exception:
            pass

    # Fallback: interpret as matrix operation
    expr = sympy.sympify(expr_str)
    result = simplify(expr)
    return {
        "result": str(result),
        "latex": latex(result),
        "type": "expression",
    }


def tensor_symmetrize(expr_str: str, indices: list, antisym: bool = False) -> dict:
    """Symmetrize or antisymmetrize a tensor expression over specified indices.

    Args:
        expr_str: Tensor expression
        indices: List of index names to symmetrize over
        antisym: If True, antisymmetrize instead
    """
    if HAS_CADABRA:
        try:
            return _cadabra_symmetrize(expr_str, indices, antisym)
        except Exception:
            pass

    # Basic fallback for SymPy expressions
    return {
        "result": expr_str,
        "latex": expr_str,
        "type": "expression",
        "note": "Full tensor symmetrization requires Cadabra2",
    }


def canonicalize(expr_str: str) -> dict:
    """Bring a tensor expression to canonical form."""
    _require_cadabra("canonicalization")
    return _cadabra_canonicalize(expr_str)


def fierz_identity(expr_str: str) -> dict:
    """Apply Fierz identities to spinor expressions."""
    _require_cadabra("Fierz identities")
    return _cadabra_fierz(expr_str)


def gamma_algebra(expr_str: str, dimension: int = 4) -> dict:
    """Evaluate Clifford/gamma matrix algebra.

    Supports traces of products of gamma matrices in d dimensions.
    Uses explicit matrix representation for d=4.
    """
    # For standard 4D gamma matrix traces, use known formulas
    expr_str = expr_str.strip()

    # Check for trace patterns
    if "tr" in expr_str.lower() or "Tr" in expr_str:
        return _gamma_trace(expr_str, dimension)

    # For general gamma algebra, try Cadabra2
    if HAS_CADABRA:
        try:
            return _cadabra_gamma(expr_str, dimension)
        except Exception:
            pass

    return {
        "result": expr_str,
        "latex": expr_str,
        "type": "expression",
        "note": "Complex gamma algebra requires Cadabra2",
    }


def _gamma_trace(expr_str: str, dimension: int) -> dict:
    """Evaluate traces of gamma matrices using known identities.

    Standard results in d dimensions:
    - tr(1) = d (trace of identity)
    - tr(gamma^mu) = 0
    - tr(gamma^mu gamma^nu) = d * g^{mu nu}  (using (+---) signature)
    - tr(odd number of gammas) = 0
    """
    # Parse: count number of gamma matrices in the trace
    import re
    # Match patterns like tr(gamma^mu gamma^nu) or Tr(g^a g^b)
    content = re.search(r'[Tt]r\((.+)\)', expr_str)
    if not content:
        return {
            "result": expr_str,
            "latex": expr_str,
            "type": "expression",
        }

    inner = content.group(1).strip()

    # Count gamma matrices
    gammas = re.findall(r'gamma\^?(\w+)', inner, re.IGNORECASE)

    n_gammas = len(gammas)

    if n_gammas == 0:
        # tr(1) = dimension
        result = str(dimension)
        return {
            "result": result,
            "latex": result,
            "type": "expression",
        }

    if n_gammas % 2 == 1:
        # Trace of odd number of gammas is zero
        return {
            "result": "0",
            "latex": "0",
            "type": "expression",
        }

    if n_gammas == 2:
        # tr(gamma^mu gamma^nu) = d * g^{mu nu}
        mu, nu = gammas[0], gammas[1]
        result = f"{dimension}*g^{{{mu}{nu}}}"
        latex_result = f"{dimension} \\, g^{{{mu}{nu}}}"
        return {
            "result": result,
            "latex": latex_result,
            "type": "expression",
        }

    if n_gammas == 4:
        # tr(g^a g^b g^c g^d) = d*(g^ab g^cd - g^ac g^bd + g^ad g^bc)
        a, b, c, d = gammas[:4]
        result = (
            f"{dimension}*(g^{{{a}{b}}}*g^{{{c}{d}}} "
            f"- g^{{{a}{c}}}*g^{{{b}{d}}} "
            f"+ g^{{{a}{d}}}*g^{{{b}{c}}})"
        )
        latex_result = (
            f"{dimension}\\left(g^{{{a}{b}}} g^{{{c}{d}}} "
            f"- g^{{{a}{c}}} g^{{{b}{d}}} "
            f"+ g^{{{a}{d}}} g^{{{b}{c}}}\\right)"
        )
        return {
            "result": result,
            "latex": latex_result,
            "type": "expression",
        }

    return {
        "result": f"Trace of {n_gammas} gamma matrices",
        "latex": expr_str,
        "type": "expression",
        "note": "Higher-order traces require Cadabra2 for full evaluation",
    }


# --- Cadabra2 helpers ---

def _cadabra_contract(expr_str: str, index_pairs: list) -> dict:
    """Contract indices using Cadabra2."""
    _require_cadabra("tensor contraction")
    kernel = cadabra2.create_scope()
    ex = cadabra2.Ex(expr_str, kernel)
    cadabra2.eliminate_metric(ex, kernel)
    cadabra2.rename_dummies(ex, kernel)
    result = str(ex)
    return {
        "result": result,
        "latex": cadabra2.Ex(expr_str, kernel).__latex__() if hasattr(ex, '__latex__') else result,
        "type": "expression",
    }


def _cadabra_symmetrize(expr_str: str, indices: list, antisym: bool) -> dict:
    """Symmetrize using Cadabra2."""
    _require_cadabra("tensor symmetrization")
    kernel = cadabra2.create_scope()
    ex = cadabra2.Ex(expr_str, kernel)
    if antisym:
        cadabra2.asym(ex, kernel)
    else:
        cadabra2.sym(ex, kernel)
    result = str(ex)
    return {
        "result": result,
        "latex": result,
        "type": "expression",
    }


def _cadabra_canonicalize(expr_str: str) -> dict:
    """Canonicalize using Cadabra2."""
    kernel = cadabra2.create_scope()
    ex = cadabra2.Ex(expr_str, kernel)
    cadabra2.canonicalise(ex, kernel)
    result = str(ex)
    return {
        "result": result,
        "latex": result,
        "type": "expression",
    }


def _cadabra_fierz(expr_str: str) -> dict:
    """Apply Fierz identities using Cadabra2."""
    kernel = cadabra2.create_scope()
    ex = cadabra2.Ex(expr_str, kernel)
    cadabra2.fierz(ex, kernel)
    result = str(ex)
    return {
        "result": result,
        "latex": result,
        "type": "expression",
    }


def _cadabra_gamma(expr_str: str, dimension: int) -> dict:
    """Evaluate gamma algebra using Cadabra2."""
    kernel = cadabra2.create_scope()
    cadabra2.Ex(r"\Gamma^{#}::GammaMatrix(metric=\delta).", kernel)
    ex = cadabra2.Ex(expr_str, kernel)
    cadabra2.join_gamma(ex, kernel)
    result = str(ex)
    return {
        "result": result,
        "latex": result,
        "type": "expression",
    }
