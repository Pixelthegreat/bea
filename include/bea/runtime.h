/*
 * Copyright 2026, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef BEA_RUNTIME_H
#define BEA_RUNTIME_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef int64_t Int;
typedef double Float;
typedef uint8_t Boolean;
typedef void *Function;

typedef struct Object Object;

/*
 * Data type identifiers
 */
typedef enum BaseType {
	BaseTypeNone = 0,

	BaseTypeU8,
	BaseTypeI8,
	BaseTypeU16,
	BaseTypeI16,
	BaseTypeU32,
	BaseTypeI32,
	BaseTypeU64,
	BaseTypeI64,
	BaseTypeF32,
	BaseTypeF64,

	BaseTypeFunction,
	BaseTypeObject,
	BaseTypeAny,

	BaseTypeCount,

	BaseTypeInt = BaseTypeI64,
	BaseTypeFloat = BaseTypeF64,
	BaseTypeBoolean = BaseTypeU8,
} BaseType;

/*
 * Object class
 */
typedef void (*ObjectDestructor)(Object *object);

typedef struct Interface {
	uint32_t id;
	size_t position;
	void *functions[];
} __attribute__((packed)) Interface;

typedef struct Class {
	size_t count;
	size_t size;
	ObjectDestructor destructor;
	void *reserved;
	Interface *interfaces[];
} __attribute__((packed)) Class;

/*
 * Base of object
 */
struct Object {
	Object *root;
	Class *class;
	Interface *vtable;
	int32_t refcnt;
} __attribute__((packed));

/*
 * Generic type container
 */
typedef struct Any {
	uint64_t type;
	uint64_t value;
} __attribute__((packed)) Any;

/*
 * Error object
 */
typedef struct Error {
	Object _base;
	Object *message;
} __attribute__((packed)) Error;

/*
 * Result object
 */
typedef struct Result {
	Object _base;
	Any value;
	Object error;
	Boolean okay;
} __attribute__((packed)) Result;

/*
 * String object
 */
typedef enum StringMode {
	StringModeRead = 0,
	StringModeRW,

	StringModeCount,
} StringMode;

typedef struct String {
	Object _base;
	void *pointer;
	uint64_t mode;
} __attribute__((packed)) String;

/*
 * Common utility functions
 */
extern Object *__std__allocate(Class *class);
extern Object *__std__assign(Object *object, size_t index, Class *class);
extern Object *__std__cast(Object *object, uint32_t id);
extern void __std__free(Object *object);
extern void __std__destroy(Object *object);
extern Object *__std__incref(Object *object);
extern void __std__decref(Object *object);
extern Object *__std__decinc(Object *old, Object *new);
extern void __std__decrefSafe(Object *object);
extern void __std__delete(Object *object);

extern void __std__increfAny(Any *result, Any *value);
extern void __std__decrefAny(Any *value);
extern Any *__std__decincAny(Any *result, Any *old, Any *new);
extern void __std__decrefSafeAny(Any *value);
extern void __std__deleteAny(Any *value);

#endif /* BEA_RUNTIME_H */
