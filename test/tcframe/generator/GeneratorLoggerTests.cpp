#include "gmock/gmock.h"
#include "../mock.hpp"

#include "../logger/MockLoggerEngine.hpp"
#include "tcframe/generator/GeneratorLogger.hpp"

using ::testing::InSequence;
using ::testing::Test;

namespace tcframe {

class GeneratorLoggerTests : public Test {
protected:
    Mock(LoggerEngine) engine;

    GeneratorLogger logger = GeneratorLogger(&engine);
};

TEST_F(GeneratorLoggerTests, Introduction) {
    EXPECT_CALL(engine, logParagraph(0, "Generating test cases..."));

    logger.logIntroduction();
}

TEST_F(GeneratorLoggerTests, Result_Successful) {
    {
        InSequence sequence;
        EXPECT_CALL(engine, logParagraph(0, ""));
        EXPECT_CALL(engine, logParagraph(0, "Generation finished. All test cases OK."));
    }

    logger.logResult(GenerationResult({}));
}

TEST_F(GeneratorLoggerTests, Result_Failed) {
    {
        InSequence sequence;
        EXPECT_CALL(engine, logParagraph(0, ""));
        EXPECT_CALL(engine, logParagraph(0, "Generation finished. Some test cases FAILED."));
    }

    logger.logResult(GenerationResult({TestGroupGenerationResult(new SimpleFailure("failed"), {})}));
}

TEST_F(GeneratorLoggerTests, TestCaseResult_Successful) {
    EXPECT_CALL(engine, logParagraph(0, "OK"));

    logger.logTestCaseResult("N = 1", TestCaseGenerationResult::successfulResult());
}

TEST_F(GeneratorLoggerTests, TestCaseResult_Failed_Verification) {
    {
        InSequence sequence;
        EXPECT_CALL(engine, logParagraph(0, "FAILED"));
        EXPECT_CALL(engine, logParagraph(2, "Description: N = 1"));
        EXPECT_CALL(engine, logParagraph(2, "Reasons:"));
        EXPECT_CALL(engine, logListItem1(2, "Does not satisfy constraints, on:"));
        EXPECT_CALL(engine, logListItem2(3, "A <= 10"));
        EXPECT_CALL(engine, logListItem2(3, "B <= 10"));
    }

    ConstraintsVerificationResult result({{-1, {"A <= 10", "B <= 10"}}}, {});
    ConstraintsVerificationFailure failure(result);
    logger.logTestCaseResult("N = 1", TestCaseGenerationResult::failedResult(&failure));
}

TEST_F(GeneratorLoggerTests, TestCaseResult_Failed_Verification_WithGroups) {
    {
        InSequence sequence;
        EXPECT_CALL(engine, logParagraph(0, "FAILED"));
        EXPECT_CALL(engine, logParagraph(2, "Description: N = 1"));
        EXPECT_CALL(engine, logParagraph(2, "Reasons:"));
        EXPECT_CALL(engine, logListItem1(2, "Does not satisfy subtask 2, on constraints:"));
        EXPECT_CALL(engine, logListItem2(3, "A <= 10"));
        EXPECT_CALL(engine, logListItem2(3, "B <= 10"));
        EXPECT_CALL(engine, logListItem1(2, "Does not satisfy subtask 4, on constraints:"));
        EXPECT_CALL(engine, logListItem2(3, "A <= B"));
        EXPECT_CALL(engine, logListItem1(2, "Satisfies subtask 1 but is not assigned to it"));
        EXPECT_CALL(engine, logListItem1(2, "Satisfies subtask 3 but is not assigned to it"));
    }

    ConstraintsVerificationResult result({{2, {"A <= 10", "B <= 10"}}, {4, {"A <= B"}}}, {1, 3});
    ConstraintsVerificationFailure failure(result);
    logger.logTestCaseResult("N = 1", TestCaseGenerationResult::failedResult(&failure));
}

TEST_F(GeneratorLoggerTests, MultipleTestCasesCombinationIntroduction) {
    EXPECT_CALL(engine, logHangingParagraph(1, "Combining test cases into a single file (foo_3): "));

    logger.logMultipleTestCasesCombinationIntroduction("foo_3");
}

TEST_F(GeneratorLoggerTests, MultipleTestCasesCombinationResult_Successful) {
    EXPECT_CALL(engine, logParagraph(0, "OK"));

    logger.logMultipleTestCasesCombinationResult(MultipleTestCasesCombinationResult(nullptr));
}

TEST_F(GeneratorLoggerTests, MultipleTestCasesCombinationResult_Failed_Verification) {
    {
        InSequence sequence;
        EXPECT_CALL(engine, logParagraph(0, "FAILED"));
        EXPECT_CALL(engine, logParagraph(2, "Reasons:"));
        EXPECT_CALL(engine, logListItem1(2, "Does not satisfy constraints, on:"));
        EXPECT_CALL(engine, logListItem2(3, "A <= 10"));
        EXPECT_CALL(engine, logListItem2(3, "B <= 10"));
    }

    MultipleTestCasesConstraintsVerificationResult result({"A <= 10", "B <= 10"});
    MultipleTestCasesConstraintsVerificationFailure failure(result);
    logger.logMultipleTestCasesCombinationResult(MultipleTestCasesCombinationResult(&failure));
}

}
