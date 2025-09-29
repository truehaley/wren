#ifndef OPCODE  // silence warnings while editing
  #define OPCODE(name, size, effect)
#endif

#define NO_ARG        (1)
#define BYTE_ARG      (2)
#define SHORT_ARG     (3)
#define SHORT_ARG2    (5)
#define VARIABLE_ARG  (-1)

// This defines the bytecode instructions used by the VM. It does so by invoking
// an OPCODE() macro which is expected to be defined at the point that this is
// included. (See: http://en.wikipedia.org/wiki/X_Macro for more.)
//
// The first argument is the name of the opcode.
// The second argument is the size of the opcode, generally expressed with the
// macros that define what arguments the opcode takes.
// The third argument is its "stack effect" -- the amount that the
// op code changes the size of the stack.
//  A stack effect of 1 means it pushes a value and the stack grows one larger.
// -2 means it pops two values, etc.
//
// Note that the order of instructions here affects the order of the dispatch
// table in the VM's interpreter loop. That in turn affects caching which
// affects overall performance. Take care to run benchmarks if you change the
// order here.

// Load the constant at index [arg].
OPCODE(CONSTANT, SHORT_ARG, 1)
// Load an immediate (Integer) constant [arg]
OPCODE(ICONSTANT, SHORT_ARG, 1)

// Push null onto the stack.
OPCODE(NULL, NO_ARG, 1)

// Push false onto the stack.
OPCODE(FALSE, NO_ARG, 1)

// Push true onto the stack.
OPCODE(TRUE, NO_ARG, 1)

// Pushes the value in the given local slot.
OPCODE(LOAD_LOCAL_0, NO_ARG, 1)
OPCODE(LOAD_LOCAL_1, NO_ARG, 1)
OPCODE(LOAD_LOCAL_2, NO_ARG, 1)
OPCODE(LOAD_LOCAL_3, NO_ARG, 1)
OPCODE(LOAD_LOCAL_4, NO_ARG, 1)
OPCODE(LOAD_LOCAL_5, NO_ARG, 1)
OPCODE(LOAD_LOCAL_6, NO_ARG, 1)
OPCODE(LOAD_LOCAL_7, NO_ARG, 1)
OPCODE(LOAD_LOCAL_8, NO_ARG, 1)

// Note: The compiler assumes the following _STORE instructions always
// immediately follow their corresponding _LOAD ones.

// Pushes the value in local slot [arg].
OPCODE(LOAD_LOCAL, BYTE_ARG, 1)

// Stores the top of stack in local slot [arg]. Does not pop it.
OPCODE(STORE_LOCAL, BYTE_ARG, 0)

// Pushes the value in upvalue [arg].
OPCODE(LOAD_UPVALUE, BYTE_ARG, 1)

// Stores the top of stack in upvalue [arg]. Does not pop it.
OPCODE(STORE_UPVALUE, BYTE_ARG, 0)

// Pushes the value of the top-level variable in slot [arg].
OPCODE(LOAD_MODULE_VAR, SHORT_ARG, 1)

// Stores the top of stack in top-level variable slot [arg]. Does not pop it.
OPCODE(STORE_MODULE_VAR, SHORT_ARG, 0)

// Pushes the value of the field in slot [arg] of the receiver of the current
// function. This is used for regular field accesses on "this" directly in
// methods. This instruction is faster than the more general CODE_LOAD_FIELD
// instruction.
OPCODE(LOAD_FIELD_THIS, BYTE_ARG, 1)

// Stores the top of the stack in field slot [arg] in the receiver of the
// current value. Does not pop the value. This instruction is faster than the
// more general CODE_LOAD_FIELD instruction.
OPCODE(STORE_FIELD_THIS, BYTE_ARG, 0)

// Pops an instance and pushes the value of the field in slot [arg] of it.
OPCODE(LOAD_FIELD, BYTE_ARG, 0)

// Pops an instance and stores the subsequent top of stack in field slot
// [arg] in it. Does not pop the value.
OPCODE(STORE_FIELD, BYTE_ARG, -1)

// Pop and discard the top of stack.
OPCODE(POP, NO_ARG, -1)

// Invoke the method with symbol [arg]. The number indicates the number of
// arguments (not including the receiver).
OPCODE(CALL_0,  SHORT_ARG,  0)
OPCODE(CALL_1,  SHORT_ARG, -1)
OPCODE(CALL_2,  SHORT_ARG, -2)
OPCODE(CALL_3,  SHORT_ARG, -3)
OPCODE(CALL_4,  SHORT_ARG, -4)
OPCODE(CALL_5,  SHORT_ARG, -5)
OPCODE(CALL_6,  SHORT_ARG, -6)
OPCODE(CALL_7,  SHORT_ARG, -7)
OPCODE(CALL_8,  SHORT_ARG, -8)
OPCODE(CALL_9,  SHORT_ARG, -9)
OPCODE(CALL_10, SHORT_ARG, -10)
OPCODE(CALL_11, SHORT_ARG, -11)
OPCODE(CALL_12, SHORT_ARG, -12)
OPCODE(CALL_13, SHORT_ARG, -13)
OPCODE(CALL_14, SHORT_ARG, -14)
OPCODE(CALL_15, SHORT_ARG, -15)
OPCODE(CALL_16, SHORT_ARG, -16)

