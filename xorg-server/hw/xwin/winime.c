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

#include "win.h"

#define NEED_REPLIES
#define NEED_EVENTS
#include "misc.h"
#include "dixstruct.h"
#include "extnsionst.h"
//#include "colormapst.h"
//#include "cursorstr.h"
//#include "scrnintstr.h"
#include "servermd.h"
//#include "swaprep.h"
#define _WINIME_SERVER_
#include "winimestr.h"
#include "winkeynames.h"
#include <imm.h>

#define CYGIME_DEBUG TRUE

extern Bool g_fIME;

static int WinIMEErrorBase;

static DISPATCH_PROC(ProcWinIMEDispatch);
static DISPATCH_PROC(SProcWinIMEDispatch);

static void WinIMEResetProc(ExtensionEntry* extEntry);

static unsigned char WinIMEReqCode = 0;
static int WinIMEEventBase = 0;

static RESTYPE ClientType, EventType; /* resource types for event masks */
static XID eventResource;

/* Currently selected events */
static unsigned int eventMask = 0;

static int WinIMEFreeClient (pointer data, XID id);
static int WinIMEFreeEvents (pointer data, XID id);
static void SNotifyEvent(xWinIMENotifyEvent *from, xWinIMENotifyEvent *to);

typedef struct _WinIMEEvent *WinIMEEventPtr;
typedef struct _WinIMEEvent {
  WinIMEEventPtr	next;
  ClientPtr		client;
  XID			clientResource;
  unsigned int		mask;
} WinIMEEventRec;

typedef struct _WIContext *WIContextPtr;
typedef struct _WIContext {
  WIContextPtr		pNext;
  int			nContext;
  HIMC			hIMC;
  BOOL			fCompositionDraw;
  int			nCursor;
  DWORD			dwCompositionStyle;
  POINT			ptCompositionPos;
  RECT			rcCompositionArea;
  char			*pszComposition;
  char			*pszCompositionResult;
  char			*pAttr;
  int			nAttr;
} WIContextRec;

static int s_nContextMax = 0;
static WIContextPtr s_pContextList = NULL;

static WIContextPtr
NewContext()
{
  WIContextPtr pWIC;

#if CYGIME_DEBUG
  winDebug ("%s\n", __FUNCTION__);
#endif

  pWIC = (WIContextPtr)malloc(sizeof(WIContextRec));

  if (pWIC)
    {
      /* Init member */
      pWIC->nContext = ++s_nContextMax;
      pWIC->hIMC = ImmCreateContext ();
      pWIC->fCompositionDraw = FALSE;
      pWIC->dwCompositionStyle = CFS_DEFAULT;
      pWIC->pszComposition = NULL;
      pWIC->pszCompositionResult = NULL;
      pWIC->pAttr = NULL;
      pWIC->nAttr = 0;

      /* Add to list. */
      pWIC->pNext = s_pContextList;
      s_pContextList = pWIC;

#if CYGIME_DEBUG
      winDebug ("nContext:%d hIMC:%d\n", pWIC->nContext, pWIC->hIMC);
#endif
    }

  return pWIC;
}

static WIContextPtr
FindContext(int nContext)
{
  WIContextPtr pWIC;

#if CYGIME_DEBUG
  winDebug ("%s %d\n", __FUNCTION__, nContext);
#endif

  for (pWIC = s_pContextList; pWIC; pWIC = pWIC->pNext)
    {
      if (pWIC->nContext == nContext)
	{
#if CYGIME_DEBUG
	  winDebug ("found.\n");
#endif
	  return pWIC;
	}
    }

#if CYGIME_DEBUG
  winDebug ("not found.\n");
#endif

  return NULL;
}

static void
DeleteContext(int nContext)
{
  WIContextPtr pWIC, pPrev = NULL;

#if CYGIME_DEBUG
  winDebug ("%s %d\n", __FUNCTION__, nContext);
#endif

  for (pWIC = s_pContextList; pWIC; pPrev = pWIC, pWIC = pWIC->pNext)
    {
      if (pWIC->nContext == nContext)
	{
	  if (pPrev)
	    pPrev->pNext = pWIC->pNext;
	  else
	    s_pContextList = pWIC->pNext;

	  ImmDestroyContext (pWIC->hIMC);
	  free (pWIC);
	}
    }
}

static void
DeleteAllContext()
{
  WIContextPtr pWIC, pNext = NULL;

  for (pWIC = s_pContextList; pWIC; pWIC = pNext)
    {
      pNext = pWIC->pNext;

      ImmDestroyContext (pWIC->hIMC);
      free (pWIC);
    }

  s_pContextList = NULL;
}

