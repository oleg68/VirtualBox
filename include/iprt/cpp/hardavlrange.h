/** @file
 * IPRT - Hardened AVL tree, unique key ranges.
 */

/*
 * Copyright (C) 2022 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 *
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL) only, as it comes in the "COPYING.CDDL" file of the
 * VirtualBox OSE distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 */

#ifndef IPRT_INCLUDED_cpp_hardavlrange_h
#define IPRT_INCLUDED_cpp_hardavlrange_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/cpp/hardavlslaballocator.h>

/** @defgroup grp_rt_cpp_hardavl    Hardened AVL Trees
 * @{
 */

/**
 * Check that the tree heights make sense for the current node.
 *
 * This is a RT_STRICT test as it's expensive and we should have sufficient
 * other checks to ensure safe AVL tree operation.
 *
 * @note the a_cStackEntries parameter is a hack to avoid running into gcc's
 *       "the address of 'AVLStack' will never be NULL" errors.
 */
#ifdef RT_STRICT
# define RTHARDAVL_STRICT_CHECK_HEIGHTS(a_pNode, a_pAvlStack, a_cStackEntries) do { \
        NodeType * const pLeftNodeX    = a_pAllocator->ptrFromInt((a_pNode)->idxLeft); \
        AssertReturnStmt(a_pAllocator->isPtrRetOkay(pLeftNodeX), m_cErrors++, a_pAllocator->ptrErrToStatus((a_pNode))); \
        NodeType * const pRightNodeX   = a_pAllocator->ptrFromInt((a_pNode)->idxRight); \
        AssertReturnStmt(a_pAllocator->isPtrRetOkay(pRightNodeX), m_cErrors++, a_pAllocator->ptrErrToStatus((a_pNode))); \
        uint8_t const    cLeftHeightX  = pLeftNodeX  ? pLeftNodeX->cHeight  : 0; \
        uint8_t const    cRightHeightX = pRightNodeX ? pRightNodeX->cHeight : 0; \
        if (RT_LIKELY((a_pNode)->cHeight == RT_MAX(cLeftHeightX, cRightHeightX) + 1)) { /*likely*/ } \
        else \
        { \
            RTAssertMsg2("line %u: %u l=%u r=%u\n", __LINE__, (a_pNode)->cHeight, cLeftHeightX, cRightHeightX); \
            if ((a_cStackEntries)) dumpStack(a_pAllocator, (a_pAvlStack)); \
            AssertMsgReturnStmt((a_pNode)->cHeight == RT_MAX(cLeftHeightX, cRightHeightX) + 1, \
                                ("%u l=%u r=%u\n", (a_pNode)->cHeight, cLeftHeightX, cRightHeightX), \
                                m_cErrors++, VERR_HARDAVL_BAD_HEIGHT); \
        } \
        AssertMsgReturnStmt(RT_ABS(cLeftHeightX - cRightHeightX) <= 1, ("l=%u r=%u\n", cLeftHeightX, cRightHeightX), \
                            m_cErrors++, VERR_HARDAVL_UNBALANCED); \
        Assert(!pLeftNodeX  || pLeftNodeX->Key < (a_pNode)->Key); \
        Assert(!pRightNodeX || pRightNodeX->Key > (a_pNode)->Key); \
    } while (0)
#else
# define RTHARDAVL_STRICT_CHECK_HEIGHTS(a_pNode, a_pAvlStack, a_cStackEntries) do { } while (0)
#endif


/**
 * Hardened AVL tree for nodes with key ranges.
 *
 * This is very crude and therefore expects the NodeType to feature:
 *      - Key and KeyLast members of KeyType.
 *      - idxLeft and idxRight members with type uint32_t.
 *      - cHeight members of type uint8_t.
 *
 * The code is very C-ish because of it's sources and initial use (ring-0
 * without C++ exceptions enabled).
 */
template<typename NodeType, typename KeyType>
struct RTCHardAvlRangeTree
{
    /** The root index. */
    uint32_t m_idxRoot;
    /** The error count. */
    uint32_t m_cErrors;

    /** The max stack depth. */
    enum { kMaxStack = 28 };
    /** The max height value we allow. */
    enum { kMaxHeight = kMaxStack + 1 };

    /** A stack used internally to avoid recursive calls.
     * This is used with operations invoking i_rebalance(). */
    typedef struct HardAvlStack
    {
        /** Number of entries on the stack.   */
        unsigned        cEntries;
        /** The stack. */
        uint32_t       *apidxEntries[kMaxStack];
    } HardAvlStack;

    /** @name Key comparisons
     * @{ */
    static inline int areKeyRangesIntersecting(KeyType a_Key1First, KeyType a_Key2First,
                                               KeyType a_Key1Last, KeyType a_Key2Last) RT_NOEXCEPT
    {
        return a_Key1First <= a_Key2Last && a_Key1Last >= a_Key2First;
    }

    static inline int isKeyInRange(KeyType a_Key, KeyType a_KeyFirst, KeyType a_KeyLast) RT_NOEXCEPT
    {
        return a_Key <= a_KeyLast && a_Key >= a_KeyFirst;
    }

    static inline int isKeyGreater(KeyType a_Key1, KeyType a_Key2) RT_NOEXCEPT
    {
        return a_Key1 > a_Key2;
    }
    /** @} */

    RTCHardAvlRangeTree()
        : m_idxRoot(0)
        , m_cErrors(0)
    { }

    RTCHardAvlRangeTree(RTCHardAvlTreeSlabAllocator<NodeType> *a_pAllocator)
        : m_idxRoot(a_pAllocator->kNilIndex)
        , m_cErrors(0)
    { }

