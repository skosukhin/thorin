#include "anydsl2/lambda.h"
#include "anydsl2/literal.h"
#include "anydsl2/primop.h"
#include "anydsl2/printer.h"
#include "anydsl2/type.h"
#include "anydsl2/world.h"
#include "anydsl2/analyses/domtree.h"
#include "anydsl2/analyses/looptree.h"
#include "anydsl2/analyses/scope.h"
#include "anydsl2/analyses/schedule.h"
#include "anydsl2/util/for_all.h"

namespace anydsl2 {

//------------------------------------------------------------------------------

/*
 * Types
 */

void Type::dump() const { Printer p(std::cout, false); print(p); p.newline(); }

Printer& Frame::print(Printer& p) const { p << "frame"; return p; }
Printer& Mem::  print(Printer& p) const { p << "mem"; return p; }
Printer& Pi   ::print(Printer& p) const { ANYDSL2_DUMP_EMBRACING_COMMA_LIST(p,    "pi(", elems(), ")"); return p; }
Printer& Sigma::print(Printer& p) const { ANYDSL2_DUMP_EMBRACING_COMMA_LIST(p, "sigma(", elems(), ")"); return p; }

Printer& Ptr::print(Printer& p) const { 
    if (is_vector())
        p << "<" << length() << " x ";
    referenced_type()->print(p); 
    p << "*";
    if (is_vector())
        p << ">";
    return p;
}

Printer& PrimType::print(Printer& p) const {
    if (is_vector())
        p << "<" << length() << " x ";
    switch (primtype_kind()) {
#define ANYDSL2_UF_TYPE(T) case Node_PrimType_##T: p << #T; break;
#include "anydsl2/tables/primtypetable.h"
        default: ANYDSL2_UNREACHABLE;
    }
    if (is_vector())
        p << ">";
    return p;
}

Printer& Generic::print(Printer& p) const {
    if (!name.empty())
        p << name;
    else
        p << "_" << index();
    return p;
}

std::ostream& operator << (std::ostream& o, const Type* type) {
    Printer p(o, false);
    type->print(p);
    return p.o;
}

//------------------------------------------------------------------------------

/*
 * Defs
 */

void Def::dump() const { 
    Printer p(std::cout, false); 
    const PrimOp* primop = this->isa<PrimOp>();
    if (primop && !primop->is_const())
        primop->print_assignment(p);
    else {
        print(p); 
        p.newline(); 
    }
}

std::ostream& operator << (std::ostream& o, const anydsl2::Def* def) { Printer p(o, false); def->print(p); return p.o; }
Printer& Def::print(Printer& p) const { return print_name(p); }

Printer& Def::print_name(Printer& p) const {
    if (p.is_fancy()) // elide white = 0 and black = 7
        p << "\33[" << (gid() % 6 + 30 + 1) << "m";

    p << unique_name();

    if (p.is_fancy())
        p << "\33[m";

    return p;
}

Printer& Lambda::print_head(Printer& p) const {
    print_name(p);
    p << "(";
    const char* sep = "";
    for_all (param, params()) { \
        p << sep << param << " : " << param->type();
        sep = ", ";
    }
    p << ")";

    if (attr().is_extern())
        p << " extern ";

    return p.up();
}

Printer& Lambda::print_jump(Printer& p) const {
    if (!empty()) {
        to()->print(p);
        ANYDSL2_DUMP_EMBRACING_COMMA_LIST(p, "(", args(), ")");
    }
    p.down();

    return p;
}

const char* PrimOp::op_name() const {
    switch (kind()) {
#define ANYDSL2_AIR_NODE(op, abbr) case Node_##op: return #abbr;
#include "anydsl2/tables/nodetable.h"
        default: ANYDSL2_UNREACHABLE;
    }
}

const char* ArithOp::op_name() const {
    switch (kind()) {
#define ANYDSL2_ARITHOP(op) case ArithOp_##op: return #op;
#include "anydsl2/tables/arithoptable.h"
        default: ANYDSL2_UNREACHABLE;
    }
}

const char* RelOp::op_name() const {
    switch (kind()) {
#define ANYDSL2_RELOP(op) case RelOp_##op: return #op;
#include "anydsl2/tables/reloptable.h"
        default: ANYDSL2_UNREACHABLE;
    }
}

const char* ConvOp::op_name() const {
    switch (kind()) {
#define ANYDSL2_CONVOP(op) case ConvOp_##op: return #op;
#include "anydsl2/tables/convoptable.h"
        default: ANYDSL2_UNREACHABLE;
    }
}

Printer& PrimOp::print(Printer& p) const {
    if (const PrimLit* primlit = this->isa<PrimLit>()) {
        type()->print(p) << " ";
        switch (primlit->primtype_kind()) {
#define ANYDSL2_UF_TYPE(T) case PrimType_##T: p.o << primlit->T##_value(); break;
#include "anydsl2/tables/primtypetable.h"
            default: ANYDSL2_UNREACHABLE; break;
        }
    } else if (is_const()) {
        if (empty()) 
            p << op_name() << " " << type();
        else {
            p << type();
            ANYDSL2_DUMP_EMBRACING_COMMA_LIST(p, "(", this->ops(), ")");
        }
    } else
        print_name(p);

    return p;
}

Printer& PrimOp::print_assignment(Printer& p) const {
    type()->print(p) << " ";
    print_name(p) << " = ";
    ArrayRef<const Def*> ops = this->ops();

    if (const VectorOp* vectorop = this->isa<VectorOp>()) {
        if (!vectorop->cond()->is_allset()) {
            p << "@ " << vectorop->cond() << " ";
        }
        ops = ops.slice_back(1);
    }

    p << op_name() << " ";
    ANYDSL2_DUMP_COMMA_LIST(p, ops);
    return p.newline();
}

//------------------------------------------------------------------------------

void World::dump(bool fancy) {
    Printer p(std::cout, fancy);

    for_all (top, top_level_lambdas(*this)) {
        Scope scope(top);
        Schedule schedule = schedule_smart(scope);
        for_all (lambda, scope.rpo()) {
            int depth = fancy ? scope.domtree().depth(lambda) : 0;
            p.indent += depth;
            p.newline();
            lambda->print_head(p);

            for_all (op, schedule[lambda->sid()])
                op->print_assignment(p);

            lambda->print_jump(p);
            p.indent -= depth;
        }

        p.newline();
    }
}

//------------------------------------------------------------------------------

} // namespace anydsl2
