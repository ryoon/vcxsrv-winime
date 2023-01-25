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

/* THIS IS NOT AN X CONSORTIUM STANDARD */

#define NEED_EVENTS
#define NEED_REPLIES
#include <X11/Xlibint.h>
#include "winimestr.h"
#include <X11/extensions/Xext.h>
#include "extutil.h"
#include <stdio.h>

static XExtensionInfo _winime_info_data;
static XExtensionInfo *winime_info = &_winime_info_data;
static char *winime_extension_name = WINIMENAME;

#define WinIMECheckExtension(dpy,i,val) \
  XextCheckExtension (dpy, i, winime_extension_name, val)

/*****************************************************************************
 *                                                                           *
 *			   private utility routines                          *
 *                                                                           *
 *****************************************************************************/

static int close_display (Display *dpy, XExtCodes *extCodes);
static Bool wire_to_event ();
static Status event_to_wire ();

static /* const */ XExtensionHooks winime_extension_hooks = {
  NULL,				/* create_gc */
  NULL,				/* copy_gc */
  NULL,				/* flush_gc */
  NULL,				/* free_gc */
  NULL,				/* create_font */
  NULL,				/* free_font */
  close_display,		/* close_display */
  wire_to_event,		/* wire_to_event */
  event_to_wire,		/* event_to_wire */
  NULL,				/* error */
  NULL,				/* error_string */
};

static XEXT_GENERATE_FIND_DISPLAY (find_display, winime_info,
				   winime_extension_name,
				   &winime_extension_hooks,
				   WinIMENumberEvents, NULL);

static XEXT_GENERATE_CLOSE_DISPLAY (close_display, winime_info);

static Bool
wire_to_event (Display *dpy, XEvent  *re, xEvent  *event)
{
  XExtDisplayInfo *info = find_display (dpy);
  XWinIMENotifyEvent *se;
  xWinIMENotifyEvent *sevent;

  WinIMECheckExtension (dpy, info, False);

  switch ((event->u.u.type & 0x7f) - info->codes->first_event)
    {
    case WinIMEControllerNotify:
      se = (XWinIMENotifyEvent *) re;
      sevent = (xWinIMENotifyEvent *) event;
      se->type = sevent->type & 0x7f;
      se->serial = _XSetLastRequestRead(dpy,(xGenericReply *) event);
      se->send_event = (sevent->type & 0x80) != 0;
      se->display = dpy;
      se->context = sevent->context;
      se->time = sevent->time;
      se->kind = sevent->kind;
      se->arg = sevent->arg;
      return True;
    }
  return False;
}

static Status
event_to_wire (Display *dpy, XEvent  *re, xEvent  *event)
{
  XExtDisplayInfo *info = find_display (dpy);
  XWinIMENotifyEvent *se;
  xWinIMENotifyEvent *sevent;

  WinIMECheckExtension (dpy, info, False);

  switch ((re->type & 0x7f) - info->codes->first_event)
    {
    case WinIMEControllerNotify:
      se = (XWinIMENotifyEvent *) re;
      sevent = (xWinIMENotifyEvent *) event;
      sevent->type = se->type | (se->send_event ? 0x80 : 0);
      sevent->sequenceNumber = se->serial & 0xffff;
      sevent->context = se->context;
      sevent->kind = se->kind;
      sevent->arg = se->arg;
      sevent->time = se->time;
      return 1;
  }
  return 0;
}

/*****************************************************************************
 *                                                                           *
 *		    public WinIME Extension routines                         *
 *                                                                           *
 *****************************************************************************/

#if 1
#include <stdio.h>
#define TRACE(msg)  fprintf(stderr, "WinIME:%s\n", msg);
#else
#define TRACE(msg)
#endif


Bool
XWinIMEQueryExtension (Display *dpy,
		       int *event_basep, int *error_basep)
{
  XExtDisplayInfo *info = find_display (dpy);

  TRACE("QueryExtension...");
  if (XextHasExtension(info))
    {
      *event_basep = info->codes->first_event;
      *error_basep = info->codes->first_error;
      TRACE("QueryExtension... return True");
      return True;
    }
  else
    {
      TRACE("QueryExtension... return False");
      return False;
    }
}

