"""Tests for GR physics operations."""

from symbolic_math.physics import (
    christoffel, riemann_tensor, ricci_tensor, ricci_scalar, einstein_tensor,
)


def test_flat_metric_christoffel():
    """Christoffel symbols of Minkowski metric should all be zero."""
    flat = [["-1", "0", "0", "0"],
            ["0", "1", "0", "0"],
            ["0", "0", "1", "0"],
            ["0", "0", "0", "1"]]
    coords = ["t", "x", "y", "z"]
    result = christoffel(flat, coords)
    assert result["result"] == "All Christoffel symbols are zero"


def test_flat_riemann():
    """Riemann tensor of flat metric is zero."""
    flat = [["-1", "0", "0", "0"],
            ["0", "1", "0", "0"],
            ["0", "0", "1", "0"],
            ["0", "0", "0", "1"]]
    coords = ["t", "x", "y", "z"]
    result = riemann_tensor(flat, coords)
    assert "zero" in str(result["result"]).lower()


def test_sphere_ricci_scalar():
    """Ricci scalar of 2-sphere metric = 2/r^2."""
    sphere = [["r**2", "0"], ["0", "r**2*sin(theta)**2"]]
    coords = ["theta", "phi"]
    result = ricci_scalar(sphere, coords)
    assert result["result"] == "2/r**2"


def test_flat_2d_christoffel():
    """Flat 2D metric has zero Christoffel symbols."""
    flat2d = [["1", "0"], ["0", "1"]]
    coords = ["x", "y"]
    result = christoffel(flat2d, coords)
    assert result["result"] == "All Christoffel symbols are zero"
