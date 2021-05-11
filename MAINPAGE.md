# ArkScript

This is the **implementation** documentation of the ArkScript programming language, if you want to contribute to it in any way.

## Creating modules

You will still need to dive a bit into the documentation of the project, to know how:
* the VM API works, and what it provides
* the possibilities of the Value type (comparisons, creations)
* how to use the `UserType`

Also, read the [RFC 004](https://github.com/ArkScript-lang/rfc/blob/master/004-module-error-handling.md) about module error handling to use the same conventions as the other modules, and the [RFC 003](https://github.com/ArkScript-lang/rfc/blob/master/003-naming-convention.md) about naming conventions in ArkScript (specifically see the *Modules (C++)* section).

## Enhancing the project

### A brief tour of the architecture of the project

* everything under `Builtins` is related to builtin functions, used with the bytecode instructions `BUILTIN id`
    * adding one will need to reference it in `include/Ark/Builtins/Builtins.hpp` and in `src/Builtins/Builtins.cpp`, + implementing it accordingly under `src/Builtins/[file].cpp`
* the Lexer, Parser, AST generation (using Nodes), AST optimizer and Compiler are located under `include/Ark/Compiler/`.
    * the Compiler calls the Parser on a given piece of code
        * the Parser calls the Lexer on the given code
            * the Lexer returns a list of tokens
        * the Parser returns an AST from the token list
    * the Compiler calls the AST optimizer on the Parser's AST
    * the Compiler generated bytecode, which can be used as is by the virtual machine, or be saved to a file
* the REPL (read eval print loop) is located under `include/Ark/REPL/`. Basically it's an abstraction level over `replxx` (external library for completion and coloration in the shell) and our virtual machine to run user inputs
* the virtual machine lies under `include/Ark/VM/` and all the folders under it
    * it handles the Closures which capture whole Scopes through `shared_ptr`. Closures are functions with a saved scope, so they can retain information over multiple calls
    * the Plugin loader is a generic DLL / SO / DYNLIB loader, used by the virtual machine to load ArkScript modules (`.arkm` files)
    * the Scope is how we store our mapping `variable id => value`, heavily optimized for our needs
    * the State is:
        * reading bytecode
        * decoding it
        * filling multiple tables with it (symbol table, value table, code pages), which are then used by the virtual machine. It allows us to load a single ArkScript bytecode file and use it in multiple virtual machines.
        * the State retains tables which are **never altered** by the virtual machines
        * it can also compile ArkScript code and files on the go, and run them right away
    * the UserType is how we store C++ types unknown to our virtual machine, to use them in ArkScript code
    * the Value is a very big proxy class to a `variant` to store our types (our custom String, double, Closure, UserType and more), thus **it must stay small** because it's the primitive type of the virtual machine and the language
        * it handles constness and type through a tag, alongside the value
        * it provides proxy functions to the underlying `variant`
    * the virtual machine handles:
        * the stack, a single `array<Value, 8192>` (the stack size is a define, thus it can be changed at compile time only)
        * a pointer to the state, to read the tables and code segments
        * a pointer to a `void*` user_data, retrievable by modules and C++ user functions
        * the scopes, and their destruction
        * the instructions, executed in `safeRun` which is enclosed in a try/catch to display tracebacks when errors occur
        * external function calls through its private `call` method (to call modules' functions and builtins)
        * value resolving, useful for modules when a function receives an ArkScript function, through the public method `resolve`

If you feel that this section is lacking information, please open an issue [on the main repository](https://github.com/ArkScript-lang/Ark).

## Embedding ArkScript in C++ code

### Using ArkScript

An example is often worth a thousands words:

    #include <Ark/Ark.hpp>

    int main()
    {
        // A state can be shared by multiple virtual machines (note that they will NEVER modify it)
        // leave constructor empty to select the default standard library (loaded from an environment variable $ARKSCRIPT_PATH/lib)
        // persistance is needed to use vm.call(function_name, args...)
        Ark::State state(/* options */ Ark::FeaturePersist);

        // Will automatically compile the file if needed (if not, will take it from the ark cache)
        state.doFile("myfile.ark");

        Ark::VM vm(&state);
        vm.run();

        /*
            If you just want to run a precompiled bytecode file:

            Ark::State state;
            state.feed("mybytecode.arkc");
            Ark::VM vm(&state);
            vm.run();
        */

        /*
            To run an ArkScript function from C++ code and retrieve the result:
            we will say the code is (let foo (fun (x y) (+ x y 2)))
        */
        auto value = vm.call("foo", 5, 6.0);
        std::cout << value << "\n";  // displays 13

        return 0;
    }

### Adding your own functions

    #include <Ark/Ark.hpp>

    Ark::Value my_function(std::vector<Ark::Value>& args, Ark::VM* vm)
    {
        // checking argument number
        if (args.size() != 4)
            throw std::runtime_error("my_function needs 4 arguments!");

        auto a = args[0],
            b = args[1],
            c = args[2],
            d = args[3];

        // checking arguments type
        if (a.valueType() != Ark::ValueType::Number ||
            b.valueType() != Ark::ValueType::Number ||
            c.valueType() != Ark::ValueType::Number ||
            d.valueType() != Ark::ValueType::Number)
            throw Ark::TypeError("Type mismatch for my_function: need only numbers");

        // type is automatically deducted from the argument
        return Ark::Value(a.number() * b.number() - c.number() / d.number());
    }

    int main()
    {
        Ark::State state(/* options */ Ark::FeaturePersist);
        state.doFile("myfile.ark");  // we can call state.doFile() before or after state.loadFunction()

        state.loadFunction("my_function", my_function);

        // we can also load C++ lambdas
        // we could have done this after creating the VM, it would still works
        // we just need to do that BEFORE we call vm.run()
        state.loadFunction("foo", [](std::vector<Ark::Value>& args, Ark::VM* vm) {
            return Ark::Value(static_cast<int>(args.size()));
        });

        Ark::VM vm(&state);
        vm.run();

        return 0;
    }

### Adding your own types in ArkScript

    enum class Breakfast { Eggs, Bacon, Pizza };

    Breakfast& getBreakfast()
    {
        static Breakfast bf = Breakfast::Pizza;
        return bf;
    }

    UserType::ControlFuncs* get_cfs()
    {
        static UserType::ControlFuncs cfs;

        cfs.ostream_func = [](std::ostream& os, const UserType& a) -> std::ostream& {
            os << "Breakfast::";
            switch (a.as<Breakfast>())
            {
                case Breakfast::Eggs:  os << "Eggs";    break;
                case Breakfast::Bacon: os << "Bacon";   break;
                case Breakfast::Pizza: os << "Pizza";   break;
                default:               os << "Unknown"; break;
            }
            return os;
        };

        return &cfs;
    }

    int main()
    {
        Ark::State state;

        state.loadFunction("getBreakfast", [](std::vector<Ark::Value>& n, Ark::VM* vm) -> Ark::Value {
            // we need to send the address of the object, which will be casted
            // to void* internally
            Ark::Value v = Ark::Value(Ark::UserType(&getBreakfast()));

            // register the unique control functions block for this usertype
            // this cfs block can be shared between multiple usertype to reduce memory usage
            v.usertype_ref().setControlFuncs(get_cfs());

            return v;
        });

        state.loadFunction("useBreakfast", [](std::vector<Ark::Value>& n, Ark::VM* vm) -> Ark::Value {
            if (n[0].valueType() == Ark::ValueType::User && n[0].usertype().is<Breakfast>())
            {
                std::cout << "UserType detected as an enum class Breakfast" << std::endl;
                Breakfast& bf = n[0].usertype().as<Breakfast>();
                std::cout << "Got " << n[0].usertype() << "\n";
                if (bf == Breakfast::Pizza)
                    std::cout << "Good choice! Have a nice breakfast ;)" << std::endl;
            }

            return Ark::Nil;
        });

        state.doString("(begin (let a (getBreakfast)) (print a) (useBreakfast a))");
        Ark::VM vm(&state);
        vm.run();

        /*
            Will print

            Breakfast::Pizza
            UserType detected as an enum class Breakfast
            Got Breakfast::Pizza
            Good choice! Have a nice breakfast ;)
        */

        return 0;
    }