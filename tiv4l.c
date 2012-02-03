/* GStreamer
 * Copyright (C) 1999,2000 Erik Walthinsen <omega@cse.ogi.edu>
 *                    2000 Wim Taymans <wtay@chello.be>
 *                    2005 Philippe Khalaf <burger@speedy.org>
 *
 * gsttiv4lsrc.c:
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
/**
 * SECTION:element-tiv4lsrc
 * @see_also: #GstFdSink
 *
 * Read data from a unix file descriptor.
 * 
 * To generate data, enter some data on the console folowed by enter.
 * The above mentioned pipeline should dump data packets to the console.
 * 
 * If the #GstTiv4lSrc:timeout property is set to a value bigger than 0, tiv4lsrc will
 * generate an element message named
 * <classname>&quot;GstTiv4lSrcTimeout&quot;</classname>
 * if no data was recieved in the given timeout.
 * The message's structure contains one field:
 * <itemizedlist>
 * <listitem>
 *   <para>
 *   #guint64
 *   <classname>&quot;timeout&quot;</classname>: the timeout in microseconds that
 *   expired when waiting for data.
 *   </para>
 * </listitem>
 * </itemizedlist>
 * 
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * echo "Hello GStreamer" | gst-launch -v tiv4lsrc ! fakesink dump=true
 * ]| A simple pipeline to read from the standard input and dump the data
 * with a fakesink as hex ascii block.
 * </refsect2>
 * 
 * Last reviewed on 2008-06-20 (0.10.21)
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <stdarg.h>
#include <syslog.h>
#include <dirent.h>
#include <termios.h>
#include <fcntl.h>
#include <math.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/videodev2.h>

#define IMG_W 720
#define IMG_H 576

#include <sys/types.h>

#ifdef G_OS_WIN32
#include <io.h>                 /* lseek, open, close, read */
#undef lseek
#define lseek _lseeki64
#undef off_t
#define off_t guint64
#endif

#include <sys/stat.h>
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#include <fcntl.h>
#include <stdio.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef _MSC_VER
#undef stat
#define stat _stat
#define fstat _fstat
#define S_ISREG(m)	(((m)&S_IFREG)==S_IFREG)
#endif
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "tiv4l.h"

static GstStaticPadTemplate srctemplate = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

GST_DEBUG_CATEGORY_STATIC (gst_tiv4l_src_debug);
#define GST_CAT_DEFAULT gst_tiv4l_src_debug

#define DEFAULT_FD              0
#define DEFAULT_TIMEOUT         0

enum
{
  PROP_0,

  PROP_FD,
  PROP_TIMEOUT,

  PROP_LAST
};

static void gst_tiv4l_src_uri_handler_init (gpointer g_iface, gpointer iface_data);

static void
_do_init (GType tiv4l_src_type)
{
  static const GInterfaceInfo urihandler_info = {
    gst_tiv4l_src_uri_handler_init,
    NULL,
    NULL
  };

  g_type_add_interface_static (tiv4l_src_type, GST_TYPE_URI_HANDLER,
      &urihandler_info);

  GST_DEBUG_CATEGORY_INIT (gst_tiv4l_src_debug, "tiv4lsrc", 0, "tiv4lsrc element");
}

GST_BOILERPLATE_FULL (GstTiv4lSrc, gst_tiv4l_src, GstPushSrc, GST_TYPE_PUSH_SRC,
    _do_init);

static void gst_tiv4l_src_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_tiv4l_src_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static void gst_tiv4l_src_dispose (GObject * obj);

static gboolean gst_tiv4l_src_start (GstBaseSrc * bsrc);
static gboolean gst_tiv4l_src_stop (GstBaseSrc * bsrc);
static gboolean gst_tiv4l_src_unlock (GstBaseSrc * bsrc);
static gboolean gst_tiv4l_src_unlock_stop (GstBaseSrc * bsrc);
static gboolean gst_tiv4l_src_is_seekable (GstBaseSrc * bsrc);
static gboolean gst_tiv4l_src_get_size (GstBaseSrc * src, guint64 * size);
static gboolean gst_tiv4l_src_do_seek (GstBaseSrc * src, GstSegment * segment);
static gboolean gst_tiv4l_src_query (GstBaseSrc * src, GstQuery * query);

