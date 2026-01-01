
# GWD Programming Language

A new programming language, **GWD** /good/,  
and its compiler/interpreter with its VM,  
dedicated to **Gries, Wirth and Dijkstra**.

David Gries, Niklaus Wirth and Edsger Dijkstra  
are the pioneers of computer science,  
programming languages and compilers.

GWD /good/ supports both procedural and  
object oriented programming paradigms.

---

## To Build and Test

**DEBUG** and **INTERPRET** macros are in `types.h`  
enable and disable as you need.

```sh
$ gcc -g lex.c parse.c bigparse.c emit.c vm.c gwd.c
$ ./a.out ./examples/average.gwd
$ cat out.ir.txt
````

look at `build.sh` for more info,
and how to include memory manager.

---

# **The GWD Programming Language**

### *Language Summary*

---

## **0. Abstract, Acknowledgements & Inspirations**

* **Language name:** GWD
* **Pronunciation:** */good/*
* **Author:** Srinivas Nayak
* **Version:** 0.0
* **Date:** 1 Jan 2026

---

## **0.1 Abstract**

* GWD is a **compact, statically typed, object-oriented programming language** developed together with its **own compiler, intermediate representation (IR), and stack-based virtual machine**.
* The language follows a **grammar-first philosophy**, where the formal grammar directly drives parser structure, semantic checks, IR generation, and execution.
* GWD emphasizes **clarity and explicitness** in all aspects of program behavior:

  * explicit control flow
  * explicit memory management
  * explicit data movement
* The pointer model is deliberately **restricted and disciplined**:

  * pointers are global-only
  * no pointer-to-pointer types
  * no address-of or dereference operators
  * dereferencing is implicit and uniform
* Structured data and behavior are expressed through **record types with `must` and `can` method contracts**, enabling controlled **polymorphism without inheritance**.
* All computation is unified under a **single integer-based value model**:

  * every expression evaluates to `int`
  * every function returns `int`
* Program execution semantics are defined concretely via a **well-specified IR and VM**, ensuring that runtime behavior is predictable, inspectable, and implementation-aligned.

---

## **0.2 Acknowledgements / Inspirations**

* GWD draws inspiration from the **classical structured programming tradition**, particularly the work of:

  * Edsger W. Dijkstra
  * Niklaus Wirth
  * David Gries
* The influence is **methodological rather than syntactic**, reflected in:

  * grammar-driven language definition
  * explicit control constructs
  * single-pass compilation strategies
* The design consciously avoids many modern conveniences in favor of **semantic transparency**, including:

  * inheritance and deep type hierarchies
  * implicit polymorphism
  * automatic memory management
* GWD aligns with the idea of a **“compiler-complete” language**:

  * grammar
  * parser
  * symbol tables
  * IR
  * virtual machine
  * debugger
* The language is intended as a **practical exploration tool**—a system where language design, compilation, and execution are clearly visible.

---

## **1. Introduction**

* GWD is designed **from the ground up**, starting with its grammar and execution model rather than surface-level syntax.
* The language exists to make the internal structure of a compiler and runtime **explicit and observable**, including:

  * parsing decisions
  * scope and symbol resolution
  * stack-frame layout
  * heap allocation behavior
  * instruction sequencing
* Core structural characteristics:

  * **line-oriented syntax** where newlines are significant
  * a **single compilation unit**
  * **declaration-before-use** enforced syntactically and semantically
* Expressiveness is intentionally constrained to promote:

  * straightforward parsing
  * deterministic static checks
  * linear IR generation
  * predictable VM execution
* GWD is not designed for general-purpose or production use. Instead, it excels as a language for:

  * learning and teaching compiler construction
  * experimenting with grammar design
  * studying explicit memory operations (`bind`, `move`, `free`)
  * exploring contract-based polymorphism without inheritance
* The result is a language that prioritizes **understanding over convenience** and **implementation clarity over abstraction**.

---

## **2. Design Goals and Principles**

* **Grammar-driven definition**

  * The grammar is the authoritative specification of the language.
  * Parsing behavior, operator precedence, and evaluation order are encoded directly in grammar structure.
* **Line-oriented structure**

  * Newlines terminate statements and declarations.
  * No semicolons and no implicit continuation rules.
* **Explicit memory model**

  * Heap allocation occurs only through `new`.
  * Deallocation is explicit via `free`.
  * No implicit copying of arrays or records.
  * Pointer operations are explicit (`bind`, `move`, `free`).
* **Controlled pointer semantics**

  * Pointers are globally scoped and strongly restricted.
  * No pointer arithmetic or pointer-to-pointer constructs.
  * Implicit dereferencing simplifies access while preserving analyzability.
* **Contract-based object model**

  * Records define behavior using `must` and `can` method contracts.
  * Polymorphism is explicit and statically checkable.
  * No inheritance, no virtual dispatch tables.
* **VM-aligned execution semantics**

  * All expressions evaluate to `int`.
  * All functions and methods return `int`.
  * Stack and heap behavior map directly to VM instructions.
* **Implementation transparency**

  * Language features exist only when they map cleanly to:

    * symbol-table entries
    * IR instructions
    * VM operations
* Together, these principles ensure that GWD remains **small, analyzable, and internally coherent**, making it an effective vehicle for understanding how programming languages are built and executed.

---

## **3. Lexical Structure**

* GWD source code uses the **ASCII character set (0–127)** only.
* The language is **line-oriented**:

  * Newlines are **syntactically significant**.
  * A single logical newline token may represent one or more physical newlines.
* **Whitespace (space, tab)** is used only to separate tokens.
* **Comments**:

  * Begin with `#` and extend to the end of the line.
  * Are removed entirely during lexical analysis.
