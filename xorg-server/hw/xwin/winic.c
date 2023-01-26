/******************************************************************

         Copyright 1993, 1994 by Hewlett-Packard Company

Permission to use, copy, modify, distribute, and sell this software
and its documentation for any purpose without fee is hereby granted,
provided that the above copyright notice appear in all copies and
that both that copyright notice and this permission notice appear
in supporting documentation, and that the name of Hewlett-Packard not
be used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.
Hewlett-Packard Company makes no representations about the suitability
of this software for any purpose.
It is provided "as is" without express or implied warranty.

HEWLETT-PACKARD COMPANY DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS,
IN NO EVENT SHALL HEWLETT-PACKARD COMPANY BE LIABLE FOR ANY SPECIAL,
INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.

Author:
    Hidetoshi Tajima	Hewlett-Packard Company.
			(tajima@kobe.hp.com)
    Kensuke Matsuzaki   (zakki@peppermint.jp)
******************************************************************/

#include <X11/Xlib.h>
#include <X11/Xos.h>
#include <X11/extensions/winime.h>
#include <stdio.h>
#include "IMdkit/IMdkit.h"
#include "IMdkit/Xi18n.h"
#include "winic.h"
#include <winmsg.h>

static IMIC *ic_list = (IMIC *) NULL;
static IMIC *free_list = (IMIC *) NULL;

/*
 * References to external symbols
 */

extern Bool			g_fIME;
extern char *display;
extern void ErrorF (const char* /*f*/, ...);
extern void winDebug (const char *format, ...);
extern void winErrorFVerb (int verb, const char *format, ...);

static IMIC *
NewIMIC (Display * dpy)
{
  static CARD16 icid = 0;
  IMIC *rec;

  if (free_list != NULL)
    {
      rec = free_list;
      free_list = free_list->next;
    }
  else
    {
      rec = (IMIC *) malloc (sizeof (IMIC));
    }
  memset (rec, 0, sizeof (IMIC));
  rec->id = ++icid;
  rec->dpy = dpy;

  XWinIMECreateContext (dpy, &rec->context);
  winDebug ("NewIMIC - %d\n", rec->context);

  rec->next = ic_list;
  ic_list = rec;
  return rec;
}

static Window
TopLevelWindow (Display * dpy, Window win)
{
  Window w, root, parent, *children;
  int nchildren;

  winDebug ("%s\n", __FUNCTION__);
  parent = win;

  do
    {
      w = parent;
      if (!XQueryTree (dpy, w, &root, &parent, &children, &nchildren))
	return None;

      if (!children)
	XFree (children);
    }
  while (root != parent);

  return w;
}