int
winHIMCtoContext(DWORD hIMC)
{
  WIContextPtr pWIC;

#if CYGIME_DEBUG
  winDebug ("%s %d\n", __FUNCTION__, hIMC);
#endif

  for (pWIC = s_pContextList; pWIC; pWIC = pWIC->pNext)
    {
      if (pWIC->hIMC == hIMC)
	{
#if CYGIME_DEBUG
	  winDebug ("found.\n");
#endif
	  return pWIC->nContext;
	}
    }

#if CYGIME_DEBUG
  winDebug ("not found.\n");
#endif
  return 0;
}

Bool
winHIMCCompositionDraw(DWORD hIMC)
{
  WIContextPtr pWIC;

#if CYGIME_DEBUG
  winDebug ("%s %d\n", __FUNCTION__, hIMC);
#endif

  for (pWIC = s_pContextList; pWIC; pWIC = pWIC->pNext)
    {
      if (pWIC->hIMC == hIMC)
	{
#if CYGIME_DEBUG
	  winDebug ("found.\n");
#endif
	  return pWIC->fCompositionDraw;
	}
    }

#if CYGIME_DEBUG
  winDebug ("not found.\n");
#endif
  return FALSE;
}

void
winCommitCompositionResult (int nContext, int nIndex, void *pData, int nLen)
{
  WIContextPtr pWIC;
#if CYGIME_DEBUG
  winDebug ("%s\n", __FUNCTION__);
#endif

  if (!(pWIC = FindContext(nContext))) return;

  switch (nIndex)
    {
    case GCS_COMPSTR:
      if (pWIC->pszComposition)
	{
	  free (pWIC->pszComposition);
	}
      pWIC->pszComposition = (char*)pData;
      break;

    case GCS_RESULTSTR:
      if (pWIC->pszCompositionResult)
	{
	  free (pWIC->pszCompositionResult);
	}
      pWIC->pszCompositionResult = (char*)pData;
      break;

    case GCS_CURSORPOS:
      pWIC->nCursor = *(int*)pData;
      break;

    case GCS_COMPATTR:
      if (pWIC->pAttr)
	{
	  free (pWIC->pAttr);
	}
      pWIC->pAttr = (char*)pData;
      pWIC->nAttr = nLen;
      break;

    default:
      break;
    }
}

static Bool
IsRoot(WindowPtr pWin)
{
  return pWin == (pWin)->drawable.pScreen->root;
}

static Bool
IsTopLevel(WindowPtr pWin)
{
  return pWin && (pWin)->parent && IsRoot(pWin->parent);
}

static WindowPtr
GetTopLevelParent(WindowPtr pWindow)
{
  WindowPtr pWin = pWindow;

  if (!pWin || IsRoot(pWin)) return NULL;

  while (pWin && !IsTopLevel(pWin))
    {
      pWin = pWin->parent;
    }
  return pWin;
}

void
winWinIMEExtensionInit ()
{
  ExtensionEntry* extEntry;

#if CYGIME_DEBUG
  winDebug ("%s\n", __FUNCTION__);
#endif

  ClientType = CreateNewResourceType(WinIMEFreeClient, "WinIMEFreeClient");
  EventType = CreateNewResourceType(WinIMEFreeEvents, "WinIMEFreeEvents");
  eventResource = FakeClientID(0);

  if (ClientType && EventType &&
      (extEntry = AddExtension(WINIMENAME,
			       WinIMENumberEvents,
			       WinIMENumberErrors,
			       ProcWinIMEDispatch,
			       SProcWinIMEDispatch,
			       WinIMEResetProc,
			       StandardMinorOpcode)))
    {
      WinIMEReqCode = (unsigned char)extEntry->base;
      WinIMEErrorBase = extEntry->errorBase;
      WinIMEEventBase = extEntry->eventBase;
      EventSwapVector[WinIMEEventBase] = (EventSwapPtr) SNotifyEvent;

      DeleteAllContext();
    }
}

/*ARGSUSED*/
static void
WinIMEResetProc (ExtensionEntry* extEntry)
{
#if CYGIME_DEBUG
  winDebug ("%s\n", __FUNCTION__);
#endif

  DeleteAllContext();
}

