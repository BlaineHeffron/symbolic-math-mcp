"""Tests for tensor algebra operations."""

from symbolic_math.tensors import gamma_algebra, tensor_contract


def test_gamma_trace_two():
    """tr(gamma^mu gamma^nu) = 4*g^{mu nu} in 4D."""
    result = gamma_algebra("tr(gamma^mu gamma^nu)", dimension=4)
    assert "4" in result["result"]
    assert "g^" in result["result"] or "g_{" in result["result"] or "g^{mu" in result["result"]


def test_gamma_trace_odd():
    """Trace of odd number of gamma matrices is zero."""
    result = gamma_algebra("tr(gamma^mu gamma^nu gamma^rho)", dimension=4)
    assert result["result"] == "0"


def test_gamma_trace_four():
    """Trace of four gamma matrices."""
    result = gamma_algebra("tr(gamma^a gamma^b gamma^c gamma^d)", dimension=4)
    assert "4" in result["result"]


def test_cadabra_optional():
    """Verify graceful handling when Cadabra2 is not installed."""
    try:
        import cadabra2
        # If cadabra2 is available, skip this test
        return
    except ImportError:
        pass

    try:
        from symbolic_math.tensors import fierz_identity
        fierz_identity("test")
        assert False, "Should have raised ImportError"
    except ImportError as e:
        assert "Cadabra2" in str(e)
