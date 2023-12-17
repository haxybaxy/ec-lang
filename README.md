# Kids++

## Usage

First build this project. Here we use CMake:

```bash
$ mkdir build && cd build  # Create a CMake workspace
$ cmake .. && make
$ ./kids # Start kids
```

Invoking `kids` without parameter starts the REPL. To run kids source files, please specify the file path. Feel free to run examples in the `examples` directory:

```
$ ./kids ../examples/hello-world.kids
Hello, world!
```

## What's Next


## Resources

- Book: [Crafting Interpreters](https://craftinginterpreters.com/)
- Blog post: [Crafting "Crafting Interpreters"](http://journal.stuffwithstuff.com/2020/04/05/crafting-crafting-interpreters/)
- Wiki: [Lox implementations](https://github.com/munificent/craftinginterpreters/wiki/Lox-implementations)