static int
ProcWinIMEQueryVersion(register ClientPtr client)
{
  xWinIMEQueryVersionReply rep;
  register int n;

#if CYGIME_DEBUG
  winDebug ("%s\n", __FUNCTION__);
#endif

  REQUEST_SIZE_MATCH(xWinIMEQueryVersionReq);
  rep.type = X_Reply;
  rep.length = 0;
  rep.sequenceNumber = client->sequence;
  rep.majorVersion = WIN_IME_MAJOR_VERSION;
  rep.minorVersion = WIN_IME_MINOR_VERSION;
  rep.patchVersion = WIN_IME_PATCH_VERSION;
  if (client->swapped)
    {
      swaps(&rep.sequenceNumber, n);
      swapl(&rep.length, n);
    }
  WriteToClient(client, sizeof(xWinIMEQueryVersionReply), (char *)&rep);
  return (client->noClientException);
}


/* events */

static inline void
updateEventMask (WinIMEEventPtr *pHead)
{
  WinIMEEventPtr pCur;

#if CYGIME_DEBUG
  winDebug ("%s\n", __FUNCTION__);
#endif

  eventMask = 0;
  for (pCur = *pHead; pCur != NULL; pCur = pCur->next)
    eventMask |= pCur->mask;
}

/*ARGSUSED*/
static int
WinIMEFreeClient (pointer data, XID id)
{
  WinIMEEventPtr   pEvent;
  WinIMEEventPtr   *pHead, pCur, pPrev;

#if CYGIME_DEBUG
  winDebug ("%s\n", __FUNCTION__);
#endif

  pEvent = (WinIMEEventPtr) data;
  pHead = dixLookupResourceByType((pointer *)&pHead, eventResource, EventType,
		  NullClient, DixUnknownAccess);
  if (pHead)
    {
      pPrev = 0;
      for (pCur = *pHead; pCur && pCur != pEvent; pCur=pCur->next)
	pPrev = pCur;
      if (pCur)
	{
	  if (pPrev)
	    pPrev->next = pEvent->next;
	  else
	    *pHead = pEvent->next;
	}
      updateEventMask (pHead);
    }
  free ((pointer) pEvent);

  return 1;
}

/*ARGSUSED*/
static int
WinIMEFreeEvents (pointer data, XID id)
{
  WinIMEEventPtr   *pHead, pCur, pNext;

#if CYGIME_DEBUG
  winDebug ("%s\n", __FUNCTION__);
#endif

  pHead = (WinIMEEventPtr *) data;
  for (pCur = *pHead; pCur; pCur = pNext)
    {
      pNext = pCur->next;
      FreeResource (pCur->clientResource, ClientType);
      free ((pointer) pCur);
    }
  free ((pointer) pHead);
  eventMask = 0;

  return 1;
}

static int
ProcWinIMESelectInput (register ClientPtr client)
{
  REQUEST(xWinIMESelectInputReq);
  WinIMEEventPtr	pEvent, pNewEvent, *pHead;
  XID			clientResource;

#if CYGIME_DEBUG
  winDebug ("%s\n", __FUNCTION__);
#endif

  REQUEST_SIZE_MATCH (xWinIMESelectInputReq);
  dixLookupResourceByType((pointer *)&pHead, eventResource,
						   EventType, client,
						   DixWriteAccess);
  if (stuff->mask != 0)
    {
      if (pHead)
	{
	  /* check for existing entry. */
	  for (pEvent = *pHead; pEvent; pEvent = pEvent->next)
	    {
	      if (pEvent->client == client)
		{
		  pEvent->mask = stuff->mask;
		  updateEventMask (pHead);
		  return Success;
		}
	    }
	}

      /* build the entry */
      pNewEvent = (WinIMEEventPtr) malloc (sizeof (WinIMEEventRec));
      if (!pNewEvent)
	return BadAlloc;
      pNewEvent->next = 0;
      pNewEvent->client = client;
      pNewEvent->mask = stuff->mask;
      /*
       * add a resource that will be deleted when
       * the client goes away
       */
      clientResource = FakeClientID (client->index);
      pNewEvent->clientResource = clientResource;
      if (!AddResource (clientResource, ClientType, (pointer)pNewEvent))
	return BadAlloc;
      /*
       * create a resource to contain a pointer to the list
       * of clients selecting input.  This must be indirect as
       * the list may be arbitrarily rearranged which cannot be
       * done through the resource database.
       */
      if (!pHead)
	{
	  pHead = (WinIMEEventPtr *) malloc (sizeof (WinIMEEventRec));
	  if (!pHead ||
	      !AddResource (eventResource, EventType, (pointer)pHead))
	    {
	      FreeResource (clientResource, RT_NONE);
	      return BadAlloc;
	    }
	  *pHead = 0;
	}
      pNewEvent->next = *pHead;
      *pHead = pNewEvent;
      updateEventMask (pHead);
    }
  else if (stuff->mask == 0)
    {
      /* delete the interest */
      if (pHead)
	{
	  pNewEvent = 0;
	  for (pEvent = *pHead; pEvent; pEvent = pEvent->next)
	    {
	      if (pEvent->client == client)
		break;
	      pNewEvent = pEvent;
	    }
	  if (pEvent)
	    {
	      FreeResource (pEvent->clientResource, ClientType);
	      if (pNewEvent)
		pNewEvent->next = pEvent->next;
	      else
		*pHead = pEvent->next;
	      free (pEvent);
	      updateEventMask (pHead);
	    }
	}
    }
  else
    {
      client->errorValue = stuff->mask;
      return BadValue;
    }
  return Success;
}

