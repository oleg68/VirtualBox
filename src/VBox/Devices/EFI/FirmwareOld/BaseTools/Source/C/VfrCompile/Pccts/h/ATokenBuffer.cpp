/* ANTLRTokenBuffer.cpp
 *
 * SOFTWARE RIGHTS
 *
 * We reserve no LEGAL rights to the Purdue Compiler Construction Tool
 * Set (PCCTS) -- PCCTS is in the public domain.  An individual or
 * company may do whatever they wish with source code distributed with
 * PCCTS or the code generated by PCCTS, including the incorporation of
 * PCCTS, or its output, into commerical software.
 *
 * We encourage users to develop software with PCCTS.  However, we do ask
 * that credit is given to us for developing PCCTS.  By "credit",
 * we mean that if you incorporate our source code into one of your
 * programs (commercial product, research project, or otherwise) that you
 * acknowledge this fact somewhere in the documentation, research report,
 * etc...  If you like PCCTS and have developed a nice tool with the
 * output, please mention that you developed it using PCCTS.  In
 * addition, we ask that this header remain intact in our source code.
 * As long as these guidelines are kept, we expect to continue enhancing
 * this system and expect to make other tools available as they are
 * completed.
 *
 * ANTLR 1.33
 * Terence Parr
 * Parr Research Corporation
 * with Purdue University and AHPCRC, University of Minnesota
 * 1989-2000
 */

typedef int ANTLRTokenType;	// fool AToken.h into compiling

class ANTLRParser;					/* MR1 */

#define ANTLR_SUPPORT_CODE

#include "pcctscfg.h"

#include ATOKENBUFFER_H
#include APARSER_H		// MR23

typedef ANTLRAbstractToken *_ANTLRTokenPtr;

#if defined(DBG_TBUF)||defined(DBG_TBUF_MARK_REW)
static unsigned char test[1000];
#endif

#ifdef DBG_REFCOUNTTOKEN
int ANTLRRefCountToken::ctor = 0; /* MR23 */
int ANTLRRefCountToken::dtor = 0; /* MR23 */
#endif

ANTLRTokenBuffer::
ANTLRTokenBuffer(ANTLRTokenStream *_input, int _k, int _chunk_size_formal) /* MR14 */
{
	this->input = _input;
	this->k = _k;
	buffer_size = chunk_size = _chunk_size_formal;
	buffer = (_ANTLRTokenPtr *)
			 calloc(chunk_size+1,sizeof(_ANTLRTokenPtr ));
	if ( buffer == NULL ) {
		panic("cannot alloc token buffer");
	}
	buffer++;				// leave the first elem empty so tp-1 is valid ptr

	tp = &buffer[0];
	last = tp-1;
	next = &buffer[0];
	num_markers = 0;
	end_of_buffer = &buffer[buffer_size-1];
	threshold = &buffer[(int)(buffer_size/2)];	// MR23 - Used to be 1.0/2.0 !
	_deleteTokens = 1; 	// assume we delete tokens
	parser=NULL;				// MR5 - uninitialized reference
}

static void f() {;}
ANTLRTokenBuffer::
~ANTLRTokenBuffer()
{
	f();
	// Delete all remaining tokens (from 0..last inclusive)
	if ( _deleteTokens )
	{
		_ANTLRTokenPtr *z;
		for (z=buffer; z<=last; z++)
		{
			(*z)->deref();
//			z->deref();
#ifdef DBG_REFCOUNTTOKEN
					/* MR23 */ printMessage(stderr, "##########dtor: deleting token '%s' (ref %d)\n",
							((ANTLRCommonToken *)*z)->getText(), (*z)->nref());
#endif
			if ( (*z)->nref()==0 )
			{
				delete (*z);
			}
		}
	}

	if ( buffer!=NULL ) free((char *)(buffer-1));
}

#if defined(DBG_TBUF)||defined(DBG_TBUF_MARK_REW)
#include "pccts_stdio.h"
PCCTS_NAMESPACE_STD
#endif

