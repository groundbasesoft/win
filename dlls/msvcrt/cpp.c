/*
 * msvcrt.dll C++ objects
 *
 * Copyright 2000 Jon Griffiths
 * Copyright 2003, 2004 Alexandre Julliard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>
#include <stdbool.h>

#include "windef.h"
#include "winternl.h"
#include "winbase.h"
#include "winver.h"
#include "imagehlp.h"
#include "wine/exception.h"
#include "wine/debug.h"
#include "msvcrt.h"
#include "mtdll.h"
#include "cxx.h"

WINE_DEFAULT_DEBUG_CHANNEL(msvcrt);

CREATE_TYPE_INFO_VTABLE
CREATE_EXCEPTION_OBJECT(exception)

struct __type_info_node
{
    void *memPtr;
    struct __type_info_node* next;
};

typedef exception bad_cast;
typedef exception bad_typeid;
typedef exception __non_rtti_object;

extern const vtable_ptr bad_typeid_vtable;
extern const vtable_ptr bad_cast_vtable;
extern const vtable_ptr __non_rtti_object_vtable;
extern const vtable_ptr type_info_vtable;

/* get the vtable pointer for a C++ object */
static inline const vtable_ptr *get_vtable( void *obj )
{
    return *(const vtable_ptr **)obj;
}

static inline const rtti_object_locator *get_obj_locator( void *cppobj )
{
    const vtable_ptr *vtable = get_vtable( cppobj );
    return (const rtti_object_locator *)vtable[-1];
}

#ifndef __x86_64__
static void dump_obj_locator( const rtti_object_locator *ptr )
{
    int i;
    const rtti_object_hierarchy *h = ptr->type_hierarchy;

    TRACE( "%p: sig=%08x base_offset=%08x flags=%08x type=%p %s hierarchy=%p\n",
           ptr, ptr->signature, ptr->base_class_offset, ptr->flags,
           ptr->type_descriptor, dbgstr_type_info(ptr->type_descriptor), ptr->type_hierarchy );
    TRACE( "  hierarchy: sig=%08x attr=%08x len=%d base classes=%p\n",
           h->signature, h->attributes, h->array_len, h->base_classes );
    for (i = 0; i < h->array_len; i++)
    {
        TRACE( "    base class %p: num %d off %d,%d,%d attr %08x type %p %s\n",
               h->base_classes->bases[i],
               h->base_classes->bases[i]->num_base_classes,
               h->base_classes->bases[i]->offsets.this_offset,
               h->base_classes->bases[i]->offsets.vbase_descr,
               h->base_classes->bases[i]->offsets.vbase_offset,
               h->base_classes->bases[i]->attributes,
               h->base_classes->bases[i]->type_descriptor,
               dbgstr_type_info(h->base_classes->bases[i]->type_descriptor) );
    }
}

#else

static void dump_obj_locator( const rtti_object_locator *ptr )
{
    int i;
    char *base = ptr->signature == 0 ? RtlPcToFileHeader((void*)ptr, (void**)&base) : (char*)ptr - ptr->object_locator;
    const rtti_object_hierarchy *h = (const rtti_object_hierarchy*)(base + ptr->type_hierarchy);
    const type_info *type_descriptor = (const type_info*)(base + ptr->type_descriptor);

    TRACE( "%p: sig=%08x base_offset=%08x flags=%08x type=%p %s hierarchy=%p\n",
            ptr, ptr->signature, ptr->base_class_offset, ptr->flags,
            type_descriptor, dbgstr_type_info(type_descriptor), h );
    TRACE( "  hierarchy: sig=%08x attr=%08x len=%d base classes=%p\n",
            h->signature, h->attributes, h->array_len, base + h->base_classes );
    for (i = 0; i < h->array_len; i++)
    {
        const rtti_base_descriptor *bases = (rtti_base_descriptor*)(base +
                ((const rtti_base_array*)(base + h->base_classes))->bases[i]);

        TRACE( "    base class %p: num %d off %d,%d,%d attr %08x type %p %s\n",
                bases,
                bases->num_base_classes,
                bases->offsets.this_offset,
                bases->offsets.vbase_descr,
                bases->offsets.vbase_offset,
                bases->attributes,
                base + bases->type_descriptor,
                dbgstr_type_info((const type_info*)(base + bases->type_descriptor)) );
    }
}
#endif

