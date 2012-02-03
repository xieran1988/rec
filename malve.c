/* GStreamer malve element
 *  Copyright 2007-2009 Collabora Ltd
 *   @author: Olivier Crete <olivier.crete@collabora.co.uk>
 *  Copyright 2007-2009 Nokia Corporation
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

/**
 * SECTION:element-malve
 *
 * The malve is a simple element that drops buffers when the #GstMalve:drop
 * property is set to %TRUE and lets then through otherwise.
 *
 * Any downstream error received while the #GstMalve:drop property is %FALSE
 * is ignored. So downstream element can be set to  %GST_STATE_NULL and removed,
 * without using pad blocking.
 *
 * This element was previously part of gst-plugins-farsight, and then
 * gst-plugins-bad.
 *
 * Documentation last reviewed on 2010-12-30 (0.10.31)
 *
 * Since: 0.10.32
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "malve.h"

#include <string.h>

GST_DEBUG_CATEGORY_STATIC (malve_debug);
#define GST_CAT_DEFAULT (malve_debug)

static GstStaticPadTemplate sinktemplate = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

static GstStaticPadTemplate srctemplate = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

enum
{
  PROP_0,
  PROP_DROP
};

#define DEFAULT_DROP FALSE

static void gst_malve_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec);
static void gst_malve_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec);

static gboolean gst_malve_event (GstPad * pad, GstEvent * event);
static GstFlowReturn gst_malve_buffer_alloc (GstPad * pad, guint64 offset,
    guint size, GstCaps * caps, GstBuffer ** buf);
static GstFlowReturn gst_malve_chain (GstPad * pad, GstBuffer * buffer);
static GstCaps *gst_malve_getcaps (GstPad * pad);

#define _do_init(bla) \
  GST_DEBUG_CATEGORY_INIT (malve_debug, "malve", 0, "Malve");

GST_BOILERPLATE_FULL (GstMalve, gst_malve, GstElement,
    GST_TYPE_ELEMENT, _do_init);

static void
gst_malve_base_init (gpointer klass)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&srctemplate));
  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&sinktemplate));

  gst_element_class_set_details_simple (element_class, "Malve element",
      "Filter", "Drops buffers and events or lets them through",
      "Olivier Crete <olivier.crete@collabora.co.uk>");
}

static void
gst_malve_class_init (GstMalveClass * klass)
{
  GObjectClass *gobject_class;

  gobject_class = (GObjectClass *) klass;

  gobject_class->set_property = gst_malve_set_property;
  gobject_class->get_property = gst_malve_get_property;

  g_object_class_install_property (gobject_class, PROP_DROP,
      g_param_spec_boolean ("drop", "Drop buffers and events",
          "Whether to drop buffers and events or let them through",
          DEFAULT_DROP, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void
gst_malve_init (GstMalve * malve, GstMalveClass * klass)
{
  malve->drop = FALSE;
  malve->discont = FALSE;

  malve->srcpad = gst_pad_new_from_static_template (&srctemplate, "src");
  gst_pad_set_getcaps_function (malve->srcpad,
      GST_DEBUG_FUNCPTR (gst_malve_getcaps));
  gst_element_add_pad (GST_ELEMENT (malve), malve->srcpad);

  malve->sinkpad = gst_pad_new_from_static_template (&sinktemplate, "sink");
  gst_pad_set_chain_function (malve->sinkpad,
      GST_DEBUG_FUNCPTR (gst_malve_chain));
  gst_pad_set_event_function (malve->sinkpad,
      GST_DEBUG_FUNCPTR (gst_malve_event));
  gst_pad_set_bufferalloc_function (malve->sinkpad,
      GST_DEBUG_FUNCPTR (gst_malve_buffer_alloc));
  gst_pad_set_getcaps_function (malve->sinkpad,
      GST_DEBUG_FUNCPTR (gst_malve_getcaps));
  gst_element_add_pad (GST_ELEMENT (malve), malve->sinkpad);
}


static void
gst_malve_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec)
{
  GstMalve *malve = GST_MALVE (object);

  switch (prop_id) {
    case PROP_DROP:
      g_atomic_int_set (&malve->drop, g_value_get_boolean (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_malve_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec)
{
  GstMalve *malve = GST_MALVE (object);

  switch (prop_id) {
    case PROP_DROP:
      g_value_set_boolean (value, g_atomic_int_get (&malve->drop));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static GstFlowReturn
gst_malve_chain (GstPad * pad, GstBuffer * buffer)
{
  GstMalve *malve = GST_MALVE (GST_OBJECT_PARENT (pad));
  GstFlowReturn ret = GST_FLOW_OK;

  if (g_atomic_int_get (&malve->drop)) {
    gst_buffer_unref (buffer);
    malve->discont = TRUE;
  } else {
    if (malve->discont) {
      buffer = gst_buffer_make_metadata_writable (buffer);
      GST_BUFFER_FLAG_SET (buffer, GST_BUFFER_FLAG_DISCONT);
      malve->discont = FALSE;
    }

    ret = gst_pad_push (malve->srcpad, buffer);
  }

  /* Ignore errors if "drop" was changed while the thread was blocked
   * downwards
   */
  if (g_atomic_int_get (&malve->drop))
    ret = GST_FLOW_OK;

	{
		static int i;
		i++;
		malve->drop = !(i % 3);
	}

  return ret;
}


