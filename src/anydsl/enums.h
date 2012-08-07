#ifndef ANYDSL_ENUMS_H
#define ANYDSL_ENUMS_H

#include "anydsl/util/types.h"

namespace anydsl {

//------------------------------------------------------------------------------


enum NodeKind {
#define ANYDSL_GLUE(pre, next)
#define ANYDSL_AIR_NODE(node) Node_##node,
#define ANYDSL_PRIMTYPE(T) Node_PrimType_##T,
#define ANYDSL_ARITHOP(op) Node_##op,
#define ANYDSL_RELOP(op) Node_##op,
#define ANYDSL_CONVOP(op) Node_##op,
#include "anydsl/tables/allnodes.h"
};

enum Markers {
#define ANYDSL_GLUE(pre, next) \
    End_##pre, \
    Begin_##next = End_##pre, \
    zzz##Begin_##next = Begin_##next - 1,
#define ANYDSL_AIR_NODE(node) zzzMarker_##node,
#define ANYDSL_PRIMTYPE(T) zzzMarker_PrimType_##T,
#define ANYDSL_ARITHOP(op) zzzMarker_##op,
#define ANYDSL_RELOP(op) zzzMarker_##op,
#define ANYDSL_CONVOP(op) zzzMarker_##op,
#include "anydsl/tables/allnodes.h"
    End_ConvOp,
    Begin_Node = 0,
    End_AllNodes    = End_ConvOp,

    Begin_AllNodes  = Begin_Node,

    Begin_PrimType  = Begin_PrimType_u,
    End_PrimType    = End_PrimType_f,

    Num_AllNodes    = End_AllNodes   - Begin_AllNodes,
    Num_Nodes       = End_Node       - Begin_Node,

    Num_PrimTypes_u = End_PrimType_u - Begin_PrimType_u,
    Num_PrimTypes_f = End_PrimType_f - Begin_PrimType_f,

    Num_ArithOps    = End_ArithOp    - Begin_ArithOp,
    Num_RelOps      = End_RelOp      - Begin_RelOp,
    Num_ConvOps     = End_ConvOp     - Begin_ConvOp,

    Num_PrimTypes = Num_PrimTypes_u + Num_PrimTypes_f,
};

enum PrimTypeKind {
#define ANYDSL_UF_TYPE(T) PrimType_##T = Node_PrimType_##T,
#include "anydsl/tables/primtypetable.h"
};

enum ArithOpKind {
#define ANYDSL_ARITHOP(op) ArithOp_##op = Node_##op,
#include "anydsl/tables/arithoptable.h"
};

enum RelOpKind {
#define ANYDSL_RELOP(op) RelOp_##op = Node_##op,
#include "anydsl/tables/reloptable.h"
};

enum ConvOpKind {
#define ANYDSL_CONVOP(op) ConvOp_##op = Node_##op,
#include "anydsl/tables/convoptable.h"
};

inline bool isInt(PrimTypeKind kind) {
    return (int) Begin_PrimType_u <= (int) kind && (int) kind < (int) End_PrimType_u;
}

inline bool isFloat(PrimTypeKind kind) {
    return (int) Begin_PrimType_f <= (int) kind && (int) kind < (int) End_PrimType_f;
}

inline bool isCoreNode(int kind){ return (int) Begin_AllNodes <= kind && kind < (int) End_AllNodes; }
inline bool isPrimType(int kind){ return (int) Begin_PrimType <= kind && kind < (int) End_PrimType; }
inline bool isArithOp(int kind) { return (int) Begin_ArithOp <= kind && kind < (int) End_ArithOp; }
inline bool isRelOp(int kind)   { return (int) Begin_RelOp   <= kind && kind < (int) End_RelOp; }
inline bool isConvOp(int kind)  { return (int) Begin_ConvOp  <= kind && kind < (int) End_ConvOp; }

template<PrimTypeKind kind> struct kind2type {};
#define ANYDSL_U_TYPE(T) template<> struct kind2type<PrimType_##T> { typedef T type; };
#define ANYDSL_F_TYPE(T) template<> struct kind2type<PrimType_##T> { typedef T type; };
#include "anydsl/tables/primtypetable.h"

template<class T> struct type2kind {};
template<> struct type2kind<bool> { static const PrimTypeKind kind = PrimType_u1; };
#define ANYDSL_U_TYPE(T) template<> struct type2kind<T> { static const PrimTypeKind kind = PrimType_##T; };
#define ANYDSL_F_TYPE(T) template<> struct type2kind<T> { static const PrimTypeKind kind = PrimType_##T; };
#include "anydsl/tables/primtypetable.h"

const char* kind2str(PrimTypeKind kind);

} // namespace anydsl

#endif