/******************************************************************
 *		??0exception@@QAE@ABQBD@Z (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(exception_ctor,8)
exception * __thiscall exception_ctor(exception * _this, const char ** name)
{
  TRACE("(%p,%s)\n", _this, *name);
  return __exception_ctor(_this, *name, &exception_vtable);
}

/******************************************************************
 *		??0exception@@QAE@ABQBDH@Z (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(exception_ctor_noalloc,12)
exception * __thiscall exception_ctor_noalloc(exception * _this, char ** name, int noalloc)
{
  TRACE("(%p,%s)\n", _this, *name);
  _this->vtable = &exception_vtable;
  _this->name = *name;
  _this->do_free = FALSE;
  return _this;
}

/******************************************************************
 *		??0exception@@QAE@XZ (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(exception_default_ctor,4)
exception * __thiscall exception_default_ctor(exception * _this)
{
  TRACE("(%p)\n", _this);
  return __exception_ctor(_this, NULL, &exception_vtable);
}

/******************************************************************
 *		??4exception@@QAEAAV0@ABV0@@Z (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(exception_opequals,8)
exception * __thiscall exception_opequals(exception * _this, const exception * rhs)
{
  TRACE("(%p %p)\n", _this, rhs);
  if (_this != rhs)
  {
      exception_dtor(_this);
      exception_copy_ctor(_this, rhs);
  }
  TRACE("name = %s\n", _this->name);
  return _this;
}

/******************************************************************
 *		??_Gexception@@UAEPAXI@Z (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(exception_scalar_dtor,8)
void * __thiscall exception_scalar_dtor(exception * _this, unsigned int flags)
{
    TRACE("(%p %x)\n", _this, flags);
    exception_dtor(_this);
    if (flags & 1) operator_delete(_this);
    return _this;
}

/******************************************************************
 *		??0bad_typeid@@QAE@ABV0@@Z (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(bad_typeid_copy_ctor,8)
bad_typeid * __thiscall bad_typeid_copy_ctor(bad_typeid * _this, const bad_typeid * rhs)
{
  TRACE("(%p %p)\n", _this, rhs);
  return __exception_copy_ctor(_this, rhs, &bad_typeid_vtable);
}

/******************************************************************
 *		??0bad_typeid@@QAE@PBD@Z (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(bad_typeid_ctor,8)
bad_typeid * __thiscall bad_typeid_ctor(bad_typeid * _this, const char * name)
{
  TRACE("(%p %s)\n", _this, name);
  return __exception_ctor(_this, name, &bad_typeid_vtable);
}

/******************************************************************
 *		??_Fbad_typeid@@QAEXXZ (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(bad_typeid_default_ctor,4)
bad_typeid * __thiscall bad_typeid_default_ctor(bad_typeid * _this)
{
  return bad_typeid_ctor( _this, "bad typeid" );
}

/******************************************************************
 *		??1bad_typeid@@UAE@XZ (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(bad_typeid_dtor,4)
void __thiscall bad_typeid_dtor(bad_typeid * _this)
{
  TRACE("(%p)\n", _this);
  exception_dtor(_this);
}

/******************************************************************
 *		??4bad_typeid@@QAEAAV0@ABV0@@Z (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(bad_typeid_opequals,8)
bad_typeid * __thiscall bad_typeid_opequals(bad_typeid * _this, const bad_typeid * rhs)
{
  TRACE("(%p %p)\n", _this, rhs);
  exception_opequals(_this, rhs);
  return _this;
}

/******************************************************************
 *              ??_Ebad_typeid@@UAEPAXI@Z (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(bad_typeid_vector_dtor,8)
void * __thiscall bad_typeid_vector_dtor(bad_typeid * _this, unsigned int flags)
{
    TRACE("(%p %x)\n", _this, flags);
    if (flags & 2)
    {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)_this - 1;

        for (i = *ptr - 1; i >= 0; i--) bad_typeid_dtor(_this + i);
        operator_delete(ptr);
    }
    else
    {
        bad_typeid_dtor(_this);
        if (flags & 1) operator_delete(_this);
    }
    return _this;
}

/******************************************************************
 *		??_Gbad_typeid@@UAEPAXI@Z (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(bad_typeid_scalar_dtor,8)
void * __thiscall bad_typeid_scalar_dtor(bad_typeid * _this, unsigned int flags)
{
    TRACE("(%p %x)\n", _this, flags);
    bad_typeid_dtor(_this);
    if (flags & 1) operator_delete(_this);
    return _this;
}

/******************************************************************
 *		??0__non_rtti_object@@QAE@ABV0@@Z (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(__non_rtti_object_copy_ctor,8)
__non_rtti_object * __thiscall __non_rtti_object_copy_ctor(__non_rtti_object * _this,
                                                                 const __non_rtti_object * rhs)
{
    TRACE("(%p %p)\n", _this, rhs);
    return __exception_copy_ctor(_this, rhs, &__non_rtti_object_vtable);
}

/******************************************************************
 *		??0__non_rtti_object@@QAE@PBD@Z (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(__non_rtti_object_ctor,8)
__non_rtti_object * __thiscall __non_rtti_object_ctor(__non_rtti_object * _this,
                                                            const char * name)
{
  TRACE("(%p %s)\n", _this, name);
  return __exception_ctor(_this, name, &__non_rtti_object_vtable);
}

/******************************************************************
 *		??1__non_rtti_object@@UAE@XZ (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(__non_rtti_object_dtor,4)
void __thiscall __non_rtti_object_dtor(__non_rtti_object * _this)
{
  TRACE("(%p)\n", _this);
  bad_typeid_dtor(_this);
}

/******************************************************************
 *		??4__non_rtti_object@@QAEAAV0@ABV0@@Z (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(__non_rtti_object_opequals,8)
__non_rtti_object * __thiscall __non_rtti_object_opequals(__non_rtti_object * _this,
                                                                const __non_rtti_object *rhs)
{
  TRACE("(%p %p)\n", _this, rhs);
  bad_typeid_opequals(_this, rhs);
  return _this;
}

/******************************************************************
 *		??_E__non_rtti_object@@UAEPAXI@Z (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(__non_rtti_object_vector_dtor,8)
void * __thiscall __non_rtti_object_vector_dtor(__non_rtti_object * _this, unsigned int flags)
{
    TRACE("(%p %x)\n", _this, flags);
    if (flags & 2)
    {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)_this - 1;

        for (i = *ptr - 1; i >= 0; i--) __non_rtti_object_dtor(_this + i);
        operator_delete(ptr);
    }
    else
    {
        __non_rtti_object_dtor(_this);
        if (flags & 1) operator_delete(_this);
    }
    return _this;
}

/******************************************************************
 *		??_G__non_rtti_object@@UAEPAXI@Z (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(__non_rtti_object_scalar_dtor,8)
void * __thiscall __non_rtti_object_scalar_dtor(__non_rtti_object * _this, unsigned int flags)
{
  TRACE("(%p %x)\n", _this, flags);
  __non_rtti_object_dtor(_this);
  if (flags & 1) operator_delete(_this);
  return _this;
}

/******************************************************************
 *		??0bad_cast@@AAE@PBQBD@Z (MSVCRT.@)
 *		??0bad_cast@@QAE@ABQBD@Z (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(bad_cast_ctor,8)
bad_cast * __thiscall bad_cast_ctor(bad_cast * _this, const char ** name)
{
  TRACE("(%p %s)\n", _this, *name);
  return __exception_ctor(_this, *name, &bad_cast_vtable);
}

/******************************************************************
 *		??0bad_cast@@QAE@ABV0@@Z (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(bad_cast_copy_ctor,8)
bad_cast * __thiscall bad_cast_copy_ctor(bad_cast * _this, const bad_cast * rhs)
{
  TRACE("(%p %p)\n", _this, rhs);
  return __exception_copy_ctor(_this, rhs, &bad_cast_vtable);
}

/******************************************************************
 *		??0bad_cast@@QAE@PBD@Z (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(bad_cast_ctor_charptr,8)
bad_cast * __thiscall bad_cast_ctor_charptr(bad_cast * _this, const char * name)
{
  TRACE("(%p %s)\n", _this, name);
  return __exception_ctor(_this, name, &bad_cast_vtable);
}

/******************************************************************
 *		??_Fbad_cast@@QAEXXZ (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(bad_cast_default_ctor,4)
bad_cast * __thiscall bad_cast_default_ctor(bad_cast * _this)
{
  return bad_cast_ctor_charptr( _this, "bad cast" );
}

/******************************************************************
 *		??1bad_cast@@UAE@XZ (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(bad_cast_dtor,4)
void __thiscall bad_cast_dtor(bad_cast * _this)
{
  TRACE("(%p)\n", _this);
  exception_dtor(_this);
}

/******************************************************************
 *		??4bad_cast@@QAEAAV0@ABV0@@Z (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(bad_cast_opequals,8)
bad_cast * __thiscall bad_cast_opequals(bad_cast * _this, const bad_cast * rhs)
{
  TRACE("(%p %p)\n", _this, rhs);
  exception_opequals(_this, rhs);
  return _this;
}

/******************************************************************
 *              ??_Ebad_cast@@UAEPAXI@Z (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(bad_cast_vector_dtor,8)
void * __thiscall bad_cast_vector_dtor(bad_cast * _this, unsigned int flags)
{
    TRACE("(%p %x)\n", _this, flags);
    if (flags & 2)
    {
        /* we have an array, with the number of elements stored before the first object */
        INT_PTR i, *ptr = (INT_PTR *)_this - 1;

        for (i = *ptr - 1; i >= 0; i--) bad_cast_dtor(_this + i);
        operator_delete(ptr);
    }
    else
    {
        bad_cast_dtor(_this);
        if (flags & 1) operator_delete(_this);
    }
    return _this;
}

