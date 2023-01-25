/*
 *Copyright (C) 2004-2005 Kensuke Matsuzaki. All Rights Reserved.
 *
 *Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 *"Software"), to deal in the Software without restriction, including
 *without limitation the rights to use, copy, modify, merge, publish,
 *distribute, sublicense, and/or sell copies of the Software, and to
 *permit persons to whom the Software is furnished to do so, subject to
 *the following conditions:
 *
 *The above copyright notice and this permission notice shall be
 *included in all copies or substantial portions of the Software.
 *
 *THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *NONINFRINGEMENT. IN NO EVENT SHALL KENSUKE MATSUZAKI BE LIABLE FOR
 *ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 *CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 *WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *Except as contained in this notice, the name of Kensuke Matsuzaki
 *shall not be used in advertising or otherwise to promote the sale, use
 *or other dealings in this Software without prior written authorization
 *from Kensuke Matsuzaki.
 *
 * Authors:	Kensuke Matsuzaki <zakki@peppermint.jp>
 */

#ifndef _WINIMESTR_H_
#define _WINIMESTR_H_

#include <X11/extensions/winime.h>
#include <X11/X.h>
#include <X11/Xmd.h>

#define WINIMENAME "WinIME"

#define WIN_IME_MAJOR_VERSION	1	/* current version numbers */
#define WIN_IME_MINOR_VERSION	0
#define WIN_IME_PATCH_VERSION	0

typedef struct _WinIMEQueryVersion {
    CARD8	reqType;		/* always IMEReqCode */
    CARD8	imeReqType;		/* always X_WinIMEQueryVersion */
    CARD16	length B16;
} xWinIMEQueryVersionReq;
#define sz_xWinIMEQueryVersionReq	4

typedef struct {
    BYTE	type;			/* X_Reply */
    BOOL	pad1;
    CARD16	sequenceNumber B16;
    CARD32	length B32;
    CARD16	majorVersion B16;	/* major version of IME protocol */
    CARD16	minorVersion B16;	/* minor version of IME protocol */
    CARD32	patchVersion B32;       /* patch version of IME protocol */
    CARD32	pad3 B32;
    CARD32	pad4 B32;
    CARD32	pad5 B32;
    CARD32	pad6 B32;
} xWinIMEQueryVersionReply;
#define sz_xWinIMEQueryVersionReply	32

typedef struct _WinIMESelectInput {
    CARD8	reqType;		/* always IMEReqCode */
    CARD8	imeReqType;		/* always X_IMESelectInput */
    CARD16	length B16;
    CARD32	mask B32;
} xWinIMESelectInputReq;
#define sz_xWinIMESelectInputReq	8

typedef struct _WinIMENotify {
	BYTE	type;		/* always eventBase + event type */
	BYTE	kind;
	CARD16	sequenceNumber B16;
	CARD32	context B32;
	Time	time B32;	/* time of change */
	CARD16	pad1 B16;
	CARD32	arg B32;
} xWinIMENotifyEvent;
#define sz_xWinIMENotifyEvent	18

typedef struct _WinIMECreateContext {
    CARD8	reqType;		/* always IMEReqCode */
    CARD8	imeReqType;		/* always X_WinIMECreateContext */
    CARD16	length B16;
} xWinIMECreateContextReq;
#define sz_xWinIMECreateContextReq	4

typedef struct {
    BYTE	type;			/* X_Reply */
    BOOL	pad1;
    CARD16	sequenceNumber B16;
    CARD32	length B32;
    CARD32	context B32;		/* input context */
    CARD32	pad2 B32;
    CARD32	pad3 B32;
    CARD32	pad4 B32;
    CARD32	pad5 B32;
    CARD32	pad6 B32;
} xWinIMECreateContextReply;
#define sz_xWinIMECreateContextReply	32

typedef struct _WinIMESetOpenStetus {
    CARD8	reqType;		/* always IMEReqCode */
    CARD8	imeReqType;		/* always X_IMEOpenStatus */
    CARD16	length B16;
    CARD32	context B32;
    CARD32	state B32;
} xWinIMESetOpenStatusReq;
#define sz_xWinIMESetOpenStatusReq	12

typedef struct _WinIMESetCompositionWindow {
    CARD8	reqType;		/* always IMEReqCode */
    CARD8	imeReqType;		/* always X_IMESetCompositionPoint */
    CARD16	length B16;
    CARD32	style B32;
    CARD32	context B32;
    INT16	ix B16;
    INT16	iy B16;
    INT16	iw B16;
    INT16	ih B16;
} xWinIMESetCompositionWindowReq;
#define sz_xWinIMESetCompositionWindowReq	20

typedef struct _WinIMEGetCompositionString {
    CARD8	reqType;		/* always IMEReqCode */
    CARD8	imeReqType;		/* always X_IMEGetCompositionString */
    CARD16	length B16;
    CARD32	context B32;
    CARD32	index B32;
    CARD32	pad1 B32;
} xWinIMEGetCompositionStringReq;
#define sz_xWinIMEGetCompositionStringReq	16

typedef struct {
    BYTE	type;			/* X_Reply */
    BOOL	pad1;
    CARD16	sequenceNumber B16;
    CARD32	length B32;
    CARD16	strLength B16;  /* # of characters in name */
    CARD16 pad2 B16;
    CARD32 pad3 B32;
    CARD32 pad4 B32;
    CARD32 pad5 B32;
    CARD32 pad6 B32;
    CARD32 pad7 B32;
} xWinIMEGetCompositionStringReply;
#define sz_xWinIMEGetCompositionStringReply	32

typedef struct _WinIMESetFocus {
    CARD8	reqType;		/* always IMEReqCode */
    CARD8	imeReqType;		/* always X_IMESetFocus */
    CARD16	length B16;
    CARD32	context B32;
    CARD32	focus B32;
} xWinIMESetFocusReq;
#define sz_xWinIMESetFocusReq	12

typedef struct _WinIMESetCompositionDraw {
    CARD8	reqType;		/* always IMEReqCode */
    CARD8	imeReqType;		/* always X_IMESetCompositionDraw */
    CARD16	length B16;
    CARD32	context B32;
    CARD32	draw B32;
} xWinIMESetCompositionDrawReq;
#define sz_xWinIMESetCompositionDrawReq	12

typedef struct _WinIMEGetCursorPosition {
    CARD8	reqType;		/* always IMEReqCode */
    CARD8	imeReqType;		/* always X_WinIMEGetCursorPosition */
    CARD16	length B16;
    CARD32	context B32;
} xWinIMEGetCursorPositionReq;
#define sz_xWinIMEGetCursorPositionReq	8

typedef struct {
    BYTE	type;			/* X_Reply */
    BOOL	pad1;
    CARD16	sequenceNumber B16;
    CARD32	length B32;
    CARD32	context B32;		/* input context */
    CARD32	cursor B32;
    CARD32	pad3 B32;
    CARD32	pad4 B32;
    CARD32	pad5 B32;
    CARD32	pad6 B32;
} xWinIMEGetCursorPositionReply;
#define sz_xWinIMEGetCursorPositionReply	32

#endif /* _WINIMESTR_H_ */