static GstFlowReturn gst_tiv4l_src_create (GstPushSrc * psrc, GstBuffer ** outbuf);

typedef struct {
	int index;
	unsigned int length;
	char *start;
} capbuf_t;
#define CAP_BUFCNT 3
#define DEF_PIX_FMT		V4L2_PIX_FMT_UYVY
static capbuf_t capbuf[CAP_BUFCNT];
static int capfd;

static int tiv4l_cam_exit(GstTiv4lSrc *src)
{
	int i;
	GST_LOG_OBJECT(src, "closing\n");
	for (i = 0; i < CAP_BUFCNT; i++) {
		if ((void *)capbuf[i].start > (void *)0) {
			munmap(capbuf[i].start,
			       capbuf[i].length);
		}
	}
	if (capfd)
		close(capfd);
	return 0;
}

static int tiv4l_cam_init(GstTiv4lSrc *src)
{
	int i, rt = 1;
	struct v4l2_requestbuffers reqbuf;
	struct v4l2_buffer buf;
	struct v4l2_capability capability;
	struct v4l2_input input;
	struct v4l2_format fmt;

retry:
	capfd = open("/dev/video0", O_RDWR);
	if (capfd <= 0) {
		GST_ERROR_OBJECT(src, "open dev failed");
		return 1;
	}

	while (1) {
		i = 0;
		GST_LOG_OBJECT(src, "ioctl G_INPUT");
		if (ioctl(capfd, VIDIOC_G_INPUT, &i) < 0) {
			GST_ERROR_OBJECT(src, "error VIDIOC_G_INPUT, retry");
			sleep(1);
			continue;
		}
		GST_LOG_OBJECT(src, "G_INPUT idx: %d", i);
		break;
	}

	memset(&input, 0, sizeof(input));
	input.index = i;
	if (ioctl(capfd, VIDIOC_ENUMINPUT, &input) < 0) {
		GST_ERROR_OBJECT(src, "error VIDIOC_ENUMINPUT");
		return 1;
	}
	GST_LOG_OBJECT(src, "input: %s\n", input.name);

	if (ioctl(capfd, VIDIOC_QUERYCAP, &capability) < 0) {
		GST_ERROR_OBJECT(src, "error VIDIOC_QUERYCAP");
		return 1;
	}
	if (!(capability.capabilities & V4L2_CAP_STREAMING)) {
		GST_ERROR_OBJECT(src, "Not capable of streaming");
		return 1;
	}

	for (i = 0; ; i++) {
		struct v4l2_fmtdesc fmtdesc = {};
		fmtdesc.index = i;
		fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		if (ioctl(capfd, VIDIOC_ENUM_FMT, &fmtdesc))
			break;
		GST_LOG_OBJECT(src, "fmt: %.32s", fmtdesc.description);
	}

	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (ioctl(capfd, VIDIOC_G_FMT, &fmt)) {
		GST_ERROR_OBJECT(src, "error VIDIOC_G_FMT");
		return 1;
	}

	fmt.fmt.pix.pixelformat = DEF_PIX_FMT;
	if (ioctl(capfd, VIDIOC_S_FMT, &fmt)) {
		GST_ERROR_OBJECT(src, "error VIDIOC_S_FMT");
		return 1;
	}

	if (ioctl(capfd, VIDIOC_G_FMT, &fmt)) {
		GST_ERROR_OBJECT(src, "error VIDIOC_G_FMT");
		return 1;
	}

	GST_LOG_OBJECT(src, "getfmt.size %dx%d", fmt.fmt.pix.width, fmt.fmt.pix.height);
	GST_LOG_OBJECT(src, "want.size %dx%d", IMG_W, IMG_H);

	if (!
			((fmt.fmt.pix.width == IMG_W*2 && fmt.fmt.pix.height == IMG_H*2) || 
			(fmt.fmt.pix.width == IMG_W && fmt.fmt.pix.height == IMG_H) )
		 )	
	{
		GST_LOG_OBJECT(src, "err getfmt.size %d,%d",fmt.fmt.pix.width,fmt.fmt.pix.height);
		GST_LOG_OBJECT(src, "retry %d ...", rt);
		rt++;
		close(capfd);
		goto retry;
	}

	if (fmt.fmt.pix.pixelformat != DEF_PIX_FMT) {
		GST_ERROR_OBJECT(src, "Requested pixel format not supported");
		return 1;
	}

	memset(&reqbuf, 0, sizeof(reqbuf));
	reqbuf.count = CAP_BUFCNT;
	reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	reqbuf.memory = V4L2_MEMORY_MMAP;
	if (ioctl(capfd, VIDIOC_REQBUFS, &reqbuf) < 0) {
		GST_ERROR_OBJECT(src, "error Cannot allocate memory");
		return 1;
	}
	GST_LOG_OBJECT(src, "reqbuf.count: %d", reqbuf.count);

	memset(&buf, 0, sizeof(buf));
	for (i = 0; i < reqbuf.count; i++) {
		buf.type = reqbuf.type;
		buf.index = i;
		buf.memory = reqbuf.memory;
		if (ioctl(capfd, VIDIOC_QUERYBUF, &buf)) {
			GST_ERROR_OBJECT(src, "error VIDIOC_QUERYCAP");
			return 1;
		}

		capbuf[i].length = buf.length;
		capbuf[i].index = i;
		capbuf[i].start = mmap(NULL, buf.length, PROT_READ|PROT_WRITE, MAP_SHARED, capfd, buf.m.offset);
		if (capbuf[i].start == MAP_FAILED) {
			GST_ERROR_OBJECT(src, "error Cannot mmap = %d buffer", i);
			return 1;
		}
		memset((void *) capbuf[i].start, 0x80,
			capbuf[i].length);
		if (ioctl(capfd, VIDIOC_QBUF, &buf)) {
			GST_ERROR_OBJECT(src, "error VIDIOC_QBUF");
			return 1;
		}
	}
	GST_LOG_OBJECT(src, "Init done successfully");

	int a = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (ioctl(capfd, VIDIOC_STREAMON, &a)) {
		GST_LOG_OBJECT(src, "error VIDIOC_STREAMON");
		return 1;
	}
	GST_LOG_OBJECT(src, "Stream on...");

	return 0;
}