_ANTLRTokenPtr ANTLRTokenBuffer::
getToken()
{
	if ( tp <= last )	// is there any buffered lookahead still to be read?
	{
		return *tp++;	// read buffered lookahead
	}
	// out of buffered lookahead, get some more "real"
	// input from getANTLRToken()
	if ( num_markers==0 )
	{
		if( next > threshold )
		{
#ifdef DBG_TBUF
/* MR23 */ printMessage(stderr,"getToken: next > threshold (high water is %d)\n", threshold-buffer);
#endif
			makeRoom();
		}
	}
	else {
		if ( next > end_of_buffer )
		{
#ifdef DBG_TBUF
/* MR23 */ printMessage(stderr,"getToken: next > end_of_buffer (size is %d)\n", buffer_size);
#endif
			extendBuffer();
		}
	}
	*next = getANTLRToken();
	(*next)->ref();				// say we have a copy of this pointer in buffer
	last = next;
	next++;
	tp = last;
	return *tp++;
}

void ANTLRTokenBuffer::
rewind(int pos)
{
#if defined(DBG_TBUF)||defined(DBG_TBUF_MARK_REW)
	/* MR23 */ printMessage(stderr, "rewind(%d)[nm=%d,from=%d,%d.n=%d]\n", pos, num_markers, tp-buffer,pos,test[pos]);
	test[pos]--;
#endif
	tp = &buffer[pos];
	num_markers--;
}

/*
 * This function is used to specify that the token pointers read
 * by the ANTLRTokenBuffer should be buffered up (to be reused later).
 */
int ANTLRTokenBuffer::
mark()
{
#if defined(DBG_TBUF)||defined(DBG_TBUF_MARK_REW)
	test[tp-buffer]++;
	/* MR23 */ printMessage(stderr,"mark(%d)[nm=%d,%d.n=%d]\n",tp-buffer,num_markers+1,tp-buffer,test[tp-buffer]);
#endif
	num_markers++;
	return tp - buffer;
}

/*
 * returns the token pointer n positions ahead.
 * This implies that bufferedToken(1) gets the NEXT symbol of lookahead.
 * This is used in conjunction with the ANTLRParser lookahead buffer.
 *
 * No markers are set or anything.  A bunch of input is buffered--that's all.
 * The tp pointer is left alone as the lookahead has not been advanced
 * with getToken().  The next call to getToken() will find a token
 * in the buffer and won't have to call getANTLRToken().
 *
 * If this is called before a consume() is done, how_many_more_i_need is
 * set to 'n'.
 */
_ANTLRTokenPtr ANTLRTokenBuffer::
bufferedToken(int n)
{
//	int how_many_more_i_need = (last-tp < 0) ? n : n-(last-tp)-1;
	int how_many_more_i_need = (tp > last) ? n : n-(last-tp)-1;
	// Make sure that at least n tokens are available in the buffer
#ifdef DBG_TBUF
	/* MR23 */ printMessage(stderr, "bufferedToken(%d)\n", n);
#endif
	for (int i=1; i<=how_many_more_i_need; i++)
	{
		if ( next > end_of_buffer )	// buffer overflow?
		{
			extendBuffer();
		}
		*next = getANTLRToken();
		(*next)->ref();		// say we have a copy of this pointer in buffer
		last = next;
		next++;
	}
	return tp[n - 1];
}

/* If no markers are set, the none of the input needs to be saved (except
 * for the lookahead Token pointers).  We save only k-1 token pointers as
 * we are guaranteed to do a getANTLRToken() right after this because otherwise
 * we wouldn't have needed to extend the buffer.
 *
 * If there are markers in the buffer, we need to save things and so
 * extendBuffer() is called.
 */
