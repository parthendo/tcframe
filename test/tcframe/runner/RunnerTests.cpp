#include "gmock/gmock.h"
#include "../mock.hpp"

#include "../generator/MockGenerator.hpp"
#include "../os/MockOperatingSystem.hpp"
#include "../submitter/MockSubmitter.hpp"
#include "MockMetadataParser.hpp"
#include "MockRunnerLogger.hpp"
#include "MockRunnerLoggerFactory.hpp"
#include "tcframe/experimental/runner.hpp"

using ::testing::_;
using ::testing::Eq;
using ::testing::Ne;
using ::testing::Return;
using ::testing::Test;

namespace tcframe {

class RunnerTests : public Test {
public:
    static int T;

protected:
    class ProblemSpec : public BaseProblemSpec {
    protected:
        void InputFormat() {}

        void Config() {
            MultipleTestCasesCount(T);
        }
    };

    class TestSpec : public BaseTestSpec<ProblemSpec> {};

    class BadTestSpec : public BaseTestSpec<ProblemSpec> {
    public:
        Spec buildSpec(const tcframe::Metadata& config) {
            throw runtime_error("An error");
        }
    };

    int argc = 1;
    char** argv =  new char*[1]{(char*) "./runner"};

    Metadata metadata = MetadataBuilder()
            .setSlug("slug")
            .setTimeLimit(3)
            .setMemoryLimit(128)
            .build();

    LoggerEngine* loggerEngine = new SimpleLoggerEngine();

    MOCK(MetadataParser) metadataParser;
    MOCK(RunnerLogger) runnerLogger;
    MOCK(Generator) generator;
    MOCK(Submitter) submitter;

    MOCK(OperatingSystem) os;
    MOCK(RunnerLoggerFactory) runnerLoggerFactory;
    MOCK(GeneratorFactory) generatorFactory;
    MOCK(SubmitterFactory) submitterFactory;

    Runner<ProblemSpec> runner = Runner<ProblemSpec>(
            new TestSpec(), loggerEngine, &os, &metadataParser,
            &runnerLoggerFactory, &generatorFactory, &submitterFactory);

    void SetUp() {
        ON_CALL(metadataParser, parse(_)).WillByDefault(Return(metadata));
        ON_CALL(runnerLoggerFactory, create(_)).WillByDefault(Return(&runnerLogger));
        ON_CALL(generatorFactory, create(_, _, _, _)).WillByDefault(Return(&generator));
        ON_CALL(submitterFactory, create(_, _)).WillByDefault(Return(&submitter));
        ON_CALL(os, execute(_)).WillByDefault(Return(
                ExecutionResult(ExecutionInfoBuilder().setExitCode(0).build(), nullptr, nullptr)));
    }
};

int RunnerTests::T;

TEST_F(RunnerTests, Run_ArgsParsing_Failed) {
    EXPECT_THAT(runner.run(2, new char*[2]{(char*) "./runner", (char*) "--blah"}), Ne(0));
}

TEST_F(RunnerTests, Run_Specification_Failed) {
    Runner<ProblemSpec> badRunner = Runner<ProblemSpec>(
            new BadTestSpec(), loggerEngine, &os, &metadataParser,
            &runnerLoggerFactory, &generatorFactory, &submitterFactory);

    EXPECT_CALL(generator, generate(_, _)).Times(0);
    EXPECT_CALL(runnerLogger, logSpecificationFailure(vector<string>{"An error"}));

    EXPECT_THAT(badRunner.run(argc, argv), Ne(0));
}

TEST_F(RunnerTests, Run_Generation_Successful) {
    ON_CALL(generator, generate(_, _)).WillByDefault(Return(true));

    EXPECT_THAT(runner.run(argc, argv), Eq(0));
}

TEST_F(RunnerTests, Run_Generation_Failed) {
    ON_CALL(generator, generate(_, _)).WillByDefault(Return(false));

    EXPECT_THAT(runner.run(argc, argv), Ne(0));
}

TEST_F(RunnerTests, Run_Generation_UseConfigOptions) {
    EXPECT_CALL(generator, generate(_, GeneratorConfigBuilder()
            .setSeed(0)
            .setSlug("slug")
            .setMultipleTestCasesCount(&T)
            .setSolutionCommand("./solution")
            .setOutputDir("tc")
            .build()));

    runner.run(argc, argv);
}

TEST_F(RunnerTests, Run_Generation_UseArgsOptions) {
    EXPECT_CALL(generator, generate(_, GeneratorConfigBuilder()
            .setSeed(42)
            .setSlug("slug")
            .setMultipleTestCasesCount(&T)
            .setSolutionCommand("\"java Solution\"")
            .setOutputDir("testdata")
            .build()));

    runner.run(4, new char*[5]{
            (char*) "./runner",
            (char*) "--seed=42",
            (char*) "--solution=\"java Solution\"",
            (char*) "--output=testdata",
            nullptr});
}

TEST_F(RunnerTests, Run_Submission) {
    EXPECT_CALL(submitter, submit(_, _, _));

    int exitStatus = runner.run(2, new char*[3]{
            (char*) "./runner",
            (char*) "submit",
            nullptr});

    EXPECT_THAT(exitStatus, Eq(0));
}

TEST_F(RunnerTests, Run_Submission_UseDefaultOptions) {
    ON_CALL(metadataParser, parse(_)).WillByDefault(Return(MetadataBuilder().setSlug("slug").build()));

    EXPECT_CALL(submitter, submit(_, _, SubmitterConfigBuilder()
            .setSlug("slug")
            .setHasMultipleTestCasesCount(true)
            .setSolutionCommand("./solution")
            .setTestCasesDir("tc")
            .build()));

    runner.run(2, new char*[3]{
            (char*) "./runner",
            (char*) "submit",
            nullptr});
}

TEST_F(RunnerTests, Run_Submission_UseConfigOptions) {
    EXPECT_CALL(submitter, submit(_, _, SubmitterConfigBuilder()
            .setSlug("slug")
            .setHasMultipleTestCasesCount(true)
            .setSolutionCommand("./solution")
            .setTestCasesDir("tc")
            .setTimeLimit(3)
            .setMemoryLimit(128)
            .build()));

    runner.run(2, new char*[3]{
            (char*) "./runner",
            (char*) "submit",
            nullptr});
}

TEST_F(RunnerTests, Run_Submission_UseArgsOptions) {
    EXPECT_CALL(submitter, submit(_, _, SubmitterConfigBuilder()
            .setSlug("slug")
            .setHasMultipleTestCasesCount(true)
            .setSolutionCommand("\"java Solution\"")
            .setTestCasesDir("testdata")
            .setTimeLimit(4)
            .setMemoryLimit(256)
            .build()));

    runner.run(6, new char*[7]{
            (char*) "./runner",
            (char*) "submit",
            (char*) "--solution=\"java Solution\"",
            (char*) "--output=testdata",
            (char*) "--time-limit=4",
            (char*) "--memory-limit=256",
            nullptr});
}

TEST_F(RunnerTests, Run_Submission_UseArgsOptions_NoLimits) {
    EXPECT_CALL(submitter, submit(_, _, SubmitterConfigBuilder()
            .setSlug("slug")
            .setHasMultipleTestCasesCount(true)
            .setSolutionCommand("\"java Solution\"")
            .setTestCasesDir("testdata")
            .build()));

    runner.run(6, new char*[7]{
            (char*) "./runner",
            (char*) "submit",
            (char*) "--solution=\"java Solution\"",
            (char*) "--output=testdata",
            (char*) "--no-time-limit",
            (char*) "--no-memory-limit",
            nullptr});
}

}
