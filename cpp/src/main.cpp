#include "mcp/server.h"

int main() {
    mcp::Server server("symbolic-math", "0.1.0");
    mcp::register_all_tools(server);
    server.run();
    return 0;
}