    /**
     * Inserts a node into the AVL-tree.
     *
     * @returns   IPRT status code.
     * @retval    VERR_ALREADY_EXISTS if a node with overlapping key range exists.
     *
     * @param     a_pAllocator  Pointer to the allocator.
     * @param     a_pNode       Pointer to the node which is to be added.
     *
     * @code
     *            Find the location of the node (using binary tree algorithm.):
     *            LOOP until KAVL_NULL leaf pointer
     *            BEGIN
     *                Add node pointer pointer to the AVL-stack.
     *                IF new-node-key < node key THEN
     *                    left
     *                ELSE
     *                    right
     *            END
     *            Fill in leaf node and insert it.
     *            Rebalance the tree.
     * @endcode
     */
    int insert(RTCHardAvlTreeSlabAllocator<NodeType> *a_pAllocator, NodeType *a_pNode)
    {
        KeyType const Key     = a_pNode->Key;
        KeyType const KeyLast = a_pNode->KeyLast;
        AssertMsgReturn(Key <= KeyLast, ("Key=%#RX64 KeyLast=%#RX64\n", (uint64_t)Key, (uint64_t)KeyLast),
                        VERR_HARDAVL_INSERT_INVALID_KEY_RANGE);

        uint32_t     *pidxCurNode = &m_idxRoot;
        HardAvlStack  AVLStack;
        AVLStack.cEntries = 0;
        for (;;)
        {
            NodeType *pCurNode = a_pAllocator->ptrFromInt(*pidxCurNode);
            AssertMsgReturnStmt(a_pAllocator->isPtrRetOkay(pCurNode), ("*pidxCurNode=%#x pCurNode=%p\n", *pidxCurNode, pCurNode),
                                m_cErrors++, a_pAllocator->ptrErrToStatus(pCurNode));
            if (!pCurNode)
                break;

            unsigned const cEntries = AVLStack.cEntries;
            AssertMsgReturnStmt(cEntries < RT_ELEMENTS(AVLStack.apidxEntries),
                                ("%p[%#x/%p] %p[%#x] %p[%#x] %p[%#x] %p[%#x] %p[%#x]\n", pidxCurNode, *pidxCurNode, pCurNode,
                                 AVLStack.apidxEntries[RT_ELEMENTS(AVLStack.apidxEntries) - 1], *AVLStack.apidxEntries[RT_ELEMENTS(AVLStack.apidxEntries) - 1],
                                 AVLStack.apidxEntries[RT_ELEMENTS(AVLStack.apidxEntries) - 2], *AVLStack.apidxEntries[RT_ELEMENTS(AVLStack.apidxEntries) - 2],
                                 AVLStack.apidxEntries[RT_ELEMENTS(AVLStack.apidxEntries) - 3], *AVLStack.apidxEntries[RT_ELEMENTS(AVLStack.apidxEntries) - 3],
                                 AVLStack.apidxEntries[RT_ELEMENTS(AVLStack.apidxEntries) - 4], *AVLStack.apidxEntries[RT_ELEMENTS(AVLStack.apidxEntries) - 4],
                                 AVLStack.apidxEntries[RT_ELEMENTS(AVLStack.apidxEntries) - 5], *AVLStack.apidxEntries[RT_ELEMENTS(AVLStack.apidxEntries) - 5]),
                                m_cErrors++, VERR_HARDAVL_STACK_OVERFLOW);
            AVLStack.apidxEntries[cEntries] = pidxCurNode;
            AVLStack.cEntries = cEntries + 1;

            RTHARDAVL_STRICT_CHECK_HEIGHTS(pCurNode, &AVLStack, AVLStack.cEntries);

            /* Range check: */
            if (areKeyRangesIntersecting(pCurNode->Key, Key, pCurNode->KeyLast, KeyLast))
                return VERR_ALREADY_EXISTS;

            /* Descend: */
            if (isKeyGreater(pCurNode->Key, Key))
                pidxCurNode = &pCurNode->idxLeft;
            else
                pidxCurNode = &pCurNode->idxRight;
        }

        a_pNode->idxLeft  = a_pAllocator->kNilIndex;
        a_pNode->idxRight = a_pAllocator->kNilIndex;
        a_pNode->cHeight  = 1;

        uint32_t const idxNode = a_pAllocator->ptrToInt(a_pNode);
        AssertMsgReturn(a_pAllocator->isIdxRetOkay(idxNode),  ("pNode=%p idxNode=%#x\n", a_pNode, idxNode),
                        a_pAllocator->idxErrToStatus(idxNode));
        *pidxCurNode = idxNode;

        return i_rebalance(a_pAllocator, &AVLStack);
    }

#ifdef RT_STRICT

    static void dumpStack(RTCHardAvlTreeSlabAllocator<NodeType> *a_pAllocator, HardAvlStack const *pStack)
    {
        uint32_t const * const *paidx = pStack->apidxEntries;
        RTAssertMsg2("stack: %u:\n", pStack->cEntries);
        for (unsigned i = 0; i < pStack->cEntries; i++)
        {
            uint32_t idx     = *paidx[i];
            uint32_t idxNext = i + 1 < pStack->cEntries ? *paidx[i + 1] : UINT32_MAX;
            NodeType const *pNode = a_pAllocator->ptrFromInt(idx);
            RTAssertMsg2(" #%02u: %p[%#06x] pNode=%p h=%02d l=%#06x%c r=%#06x%c\n", i, paidx[i], idx, pNode, pNode->cHeight,
                         pNode->idxLeft,  pNode->idxLeft  == idxNext ? '*' : ' ',
                         pNode->idxRight, pNode->idxRight == idxNext ? '*' : ' ');
        }
    }

    static void printTree(RTCHardAvlTreeSlabAllocator<NodeType> *a_pAllocator, uint32_t a_idxRoot, unsigned iLevel = 0)
    {
        if (a_idxRoot == a_pAllocator->kNilIndex)
            RTAssertMsg2("%*snil\n", iLevel * 6, "");
        else if (iLevel < 7)
        {
            NodeType *pNode = a_pAllocator->ptrFromInt(a_idxRoot);
            printTree(a_pAllocator, pNode->idxRight, iLevel + 1);
            RTAssertMsg2("%*s%#x/%u\n", iLevel * 6, "", a_idxRoot, pNode->cHeight);
            printTree(a_pAllocator, pNode->idxLeft, iLevel + 1);
        }
        else
            RTAssertMsg2("%*stoo deep\n", iLevel * 6, "");
    }

#endif