static int
ProcWinIMECreateContext(register ClientPtr client)
{
  xWinIMECreateContextReply rep;
  WIContextPtr pWIC;
  register int n;

#if CYGIME_DEBUG
  winDebug ("%s\n", __FUNCTION__);
#endif

  if (!(pWIC = NewContext()))
    {
      return BadValue;
    }

  REQUEST_SIZE_MATCH(xWinIMECreateContextReq);
  rep.type = X_Reply;
  rep.length = 0;
  rep.sequenceNumber = client->sequence;
  rep.context = pWIC->nContext;
  if (client->swapped)
    {
      swaps(&rep.sequenceNumber, n);
      swapl(&rep.length, n);
    }
  WriteToClient(client, sizeof(xWinIMECreateContextReply), (char *)&rep);
  return (client->noClientException);
}

static int
ProcWinIMESetOpenStatus (register ClientPtr client)
{
  REQUEST(xWinIMESetOpenStatusReq);
  WIContextPtr pWIC;

  REQUEST_SIZE_MATCH(xWinIMESetOpenStatusReq);

#if CYGIME_DEBUG
  winDebug ("%s\n", __FUNCTION__);
#endif

  if (!(pWIC = FindContext(stuff->context)))
    {
      return BadValue;
    }

  ImmSetOpenStatus (pWIC->hIMC, stuff->state);

  return (client->noClientException);
}

static int
ProcWinIMESetCompositionWindow (register ClientPtr client)
{
  REQUEST(xWinIMESetCompositionWindowReq);
  WIContextPtr pWIC;
  COMPOSITIONFORM cf;
  REQUEST_SIZE_MATCH(xWinIMESetCompositionWindowReq);

#if CYGIME_DEBUG
  winDebug ("%s\n", __FUNCTION__);
#endif

  if (!(pWIC = FindContext(stuff->context)))
    {
      return BadValue;
    }

  pWIC->dwCompositionStyle = stuff->style;
  pWIC->ptCompositionPos.x = stuff->ix;
  pWIC->ptCompositionPos.y = stuff->iy;
  pWIC->rcCompositionArea.left = stuff->ix;
  pWIC->rcCompositionArea.top = stuff->iy;
  pWIC->rcCompositionArea.right = stuff->ix + stuff->iw;
  pWIC->rcCompositionArea.bottom = stuff->iy + stuff->ih;

  cf.dwStyle = pWIC->dwCompositionStyle;
  cf.ptCurrentPos.x = pWIC->ptCompositionPos.x;
  cf.ptCurrentPos.y = pWIC->ptCompositionPos.y;
  cf.rcArea.left = pWIC->rcCompositionArea.left;
  cf.rcArea.top = pWIC->rcCompositionArea.top;
  cf.rcArea.right = pWIC->rcCompositionArea.right;
  cf.rcArea.bottom = pWIC->rcCompositionArea.bottom;
  ImmSetCompositionWindow(pWIC->hIMC, &cf);

  return (client->noClientException);
}

