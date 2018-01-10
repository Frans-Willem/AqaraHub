#include "coro.h"

namespace coro {
Await::Await(coro_t::push_type& sink) : sink_(sink) {}
} // namespace coro
