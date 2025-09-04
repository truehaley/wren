#include <stdio.h>
#include <string.h>

#include "foreign_class.h"

static int finalized = 0;

static void apiFinalized(WrenVM* vm)
{
  wrenSetSlotNumber(vm, 0, finalized);
}

static void counterAllocate(WrenVM* vm)
{
  WrenNum* value = (WrenNum*)wrenSetSlotNewForeign(vm, 0, 0, sizeof(WrenNum));
  *value = 0;
}

static void counterIncrement(WrenVM* vm)
{
  WrenNum* value = (WrenNum*)wrenGetSlotForeign(vm, 0);
  WrenNum increment = wrenGetSlotNumber(vm, 1);

  *value += increment;
}

static void counterValue(WrenVM* vm)
{
  WrenNum value = *(WrenNum*)wrenGetSlotForeign(vm, 0);
  wrenSetSlotNumber(vm, 0, value);
}

static void pointAllocate(WrenVM* vm)
{
  WrenNum* coordinates = (WrenNum*)wrenSetSlotNewForeign(vm, 0, 0, sizeof(WrenNum[3]));

  // This gets called by both constructors, so sniff the slot count to see
  // which one was invoked.
  if (wrenGetSlotCount(vm) == 1)
  {
    coordinates[0] = 0.0;
    coordinates[1] = 0.0;
    coordinates[2] = 0.0;
  }
  else
  {
    coordinates[0] = wrenGetSlotNumber(vm, 1);
    coordinates[1] = wrenGetSlotNumber(vm, 2);
    coordinates[2] = wrenGetSlotNumber(vm, 3);
  }
}

static void pointTranslate(WrenVM* vm)
{
  WrenNum* coordinates = (WrenNum*)wrenGetSlotForeign(vm, 0);
  coordinates[0] += wrenGetSlotNumber(vm, 1);
  coordinates[1] += wrenGetSlotNumber(vm, 2);
  coordinates[2] += wrenGetSlotNumber(vm, 3);
}

static void pointToString(WrenVM* vm)
{
  WrenNum* coordinates = (WrenNum*)wrenGetSlotForeign(vm, 0);
  char result[100];
  sprintf(result, "(%g, %g, %g)",
      coordinates[0], coordinates[1], coordinates[2]);
  wrenSetSlotString(vm, 0, result);
}

static void resourceAllocate(WrenVM* vm)
{
  int* value = (int*)wrenSetSlotNewForeign(vm, 0, 0, sizeof(int));
  *value = 123;
}

static void resourceFinalize(void* data)
{
  // Make sure we get the right data back.
  int* value = (int*)data;
  if (*value != 123) exit(1);

  finalized++;
}

static void badClassAllocate(WrenVM* vm)
{
  wrenEnsureSlots(vm, 1);
  wrenSetSlotString(vm, 0, "Something went wrong");
  wrenAbortFiber(vm, 0);
}

WrenForeignMethodFn foreignClassBindMethod(const char* signature)
{
  if (strcmp(signature, "static ForeignClass.finalized") == 0) return apiFinalized;
  if (strcmp(signature, "Counter.increment(_)") == 0) return counterIncrement;
  if (strcmp(signature, "Counter.value") == 0) return counterValue;
  if (strcmp(signature, "Point.translate(_,_,_)") == 0) return pointTranslate;
  if (strcmp(signature, "Point.toString") == 0) return pointToString;

  return NULL;
}

void foreignClassBindClass(
    const char* className, WrenForeignClassMethods* methods)
{
  if (strcmp(className, "Counter") == 0)
  {
    methods->allocate = counterAllocate;
    return;
  }

  if (strcmp(className, "Point") == 0)
  {
    methods->allocate = pointAllocate;
    return;
  }

  if (strcmp(className, "Resource") == 0)
  {
    methods->allocate = resourceAllocate;
    methods->finalize = resourceFinalize;
    return;
  }

  if (strcmp(className, "BadClass") == 0)
  {
    methods->allocate = badClassAllocate;
    return;
  }
}