static int
ProcWinIMEGetCompositionString (register ClientPtr client)
{
  REQUEST(xWinIMEGetCompositionStringReq);
  WIContextPtr pWIC;
  int len;
  xWinIMEGetCompositionStringReply rep;

#if CYGIME_DEBUG
  winDebug ("%s %d\n", __FUNCTION__, stuff->context);
#endif

  REQUEST_SIZE_MATCH(xWinIMEGetCompositionStringReq);

  if ((pWIC = FindContext(stuff->context)))
    {
      switch (stuff->index)
	{
	case WinIMECMPCompStr:
	  {
	    if (pWIC->pszComposition)
	      {
		len = strlen(pWIC->pszComposition);
		rep.type = X_Reply;
		rep.length = (len + 3) >> 2;
		rep.sequenceNumber = client->sequence;
		rep.strLength = len;
		WriteReplyToClient(client, sizeof(xWinIMEGetCompositionStringReply), &rep);
		(void)WriteToClient(client, len, pWIC->pszComposition);
	      }
	    else
	      {
#if CYGIME_DEBUG
		winDebug ("no composition result.\n");
#endif
		return BadValue;
	      }
	  }
	  break;
	case WinIMECMPResultStr:
	  {
	    if (pWIC->pszCompositionResult)
	      {
		len = strlen(pWIC->pszCompositionResult);
		rep.type = X_Reply;
		rep.length = (len + 3) >> 2;
		rep.sequenceNumber = client->sequence;
		rep.strLength = len;
		WriteReplyToClient(client, sizeof(xWinIMEGetCompositionStringReply), &rep);
		(void)WriteToClient(client, len, pWIC->pszCompositionResult);
	      }
	    else
	      {
#if CYGIME_DEBUG
		winDebug ("no composition result.\n");
#endif
		return BadValue;
	      }
	  }
	  break;
	case WinIMECMPCompAttr:
	  {
	    if (pWIC->pszComposition)
	      {
		len = pWIC->nAttr;
		rep.type = X_Reply;
		rep.length = (len + 3) >> 2;
		rep.sequenceNumber = client->sequence;
		rep.strLength = len;
		WriteReplyToClient(client, sizeof(xWinIMEGetCompositionStringReply), &rep);
		(void)WriteToClient(client, len, pWIC->pAttr);
	      }
	    else
	      {
#if CYGIME_DEBUG
		winDebug ("no composition result.\n");
#endif
		return BadValue;
	      }
	  }
	  break;
	default:
	  {
#if CYGIME_DEBUG
	    winDebug ("bad index.\n");
#endif
	    return BadValue;
	  }
	}
    }
  else
    {
#if CYGIME_DEBUG
      winDebug ("context is not found.\n");
#endif
      return BadValue;
    }

  return (client->noClientException);
}

#if 0
static int
ProcWinIMEFilterKeyEvent (register ClientPtr client)
{
  REQUEST(xWinIMEFilterKeyEventReq);
  WindowPtr pWin;
  winPrivWinPtr	pWinPriv;
  MSG msg;

  REQUEST_SIZE_MATCH(xWinIMEFilterKeyEventReq);

#if CYGIME_DEBUG
  winDebug ("%s\n", __FUNCTION__);
#endif

#if CYGIME_DEBUG
  //winDebug ("0x%02x 0x%02x\n", stuff->keycode, MapVirtualKey (stuff->keycode, 1));
  //winDebug ("0x%02x 0x%02x\n", stuff->keycode - MIN_KEYCODE, MapVirtualKey (stuff->keycode - MIN_KEYCODE, 1));
#endif
#if 0
  msg.hwnd = GetActiveWindow ();
  msg.lParam = MapVirtualKey (stuff->keycode - MIN_KEYCODE, 1);
  msg.wParam = 1
    |  (stuff->keycode - MIN_KEYCODE << 16); //scancode
  msg.time = GetTickCount ();

  switch (stuff->type)
    {
    case KeyPress:
      msg.message = WM_KEYDOWN;
      msg.wParam |= 0;
      break;
    case KeyRelease:
      msg.message = WM_KEYUP;
      msg.wParam |= (1 << 30) | (1 << 31) ;
      break;
    default:
      return BadValue;
    }
  if (g_fIME)
    {
      WORD pszKey[2];
      BYTE aKeyState[256];
      HKL hkl = GetKeyboardLayout(0);
      int nRet;

      memset(aKeyState, 0, 256*sizeof(BYTE));

      nRet = ToAsciiEx(msg.lParam, stuff->keycode - MIN_KEYCODE, aKeyState, pszKey, 0, hkl);
      winDebug ("ToUnicodeEx return %d %c%c\n", nRet, pszKey[0], pszKey[1]);
      //TranslateMessage(&msg);
    }
#endif

  return (client->noClientException);
}
#endif

static HWND s_hAssociatedWnd = NULL;

static int
ProcWinIMESetFocus (register ClientPtr client)
{
  REQUEST(xWinIMESetFocusReq);
  WIContextPtr pWIC;

  REQUEST_SIZE_MATCH(xWinIMESetFocusReq);

#if CYGIME_DEBUG
  winDebug ("%s %d\n", __FUNCTION__, stuff->context);
#endif

  if (!(pWIC = FindContext(stuff->context)))
    {
      return BadValue;
    }

  if (stuff->focus)
    {
      if (s_hAssociatedWnd)
	{
	  ImmAssociateContext (s_hAssociatedWnd, (HIMC)0);
	}

      s_hAssociatedWnd = GetActiveWindow ();//FIXME

      ImmAssociateContext (s_hAssociatedWnd, pWIC->hIMC);
    }
  else
    {
      /* do nothing */
    }

  return (client->noClientException);
}