* **Identifiers**:

  * Start with an alphabetic character.
  * Followed by alphanumeric characters.
  * Maximum length: **128 characters**.
  * Case-sensitive.
* **Integer literals**:

  * Decimal only.
  * Limited to **1–9 digits**.
  * Larger values must be produced via computation or runtime behavior.
* **Character literals**:

  * Single ASCII character.
  * No escape sequences.
* **String literals**:

  * Double-quoted.
  * Exist only as literals for I/O; there is **no string type**.
* Tokenization is **fully context-free**:

  * No semantic feedback from the parser.
  * No lexer states or indentation rules.

---

## **4. Concrete Syntax (Grammar)**

* The syntax of GWD is defined by a **single, explicit context-free grammar**.
* The grammar is **normative**:

  * Parser behavior follows the grammar exactly.
  * Static semantic checks are layered on top, not embedded.
* Grammar properties:

  * Designed for **hand-written recursive-descent parsing**.
  * No ambiguity in statement termination due to newline tokens.
  * Operator precedence and associativity are encoded structurally.
* Key structural decisions:

  * **Declarations precede all executable code**.
  * Procedures, functions, and record methods have distinct grammar forms.
  * Statements and expressions are clearly separated syntactic categories.
* Multi-line constructs (`if`, `while`, record definitions) are:

  * Explicitly delimited by keywords.
  * Never inferred from indentation or layout.
* The grammar directly encodes:

  * Pointer restrictions (no pointer dereference syntax).
  * Method-call distinctions (`..` vs `::`).
  * Explicit memory operations (`bind`, `move`, `free`).
* The complete grammar appears in **Appendix A** and matches the parser implementation.

---

## **5. Program Structure**

* A GWD program consists of:

  1. **Global declarations**
  2. **Procedure and method definitions**
* No executable statements are permitted at top level.
* **All declarations are global**:

  * Types
  * Variables
  * Pointers
  * Function and method contracts
* Program ordering rules:

  * Identifiers must be declared **before use**.
  * Definitions/Use must appear **after declarations**.
  * Forward references are not permitted except via explicit contracts.
* Functions and procedures:

  * Must correspond exactly to prior declarations.
  * Always return an `int`.
* Local declarations:

  * Allowed only inside procedures.
  * Restricted to **primitive types only**.
  * No local arrays, records, or pointers.
* There is:

  * No module system
  * No separate compilation
  * No linking stage
