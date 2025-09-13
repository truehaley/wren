#include <stdio.h>

#include "wren_debug.h"

void wrenDebugPrintStackTrace(WrenVM* vm)
{
  // Bail if the host doesn't enable printing errors.
  if (vm->config.errorFn == NULL) return;

  ObjFiber* fiber = vm->fiber;
  if (IS_STRING(fiber->error))
  {
    vm->config.errorFn(vm, WREN_ERROR_RUNTIME,
                       NULL, -1, AS_CSTRING(fiber->error));
  }
  else
  {
    // TODO: Print something a little useful here. Maybe the name of the error's
    // class?
    vm->config.errorFn(vm, WREN_ERROR_RUNTIME,
                       NULL, -1, "[error object]");
  }

  for (int i = fiber->numFrames - 1; i >= 0; i--)
  {
    CallFrame* frame = &fiber->frames[i];
    ObjFn* fn = frame->closure->fn;

    // Skip over stub functions for calling methods from the C API.
    if (fn->module == NULL) continue;

    // The built-in core module has no name. We explicitly omit it from stack
    // traces since we don't want to highlight to a user the implementation
    // detail of what part of the core module is written in C and what is Wren.
    if (fn->module->name == NULL) continue;

    // -1 because IP has advanced past the instruction that it just executed.
    int line = fn->debug->sourceLines.data[frame->ip - fn->code.data - 1];
    vm->config.errorFn(vm, WREN_ERROR_STACK_TRACE,
                       fn->module->name->value, line,
                       fn->debug->name);
  }
}

static void dumpObject(Obj* obj)
{
  switch (obj->type)
  {
    case OBJ_CLASS:
      printf("[class %s %p]", ((ObjClass*)obj)->name->value, obj);
      break;
    case OBJ_CLOSURE: printf("[closure %p]", obj); break;
    case OBJ_FIBER: printf("[fiber %p]", obj); break;
    case OBJ_FN: printf("[fn %p]", obj); break;
    case OBJ_FOREIGN: printf("[foreign %p]", obj); break;
    case OBJ_INSTANCE: printf("[instance %p]", obj); break;
    case OBJ_LIST: printf("[list %p]", obj); break;
    case OBJ_MAP: printf("[map %p]", obj); break;
    case OBJ_MODULE: printf("[module %p]", obj); break;
    case OBJ_RANGE: printf("[range %p]", obj); break;
    case OBJ_STRING: printf("%s", ((ObjString*)obj)->value); break;
    case OBJ_UPVALUE: printf("[upvalue %p]", obj); break;
    default: printf("[unknown object %d]", obj->type); break;
  }
}

void wrenDumpValue(Value value)
{
#if WREN_NAN_TAGGING
  if (IS_NUM(value))
  {
    #ifdef WREN_FLOAT32
      printf("%.8g", AS_NUM(value));
    #else
      printf("%.14g", AS_NUM(value));
    #endif
  }
  else if (IS_OBJ(value))
  {
    dumpObject(AS_OBJ(value));
  }
  else
  {
    switch (GET_TAG(value))
    {
      case TAG_FALSE:     printf("false"); break;
      case TAG_NAN:       printf("NaN"); break;
      case TAG_NULL:      printf("null"); break;
      case TAG_TRUE:      printf("true"); break;
      case TAG_UNDEFINED: UNREACHABLE();
    }
  }
#else
  switch (value.type)
  {
    case VAL_FALSE:     printf("false"); break;
    case VAL_NULL:      printf("null"); break;
    case VAL_TRUE:      printf("true"); break;
    case VAL_OBJ:       dumpObject(AS_OBJ(value)); break;
    case VAL_UNDEFINED: UNREACHABLE();
    case VAL_NUM:
    #ifdef WREN_FLOAT32
      printf("%.8g", AS_NUM(value)); break;
    #else
      printf("%.14g", AS_NUM(value)); break;
    #endif
  }
#endif
}

static const char* const opcodeName[] = {
  #define OPCODE(name, _) #name,
  #include "wren_opcodes.h"
};

