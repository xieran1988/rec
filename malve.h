/* GStreamer malve element
 *
 *  Copyright 2007 Collabora Ltd, 
 *  Copyright 2007 Nokia Corporation
 *   @author: Olivier Crete <olivier.crete@collabora.co.uk>
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
 *
 */

#ifndef __GST_MALVE_H__
#define __GST_MALVE_H__

#include <gst/gst.h>

G_BEGIN_DECLS
/* #define's don't like whitespacey bits */
#define GST_TYPE_MALVE \
  (gst_malve_get_type())
#define GST_MALVE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), \
  GST_TYPE_MALVE,GstMalve))
#define GST_MALVE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), \
  GST_TYPE_MALVE,GstMalveClass))
#define GST_IS_MALVE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_MALVE))
#define GST_IS_MALVE_CLASS(obj) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_MALVE))

typedef struct _GstMalve GstMalve;
typedef struct _GstMalveClass GstMalveClass;

/**
 * GstMalve:
 *
 * The private malve structure
 *
 * Since: 0.10.32
 */
struct _GstMalve
{
  /*< private >*/
  GstElement parent;

  /* atomic boolean */
  volatile gint drop;

  /* Protected by the stream lock */
  gboolean discont;

  GstPad *srcpad;
  GstPad *sinkpad;
};

struct _GstMalveClass
{
  GstElementClass parent_class;
};

GType gst_malve_get_type (void);

G_END_DECLS

#endif /* __GST_MALVE_H__ */
