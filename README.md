# Symbolic Math MCP Server

An [MCP](https://modelcontextprotocol.io/) server that gives LLMs access to symbolic mathematics and theoretical physics computations. Built on SymPy (Python) and SymEngine (C++), it exposes 21 tools for algebra, tensor calculus, and general relativity.

## Features

**Core Algebra** — simplify, expand, factor, differentiate, integrate, series expansion, solve equations, substitute, limits, LaTeX rendering

**Tensor Algebra** — index contraction, symmetrization/antisymmetrization, gamma matrix (Clifford) algebra with trace evaluation, Fierz identities

**General Relativity** — Christoffel symbols, Riemann tensor, Ricci tensor, Ricci scalar, Einstein tensor, covariant derivatives, component evaluation

Both LaTeX and SymPy syntax are accepted as input. All results include LaTeX-rendered output.

## Implementations

There are two implementations with identical tool interfaces:

| | Python | C++ |
|---|---|---|
| **Engine** | SymPy (+ optional SymEngine acceleration) | SymEngine |
| **Tensor backend** | Optional Cadabra2 | Optional Cadabra2 |
| **MCP transport** | `mcp` Python SDK, stdio | Custom JSON-RPC, stdio |
| **Build** | `pip install` | CMake 3.15+ |

Both communicate over stdio using the MCP protocol and expose the same 21 tools.

## Quick Start (Python)

```bash
cd symbolic-math
python3 -m venv .venv && source .venv/bin/activate
pip install -e .
```

Run the server:

```bash
symbolic-math-mcp
```

### Running Tests

```bash
pip install -e ".[dev]"
PYTHONPATH=src pytest tests/ -v
```

## Quick Start (C++)

```bash
cd symbolic-math/cpp
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Run the server:

```bash
./build/symbolic-math-mcp
```

### Running Tests

```bash
cd build && ctest --output-on-failure
```

### C++ Dependencies

Fetched automatically via CMake FetchContent:
- [SymEngine](https://github.com/symengine/symengine) v0.13+ (or system install)
- [nlohmann/json](https://github.com/nlohmann/json) v3.11+
- [Google Test](https://github.com/google/googletest) v1.14+ (tests only)

Optional system packages:
- GMP (preferred) or Boost multiprecision — for arbitrary-precision integers
- [Cadabra2](https://cadabra.science/) — enables full tensor algebra

## MCP Configuration

Add to your Claude Desktop or Claude Code MCP config:

```json
{
  "mcpServers": {
    "symbolic-math": {
      "command": "/path/to/symbolic-math/.venv/bin/symbolic-math-mcp"
    }
  }
}
```

For the C++ build, point `command` at the compiled binary instead.

## Tools

### Algebra

| Tool | Description | Key Parameters |
|---|---|---|
| `simplify` | Simplify an expression | `expression` |
| `expand` | Expand products and powers | `expression` |
| `factor` | Factor polynomials | `expression` |
| `differentiate` | Symbolic differentiation | `expression`, `variable`, `order` |
| `integrate` | Indefinite or definite integration | `expression`, `variable`, `lower`, `upper` |
| `series` | Taylor/Laurent series expansion | `expression`, `variable`, `point`, `order` |
| `solve` | Solve equations (expression = 0) | `expression`, `variable` |
| `substitute` | Substitute values into an expression | `expression`, `substitutions` |
| `limit` | Compute limits | `expression`, `variable`, `point` |
| `latex_render` | Parse and render as LaTeX | `expression` |

### Tensor Algebra

| Tool | Description | Key Parameters |
|---|---|---|
| `tensor_contract` | Contract tensor indices | `expression`, `index_pairs` |
| `tensor_symmetrize` | Symmetrize/antisymmetrize tensors | `expression`, `indices`, `antisymmetric` |
| `gamma_algebra` | Evaluate gamma matrix traces and products | `expression`, `dimension` |
| `fierz_identity` | Apply Fierz identities (requires Cadabra2) | `expression` |

### General Relativity

| Tool | Description | Key Parameters |
|---|---|---|
| `christoffel` | Christoffel symbols of the second kind | `metric`, `coordinates` |
| `riemann_tensor` | Riemann curvature tensor | `metric`, `coordinates` |
| `ricci_tensor` | Ricci tensor | `metric`, `coordinates` |
| `ricci_scalar` | Ricci scalar curvature | `metric`, `coordinates` |
| `einstein_tensor` | Einstein tensor | `metric`, `coordinates` |
| `covariant_derivative` | Covariant derivative of a scalar | `expression`, `metric`, `coordinates`, `direction` |
| `evaluate_component` | Evaluate a tensor component numerically | `expression`, `substitutions` |

## Examples

### Algebra

```
simplify("sin(x)**2 + cos(x)**2")
→ {"result": "1", "latex": "1", "type": "expression"}

integrate("exp(-x**2)", "x", "-oo", "oo")
→ {"result": "sqrt(pi)", "latex": "\\sqrt{\\pi}", "type": "expression"}

solve("a*x**2 + b*x + c", "x")
→ quadratic formula solutions
```

LaTeX input is also accepted:

```
differentiate("\\frac{x^3}{3}", "x")
→ {"result": "x**2", "latex": "x^{2}", "type": "expression"}
```

### Gamma Matrix Traces

```
gamma_algebra("tr(gamma^mu gamma^nu)", dimension=4)
→ {"result": "4*g^{munu}", "latex": "4 \\, g^{munu}", "type": "expression"}

gamma_algebra("tr(gamma^a gamma^b gamma^c gamma^d)", dimension=4)
→ 4*(g^{ab}g^{cd} - g^{ac}g^{bd} + g^{ad}g^{bc})
```

### General Relativity

Compute the Ricci scalar of a 2-sphere:

```
ricci_scalar(
    metric=[["r**2", "0"], ["0", "r**2*sin(theta)**2"]],
    coordinates=["theta", "phi"]
)
→ {"result": "2/r**2", "latex": "\\frac{2}{r^{2}}", "type": "scalar"}
```

Verify flat Minkowski space has vanishing curvature:

```
riemann_tensor(
    metric=[["-1","0","0","0"],["0","1","0","0"],["0","0","1","0"],["0","0","0","1"]],
    coordinates=["t","x","y","z"]
)
→ "Riemann tensor is identically zero (flat space)"
```

## Optional Dependencies

| Dependency | Effect |
|---|---|
| [SymEngine](https://github.com/symengine/symengine) (Python) | Accelerates `expand`, `simplify`, `differentiate` |
| [Cadabra2](https://cadabra.science/) | Full tensor contraction, symmetrization, Fierz identities |

Both are optional. The server reports available backends and degrades gracefully when they are not installed.

## Project Structure

```
symbolic-math/
├── pyproject.toml
├── src/symbolic_math/
│   ├── server.py          # MCP server, tool definitions, dispatch
│   ├── engine.py          # Core algebra (SymPy)
│   ├── tensors.py         # Tensor and gamma matrix algebra
│   └── physics.py         # General relativity computations
├── tests/
│   ├── test_engine.py     # 13 algebra tests
│   ├── test_tensors.py    # 4 tensor/gamma tests
│   ├── test_physics.py    # 4 GR tests
│   └── test_server.py     # 6 MCP protocol tests
└── cpp/                   # C++ implementation
    ├── CMakeLists.txt
    ├── src/
    │   ├── main.cpp
    │   ├── mcp/           # JSON-RPC protocol and server
    │   ├── engine/        # SymEngine algebra, integration, limits, parsing
    │   ├── physics/       # Metric parsing, GR tensor computations
    │   └── tensors/       # Tensor ops, Cadabra2 bridge
    └── tests/             # Google Test suite
```

## License

MIT
