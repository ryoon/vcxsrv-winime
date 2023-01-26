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
#include <stdio.h>
#include <string.h>
#include <X11/Xlocale.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include "IMdkit/IMdkit.h"
#include "IMdkit/Xi18n.h"
#include <X11/extensions/winime.h>
#include <pthread.h>
#include <winmsg.h>

/* Windows headers */
#define NONAMELESSUNION
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif

#define ATOM DWORD
#include <X11/Xwindows.h>

#include "winic.h"

#define DEFAULT_IMNAME "XIME"
#define DEFAULT_LOCALE "ja_JP,ko_KR,zh_CN,zh_HK,zh_SG,zh_TW"
#define STRING_BUFFER_SIZE 1024
#define WIN_CONNECT_RETRIES			40
#define WIN_CONNECT_DELAY			4

/* Supported Inputstyles */
static XIMStyle Styles[] = {
  XIMPreeditCallbacks | XIMStatusArea,
  XIMPreeditCallbacks | XIMStatusNothing,
  XIMPreeditCallbacks | XIMStatusNone,
  XIMPreeditPosition | XIMStatusArea,
  XIMPreeditPosition | XIMStatusNothing,
  XIMPreeditPosition | XIMStatusNone,
  XIMPreeditArea | XIMStatusArea,
  XIMPreeditNothing | XIMStatusNothing,
  XIMPreeditNothing | XIMStatusNone,
  0
};

/* Supported Japanese Encodings */
static XIMEncoding jaEncodings[] = {
  "COMPOUND_TEXT",
  NULL
};

pthread_t		g_ptImServerProc;


/*
 * References to external symbols
 */

extern Bool			g_fIME;
extern char *display;
extern void ErrorF (const char* /*f*/, ...);
extern void winDebug (const char *format, ...);
extern void winErrorFVerb (int verb, const char *format, ...);

#define GET_DISPLAY(pIms) ((Xi18n)(pIms)->protocol)->address.dpy

static Bool
winXIMEGetICValuesHandler (XIMS pIms, IMChangeICStruct * pCallData)
{
  winDebug ("%s\n", __FUNCTION__);

  GetIMIC (pCallData);
  return True;
}

static Bool
winXIMESetICValuesHandler (XIMS pIms, IMChangeICStruct * pCallData)
{
  winDebug ("%s\n", __FUNCTION__);

  SetIMIC (pCallData);
  return True;
}

static Bool
winXIMEOpenHandler (XIMS pIms, IMOpenStruct * pCallData)
{
  winDebug ("%s\n", __FUNCTION__);

  winDebug ("new_client lang is %s\n", pCallData->lang.name);
  return True;
}

static Bool
winXIMECreateICHandler (XIMS pIms, IMChangeICStruct * pCallData)
{
  winDebug ("%s\n", __FUNCTION__);

  CreateIMIC (GET_DISPLAY (pIms), pCallData);
  return True;
}

static Bool
winXIMEForwardEventHandler (XIMS pIms, IMForwardEventStruct * pCallData)
{
  IMIC *pIC;

  winDebug ("%s\n", __FUNCTION__);

  /* Filtered toggle ime key like Alt+VK_KANJI */
  pIC = FindIMIC (pCallData->icid);
  if (pIC && pIC->toggled)
    {
      pIC->toggled = False;
      return True;
    }

  /* Events for IME is filtered by XWin, so forward all event. */
  IMForwardEvent (pIms, (XPointer) pCallData);

  return True;
}

static Bool
winXIMESetICFocusHandler (XIMS pIms, IMChangeFocusStruct * pCallData)
{
  IMIC *pIC;

  winDebug ("%s\n", __FUNCTION__);

  pIC = FindIMIC (pCallData->icid);
  XWinIMESetFocus (GET_DISPLAY (pIms), pIC->context, True);
  return True;
}

