# Testing a better way to have common data structures in c

Quite often, I have the need to have a flexible library that allows creation of common
datastructures in c, with explicit support for custom allocators and not too much under-the-hood
magic in order to provide a simple interface to calling code.

This is my attempt in designing a system with context allocators similar in spirit to Jai or Zig
but in c.

All of these will probably be merged into [extlib](https://github.com/bamless/extlib) version 2.0

## Supported features for now

1. Dynamic arrays
1. Hashmaps
1. Explicit and context allocators
1. Temp allocator
1. Optional no-libc support

## Desiderata

1. `StringBuffer` implementation for easy string creation
1. `StringSlice` for easy manipulation of 'views' over `char*`
1. Common macros for `assert`, `static_assert` and `unreachable` that are cross platform, and do not
   depend on c11 features
1. A logging system, possibly integrated with the context system already inplace for allocators
1. Obviously, testing.

## Examples

Pretty minimal for now, check out `./main.c`, `./threads.c` and `./wasm.c`