static int
ProcWinIMESetCompositionDraw (register ClientPtr client)
{
  REQUEST(xWinIMESetCompositionDrawReq);
  WIContextPtr pWIC;

  REQUEST_SIZE_MATCH(xWinIMESetCompositionDrawReq);

#if CYGIME_DEBUG
  winDebug ("%s %d\n", __FUNCTION__, stuff->context);
#endif

  if (!(pWIC = FindContext(stuff->context)))
    {
      return BadValue;
    }

  pWIC->fCompositionDraw = stuff->draw;

  return (client->noClientException);
}

static int
ProcWinIMEGetCursorPosition(register ClientPtr client)
{
  REQUEST(xWinIMEGetCursorPositionReq);
  WIContextPtr pWIC;
  xWinIMEGetCursorPositionReply rep;

  REQUEST_SIZE_MATCH(xWinIMEGetCursorPositionReq);

#if CYGIME_DEBUG
  winDebug ("%s\n", __FUNCTION__);
#endif

  if (!(pWIC = FindContext(stuff->context)))
    {
      return BadValue;
    }

  REQUEST_SIZE_MATCH(xWinIMEGetCursorPositionReq);
  rep.type = X_Reply;
  rep.length = 0;
  rep.sequenceNumber = client->sequence;
  rep.cursor = pWIC->nCursor;

  WriteToClient(client, sizeof(xWinIMEGetCursorPositionReply), (char *)&rep);
  return (client->noClientException);
}

/*
 * deliver the event
 */

void
winWinIMESendEvent (int type, unsigned int mask, int kind, int arg, int context)
{
  WinIMEEventPtr	*pHead, pEvent;
  ClientPtr		client;
  xWinIMENotifyEvent se;
#if CYGIME_DEBUG
  ErrorF ("%s %d %d %d %d\n",
	  __FUNCTION__, type, mask, kind, arg);
#endif
  dixLookupResourceByType((pointer *)&pHead, eventResource, EventType,
		  NullClient, DixUnknownAccess);
  if (!pHead)
    return;
  for (pEvent = *pHead; pEvent; pEvent = pEvent->next)
    {
      client = pEvent->client;
#if CYGIME_DEBUG
      ErrorF ("winWinIMESendEvent - x%08x\n", (int) client);
#endif
      if ((pEvent->mask & mask) == 0
	  || client == serverClient || client->clientGone)
	{
	  continue;
	}
#if CYGIME_DEBUG 
      ErrorF ("winWinIMESendEvent - send\n");
#endif
      se.type = type + WinIMEEventBase;
      se.kind = kind;
      se.context = context;
      se.arg = arg;
      se.sequenceNumber = client->sequence;
      se.time = currentTime.milliseconds;
      WriteEventsToClient (client, 1, (xEvent *) &se);
    }
}

/* dispatch */

static int
ProcWinIMEDispatch (register ClientPtr client)
{
  REQUEST(xReq);

#if CYGIME_DEBUG
  winDebug ("%s\n", __FUNCTION__);
#endif

  switch (stuff->data)
    {
    case X_WinIMEQueryVersion:
      return ProcWinIMEQueryVersion (client);
    case X_WinIMESelectInput:
      return ProcWinIMESelectInput (client);
    case X_WinIMECreateContext:
      return ProcWinIMECreateContext (client);
    case X_WinIMESetOpenStatus:
      return ProcWinIMESetOpenStatus (client);
    case X_WinIMESetCompositionWindow:
      return ProcWinIMESetCompositionWindow (client);
    case X_WinIMEGetCompositionString:
      return ProcWinIMEGetCompositionString (client);
    case X_WinIMESetFocus:
      return ProcWinIMESetFocus (client);
    case X_WinIMESetCompositionDraw:
      return ProcWinIMESetCompositionDraw (client);
    case X_WinIMEGetCursorPosition:
      return ProcWinIMEGetCursorPosition (client);
    default:
      return BadRequest;
    }
}

