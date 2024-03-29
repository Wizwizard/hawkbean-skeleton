/* 
 * This file is part of the Hawkbeans JVM developed by
 * the HExSA Lab at Illinois Institute of Technology.
 *
 * Copyright (c) 2019, Kyle C. Hale <khale@cs.iit.edu>
 *
 * All rights reserved.
 *
 * Author: Kyle C. Hale <khale@cs.iit.edu>
 *
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the 
 * file "LICENSE.txt".
 */
#include <stdlib.h>
#include <string.h>

#include <types.h>
#include <class.h>
#include <stack.h>
#include <mm.h>
#include <thread.h>
#include <exceptions.h>
#include <bc_interp.h>
#include <gc.h>

extern jthread_t * cur_thread;

/* 
 * Maps internal exception identifiers to fully
 * qualified class paths for the exception classes.
 * Note that the ones without fully qualified paths
 * will not be properly raised. 
 *
 * TODO: add the classes for these
 *
 */
static const char * excp_strs[16] __attribute__((used)) =
{
	"java/lang/NullPointerException",
	"java/lang/IndexOutOfBoundsException",
	"java/lang/ArrayIndexOutOfBoundsException",
	"IncompatibleClassChangeError",
	"java/lang/NegativeArraySizeException",
	"java/lang/OutOfMemoryError",
	"java/lang/ClassNotFoundException",
	"java/lang/ArithmeticException",
	"java/lang/NoSuchFieldError",
	"java/lang/NoSuchMethodError",
	"java/lang/RuntimeException",
	"java/io/IOException",
	"FileNotFoundException",
	"java/lang/InterruptedException",
	"java/lang/NumberFormatException",
	"java/lang/StringIndexOutOfBoundsException",
};

int 
hb_excp_str_to_type (char * str)
{
    for (int i = 0; i < sizeof(excp_strs)/sizeof(char*); i++) {
        if (strstr(excp_strs[i], str))
                return i;
    }
    return -1;
}



/*
 * Throws an exception given an internal ID
 * that refers to an exception type. This is to 
 * be used by the runtime (there is no existing
 * exception object, so we have to create a new one
 * and init it).
 *
 * @return: none. exits on failure.
 *
 */
// WRITE ME
void
hb_throw_and_create_excp (u1 type)
{
	java_class_t * ecls = NULL;
	obj_ref_t * eref = NULL;

	ecls = hb_get_or_load_class(excp_strs[type]);
	if(!ecls) {
		// MAX to-do
		// how to check properly?
		HB_ERR("NULL cls in %s", __func__);
		exit(EXIT_FAILURE);
	}
	eref = gc_obj_alloc(ecls);

	if(!eref) {
		HB_ERR("OOM when creating excp %s", excp_strs[type]);
		exit(EXIT_FAILURE);
	}	

	hb_throw_exception(eref);
}

/* 
 * gets the exception message from the object 
 * ref referring to the exception object.
 *
 * NOTE: caller must free the string
 *
 */
static char *
get_excp_str (obj_ref_t * eref)
{
	// check this function
	char * ret;
	native_obj_t * obj = (native_obj_t*)eref->heap_ptr;
		
	obj_ref_t * str_ref = obj->fields[0].obj;
	native_obj_t * str_obj;
	obj_ref_t * arr_ref;
	native_obj_t * arr_obj;
	int i;
	
	if (!str_ref) {
		return NULL;
	}

	str_obj = (native_obj_t*)str_ref->heap_ptr;
	
	arr_ref = str_obj->fields[0].obj;

	if (!arr_ref) {
		return NULL;
	}

	arr_obj = (native_obj_t*)arr_ref->heap_ptr;

	ret = malloc(arr_obj->flags.array.length+1);

	for (i = 0; i < arr_obj->flags.array.length; i++) {
		ret[i] = arr_obj->fields[i].char_val;
	}

	ret[i] = 0;

	return ret;
}


bool check_if_class_or_subclass(java_class_t * cls, const char * target_cls_name)
{
	if (!cls) {
		return false;
	}

	if(strcmp(hb_get_class_name(cls), target_cls_name) == 0) {
		return true;
	} else {
		return check_if_class_or_subclass(hb_get_super_class(cls), target_cls_name);
	}
}

/*
 * Throws an exception using an
 * object reference to some exception object (which
 * implements Throwable). To be used with athrow.
 * If we're given a bad reference, we throw a 
 * NullPointerException.
 *
 * @return: none. exits on failure.  
 *
 */
void
hb_throw_exception (obj_ref_t * eref)
{
	if (!eref) {
		hb_throw_and_create_excp(EXCP_NULL_PTR);
	}

	stack_frame_t * frame = NULL;
	java_class_t * cls = NULL;
	java_class_t * ecls = NULL;
	native_obj_t * obj = NULL;

	obj = (native_obj_t *)eref->heap_ptr;
	ecls = obj->class;

	// MAX to-do how to identify the class instance?

	code_attr_t * code_attr = NULL;
	int table_len = 0;
	excp_table_t * excp_table = NULL;
	CONSTANT_Class_info_t * exp_cls_info = NULL;


	int i;

	while(cur_thread->cur_frame != NULL) {
		frame = cur_thread->cur_frame;
		cls = frame->cls;
		code_attr = frame->minfo->code_attr;
		table_len = (int) code_attr->excp_table_len;
		excp_table = code_attr->excp_table;

		for (i = 0; i < table_len; i ++) {
			// catch range
			if (excp_table[i].start_pc <= frame->pc < excp_table[i].end_pc) {
				// NULL 
				if(excp_table[i].catch_type == 0) {
					frame->pc = excp_table[i].handler_pc;
					return;
				}

				exp_cls_info = (CONSTANT_Class_info_t *)cls->const_pool[excp_table[i].catch_type];
				const char * exp_cls_name = hb_get_const_str(exp_cls_info->name_idx, cls);
				// MAX to-do check all super classes
				if(check_if_class_or_subclass(ecls, exp_cls_name)) {
					frame->pc = excp_table[i].handler_pc;
					return;
				}
			}
		}
		hb_pop_frame(cur_thread);
	}

	HB_ERR("No handler for exception %s!", obj->class->name);
    exit(EXIT_FAILURE);
}
