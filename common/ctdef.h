
/* Define to prevent recursive inclusion */
#ifndef __CTDEF_H
#define __CTDEF_H


/* Exported Types ---------------------------------------------------------- */

/* Common unsigned types */
#ifndef DEFINED_U8
#define DEFINED_U8
typedef unsigned char  U8;
#endif

#ifndef DEFINED_U16
#define DEFINED_U16
typedef unsigned short U16;
#endif

#ifndef DEFINED_U32
#define DEFINED_U32
typedef unsigned int   U32;
#endif

/* Common signed types */
#ifndef DEFINED_S8
#define DEFINED_S8
typedef signed char  S8;
#endif

#ifndef DEFINED_S16
#define DEFINED_S16
typedef signed short S16;
#endif

#ifndef DEFINED_S32
#define DEFINED_S32
typedef signed int   S32;
#endif

#ifndef DEFINED_BOOL
#define DEFINED_BOOL int;
#endif

#ifndef DEFINED_U64
#define DEFINED_U64
typedef struct U64_s
{
	unsigned int LSW;
	unsigned int MSW;
} U64;
#endif

#ifndef DEFINED_S64
#define DEFINED_S64
typedef U64 S64;
#endif
#if 0 
#ifndef DOUBLE
typedef double DOUBLE;
#endif

#ifndef FLOAT
typedef float FLOAT;
#endif

#ifndef IL32
typedef long IL32; 
#endif

#ifndef DU8
typedef volatile unsigned char DU8;
#endif

#ifndef DU16
typedef volatile unsigned short DU16;
#endif

#ifndef DU32
typedef volatile unsigned int DU32;
#endif

#endif
/* BOOL type constant values */
#ifndef TRUE
    #define TRUE (1 == 1)
#endif
#ifndef FALSE
    #define FALSE (!TRUE)
#endif

/* Maximun name length supported in cmw */
#define MAX_NAME_LENGTH  (32)

/* Maximun file path length suooprted in cmw */
#define MAX_PATH_LENGTH  (256)

/* Maximun length of IP address "xxx.xxx.xxx.xxx" */
#define MAX_IPADDR_LENGTH	(15)

/* NO Error */
#define CT_NO_ERROR (0)

/* General purpose string type */
typedef char* CT_String_t;

/* Function return error code */
typedef U32 CT_ErrorCode_t;

/* Revision structure */
typedef const char * CT_Revision_t;

/* Module ID */
typedef U16 CT_Module_t;

/* Device ID */
typedef U8 CT_Device_t;

#define CT_MAX_DEVICE_NAME 16  /* 15 characters plus '\0' */ /*Jerry add this line*/
typedef char CT_DeviceName_t[CT_MAX_DEVICE_NAME];   /*Jerry add this line*/

#endif
