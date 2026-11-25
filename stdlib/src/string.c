#include <stdio.h>
#include <stdlib.h>
#include <bea/runtime.h>

#define StringClass std__string__String____class
extern Class StringClass;

extern Object *std__string__String__new0(void);

/* custom string destructor */
static void dtor(String *string) {

	if (string->mode == StringModeRW)
		free(string->pointer);
	string->pointer = NULL;
}

/* create string object from literal */
extern Object *__std__string__newconst(uint8_t *literal) {

	String *string = (String *)__std__allocate(&StringClass);
	string->_base.refcnt++;

	string->mode = StringModeRead;
	string->pointer = literal;

	return (Object *)string;
}

/* initialize string object */
extern Object *std__string__String__init____0(String *string) {

	string->_base.class->destructor = (ObjectDestructor)dtor;
	return NULL;
}