    /**
     * Removes a node from the AVL-tree by a key value.
     *
     * @returns   IPRT status code.
     * @retval    VERR_NOT_FOUND if not found.
     * @param     a_pAllocator  Pointer to the allocator.
     * @param     a_Key         A key value in the range of the node to be removed.
     * @param     a_ppRemoved   Where to return the pointer to the removed node.
     *
     * @code
     *            Find the node which is to be removed:
     *            LOOP until not found
     *            BEGIN
     *                Add node pointer pointer to the AVL-stack.
     *                IF the keys matches THEN break!
     *                IF remove key < node key THEN
     *                    left
     *                ELSE
     *                    right
     *            END
     *            IF found THEN
     *            BEGIN
     *                IF left node not empty THEN
     *                BEGIN
     *                    Find the right most node in the left tree while adding the pointer to the pointer to it's parent to the stack:
     *                    Start at left node.
     *                    LOOP until right node is empty
     *                    BEGIN
     *                        Add to stack.
     *                        go right.
     *                    END
     *                    Link out the found node.
     *                    Replace the node which is to be removed with the found node.
     *                    Correct the stack entry for the pointer to the left tree.
     *                END
     *                ELSE
     *                BEGIN
     *                    Move up right node.
     *                    Remove last stack entry.
     *                END
     *                Balance tree using stack.
     *            END
     *            return pointer to the removed node (if found).
     * @endcode
     */
    int remove(RTCHardAvlTreeSlabAllocator<NodeType> *a_pAllocator, KeyType a_Key, NodeType **a_ppRemoved)
    {
        *a_ppRemoved = NULL;

        /*
         * Walk the tree till we locate the node that is to be deleted.
         */
        uint32_t     *pidxDeleteNode = &m_idxRoot;
        NodeType     *pDeleteNode;
        HardAvlStack  AVLStack;
        AVLStack.cEntries = 0;
        for (;;)
        {
            pDeleteNode = a_pAllocator->ptrFromInt(*pidxDeleteNode);
            AssertMsgReturnStmt(a_pAllocator->isPtrRetOkay(pDeleteNode),
                                ("*pidxCurNode=%#x pDeleteNode=%p\n", *pidxDeleteNode, pDeleteNode),
                                m_cErrors++, a_pAllocator->ptrErrToStatus(pDeleteNode));
            if (pDeleteNode)
            { /*likely*/ }
            else
                return VERR_NOT_FOUND;

            unsigned const cEntries = AVLStack.cEntries;
            AssertMsgReturnStmt(cEntries < RT_ELEMENTS(AVLStack.apidxEntries),
                                ("%p[%#x/%p] %p[%#x] %p[%#x] %p[%#x] %p[%#x] %p[%#x]\n",
                                 pidxDeleteNode, *pidxDeleteNode, pDeleteNode,
                                 AVLStack.apidxEntries[RT_ELEMENTS(AVLStack.apidxEntries) - 1], *AVLStack.apidxEntries[RT_ELEMENTS(AVLStack.apidxEntries) - 1],
                                 AVLStack.apidxEntries[RT_ELEMENTS(AVLStack.apidxEntries) - 2], *AVLStack.apidxEntries[RT_ELEMENTS(AVLStack.apidxEntries) - 2],
                                 AVLStack.apidxEntries[RT_ELEMENTS(AVLStack.apidxEntries) - 3], *AVLStack.apidxEntries[RT_ELEMENTS(AVLStack.apidxEntries) - 3],
                                 AVLStack.apidxEntries[RT_ELEMENTS(AVLStack.apidxEntries) - 4], *AVLStack.apidxEntries[RT_ELEMENTS(AVLStack.apidxEntries) - 4],
                                 AVLStack.apidxEntries[RT_ELEMENTS(AVLStack.apidxEntries) - 5], *AVLStack.apidxEntries[RT_ELEMENTS(AVLStack.apidxEntries) - 5]),
                                m_cErrors++, VERR_HARDAVL_STACK_OVERFLOW);
            AVLStack.apidxEntries[cEntries] = pidxDeleteNode;
            AVLStack.cEntries = cEntries + 1;

            RTHARDAVL_STRICT_CHECK_HEIGHTS(pDeleteNode, &AVLStack, AVLStack.cEntries);

            /* Range check: */
            if (isKeyInRange(a_Key, pDeleteNode->Key, pDeleteNode->KeyLast))
                break;

            /* Descend: */
            if (isKeyGreater(pDeleteNode->Key, a_Key))
                pidxDeleteNode = &pDeleteNode->idxLeft;
            else
                pidxDeleteNode = &pDeleteNode->idxRight;
        }

        /*
         * Do the deletion.
         */
        uint32_t const idxDeleteLeftNode = pDeleteNode->idxLeft;
        if (idxDeleteLeftNode != a_pAllocator->kNilIndex)
        {
            /*
             * Replace the deleted node with the rightmost node in the left subtree.
             */
            NodeType * const  pDeleteLeftNode   = a_pAllocator->ptrFromInt(idxDeleteLeftNode);
            AssertMsgReturnStmt(a_pAllocator->isPtrRetOkay(pDeleteLeftNode),
                                ("idxDeleteLeftNode=%#x pDeleteLeftNode=%p\n", idxDeleteLeftNode, pDeleteLeftNode),
                                m_cErrors++, a_pAllocator->ptrErrToStatus(pDeleteLeftNode));

            uint32_t const   idxDeleteRightNode = pDeleteNode->idxRight;
            AssertReturnStmt(a_pAllocator->isIntValid(idxDeleteRightNode), m_cErrors++, VERR_HARDAVL_INDEX_OUT_OF_BOUNDS);

            const unsigned   iStackEntry = AVLStack.cEntries;

            uint32_t *pidxLeftBiggest    = &pDeleteNode->idxLeft;
            uint32_t  idxLeftBiggestNode = idxDeleteLeftNode;
            NodeType *pLeftBiggestNode   = pDeleteLeftNode;
            RTHARDAVL_STRICT_CHECK_HEIGHTS(pLeftBiggestNode, &AVLStack, AVLStack.cEntries);

            while (pLeftBiggestNode->idxRight != a_pAllocator->kNilIndex)
            {
                unsigned const cEntries = AVLStack.cEntries;
                AssertMsgReturnStmt(cEntries < RT_ELEMENTS(AVLStack.apidxEntries),
                                    ("%p[%#x/%p] %p[%#x] %p[%#x] %p[%#x] %p[%#x] %p[%#x]\n",
                                     pidxLeftBiggest, *pidxLeftBiggest, pLeftBiggestNode,
                                     AVLStack.apidxEntries[RT_ELEMENTS(AVLStack.apidxEntries) - 1], *AVLStack.apidxEntries[RT_ELEMENTS(AVLStack.apidxEntries) - 1],
                                     AVLStack.apidxEntries[RT_ELEMENTS(AVLStack.apidxEntries) - 2], *AVLStack.apidxEntries[RT_ELEMENTS(AVLStack.apidxEntries) - 2],
                                     AVLStack.apidxEntries[RT_ELEMENTS(AVLStack.apidxEntries) - 3], *AVLStack.apidxEntries[RT_ELEMENTS(AVLStack.apidxEntries) - 3],
                                     AVLStack.apidxEntries[RT_ELEMENTS(AVLStack.apidxEntries) - 4], *AVLStack.apidxEntries[RT_ELEMENTS(AVLStack.apidxEntries) - 4],
                                     AVLStack.apidxEntries[RT_ELEMENTS(AVLStack.apidxEntries) - 5], *AVLStack.apidxEntries[RT_ELEMENTS(AVLStack.apidxEntries) - 5]),
                                    m_cErrors++, VERR_HARDAVL_STACK_OVERFLOW);
                AVLStack.apidxEntries[cEntries] = pidxLeftBiggest;
                AVLStack.cEntries = cEntries + 1;

                pidxLeftBiggest    = &pLeftBiggestNode->idxRight;
                idxLeftBiggestNode = pLeftBiggestNode->idxRight;
                pLeftBiggestNode   = a_pAllocator->ptrFromInt(idxLeftBiggestNode);
                AssertMsgReturnStmt(a_pAllocator->isPtrRetOkay(pLeftBiggestNode),
                                    ("idxLeftBiggestNode=%#x pLeftBiggestNode=%p\n", idxLeftBiggestNode, pLeftBiggestNode),
                                    m_cErrors++, a_pAllocator->ptrErrToStatus(pLeftBiggestNode));
                RTHARDAVL_STRICT_CHECK_HEIGHTS(pLeftBiggestNode, &AVLStack, AVLStack.cEntries);
            }

            uint32_t const idxLeftBiggestLeftNode = pLeftBiggestNode->idxLeft;
            AssertReturnStmt(a_pAllocator->isIntValid(idxLeftBiggestLeftNode), m_cErrors++, VERR_HARDAVL_INDEX_OUT_OF_BOUNDS);

            /* link out pLeftBiggestNode */
            *pidxLeftBiggest = idxLeftBiggestLeftNode;

            /* link it in place of the deleted node. */
            if (idxDeleteLeftNode != idxLeftBiggestNode)
                pLeftBiggestNode->idxLeft = idxDeleteLeftNode;
            pLeftBiggestNode->idxRight    = idxDeleteRightNode;
            pLeftBiggestNode->cHeight     = AVLStack.cEntries > iStackEntry ? pDeleteNode->cHeight : 0;

            *pidxDeleteNode = idxLeftBiggestNode;

            if (AVLStack.cEntries > iStackEntry)
                AVLStack.apidxEntries[iStackEntry] = &pLeftBiggestNode->idxLeft;
        }
        else
        {
            /* No left node, just pull up the right one. */
            uint32_t const idxDeleteRightNode = pDeleteNode->idxRight;
            AssertReturnStmt(a_pAllocator->isIntValid(idxDeleteRightNode), m_cErrors++, VERR_HARDAVL_INDEX_OUT_OF_BOUNDS);
            *pidxDeleteNode = idxDeleteRightNode;
            AVLStack.cEntries--;
        }
        *a_ppRemoved = pDeleteNode;

        return i_rebalance(a_pAllocator, &AVLStack);
    }

