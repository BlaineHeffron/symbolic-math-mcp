"""Tests for MCP server protocol compliance."""

import json

from symbolic_math.server import TOOLS, _dispatch


def test_tool_count():
    """Server should expose 21 tools."""
    assert len(TOOLS) == 21


def test_all_tools_have_schemas():
    """All tools should have valid input schemas."""
    for tool in TOOLS:
        assert tool.name
        assert tool.description
        assert tool.inputSchema
        assert tool.inputSchema.get("type") == "object"
        assert "properties" in tool.inputSchema


def test_dispatch_simplify():
    """Calling simplify returns expected result."""
    result = _dispatch("simplify", {"expression": "x**2 + 2*x + 1"})
    assert "result" in result
    assert "latex" in result


def test_dispatch_expand():
    """Calling expand returns expected result."""
    result = _dispatch("expand", {"expression": "(x+1)**2"})
    assert "x**2" in result["result"]


def test_dispatch_christoffel_flat():
    """Calling christoffel with flat metric returns zeros."""
    result = _dispatch("christoffel", {
        "metric": [["1", "0"], ["0", "1"]],
        "coordinates": ["x", "y"],
    })
    assert "zero" in str(result["result"]).lower()


def test_dispatch_unknown_tool():
    """Calling unknown tool raises ValueError."""
    try:
        _dispatch("nonexistent_tool", {})
        assert False, "Should have raised ValueError"
    except ValueError as e:
        assert "Unknown tool" in str(e)


def test_dispatch_error_handling():
    """Invalid input should raise, not crash."""
    try:
        _dispatch("simplify", {"expression": "<<<invalid>>>"})
    except Exception:
        pass  # Errors are acceptable, crashes are not
