/* GStreamer
 * Copyright (C) 1999,2000 Erik Walthinsen <omega@cse.ogi.edu>
 *                    2000 Wim Taymans <wtay@chello.be>
 *                    2005 Philippe Khalaf <burger@speedy.org>
 *
 * gsttiv4lsrc.h:
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */


#ifndef __GST_TIV4L_SRC_H__
#define __GST_TIV4L_SRC_H__

#include <gst/gst.h>
#include <gst/base/gstpushsrc.h>

G_BEGIN_DECLS


#define GST_TYPE_TIV4L_SRC \
  (gst_tiv4l_src_get_type())
#define GST_TIV4L_SRC(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_TIV4L_SRC,GstTiv4lSrc))
#define GST_TIV4L_SRC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_TIV4L_SRC,GstTiv4lSrcClass))
#define GST_IS_TIV4L_SRC(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_TIV4L_SRC))
#define GST_IS_TIV4L_SRC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_TIV4L_SRC))


typedef struct _GstTiv4lSrc GstTiv4lSrc;
typedef struct _GstTiv4lSrcClass GstTiv4lSrcClass;

/**
 * GstTiv4lSrc:
 *
 * Opaque #GstTiv4lSrc data structure.
 */
struct _GstTiv4lSrc {
  GstPushSrc element;

  /*< private >*/
  /* new_fd is copied to fd on READY->PAUSED */
  gint new_fd;

  /* fd and flag indicating whether fd is seekable */
  gint fd;
  gboolean seekable_fd;
  guint64 size;

  /* poll timeout */
  guint64 timeout;

  gchar *uri;

  GstPoll *fdset;

  gulong curoffset; /* current offset in file */
};

struct _GstTiv4lSrcClass {
  GstPushSrcClass parent_class;

  /* signals */
  void (*timeout) (GstElement *element);
};

GType gst_tiv4l_src_get_type(void);

G_END_DECLS

#endif /* __GST_TIV4L_SRC_H__ */
