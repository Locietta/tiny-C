# [WIP] tiny-C
![CI](https://img.shields.io/github/workflow/status/Locietta/tiny-C/CI/main)
![](https://img.shields.io/github/license/Locietta/xanmod-kernel-WSL2)

❗This is still a WIP project.

This is a tiny C compiler built with Antlr4 and LLVM, as a simple exercise on compiler design.

## Dependencies

DevContainer is configured for build dependencies. With VSCode  Remote-Container, you can use it easily. Or you can pull the docker image from `locietta/loia-dev-base:antlr4`.

We basically base our work on ANTLR4 and LLVM13, while also use graphviz and fmt.

gcc >= 11 or clang >= 13 is needed, for we use a few C++20 features. Lower version of compilers might also work, but we didn't test them.

## Build

We use CMake to build.

```
cmake --build build --config Debug
cmake --build build --config Release
```

## Usage

```
tinycc <source file> [-O=<opt-level>]
```

For example, `tinycc a.c -O=1` will produce optimized code under `output` folder, including `a.ll`(LLVM IR code), `a.o`(x86 machine code) and some AST graphs.

Later you can also use `./view-cfg.sh output/a.ll` to view Control Flow Graph (CFG) of generated IR code.

### Some Reference Links

* [Antlr4 CMake Documentation](https://github.com/antlr/antlr4/tree/master/runtime/Cpp/cmake)
* [Embedding LLVM in CMake Project](https://llvm.org/docs/CMake.html#embedding-llvm-in-your-project)
* [Kaleidoscope](https://llvm.org/docs/tutorial/MyFirstLanguageFrontend/index.html) & [Bolt based on LLVM](https://mukulrathi.com/create-your-own-programming-language/llvm-ir-cpp-api-tutorial/)
* [How to use the New Pass Manager (PM)](https://llvm.org/docs/NewPassManager.html)
* [LLVM IR Reference](https://llvm.org/docs/LangRef.html)