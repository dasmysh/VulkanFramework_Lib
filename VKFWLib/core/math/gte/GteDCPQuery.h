/**
 * @file   GteDCPQuery.h
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2015.07.28
 *
 * @brief  Adaption of the GTE header file.
 */

#pragma once

// Geometric Tools LLC, Redmond WA 98052
// Copyright (c) 1998-2015
// Distributed under the Boost Software License, Version 1.0.
// http://www.boost.org/LICENSE_1_0.txt
// http://www.geometrictools.com/License/Boost/LICENSE_1_0.txt
// File Version: 1.0.3 (2015/03/04)

namespace gte
{

    // Distance and closest-point queries.
    template <typename Real, typename Vector, typename Type0, typename Type1>
    class DCPQuery
    {
    public:
        struct Result
        {
            // A DCPQuery-base class B must define a B::Result struct with member
            // 'Real distance'.  A DCPQuery-derived class D must also derive a
            // D::Result from B:Result but may have no members.  The idea is to
            // allow Result to store closest-point information in addition to the
            // distance.  The operator() is non-const to allow DCPQuery to store
            // and modify private state that supports the query.
        };
        Result operator()(Type0 const&, Type1 const&) { return Result(); };
    };

}