/******************************************************************
 *		??_Gbad_cast@@UAEPAXI@Z (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(bad_cast_scalar_dtor,8)
void * __thiscall bad_cast_scalar_dtor(bad_cast * _this, unsigned int flags)
{
  TRACE("(%p %x)\n", _this, flags);
  bad_cast_dtor(_this);
  if (flags & 1) operator_delete(_this);
  return _this;
}

/******************************************************************
 *		??8type_info@@QBEHABV0@@Z (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(type_info_opequals_equals,8)
int __thiscall type_info_opequals_equals(type_info * _this, const type_info * rhs)
{
    int ret = !strcmp(_this->mangled + 1, rhs->mangled + 1);
    TRACE("(%p %p) returning %d\n", _this, rhs, ret);
    return ret;
}

/******************************************************************
 *		??9type_info@@QBEHABV0@@Z (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(type_info_opnot_equals,8)
int __thiscall type_info_opnot_equals(type_info * _this, const type_info * rhs)
{
    int ret = !!strcmp(_this->mangled + 1, rhs->mangled + 1);
    TRACE("(%p %p) returning %d\n", _this, rhs, ret);
    return ret;
}

/******************************************************************
 *		?before@type_info@@QBEHABV1@@Z (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(type_info_before,8)
int __thiscall type_info_before(type_info * _this, const type_info * rhs)
{
    int ret = strcmp(_this->mangled + 1, rhs->mangled + 1) < 0;
    TRACE("(%p %p) returning %d\n", _this, rhs, ret);
    return ret;
}

/******************************************************************
 *		??1type_info@@UAE@XZ (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(type_info_dtor,4)
void __thiscall type_info_dtor(type_info * _this)
{
  TRACE("(%p)\n", _this);
  free(_this->name);
}

/******************************************************************
 *		?name@type_info@@QBEPBDXZ (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(type_info_name,4)
const char * __thiscall type_info_name(type_info * _this)
{
  if (!_this->name)
  {
    /* Create and set the demangled name */
    /* Note: mangled name in type_info struct always starts with a '.', while
     * it isn't valid for mangled name.
     * Is this '.' really part of the mangled name, or has it some other meaning ?
     */
    char* name = __unDName(0, _this->mangled + 1, 0,
                           malloc, free, UNDNAME_NO_ARGUMENTS | UNDNAME_32_BIT_DECODE);
    if (name)
    {
      unsigned int len = strlen(name);

      /* It seems _unDName may leave blanks at the end of the demangled name */
      while (len && name[--len] == ' ')
        name[len] = '\0';

      if (InterlockedCompareExchangePointer((void**)&_this->name, name, NULL))
      {
        /* Another thread set this member since we checked above - use it */
        free(name);
      }
    }
  }
  TRACE("(%p) returning %s\n", _this, _this->name);
  return _this->name;
}

