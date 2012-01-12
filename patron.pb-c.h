/* Generated by the protocol buffer compiler.  DO NOT EDIT! */

#ifndef PROTOBUF_C_patron_2eproto__INCLUDED
#define PROTOBUF_C_patron_2eproto__INCLUDED

#include "proto/include/google/protobuf-c/protobuf-c.h"

PROTOBUF_C_BEGIN_DECLS


typedef struct _Patron Patron;
typedef struct _PatroList PatroList;


/* --- enums --- */


/* --- messages --- */

struct  _Patron
{
  ProtobufCMessage base;
  char *name;
  protobuf_c_boolean has_passangers;
  uint32_t passangers;
  char *pickup;
  char *dropoff;
  char *phone;
  char *status;
  char *notes;
  char *timetaken;
  char *timeassigned;
  char *timedone;
  protobuf_c_boolean has_pid;
  int32_t pid;
};
#define PATRON__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&patron__descriptor) \
    , NULL, 0,0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0,0 }


struct  _PatroList
{
  ProtobufCMessage base;
  size_t n_patron;
  Patron **patron;
};
#define PATRO_LIST__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&patro_list__descriptor) \
    , 0,NULL }


/* Patron methods */
void   patron__init
                     (Patron         *message);
size_t patron__get_packed_size
                     (const Patron   *message);
size_t patron__pack
                     (const Patron   *message,
                      uint8_t             *out);
size_t patron__pack_to_buffer
                     (const Patron   *message,
                      ProtobufCBuffer     *buffer);
Patron *
       patron__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   patron__free_unpacked
                     (Patron *message,
                      ProtobufCAllocator *allocator);
/* PatroList methods */
void   patro_list__init
                     (PatroList         *message);
size_t patro_list__get_packed_size
                     (const PatroList   *message);
size_t patro_list__pack
                     (const PatroList   *message,
                      uint8_t             *out);
size_t patro_list__pack_to_buffer
                     (const PatroList   *message,
                      ProtobufCBuffer     *buffer);
PatroList *
       patro_list__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   patro_list__free_unpacked
                     (PatroList *message,
                      ProtobufCAllocator *allocator);
/* --- per-message closures --- */

typedef void (*Patron_Closure)
                 (const Patron *message,
                  void *closure_data);
typedef void (*PatroList_Closure)
                 (const PatroList *message,
                  void *closure_data);

/* --- services --- */


/* --- descriptors --- */

extern const ProtobufCMessageDescriptor patron__descriptor;
extern const ProtobufCMessageDescriptor patro_list__descriptor;

PROTOBUF_C_END_DECLS


#endif  /* PROTOBUF_patron_2eproto__INCLUDED */