/*******************************************************************/
/*                               XDMF                              */
/*                   eXtensible Data Model and Format              */
/*                                                                 */
/*  Id : $Id: XdmfDice.h,v 1.1 2002-12-02 17:11:03 clarke Exp $  */
/*  Date : $Date: 2002-12-02 17:11:03 $ */
/*  Version : $Revision: 1.1 $ */
/*                                                                 */
/*  Author:                                                        */
/*     Jerry A. Clarke                                             */
/*     clarke@arl.army.mil                                         */
/*     US Army Research Laboratory                                 */
/*     Aberdeen Proving Ground, MD                                 */
/*                                                                 */
/*     Copyright @ 2002 US Army Research Laboratory                */
/*     All Rights Reserved                                         */
/*     See Copyright.txt or http://www.arl.hpc.mil/ice for details */
/*                                                                 */
/*     This software is distributed WITHOUT ANY WARRANTY; without  */
/*     even the implied warranty of MERCHANTABILITY or FITNESS     */
/*     FOR A PARTICULAR PURPOSE.  See the above copyright notice   */
/*     for more information.                                       */
/*                                                                 */
/*******************************************************************/
#ifndef XDMF_LIST_INCLUDED
#define XDMF_LIST_INCLUDED

#include "ice.h"
#include "stdio.h"
#include "stdlib.h"
#include "malloc.h"

typedef unsigned long   XDMF_LIST_KEY;

#define XDMF_LIST_NDGM_SEMA_KEY    0x1000
#define XDMF_LIST_NDGM_LOCK_KEY    0x1001
#define XDMF_LIST_NDGM_BARRIER_KEY  0x1002
#define XDMF_LIST_NDGM_LAST_KEY    0x2000



typedef struct XdmfTree_Node_Struct {
  ICE_32_INT      nchild;
  ICE_PTR      data;
  struct XdmfTree_Node_Struct  *parent;
  struct XdmfTree_Node_Struct  **children;
  } XDMF_TREE_NODE; 

#define XDMF_TREE_NCHILD(a)    (a)->nchild
#define XDMF_TREE_CLIENT_DATA(a)  (a)->data
#define XDMF_TREE_PARENT(a)    (a)->parent
#define XDMF_TREE_CHILDREN(a)    (a)->children

typedef struct ICE_LLIST_ITEM {
  struct ICE_LLIST_ITEM    *prev;
  struct ICE_LLIST_ITEM    *next;
  double        data;  /* This Grows */
  } XdmfLlist;

typedef struct ICE_LLIST_ANCHOR_ITEM {
  XDMF_LIST_KEY      key;
  struct ICE_LLIST_ANCHOR_ITEM  *next;
  XdmfLlist      *member;
  } XdmfLlist_Anchor;

extern   int    XdmfLlist_init(XDMF_LIST_KEY key);
extern   void    *XdmfLlist_add_to_tail(XDMF_LIST_KEY key, XDMF_LIST_KEY body_size);
extern   void    *XdmfLlist_add_to_head(XDMF_LIST_KEY key, XDMF_LIST_KEY body_size);
extern   void    XdmfLlist_delete_item(XDMF_LIST_KEY key, void *item_to_delete);
extern   void    *XdmfLlist_first_member(XDMF_LIST_KEY key);
extern   void    *XdmfLlist_next_member(void *member);
extern   void    *XdmfLlist_add_before(XDMF_LIST_KEY key, void *item_after, XDMF_LIST_KEY body_size);
extern   void    *XdmfLlist_add_after(XDMF_LIST_KEY key, void *item_before, XDMF_LIST_KEY body_size);
extern   XDMF_LIST_KEY  XdmfLlist_new_key(void);


extern   XDMF_TREE_NODE       *XdmfTree_add_branch(XDMF_TREE_NODE *parent, XDMF_TREE_NODE *branch);
extern   XDMF_TREE_NODE  *XdmfTree_add(XDMF_TREE_NODE *parent, ICE_PTR data);
extern   ICE_VOID  XdmfTree_remove(XDMF_TREE_NODE *this_node,
        ICE_VOID (*client_data_free)(ICE_PTR));
extern   ICE_32_INT  XdmfTree_walk(XDMF_TREE_NODE *this_node,
        ICE_32_INT (*node_list_call)(XDMF_TREE_NODE *, void *),
        void *clientdata);


#endif /* XDMF_LIST_INCLUDED */
