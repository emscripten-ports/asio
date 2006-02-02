//
// read_test.cpp
// ~~~~~~~~~~~~~
//
// Copyright (c) 2003-2006 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

// Test that header file is self-contained.
#include "asio/read.hpp"

#include <boost/bind.hpp>
#include <boost/noncopyable.hpp>
#include <cstring>
#include "asio.hpp"
#include "unit_test.hpp"

using namespace std; // For memcmp, memcpy and memset.

class test_stream
  : private boost::noncopyable
{
public:
  typedef asio::io_service io_service_type;
  typedef asio::error error_type;

  test_stream(asio::io_service& io_service)
    : io_service_(io_service),
      length_(0),
      position_(0),
      next_read_length_(0)
  {
  }

  io_service_type& io_service()
  {
    return io_service_;
  }

  void reset(const void* data, size_t length)
  {
    BOOST_CHECK(length <= max_length);

    memcpy(data_, data, length);
    length_ = length;
    position_ = 0;
    next_read_length_ = length;
  }

  void next_read_length(size_t length)
  {
    next_read_length_ = length;
  }

  template <typename Const_Buffers>
  bool check(const Const_Buffers& buffers, size_t length)
  {
    if (length != position_)
      return false;

    typename Const_Buffers::const_iterator iter = buffers.begin();
    typename Const_Buffers::const_iterator end = buffers.end();
    size_t checked_length = 0;
    for (; iter != end && checked_length < length; ++iter)
    {
      size_t buffer_length = asio::buffer_size(*iter);
      if (buffer_length > length - checked_length)
        buffer_length = length - checked_length;
      if (memcmp(data_ + checked_length,
            asio::buffer_cast<const void*>(*iter), buffer_length) != 0)
        return false;
      checked_length += buffer_length;
    }

    return true;
  }

  template <typename Mutable_Buffers>
  size_t read_some(const Mutable_Buffers& buffers)
  {
    size_t total_length = 0;

    typename Mutable_Buffers::const_iterator iter = buffers.begin();
    typename Mutable_Buffers::const_iterator end = buffers.end();
    for (; iter != end && total_length < next_read_length_; ++iter)
    {
      size_t length = asio::buffer_size(*iter);
      if (length > length_ - position_)
        length = length_ - position_;

      if (length > next_read_length_ - total_length)
        length = next_read_length_ - total_length;

      memcpy(asio::buffer_cast<void*>(*iter), data_ + position_, length);
      position_ += length;
      total_length += length;
    }

    return total_length;
  }

  template <typename Mutable_Buffers, typename Error_Handler>
  size_t read_some(const Mutable_Buffers& buffers, Error_Handler)
  {
    return read_some(buffers);
  }

  template <typename Mutable_Buffers, typename Handler>
  void async_read_some(const Mutable_Buffers& buffers, Handler handler)
  {
    size_t bytes_transferred = read_some(buffers);
    asio::error error;
    io_service_.post(
        asio::detail::bind_handler(handler, error, bytes_transferred));
  }

private:
  io_service_type& io_service_;
  enum { max_length = 8192 };
  char data_[max_length];
  size_t length_;
  size_t position_;
  size_t next_read_length_;
};

static const char read_data[]
  = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

