#include "rtmp_server.hpp"
#include <stdint.h>
#include <stddef.h>

int main(int argn, char** argv) {
    const uint16_t rtmp_def_port = 1935;
    boost::asio::io_context io_context;
    boost::asio::io_service::work work(io_context);

    try {
        rtmp_server server(io_context, rtmp_def_port);
        io_context.run();
    }
    catch(const std::exception& e) {
        std::cerr << e.what() << '\n';
    }
    return 0;
}