/******************************************************************
 *		?raw_name@type_info@@QBEPBDXZ (MSVCRT.@)
 */
DEFINE_THISCALL_WRAPPER(type_info_raw_name,4)
const char * __thiscall type_info_raw_name(type_info * _this)
{
  TRACE("(%p) returning %s\n", _this, _this->mangled);
  return _this->mangled;
}

#if _MSVCR_VER >= 80

typedef exception bad_alloc;
extern const vtable_ptr bad_alloc_vtable;

/* bad_alloc class implementation */
DEFINE_THISCALL_WRAPPER(bad_alloc_copy_ctor,8)
bad_alloc * __thiscall bad_alloc_copy_ctor(bad_alloc * _this, const bad_alloc * rhs)
{
    TRACE("(%p %p)\n", _this, rhs);
    return __exception_copy_ctor(_this, rhs, &bad_alloc_vtable);
}

DEFINE_THISCALL_WRAPPER(bad_alloc_dtor,4)
void __thiscall bad_alloc_dtor(bad_alloc * _this)
{
    TRACE("(%p)\n", _this);
    exception_dtor(_this);
}

#endif /* _MSVCR_VER >= 80 */

__ASM_BLOCK_BEGIN(vtables)

#if _MSVCR_VER >= 80
__ASM_VTABLE(exception_old,
        VTABLE_ADD_FUNC(exception_vector_dtor)
        VTABLE_ADD_FUNC(exception_what));
__ASM_VTABLE(bad_alloc,
        VTABLE_ADD_FUNC(exception_vector_dtor)
        VTABLE_ADD_FUNC(exception_what));
#endif
__ASM_VTABLE(bad_typeid,
        VTABLE_ADD_FUNC(bad_typeid_vector_dtor)
        VTABLE_ADD_FUNC(exception_what));
__ASM_VTABLE(bad_cast,
        VTABLE_ADD_FUNC(bad_cast_vector_dtor)
        VTABLE_ADD_FUNC(exception_what));
__ASM_VTABLE(__non_rtti_object,
        VTABLE_ADD_FUNC(__non_rtti_object_vector_dtor)
        VTABLE_ADD_FUNC(exception_what));

__ASM_BLOCK_END

#if _MSVCR_VER >= 80
DEFINE_RTTI_DATA0( exception_old, 0, ".?AVexception@@" )
DEFINE_RTTI_DATA1( bad_typeid, 0, &exception_rtti_base_descriptor, ".?AVbad_typeid@std@@" )
DEFINE_RTTI_DATA1( bad_cast, 0, &exception_rtti_base_descriptor, ".?AVbad_cast@std@@" )
DEFINE_RTTI_DATA2( __non_rtti_object, 0, &bad_typeid_rtti_base_descriptor, &exception_rtti_base_descriptor, ".?AV__non_rtti_object@std@@" )
DEFINE_RTTI_DATA1( bad_alloc, 0, &exception_rtti_base_descriptor, ".?AVbad_alloc@std@@" )
#else
DEFINE_RTTI_DATA1( bad_typeid, 0, &exception_rtti_base_descriptor, ".?AVbad_typeid@@" )
DEFINE_RTTI_DATA1( bad_cast, 0, &exception_rtti_base_descriptor, ".?AVbad_cast@@" )
DEFINE_RTTI_DATA2( __non_rtti_object, 0, &bad_typeid_rtti_base_descriptor, &exception_rtti_base_descriptor, ".?AV__non_rtti_object@@" )
#endif

#if _MSVCR_VER >= 100
DEFINE_CXX_EXCEPTION0( exception, exception_dtor )
#endif
DEFINE_CXX_DATA1( bad_typeid, &exception_cxx_type_info, bad_typeid_dtor )
DEFINE_CXX_DATA1( bad_cast, &exception_cxx_type_info, bad_cast_dtor )
DEFINE_CXX_DATA2( __non_rtti_object, &bad_typeid_cxx_type_info,
        &exception_cxx_type_info, __non_rtti_object_dtor )
#if _MSVCR_VER >= 80
DEFINE_CXX_DATA1( bad_alloc, &exception_cxx_type_info, bad_alloc_dtor )
#endif