void ANTLRTokenBuffer::
makeRoom()
{
#ifdef DBG_TBUF
	/* MR23 */ printMessage(stderr, "in makeRoom.................\n");
	/* MR23 */ printMessage(stderr, "num_markers==%d\n", num_markers);
#endif
/*
	if ( num_markers == 0 )
	{
*/
#ifdef DBG_TBUF
		/* MR23 */ printMessage(stderr, "moving lookahead and resetting next\n");

		_ANTLRTokenPtr *r;
		/* MR23 */ printMessage(stderr, "tbuf = [");
		for (r=buffer; r<=last; r++)
		{
			if ( *r==NULL ) /* MR23 */ printMessage(stderr, " xxx");
			else /* MR23 */ printMessage(stderr, " '%s'", ((ANTLRCommonToken *)*r)->getText());
		}
		/* MR23 */ printMessage(stderr, " ]\n");

		/* MR23 */ printMessage(stderr,
		"before: tp=%d, last=%d, next=%d, threshold=%d\n",tp-buffer,last-buffer,next-buffer,threshold-buffer);
#endif

		// Delete all tokens from 0..last-(k-1) inclusive
		if ( _deleteTokens )
		{
			_ANTLRTokenPtr *z;
			for (z=buffer; z<=last-(k-1); z++)
			{
				(*z)->deref();
//				z->deref();
#ifdef DBG_REFCOUNTTOKEN
					/* MR23 */ printMessage(stderr, "##########makeRoom: deleting token '%s' (ref %d)\n",
							((ANTLRCommonToken *)*z)->getText(), (*z)->nref());
#endif
				if ( (*z)->nref()==0 )
				{
					delete (*z);
				}
			}
		}

		// reset the buffer to initial conditions, but move k-1 symbols
		// to the beginning of buffer and put new input symbol at k
		_ANTLRTokenPtr *p = buffer, *q = last-(k-1)+1;
//		ANTLRAbstractToken **p = buffer, **q = end_of_buffer-(k-1)+1;
#ifdef DBG_TBUF
		/* MR23 */ printMessage(stderr, "lookahead buffer = [");
#endif
		for (int i=1; i<=(k-1); i++)
		{
			*p++ = *q++;
#ifdef DBG_TBUF
			/* MR23 */ printMessage(stderr,
			" '%s'", ((ANTLRCommonToken *)buffer[i-1])->getText());
#endif
		}
#ifdef DBG_TBUF
		/* MR23 */ printMessage(stderr, " ]\n");
#endif
		next = &buffer[k-1];
		tp = &buffer[k-1];	// tp points to what will be filled in next
		last = tp-1;
#ifdef DBG_TBUF
		/* MR23 */ printMessage(stderr,
		"after: tp=%d, last=%d, next=%d\n",
		tp-buffer, last-buffer, next-buffer);
#endif
/*
	}
	else {
		extendBuffer();
	}
*/
}

/* This function extends 'buffer' by chunk_size and returns with all
 * pointers at the same relative positions in the buffer (the buffer base
 * address could have changed in realloc()) except that 'next' comes
 * back set to where the next token should be stored.  All other pointers
 * are untouched.
 */
void
ANTLRTokenBuffer::
extendBuffer()
{
	int save_last = last-buffer, save_tp = tp-buffer, save_next = next-buffer;
#ifdef DBG_TBUF
	/* MR23 */ printMessage(stderr, "extending physical buffer\n");
#endif
	buffer_size += chunk_size;
	buffer = (_ANTLRTokenPtr *)
		realloc((char *)(buffer-1),
				(buffer_size+1)*sizeof(_ANTLRTokenPtr ));
	if ( buffer == NULL ) {
		panic("cannot alloc token buffer");
	}
	buffer++;				// leave the first elem empty so tp-1 is valid ptr

	tp = buffer + save_tp;	// put the pointers back to same relative position
	last = buffer + save_last;
	next = buffer + save_next;
	end_of_buffer = &buffer[buffer_size-1];
	threshold = &buffer[(int)(buffer_size*(1.0/2.0))];

/*
	// zero out new token ptrs so we'll know if something to delete in buffer
	ANTLRAbstractToken **p = end_of_buffer-chunk_size+1;
	for (; p<=end_of_buffer; p++) *p = NULL;
*/
}

ANTLRParser * ANTLRTokenBuffer::				// MR1
setParser(ANTLRParser *p) {					// MR1
  ANTLRParser	*old=parser;					// MR1
  parser=p;							// MR1
  input->setParser(p);						// MR1
  return old;							// MR1
}								// MR1
								// MR1
ANTLRParser * ANTLRTokenBuffer::				// MR1
getParser() {							// MR1
  return parser;						// MR1
}								// MR1

void ANTLRTokenBuffer::panic(const char *msg) // MR23
{ 
	if (parser)				//MR23
		parser->panic(msg);	//MR23
	else					//MR23
		exit(PCCTS_EXIT_FAILURE); 
} 

//MR23
int ANTLRTokenBuffer::printMessage(FILE* pFile, const char* pFormat, ...)
{
	va_list marker;
	va_start( marker, pFormat );

	int iRet = 0;
	if (parser)
		parser->printMessageV(pFile, pFormat, marker);
	else
  		iRet = vfprintf(pFile, pFormat, marker);

	va_end( marker );
	return iRet;
}

/* to avoid having to link in another file just for the smart token ptr
 * stuff, we include it here.  Ugh.
 *
 * MR23 This causes nothing but problems for IDEs.
 *      Change from .cpp to .h
 *
 */

#include ATOKPTR_IMPL_H