static Bool
winXIMEUnsetICFocusHandler (XIMS pIms, IMChangeFocusStruct * pCallData)
{
  IMIC *pIC;

  winDebug ("%s\n", __FUNCTION__);

  pIC = FindIMIC (pCallData->icid);
  XWinIMESetFocus (GET_DISPLAY (pIms), pIC->context, False);
  return True;
}

static Bool
winXIMETriggerNotifyHandler (XIMS pIms, IMTriggerNotifyStruct * pCallData)
{
  winDebug ("%s\n", __FUNCTION__);

  /* never happens */
  return False;
}

static Bool
winXIMEPreeditStartReplyHandler (XIMS pIms, IMPreeditCBStruct * pCallData)
{
  winDebug ("%s\n", __FUNCTION__);

  return True;
}

static Bool
winXIMEPreeditCaretReplyHandler (XIMS pIms, IMPreeditCBStruct * pCallData)
{
  winDebug ("%s\n", __FUNCTION__);

  return True;
}

static Bool
winXIMEMoveStructHandler (XIMS pIms, IMMoveStruct * pCallData)
{
  winDebug ("%s\n", __FUNCTION__);

  return True;
}

static Bool
winXIMEProtoHandler (XIMS pIms, IMProtocol * pCallData)
{
  winDebug ("%s\n", __FUNCTION__);

  switch (pCallData->major_code)
    {
    case XIM_OPEN:
      winXIMEOpenHandler (pIms, (IMOpenStruct *) pCallData);
      break;

    case XIM_CREATE_IC:
      winXIMECreateICHandler (pIms, (IMChangeICStruct *) pCallData);
      break;

    case XIM_DESTROY_IC:
      break;

    case XIM_SET_IC_VALUES:
      winXIMESetICValuesHandler (pIms, (IMChangeICStruct *) pCallData);
      break;

    case XIM_GET_IC_VALUES:
      winXIMEGetICValuesHandler (pIms, (IMChangeICStruct *) pCallData);
      break;

    case XIM_FORWARD_EVENT:
      winXIMEForwardEventHandler (pIms, (IMForwardEventStruct *) pCallData);
      break;

    case XIM_SET_IC_FOCUS:
      winXIMESetICFocusHandler (pIms, (IMChangeFocusStruct *) pCallData);
      break;

    case XIM_UNSET_IC_FOCUS:
      winXIMEUnsetICFocusHandler (pIms, (IMChangeFocusStruct *) pCallData);
      break;

    case XIM_RESET_IC:
      break;

    case XIM_TRIGGER_NOTIFY:
      winXIMETriggerNotifyHandler (pIms, (IMTriggerNotifyStruct *) pCallData);
      break;

    case XIM_PREEDIT_START_REPLY:
      winXIMEPreeditStartReplyHandler (pIms, (IMPreeditCBStruct *) pCallData);
      break;

    case XIM_PREEDIT_CARET_REPLY:
      winXIMEPreeditCaretReplyHandler (pIms, (IMPreeditCBStruct *) pCallData);
      break;

    case XIM_EXT_MOVE:
      winXIMEMoveStructHandler (pIms, (IMMoveStruct *) pCallData);
      break;
    }
  return True;
}