* The program forms a **single compilation unit**, enabling:

  * One-pass parsing
  * Deterministic symbol-table construction
  * Straightforward IR generation

---

## **6. Type System Overview**

* GWD is **statically and nominally typed**.
* All types are:

  * explicitly declared,
  * globally scoped,
  * known at parse / symbol-table construction time.
* Type checking is:

  * single-pass,
  * symbol-table driven,
  * grammar-aligned.
* There are **no implicit conversions** between types.
* Every expression in GWD evaluates to an **`int`** value at runtime.
* The type system enforces:

  * assignment compatibility,
  * argument compatibility in calls,
  * field and element access correctness,
  * method contract satisfaction.
* Type safety is achieved by **restriction**, not analysis:

  * no lifetime inference,
  * no ownership tracking (in future ownership tracking may be added),
  * no alias analysis.

---

## **7. Primitive Types**

* GWD defines exactly two primitive types:

  * `int` — 32-bit signed integer
  * `char` — 8-bit ASCII character
* `int`:

  * used for arithmetic, control flow, return values, and error signaling.
  * arithmetic overflow is unchecked.
  * integer literals are limited to **≤ 9 digits**.
* `char`:

  * represents ASCII values 0–127.
  * participates in expressions as integer values.
* There is **no boolean type**:

  * `0` represents false.
  * any non-zero value represents true.
* Primitive values are:

  * stored directly in stack slots or global memory,
  * passed by reference as per calling convention.

---

## **8. Composite Types**

* GWD provides two composite types:

  * **arrays**
  * **records**
* Arrays:

  * fixed-size, compile-time length,
  * single element type,
  * indexed by `int`,
  * no runtime bounds checking.
* Records:

  * nominal types with fixed field layout,
  * may include method contracts (`must`, `can`),
  * do not support inheritance.
  * advocates `program to an interface, not an implementation`
  * advocates `favor composition over inheritance`
  
* Composite type properties:

  * layout is determined at compile time,
  * no implicit copying of entire values,
  * assignment is allowed only to:

    * individual array elements,
    * individual record fields.
* Declaration restrictions:

  * composite types may be declared only at global scope.
  * composite variables may not be declared locally.
* Composite values exist only as:

  * global variables, or
  * heap-allocated objects accessed through pointers.

---

## **9. Pointer Types and References**

* Pointer support in GWD is **explicit and restricted**.
* Pointer declarations:

  * are allowed **only at global scope**.
  * must specify the referenced base type.
* Pointer restrictions:

  * no pointer-to-pointer types,
  * no pointer arithmetic,
  * no address-of operator,
  * no explicit dereference syntax.
* Pointer semantics:

  * accessing a pointer implicitly accesses the referenced object.
  * pointers may refer only to:

    * heap-allocated objects, or
    * global objects.
* Pointer operations are explicit statements:

  * `bind` — associate pointer with a newly allocated or existing object
  * `move` — transfer a reference between pointers
  * `free` — deallocate a heap object
* The language does **not** enforce:

  * single ownership,
  * alias prevention,
  * use-after-free checks.
* Pointer restrictions are designed to:

  * reduce multiple ownership,
  * reduce aliasing forms,
  * keep heap behavior analyzable.

---

## **10. Identifiers and Namespaces**

* GWD uses **distinct, non-overlapping namespaces** for:

  * types
  * variables (including pointers)
  * functions / procedures
  * record members
* Identifiers in different namespaces may share the same textual name.
* Namespace separation is enforced during **symbol-table construction**, not at runtime.
* Identifier constraints:

  * names must be unique **within the same namespace and scope**.
  * duplicate declarations in the same namespace are compile-time errors.
* Record member names:

  * are resolved only through explicit record access.
  * never participate in global or local name lookup.
* Method names (`can`, `must`) are:

  * stored as part of the record type definition.
  * resolved based on the static type of the receiver.

---

## **11. Scope Rules**

* All scope in GWD is **static and lexical**.
* Defined scope levels:

  * **Global scope**
  * **Procedure / function scope**
  * **Record scope** (for fields and methods)
* Global scope:

  * contains all type declarations, global variables, pointers, and function contracts.
  * is visible throughout the program unless shadowed.