static void
SNotifyEvent (xWinIMENotifyEvent *from, xWinIMENotifyEvent *to)
{
#if CYGIME_DEBUG
  winDebug ("%s\n", __FUNCTION__);
#endif

  to->type = from->type;
  to->kind = from->kind;
  cpswaps (from->sequenceNumber, to->sequenceNumber);
  cpswapl (from->context, to->context);
  cpswapl (from->time, to->time);
  cpswapl (from->arg, to->arg);
}

static int
SProcWinIMEQueryVersion (register ClientPtr client)
{
  register int n;
  REQUEST(xWinIMEQueryVersionReq);

#if CYGIME_DEBUG
  winDebug ("%s\n", __FUNCTION__);
#endif

  swaps(&stuff->length, n);
  return ProcWinIMEQueryVersion(client);
}

static int
SProcWinIMEDispatch (register ClientPtr client)
{
  REQUEST(xReq);

#if CYGIME_DEBUG
  winDebug ("%s\n", __FUNCTION__);
#endif

  /* It is bound to be non-local when there is byte swapping */
  if (!ComputeLocalClient(client))
    return WinIMEErrorBase + WinIMEClientNotLocal;

  /* only local clients are allowed access */
  switch (stuff->data)
    {
    case X_WinIMEQueryVersion:
      return SProcWinIMEQueryVersion(client);
    default:
      return BadRequest;
    }
}