void msvcrt_init_exception(void *base)
{
#ifdef __x86_64__
    init_type_info_rtti(base);
    init_exception_rtti(base);
#if _MSVCR_VER >= 80
    init_exception_old_rtti(base);
    init_bad_alloc_rtti(base);
#endif
    init_bad_typeid_rtti(base);
    init_bad_cast_rtti(base);
    init___non_rtti_object_rtti(base);

#if _MSVCR_VER >= 100
    init_exception_cxx(base);
#else
    init_exception_cxx_type_info(base);
#endif
    init_bad_typeid_cxx(base);
    init_bad_cast_cxx(base);
    init___non_rtti_object_cxx(base);
#if _MSVCR_VER >= 80
    init_bad_alloc_cxx(base);
#endif
#endif
}

#if _MSVCR_VER >= 80
void throw_bad_alloc(void)
{
    bad_alloc e;
    __exception_ctor(&e, "bad allocation", &bad_alloc_vtable);
    _CxxThrowException(&e, &bad_alloc_exception_type);
}
#endif

#if _MSVCR_VER >= 100
void throw_exception(const char* msg)
{
    exception e;
    __exception_ctor(&e, msg, &exception_vtable);
    _CxxThrowException(&e, &exception_exception_type);
}
#endif

/******************************************************************
 *		?set_terminate@@YAP6AXXZP6AXXZ@Z (MSVCRT.@)
 *
 * Install a handler to be called when terminate() is called.
 *
 * PARAMS
 *  func [I] Handler function to install
 *
 * RETURNS
 *  The previously installed handler function, if any.
 */
terminate_function CDECL set_terminate(terminate_function func)
{
    thread_data_t *data = msvcrt_get_thread_data();
    terminate_function previous = data->terminate_handler;
    TRACE("(%p) returning %p\n",func,previous);
    data->terminate_handler = func;
    return previous;
}

/******************************************************************
 *              _get_terminate (MSVCRT.@)
 */
terminate_function CDECL _get_terminate(void)
{
    thread_data_t *data = msvcrt_get_thread_data();
    TRACE("returning %p\n", data->terminate_handler);
    return data->terminate_handler;
}

/******************************************************************
 *		?set_unexpected@@YAP6AXXZP6AXXZ@Z (MSVCRT.@)
 *
 * Install a handler to be called when unexpected() is called.
 *
 * PARAMS
 *  func [I] Handler function to install
 *
 * RETURNS
 *  The previously installed handler function, if any.
 */
unexpected_function CDECL set_unexpected(unexpected_function func)
{
    thread_data_t *data = msvcrt_get_thread_data();
    unexpected_function previous = data->unexpected_handler;
    TRACE("(%p) returning %p\n",func,previous);
    data->unexpected_handler = func;
    return previous;
}

/******************************************************************
 *              _get_unexpected (MSVCRT.@)
 */
unexpected_function CDECL _get_unexpected(void)
{
    thread_data_t *data = msvcrt_get_thread_data();
    TRACE("returning %p\n", data->unexpected_handler);
    return data->unexpected_handler;
}

/******************************************************************
 *              ?_set_se_translator@@YAP6AXIPAU_EXCEPTION_POINTERS@@@ZP6AXI0@Z@Z  (MSVCRT.@)
 */
_se_translator_function CDECL _set_se_translator(_se_translator_function func)
{
    thread_data_t *data = msvcrt_get_thread_data();
    _se_translator_function previous = data->se_translator;
    TRACE("(%p) returning %p\n",func,previous);
    data->se_translator = func;
    return previous;
}

/******************************************************************
 *		?terminate@@YAXXZ (MSVCRT.@)
 *
 * Default handler for an unhandled exception.
 *
 * PARAMS
 *  None.
 *
 * RETURNS
 *  This function does not return. Either control resumes from any
 *  handler installed by calling set_terminate(), or (by default) abort()
 *  is called.
 */
void CDECL terminate(void)
{
    thread_data_t *data = msvcrt_get_thread_data();
    if (data->terminate_handler) data->terminate_handler();
    abort();
}

/******************************************************************
 *		?unexpected@@YAXXZ (MSVCRT.@)
 */
void CDECL unexpected(void)
{
    thread_data_t *data = msvcrt_get_thread_data();
    if (data->unexpected_handler) data->unexpected_handler();
    terminate();
}


/******************************************************************
 *		__RTtypeid (MSVCRT.@)
 *
 * Retrieve the Run Time Type Information (RTTI) for a C++ object.
 *
 * PARAMS
 *  cppobj [I] C++ object to get type information for.
 *
 * RETURNS
 *  Success: A type_info object describing cppobj.
 *  Failure: If the object to be cast has no RTTI, a __non_rtti_object
 *           exception is thrown. If cppobj is NULL, a bad_typeid exception
 *           is thrown. In either case, this function does not return.
 *
 * NOTES
 *  This function is usually called by compiler generated code as a result
 *  of using one of the C++ dynamic cast statements.
 */
