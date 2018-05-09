//
// Copyright (c) 2018 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/vinniefalco/CppCon2018
//

//------------------------------------------------------------------------------
/*
    WebSocket chat server

    This implements a multi-user chat room using WebSocket.
*/
//------------------------------------------------------------------------------

#include "listener.hpp"
#include "shared_state.hpp"
#include <boost/asio/signal_set.hpp>
#include <iostream>

int
main(int argc, char* argv[])
{
    // Check command line arguments.
    if (argc != 4)
    {
        std::cerr <<
            "Usage: websocket-chat-server <address> <port> <doc_root>\n" <<
            "Example:\n" <<
            "    websocket-chat-server 0.0.0.0 8080 .\n";
        return EXIT_FAILURE;
    }
    auto const address = asio::ip::make_address(argv[1]);
    auto const port = static_cast<unsigned short>(std::atoi(argv[2]));
    auto const doc_root = argv[3];

    // The io_context is required for all I/O
    asio::io_context ioc;

    // Create and launch a listening port
    std::make_shared<listener>(
        ioc,
        tcp::endpoint{address, port},
        std::make_shared<shared_state>(doc_root))->run();

    // Capture SIGINT and SIGTERM to perform a clean shutdown
    asio::signal_set signals(ioc, SIGINT, SIGTERM);
    signals.async_wait(
        [&](boost::system::error_code const&, int)
        {
            // Stop the `io_context`. This will cause `run()`
            // to return immediately, eventually destroying the
            // `io_context` and all of the handlers in it.
            ioc.stop();
        });

    // Run the I/O service on the main thread
    ioc.run();

    // (If we get here, it means we got a SIGINT or SIGTERM)

    return EXIT_SUCCESS;
}