Bool
XWinIMEQueryVersion (Display* dpy, int* majorVersion,
		     int* minorVersion, int* patchVersion)
{
  XExtDisplayInfo *info = find_display (dpy);
  xWinIMEQueryVersionReply rep;
  xWinIMEQueryVersionReq *req;

  TRACE("QueryVersion...");
  WinIMECheckExtension (dpy, info, False);

  LockDisplay(dpy);
  GetReq(WinIMEQueryVersion, req);
  req->reqType = info->codes->major_opcode;
  req->imeReqType = X_WinIMEQueryVersion;
  if (!_XReply(dpy, (xReply *)&rep, 0, xFalse))
    {
      UnlockDisplay(dpy);
      SyncHandle();
      TRACE("QueryVersion... return False");
      return False;
    }
  *majorVersion = rep.majorVersion;
  *minorVersion = rep.minorVersion;
  *patchVersion = rep.patchVersion;
  UnlockDisplay(dpy);
  SyncHandle();
  TRACE("QueryVersion... return True");
  return True;
}

Bool
XWinIMESelectInput (Display* dpy, unsigned long mask)
{
  XExtDisplayInfo *info = find_display (dpy);
  xWinIMESelectInputReq *req;

  TRACE("SelectInput...");
  WinIMECheckExtension (dpy, info, False);

  LockDisplay(dpy);
  GetReq(WinIMESelectInput, req);
  req->reqType = info->codes->major_opcode;
  req->imeReqType = X_WinIMESelectInput;
  req->mask = mask;
  UnlockDisplay(dpy);
  SyncHandle();
  TRACE("SetlectInput... return True");
  return True;
}

Bool
XWinIMECreateContext (Display* dpy, int* context)
{
  XExtDisplayInfo *info = find_display (dpy);
  xWinIMECreateContextReply rep;
  xWinIMECreateContextReq *req;

  TRACE("CreateContext...");
  WinIMECheckExtension (dpy, info, False);

  LockDisplay(dpy);
  GetReq(WinIMECreateContext, req);
  req->reqType = info->codes->major_opcode;
  req->imeReqType = X_WinIMECreateContext;
  if (!_XReply(dpy, (xReply *)&rep, 0, xFalse))
    {
      UnlockDisplay(dpy);
      SyncHandle();
      TRACE("CreateContext... return False");
      return False;
    }
  *context = rep.context;
  UnlockDisplay(dpy);
  SyncHandle();
  TRACE("CreateContext... return True");
  return True;
}

Bool
XWinIMESetOpenStatus (Display* dpy, int context, Bool state)
{
  XExtDisplayInfo *info = find_display (dpy);
  xWinIMESetOpenStatusReq *req;

  TRACE("IMESetOpenStatus...");
  WinIMECheckExtension (dpy, info, False);

  LockDisplay(dpy);
  GetReq(WinIMESetOpenStatus, req);
  req->reqType = info->codes->major_opcode;
  req->imeReqType = X_WinIMESetOpenStatus;
  req->context = context;
  req->state = state;
  UnlockDisplay(dpy);
  SyncHandle();
  TRACE("IMESetOpenStatus... return True");

  return True;
}

Bool
XWinIMESetCompositionWindow (Display* dpy, int context,
			     int style,
			     short cf_x, short cf_y,
			     short cf_w, short cf_h)
{
  XExtDisplayInfo *info = find_display (dpy);
  xWinIMESetCompositionWindowReq *req;

  TRACE("SetCompositionWindow...");
  WinIMECheckExtension (dpy, info, False);

  LockDisplay(dpy);
  GetReq(WinIMESetCompositionWindow, req);
  req->reqType = info->codes->major_opcode;
  req->imeReqType = X_WinIMESetCompositionWindow;
  req->context = context;
  req->style = style;
  req->ix = cf_x;
  req->iy = cf_y;
  req->iw = cf_w;
  req->ih = cf_h;

  UnlockDisplay(dpy);
  SyncHandle();
  TRACE("SetCompositionWindow... return True");
  return True;
}