#ifndef __x86_64__
const type_info* CDECL __RTtypeid(void *cppobj)
{
    const type_info *ret;

    if (!cppobj)
    {
        bad_typeid e;
        bad_typeid_ctor( &e, "Attempted a typeid of NULL pointer!" );
        _CxxThrowException( &e, &bad_typeid_exception_type );
    }

    __TRY
    {
        const rtti_object_locator *obj_locator = get_obj_locator( cppobj );
        ret = obj_locator->type_descriptor;
    }
    __EXCEPT_PAGE_FAULT
    {
        __non_rtti_object e;
        __non_rtti_object_ctor( &e, "Bad read pointer - no RTTI data!" );
        _CxxThrowException( &e, &__non_rtti_object_exception_type );
    }
    __ENDTRY
    return ret;
}

#else

const type_info* CDECL __RTtypeid(void *cppobj)
{
    const type_info *ret;

    if (!cppobj)
    {
        bad_typeid e;
        bad_typeid_ctor( &e, "Attempted a typeid of NULL pointer!" );
        _CxxThrowException( &e, &bad_typeid_exception_type );
    }

    __TRY
    {
        const rtti_object_locator *obj_locator = get_obj_locator( cppobj );
        char *base;

        if(obj_locator->signature == 0)
            base = RtlPcToFileHeader((void*)obj_locator, (void**)&base);
        else
            base = (char*)obj_locator - obj_locator->object_locator;

        ret = (type_info*)(base + obj_locator->type_descriptor);
    }
    __EXCEPT_PAGE_FAULT
    {
        __non_rtti_object e;
        __non_rtti_object_ctor( &e, "Bad read pointer - no RTTI data!" );
        _CxxThrowException( &e, &__non_rtti_object_exception_type );
    }
    __ENDTRY
    return ret;
}
#endif

/******************************************************************
 *		__RTDynamicCast (MSVCRT.@)
 *
 * Dynamically cast a C++ object to one of its base classes.
 *
 * PARAMS
 *  cppobj   [I] Any C++ object to cast
 *  unknown  [I] Reserved, set to 0
 *  src      [I] type_info object describing cppobj
 *  dst      [I] type_info object describing the base class to cast to
 *  do_throw [I] TRUE = throw an exception if the cast fails, FALSE = don't
 *
 * RETURNS
 *  Success: The address of cppobj, cast to the object described by dst.
 *  Failure: NULL, If the object to be cast has no RTTI, or dst is not a
 *           valid cast for cppobj. If do_throw is TRUE, a bad_cast exception
 *           is thrown and this function does not return.
 *
 * NOTES
 *  This function is usually called by compiler generated code as a result
 *  of using one of the C++ dynamic cast statements.
 */
#ifndef __x86_64__
void* CDECL __RTDynamicCast(void *cppobj, int unknown,
                                   type_info *src, type_info *dst,
                                   int do_throw)
{
    void *ret;

    if (!cppobj) return NULL;

    TRACE("obj: %p unknown: %d src: %p %s dst: %p %s do_throw: %d)\n",
          cppobj, unknown, src, dbgstr_type_info(src), dst, dbgstr_type_info(dst), do_throw);

    /* To cast an object at runtime:
     * 1.Find out the true type of the object from the typeinfo at vtable[-1]
     * 2.Search for the destination type in the class hierarchy
     * 3.If destination type is found, return base object address + dest offset
     *   Otherwise, fail the cast
     *
     * FIXME: the unknown parameter doesn't seem to be used for anything
     */
    __TRY
    {
        int i;
        const rtti_object_locator *obj_locator = get_obj_locator( cppobj );
        const rtti_object_hierarchy *obj_bases = obj_locator->type_hierarchy;
        const rtti_base_descriptor * const* base_desc = obj_bases->base_classes->bases;

        if (TRACE_ON(msvcrt)) dump_obj_locator(obj_locator);

        ret = NULL;
        for (i = 0; i < obj_bases->array_len; i++)
        {
            const type_info *typ = base_desc[i]->type_descriptor;

            if (!strcmp(typ->mangled, dst->mangled))
            {
                /* compute the correct this pointer for that base class */
                void *this_ptr = (char *)cppobj - obj_locator->base_class_offset;
                ret = get_this_pointer( &base_desc[i]->offsets, this_ptr );
                break;
            }
        }
        /* VC++ sets do_throw to 1 when the result of a dynamic_cast is assigned
         * to a reference, since references cannot be NULL.
         */
        if (!ret && do_throw)
        {
            const char *msg = "Bad dynamic_cast!";
            bad_cast e;
            bad_cast_ctor( &e, &msg );
            _CxxThrowException( &e, &bad_cast_exception_type );
        }
    }
    __EXCEPT_PAGE_FAULT
    {
        __non_rtti_object e;
        __non_rtti_object_ctor( &e, "Access violation - no RTTI data!" );
        _CxxThrowException( &e, &__non_rtti_object_exception_type );
    }
    __ENDTRY
    return ret;
}

#else