* Procedure scope:

  * contains parameters and local primitive variables.
  * parameters and locals may shadow global identifiers of the same namespace.
* Record scope:

  * contains fields and method contracts.
  * is accessible only through record values or pointers.
* Shadowing rules:

  * allowed only within the same namespace.
  * resolved statically at compile time.
* Scope determines **visibility only**, not lifetime:

  * global objects have program lifetime,
  * local primitives have procedure lifetime,
  * record fields live as long as their enclosing record instance.

---

## **12. Symbol Tables and Static Semantics**

* GWD performs **static semantic checks during parsing** using symbol tables.
* A symbol table entry records:

  * identifier name
  * namespace (type, variable, function, record member)
  * kind and associated type or signature
  * enclosing scope
* Symbol tables are:

  * created on scope entry,
  * discarded on scope exit,
  * linked to their enclosing tables.
* Static checks enforced by the compiler:

  * declaration-before-use
  * no duplicate declarations in the same namespace and scope
  * type compatibility in assignments and expressions
  * argument count and type matching in calls
* Record-specific checks:

  * all `must` method contracts are defined exactly once
  * `can` methods are defined at most once
  * method definitions match declared signatures
* Polymorphism checks:

  * `poly` assignments verify contract satisfaction
  * failure results in a compile-time or explicit runtime error
* The compiler does **not** perform:

  * flow-sensitive analysis
  * lifetime inference
  * ownership or alias analysis
* All static semantics are:

  * decidable in a single pass,
  * implemented via symbol-table lookups,
  * independent of runtime execution.

---

## **13. Function Declarations**

* Functions are declared **before definition** using `func`, `must`, or `can`.
* A declaration introduces:

  * function name
  * ordered parameter list with types
  * calling interface (but no body)
* All functions in GWD:

  * **return `int`** (no other return type exists)
  * participate in the unified integer return convention.
* Declarations act as **contracts**:

  * calls are checked against declared parameter lists,
  * definitions must match declarations exactly.
* Static checks:

  * function names must be unique within the function namespace,
  * parameter names must be unique within a declaration,
  * parameter types must be previously declared.
* Method declarations inside records:

  * `must` — required method contract
  * `can` — optional method contract
  * are recorded as part of the record’s type.

---

## **14. Procedure and Function Definitions**

* Definitions provide executable bodies for:

  * global functions (`fdef`)
  * record methods (`cdef`, `mdef`)
* Every definition must match a prior declaration:

  * same name
  * same parameter count
  * same parameter order and types
* Definitions introduce a new scope containing:

  * parameters
  * local primitive variables (`int`, `char` only)
* Procedure body structure is fixed by grammar:

  1. local primitive declarations
  2. executable statements
  3. a mandatory final `return` expression
* Control flow cannot bypass the `return`:

  * all execution paths reach exactly one return.
* Record-specific enforcement:

  * each `must` method is defined exactly once
  * each `can` method is defined at most once
  * method definitions are scoped to their record type

---

## **15. Calling Conventions**

* GWD uses a **single, uniform calling convention**.
* Argument passing:

  * all arguments are passed **by reference**
  * parameters act as new names for arguments
  * arguments are evaluated and passed **left to right**
* Stack frame layout (conceptual):

  * return address
  * previous base pointer
  * parameters (left to right)
  * local variables
  * temporaries
* Return values:

  * primitive results are returned via `ax`
  * composite values and pointers are returned via output parameters
* Caller responsibilities:

  * Push arguments in left-to-right order.
  * Save and restore any registers it modifies.
  * Remove arguments from the stack after the callee returns.
* Callee responsibilities:

  * create and tear down its stack frame and local variables
  * Save and restore any registers it modifies.
  * place return value correctly
* The VM performs:

  * no alias checking
  * no lifetime or ownership validation
* Correctness of pointer and reference usage is the programmer’s responsibility.

---

## **16. Statements Overview**

* Executable code in GWD consists of **explicit statements**, each terminated by a newline.
* Statements are permitted only inside:

  * procedures (`fdef`)
  * record method bodies (`cdef`, `mdef`)