LRESULT
winIMEMessageHandler (HWND hwnd, UINT message,
		      WPARAM wParam, LPARAM lParam)
{
  switch (message)
    {
    case WM_IME_NOTIFY:
      ErrorF ("winIMEMessageHandler - WM_IME_NOTIFY\n");
      {
	if (wParam == IMN_SETOPENSTATUS)
	  {
	    HIMC hIMC = ImmGetContext(hwnd);
	    BOOL fStatus = ImmGetOpenStatus(hIMC);

	    winWinIMESendEvent (WinIMEControllerNotify,
				WinIMENotifyMask,
				WinIMEOpenStatus,
				fStatus,
				winHIMCtoContext(hIMC));
	    ImmReleaseContext(hwnd, hIMC);
	  }
      }
      break;

    case WM_IME_STARTCOMPOSITION:
      ErrorF ("winIMEMessageHandler - WM_IME_STARTCOMPOSITION\n");
      {
	HIMC hIMC = ImmGetContext(hwnd);
#if 0
	COMPOSITIONFORM cf;

	if (pWinPriv->dwCompositionStyle)
	  {
	    cf.dwStyle = pWinPriv->dwCompositionStyle;
	    cf.ptCurrentPos.x = pWinPriv->ptCompositionPos.x;
	    cf.ptCurrentPos.y = pWinPriv->ptCompositionPos.y;
	    cf.rcArea.left = pWinPriv->rcCompositionArea.left;
	    cf.rcArea.top = pWinPriv->rcCompositionArea.top;
	    cf.rcArea.right = pWinPriv->rcCompositionArea.right;
	    cf.rcArea.bottom = pWinPriv->rcCompositionArea.bottom;
	    ImmSetCompositionWindow(hIMC, &cf);
	  }
#endif
	winWinIMESendEvent (WinIMEControllerNotify,
			    WinIMENotifyMask,
			    WinIMEStartComposition,
			    0,
			    winHIMCtoContext(hIMC));
	ImmReleaseContext(hwnd, hIMC);
	if (!winHIMCCompositionDraw(hIMC)) return 0;
      }
      break;

    case WM_IME_COMPOSITION:
      ErrorF ("winIMEMessageHandler - WM_IME_STARTCOMPOSITION\n");
      {
	HIMC hIMC = ImmGetContext(hwnd);

	if (lParam & GCS_COMPSTR)
	  {
	    int			nUnicodeSize = 0;
	    wchar_t		*pwszUnicodeStr = NULL;
	    char		*pszUTF8 = NULL;
	    int			nUTF8;
	    int			nLen;

	    /* Get result */
	    nUnicodeSize = ImmGetCompositionStringW(hIMC, GCS_COMPSTR, NULL, 0);
	    pwszUnicodeStr = (wchar_t*) malloc (nUnicodeSize);
	    ImmGetCompositionStringW(hIMC, GCS_COMPSTR, pwszUnicodeStr, nUnicodeSize);

	    /* Convert to UTF8 */
	    nLen = WideCharToMultiByte (CP_UTF8,
                                 0,
                                 (LPCWSTR)pwszUnicodeStr,
                                 nUnicodeSize/2,
                                 NULL,
                                 0,
                                 NULL,
                                 NULL);
	    pszUTF8 = (char *) malloc (nLen+1);
	    WideCharToMultiByte (CP_UTF8,
                         0,
                         (LPCWSTR)pwszUnicodeStr,
                         nUnicodeSize/2,
                         pszUTF8,
                         nLen,
                         NULL,
                         NULL);
            pszUTF8[nLen] = '\0';
	    free (pwszUnicodeStr);

	    winCommitCompositionResult (winHIMCtoContext(hIMC), GCS_COMPSTR, pszUTF8, nLen);

	    winWinIMESendEvent (WinIMEControllerNotify,
				WinIMENotifyMask,
				WinIMEComposition,
				WinIMECMPCompStr,
				winHIMCtoContext(hIMC));
	  }

	if (lParam & GCS_CURSORPOS)
	  {
	    int nCursor = ImmGetCompositionStringW(hIMC, GCS_CURSORPOS, NULL, 0);

	    winCommitCompositionResult (winHIMCtoContext(hIMC), GCS_CURSORPOS, (int*)&nCursor, 0);

	    winWinIMESendEvent (WinIMEControllerNotify,
				WinIMENotifyMask,
				WinIMEComposition,
				WinIMECMPCursorPos,
				winHIMCtoContext(hIMC));
	  }

	if (lParam & GCS_RESULTSTR)
	  {
	    int			nUnicodeSize = 0;
	    wchar_t		*pwszUnicodeStr = NULL;
	    char		*pszUTF8 = NULL;
	    int			nUTF8;
	    int			nLen;

	    /* Get result */
	    nUnicodeSize = ImmGetCompositionStringW(hIMC, GCS_RESULTSTR, NULL, 0);
	    pwszUnicodeStr = (wchar_t*) malloc (nUnicodeSize);
	    ImmGetCompositionStringW(hIMC, GCS_RESULTSTR, pwszUnicodeStr, nUnicodeSize);

	    /* Convert to UTF8 */
            nLen = WideCharToMultiByte (CP_UTF8,
                                 0,
                                 (LPCWSTR)pwszUnicodeStr,
                                 nUnicodeSize/2,
                                 NULL,
                                 0,
                                 NULL,
                                 NULL);
            pszUTF8 = (char *) malloc (nLen+1);
            WideCharToMultiByte (CP_UTF8,
                         0,
                         (LPCWSTR)pwszUnicodeStr,
                         nUnicodeSize/2,
                         pszUTF8,
                         nLen,
                         NULL,
                         NULL);
            pszUTF8[nLen] = '\0';
	    free (pwszUnicodeStr);

	    winCommitCompositionResult (winHIMCtoContext(hIMC), GCS_RESULTSTR, pszUTF8, nLen);

	    winWinIMESendEvent (WinIMEControllerNotify,
				WinIMENotifyMask,
				WinIMEComposition,
				WinIMECMPResultStr,
				winHIMCtoContext(hIMC));
	  }


	if (lParam & GCS_COMPATTR)
	  {
	    int			nLen = 0;
	    char		*pAttr = NULL;

	    /* Get result */
	    nLen = ImmGetCompositionStringW(hIMC, GCS_COMPATTR, NULL, 0);
	    pAttr = (char*) malloc (nLen);
	    ImmGetCompositionStringW(hIMC, GCS_COMPATTR, pAttr, nLen);

	    winCommitCompositionResult (winHIMCtoContext(hIMC), GCS_COMPATTR, pAttr, nLen);

	    winWinIMESendEvent (WinIMEControllerNotify,
				WinIMENotifyMask,
				WinIMEComposition,
				WinIMECMPCompAttr,
				winHIMCtoContext(hIMC));
	  }

	ImmReleaseContext(hwnd, hIMC);
	if (!winHIMCCompositionDraw(hIMC)) return 0;
      }
      break;

    case WM_IME_ENDCOMPOSITION:
      ErrorF ("winIMEMessageHandler - WM_IME_ENDCOMPOSITION\n");
      {
	HIMC hIMC = ImmGetContext(hwnd);
	winWinIMESendEvent (WinIMEControllerNotify,
			    WinIMENotifyMask,
			    WinIMEEndComposition,
			    0,
			    winHIMCtoContext(hIMC));
	ImmReleaseContext(hwnd, hIMC);
	if (!winHIMCCompositionDraw(hIMC)) return 0;
      }
      break;

    case WM_IME_CHAR:
      ErrorF ("winIMEMessageHandler - WM_IME_CHAR\n");
      break;

    case WM_CHAR:
      ErrorF ("winIMEMessageHandler - WM_CHAR\n");
      break;
    }
  return DefWindowProc (hwnd, message, wParam, lParam);
}
