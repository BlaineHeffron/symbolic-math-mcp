"""MCP server for symbolic mathematics and tensor algebra."""

import asyncio
import json
import traceback
from typing import Any

import mcp.server.stdio
from mcp.server.lowlevel import NotificationOptions, Server
from mcp.server.models import InitializationOptions
from mcp import types

from symbolic_math import engine, tensors, physics


server = Server("symbolic-math")


TOOLS = [
    # --- Core algebra (engine.py) ---
    types.Tool(
        name="simplify",
        description="Simplify a symbolic expression",
        inputSchema={
            "type": "object",
            "properties": {
                "expression": {"type": "string", "description": "Expression to simplify"},
            },
            "required": ["expression"],
        },
    ),
    types.Tool(
        name="expand",
        description="Expand products and powers in an expression",
        inputSchema={
            "type": "object",
            "properties": {
                "expression": {"type": "string", "description": "Expression to expand"},
            },
            "required": ["expression"],
        },
    ),
    types.Tool(
        name="factor",
        description="Factor a polynomial expression",
        inputSchema={
            "type": "object",
            "properties": {
                "expression": {"type": "string", "description": "Expression to factor"},
            },
            "required": ["expression"],
        },
    ),
    types.Tool(
        name="differentiate",
        description="Symbolic differentiation (partial derivatives)",
        inputSchema={
            "type": "object",
            "properties": {
                "expression": {"type": "string", "description": "Expression to differentiate"},
                "variable": {"type": "string", "description": "Variable to differentiate with respect to"},
                "order": {"type": "integer", "description": "Order of derivative (default: 1)", "default": 1},
            },
            "required": ["expression", "variable"],
        },
    ),
    types.Tool(
        name="integrate",
        description="Symbolic integration (definite or indefinite)",
        inputSchema={
            "type": "object",
            "properties": {
                "expression": {"type": "string", "description": "Expression to integrate"},
                "variable": {"type": "string", "description": "Variable of integration"},
                "lower": {"type": "string", "description": "Lower bound (for definite integral)"},
                "upper": {"type": "string", "description": "Upper bound (for definite integral)"},
            },
            "required": ["expression", "variable"],
        },
    ),
    types.Tool(
        name="series",
        description="Taylor/Laurent series expansion",
        inputSchema={
            "type": "object",
            "properties": {
                "expression": {"type": "string", "description": "Expression to expand"},
                "variable": {"type": "string", "description": "Variable for expansion"},
                "point": {"type": "string", "description": "Point to expand around (default: 0)", "default": "0"},
                "order": {"type": "integer", "description": "Number of terms (default: 6)", "default": 6},
            },
            "required": ["expression", "variable"],
        },
    ),
    types.Tool(
        name="solve",
        description="Solve an equation (expression = 0) for a variable",
        inputSchema={
            "type": "object",
            "properties": {
                "expression": {"type": "string", "description": "Expression to solve (set equal to 0)"},
                "variable": {"type": "string", "description": "Variable to solve for"},
            },
            "required": ["expression", "variable"],
        },
    ),
    types.Tool(
        name="substitute",
        description="Substitute values or expressions into an expression",
        inputSchema={
            "type": "object",
            "properties": {
                "expression": {"type": "string", "description": "Expression to substitute into"},
                "substitutions": {"type": "object", "description": "Mapping of variable names to values/expressions"},
            },
            "required": ["expression", "substitutions"],
        },
    ),
    types.Tool(
        name="limit",
        description="Compute the limit of an expression",
        inputSchema={
            "type": "object",
            "properties": {
                "expression": {"type": "string", "description": "Expression to take limit of"},
                "variable": {"type": "string", "description": "Variable approaching the point"},
                "point": {"type": "string", "description": "Point the variable approaches"},
            },
            "required": ["expression", "variable", "point"],
        },
    ),
    types.Tool(
        name="latex_render",
        description="Parse an expression and render it as LaTeX",
        inputSchema={
            "type": "object",
            "properties": {
                "expression": {"type": "string", "description": "Expression to render as LaTeX"},
            },
            "required": ["expression"],
        },
    ),
    # --- Tensor operations (tensors.py) ---
    types.Tool(
        name="tensor_contract",
        description="Contract tensor indices",
        inputSchema={
            "type": "object",
            "properties": {
                "expression": {"type": "string", "description": "Tensor expression"},
                "index_pairs": {"type": "array", "items": {"type": "array", "items": {"type": "integer"}},
                                "description": "Pairs of indices to contract, e.g. [[0,1]]"},
            },
            "required": ["expression", "index_pairs"],
        },
    ),
    types.Tool(
        name="tensor_symmetrize",
        description="Symmetrize or antisymmetrize a tensor expression",
        inputSchema={
            "type": "object",
            "properties": {
                "expression": {"type": "string", "description": "Tensor expression"},
                "indices": {"type": "array", "items": {"type": "string"},
                            "description": "Index names to symmetrize over"},
                "antisymmetric": {"type": "boolean", "description": "Antisymmetrize instead (default: false)",
                                  "default": False},
            },
            "required": ["expression", "indices"],
        },
    ),
    types.Tool(
        name="gamma_algebra",
        description="Evaluate Clifford/gamma matrix algebra (traces, products)",
        inputSchema={
            "type": "object",
            "properties": {
                "expression": {"type": "string", "description": "Gamma matrix expression, e.g. 'tr(gamma^mu gamma^nu)'"},
                "dimension": {"type": "integer", "description": "Spacetime dimension (default: 4)", "default": 4},
            },
            "required": ["expression"],
        },
    ),
    types.Tool(
        name="fierz_identity",
        description="Apply Fierz identities to spinor expressions (requires Cadabra2)",
        inputSchema={
            "type": "object",
            "properties": {
                "expression": {"type": "string", "description": "Spinor expression"},
            },
            "required": ["expression"],
        },
    ),
    # --- Physics operations (physics.py) ---
    types.Tool(
        name="christoffel",
        description="Compute Christoffel symbols from a metric tensor",
        inputSchema={
            "type": "object",
            "properties": {
                "metric": {"type": "array", "items": {"type": "array", "items": {"type": "string"}},
                           "description": "Metric tensor as 2D array of expressions"},
                "coordinates": {"type": "array", "items": {"type": "string"},
                                "description": "Coordinate names, e.g. ['t','r','theta','phi']"},
            },
            "required": ["metric", "coordinates"],
        },
    ),
    types.Tool(
        name="riemann_tensor",
        description="Compute the Riemann curvature tensor from a metric",
        inputSchema={
            "type": "object",
            "properties": {
                "metric": {"type": "array", "items": {"type": "array", "items": {"type": "string"}},
                           "description": "Metric tensor as 2D array of expressions"},
                "coordinates": {"type": "array", "items": {"type": "string"},
                                "description": "Coordinate names"},
            },
            "required": ["metric", "coordinates"],
        },
    ),
    types.Tool(
        name="ricci_tensor",
        description="Compute the Ricci tensor from a metric",
        inputSchema={
            "type": "object",
            "properties": {
                "metric": {"type": "array", "items": {"type": "array", "items": {"type": "string"}},
                           "description": "Metric tensor as 2D array"},
                "coordinates": {"type": "array", "items": {"type": "string"},
                                "description": "Coordinate names"},
            },
            "required": ["metric", "coordinates"],
        },
    ),
    types.Tool(
        name="ricci_scalar",
        description="Compute the Ricci scalar curvature from a metric",
        inputSchema={
            "type": "object",
            "properties": {
                "metric": {"type": "array", "items": {"type": "array", "items": {"type": "string"}},
                           "description": "Metric tensor as 2D array"},
                "coordinates": {"type": "array", "items": {"type": "string"},
                                "description": "Coordinate names"},
            },
            "required": ["metric", "coordinates"],
        },
    ),
    types.Tool(
        name="einstein_tensor",
        description="Compute the Einstein tensor G_{mu nu} from a metric",
        inputSchema={
            "type": "object",
            "properties": {
                "metric": {"type": "array", "items": {"type": "array", "items": {"type": "string"}},
                           "description": "Metric tensor as 2D array"},
                "coordinates": {"type": "array", "items": {"type": "string"},
                                "description": "Coordinate names"},
            },
            "required": ["metric", "coordinates"],
        },
    ),
    types.Tool(
        name="covariant_derivative",
        description="Compute the covariant derivative of a scalar expression",
        inputSchema={
            "type": "object",
            "properties": {
                "expression": {"type": "string", "description": "Scalar expression"},
                "metric": {"type": "array", "items": {"type": "array", "items": {"type": "string"}},
                           "description": "Metric tensor as 2D array"},
                "coordinates": {"type": "array", "items": {"type": "string"},
                                "description": "Coordinate names"},
                "direction": {"type": "string", "description": "Coordinate direction for derivative"},
            },
            "required": ["expression", "metric", "coordinates", "direction"],
        },
    ),
    types.Tool(
        name="evaluate_component",
        description="Evaluate a tensor component expression with numerical substitutions",
        inputSchema={
            "type": "object",
            "properties": {
                "expression": {"type": "string", "description": "Expression to evaluate"},
                "substitutions": {"type": "object", "description": "Variable -> value mapping"},
            },
            "required": ["expression", "substitutions"],
        },
    ),
]


