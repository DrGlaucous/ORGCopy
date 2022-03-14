#pragma once

#include <stddef.h>
#include <stdio.h>


#ifndef NULL
	#ifdef __cplusplus
		#define NULL 0
	#else
		#define NULL ((void *)0)
	#endif
#endif

#define MAXTRACK 16


typedef struct TRACKINFO
{
	size_t note_num;


} TRACKINFO; 

typedef struct ORGFILES
{
	TRACKINFO tracks[MAXTRACK];
}ORGFILES;


typedef struct NOTEDATA
{
	unsigned int x;
	unsigned char y;
	unsigned char length;
	unsigned char volume;
	unsigned char pan;
	unsigned char trackPrio;//what track did this note come from? (used for track priority)

}NOTEDATA;