static void
winXIMEXEventHandler (Display * pDisplay, Window im_window, XEvent * event, XIMS pIms,
		 int ime_event_base, int ime_error_base)
{
  int i;

  winDebug ("%s\n", __FUNCTION__);

  switch (event->type)
    {
    case DestroyNotify:
      break;

    case ButtonPress:
      switch (event->xbutton.button)
	{
	case Button3:
	  if (event->xbutton.window == im_window)
	    goto Exit;
	  break;
	}
      break;

    default:
      if (event->type == ime_event_base + WinIMEControllerNotify)
	{
	  XWinIMENotifyEvent *ime_event = (XWinIMENotifyEvent *) event;
	  IMIC *pIC;

	  pIC = FindIMICbyContext (ime_event->context);

	  switch (ime_event->kind)
	    {
	    case WinIMEOpenStatus:
	      {
		winDebug ("WinIMEOpenStatus %d %s\n", ime_event->context,
			ime_event->arg ? "Open" : "Close");

		if (!pIC)
		  {
		    winDebug ("context %d IC is not found", ime_event->context);
		    break;
		  }

		pIC->toggled = True;

		if (ime_event->arg)
		  {
		    IMPreeditStart (pIms, (XPointer) & pIC->pCallData);
		  }
		else
		  {
		    IMPreeditStart (pIms, (XPointer) & pIC->pCallData);
		  }
	      }
	      break;

	    case WinIMEComposition:
	      {
		winDebug ("WinIMEComposition %d %d\n", ime_event->context,
			ime_event->arg);

		if (!pIC)
		  {
		    winDebug ("context %d IC is not found", ime_event->context);
		    break;
		  }

		switch (ime_event->arg)
		  {
		  case WinIMECMPCompStr:
		    {
		      char szCompositionString[STRING_BUFFER_SIZE];
		      int iCursor;
		      int iLen;
		      char szCompositionAttr[STRING_BUFFER_SIZE];
		      XIMFeedback aFeedback[STRING_BUFFER_SIZE] =
			{ XIMUnderline };
		      int iAttrLen;

		      iLen =
			XWinIMEGetCompositionString (pDisplay, ime_event->context,
						     WinIMECMPCompStr,
						     STRING_BUFFER_SIZE,
						     szCompositionString);
		      if (iLen < 0)
			{
			  winDebug ("XWinIMEGetCompositionString failed.\n");
			  break;
			}
		      szCompositionString[iLen] = '\0';

		      iAttrLen =
			XWinIMEGetCompositionString (pDisplay, ime_event->context,
						     WinIMECMPCompAttr,
						     STRING_BUFFER_SIZE,
						     szCompositionAttr);
		      if (iAttrLen < 0)
			{
			  winDebug ("XWinIMEGetCompositionString failed.\n");
			}

		      winDebug ("attribute \n");
		      for (i = 0; i < iAttrLen; i++)
			{
			  switch (szCompositionAttr[i])
			    {
			    case WinIMEAttrInput:
			      winDebug ("[input]");
			      aFeedback[i] = XIMUnderline;
			      break;

			    case WinIMEAttrTargetConverted:
			      winDebug ("[target converted]");
			      aFeedback[i] = XIMUnderline | XIMReverse;
			      break;

			    case WinIMEAttrConverted:
			      winDebug ("[converted]");
			      aFeedback[i] = XIMPrimary;
			      break;

			    case WinIMEAttrTargetNotconverted:
			      winDebug ("[target not converted]");
			      aFeedback[i] = XIMReverse;
			      break;

			    case WinIMEAttrInputError:
			      winDebug ("[error]");
			      aFeedback[i] = XIMUnderline;
			      break;

			    case WinIMEAttrFixedconverted:
			      winDebug ("[fixed converted]");
			      aFeedback[i] = XIMUnderline;
			      break;

			    default:
			      winDebug ("[%d]", (int) szCompositionAttr[i]);
			    }
			}
		      winDebug ("\n");

		      if (!XWinIMEGetCursorPosition (pDisplay, ime_event->context,
						     &iCursor))
			{
			  winDebug ("XWinIMEGetCursorPosition failed.\n");
			}
		      pIC->caret = iCursor;

		      if (strlen (szCompositionString) > 0)
			{
			  XTextProperty tp;
			  XIMText text;
			  char *pszStr = szCompositionString;

			  iLen = MultiByteToWideChar (CP_UTF8, 0,
						      (LPCSTR)
						      szCompositionString,
						      strlen (pszStr),
						      NULL, 0);

			  winDebug ("compstr: %s\n", pszStr);
			  //setlocale (LC_CTYPE, "");
			  Xutf8TextListToTextProperty (pDisplay,
						       (char **) &pszStr,
						       1,
						       XCompoundTextStyle,
						       &tp);

			  text.length = tp.nitems;
			  text.string.multi_byte = tp.value;
			  //for (i = 0; i < text.length; i ++) feedback[i] = XIMUnderline;
			  aFeedback[iLen] = 0;
			  text.feedback = aFeedback;

			  pIC->pCallData.major_code = XIM_PREEDIT_DRAW;
			  pIC->pCallData.preedit_callback.icid = pIC->id;
			  pIC->pCallData.preedit_callback.todo.draw.caret =
			    pIC->caret;
			  pIC->pCallData.preedit_callback.todo.draw.
			    chg_first = 0;
			  pIC->pCallData.preedit_callback.todo.draw.
			    chg_length = pIC->length;
			  pIC->pCallData.preedit_callback.todo.draw.text =
			    &text;

			  pIC->length = iLen;

			  winDebug ("%d\n", pIC->length);

			  IMCallCallback (pIms, (XPointer) & pIC->pCallData);
			  szCompositionString[0] = '\0';
			}
		    }
		    break;

		  case WinIMECMPResultStr:
		    {
		      XIMFeedback aFeedback[1] = { 0 };
		      char szResult[STRING_BUFFER_SIZE];
		      int iLen =
			XWinIMEGetCompositionString (pDisplay, ime_event->context,
						     WinIMECMPResultStr,
						     STRING_BUFFER_SIZE,
						     szResult);
		      if (iLen < 0)
			{
			  winDebug ("XWinIMEGetCompositionString failed.\n");
			  break;
			}
		      szResult[iLen] = '\0';

		      if (strlen (szResult) > 0)
			{
			  XTextProperty tp;
			  XIMText text;
			  char *pszStr = szResult;

			  winDebug ("commit result: %s\n", pszStr);
			  //setlocale (LC_CTYPE, "");
			  Xutf8TextListToTextProperty (pDisplay,
						       (char **) &pszStr, 1,
						       XCompoundTextStyle,
						       &tp);

			  pIC->pCallData.commitstring.flag = XimLookupChars;
			  pIC->pCallData.commitstring.commit_string =
			    (char *) tp.value;
			  pIC->pCallData.commitstring.icid = pIC->id;

			  IMCommitString (pIms, (XPointer) & pIC->pCallData);

			  text.length = 0;
			  text.string.multi_byte = "";
			  text.feedback = aFeedback;

			  /* Clear preedit. */
			  pIC->pCallData.major_code = XIM_PREEDIT_DRAW;
			  pIC->pCallData.preedit_callback.icid = pIC->id;
			  pIC->pCallData.preedit_callback.todo.draw.caret = 0;
			  pIC->pCallData.preedit_callback.todo.draw.
			    chg_first = 0;
			  pIC->pCallData.preedit_callback.todo.draw.
			    chg_length = pIC->length;
			  pIC->pCallData.preedit_callback.todo.draw.text =
			    &text;

			  pIC->length = 0;

			  IMCallCallback (pIms, (XPointer) & pIC->pCallData);
			}
		    }
		    break;

		  default:
		    break;
		  }
	      }
	      break;

	    case WinIMEStartComposition:
	      {
		winDebug ("WinIMEStartComposition %d\n", ime_event->context);

		if (!pIC)
		  {
		    winDebug ("context %d IC is not found", ime_event->context);
		    break;
		  }

		pIC->pCallData.major_code = XIM_PREEDIT_START;
		IMCallCallback (pIms, (XPointer) & pIC->pCallData);
	      }
	      break;

	    case WinIMEEndComposition:
	      {
		winDebug ("WinIMEEndComposition %d\n", ime_event->context);

		if (!pIC)
		  {
		    winDebug ("context %d IC is not found", ime_event->context);
		    break;
		  }


		if (pIC->length > 0)
		  {
		    XIMText text;
		    XIMFeedback aFeedback[1] = { 0 };

		    text.length = 0;
		    text.string.multi_byte = "";
		    text.feedback = aFeedback;

		    /* Clear preedit. */
		    pIC->pCallData.major_code = XIM_PREEDIT_DRAW;
		    pIC->pCallData.preedit_callback.icid = pIC->id;
		    pIC->pCallData.preedit_callback.todo.draw.caret = 0;
		    pIC->pCallData.preedit_callback.todo.draw.chg_first = 0;
		    pIC->pCallData.preedit_callback.todo.draw.chg_length =
		      pIC->length;
		    pIC->pCallData.preedit_callback.todo.draw.text = &text;

		    pIC->length = 0;

		    IMCallCallback (pIms, (XPointer) & pIC->pCallData);
		  }

		pIC->pCallData.major_code = XIM_PREEDIT_DONE;
		IMCallCallback (pIms, (XPointer) & pIC->pCallData);
	      }
	      break;

	    default:
	      break;
	    }
	}
      break;
    }
  return;
Exit:
  XDestroyWindow (event->xbutton.display, im_window);
  pthread_exit (NULL);
}