// Invoke a superclass method with symbol [arg] and superclass [arg].
// The number indicates the number of arguments (not including the receiver).
OPCODE(SUPER_0,  SHORT_ARG2,  0)
OPCODE(SUPER_1,  SHORT_ARG2, -1)
OPCODE(SUPER_2,  SHORT_ARG2, -2)
OPCODE(SUPER_3,  SHORT_ARG2, -3)
OPCODE(SUPER_4,  SHORT_ARG2, -4)
OPCODE(SUPER_5,  SHORT_ARG2, -5)
OPCODE(SUPER_6,  SHORT_ARG2, -6)
OPCODE(SUPER_7,  SHORT_ARG2, -7)
OPCODE(SUPER_8,  SHORT_ARG2, -8)
OPCODE(SUPER_9,  SHORT_ARG2, -9)
OPCODE(SUPER_10, SHORT_ARG2, -10)
OPCODE(SUPER_11, SHORT_ARG2, -11)
OPCODE(SUPER_12, SHORT_ARG2, -12)
OPCODE(SUPER_13, SHORT_ARG2, -13)
OPCODE(SUPER_14, SHORT_ARG2, -14)
OPCODE(SUPER_15, SHORT_ARG2, -15)
OPCODE(SUPER_16, SHORT_ARG2, -16)

// Jump the instruction pointer [arg] forward.
OPCODE(JUMP, SHORT_ARG, 0)

// Jump the instruction pointer [arg] backward.
OPCODE(LOOP, SHORT_ARG, 0)

// Pop and if not truthy then jump the instruction pointer [arg] forward.
OPCODE(JUMP_IF, SHORT_ARG, -1)

// Standard arithmetic operations.  If called on two number values
// the VM will execute the operation directly, otherwise it will
// call the associated method with symbol [arg]
OPCODE(ADD, SHORT_ARG, -1)
OPCODE(SUB, SHORT_ARG, -1)
OPCODE(MUL, SHORT_ARG, -1)
OPCODE(DIV, SHORT_ARG, -1)
OPCODE(MOD, SHORT_ARG, -1)

// If the top of the stack is false, jump [arg] forward. Otherwise, pop and
// continue.
OPCODE(AND, SHORT_ARG, -1)

// If the top of the stack is non-false, jump [arg] forward. Otherwise, pop
// and continue.
OPCODE(OR, SHORT_ARG, -1)

// Close the upvalue for the local on the top of the stack, then pop it.
OPCODE(CLOSE_UPVALUE, NO_ARG, -1)

// Exit from the current function and return the value on the top of the
// stack.
OPCODE(RETURN, NO_ARG, 0)

// Creates a closure for the function stored at [arg] in the constant table.
//
// Following the function argument is a number of arguments, two for each
// upvalue. The first is true if the variable being captured is a local (as
// opposed to an upvalue), and the second is the index of the local or
// upvalue being captured.
//
// Pushes the created closure.
OPCODE(CLOSURE, VARIABLE_ARG, 1)

// Creates a new instance of a class.
//
// Assumes the class object is in slot zero, and replaces it with the new
// uninitialized instance of that class. This opcode is only emitted by the
// compiler-generated constructor metaclass methods.
OPCODE(CONSTRUCT, NO_ARG, 0)

// Creates a new instance of a foreign class.
//
// Assumes the class object is in slot zero, and replaces it with the new
// uninitialized instance of that class. This opcode is only emitted by the
// compiler-generated constructor metaclass methods.
OPCODE(FOREIGN_CONSTRUCT, NO_ARG, 0)

// Creates a class. Top of stack is the superclass. Below that is a string for
// the name of the class. Byte [arg] is the number of fields in the class.
OPCODE(CLASS, BYTE_ARG, -1)

// Ends a class.
// Atm the stack contains the class and the ClassAttributes (or null).
OPCODE(END_CLASS, NO_ARG, -2)

// Creates a foreign class. Top of stack is the superclass. Below that is a
// string for the name of the class.
OPCODE(FOREIGN_CLASS, NO_ARG, -1)

// Define a method for symbol [arg]. The class receiving the method is popped
// off the stack, then the function defining the body is popped.
//
// If a foreign method is being defined, the "function" will be a string
// identifying the foreign method. Otherwise, it will be a function or
// closure.
OPCODE(METHOD_INSTANCE, SHORT_ARG, -2)

// Define a method for symbol [arg]. The class whose metaclass will receive
// the method is popped off the stack, then the function defining the body is
// popped.
//
// If a foreign method is being defined, the "function" will be a string
// identifying the foreign method. Otherwise, it will be a function or
// closure.
OPCODE(METHOD_STATIC, SHORT_ARG, -2)

// This is executed at the end of the module's body. Pushes NULL onto the stack
// as the "return value" of the import statement and stores the module as the
// most recently imported one.
OPCODE(END_MODULE, NO_ARG, 1)

// Import a module whose name is the string stored at [arg] in the constant
// table.
//
// Pushes null onto the stack so that the fiber for the imported module can
// replace that with a dummy value when it returns. (Fibers always return a
// value when resuming a caller.)
OPCODE(IMPORT_MODULE, SHORT_ARG, 1)

// Import a variable from the most recently imported module. The name of the
// variable to import is at [arg] in the constant table. Pushes the loaded
// variable's value.
OPCODE(IMPORT_VARIABLE, SHORT_ARG, 1)

// This pseudo-instruction indicates the end of the bytecode. It should
// always be preceded by a `CODE_RETURN`, so is never actually executed.
// It should also always be the last opcode defined
OPCODE(END, NO_ARG, 0)

#undef OPCODE
#undef NO_ARG
#undef BYTE_ARG
#undef SHORT_ARG
#undef SHORT_ARG2
#undef VARIABLE_ARG
