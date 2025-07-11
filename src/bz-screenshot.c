/* bz-screenshot.c
 *
 * Copyright 2025 Adam Masciola
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "config.h"

#include "bz-screenshot.h"

struct _BzScreenshot
{
  GtkWidget parent_instance;

  GdkPaintable *paintable;
};

G_DEFINE_FINAL_TYPE (BzScreenshot, bz_screenshot, GTK_TYPE_WIDGET)

enum
{
  PROP_0,

  PROP_PAINTABLE,

  LAST_PROP
};
static GParamSpec *props[LAST_PROP] = { 0 };

static void
invalidate_contents (GdkPaintable *paintable,
                     BzScreenshot *self);

static void
invalidate_size (GdkPaintable *paintable,
                 BzScreenshot *self);

static void
bz_screenshot_dispose (GObject *object)
{
  BzScreenshot *self = BZ_SCREENSHOT (object);

  if (self->paintable != NULL)
    {
      g_signal_handlers_disconnect_by_func (self->paintable, invalidate_contents, self);
      g_signal_handlers_disconnect_by_func (self->paintable, invalidate_size, self);
    }
  g_clear_object (&self->paintable);

  G_OBJECT_CLASS (bz_screenshot_parent_class)->dispose (object);
}

static void
bz_screenshot_get_property (GObject    *object,
                            guint       prop_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  BzScreenshot *self = BZ_SCREENSHOT (object);

  switch (prop_id)
    {
    case PROP_PAINTABLE:
      g_value_set_object (value, bz_screenshot_get_paintable (self));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
bz_screenshot_set_property (GObject      *object,
                            guint         prop_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  BzScreenshot *self = BZ_SCREENSHOT (object);

  switch (prop_id)
    {
    case PROP_PAINTABLE:
      bz_screenshot_set_paintable (self, g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static GtkSizeRequestMode
bz_screenshot_get_request_mode (GtkWidget *widget)
{
  return GTK_SIZE_REQUEST_HEIGHT_FOR_WIDTH;
}

static void
bz_screenshot_measure (GtkWidget     *widget,
                       GtkOrientation orientation,
                       int            for_size,
                       int           *minimum,
                       int           *natural,
                       int           *minimum_baseline,
                       int           *natural_baseline)
{
  BzScreenshot *self = BZ_SCREENSHOT (widget);

  if (self->paintable == NULL)
    return;

  if (orientation == GTK_ORIENTATION_VERTICAL)
    {
      int    intrinsic_height      = 0;
      double intrinsic_aspect_rato = 0.0;

      intrinsic_height      = gdk_paintable_get_intrinsic_height (self->paintable);
      intrinsic_aspect_rato = gdk_paintable_get_intrinsic_aspect_ratio (self->paintable);

      if (for_size >= 0 && intrinsic_aspect_rato > 0.0)
        {
          double result = 0.0;

          result = ceil ((double) for_size / intrinsic_aspect_rato);

          *minimum = MIN (intrinsic_height, result);
          *natural = MIN (intrinsic_height, result);
        }
      else
        {
          *minimum = 0.0;
          *natural = intrinsic_height;
        }
    }
  else
    {
      *minimum = 0;
      *natural = gdk_paintable_get_intrinsic_width (self->paintable);
    }
}

static void
bz_screenshot_snapshot (GtkWidget   *widget,
                        GtkSnapshot *snapshot)
{
  BzScreenshot  *self            = BZ_SCREENSHOT (widget);
  int            widget_width    = 0;
  int            paintable_width = 0;
  GskRoundedRect rect            = { 0 };

  if (self->paintable == NULL)
    return;

  widget_width    = gtk_widget_get_width (widget);
  paintable_width = gdk_paintable_get_intrinsic_width (self->paintable);

  if (widget_width > paintable_width)
    gtk_snapshot_translate (
        snapshot,
        &GRAPHENE_POINT_INIT (
            floor ((widget_width - paintable_width) / 2.0),
            0));

  rect.bounds = GRAPHENE_RECT_INIT (
      0, 0,
      MIN (widget_width, paintable_width),
      gtk_widget_get_height (widget));

  rect.corner[0].width  = 10.0;
  rect.corner[0].height = 10.0;
  rect.corner[1].width  = 10.0;
  rect.corner[1].height = 10.0;
  rect.corner[2].width  = 10.0;
  rect.corner[2].height = 10.0;
  rect.corner[3].width  = 10.0;
  rect.corner[3].height = 10.0;

  gtk_snapshot_push_rounded_clip (snapshot, &rect);
  gdk_paintable_snapshot (
      self->paintable,
      snapshot,
      rect.bounds.size.width,
      rect.bounds.size.height);
  gtk_snapshot_pop (snapshot);
}

static void
bz_screenshot_class_init (BzScreenshotClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose      = bz_screenshot_dispose;
  object_class->get_property = bz_screenshot_get_property;
  object_class->set_property = bz_screenshot_set_property;

  props[PROP_PAINTABLE] =
      g_param_spec_object (
          "paintable",
          NULL, NULL,
          GDK_TYPE_PAINTABLE,
          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, LAST_PROP, props);

  widget_class->get_request_mode = bz_screenshot_get_request_mode;
  widget_class->measure          = bz_screenshot_measure;
  widget_class->snapshot         = bz_screenshot_snapshot;
}

static void
bz_screenshot_init (BzScreenshot *self)
{
}

GtkWidget *
bz_screenshot_new (void)
{
  return g_object_new (BZ_TYPE_SCREENSHOT, NULL);
}

void
bz_screenshot_set_paintable (BzScreenshot *self,
                             GdkPaintable *paintable)
{
  g_return_if_fail (BZ_IS_SCREENSHOT (self));
  g_return_if_fail (paintable == NULL || GDK_IS_PAINTABLE (paintable));

  if (self->paintable != NULL)
    {
      g_signal_handlers_disconnect_by_func (self->paintable, invalidate_contents, self);
      g_signal_handlers_disconnect_by_func (self->paintable, invalidate_size, self);
    }
  g_clear_object (&self->paintable);

  if (paintable != NULL)
    {
      self->paintable = g_object_ref (paintable);
      g_signal_connect (paintable, "invalidate-contents",
                        G_CALLBACK (invalidate_contents), self);
      g_signal_connect (paintable, "invalidate-size",
                        G_CALLBACK (invalidate_size), self);
    }

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_PAINTABLE]);
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

GdkPaintable *
bz_screenshot_get_paintable (BzScreenshot *self)
{
  g_return_val_if_fail (BZ_IS_SCREENSHOT (self), NULL);
  return self->paintable;
}

static void
invalidate_contents (GdkPaintable *paintable,
                     BzScreenshot *self)
{
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

static void
invalidate_size (GdkPaintable *paintable,
                 BzScreenshot *self)
{
  gtk_widget_queue_resize (GTK_WIDGET (self));
}