    /**
     * Looks up a node from the tree.
     *
     * @returns   IPRT status code.
     * @retval    VERR_NOT_FOUND if not found.
     *
     * @param     a_pAllocator  Pointer to the allocator.
     * @param     a_Key         A key value in the range of the desired node.
     * @param     a_ppFound     Where to return the pointer to the node.
     */
    int lookup(RTCHardAvlTreeSlabAllocator<NodeType> *a_pAllocator, KeyType a_Key, NodeType **a_ppFound)
    {
        *a_ppFound = NULL;

        NodeType *pNode = a_pAllocator->ptrFromInt(m_idxRoot);
        AssertMsgReturnStmt(a_pAllocator->isPtrRetOkay(pNode), ("m_idxRoot=%#x pNode=%p\n", m_idxRoot, pNode),
                            m_cErrors++, a_pAllocator->ptrErrToStatus(pNode));
#ifdef RT_STRICT
        HardAvlStack  AVLStack;
        AVLStack.apidxEntries[0] = &m_idxRoot;
        AVLStack.cEntries = 1;
#endif
        unsigned cDepth = 0;
        while (pNode)
        {
            RTHARDAVL_STRICT_CHECK_HEIGHTS(pNode, &AVLStack, AVLStack.cEntries);
            AssertReturn(cDepth <= kMaxHeight, VERR_HARDAVL_LOOKUP_TOO_DEEP);
            cDepth++;

            if (isKeyInRange(a_Key, pNode->Key, pNode->KeyLast))
            {
                *a_ppFound = pNode;
                return VINF_SUCCESS;
            }
            if (isKeyGreater(pNode->Key, a_Key))
            {
#ifdef RT_STRICT
                AVLStack.apidxEntries[AVLStack.cEntries++] = &pNode->idxLeft;
#endif
                uint32_t const idxLeft = pNode->idxLeft;
                pNode = a_pAllocator->ptrFromInt(idxLeft);
                AssertMsgReturnStmt(a_pAllocator->isPtrRetOkay(pNode), ("idxLeft=%#x pNode=%p\n", idxLeft, pNode),
                                    m_cErrors++, a_pAllocator->ptrErrToStatus(pNode));
            }
            else
            {
#ifdef RT_STRICT
                AVLStack.apidxEntries[AVLStack.cEntries++] = &pNode->idxRight;
#endif
                uint32_t const idxRight = pNode->idxRight;
                pNode = a_pAllocator->ptrFromInt(idxRight);
                AssertMsgReturnStmt(a_pAllocator->isPtrRetOkay(pNode), ("idxRight=%#x pNode=%p\n", idxRight, pNode),
                                    m_cErrors++, a_pAllocator->ptrErrToStatus(pNode));
            }
        }

        return VERR_NOT_FOUND;
    }

