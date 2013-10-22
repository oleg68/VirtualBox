# Copyright (c) 2001, Stanford University
# All rights reserved.
#
# See the file LICENSE.txt for information on redistributing this software.

import sys

import apiutil


apiutil.CopyrightC()

print """
/* DO NOT EDIT - THIS FILE AUTOMATICALLY GENERATED BY server_retval.py SCRIPT */
#include "chromium.h"
#include "cr_mem.h"
#include "cr_net.h"
#include "server_dispatch.h"
#include "server.h"

void crServerReturnValue( const void *payload, unsigned int payload_len )
{
    CRMessageReadback *rb;
    int msg_len = sizeof( *rb ) + payload_len;

    /* Don't reply to client if we're loading VM snapshot*/
    if (cr_server.bIsInLoadingState)
        return;

    if (cr_server.curClient->conn->type == CR_FILE)
    {
        return;
    }

    rb = (CRMessageReadback *) crAlloc( msg_len );

    rb->header.type = CR_MESSAGE_READBACK;
    CRDBGPTR_PRINTRB(cr_server.curClient->conn->u32ClientID, &cr_server.writeback_ptr);
    CRDBGPTR_CHECKNZ(&cr_server.writeback_ptr);
    CRDBGPTR_CHECKNZ(&cr_server.return_ptr);
    crMemcpy( &(rb->writeback_ptr), &(cr_server.writeback_ptr), sizeof( rb->writeback_ptr ) );
    crMemcpy( &(rb->readback_ptr), &(cr_server.return_ptr), sizeof( rb->readback_ptr ) );
    crMemcpy( rb+1, payload, payload_len );
    crNetSend( cr_server.curClient->conn, NULL, rb, msg_len );
    CRDBGPTR_SETZ(&cr_server.writeback_ptr);
    CRDBGPTR_SETZ(&cr_server.return_ptr);
    crFree( rb );
}
"""

keys = apiutil.GetDispatchedFunctions(sys.argv[1]+"/APIspec.txt")

for func_name in keys:
    params = apiutil.Parameters(func_name)
    return_type = apiutil.ReturnType(func_name)
    if apiutil.FindSpecial( "server", func_name ):
        continue
    if "VBox" == apiutil.Category(func_name):
        continue
    if return_type != 'void':
        print '%s SERVER_DISPATCH_APIENTRY crServerDispatch%s( %s )' % ( return_type, func_name, apiutil.MakeDeclarationString(params))
        print '{'
        print '\t%s retval;' % return_type
        print '\tretval = cr_server.head_spu->dispatch_table.%s( %s );' % (func_name, apiutil.MakeCallString(params) );
        print '\tcrServerReturnValue( &retval, sizeof(retval) );'
        print '\treturn retval; /* WILL PROBABLY BE IGNORED */'
        print '}'