void* CDECL __RTDynamicCast(void *cppobj, int unknown,
        type_info *src, type_info *dst,
        int do_throw)
{
    void *ret;

    if (!cppobj) return NULL;

    TRACE("obj: %p unknown: %d src: %p %s dst: %p %s do_throw: %d)\n",
            cppobj, unknown, src, dbgstr_type_info(src), dst, dbgstr_type_info(dst), do_throw);

    __TRY
    {
        int i;
        const rtti_object_locator *obj_locator = get_obj_locator( cppobj );
        const rtti_object_hierarchy *obj_bases;
        const rtti_base_array *base_array;
        char *base;

        if (TRACE_ON(msvcrt)) dump_obj_locator(obj_locator);

        if(obj_locator->signature == 0)
            base = RtlPcToFileHeader((void*)obj_locator, (void**)&base);
        else
            base = (char*)obj_locator - obj_locator->object_locator;

        obj_bases = (const rtti_object_hierarchy*)(base + obj_locator->type_hierarchy);
        base_array = (const rtti_base_array*)(base + obj_bases->base_classes);

        ret = NULL;
        for (i = 0; i < obj_bases->array_len; i++)
        {
            const rtti_base_descriptor *base_desc = (const rtti_base_descriptor*)(base + base_array->bases[i]);
            const type_info *typ = (const type_info*)(base + base_desc->type_descriptor);

            if (!strcmp(typ->mangled, dst->mangled))
            {
                void *this_ptr = (char *)cppobj - obj_locator->base_class_offset;
                ret = get_this_pointer( &base_desc->offsets, this_ptr );
                break;
            }
        }
        if (!ret && do_throw)
        {
            const char *msg = "Bad dynamic_cast!";
            bad_cast e;
            bad_cast_ctor( &e, &msg );
            _CxxThrowException( &e, &bad_cast_exception_type );
        }
    }
    __EXCEPT_PAGE_FAULT
    {
        __non_rtti_object e;
        __non_rtti_object_ctor( &e, "Access violation - no RTTI data!" );
        _CxxThrowException( &e, &__non_rtti_object_exception_type );
    }
    __ENDTRY
    return ret;
}
#endif


/******************************************************************
 *		__RTCastToVoid (MSVCRT.@)
 *
 * Dynamically cast a C++ object to a void*.
 *
 * PARAMS
 *  cppobj [I] The C++ object to cast
 *
 * RETURNS
 *  Success: The base address of the object as a void*.
 *  Failure: NULL, if cppobj is NULL or has no RTTI.
 *
 * NOTES
 *  This function is usually called by compiler generated code as a result
 *  of using one of the C++ dynamic cast statements.
 */
void* CDECL __RTCastToVoid(void *cppobj)
{
    void *ret;

    if (!cppobj) return NULL;

    __TRY
    {
        const rtti_object_locator *obj_locator = get_obj_locator( cppobj );
        ret = (char *)cppobj - obj_locator->base_class_offset;
    }
    __EXCEPT_PAGE_FAULT
    {
        __non_rtti_object e;
        __non_rtti_object_ctor( &e, "Access violation - no RTTI data!" );
        _CxxThrowException( &e, &__non_rtti_object_exception_type );
    }
    __ENDTRY
    return ret;
}


/*********************************************************************
 *		_CxxThrowException (MSVCRT.@)
 */
#ifndef __x86_64__
void WINAPI _CxxThrowException( void *object, const cxx_exception_type *type )
{
    ULONG_PTR args[3];

    args[0] = CXX_FRAME_MAGIC_VC6;
    args[1] = (ULONG_PTR)object;
    args[2] = (ULONG_PTR)type;
    RaiseException( CXX_EXCEPTION, EXCEPTION_NONCONTINUABLE, 3, args );
}
#else
void WINAPI _CxxThrowException( void *object, const cxx_exception_type *type )
{
    ULONG_PTR args[4];

    args[0] = CXX_FRAME_MAGIC_VC6;
    args[1] = (ULONG_PTR)object;
    args[2] = (ULONG_PTR)type;
    RtlPcToFileHeader( (void*)type, (void**)&args[3]);
    RaiseException( CXX_EXCEPTION, EXCEPTION_NONCONTINUABLE, 4, args );
}
#endif

#if _MSVCR_VER >= 80

/*********************************************************************
 * ?_is_exception_typeof@@YAHABVtype_info@@PAU_EXCEPTION_POINTERS@@@Z
 * ?_is_exception_typeof@@YAHAEBVtype_info@@PEAU_EXCEPTION_POINTERS@@@Z
 */
#ifndef __x86_64__
int __cdecl _is_exception_typeof(const type_info *ti, EXCEPTION_POINTERS *ep)
{
    int ret = -1;

    TRACE("(%p %p)\n", ti, ep);

    __TRY
    {
        EXCEPTION_RECORD *rec = ep->ExceptionRecord;

        if (rec->ExceptionCode==CXX_EXCEPTION && rec->NumberParameters==3 &&
                (rec->ExceptionInformation[0]==CXX_FRAME_MAGIC_VC6 ||
                 rec->ExceptionInformation[0]==CXX_FRAME_MAGIC_VC7 ||
                 rec->ExceptionInformation[0]==CXX_FRAME_MAGIC_VC8))
        {
            const cxx_type_info_table *tit = ((cxx_exception_type*)rec->ExceptionInformation[2])->type_info_table;
            int i;

            for (i=0; i<tit->count; i++) {
                if (ti==tit->info[i]->type_info || !strcmp(ti->mangled, tit->info[i]->type_info->mangled))
                {
                    ret = 1;
                    break;
                }
            }

            if (i == tit->count)
                ret = 0;
        }
    }
    __EXCEPT_PAGE_FAULT
    __ENDTRY

    if(ret == -1)
        terminate();
    return ret;
}
#else
int __cdecl _is_exception_typeof(const type_info *ti, EXCEPTION_POINTERS *ep)
{
    int ret = -1;

    TRACE("(%p %p)\n", ti, ep);

    __TRY
    {
        EXCEPTION_RECORD *rec = ep->ExceptionRecord;

        if (rec->ExceptionCode==CXX_EXCEPTION && rec->NumberParameters==4 &&
                (rec->ExceptionInformation[0]==CXX_FRAME_MAGIC_VC6 ||
                 rec->ExceptionInformation[0]==CXX_FRAME_MAGIC_VC7 ||
                 rec->ExceptionInformation[0]==CXX_FRAME_MAGIC_VC8))
        {
            const cxx_exception_type *et = (cxx_exception_type*)rec->ExceptionInformation[2];
            const cxx_type_info_table *tit = (const cxx_type_info_table*)(rec->ExceptionInformation[3]+et->type_info_table);
            int i;

            for (i=0; i<tit->count; i++) {
                const cxx_type_info *cti = (const cxx_type_info*)(rec->ExceptionInformation[3]+tit->info[i]);
                const type_info *except_ti = (const type_info*)(rec->ExceptionInformation[3]+cti->type_info);
                if (ti==except_ti || !strcmp(ti->mangled, except_ti->mangled))
                {
                    ret = 1;
                    break;
                }
            }

            if (i == tit->count)
                ret = 0;
        }
    }
    __EXCEPT_PAGE_FAULT
    __ENDTRY

    if(ret == -1)
        terminate();
    return ret;
}
#endif