static int tiv4l_cam_poll(GstTiv4lSrc *src, GstBuffer *buf)
{
	struct v4l2_buffer capture_buf;
	
	capture_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	capture_buf.index = 0;
	capture_buf.memory = V4L2_MEMORY_MMAP;

	if (ioctl(capfd, VIDIOC_DQBUF, &capture_buf)) {
		GST_LOG_OBJECT(src, "error VIDIOC_DQBUF");
		return 1;
	}
	gst_buffer_set_data(buf, capbuf[capture_buf.index].start, IMG_W*IMG_H*2);
	GST_LOG("bufaddr %p", capbuf[capture_buf.index].start);
	if (ioctl(capfd, VIDIOC_QBUF, &capture_buf)) {
		GST_LOG_OBJECT(src, "error VIDIOC_QBUF");
		return 1;
	}
	return 0;
}

static void
gst_tiv4l_src_base_init (gpointer g_class)
{
  GstElementClass *gstelement_class = GST_ELEMENT_CLASS (g_class);

  gst_element_class_set_details_simple (gstelement_class,
      "Filedescriptor Source",
      "Source/File",
      "Read from a file descriptor", "Erik Walthinsen <omega@cse.ogi.edu>");
  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&srctemplate));
}

static void
gst_tiv4l_src_class_init (GstTiv4lSrcClass * klass)
{
  GObjectClass *gobject_class;
  GstBaseSrcClass *gstbasesrc_class;
  GstPushSrcClass *gstpush_src_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gstbasesrc_class = GST_BASE_SRC_CLASS (klass);
  gstpush_src_class = GST_PUSH_SRC_CLASS (klass);

  gobject_class->set_property = gst_tiv4l_src_set_property;
  gobject_class->get_property = gst_tiv4l_src_get_property;
  gobject_class->dispose = gst_tiv4l_src_dispose;

  g_object_class_install_property (gobject_class, PROP_FD,
      g_param_spec_int ("fd", "fd", "An open file descriptor to read from",
          0, G_MAXINT, DEFAULT_FD, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  /**
   * GstTiv4lSrc:timeout
   *
   * Post a message after timeout microseconds
   *
   * Since: 0.10.21
   */
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_TIMEOUT,
      g_param_spec_uint64 ("timeout", "Timeout",
          "Post a message after timeout microseconds (0 = disabled)", 0,
          G_MAXUINT64, DEFAULT_TIMEOUT,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  gstbasesrc_class->start = GST_DEBUG_FUNCPTR (gst_tiv4l_src_start);
  gstbasesrc_class->stop = GST_DEBUG_FUNCPTR (gst_tiv4l_src_stop);
  gstpush_src_class->create = GST_DEBUG_FUNCPTR (gst_tiv4l_src_create);
	/*
  gstbasesrc_class->unlock = GST_DEBUG_FUNCPTR (gst_tiv4l_src_unlock);
  gstbasesrc_class->unlock_stop = GST_DEBUG_FUNCPTR (gst_tiv4l_src_unlock_stop);
  gstbasesrc_class->is_seekable = GST_DEBUG_FUNCPTR (gst_tiv4l_src_is_seekable);
  gstbasesrc_class->get_size = GST_DEBUG_FUNCPTR (gst_tiv4l_src_get_size);
  gstbasesrc_class->do_seek = GST_DEBUG_FUNCPTR (gst_tiv4l_src_do_seek);
  gstbasesrc_class->query = GST_DEBUG_FUNCPTR (gst_tiv4l_src_query);
	*/
}

static void
gst_tiv4l_src_init (GstTiv4lSrc * tiv4lsrc, GstTiv4lSrcClass * klass)
{
  tiv4lsrc->new_fd = DEFAULT_FD;
  tiv4lsrc->seekable_fd = FALSE;
  tiv4lsrc->fd = -1;
  tiv4lsrc->size = -1;
  tiv4lsrc->timeout = DEFAULT_TIMEOUT;
  tiv4lsrc->uri = g_strdup_printf ("fd://0");
  tiv4lsrc->curoffset = 0;

}

static void
gst_tiv4l_src_dispose (GObject * obj)
{
  GstTiv4lSrc *src = GST_TIV4L_SRC (obj);

  g_free (src->uri);
  src->uri = NULL;

  G_OBJECT_CLASS (parent_class)->dispose (obj);
}

static void
gst_tiv4l_src_update_fd (GstTiv4lSrc * src, guint64 size)
{
  struct stat stat_results;

  GST_DEBUG_OBJECT (src, "fdset %p, old_fd %d, new_fd %d", src->fdset, src->fd,
      src->new_fd);

  /* we need to always update the fdset since it may not have existed when
   * gst_tiv4l_src_update_fd () was called earlier */
  if (src->fdset != NULL) {
    GstPollFD fd = GST_POLL_FD_INIT;

    if (src->fd >= 0) {
      fd.fd = src->fd;
      /* this will log a harmless warning, if it was never added */
      gst_poll_remove_fd (src->fdset, &fd);
    }

    fd.fd = src->new_fd;
    gst_poll_add_fd (src->fdset, &fd);
    gst_poll_fd_ctl_read (src->fdset, &fd, TRUE);
  }


  if (src->fd != src->new_fd) {
    GST_INFO_OBJECT (src, "Updating to fd %d", src->new_fd);

    src->fd = src->new_fd;

    GST_INFO_OBJECT (src, "Setting size to fd %" G_GUINT64_FORMAT, size);
    src->size = size;

    g_free (src->uri);
    src->uri = g_strdup_printf ("fd://%d", src->fd);

    if (fstat (src->fd, &stat_results) < 0)
      goto not_seekable;

    if (!S_ISREG (stat_results.st_mode))
      goto not_seekable;

    /* Try a seek of 0 bytes offset to check for seekability */
    if (lseek (src->fd, 0, SEEK_CUR) < 0)
      goto not_seekable;

    GST_INFO_OBJECT (src, "marking fd %d as seekable", src->fd);
    src->seekable_fd = TRUE;
  }
  return;

not_seekable:
  {
    GST_INFO_OBJECT (src, "marking fd %d as NOT seekable", src->fd);
    src->seekable_fd = FALSE;
  }
}

static gboolean
gst_tiv4l_src_start (GstBaseSrc * bsrc)
{
  GstTiv4lSrc *src = GST_TIV4L_SRC (bsrc);

  src->curoffset = 0;

  if ((src->fdset = gst_poll_new (TRUE)) == NULL)
    goto socket_pair;

  gst_tiv4l_src_update_fd (src, -1);

	if (tiv4l_cam_init(src)) 
		return FALSE;

  return TRUE;

  /* ERRORS */
socket_pair:
  {
    GST_ELEMENT_ERROR (src, RESOURCE, OPEN_READ_WRITE, (NULL),
        GST_ERROR_SYSTEM);
    return FALSE;
  }
}

static gboolean
gst_tiv4l_src_stop (GstBaseSrc * bsrc)
{
  GstTiv4lSrc *src = GST_TIV4L_SRC (bsrc);

  if (src->fdset) {
    gst_poll_free (src->fdset);
    src->fdset = NULL;
  }

	tiv4l_cam_exit(src);

  return TRUE;
}

static gboolean
gst_tiv4l_src_unlock (GstBaseSrc * bsrc)
{
  GstTiv4lSrc *src = GST_TIV4L_SRC (bsrc);

  GST_LOG_OBJECT (src, "Flushing");
  GST_OBJECT_LOCK (src);
  gst_poll_set_flushing (src->fdset, TRUE);
  GST_OBJECT_UNLOCK (src);

  return TRUE;
}

static gboolean
gst_tiv4l_src_unlock_stop (GstBaseSrc * bsrc)
{
  GstTiv4lSrc *src = GST_TIV4L_SRC (bsrc);

  GST_LOG_OBJECT (src, "No longer flushing");
  GST_OBJECT_LOCK (src);
  gst_poll_set_flushing (src->fdset, FALSE);
  GST_OBJECT_UNLOCK (src);

  return TRUE;
}

static void
gst_tiv4l_src_set_property (GObject * object, guint prop_id, const GValue * value,
    GParamSpec * pspec)
{
  GstTiv4lSrc *src = GST_TIV4L_SRC (object);

  switch (prop_id) {
    case PROP_FD:
      src->new_fd = g_value_get_int (value);

      /* If state is ready or below, update the current fd immediately
       * so it is reflected in get_properties and uri */
      GST_OBJECT_LOCK (object);
      if (GST_STATE (GST_ELEMENT (src)) <= GST_STATE_READY) {
        GST_DEBUG_OBJECT (src, "state ready or lower, updating to use new fd");
        gst_tiv4l_src_update_fd (src, -1);
      } else {
        GST_DEBUG_OBJECT (src, "state above ready, not updating to new fd yet");
      }
      GST_OBJECT_UNLOCK (object);
      break;
    case PROP_TIMEOUT:
      src->timeout = g_value_get_uint64 (value);
      GST_DEBUG_OBJECT (src, "poll timeout set to %" GST_TIME_FORMAT,
          GST_TIME_ARGS (src->timeout));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_tiv4l_src_get_property (GObject * object, guint prop_id, GValue * value,
    GParamSpec * pspec)
{
  GstTiv4lSrc *src = GST_TIV4L_SRC (object);

  switch (prop_id) {
    case PROP_FD:
      g_value_set_int (value, src->fd);
      break;
    case PROP_TIMEOUT:
      g_value_set_uint64 (value, src->timeout);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static GstFlowReturn
gst_tiv4l_src_create (GstPushSrc * psrc, GstBuffer ** outbuf)
{
  GstTiv4lSrc *src;
  GstBuffer *buf;
  gssize readbytes;
  guint blocksize;
  GstClockTime timeout;

#ifndef HAVE_WIN32
  gboolean try_again;
  gint retval;
#endif

  GST_LOG_OBJECT (psrc, "src_create");

  src = GST_TIV4L_SRC (psrc);

  if (src->timeout > 0) {
    timeout = src->timeout * GST_USECOND;
  } else {
    timeout = GST_CLOCK_TIME_NONE;
  }

#if 0

#ifndef HAVE_WIN32
  do {
    try_again = FALSE;

    GST_LOG_OBJECT (src, "doing poll, timeout %" GST_TIME_FORMAT,
        GST_TIME_ARGS (src->timeout));

    retval = gst_poll_wait (src->fdset, timeout);
    GST_LOG_OBJECT (src, "poll returned %d", retval);

    if (G_UNLIKELY (retval == -1)) {
      if (errno == EINTR || errno == EAGAIN) {
        /* retry if interrupted */
        try_again = TRUE;
      } else if (errno == EBUSY) {
        goto stopped;
      } else {
        goto poll_error;
      }
    } else if (G_UNLIKELY (retval == 0)) {
      try_again = TRUE;
      /* timeout, post element message */
      gst_element_post_message (GST_ELEMENT_CAST (src),
          gst_message_new_element (GST_OBJECT_CAST (src),
              gst_structure_new ("GstTiv4lSrcTimeout",
                  "timeout", G_TYPE_UINT64, src->timeout, NULL)));
    }
  } while (G_UNLIKELY (try_again));     /* retry if interrupted or timeout */
#endif

  blocksize = GST_BASE_SRC (src)->blocksize;
#endif

	readbytes = IMG_W * IMG_H * 2;
	blocksize = readbytes;

  /* create the buffer */
  buf = gst_buffer_new ();
  if (G_UNLIKELY (buf == NULL)) {
    GST_ERROR_OBJECT (src, "Failed to allocate %u bytes", blocksize);
    return GST_FLOW_ERROR;
  }

  tiv4l_cam_poll(src, buf);

#if 0
  do {
    readbytes = read (src->fd, GST_BUFFER_DATA (buf), blocksize);
    GST_LOG_OBJECT (src, "read %" G_GSSIZE_FORMAT, readbytes);
  } while (readbytes == -1 && errno == EINTR);  /* retry if interrupted */

  if (readbytes < 0)
    goto read_error;

  if (readbytes == 0)
    goto eos;
#endif
	
  GST_BUFFER_OFFSET (buf) = src->curoffset;
  GST_BUFFER_SIZE (buf) = readbytes;
  GST_BUFFER_TIMESTAMP (buf) = GST_CLOCK_TIME_NONE;
  src->curoffset += readbytes;

  GST_INFO_OBJECT (psrc, "Read buffer of size %" G_GSSIZE_FORMAT, readbytes);

  /* we're done, return the buffer */
  *outbuf = buf;

  return GST_FLOW_OK;

#if 0
  /* ERRORS */
#ifndef HAVE_WIN32
poll_error:
  {
    GST_ELEMENT_ERROR (src, RESOURCE, READ, (NULL),
        ("poll on file descriptor: %s.", g_strerror (errno)));
    GST_DEBUG_OBJECT (psrc, "Error during poll");
    return GST_FLOW_ERROR;
  }
stopped:
  {
    GST_DEBUG_OBJECT (psrc, "Poll stopped");
    return GST_FLOW_WRONG_STATE;
  }
#endif
eos:
  {
    GST_DEBUG_OBJECT (psrc, "Read 0 bytes. EOS.");
    gst_buffer_unref (buf);
    return GST_FLOW_UNEXPECTED;
  }
read_error:
  {
    GST_ELEMENT_ERROR (src, RESOURCE, READ, (NULL),
        ("read on file descriptor: %s.", g_strerror (errno)));
    GST_DEBUG_OBJECT (psrc, "Error reading from fd");
    gst_buffer_unref (buf);
    return GST_FLOW_ERROR;
  }
#endif
}

static gboolean
gst_tiv4l_src_query (GstBaseSrc * basesrc, GstQuery * query)
{
  gboolean ret = FALSE;
  GstTiv4lSrc *src = GST_TIV4L_SRC (basesrc);

  GST_LOG_OBJECT (src, "src_query");

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_URI:
      gst_query_set_uri (query, src->uri);
      ret = TRUE;
      break;
    default:
      ret = FALSE;
      break;
  }

  if (!ret)
    ret = GST_BASE_SRC_CLASS (parent_class)->query (basesrc, query);

  return ret;
}

static gboolean
gst_tiv4l_src_is_seekable (GstBaseSrc * bsrc)
{
  GstTiv4lSrc *src = GST_TIV4L_SRC (bsrc);

  return src->seekable_fd;
}

static gboolean
gst_tiv4l_src_get_size (GstBaseSrc * bsrc, guint64 * size)
{
  GstTiv4lSrc *src = GST_TIV4L_SRC (bsrc);
  struct stat stat_results;

  GST_LOG_OBJECT (src, "get_size");

  if (src->size != -1) {
    *size = src->size;
    return TRUE;
  }

  if (!src->seekable_fd) {
    /* If it isn't seekable, we won't know the length (but fstat will still
     * succeed, and wrongly say our length is zero. */
    return FALSE;
  }

  if (fstat (src->fd, &stat_results) < 0)
    goto could_not_stat;

  *size = stat_results.st_size;

  return TRUE;

  /* ERROR */
could_not_stat:
  {
    return FALSE;
  }
}

static gboolean
gst_tiv4l_src_do_seek (GstBaseSrc * bsrc, GstSegment * segment)
{
  gint res;
  gint64 offset;
  GstTiv4lSrc *src = GST_TIV4L_SRC (bsrc);

  GST_LOG_OBJECT (src, "do_seek");

  offset = segment->start;

  /* No need to seek to the current position */
  if (offset == src->curoffset)
    return TRUE;

  res = lseek (src->fd, offset, SEEK_SET);
  if (G_UNLIKELY (res < 0 || res != offset))
    goto seek_failed;

  segment->last_stop = segment->start;
  segment->time = segment->start;

  return TRUE;

seek_failed:
  GST_DEBUG_OBJECT (src, "lseek returned %" G_GINT64_FORMAT, offset);
  return FALSE;
}

/*** GSTURIHANDLER INTERFACE *************************************************/

static GstURIType
gst_tiv4l_src_uri_get_type (void)
{
  return GST_URI_SRC;
}

static gchar **
gst_tiv4l_src_uri_get_protocols (void)
{
  static gchar *protocols[] = { (char *) "fd", NULL };

  return protocols;
}

static const gchar *
gst_tiv4l_src_uri_get_uri (GstURIHandler * handler)
{
  GstTiv4lSrc *src = GST_TIV4L_SRC (handler);

  return src->uri;
}

static gboolean
gst_tiv4l_src_uri_set_uri (GstURIHandler * handler, const gchar * uri)
{
  gchar *protocol, *q;
  GstTiv4lSrc *src = GST_TIV4L_SRC (handler);
  gint fd;
  guint64 size = -1;

  GST_INFO_OBJECT (src, "checking uri %s", uri);

  protocol = gst_uri_get_protocol (uri);
  if (strcmp (protocol, "fd") != 0) {
    g_free (protocol);
    return FALSE;
  }
  g_free (protocol);

  if (sscanf (uri, "fd://%d", &fd) != 1 || fd < 0)
    return FALSE;

  if ((q = g_strstr_len (uri, -1, "?"))) {
    gchar *sp;

    GST_INFO_OBJECT (src, "found ?");

    if ((sp = g_strstr_len (q, -1, "size="))) {
      if (sscanf (sp, "size=%" G_GUINT64_FORMAT, &size) != 1) {
        GST_INFO_OBJECT (src, "parsing size failed");
        size = -1;
      } else {
        GST_INFO_OBJECT (src, "found size %" G_GUINT64_FORMAT, size);
      }
    }
  }

  src->new_fd = fd;

  GST_OBJECT_LOCK (src);
  if (GST_STATE (GST_ELEMENT (src)) <= GST_STATE_READY) {
    gst_tiv4l_src_update_fd (src, size);
  }
  GST_OBJECT_UNLOCK (src);

  return TRUE;
}

static void
gst_tiv4l_src_uri_handler_init (gpointer g_iface, gpointer iface_data)
{
  GstURIHandlerInterface *iface = (GstURIHandlerInterface *) g_iface;

  iface->get_type = gst_tiv4l_src_uri_get_type;
  iface->get_protocols = gst_tiv4l_src_uri_get_protocols;
  iface->get_uri = gst_tiv4l_src_uri_get_uri;
  iface->set_uri = gst_tiv4l_src_uri_set_uri;
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
			"tiv4lsrc", // modify here
			GST_RANK_NONE,
      gst_tiv4l_src_get_type());
}

/* PACKAGE: this is usually set by autotools depending on some _INIT macro
 * in configure.ac and then written into and defined in config.h, but we can
 * just set it ourselves here in case someone doesn't use autotools to
 * compile this code. GST_PLUGIN_DEFINE needs PACKAGE to be defined.
 */
#ifndef PACKAGE
// modify here
#define PACKAGE "tiv4lpackage"
#endif

/* gstreamer looks for this structure to register plugins
 *
 * exchange the string 'Template plugin' with your plugin description
 */
GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
		// and modify here
    "tiv4lplugin",
    "My tiv4l plugin",
    plugin_init,
    "0.10",
    "LGPL",
    "GStreamer",
    "http://gstreamer.net/"
)

