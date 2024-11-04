#ifndef COMPILE_H
#define COMPILE_H

//
// optional defines
//

// #define DEBUG_MODE
// #define USE_BLUETOOTH

//
// lamp type detection
//

// LMBD_LAMP_TYPE__SIMPLE
#ifdef LMBD_LAMP_TYPE__SIMPLE

#warning Compiling SIMPLE lamp type

#ifdef LMBD_LAMP_TYPE__CCT
#error "Cannot define CCT if SIMPLE is defined"
#endif

#ifdef LMBD_LAMP_TYPE__INDEXABLE
#error "Cannot define INDEXABLE if SIMPLE is defined"
#endif

#endif  // LMBD_LAMP_TYPE__SIMPLE

// LMBD_LAMP_TYPE__CCT
#ifdef LMBD_LAMP_TYPE__CCT

#warning Compiling CCT lamp type

#ifdef LMBD_LAMP_TYPE__SIMPLE
#error "Cannot define SIMPLE if CCT is defined"
#endif

#ifdef LMBD_LAMP_TYPE__INDEXABLE
#error "Cannot define INDEXABLE if CCT is defined"
#endif

#endif  // LMBD_LAMP_TYPE__CCT

// LMBD_LAMP_TYPE__CCT
#ifdef LMBD_LAMP_TYPE__INDEXABLE

#warning Compiling INDEXABLE lamp type

#ifdef LMBD_LAMP_TYPE__SIMPLE
#error "Cannot define SIMPLE if INDEXABLE is defined"
#endif

#ifdef LMBD_LAMP_TYPE__CCT
#error "Cannot define CCT if INDEXABLE is defined"
#endif

#endif  // LMBD_LAMP_TYPE__CCT

#ifndef LMBD_LAMP_TYPE__SIMPLE
#ifndef LMBD_LAMP_TYPE__CCT
#ifndef LMBD_LAMP_TYPE__INDEXABLE
#error Must compile the program with a defined user mode
#endif
#endif
#endif

// Users triggering the error below, will either:
//  - have no idea what they are doing, and shell prefer using the Makefile
//  - be autonomous, and can add all the necessary build flags
//      (among which -I$(BUILD_DIR)/objs/sketch and other -D<define>)

#ifndef LMBD_MISSING_DEFINE
#error "LMBD_MISSING_DEFINE missing, are you using the Makefile to build?"
#endif

#endif
