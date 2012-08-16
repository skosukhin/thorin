#include "anydsl/dom.h"

#include "anydsl/def.h"
#include "anydsl/order.h"
#include "anydsl/util/assert.h"
#include "anydsl/util/for_all.h"

#include <memory>

namespace anydsl {

Dominators::Dominators(const Def* def) {
    PostOrder order(def);
    init(def, order);
}

Dominators::Dominators(const Def* def, const PostOrder& order) {
    init(def, order);
}

Dominators::~Dominators() {
    relation.clear();
}

void Dominators::init(const Def* def, const PostOrder& order) {
    Doms doms(order.size());
    std::fill(doms.begin(), doms.end(), -1);
    const int def_index = order[def];
    anydsl_assert(def_index >= 0, "invalid post index for given def");
    doms[def_index] = def_index; // set initial dominator
    bool updated;
    do {
        updated = false;
        for(PostOrder::reverse_iterator it = order.rbegin(), e = order.rend();
            it != e; ++it) {
            // load current node and its index
            const Def* b = *it;
            const int b_index = order[b];
            anydsl_assert(b_index >= 0, "invalid post index for given b");
            if(b_index == def_index)
                continue;
            // handle predecessors
            Uses::const_iterator b_preds = b->uses().begin();
            Uses::const_iterator b_preds_end = b->uses().end();
            int newdom = -1;
            if(b_preds != b_preds_end) {
                newdom = order[b_preds->def()];
                for(Uses::const_iterator b_preds_it = ++b_preds;
                    b_preds_it != b_preds_end; ++b_preds_it) {
                    const int b_pred_id = order[b_preds->def()];
                    anydsl_assert(b_pred_id >= 0, "invalid post index for given b");
                    if(doms[b_pred_id] >= 0)
                        newdom = intersect(b_pred_id, newdom, doms);
                }
            }
            // update dominator if required
            if(doms[b_index] != newdom) {
                doms[b_index] = newdom;
                updated = true;
            }
        }
    } while(updated);
    // fill final relation map
    // -> resolve indices
    for(int i = 0, e = doms.size(); i < e; ++i) {
        relation[order[i]] = order[doms[i]];
    }

    for_all (p, relation)
        children_.insert(std::make_pair(p.second, p.first));
}

int Dominators::intersect(int first, int second, const Dominators::Doms& doms) {
    while(first != second) {
        while(first < second) {
            first = doms[first];
        }
        while(second < first) {
            if(second == -1)
                first = second; // we have hit an unknown entry... probably not the right entry point?
            else
                second = doms[second];
        }
    }
    return first;
}

}
