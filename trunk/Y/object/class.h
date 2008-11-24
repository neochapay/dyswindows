/************************************************************************
 *   Copyright (C) Mark Thomas <markbt@efaref.net>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#ifndef Y_OBJECT_CLASS_H
#define Y_OBJECT_CLASS_H

struct Class;

/*
 * This struct holds all the virtual functions associated 
 * with a given class object
 * all abstract classes should extend this struct by making
 * the super vtable the first element.
 */  
typedef struct _VTable
{
  //fill in with global abstract methods
} VTable;


#include <Y/y.h>
#include <Y/object/object.h>
#include <Y/message/tuple.h>

typedef struct Tuple *ClassMethod(struct Client *, const struct Tuple *);

void class_set_vtable (struct Class *, VTable *vtable);

void classInitialise (void);
void classFinalise (void);

struct Class *classCreate(const char *name, uint32_t superc, const char **superv);

void classAddInstanceMethod(struct Class *class, const char *name, InstanceMethod *method);
void classAddClassMethod(struct Class *class, const char *name, ClassMethod *method);

typedef void PropertyHook(struct Object *obj, const struct Value *old, const struct Value *new);

void classAddProperty(struct Class *class, const char *name, enum Type, PropertyHook *);

const struct Class *classGetPropertyClass (const struct Class *base, const char *name, enum Type *type);//added by DN

enum Type classGetProperty(const struct Class *class, const char *name);
void classCallPropertyHook(struct Object *o, const char *name, const struct Value *old, const struct Value *new);

void classCallClassPropertyHook (const struct Class *c, struct Object *o, 
				const char *name, const struct Value *old, 
				const struct Value *new); //add by DN

const char *classGetName      (const struct Class *);
int         classGetID        (const struct Class *);

struct Tuple *classInvokeClassMethod (const struct Class *, struct Client *,
                                      const char *method, const struct Tuple *);
struct Tuple *classInvokeInstanceMethod (struct Object *, struct Client *,
                                         const char *method, const struct Tuple *);

struct Class *classFindByName (const char *name);
struct Class *classFindByID   (int id);

bool classInherits (const struct Class *c, const struct Class *super);

#define DEFINE_CLASS(C)
#define CLASS(C) _Y__ ## C ## _class

typedef struct Tuple *instanceFunctionWrapper(struct Object *obj,
                                              struct Client *from,
                                              const struct Tuple *args,
                                              const struct MethodType *type);

typedef struct Tuple *classFunctionWrapper(struct Client *from,
                                           const struct Tuple *args,
                                           const struct MethodType *type);

#endif


