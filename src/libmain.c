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
 * Author: Roberto Majadas <roberto.majadas@hispalinux.es>
 */


#include <config.h>
#include <string.h>
#include "nautilus-sendto-component.h"
#include <libbonobo.h>
#include <bonobo-activation/bonobo-activation.h>

#define VIEW_IID_SEND      "OAFIID:Nautilus_Sendto_Component_Send"


static CORBA_Object
nsc_shlib_make_object (PortableServer_POA poa,
			 const char *iid,
			 gpointer impl_ptr,
			 CORBA_Environment *ev)
{
	NautilusSendtoComponent *nsc;

	nsc = g_object_new (TYPE_NAUTILUS_SENDTO_COMPONENT, NULL);

	bonobo_activation_plugin_use (poa, impl_ptr);

	return CORBA_Object_duplicate (BONOBO_OBJREF (nsc), ev);
}

static const BonoboActivationPluginObject plugin_list[] = {
	{ VIEW_IID_SEND, nsc_shlib_make_object },
	{ NULL }
};

const BonoboActivationPlugin Bonobo_Plugin_info = {
	plugin_list,
	"Nautilus Sendto Component"
};