static gboolean
gst_malve_event (GstPad * pad, GstEvent * event)
{
  GstMalve *malve = GST_MALVE (gst_pad_get_parent_element (pad));
  gboolean ret = TRUE;

  if (g_atomic_int_get (&malve->drop))
    gst_event_unref (event);
  else
    ret = gst_pad_push_event (malve->srcpad, event);

  /* Ignore errors if "drop" was changed while the thread was blocked
   * downwards.
   */
  if (g_atomic_int_get (&malve->drop))
    ret = TRUE;

  gst_object_unref (malve);
  return ret;
}

static GstFlowReturn
gst_malve_buffer_alloc (GstPad * pad, guint64 offset, guint size,
    GstCaps * caps, GstBuffer ** buf)
{
  GstMalve *malve = GST_MALVE (gst_pad_get_parent_element (pad));
  GstFlowReturn ret = GST_FLOW_OK;

  if (g_atomic_int_get (&malve->drop))
    *buf = NULL;
  else
    ret = gst_pad_alloc_buffer (malve->srcpad, offset, size, caps, buf);

  /* Ignore errors if "drop" was changed while the thread was blocked
   * downwards
   */
  if (g_atomic_int_get (&malve->drop))
    ret = GST_FLOW_OK;

  gst_object_unref (malve);

  return ret;
}

static GstCaps *
gst_malve_getcaps (GstPad * pad)
{
  GstMalve *malve = GST_MALVE (gst_pad_get_parent (pad));
  GstCaps *caps;

  if (pad == malve->sinkpad)
    caps = gst_pad_peer_get_caps (malve->srcpad);
  else
    caps = gst_pad_peer_get_caps (malve->sinkpad);

  if (caps == NULL)
    caps = gst_caps_copy (gst_pad_get_pad_template_caps (pad));

  gst_object_unref (malve);

  return caps;
}

static gboolean
plugin_init (GstPlugin * plugin)
{
  /* debug category for fltering log messages
   *
   * exchange the string 'Template plugin' with your description
  GST_DEBUG_CATEGORY_INIT (gst_plugin_template_debug, "plugin",
      0, "Template plugin");
   */
  return gst_element_register (plugin, 
			"malve", // modify here
			GST_RANK_NONE,
      gst_malve_get_type());
}

/* PACKAGE: this is usually set by autotools depending on some _INIT macro
 * in configure.ac and then written into and defined in config.h, but we can
 * just set it ourselves here in case someone doesn't use autotools to
 * compile this code. GST_PLUGIN_DEFINE needs PACKAGE to be defined.
 */
#ifndef PACKAGE
// modify here
#define PACKAGE "malvepackage"
#endif

/* gstreamer looks for this structure to register plugins
 *
 * exchange the string 'Template plugin' with your plugin description
 */
GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
		// and modify here
    "malveplugin",
    "My malve plugin",
    plugin_init,
    "0.10",
    "LGPL",
    "GStreamer",
    "http://gstreamer.net/"
)

