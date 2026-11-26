/*
 * Copyright 2026, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <bea/runtime.h>

extern Object *__main__main();

/* allocate object */
extern Object *__std__allocate(Class *class) {

	Object *object = (Object *)malloc(class->size);
	memset(object, 0, class->size);

	object->root = object;
	object->class = class;
	object->vtable = class->interfaces[0];

	return object;
}

/* assign interface */
extern Object *__std__assign(Object *object, size_t index, Class *class) {

	Interface *interface = object->class->interfaces[index];
	Object *component = (Object *)((void *)object + interface->position);

	component->root = object->root;
	component->class = class;
	component->vtable = class->interfaces[0];

	return object;
}

/* cast object */
extern Object *__std__cast(Object *object, uint32_t id) {

	if (object->class->interfaces[0]->id == id)
		return object;

	object = object->root;
	if (object->class->interfaces[0]->id == id)
		return object;

	for (size_t i = 1; i < object->class->count; i++) {

		Interface *interface = object->class->interfaces[i];
		if (interface->id == id)
			return (Object *)((void *)object + interface->position);
	}
	return NULL;
}

/* free object */
extern void __std__free(Object *object) {

	object = object->root;
	free(object);
}

/* destroy object */
extern void __std__destroy(Object *object) {

	if (!object) return;

	object = object->root;
	object->class->destructor(object);
	__std__free(object);
}

/* increase or decrease reference count */
extern Object *__std__incref(Object *object) {

	if (!object) return NULL;

	Object *orig = object;

	object = object->root;
	object->refcnt++;

	return orig;
}

extern void __std__decref(Object *object) {

	if (!object) return;

	object = object->root;
	object->refcnt--;

	if (object->refcnt <= 0)
		__std__destroy(object);
}

extern Object *__std__decinc(Object *old, Object *new) {

	if (old == new)
		return new;

	__std__decref(old);
	return __std__incref(new);
}

extern void __std__decrefSafe(Object *object) {

	object = object->root;
	object->refcnt--;
}

extern void __std__delete(Object *object) {

	__std__incref(object);
	__std__decref(object);
}

/* increase or decrease reference count for any type */
extern void __std__increfAny(Any *result, Any *value) {

	if (result) *result = *value;

	if (value->type != BaseTypeObject)
		return;
	__std__incref((Object *)value->value);
}

extern void __std__decrefAny(Any *value) {

	if (value->type != BaseTypeObject)
		return;
	__std__decref((Object *)value->value);
}

extern Any *__std__decincAny(Any *result, Any *old, Any *new) {

	if (result) *result = *new;

	if (old->type == BaseTypeObject &&
	    new->type == BaseTypeObject &&
	    old->value == new->value)
		return result;

	__std__decrefAny(old);
	__std__increfAny(NULL, new);

	return result;
}

extern void __std__decrefSafeAny(Any *value) {

	if (value->type != BaseTypeObject)
		return;
	__std__decrefSafe((Object *)value->value);
}

extern void __std__deleteAny(Any *value) {

	if (value->type != BaseTypeObject)
		return;
	__std__delete((Object *)value->value);
}

/* print message (std::runtime::println) */
extern Object *std__runtime__println____0(Object *object) {

	String *message = (String *)object->root;

	printf("%s\n", message->pointer);

	__std__delete(object);
	return NULL;
}

/* main */
extern uint64_t ____beainit();

extern uint64_t __std__main(void (*proc_main)(void)) {

	proc_main();
	return 0;
}

int main() {

	return (int)____beainit();
}