static void
StoreIMIC (IMIC *rec, IMChangeICStruct *pCallData)
{
  XICAttribute *ic_attr = pCallData->ic_attr;
  XICAttribute *pre_attr = pCallData->preedit_attr;
  XICAttribute *sts_attr = pCallData->status_attr;
  register int i;

  for (i = 0; i < (int) pCallData->ic_attr_num; i++, ic_attr++)
    {
      winDebug ("StoreIMIC.ic: %s\n", pre_attr->name);
      if (!strcmp (XNInputStyle, ic_attr->name))
	{
	  winDebug ("XNInputStyle");
	  rec->input_style = *(INT32 *) ic_attr->value;
	  switch (rec->
		  input_style & (XIMPreeditCallbacks | XIMPreeditPosition |
				 XIMPreeditArea | XIMPreeditNothing))
	    {
	    case XIMPreeditCallbacks:
	      winDebug ("XIMPreeditCallbacks:\n");
	      XWinIMESetCompositionDraw (rec->dpy, rec->context, False);
	      break;
	    case XIMPreeditPosition:
	      winDebug ("XIMPreeditPosition:\n");
	      XWinIMESetCompositionDraw (rec->dpy, rec->context, True);
	      break;
	    case XIMPreeditArea:
	      winDebug ("XIMPreeditArea:\n");
	      XWinIMESetCompositionDraw (rec->dpy, rec->context, True);
	      break;
	    case XIMPreeditNothing:
	      winDebug ("XIMPreeditNothing:\n");
	      XWinIMESetCompositionDraw (rec->dpy, rec->context, True);
	      XWinIMESetCompositionWindow (rec->dpy, rec->context,
					   WinIMECSDefault, 0, 0, 0, 0);
	      break;
	    default:
	      winDebug ("No preedit style:\n");
	      break;
	    }
	  switch (rec->
		  input_style & (XIMStatusCallbacks | XIMStatusArea |
				 XIMStatusNothing | XIMStatusNone))
	    {
	    case XIMStatusCallbacks:
	      winDebug ("XIMStatusCallbacks\n");
	      break;
	    case XIMStatusArea:
	      winDebug ("XIMStatusArea\n");
	      break;
	    case XIMStatusNothing:
	      winDebug ("XIMStatusNothing\n");
	      break;
	    case XIMStatusNone:
	      winDebug ("XIMStatusNone\n");
	      break;
	    default:
	      winDebug ("No status style\n");
	      break;
	    }
	}
      else if (!strcmp (XNClientWindow, ic_attr->name))
	rec->client_win = *(Window *) ic_attr->value;
      else if (!strcmp (XNFocusWindow, ic_attr->name))
	rec->focus_win = *(Window *) ic_attr->value;
    }
  for (i = 0; i < (int) pCallData->preedit_attr_num; i++, pre_attr++)
    {
      winDebug ("StoreIMIC.preedit: %s\n", pre_attr->name);
      if (!strcmp (XNArea, pre_attr->name))
	{
	  int x, y, w, h;
	  Window child;

	  rec->pre_attr.area = *(XRectangle *) pre_attr->value;
	  winDebug ("StoreIMIC: XArea(%d, %d, %d, %d)\n",
		    rec->pre_attr.area.x, rec->pre_attr.area.y,
		    rec->pre_attr.area.width, rec->pre_attr.area.height);
	  x = rec->pre_attr.area.x;
	  y = rec->pre_attr.area.y;
	  w = rec->pre_attr.area.width;
	  h = rec->pre_attr.area.height;

	  if (rec->focus_win)
	    {
	      Window top = TopLevelWindow (rec->dpy, rec->focus_win);
	      if (top != None)
		{
		  XTranslateCoordinates (rec->dpy, rec->focus_win,
					 top, x, y, &x, &y, &child);
		  XTranslateCoordinates (rec->dpy, rec->focus_win,
					 top, w, h, &w, &h, &child);
		  winDebug ("(%d, %d) to (%d %d)\n", rec->pre_attr.area.x,
			    rec->pre_attr.area.y, x, y);
		}
	      else
		{
		  winDebug ("failed. use (%d, %d)\n", x, y);
		}
	    }

	  XWinIMESetCompositionWindow (rec->dpy, rec->context,
				       WinIMECSRect, x, y, w, h);
	}
      else if (!strcmp (XNAreaNeeded, pre_attr->name))
	{
	  rec->pre_attr.area_needed = *(XRectangle *) pre_attr->value;
	}
      else if (!strcmp (XNSpotLocation, pre_attr->name))
	{
	  int x, y;
	  Window child;

	  rec->pre_attr.spot_location = *(XPoint *) pre_attr->value;
	  winDebug ("StoreIMIC: XNSpotLocation(%d,%d)\n",
		    rec->pre_attr.spot_location.x,
		    rec->pre_attr.spot_location.y);

	  x = rec->pre_attr.spot_location.x;
	  y = rec->pre_attr.spot_location.y;

	  if (rec->focus_win)
	    {
	      Window top = TopLevelWindow (rec->dpy, rec->focus_win);
	      if (top != None)
		{
		  XTranslateCoordinates (rec->dpy, rec->focus_win,
					 top, x, y, &x, &y, &child);
		  winDebug ("(%d, %d) to (%d %d)\n",
			    rec->pre_attr.spot_location.x,
			    rec->pre_attr.spot_location.y, x, y);
		}
	      else
		{
		  winDebug ("failed. use (%d, %d)\n", x, y);
		}
	    }
	  XWinIMESetCompositionWindow (rec->dpy, rec->context,
				       WinIMECSPoint, x, y, 0, 0);
	}
      else if (!strcmp (XNColormap, pre_attr->name))
	rec->pre_attr.cmap = *(Colormap *) pre_attr->value;
      else if (!strcmp (XNStdColormap, pre_attr->name))
	rec->pre_attr.cmap = *(Colormap *) pre_attr->value;
      else if (!strcmp (XNForeground, pre_attr->name))
	rec->pre_attr.foreground = *(CARD32 *) pre_attr->value;
      else if (!strcmp (XNBackground, pre_attr->name))
	rec->pre_attr.background = *(CARD32 *) pre_attr->value;
      else if (!strcmp (XNBackgroundPixmap, pre_attr->name))
	rec->pre_attr.bg_pixmap = *(Pixmap *) pre_attr->value;
      else if (!strcmp (XNFontSet, pre_attr->name))
	{
	  int str_length = strlen (pre_attr->value);
	  if (rec->pre_attr.base_font != NULL)
	    {
	      if (strcmp (rec->pre_attr.base_font, pre_attr->value))
		{
		  XFree (rec->pre_attr.base_font);
		}
	      else
		{
		  continue;
		}
	    }
	  rec->pre_attr.base_font = malloc (str_length + 1);
	  strcpy (rec->pre_attr.base_font, pre_attr->value);
	}
      else if (!strcmp (XNLineSpace, pre_attr->name))
	rec->pre_attr.line_space = *(CARD32 *) pre_attr->value;
      else if (!strcmp (XNCursor, pre_attr->name))
	rec->pre_attr.cursor = *(Cursor *) pre_attr->value;
    }
  for (i = 0; i < (int) pCallData->status_attr_num; i++, sts_attr++)
    {
      winDebug ("StoreIMIC.status: %s", sts_attr->name);
      if (!strcmp (XNArea, sts_attr->name))
	{
	  rec->sts_attr.area = *(XRectangle *) sts_attr->value;
	}
      else if (!strcmp (XNAreaNeeded, sts_attr->name))
	rec->sts_attr.area_needed = *(XRectangle *) sts_attr->value;
      else if (!strcmp (XNColormap, sts_attr->name))
	rec->sts_attr.cmap = *(Colormap *) sts_attr->value;
      else if (!strcmp (XNStdColormap, sts_attr->name))
	rec->sts_attr.cmap = *(Colormap *) sts_attr->value;
      else if (!strcmp (XNForeground, sts_attr->name))
	rec->sts_attr.foreground = *(CARD32 *) sts_attr->value;
      else if (!strcmp (XNBackground, sts_attr->name))
	rec->sts_attr.background = *(CARD32 *) sts_attr->value;
      else if (!strcmp (XNBackgroundPixmap, sts_attr->name))
	rec->sts_attr.bg_pixmap = *(Pixmap *) sts_attr->value;
      else if (!strcmp (XNFontSet, sts_attr->name))
	{
	  int str_length = strlen (sts_attr->value);
	  if (rec->sts_attr.base_font != NULL)
	    {
	      if (strcmp (rec->sts_attr.base_font, sts_attr->value))
		{
		  XFree (rec->sts_attr.base_font);
		}
	      else
		{
		  continue;
		}
	    }
	  rec->sts_attr.base_font = malloc (str_length + 1);
	  strcpy (rec->sts_attr.base_font, sts_attr->value);
	}
      else if (!strcmp (XNLineSpace, sts_attr->name))
	rec->sts_attr.line_space = *(CARD32 *) sts_attr->value;
      else if (!strcmp (XNCursor, sts_attr->name))
	rec->sts_attr.cursor = *(Cursor *) sts_attr->value;
    }

  rec->pCallData.major_code = pCallData->major_code;
  rec->pCallData.any.minor_code = pCallData->minor_code;
  rec->pCallData.any.connect_id = pCallData->connect_id;
}