static int dumpInstruction(WrenVM* vm, ObjFn* fn, int i, int* lastLine)
{
  int start = i;
  uint8_t* bytecode = fn->code.data;
  Code code = (Code)bytecode[i];

  int line = fn->debug->sourceLines.data[i];
  if (lastLine == NULL || *lastLine != line)
  {
    printf("%4d:", line);
    if (lastLine != NULL) *lastLine = line;
  }
  else
  {
    printf("     ");
  }

  printf(" %04d  ", i++);

  #define READ_BYTE() (bytecode[i++])
  #define READ_SHORT() (i += 2, (bytecode[i - 2] << 8) | bytecode[i - 1])

  #define BARE_INSTRUCTION()  do { printf("%s\n", opcodeName[code]); } while(0)

  #define BYTE_INSTRUCTION()  do { printf("%-16s %5d\n", opcodeName[code], READ_BYTE()); } while(0)

  switch (code)
  {
    case CODE_CONSTANT:
    {
      int constant = READ_SHORT();
      printf("%-16s %5d '", opcodeName[code], constant);
      wrenDumpValue(fn->constants.data[constant]);
      printf("'\n");
      break;
    }

    case CODE_ICONSTANT:
    {
      int num = READ_SHORT();
      printf("%-16s %5d '%u'\n", opcodeName[code], num, num);
      break;
    }

    case CODE_NULL:
    case CODE_FALSE:
    case CODE_TRUE:
      BARE_INSTRUCTION();
      break;

    case CODE_LOAD_LOCAL_0:
    case CODE_LOAD_LOCAL_1:
    case CODE_LOAD_LOCAL_2:
    case CODE_LOAD_LOCAL_3:
    case CODE_LOAD_LOCAL_4:
    case CODE_LOAD_LOCAL_5:
    case CODE_LOAD_LOCAL_6:
    case CODE_LOAD_LOCAL_7:
    case CODE_LOAD_LOCAL_8:
      BARE_INSTRUCTION();
      break;

    case CODE_LOAD_LOCAL:
    case CODE_STORE_LOCAL:
    case CODE_LOAD_UPVALUE:
    case CODE_STORE_UPVALUE:
      BYTE_INSTRUCTION();
      break;

    case CODE_LOAD_MODULE_VAR:
    case CODE_STORE_MODULE_VAR:
    {
      int slot = READ_SHORT();
      printf("%-16s %5d '%s'\n", opcodeName[code], slot,
             fn->module->variableNames.data[slot]->value);
      break;
    }

    case CODE_LOAD_FIELD_THIS:
    case CODE_STORE_FIELD_THIS:
    case CODE_LOAD_FIELD:
    case CODE_STORE_FIELD:
      BYTE_INSTRUCTION();
      break;

    case CODE_POP: BARE_INSTRUCTION(); break;

    case CODE_CALL_0:
    case CODE_CALL_1:
    case CODE_CALL_2:
    case CODE_CALL_3:
    case CODE_CALL_4:
    case CODE_CALL_5:
    case CODE_CALL_6:
    case CODE_CALL_7:
    case CODE_CALL_8:
    case CODE_CALL_9:
    case CODE_CALL_10:
    case CODE_CALL_11:
    case CODE_CALL_12:
    case CODE_CALL_13:
    case CODE_CALL_14:
    case CODE_CALL_15:
    case CODE_CALL_16:
    {
      int symbol = READ_SHORT();
      printf("%-16s %5d '%s'\n", opcodeName[code], symbol,
             vm->methodNames.data[symbol]->value);
      break;
    }

    case CODE_SUPER_0:
    case CODE_SUPER_1:
    case CODE_SUPER_2:
    case CODE_SUPER_3:
    case CODE_SUPER_4:
    case CODE_SUPER_5:
    case CODE_SUPER_6:
    case CODE_SUPER_7:
    case CODE_SUPER_8:
    case CODE_SUPER_9:
    case CODE_SUPER_10:
    case CODE_SUPER_11:
    case CODE_SUPER_12:
    case CODE_SUPER_13:
    case CODE_SUPER_14:
    case CODE_SUPER_15:
    case CODE_SUPER_16:
    {
      int symbol = READ_SHORT();
      int superclass = READ_SHORT();
      printf("%-16s %5d '%s' %5d\n", opcodeName[code], symbol,
             vm->methodNames.data[symbol]->value, superclass);
      break;
    }


    case CODE_LOOP:
    {
      int offset = READ_SHORT();
      printf("%-16s %5d to %d\n", opcodeName[code], offset, i - offset);
      break;
    }

    case CODE_JUMP:
    case CODE_JUMP_IF:
    case CODE_AND:
    case CODE_OR:
    {
      int offset = READ_SHORT();
      printf("%-16s %5d to %d\n", opcodeName[code], offset, i + offset);
      break;
    }

    case CODE_ADD:
    case CODE_SUB:
    case CODE_MUL:
    case CODE_DIV:
    case CODE_MOD:
    {
      int symbol = READ_SHORT();
      printf("%-16s %5d '%s'\n", opcodeName[code], symbol,
             vm->methodNames.data[symbol]->value);
      break;
    }

    case CODE_CLOSE_UPVALUE:
    case CODE_RETURN:
      BARE_INSTRUCTION();
      break;

    case CODE_CLOSURE:
    {
      int constant = READ_SHORT();
      printf("%-16s %5d ", opcodeName[code], constant);
      wrenDumpValue(fn->constants.data[constant]);
      printf(" ");
      ObjFn* loadedFn = AS_FN(fn->constants.data[constant]);
      for (int j = 0; j < loadedFn->numUpvalues; j++)
      {
        int isLocal = READ_BYTE();
        int index = READ_BYTE();
        if (j > 0) printf(", ");
        printf("%s %d", isLocal ? "local" : "upvalue", index);
      }
      printf("\n");
      break;
    }

    case CODE_CONSTRUCT:
    case CODE_FOREIGN_CONSTRUCT:
      BARE_INSTRUCTION();
      break;

    case CODE_CLASS:
    {
      int numFields = READ_BYTE();
      printf("%-16s %5d fields\n", opcodeName[code], numFields);
      break;
    }

    case CODE_FOREIGN_CLASS:
    case CODE_END_CLASS:
      BARE_INSTRUCTION();
      break;

    case CODE_METHOD_INSTANCE:
    case CODE_METHOD_STATIC:
    {
      int symbol = READ_SHORT();
      printf("%-16s %5d '%s'\n", opcodeName[code], symbol,
             vm->methodNames.data[symbol]->value);
      break;
    }

    case CODE_END_MODULE:
      BARE_INSTRUCTION();
      break;

    case CODE_IMPORT_MODULE:
    case CODE_IMPORT_VARIABLE:
    {
      int name = READ_SHORT();
      printf("%-16s %5d '", opcodeName[code], name);
      wrenDumpValue(fn->constants.data[name]);
      printf("'\n");
      break;
    }

    case CODE_END:
      printf("END\n");
      break;

    default:
      printf("UKNOWN! [%d]\n", bytecode[i - 1]);
      break;
  }

  // Return how many bytes this instruction takes, or -1 if it's an END.
  if (code == CODE_END) return -1;
  return i - start;

  #undef READ_BYTE
  #undef READ_SHORT
}

int wrenDumpInstruction(WrenVM* vm, ObjFn* fn, int i)
{
  return dumpInstruction(vm, fn, i, NULL);
}

void wrenDumpCode(WrenVM* vm, ObjFn* fn)
{
  printf("%s: %s\n",
         fn->module->name == NULL ? "<core>" : fn->module->name->value,
         fn->debug->name);

  int i = 0;
  int lastLine = -1;
  for (;;)
  {
    int offset = dumpInstruction(vm, fn, i, &lastLine);
    if (offset == -1) break;
    i += offset;
  }

  printf("\n");
}

void wrenDumpStack(ObjFiber* fiber)
{
  printf("(fiber %p) ", fiber);
  for (Value* slot = fiber->stack; slot < fiber->stackTop; slot++)
  {
    wrenDumpValue(*slot);
    printf(" | ");
  }
  printf("\n");
}
