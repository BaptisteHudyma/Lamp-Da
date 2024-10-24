#ifndef COMPILE_H
#define COMPILE_H

// LMBD_LAMP_TYPE__SIMPLE
#ifdef LMBD_LAMP_TYPE__SIMPLE

#ifdef LMBD_LAMP_TYPE__CCT
#error "Cannot define CCT if SIMPLE is defined"
#endif

#ifdef LMBD_LAMP_TYPE__INDEXABLE
#error "Cannot define INDEXABLE if SIMPLE is defined"
#endif

#endif  // LMBD_LAMP_TYPE__SIMPLE

// LMBD_LAMP_TYPE__CCT
#ifdef LMBD_LAMP_TYPE__CCT

#ifdef LMBD_LAMP_TYPE__SIMPLE
#error "Cannot define SIMPLE if CCT is defined"
#endif

#ifdef LMBD_LAMP_TYPE__INDEXABLE
#error "Cannot define INDEXABLE if CCT is defined"
#endif

#endif  // LMBD_LAMP_TYPE__CCT

// LMBD_LAMP_TYPE__CCT
#ifdef LMBD_LAMP_TYPE__INDEXABLE

#ifdef LMBD_LAMP_TYPE__SIMPLE
#error "Cannot define SIMPLE if INDEXABLE is defined"
#endif

#ifdef LMBD_LAMP_TYPE__CCT
#error "Cannot define CCT if INDEXABLE is defined"
#endif

#endif  // LMBD_LAMP_TYPE__CCT

#endif