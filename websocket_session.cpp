//
// Copyright (c) 2018 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/vinniefalco/CppCon2018
//

#include "websocket_session.hpp"

websocket_session::
websocket_session(
    tcp::socket socket,
    std::shared_ptr<shared_state> const& state)
    : ws_(std::move(socket))
    , state_(state)
{
}

websocket_session::
~websocket_session()
{
    // Remove this session from the list of active sessions
    state_->leave(*this);
}

void
websocket_session::
fail(error_code ec, char const* what)
{
    // Don't report on canceled operations
    if(ec != asio::error::operation_aborted)
        std::cerr << what << ": " << ec.message() << "\n";
}

void
websocket_session::
on_accept(error_code ec)
{
    // Handle the error, if any
    if(ec)
        return fail(ec, "accept");

    // Add this session to the list of active sessions
    state_->join(*this);

    // Read a message
    ws_.async_read(
        buffer_,
        std::bind(
            &websocket_session::on_read,
            shared_from_this(),
            std::placeholders::_1,
            std::placeholders::_2));
}

void
websocket_session::
on_read(error_code ec, std::size_t)
{
    // This indicates that the websocket_session was closed
    if(ec == websocket::error::closed)
        return;

    // Handle the error, if any
    if(ec)
        fail(ec, "read");

    // Send the message to all connected clients, including this one
    asio::const_buffer cb = beast::buffers_front(buffer_.data());
    state_->send(std::string(reinterpret_cast<char const*>(cb.data()), cb.size()));

    // Clear the buffer
    buffer_.consume(buffer_.size());

    // Read another message
    ws_.async_read(
        buffer_,
        std::bind(
            &websocket_session::on_read,
            shared_from_this(),
            std::placeholders::_1,
            std::placeholders::_2));
}

void
websocket_session::
on_write(error_code ec, std::size_t)
{
    // Handle the error, if any
    if(ec)
        return fail(ec, "write");

    // Remove the string from the queue
    queue_.erase(queue_.begin());

    // Send the next message if any
    if(! queue_.empty())
        ws_.async_write(
            boost::asio::buffer(*queue_.front()),
            std::bind(
                &websocket_session::on_write,
                shared_from_this(),
                std::placeholders::_1,
                std::placeholders::_2));
}

void
websocket_session::
send(std::shared_ptr<std::string const> const& ss)
{
    // Add the string to the queue no matter what
    queue_.push_back(ss);

    // See if we are currently busy writing a message
    if(queue_.size() > 1)
        return;

    // We are not currently writing, so send this immediately
    ws_.async_write(
        boost::asio::buffer(*queue_.front()),
        std::bind(
            &websocket_session::on_write,
            shared_from_this(),
            std::placeholders::_1,
            std::placeholders::_2));
}