static int
winImServerErrorHandler (Display * pDisplay, XErrorEvent * pErr)
{
  char pszErrorMsg[100];

  XGetErrorText (pDisplay,
		 pErr->error_code,
		 pszErrorMsg,
		 sizeof (pszErrorMsg));
  ErrorF ("winImServerErrorHandler - ERROR: \n\t%s\n"
	  "\tSerial: %d, Request Code: %d, Minor Code: %d\n",
	  pszErrorMsg,
	  pErr->serial,
	  pErr->request_code,
	  pErr->minor_code);

  return 0;
}

static int
winImServerIOErrorHandler (Display *pDisplay)
{
  ErrorF ("\nwinClipboardIOErrorHandler!\n\n");

  pthread_exit (NULL);
  return 0;
}

static void*
winImServerProc (void *pvNotUsed)
{
  char szDisplay[512];
  char *pszIMName = NULL;
  char *pszLocale = NULL;
  XIMS pIms;
  XIMStyles *pInputStyles;
  XIMEncodings *pEncodings;
  Window pIMWindow;
  Display *pDisplay;
  int i;
  int iRetries = 0;
  char szTransport[80];		/* enough */
  long lFilterMask = KeyPressMask | KeyReleaseMask;
  int iIMEEventBase, iIMEErrorBase;


  /* Allow multiple threads to access Xlib */
  if (XInitThreads () == 0)
    {
      ErrorF ("winClipboardProc - XInitThreads failed.\n");
      pthread_exit (NULL);
    }

  /* See if X supports the current locale */
  if (XSupportsLocale () == False)
    {
      ErrorF ("winImServerProc - Locale not supported by X.  Exiting.\n");
      pthread_exit (NULL);
    }

  /* Set error handler */
  XSetErrorHandler (winImServerErrorHandler);
  XSetIOErrorHandler (winImServerIOErrorHandler);

  if (!pszIMName)
    pszIMName = DEFAULT_IMNAME;

  if (!pszLocale)
    pszLocale = DEFAULT_LOCALE;

  /* Setup the display connection string x */
  /*
   * NOTE: Always connect to screen 0 since we require that screen
   * numbers start at 0 and increase without gaps.  We only need
   * to connect to one screen on the display to get events
   * for all screens on the display.  That is why there is only
   * one clipboard client thread.
   */
  snprintf (szDisplay,
	    512,
	    "127.0.0.1:%s.0",
	    display);

  /* Print the display connection string */
  ErrorF ("winImServerProc - DISPLAY=%s\n", szDisplay);

  /* Open the X display */
  do
    {
      pDisplay = XOpenDisplay (szDisplay);
      if (pDisplay == NULL)
	{
	  ErrorF ("winImServerProc - Could not open display, "
		  "try: %d, sleeping: %d\n",
		  iRetries + 1, WIN_CONNECT_DELAY);
	  ++iRetries;
	  sleep (WIN_CONNECT_DELAY);
	  continue;
	}
      else
	break;
    }
  while (pDisplay == NULL && iRetries < WIN_CONNECT_RETRIES);

  /* Make sure that the display opened */
  if (pDisplay == NULL)
    {
      ErrorF ("winImServerProc - Failed opening the display, giving up\n");
      pthread_exit (NULL);
    }

  winDebug ("imname=%s locale=%s\n", pszIMName, pszLocale);

  if (!XWinIMEQueryExtension (pDisplay, &iIMEEventBase, &iIMEErrorBase))
    {
      ErrorF ("winImServerProc - No IME Extension\n");
      pthread_exit (NULL);
    }

  XWinIMESelectInput (pDisplay, WinIMENotifyMask);

  pIMWindow = XCreateSimpleWindow (pDisplay, DefaultRootWindow (pDisplay),
				   0, 0, 1, 1, 1, 0, 0);
  if (pIMWindow == (Window) NULL)
    {
      ErrorF ("winImServerProc - Can't Create Window\n");
      pthread_exit (NULL);
    }
  XStoreName (pDisplay, pIMWindow, "Windows IME Input Method Server");

  if ((pInputStyles = (XIMStyles *) malloc (sizeof (XIMStyles))) == NULL)
    {
      ErrorF ("winImServerProc - Can't allocate\n");
      pthread_exit (NULL);
    }
  pInputStyles->count_styles = sizeof (Styles) / sizeof (XIMStyle) - 1;
  pInputStyles->supported_styles = Styles;

  if ((pEncodings = (XIMEncodings *) malloc (sizeof (XIMEncodings))) == NULL)
    {
      ErrorF ("winImServerProc - Can't allocate\n");
      pthread_exit (NULL);
    }
  pEncodings->count_encodings =
    sizeof (jaEncodings) / sizeof (XIMEncoding) - 1;
  pEncodings->supported_encodings = jaEncodings;

  strcpy (szTransport, "X/");

  pIms = IMOpenIM (pDisplay,
		   IMModifiers, "Xi18n",
		   IMServerWindow, pIMWindow,
		   IMServerName, pszIMName,
		   IMLocale, pszLocale,
		   IMServerTransport, szTransport,
		   IMInputStyles, pInputStyles, NULL);
  if (pIms == (XIMS) NULL)
    {
      ErrorF ("winImServerProc - Can't Open Input Method Service:\n"
	      "\tInput Method Name :%s\n"
	      "\tTranport Address:%s\n",
	      pszIMName, szTransport);
      pthread_exit (NULL);
    }
  IMSetIMValues (pIms,
		 IMEncodingList, pEncodings,
		 IMProtocolHandler, winXIMEProtoHandler,
		 IMFilterEventMask, lFilterMask, NULL);

  XSelectInput (pDisplay, pIMWindow, StructureNotifyMask | ButtonPressMask);
  //XMapWindow (pDisplay, pIMWindow);
  XFlush (pDisplay);			/* necessary flush for tcp/ip connection */

  for (;;)
    {
      XEvent event;
      XNextEvent (pDisplay, &event);
      if (XFilterEvent (&event, None) == True)
	continue;
      winXIMEXEventHandler (pDisplay, pIMWindow, &event, pIms, iIMEEventBase,
			    iIMEErrorBase);
    }
  pthread_exit (NULL);
}


/*
 * Intialize the Clipboard module
 */

Bool
winInitImServer ()
{
  ErrorF ("winInitImServer ()\n");

  /* Spawn a thread for the Clipboard module */
  if (pthread_create (&g_ptImServerProc,
		      NULL,
		      winImServerProc,
		      NULL))
    {
      /* Bail if thread creation failed */
      ErrorF ("winInitImServer - pthread_create failed.\n");
      return FALSE;
    }

  return TRUE;
}