int
XWinIMEGetCompositionString (Display *dpy, int context,
			     int index,
			     int count,
			     char* str_return)
{
  XExtDisplayInfo *info = find_display (dpy);
  xWinIMEGetCompositionStringReq *req;
  xWinIMEGetCompositionStringReply rep;
  char *str;

  TRACE("GetCompositionString...");
  WinIMECheckExtension (dpy, info, False);

  LockDisplay(dpy);
  GetReq(WinIMEGetCompositionString, req);
  req->reqType = info->codes->major_opcode;
  req->imeReqType = X_WinIMEGetCompositionString;
  req->context = context;
  req->index = index;
  rep.strLength = 0;

  if (!_XReply(dpy, (xReply *)&rep, 0, xFalse))
    {
      UnlockDisplay(dpy);
      SyncHandle();
      TRACE("GetCompositionString... return False");
      return -1;
    }

  if ((str = (char *) Xmalloc(rep.strLength+1)))
    {
      _XReadPad(dpy, str, (long)rep.strLength);
    }
  else
    {
      _XEatData(dpy, (unsigned long) (rep.strLength + 3) & ~3);
      str = (char *) NULL;
    }

  memcpy(str_return, str, rep.strLength);
  //fprintf(stderr, "%s(%d/%d) %s(%d/%d)", str, rep.strLength, strlen(str),
  //str_return, count, strlen(str_return));
  Xfree(str);

  UnlockDisplay(dpy);
  SyncHandle();
  TRACE("GetCompositionString... return True");

  return rep.strLength;
}

Bool
XWinIMESetFocus (Display* dpy, int context, Bool focus)
{
  XExtDisplayInfo *info = find_display (dpy);
  xWinIMESetFocusReq *req;

  TRACE("IMESetFocus...");
  WinIMECheckExtension (dpy, info, False);

  LockDisplay(dpy);
  GetReq(WinIMESetFocus, req);
  req->reqType = info->codes->major_opcode;
  req->imeReqType = X_WinIMESetFocus;
  req->context = context;
  req->focus = focus;
  UnlockDisplay(dpy);
  SyncHandle();
  TRACE("IMESetFocus... return True");

  return True;
}

Bool
XWinIMESetCompositionDraw (Display* dpy, int context, Bool draw)
{
  XExtDisplayInfo *info = find_display (dpy);
  xWinIMESetCompositionDrawReq *req;

  TRACE("IMESetCompositionDraw...");
  WinIMECheckExtension (dpy, info, False);

  LockDisplay(dpy);
  GetReq(WinIMESetCompositionDraw, req);
  req->reqType = info->codes->major_opcode;
  req->imeReqType = X_WinIMESetCompositionDraw;
  req->context = context;
  req->draw = draw;
  UnlockDisplay(dpy);
  SyncHandle();
  TRACE("IMESetCompositionDraw... return True");

  return True;
}

Bool
XWinIMEGetCursorPosition (Display* dpy, int context, int *cursor)
{
  XExtDisplayInfo *info = find_display (dpy);
  xWinIMEGetCursorPositionReply rep;
  xWinIMEGetCursorPositionReq *req;

  TRACE("GetCursorPosition...");
  WinIMECheckExtension (dpy, info, False);

  LockDisplay(dpy);
  GetReq(WinIMEGetCursorPosition, req);
  req->reqType = info->codes->major_opcode;
  req->imeReqType = X_WinIMEGetCursorPosition;
  req->context = context;
  if (!_XReply(dpy, (xReply *)&rep, 0, xFalse))
    {
      UnlockDisplay(dpy);
      SyncHandle();
      TRACE("GetCursorPosition... return False");
      return False;
    }
  *cursor = rep.cursor;
  UnlockDisplay(dpy);
  SyncHandle();
  TRACE("GetCursorPosition... return True");
  return True;
}
