#pragma once

#include <algorithm>
#include <queue>
#include <string>

#include "tcframe/experimental/io/LineIOSegment.hpp"
#include "tcframe/experimental/type/Scalar.hpp"
#include "tcframe/experimental/util/StringUtils.hpp"

using std::forward;
using std::queue;
using std::string;

#define CASE(...) addTestCase(TestCase([=] {__VA_ARGS__;}, #__VA_ARGS__))
#define LINE(...) addIOSegment((MagicLineIOSegmentBuilder(#__VA_ARGS__), __VA_ARGS__).build())

namespace tcframe { namespace experimental {

class MagicLineIOSegmentBuilder {
private:
    LineIOSegmentBuilder builder_;
    queue<string> names_;

public:
    MagicLineIOSegmentBuilder(string names) {
        for (string name : StringUtils::split(names)) {
            names_.push(name);
        }
    }

    template<typename T>
    MagicLineIOSegmentBuilder& operator,(T&& var) {
        string name = names_.front();
        names_.pop();
        builder_.addVariable(forward<T>(var), name);

        return *this;
    }

    LineIOSegment* build() {
        return builder_.build();
    }
};

}}