* Statement execution is **strictly sequential** unless altered by control statements.
* There are **no expression statements**:

  * every statement has an explicit keyword or assignment form.
* Side effects are confined to:

  * variable updates
  * heap operations
  * I/O statements

---

## **17. Assignment and I/O Statements**

* Assignment:

  * form: `lvalue = expression`
  * lvalues may be:

    * variables
    * record fields
    * array elements
  * composite values cannot be assigned as wholes.
* Special assignment forms:

  * `poly must_lvalue = record_lvalue`

    * enforces record conformance to a `must` contract.
* Input:

  * `input lvalue`
  * reads a value into an lvalue.
* Output:

  * `print` supports:

    * character array elements
    * record or variable accessors
    * string literals
* I/O operations:

  * are synchronous and blocking
  * perform no formatting beyond raw value output.
* Type checks:

  * assignment types must match exactly
  * no implicit casts are performed.

---

## **18. Control Flow Statements**

* Conditional statement:

  * `if <logical_expression> then`
  * `else`
  * `endif`
* Loop statement:

  * `while <logical_expression> repeat`
  * `endwhile`
* Control constructs:

  * require explicit delimiters
  * do not allow fall-through
  * are not expressions.
* Logical expressions:

  * evaluate to `int`
  * `0` is false, non-zero is true.
* Nested control structures:

  * are fully supported
  * are resolved statically via grammar nesting.
* There are:

  * no `break` or `continue`
  * no `goto`
  * no short-circuit evaluation.

---

## **19. Expressions**

* Expressions in GWD are:

  * side-effect free,
  * evaluated immediately,
  * guaranteed to produce an `int` value.
* Expression evaluation order is:

  * strictly **left to right**,
  * fully determined by the grammar.
* Expressions may appear in:

  * assignments
  * control-flow conditions
  * return statements
* There are:

  * no function calls with side effects inside expressions,
  * no assignment expressions,
  * no conditional expressions.

---

## **20. Operators and Precedence**

* Arithmetic operators:

  * unary: `+`, `-`
  * binary: `+`, `-`, `*`, `/`
* Comparison operators:

  * `==`, `!=`, `<`, `<=`, `>`, `>=`
* Logical operators:

  * unary: `~` (not)
  * binary: `&` (and), `|` (or)
* Operator properties:

  * precedence and associativity are **encoded in the grammar**,
  * no user-defined operators,
  * no operator overloading.
* Logical operators:

  * operate on integer values,
  * no short-circuit behavior.

---

## **21. Accessors and Calls**

* Variable access:

  * direct variable name
  * chained record field access using `.`
  * array indexing using `[ ]`
* Access restrictions:

  * pointer dereference is implicit,
  * no address-of or explicit dereference operators.
* Call forms:

  * function calls: `f(x, y)`
  * record method calls:

    * `obj..method()` for `can` calls and `must` calls
    * `poly_ptr::method()` for polymorphic `must` calls
* Call semantics:

  * argument evaluation is left to right,
  * arguments must be variables (no expression arguments),
  * argument count and order must match declarations.
* Access and call resolution:

  * performed statically for non polymorphic calls,
  * **dynamic dispatch or runtime binding** available for polymorphism.
  * `must` polymorphic calls are checked statically but resolved to indirect calls,
  * no virtual methods or dynamic dispatch table is necessary.

---

## **22. Record Types**

* Records are **nominal composite types** with fixed layout.
* A record definition may contain:

  * primitive or composite fields
  * pointer fields via `ref`
  * method contracts (`can`, `must`)
  * method definitions (`cdef`, `mdef`)
* Record layout:

  * fully determined at compile time,
  * field order is declaration order,
  * there is padding by the compiler.
* Records do **not** support:

  * inheritance
  * subtyping
  * implicit copying
* Record values exist only as:

  * global variables, or
  * heap-allocated objects accessed through pointers.

---

## **23. Methods and Contracts**

* GWD uses **contract-based behavior**, not inheritance.
* Two kinds of method contracts:

  * `must` — required behavior
  * `can` — optional behavior