IMIC *
FindIMIC (CARD16 icid)
{
  IMIC *rec = ic_list;

  while (rec != NULL)
    {
      if (rec->id == icid)
	return rec;
      rec = rec->next;
    }

  return NULL;
}

IMIC *
FindIMICbyContext (int context)
{
  IMIC *rec = ic_list;

  while (rec != NULL)
    {
      if (rec->context == context)
	return rec;
      rec = rec->next;
    }

  return NULL;
}

void
CreateIMIC (Display * dpy, IMChangeICStruct * pCallData)
{
  IMIC *rec;

  rec = NewIMIC (dpy);
  if (rec == NULL)
    return;

  memset (&rec->pCallData, 0, sizeof (IMProtocol));

  StoreIMIC (rec, pCallData);
  pCallData->icid = rec->id;

  return;
}

void
SetIMIC (IMChangeICStruct *pCallData)
{
  IMIC *rec = FindIMIC (pCallData->icid);

  if (rec == NULL)
    return;
  StoreIMIC (rec, pCallData);
  return;
}

void
GetIMIC (IMChangeICStruct *pCallData)
{
  XICAttribute *ic_attr = pCallData->ic_attr;
  XICAttribute *pre_attr = pCallData->preedit_attr;
  XICAttribute *sts_attr = pCallData->status_attr;
  register int i;
  IMIC *rec = FindIMIC (pCallData->icid);

  if (rec == NULL)
    return;
  for (i = 0; i < (int) pCallData->ic_attr_num; i++, ic_attr++)
    {
      if (!strcmp (XNFilterEvents, ic_attr->name))
	{
	  ic_attr->value = (void *) malloc (sizeof (CARD32));
	  *(CARD32 *) ic_attr->value = KeyPressMask | KeyReleaseMask;
	  ic_attr->value_length = sizeof (CARD32);
	}
    }

  /* preedit attributes */
  for (i = 0; i < (int) pCallData->preedit_attr_num; i++, pre_attr++)
    {
      if (!strcmp (XNArea, pre_attr->name))
	{
	  pre_attr->value = (void *) malloc (sizeof (XRectangle));
	  *(XRectangle *) pre_attr->value = rec->pre_attr.area;
	  pre_attr->value_length = sizeof (XRectangle);
	}
      else if (!strcmp (XNAreaNeeded, pre_attr->name))
	{
	  pre_attr->value = (void *) malloc (sizeof (XRectangle));
	  *(XRectangle *) pre_attr->value = rec->pre_attr.area_needed;
	  pre_attr->value_length = sizeof (XRectangle);
	}
      else if (!strcmp (XNSpotLocation, pre_attr->name))
	{
	  pre_attr->value = (void *) malloc (sizeof (XPoint));
	  *(XPoint *) pre_attr->value = rec->pre_attr.spot_location;
	  pre_attr->value_length = sizeof (XPoint);
	}
      else if (!strcmp (XNFontSet, pre_attr->name))
	{
	  CARD16 base_len = (CARD16) strlen (rec->pre_attr.base_font);
	  int total_len = sizeof (CARD16) + (CARD16) base_len;
	  char *p;

	  pre_attr->value = (void *) malloc (total_len);
	  p = (char *) pre_attr->value;
	  memmove (p, &base_len, sizeof (CARD16));
	  p += sizeof (CARD16);
	  strncpy (p, rec->pre_attr.base_font, base_len);
	  pre_attr->value_length = total_len;
	}
      else if (!strcmp (XNForeground, pre_attr->name))
	{
	  pre_attr->value = (void *) malloc (sizeof (long));
	  *(long *) pre_attr->value = rec->pre_attr.foreground;
	  pre_attr->value_length = sizeof (long);
	}
      else if (!strcmp (XNBackground, pre_attr->name))
	{
	  pre_attr->value = (void *) malloc (sizeof (long));
	  *(long *) pre_attr->value = rec->pre_attr.background;
	  pre_attr->value_length = sizeof (long);
	}
      else if (!strcmp (XNLineSpace, pre_attr->name))
	{
	  pre_attr->value = (void *) malloc (sizeof (long));
#if 0
	  *(long *) pre_attr->value = rec->pre_attr.line_space;
#endif
	  *(long *) pre_attr->value = 18;
	  pre_attr->value_length = sizeof (long);
	}
    }

  /* status attributes */
  for (i = 0; i < (int) pCallData->status_attr_num; i++, sts_attr++)
    {
      if (!strcmp (XNArea, sts_attr->name))
	{
	  sts_attr->value = (void *) malloc (sizeof (XRectangle));
	  *(XRectangle *) sts_attr->value = rec->sts_attr.area;
	  sts_attr->value_length = sizeof (XRectangle);
	}
      else if (!strcmp (XNAreaNeeded, sts_attr->name))
	{
	  sts_attr->value = (void *) malloc (sizeof (XRectangle));
	  *(XRectangle *) sts_attr->value = rec->sts_attr.area_needed;
	  sts_attr->value_length = sizeof (XRectangle);
	}
      else if (!strcmp (XNFontSet, sts_attr->name))
	{
	  CARD16 base_len = (CARD16) strlen (rec->sts_attr.base_font);
	  int total_len = sizeof (CARD16) + (CARD16) base_len;
	  char *p;

	  sts_attr->value = (void *) malloc (total_len);
	  p = (char *) sts_attr->value;
	  memmove (p, &base_len, sizeof (CARD16));
	  p += sizeof (CARD16);
	  strncpy (p, rec->sts_attr.base_font, base_len);
	  sts_attr->value_length = total_len;
	}
      else if (!strcmp (XNForeground, sts_attr->name))
	{
	  sts_attr->value = (void *) malloc (sizeof (long));
	  *(long *) sts_attr->value = rec->sts_attr.foreground;
	  sts_attr->value_length = sizeof (long);
	}
      else if (!strcmp (XNBackground, sts_attr->name))
	{
	  sts_attr->value = (void *) malloc (sizeof (long));
	  *(long *) sts_attr->value = rec->sts_attr.background;
	  sts_attr->value_length = sizeof (long);
	}
      else if (!strcmp (XNLineSpace, sts_attr->name))
	{
	  sts_attr->value = (void *) malloc (sizeof (long));
#if 0
	  *(long *) sts_attr->value = rec->sts_attr.line_space;
#endif
	  *(long *) sts_attr->value = 18;
	  sts_attr->value_length = sizeof (long);
	}
    }
}