    /**
     * A callback for doWithAllFromLeft and doWithAllFromRight.
     *
     * @returns IPRT status code.  Any non-zero status causes immediate return from
     *          the enumeration function.
     * @param   pNode   The current node.
     * @param   pvUser  The user argument.
     */
    typedef DECLCALLBACKTYPE(int, FNCALLBACK,(NodeType *pNode, void *pvUser));
    /** Pointer to a callback for doWithAllFromLeft and doWithAllFromRight. */
    typedef FNCALLBACK *PFNCALLBACK;

    /**
     * Iterates thru all nodes in the tree from left (smaller) to right.
     *
     * @returns   IPRT status code.
     *
     * @param     a_pAllocator  Pointer to the allocator.
     * @param     a_pfnCallBack Pointer to callback function.
     * @param     a_pvUser      Callback user argument.
     */
    int doWithAllFromLeft(RTCHardAvlTreeSlabAllocator<NodeType> *a_pAllocator, PFNCALLBACK a_pfnCallBack, void *a_pvUser)
    {
        NodeType *pNode = a_pAllocator->ptrFromInt(m_idxRoot);
        AssertMsgReturnStmt(a_pAllocator->isPtrRetOkay(pNode), ("m_idxRoot=%#x pNode=%p\n", m_idxRoot, pNode),
                            m_cErrors++, a_pAllocator->ptrErrToStatus(pNode));
        if (!pNode)
            return VINF_SUCCESS;

        /*
         * We simulate recursive calling here.  For safety reasons, we do not
         * pop before going down the right tree like the original code did.
         */
        uint32_t  cNodesLeft = a_pAllocator->m_cNodes;
        NodeType *apEntries[kMaxStack];
        uint8_t   abState[kMaxStack];
        unsigned  cEntries = 1;
        abState[0]   = 0;
        apEntries[0] = pNode;
        while (cEntries > 0)
        {
            pNode = apEntries[cEntries - 1];
            switch (abState[cEntries - 1])
            {
                /* Go left. */
                case 0:
                {
                    abState[cEntries - 1] = 1;

                    NodeType * const pLeftNode = a_pAllocator->ptrFromInt(pNode->idxLeft);
                    AssertMsgReturnStmt(a_pAllocator->isPtrRetOkay(pLeftNode),
                                        ("idxLeft=%#x pLeftNode=%p\n", pNode->idxLeft, pLeftNode),
                                        m_cErrors++, a_pAllocator->ptrErrToStatus(pLeftNode));
                    if (pLeftNode)
                    {
#if RT_GNUC_PREREQ_EX(4,7, 1) /* 32-bit 4.4.7 has trouble, dunno when it started working exactly */
                        AssertCompile(kMaxStack > 6);
#endif
                        AssertMsgReturnStmt(cEntries < RT_ELEMENTS(apEntries),
                                            ("%p[%#x] %p %p %p %p %p %p\n", pLeftNode, pNode->idxLeft, apEntries[kMaxStack - 1],
                                             apEntries[kMaxStack - 2], apEntries[kMaxStack - 3], apEntries[kMaxStack - 4],
                                             apEntries[kMaxStack - 5], apEntries[kMaxStack - 6]),
                                            m_cErrors++, VERR_HARDAVL_STACK_OVERFLOW);
                        apEntries[cEntries] = pLeftNode;
                        abState[cEntries]   = 0;
                        cEntries++;

                        AssertReturn(cNodesLeft > 0, VERR_HARDAVL_TRAVERSED_TOO_MANY_NODES);
                        cNodesLeft--;
                        break;
                    }
                    RT_FALL_THROUGH();
                }

                /* center then right. */
                case 1:
                {
                    abState[cEntries - 1] = 2;

                    RTHARDAVL_STRICT_CHECK_HEIGHTS(pNode, NULL, 0);

                    int rc = a_pfnCallBack(pNode, a_pvUser);
                    if (rc != VINF_SUCCESS)
                        return rc;

                    NodeType * const pRightNode = a_pAllocator->ptrFromInt(pNode->idxRight);
                    AssertMsgReturnStmt(a_pAllocator->isPtrRetOkay(pRightNode),
                                        ("idxRight=%#x pRightNode=%p\n", pNode->idxRight, pRightNode),
                                        m_cErrors++, a_pAllocator->ptrErrToStatus(pRightNode));
                    if (pRightNode)
                    {
#if RT_GNUC_PREREQ_EX(4,7, 1) /* 32-bit 4.4.7 has trouble, dunno when it started working exactly */
                        AssertCompile(kMaxStack > 6);
#endif
                        AssertMsgReturnStmt(cEntries < RT_ELEMENTS(apEntries),
                                            ("%p[%#x] %p %p %p %p %p %p\n", pRightNode, pNode->idxRight, apEntries[kMaxStack - 1],
                                             apEntries[kMaxStack - 2], apEntries[kMaxStack - 3], apEntries[kMaxStack - 4],
                                             apEntries[kMaxStack - 5], apEntries[kMaxStack - 6]),
                                            m_cErrors++, VERR_HARDAVL_STACK_OVERFLOW);
                        apEntries[cEntries] = pRightNode;
                        abState[cEntries]   = 0;
                        cEntries++;

                        AssertReturn(cNodesLeft > 0, VERR_HARDAVL_TRAVERSED_TOO_MANY_NODES);
                        cNodesLeft--;
                        break;
                    }
                    RT_FALL_THROUGH();
                }

                default:
                    /* pop it. */
                    cEntries -= 1;
                    break;
            }
        }
        return VINF_SUCCESS;
    }

    /**
     * A callback for destroy to do additional cleanups before the node is freed.
     *
     * @param   pNode   The current node.
     * @param   pvUser  The user argument.
     */
    typedef DECLCALLBACKTYPE(void, FNDESTROYCALLBACK,(NodeType *pNode, void *pvUser));
    /** Pointer to a callback for destroy. */
    typedef FNDESTROYCALLBACK *PFNDESTROYCALLBACK;