* Contract rules:

  * each `must` method must be defined exactly once
  * each `can` method may be defined at most once
* Method definitions:

  * `mdef` implements a `must` contract
  * `cdef` implements a `can` contract
* Methods:

  * are statically bound,
  * always return `int`,
  * receive their receiver implicitly as the first argument.
* Method names are resolved:

  * using the static record type,
  * without runtime dispatch for non polymorphic calls.

---

## **24. Polymorphism**

* Polymorphism in GWD is **explicit and structural**.
* Implemented via the `poly` statement:

  * `poly must_accessor = record_accessor`
* Semantics:

  * assigns a record instance to a variable typed by a `must` contract,
  * enforces that the record satisfies required `must` method.
* Polymorphic variables:

  * hold references, not values,
  * cannot be used without prior contract satisfaction.
* Dispatch behavior:

  * `must` method calls are checked statically but resolved to indirect calls,
  * no virtual methods or dynamic dispatch table is necessary.
* The polymorphism mechanism:

  * is checked using symbol-table information,
  * does not require runtime type tags or RTTI.

---

## **25. Memory Allocation**

* GWD uses **explicit, programmer-controlled heap allocation**.
* Heap objects are created only via:

  * `new typename`
* Allocation properties:

  * size and layout are known at compile time,
  * memory is contiguous and record- or array-sized,
  * no automatic initialization beyond default zeroing (if any).
* Heap allocation:

  * returns an address stored in a pointer variable,
  * is permitted only through `bind` statements.
* The language provides:

  * no implicit allocation,
  * no stack allocation for composite types,
  * no garbage collection.

---

## **26. Deallocation and Pointer Movement**

* Heap objects are explicitly deallocated using:

  * `free ptr_accessor`
* Pointer reassignment rules:

  * pointers cannot be reassigned via `=`,
  * reassignment must use `move`.
* Pointer movement:

  * `move dst = src`
  * copies the reference value,
  * does not clone the underlying object.
* Deallocation semantics:

  * `free` invalidates the referenced heap object,
  * all aliases become dangling.
* The compiler and VM:

  * do not detect double-free,
  * do not detect use-after-free,
  * do not enforce ownership.
* Correct memory usage is:

  * syntactically constrained,
  * semantically the programmer’s responsibility.

---

## **27. Intermediate Representation (IR)**

* GWD programs are compiled into a **linear, low-level intermediate representation**.
* The IR is:

  * stack-oriented,
  * instruction-sequenced,
  * designed to map directly onto the VM.
* IR generation occurs:

  * after parsing and static checks,
  * in a single pass over the syntax tree.
* IR instructions encode:

  * arithmetic and logical operations,
  * control-flow jumps,
  * function and method calls,
  * explicit memory operations (`bind`, `move`, `free`).
* There is:

  * no optimization phase,
  * no instruction reordering,
  * no SSA or register allocation.
* IR correctness relies on:

  * prior grammar and symbol-table enforcement.

---

## **28. Virtual Machine**

* GWD executes IR on a **custom stack-based virtual machine**.
* VM components:

  * instruction pointer (IP)
  * stack pointer (SP)
  * base pointer (BP)
  * general-purpose register `ax`
* Execution model:

  * function calls push new stack frames,
  * frames are linked via base pointers,
  * locals and parameters reside on the stack.
* Heap management:

  * allocation and deallocation are driven directly by IR instructions,
  * the VM does not track object ownership or aliases.
* The VM performs:

  * no runtime type checking,
  * no bounds checking,
  * no memory safety checks.

---

## **29. Execution Semantics**

* Program execution begins at the first `fdef` entry point.
* Control flow is:

  * strictly sequential,
  * altered only by explicit jumps generated from control statements.
* Method calls:

  * use the same calling convention as functions,
  * differ only in receiver binding.
* Expression evaluation:

  * produces integer values pushed onto the stack or `ax`.
* Program termination:

  * occurs when the top-level function returns,
  * return value is discarded unless printed.
* Runtime errors:

  * may arise from invalid memory usage or arithmetic faults,
  * are not intercepted by the VM.
