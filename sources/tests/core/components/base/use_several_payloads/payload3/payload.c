/*
 * Payload with some content.
 */

/* ========================================================================
 * Copyright (C) 2012, KEDR development team
 * Copyright (C) 2010-2012, Institute for System Programming 
 *                          of the Russian Academy of Sciences (ISPRAS)
 * Authors: 
 *      Eugene A. Shatokhin <spectre@ispras.ru>
 *      Andrey V. Tsyvarev  <tsyvarev@ispras.ru>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 ======================================================================== */

#include <kedr/core/kedr.h>

#include <linux/module.h>
#include <linux/init.h>

MODULE_AUTHOR("Tsyvarev Andrey");
MODULE_LICENSE("GPL");

static struct kedr_pre_pair pre_pairs[] =
{
    {
        .orig = (void*)0x1001,
        .pre = (void*)0x4001
    },
    {
        .orig = (void*)0x1001,
        .pre = (void*)0x4002
    },
    {
        .orig = (void*)0x1003,
        .pre = (void*)0x4003
    },
    {
        .orig = NULL
    }
};

static struct kedr_replace_pair replace_pairs[] =
{
    {
        .orig = (void*)0x1001,
        .replace = (void*)0x5002
    },
    {
        .orig = NULL
    }
};


static struct kedr_payload payload =
{
    .mod = THIS_MODULE,
    
    .pre_pairs = pre_pairs,
    .replace_pairs = replace_pairs,
};

static int __init
payload_init(void)
{
    return kedr_payload_register(&payload);
}

static void __exit
payload_exit(void)
{
    kedr_payload_unregister(&payload);
}

module_init(payload_init);
module_exit(payload_exit);