    /**
     * Destroys the tree, starting with the root node.
     *
     * This will invoke the freeNode() method on the allocate for every node after
     * first doing the callback to let the caller free additional resources
     * referenced by the node.
     *
     * @returns IPRT status code.
     *
     * @param     a_pAllocator  Pointer to the allocator.
     * @param     a_pfnCallBack Pointer to callback function.  Optional.
     * @param     a_pvUser      Callback user argument.
     *
     * @note      This is mostly the same code as the doWithAllFromLeft().
     */
    int destroy(RTCHardAvlTreeSlabAllocator<NodeType> *a_pAllocator, PFNDESTROYCALLBACK a_pfnCallBack = NULL, void *a_pvUser = NULL)
    {
        NodeType *pNode = a_pAllocator->ptrFromInt(m_idxRoot);
        AssertMsgReturnStmt(a_pAllocator->isPtrRetOkay(pNode), ("m_idxRoot=%#x pNode=%p\n", m_idxRoot, pNode),
                            m_cErrors++, a_pAllocator->ptrErrToStatus(pNode));
        if (!pNode)
            return VINF_SUCCESS;

        /*
         * We simulate recursive calling here.  For safety reasons, we do not
         * pop before going down the right tree like the original code did.
         */
        uint32_t  cNodesLeft = a_pAllocator->m_cNodes;
        NodeType *apEntries[kMaxStack];
        uint8_t   abState[kMaxStack];
        unsigned  cEntries = 1;
        abState[0]   = 0;
        apEntries[0] = pNode;
        while (cEntries > 0)
        {
            pNode = apEntries[cEntries - 1];
            switch (abState[cEntries - 1])
            {
                /* Go left. */
                case 0:
                {
                    abState[cEntries - 1] = 1;

                    NodeType * const pLeftNode = a_pAllocator->ptrFromInt(pNode->idxLeft);
                    AssertMsgReturnStmt(a_pAllocator->isPtrRetOkay(pLeftNode),
                                        ("idxLeft=%#x pLeftNode=%p\n", pNode->idxLeft, pLeftNode),
                                        m_cErrors++, a_pAllocator->ptrErrToStatus(pLeftNode));
                    if (pLeftNode)
                    {
#if RT_GNUC_PREREQ_EX(4,7, 1) /* 32-bit 4.4.7 has trouble, dunno when it started working exactly */
                        AssertCompile(kMaxStack > 6);
#endif
                        AssertMsgReturnStmt(cEntries < RT_ELEMENTS(apEntries),
                                            ("%p[%#x] %p %p %p %p %p %p\n", pLeftNode, pNode->idxLeft, apEntries[kMaxStack - 1],
                                             apEntries[kMaxStack - 2], apEntries[kMaxStack - 3], apEntries[kMaxStack - 4],
                                             apEntries[kMaxStack - 5], apEntries[kMaxStack - 6]),
                                            m_cErrors++, VERR_HARDAVL_STACK_OVERFLOW);
                        apEntries[cEntries] = pLeftNode;
                        abState[cEntries]   = 0;
                        cEntries++;

                        AssertReturn(cNodesLeft > 0, VERR_HARDAVL_TRAVERSED_TOO_MANY_NODES);
                        cNodesLeft--;
                        break;
                    }
                    RT_FALL_THROUGH();
                }

                /* right. */
                case 1:
                {
                    abState[cEntries - 1] = 2;

                    NodeType * const pRightNode = a_pAllocator->ptrFromInt(pNode->idxRight);
                    AssertMsgReturnStmt(a_pAllocator->isPtrRetOkay(pRightNode),
                                        ("idxRight=%#x pRightNode=%p\n", pNode->idxRight, pRightNode),
                                        m_cErrors++, a_pAllocator->ptrErrToStatus(pRightNode));
                    if (pRightNode)
                    {
#if RT_GNUC_PREREQ_EX(4,7, 1) /* 32-bit 4.4.7 has trouble, dunno when it started working exactly */
                        AssertCompile(kMaxStack > 6);
#endif
                        AssertMsgReturnStmt(cEntries < RT_ELEMENTS(apEntries),
                                            ("%p[%#x] %p %p %p %p %p %p\n", pRightNode, pNode->idxRight, apEntries[kMaxStack - 1],
                                             apEntries[kMaxStack - 2], apEntries[kMaxStack - 3], apEntries[kMaxStack - 4],
                                             apEntries[kMaxStack - 5], apEntries[kMaxStack - 6]),
                                            m_cErrors++, VERR_HARDAVL_STACK_OVERFLOW);
                        apEntries[cEntries] = pRightNode;
                        abState[cEntries]   = 0;
                        cEntries++;

                        AssertReturn(cNodesLeft > 0, VERR_HARDAVL_TRAVERSED_TOO_MANY_NODES);
                        cNodesLeft--;
                        break;
                    }
                    RT_FALL_THROUGH();
                }

                default:
                {
                    /* pop it and destroy it. */
                    if (a_pfnCallBack)
                        a_pfnCallBack(pNode, a_pvUser);

                    int rc = a_pAllocator->freeNode(pNode);
                    AssertRCReturnStmt(rc, m_cErrors++, rc);

                    cEntries -= 1;
                    break;
                }
            }
        }

        Assert(m_idxRoot == a_pAllocator->kNilIndex);
        return VINF_SUCCESS;
    }

private:
    /**
     * Rewinds a stack of pointers to pointers to nodes, rebalancing the tree.
     *
     * @returns   IPRT status code.
     *
     * @param     a_pAllocator  Pointer to the allocator.
     * @param     a_pStack      Pointer to stack to rewind.
     * @param     a_fLog        Log is done (DEBUG builds only).
     *
     * @code
     *            LOOP thru all stack entries
     *            BEGIN
     *                Get pointer to pointer to node (and pointer to node) from the stack.
     *                IF 2 higher left subtree than in right subtree THEN
     *                BEGIN
     *                    IF higher (or equal) left-sub-subtree than right-sub-subtree THEN
     *                                *                       n+2|n+3
     *                              /   \                     /     \
     *                            n+2    n       ==>         n+1   n+1|n+2
     *                           /   \                             /     \
     *                         n+1 n|n+1                          n|n+1  n
     *
     *                         Or with keys:
     *
     *                               4                           2
     *                             /   \                       /   \
     *                            2     5        ==>          1     4
     *                           / \                               / \
     *                          1   3                             3   5
     *
     *                    ELSE
     *                                *                         n+2
     *                              /   \                      /   \
     *                            n+2    n                   n+1   n+1
     *                           /   \           ==>        /  \   /  \
     *                          n    n+1                    n  L   R   n
     *                               / \
     *                              L   R
     *
     *                         Or with keys:
     *                               6                           4
     *                             /   \                       /   \
     *                            2     7        ==>          2     6
     *                          /   \                       /  \  /  \
     *                          1    4                      1  3  5  7
     *                              / \
     *                             3   5
     *                END
     *                ELSE IF 2 higher in right subtree than in left subtree THEN
     *                BEGIN
     *                    Same as above but left <==> right. (invert the picture)
     *                ELSE
     *                    IF correct height THEN break
     *                    ELSE correct height.
     *            END
     * @endcode
     * @internal
     */
    int i_rebalance(RTCHardAvlTreeSlabAllocator<NodeType> *a_pAllocator, HardAvlStack *a_pStack, bool a_fLog = false)
    {
        RT_NOREF(a_fLog);

        while (a_pStack->cEntries > 0)
        {
            /* pop */
            uint32_t * const pidxNode  = a_pStack->apidxEntries[--a_pStack->cEntries];
            uint32_t const   idxNode   = *pidxNode;
            NodeType * const pNode     = a_pAllocator->ptrFromInt(idxNode);
            AssertMsgReturnStmt(a_pAllocator->isPtrRetOkay(pNode),
                                ("pidxNode=%p[%#x] pNode=%p\n", pidxNode, *pidxNode, pNode),
                                m_cErrors++, a_pAllocator->ptrErrToStatus(pNode));

            /* Read node properties: */
            uint32_t const   idxLeftNode = pNode->idxLeft;
            NodeType * const pLeftNode   = a_pAllocator->ptrFromInt(idxLeftNode);
            AssertMsgReturnStmt(a_pAllocator->isPtrRetOkay(pLeftNode),
                                ("idxLeftNode=%#x pLeftNode=%p\n", idxLeftNode, pLeftNode),
                                m_cErrors++, a_pAllocator->ptrErrToStatus(pLeftNode));

            uint32_t const   idxRightNode = pNode->idxRight;
            NodeType * const pRightNode   = a_pAllocator->ptrFromInt(idxRightNode);
            AssertMsgReturnStmt(a_pAllocator->isPtrRetOkay(pRightNode),
                                ("idxRight=%#x pRightNode=%p\n", idxRightNode, pRightNode),
                                m_cErrors++, a_pAllocator->ptrErrToStatus(pRightNode));

            uint8_t const cLeftHeight  = pLeftNode  ? pLeftNode->cHeight  : 0;
            AssertReturnStmt(cLeftHeight <= kMaxHeight, m_cErrors++, VERR_HARDAVL_BAD_LEFT_HEIGHT);

            uint8_t const cRightHeight = pRightNode ? pRightNode->cHeight : 0;
            AssertReturnStmt(cRightHeight <= kMaxHeight, m_cErrors++, VERR_HARDAVL_BAD_RIGHT_HEIGHT);

            /* Decide what needs doing: */
            if (cRightHeight + 1 < cLeftHeight)
            {
                Assert(cRightHeight + 2 == cLeftHeight);
                AssertReturnStmt(pLeftNode, m_cErrors++, VERR_HARDAVL_UNEXPECTED_NULL_LEFT);

                uint32_t const   idxLeftLeftNode = pLeftNode->idxLeft;
                NodeType * const pLeftLeftNode   = a_pAllocator->ptrFromInt(idxLeftLeftNode);
                AssertMsgReturnStmt(a_pAllocator->isPtrRetOkay(pLeftLeftNode),
                                    ("idxLeftLeftNode=%#x pLeftLeftNode=%p\n", idxLeftLeftNode, pLeftLeftNode),
                                    m_cErrors++, a_pAllocator->ptrErrToStatus(pLeftLeftNode));

                uint32_t const   idxLeftRightNode = pLeftNode->idxRight;
                NodeType * const pLeftRightNode   = a_pAllocator->ptrFromInt(idxLeftRightNode);
                AssertMsgReturnStmt(a_pAllocator->isPtrRetOkay(pLeftRightNode),
                                    ("idxLeftRightNode=%#x pLeftRightNode=%p\n", idxLeftRightNode, pLeftRightNode),
                                    m_cErrors++, a_pAllocator->ptrErrToStatus(pLeftRightNode));

                uint8_t const cLeftRightHeight = pLeftRightNode ? pLeftRightNode->cHeight : 0;
                if ((pLeftLeftNode ? pLeftLeftNode->cHeight : 0) >= cLeftRightHeight)
                {
                    AssertReturnStmt(cLeftRightHeight + 2 <= kMaxHeight, m_cErrors++, VERR_HARDAVL_BAD_NEW_HEIGHT);
                    pNode->idxLeft      = idxLeftRightNode;
                    pNode->cHeight      = (uint8_t)(cLeftRightHeight + 1);
                    pLeftNode->cHeight  = (uint8_t)(cLeftRightHeight + 2);
                    pLeftNode->idxRight = idxNode;
                    *pidxNode = idxLeftNode;
#ifdef DEBUG
                    if (a_fLog) RTAssertMsg2("rebalance: %#2u: op #1\n", a_pStack->cEntries);
#endif
                }
                else
                {
                    AssertReturnStmt(cLeftRightHeight <= kMaxHeight, m_cErrors++, VERR_HARDAVL_BAD_RIGHT_HEIGHT);
                    AssertReturnStmt(pLeftRightNode, m_cErrors++, VERR_HARDAVL_UNEXPECTED_NULL_RIGHT);

                    uint32_t const idxLeftRightLeftNode  = pLeftRightNode->idxLeft;
                    AssertReturnStmt(a_pAllocator->isIntValid(idxLeftRightLeftNode), m_cErrors++, VERR_HARDAVL_INDEX_OUT_OF_BOUNDS);
                    uint32_t const idxLeftRightRightNode = pLeftRightNode->idxRight;
                    AssertReturnStmt(a_pAllocator->isIntValid(idxLeftRightRightNode), m_cErrors++, VERR_HARDAVL_INDEX_OUT_OF_BOUNDS);
                    pLeftNode->idxRight = idxLeftRightLeftNode;
                    pNode->idxLeft      = idxLeftRightRightNode;

                    pLeftRightNode->idxLeft  = idxLeftNode;
                    pLeftRightNode->idxRight = idxNode;
                    pLeftNode->cHeight       = cLeftRightHeight;
                    pNode->cHeight           = cLeftRightHeight;
                    pLeftRightNode->cHeight  = cLeftHeight;
                    *pidxNode = idxLeftRightNode;
#ifdef DEBUG
                    if (a_fLog) RTAssertMsg2("rebalance: %#2u: op #2\n", a_pStack->cEntries);
#endif
                }
            }
            else if (cLeftHeight + 1 < cRightHeight)
            {
                Assert(cLeftHeight + 2 == cRightHeight);
                AssertReturnStmt(pRightNode, m_cErrors++, VERR_HARDAVL_UNEXPECTED_NULL_RIGHT);

                uint32_t const   idxRightLeftNode = pRightNode->idxLeft;
                NodeType * const pRightLeftNode   = a_pAllocator->ptrFromInt(idxRightLeftNode);
                AssertMsgReturnStmt(a_pAllocator->isPtrRetOkay(pRightLeftNode),
                                    ("idxRightLeftNode=%#x pRightLeftNode=%p\n", idxRightLeftNode, pRightLeftNode),
                                    m_cErrors++, a_pAllocator->ptrErrToStatus(pRightLeftNode));

                uint32_t const   idxRightRightNode = pRightNode->idxRight;
                NodeType * const pRightRightNode   = a_pAllocator->ptrFromInt(idxRightRightNode);
                AssertMsgReturnStmt(a_pAllocator->isPtrRetOkay(pRightRightNode),
                                    ("idxRightRightNode=%#x pRightRightNode=%p\n", idxRightRightNode, pRightRightNode),
                                    m_cErrors++, a_pAllocator->ptrErrToStatus(pRightRightNode));

                uint8_t const cRightLeftHeight = pRightLeftNode ? pRightLeftNode->cHeight : 0;
                if ((pRightRightNode ? pRightRightNode->cHeight : 0) >= cRightLeftHeight)
                {
                    AssertReturnStmt(cRightLeftHeight + 2 <= kMaxHeight, m_cErrors++, VERR_HARDAVL_BAD_NEW_HEIGHT);

                    pNode->idxRight     = idxRightLeftNode;
                    pRightNode->idxLeft = idxNode;
                    pNode->cHeight      = (uint8_t)(cRightLeftHeight + 1);
                    pRightNode->cHeight = (uint8_t)(cRightLeftHeight + 2);
                    *pidxNode = idxRightNode;
#ifdef DEBUG
                    if (a_fLog) RTAssertMsg2("rebalance: %#2u: op #3 h=%d, *pidxNode=%#x\n", a_pStack->cEntries, pRightNode->cHeight, *pidxNode);
#endif
                    RTHARDAVL_STRICT_CHECK_HEIGHTS(pRightNode, NULL, 0);
                    RTHARDAVL_STRICT_CHECK_HEIGHTS(pNode, NULL, 0);
                }
                else
                {
                    AssertReturnStmt(cRightLeftHeight <= kMaxHeight, m_cErrors++, VERR_HARDAVL_BAD_LEFT_HEIGHT);
                    AssertReturnStmt(pRightLeftNode, m_cErrors++, VERR_HARDAVL_UNEXPECTED_NULL_LEFT);

                    uint32_t const idxRightLeftRightNode = pRightLeftNode->idxRight;
                    AssertReturnStmt(a_pAllocator->isIntValid(idxRightLeftRightNode), m_cErrors++, VERR_HARDAVL_INDEX_OUT_OF_BOUNDS);
                    uint32_t const idxRightLeftLeftNode  = pRightLeftNode->idxLeft;
                    AssertReturnStmt(a_pAllocator->isIntValid(idxRightLeftLeftNode), m_cErrors++, VERR_HARDAVL_INDEX_OUT_OF_BOUNDS);
                    pRightNode->idxLeft      = idxRightLeftRightNode;
                    pNode->idxRight          = idxRightLeftLeftNode;

                    pRightLeftNode->idxRight = idxRightNode;
                    pRightLeftNode->idxLeft  = idxNode;
                    pRightNode->cHeight      = cRightLeftHeight;
                    pNode->cHeight           = cRightLeftHeight;
                    pRightLeftNode->cHeight  = cRightHeight;
                    *pidxNode = idxRightLeftNode;
#ifdef DEBUG
                    if (a_fLog) RTAssertMsg2("rebalance: %#2u: op #4 h=%d, *pidxNode=%#x\n", a_pStack->cEntries, pRightLeftNode->cHeight, *pidxNode);
#endif
                }
            }
            else
            {
                uint8_t const cHeight = (uint8_t)(RT_MAX(cLeftHeight, cRightHeight) + 1);
                AssertReturnStmt(cHeight <= kMaxHeight, m_cErrors++, VERR_HARDAVL_BAD_NEW_HEIGHT);
                if (cHeight == pNode->cHeight)
                {
#ifdef DEBUG
                    if (a_fLog) RTAssertMsg2("rebalance: %#2u: op #5, h=%d - done\n", a_pStack->cEntries, cHeight);
#endif
                    RTHARDAVL_STRICT_CHECK_HEIGHTS(pNode, NULL, 0);
                    if (pLeftNode)
                        RTHARDAVL_STRICT_CHECK_HEIGHTS(pLeftNode, NULL, 0);
                    if (pRightNode)
                        RTHARDAVL_STRICT_CHECK_HEIGHTS(pRightNode, NULL, 0);
                    break;
                }
#ifdef DEBUG
                if (a_fLog) RTAssertMsg2("rebalance: %#2u: op #5, h=%d - \n", a_pStack->cEntries, cHeight);
#endif
                pNode->cHeight = cHeight;
            }
        }
        return VINF_SUCCESS;
    }
};

/** @} */

#endif /* !IPRT_INCLUDED_cpp_hardavlrange_h */