* The execution model prioritizes:

  * simplicity,
  * traceability,
  * direct correspondence between source, IR, and VM behavior.

---

## **30. Unified Return Value Scheme**

* GWD uses a **single return-value convention** for all functions and methods.
* Every function and method:

  * returns exactly one `int`,
  * uses the same return register (`ax`).
* The return value is used uniformly to represent:

  * arithmetic results,
  * boolean conditions (`0` / non-zero),
  * error codes,
  * status values.
* Composite objects and pointers:

  * are **not returned directly**,
  * are produced via:

    * output parameters, or
    * pointer mutation.
* Advantages of the unified scheme:

  * simplifies grammar (no return types),
  * simplifies symbol-table entries,
  * avoids multiple calling conventions,
  * maps directly to the VM register model.
* The scheme deliberately avoids:

  * tuple or multiple returns,
  * return-by-structure,
  * implicit heap allocation on return.
* The return model enforces:

  * explicit data flow,
  * visible side effects,
  * predictable stack behavior.

---

## **31. Design Rationale**

* GWD is designed **from implementation constraints outward**, not from surface syntax inward.
* Each language feature exists only if it:

  * can be parsed in a single pass,
  * has a clear symbol-table representation,
  * maps directly to IR instructions,
  * executes predictably on the VM.
* Restrictions are intentional:

  * no inheritance → simpler type checking and layout,
  * restricted pointers → manageable aliasing,
  * explicit memory → transparent heap behavior,
  * no implicit conversions → predictable semantics.
* The grammar is treated as:

  * the authoritative language definition,
  * a direct blueprint for the parser.
* Polymorphism is explicit and contract-based to:

  * avoid dynamic dispatch machinery,
  * keep method resolution static,
  * make object compatibility checkable.
* The unified return scheme reflects the core goal:

  * **one execution model, one calling convention, one value domain**.
* GWD is not meant to scale in features, but in **clarity**:

  * clarity of execution,
  * clarity of compilation,
  * clarity of design trade-offs.

---

## **32. Limitations**

* GWD is intentionally **not feature-complete**.
* Language limitations:

  * no module system or separate compilation,
  * no recursion depth checks,
  * no user-defined operators or macros.
* Type system limitations:

  * no generics or parametric polymorphism,
  * no subtyping or inheritance,
  * no type inference.
* Memory model limitations:

  * no garbage collection,
  * no ownership or lifetime tracking,
  * no detection of double-free or dangling pointers.
* Runtime limitations:

  * no bounds checking for arrays,
  * no runtime type checks,
  * no exception or error-handling mechanism.
* Control-flow limitations:

  * no early returns,
  * no `break` or `continue`,
  * no short-circuit guarantees beyond grammar order.
* These limitations are **deliberate design choices** to:

  * preserve grammar simplicity,
  * keep IR generation linear,
  * maintain a transparent VM execution model.

---

## **33. Conclusion**

* GWD is a **language–compiler–VM co-design**, not just a syntax specification.
* The project demonstrates:

  * how a small grammar can define a complete execution system,
  * how static restrictions simplify runtime behavior,
  * how implementation clarity can guide language design.
* Key characteristics of GWD:

  * grammar-driven parsing,
  * explicit memory and control semantics,
  * contract-based polymorphism without inheritance,
  * a unified integer-based return model.
* GWD serves as:

  * a reference implementation for compiler construction,
  * a controlled environment for language experimentation,
  * an executable design document.
* The language prioritizes:

  * predictability over convenience,
  * transparency over abstraction,
  * coherence between specification and implementation.

---

## Done

* IR output
* vm ISA: 32bit opcode, optional 32bit operand
* disassembler
* interpreter
* debugger
* int datatype 32bit
* char datatype 8bit
* relational operator
* arithmetic operator
* unary arithmetic operator
* assignment
* branching
* looping
* int array
* char array
* record
* pointer datatype
* ref var for easiness
* function
* print string literal
* print char array
* must funcs in record
* can funcs in record
* must var to hold objects
* polymorphism
* new and free
* boolean operator

---

## Plan

* ptr ownership transfer

---