@server.list_tools()
async def list_tools() -> list[types.Tool]:
    return TOOLS


@server.call_tool()
async def call_tool(name: str, arguments: dict[str, Any]) -> list[types.TextContent]:
    """Dispatch tool calls to the appropriate engine function."""
    try:
        result = _dispatch(name, arguments)
        return [types.TextContent(type="text", text=json.dumps(result, indent=2))]
    except ImportError as e:
        return [types.TextContent(type="text", text=json.dumps({
            "error": str(e),
            "type": "missing_dependency",
        }))]
    except Exception as e:
        return [types.TextContent(type="text", text=json.dumps({
            "error": str(e),
            "traceback": traceback.format_exc(),
            "type": "error",
        }))]


def _dispatch(name: str, args: dict) -> dict:
    """Route tool name to the correct function."""
    # Core algebra
    if name == "simplify":
        return engine.simplify(args["expression"])
    elif name == "expand":
        return engine.expand(args["expression"])
    elif name == "factor":
        return engine.factor(args["expression"])
    elif name == "differentiate":
        return engine.differentiate(args["expression"], args["variable"], args.get("order", 1))
    elif name == "integrate":
        return engine.integrate(args["expression"], args["variable"],
                                args.get("lower"), args.get("upper"))
    elif name == "series":
        return engine.series(args["expression"], args["variable"],
                             args.get("point", "0"), args.get("order", 6))
    elif name == "solve":
        return engine.solve(args["expression"], args["variable"])
    elif name == "substitute":
        return engine.substitute(args["expression"], args["substitutions"])
    elif name == "limit":
        return engine.limit(args["expression"], args["variable"], args["point"])
    elif name == "latex_render":
        return engine.latex_render(args["expression"])
    # Tensor operations
    elif name == "tensor_contract":
        return tensors.tensor_contract(args["expression"], args["index_pairs"])
    elif name == "tensor_symmetrize":
        return tensors.tensor_symmetrize(args["expression"], args["indices"],
                                         args.get("antisymmetric", False))
    elif name == "gamma_algebra":
        return tensors.gamma_algebra(args["expression"], args.get("dimension", 4))
    elif name == "fierz_identity":
        return tensors.fierz_identity(args["expression"])
    # Physics operations
    elif name == "christoffel":
        return physics.christoffel(args["metric"], args["coordinates"])
    elif name == "riemann_tensor":
        return physics.riemann_tensor(args["metric"], args["coordinates"])
    elif name == "ricci_tensor":
        return physics.ricci_tensor(args["metric"], args["coordinates"])
    elif name == "ricci_scalar":
        return physics.ricci_scalar(args["metric"], args["coordinates"])
    elif name == "einstein_tensor":
        return physics.einstein_tensor(args["metric"], args["coordinates"])
    elif name == "covariant_derivative":
        return physics.covariant_derivative(args["expression"], args["metric"],
                                            args["coordinates"], args["direction"])
    elif name == "evaluate_component":
        return physics.evaluate_component(args["expression"], args["substitutions"])
    else:
        raise ValueError(f"Unknown tool: {name}")


async def run():
    """Run the symbolic math MCP server."""
    async with mcp.server.stdio.stdio_server() as (read_stream, write_stream):
        await server.run(
            read_stream,
            write_stream,
            InitializationOptions(
                server_name="symbolic-math",
                server_version="0.1.0",
                capabilities=server.get_capabilities(
                    notification_options=NotificationOptions(),
                    experimental_capabilities={},
                ),
            ),
        )


def main():
    """Entry point."""
    asyncio.run(run())


if __name__ == "__main__":
    main()
