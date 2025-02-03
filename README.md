# Poafloc

Command-line argument parser for C and C++ written in C++20

## Description

This project is heavily inspired by GNU argp and the initial goal was to have
a drop in replacement. I've managed to implement most of the features I plan
on using, but there is still space for future improvements, maybe even in
other directions.

The main motivation behind this project is gaining an in depth understanding
of all of the syntax rules of command line arguments, to be used for all of
the UNIX utilities.

This project included a few challenges in both the design and implementation.
There was a lot of experimentation with interface that works for both C and
C++, as well as having a single library that provides bindings and works with
both languages simultaneously. There were a lot of caveats and edge cases to
be understood first, and later implemented.


## Dependencies

* CMake 3.25.2 or latter
* Compiler with C++20 support (tested: clang 16.0.5, gcc 13.2.0)


## Building and installing

See the [BUILDING](BUILDING.md) document.


## Usage

> Please reference example folder for relevant usage example.


## Help

Refer to [GNU argp documentation](https://www.gnu.org/software/libc/manual/html_node/Argp.html)


## Version History

- 1.2
    * Propagate parser generated error

- 1.1
    * Modernize CMake project
    * Modernize codebase
    * Add test suite

- 1.0
    * Initial Release


## Contributing

See the [CONTRIBUTING](CONTRIBUTING.md) document.


## License

This project is licensed under the MIT License -
see the [LICENSE](LICENSE.md) document for details


## Acknowledgments

Inspiration, code snippets, etc.
* [GNU argp documentation](https://www.gnu.org/software/libc/manual/html_node/Argp.html)
* [Step-by-Step into Argp](http://nongnu.askapache.com/argpbook/step-by-step-into-argp.pdf)
* [ericonr/argp-standalone](https://github.com/ericonr/argp-standalone)