void test_2_arg_read()
{
  asio::io_service ios;
  test_stream s(ios);
  char read_buf[sizeof(read_data)];
  asio::mutable_buffer_container_1 buffers
    = asio::buffer(read_buf, sizeof(read_buf));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  size_t bytes_transferred = asio::read(s, buffers);
  BOOST_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_CHECK(s.check(buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = asio::read(s, buffers);
  BOOST_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_CHECK(s.check(buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = asio::read(s, buffers);
  BOOST_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_CHECK(s.check(buffers, sizeof(read_data)));
}

void test_3_arg_read()
{
  asio::io_service ios;
  test_stream s(ios);
  char read_buf[sizeof(read_data)];
  asio::mutable_buffer_container_1 buffers
    = asio::buffer(read_buf, sizeof(read_buf));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  size_t bytes_transferred = asio::read(s, buffers,
      asio::transfer_all());
  BOOST_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_CHECK(s.check(buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = asio::read(s, buffers,
      asio::transfer_all());
  BOOST_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_CHECK(s.check(buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = asio::read(s, buffers,
      asio::transfer_all());
  BOOST_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_CHECK(s.check(buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = asio::read(s, buffers,
      asio::transfer_at_least(1));
  BOOST_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_CHECK(s.check(buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = asio::read(s, buffers,
      asio::transfer_at_least(1));
  BOOST_CHECK(bytes_transferred == 1);
  BOOST_CHECK(s.check(buffers, 1));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = asio::read(s, buffers,
      asio::transfer_at_least(1));
  BOOST_CHECK(bytes_transferred == 10);
  BOOST_CHECK(s.check(buffers, 10));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = asio::read(s, buffers,
      asio::transfer_at_least(10));
  BOOST_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_CHECK(s.check(buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = asio::read(s, buffers,
      asio::transfer_at_least(10));
  BOOST_CHECK(bytes_transferred == 10);
  BOOST_CHECK(s.check(buffers, 10));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = asio::read(s, buffers,
      asio::transfer_at_least(10));
  BOOST_CHECK(bytes_transferred == 10);
  BOOST_CHECK(s.check(buffers, 10));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = asio::read(s, buffers,
      asio::transfer_at_least(42));
  BOOST_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_CHECK(s.check(buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = asio::read(s, buffers,
      asio::transfer_at_least(42));
  BOOST_CHECK(bytes_transferred == 42);
  BOOST_CHECK(s.check(buffers, 42));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  bytes_transferred = asio::read(s, buffers,
      asio::transfer_at_least(42));
  BOOST_CHECK(bytes_transferred == 50);
  BOOST_CHECK(s.check(buffers, 50));
}

void test_4_arg_read()
{
  asio::io_service ios;
  test_stream s(ios);
  char read_buf[sizeof(read_data)];
  asio::mutable_buffer_container_1 buffers
    = asio::buffer(read_buf, sizeof(read_buf));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  asio::error error;
  size_t bytes_transferred = asio::read(s, buffers,
      asio::transfer_all(), asio::assign_error(error));
  BOOST_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_CHECK(s.check(buffers, sizeof(read_data)));
  BOOST_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  error = asio::error();
  bytes_transferred = asio::read(s, buffers,
      asio::transfer_all(), asio::assign_error(error));
  BOOST_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_CHECK(s.check(buffers, sizeof(read_data)));
  BOOST_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  error = asio::error();
  bytes_transferred = asio::read(s, buffers,
      asio::transfer_all(), asio::assign_error(error));
  BOOST_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_CHECK(s.check(buffers, sizeof(read_data)));
  BOOST_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  error = asio::error();
  bytes_transferred = asio::read(s, buffers,
      asio::transfer_at_least(1), asio::assign_error(error));
  BOOST_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_CHECK(s.check(buffers, sizeof(read_data)));
  BOOST_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  error = asio::error();
  bytes_transferred = asio::read(s, buffers,
      asio::transfer_at_least(1), asio::assign_error(error));
  BOOST_CHECK(bytes_transferred == 1);
  BOOST_CHECK(s.check(buffers, 1));
  BOOST_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  error = asio::error();
  bytes_transferred = asio::read(s, buffers,
      asio::transfer_at_least(1), asio::assign_error(error));
  BOOST_CHECK(bytes_transferred == 10);
  BOOST_CHECK(s.check(buffers, 10));
  BOOST_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  error = asio::error();
  bytes_transferred = asio::read(s, buffers,
      asio::transfer_at_least(10), asio::assign_error(error));
  BOOST_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_CHECK(s.check(buffers, sizeof(read_data)));
  BOOST_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  error = asio::error();
  bytes_transferred = asio::read(s, buffers,
      asio::transfer_at_least(10), asio::assign_error(error));
  BOOST_CHECK(bytes_transferred == 10);
  BOOST_CHECK(s.check(buffers, 10));
  BOOST_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  error = asio::error();
  bytes_transferred = asio::read(s, buffers,
      asio::transfer_at_least(10), asio::assign_error(error));
  BOOST_CHECK(bytes_transferred == 10);
  BOOST_CHECK(s.check(buffers, 10));
  BOOST_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  error = asio::error();
  bytes_transferred = asio::read(s, buffers,
      asio::transfer_at_least(42), asio::assign_error(error));
  BOOST_CHECK(bytes_transferred == sizeof(read_data));
  BOOST_CHECK(s.check(buffers, sizeof(read_data)));
  BOOST_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  error = asio::error();
  bytes_transferred = asio::read(s, buffers,
      asio::transfer_at_least(42), asio::assign_error(error));
  BOOST_CHECK(bytes_transferred == 42);
  BOOST_CHECK(s.check(buffers, 42));
  BOOST_CHECK(!error);

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  error = asio::error();
  bytes_transferred = asio::read(s, buffers,
      asio::transfer_at_least(42), asio::assign_error(error));
  BOOST_CHECK(bytes_transferred == 50);
  BOOST_CHECK(s.check(buffers, 50));
  BOOST_CHECK(!error);
}

void async_read_handler(const asio::error& e, size_t bytes_transferred,
    size_t expected_bytes_transferred, bool* called)
{
  *called = true;
  BOOST_CHECK(!e);
  BOOST_CHECK(bytes_transferred == expected_bytes_transferred);
}

void test_3_arg_async_read()
{
  asio::io_service ios;
  test_stream s(ios);
  char read_buf[sizeof(read_data)];
  asio::mutable_buffer_container_1 buffers
    = asio::buffer(read_buf, sizeof(read_buf));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  bool called = false;
  asio::async_read(s, buffers,
      boost::bind(async_read_handler,
        asio::placeholders::error,
        asio::placeholders::bytes_transferred,
        sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_CHECK(called);
  BOOST_CHECK(s.check(buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  asio::async_read(s, buffers,
      boost::bind(async_read_handler,
        asio::placeholders::error,
        asio::placeholders::bytes_transferred,
        sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_CHECK(called);
  BOOST_CHECK(s.check(buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  asio::async_read(s, buffers,
      boost::bind(async_read_handler,
        asio::placeholders::error,
        asio::placeholders::bytes_transferred,
        sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_CHECK(called);
  BOOST_CHECK(s.check(buffers, sizeof(read_data)));
}

void test_4_arg_async_read()
{
  asio::io_service ios;
  test_stream s(ios);
  char read_buf[sizeof(read_data)];
  asio::mutable_buffer_container_1 buffers
    = asio::buffer(read_buf, sizeof(read_buf));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  bool called = false;
  asio::async_read(s, buffers, asio::transfer_all(),
      boost::bind(async_read_handler,
        asio::placeholders::error,
        asio::placeholders::bytes_transferred,
        sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_CHECK(called);
  BOOST_CHECK(s.check(buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  asio::async_read(s, buffers, asio::transfer_all(),
      boost::bind(async_read_handler,
        asio::placeholders::error,
        asio::placeholders::bytes_transferred,
        sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_CHECK(called);
  BOOST_CHECK(s.check(buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  asio::async_read(s, buffers, asio::transfer_all(),
      boost::bind(async_read_handler,
        asio::placeholders::error,
        asio::placeholders::bytes_transferred,
        sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_CHECK(called);
  BOOST_CHECK(s.check(buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  asio::async_read(s, buffers, asio::transfer_at_least(1),
      boost::bind(async_read_handler,
        asio::placeholders::error,
        asio::placeholders::bytes_transferred,
        sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_CHECK(called);
  BOOST_CHECK(s.check(buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  asio::async_read(s, buffers, asio::transfer_at_least(1),
      boost::bind(async_read_handler,
        asio::placeholders::error,
        asio::placeholders::bytes_transferred,
        1, &called));
  ios.reset();
  ios.run();
  BOOST_CHECK(called);
  BOOST_CHECK(s.check(buffers, 1));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  asio::async_read(s, buffers, asio::transfer_at_least(1),
      boost::bind(async_read_handler,
        asio::placeholders::error,
        asio::placeholders::bytes_transferred,
        10, &called));
  ios.reset();
  ios.run();
  BOOST_CHECK(called);
  BOOST_CHECK(s.check(buffers, 10));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  asio::async_read(s, buffers, asio::transfer_at_least(10),
      boost::bind(async_read_handler,
        asio::placeholders::error,
        asio::placeholders::bytes_transferred,
        sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_CHECK(called);
  BOOST_CHECK(s.check(buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  asio::async_read(s, buffers, asio::transfer_at_least(10),
      boost::bind(async_read_handler,
        asio::placeholders::error,
        asio::placeholders::bytes_transferred,
        10, &called));
  ios.reset();
  ios.run();
  BOOST_CHECK(called);
  BOOST_CHECK(s.check(buffers, 10));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  asio::async_read(s, buffers, asio::transfer_at_least(10),
      boost::bind(async_read_handler,
        asio::placeholders::error,
        asio::placeholders::bytes_transferred,
        10, &called));
  ios.reset();
  ios.run();
  BOOST_CHECK(called);
  BOOST_CHECK(s.check(buffers, 10));

  s.reset(read_data, sizeof(read_data));
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  asio::async_read(s, buffers, asio::transfer_at_least(42),
      boost::bind(async_read_handler,
        asio::placeholders::error,
        asio::placeholders::bytes_transferred,
        sizeof(read_data), &called));
  ios.reset();
  ios.run();
  BOOST_CHECK(called);
  BOOST_CHECK(s.check(buffers, sizeof(read_data)));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(1);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  asio::async_read(s, buffers, asio::transfer_at_least(42),
      boost::bind(async_read_handler,
        asio::placeholders::error,
        asio::placeholders::bytes_transferred,
        42, &called));
  ios.reset();
  ios.run();
  BOOST_CHECK(called);
  BOOST_CHECK(s.check(buffers, 42));

  s.reset(read_data, sizeof(read_data));
  s.next_read_length(10);
  memset(read_buf, 0, sizeof(read_buf));
  called = false;
  asio::async_read(s, buffers, asio::transfer_at_least(42),
      boost::bind(async_read_handler,
        asio::placeholders::error,
        asio::placeholders::bytes_transferred,
        50, &called));
  ios.reset();
  ios.run();
  BOOST_CHECK(called);
  BOOST_CHECK(s.check(buffers, 50));
}

test_suite* init_unit_test_suite(int argc, char* argv[])
{
  test_suite* test = BOOST_TEST_SUITE("read");
  test->add(BOOST_TEST_CASE(&test_2_arg_read));
  test->add(BOOST_TEST_CASE(&test_3_arg_read));
  test->add(BOOST_TEST_CASE(&test_4_arg_read));
  test->add(BOOST_TEST_CASE(&test_3_arg_async_read));
  test->add(BOOST_TEST_CASE(&test_4_arg_async_read));
  return test;
}
