/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* 
 * Copyright (C) 2004 Roberto Majadas
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Author:  Roberto Majadas  <roberto.majadas>
 */

#ifndef NAUTILUS_SENDTO_COMPONENT_H
#define NAUTILUS_SENDTO_COMPONENT_H

#include <libbonobo.h>

#define TYPE_NAUTILUS_SENDTO_COMPONENT	     (nautilus_sendto_component_get_type ())
#define NAUTILUS_SENDTO_COMPONENT(obj)	     (GTK_CHECK_CAST ((obj), TYPE_NAUTILUS_SENDTO_COMPONENT, NautilusSendtoComponent))
#define NAUTILUS_SENDTO_COMPONENT_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), TYPE_NAUTILUS_SENDTO_COMPONENT, NautilusSendtoComponentClass))
#define IS_NAUTILUS_SENDTO_COMPONENT(obj)	     (GTK_CHECK_TYPE ((obj), TYPE_NAUTILUS_SENDTO_COMPONENT))
#define IS_NAUTILUS_SENDTO_COMPONENT_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), TYPE_NAUTILUS_SENDTO_COMPONENT))

typedef struct {
	BonoboObject parent;
} NautilusSendtoComponent;

typedef struct {
	BonoboObjectClass parent;

	POA_Bonobo_Listener__epv epv;
} NautilusSendtoComponentClass;

GType nautilus_sendto_component_get_type (void);

#endif /* NAUTILUS_SENDTO_COMPONENT_H */