/*********************************************************************
 * __clean_type_info_names_internal (MSVCR80.@)
 */
void CDECL __clean_type_info_names_internal(void *p)
{
    FIXME("(%p) stub\n", p);
}

/*********************************************************************
 * ?_name_internal_method@type_info@@QBEPBDPAU__type_info_node@@@Z (MSVCR100.@)
 */
DEFINE_THISCALL_WRAPPER(type_info_name_internal_method,8)
const char * __thiscall type_info_name_internal_method(type_info * _this, struct __type_info_node *node)
{
    static int once;
    if (node && !once++) FIXME("type_info_node parameter ignored\n");

    return type_info_name(_this);
}

void* __cdecl __AdjustPointer(void *obj, const this_ptr_offsets *off)
{
    return get_this_pointer(off, obj);
}

#endif

#if _MSVCR_VER >= 140

typedef struct
{
    char *name;
    char mangled[1];
} type_info140;

typedef struct
{
    SLIST_ENTRY entry;
    char name[1];
} type_info_entry;

static void* CDECL type_info_entry_malloc(size_t size)
{
    type_info_entry *ret = malloc(FIELD_OFFSET(type_info_entry, name) + size);
    return ret->name;
}

static void CDECL type_info_entry_free(void *ptr)
{
    ptr = (char*)ptr - FIELD_OFFSET(type_info_entry, name);
    free(ptr);
}

/******************************************************************
 *		__std_type_info_compare (UCRTBASE.@)
 */
int CDECL __std_type_info_compare(const type_info140 *l, const type_info140 *r)
{
    int ret;

    if (l == r) ret = 0;
    else ret = strcmp(l->mangled + 1, r->mangled + 1);
    TRACE("(%p %p) returning %d\n", l, r, ret);
    return ret;
}

/******************************************************************
 *		__std_type_info_name (UCRTBASE.@)
 */
const char* CDECL __std_type_info_name(type_info140 *ti, SLIST_HEADER *header)
{
      if (!ti->name)
      {
          char* name = __unDName(0, ti->mangled + 1, 0,
                  type_info_entry_malloc, type_info_entry_free, UNDNAME_NO_ARGUMENTS | UNDNAME_32_BIT_DECODE);
          if (name)
          {
              unsigned int len = strlen(name);

              while (len && name[--len] == ' ')
                  name[len] = '\0';

              if (InterlockedCompareExchangePointer((void**)&ti->name, name, NULL))
              {
                  type_info_entry_free(name);
              }
              else
              {
                  type_info_entry *entry = (type_info_entry*)(name-FIELD_OFFSET(type_info_entry, name));
                  InterlockedPushEntrySList(header, &entry->entry);
              }
          }
      }
      TRACE("(%p) returning %s\n", ti, ti->name);
      return ti->name;
}

/******************************************************************
 *		__std_type_info_destroy_list  (UCRTBASE.@)
 */
void CDECL __std_type_info_destroy_list(SLIST_HEADER *header)
{
    SLIST_ENTRY *cur, *next;

    TRACE("(%p)\n", header);

    for(cur = InterlockedFlushSList(header); cur; cur = next)
    {
        next = cur->Next;
        free(cur);
    }
}

/******************************************************************
 *              __std_type_info_hash (UCRTBASE.@)
 */
size_t CDECL __std_type_info_hash(const type_info140 *ti)
{
    size_t hash, fnv_prime;
    const char *p;

#ifdef _WIN64
    hash = 0xcbf29ce484222325;
    fnv_prime = 0x100000001b3;
#else
    hash = 0x811c9dc5;
    fnv_prime = 0x1000193;
#endif

    TRACE("(%p)->%s\n", ti, ti->mangled);

    for(p = ti->mangled+1; *p; p++) {
        hash ^= *p;
        hash *= fnv_prime;
    }

#ifdef _WIN64
    hash ^= hash >> 32;
#endif

    return hash;
}

#endif /* _MSVCR_VER >= 140 */
