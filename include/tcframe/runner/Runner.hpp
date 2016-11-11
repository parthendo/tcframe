#pragma once

#include <iostream>

#include "Args.hpp"
#include "ArgsParser.hpp"
#include "MetadataParser.hpp"
#include "RunnerLogger.hpp"
#include "RunnerLoggerFactory.hpp"
#include "tcframe/generator.hpp"
#include "tcframe/os.hpp"
#include "tcframe/spec.hpp"
#include "tcframe/submitter.hpp"
#include "tcframe/util.hpp"
#include "tcframe/verifier.hpp"

using std::cout;
using std::endl;

namespace tcframe {

template<typename TProblemSpec>
class Runner {
private:
    const int DEFAULT_SEED = 0;
    const char* DEFAULT_SOLUTION_COMMAND = "./solution";
    const char* DEFAULT_OUTPUT_DIR = "tc";

    BaseTestSpec<TProblemSpec>* testSpec_;

    LoggerEngine* loggerEngine_;
    OperatingSystem* os_;

    MetadataParser* metadataParser_;
    RunnerLoggerFactory* runnerLoggerFactory_;
    GeneratorFactory* generatorFactory_;
    SubmitterFactory* submitterFactory_;

public:
    Runner(
            BaseTestSpec<TProblemSpec>* testSpec,
            LoggerEngine* loggerEngine,
            OperatingSystem* os,
            MetadataParser* metadataParser,
            RunnerLoggerFactory* runnerLoggerFactory,
            GeneratorFactory* generatorFactory,
            SubmitterFactory* submitterFactory)
            : testSpec_(testSpec)
            , loggerEngine_(loggerEngine)
            , metadataParser_(metadataParser)
            , os_(os)
            , runnerLoggerFactory_(runnerLoggerFactory)
            , generatorFactory_(generatorFactory)
            , submitterFactory_(submitterFactory) {}

    int run(int argc, char* argv[]) {
        auto runnerLogger = runnerLoggerFactory_->create(loggerEngine_);

        try {
            Args args = parseArgs(argc, argv);
            Metadata metadata = parseMetadata(argv[0]);
            Spec spec = buildSpec(metadata, runnerLogger);

            int result;
            if (args.command() == Args::Command::GEN) {
                result = generate(args, spec);
            } else {
                result = submit(args, spec);
            }
            cleanUp();
            return result;
        } catch (...) {
            return 1;
        }
    }

private:
    Args parseArgs(int argc, char* argv[]) {
        try {
            return ArgsParser::parse(argc, argv);
        } catch (runtime_error& e) {
            cout << e.what() << endl;
            throw;
        }
    }

    Metadata parseMetadata(const string& runnerPath) {
        try {
            return metadataParser_->parse(runnerPath);
        } catch (runtime_error& e) {
            cout << e.what() << endl;
            throw;
        }
    }

    Spec buildSpec(const Metadata& metadata, RunnerLogger* logger) {
        try {
            return testSpec_->buildSpec(metadata);
        } catch (runtime_error& e) {
            logger->logSpecificationFailure({e.what()});
            throw;
        }
    }

    int generate(const Args& args, const Spec& spec) {
        const Metadata& metadata = spec.metadata();
        const ProblemConfig& problemConfig = spec.problemConfig();

        GeneratorConfig generatorConfig = GeneratorConfigBuilder()
                .setMultipleTestCasesCount(problemConfig.multipleTestCasesCount().value_or(nullptr))
                .setSeed(args.seed().value_or(DEFAULT_SEED))
                .setSlug(metadata.slug())
                .setSolutionCommand(args.solution().value_or(DEFAULT_SOLUTION_COMMAND))
                .setOutputDir(args.output().value_or(DEFAULT_OUTPUT_DIR))
                .build();

        auto ioManipulator = new IOManipulator(spec.ioFormat());
        auto verifier = new Verifier(spec.constraintSuite());
        auto logger = new GeneratorLogger(loggerEngine_);
        auto testCaseGenerator = new TestCaseGenerator(verifier, ioManipulator, os_, logger);
        auto generator = generatorFactory_->create(testCaseGenerator, verifier, os_, logger);

        return generator->generate(spec.testSuite(), generatorConfig) ? 0 : 1;
    }

    int submit(const Args& args, const Spec& spec) {
        const Metadata& metadata = spec.metadata();
        const ProblemConfig& problemConfig = spec.problemConfig();

        SubmitterConfigBuilder configBuilder = SubmitterConfigBuilder()
                .setHasMultipleTestCasesCount(problemConfig.multipleTestCasesCount())
                .setSlug(metadata.slug())
                .setSolutionCommand(args.solution().value_or(DEFAULT_SOLUTION_COMMAND))
                .setTestCasesDir(args.output().value_or(DEFAULT_OUTPUT_DIR));

        if (!args.noTimeLimit()) {
            if (args.timeLimit()) {
                configBuilder.setTimeLimit(args.timeLimit().value());
            } else if (metadata.timeLimit()) {
                configBuilder.setTimeLimit(metadata.timeLimit().value());
            }
        }
        if (!args.noMemoryLimit()) {
            if (args.memoryLimit()) {
                configBuilder.setMemoryLimit(args.memoryLimit().value());
            } else if (metadata.memoryLimit()) {
                configBuilder.setMemoryLimit(metadata.memoryLimit().value());
            }
        }

        SubmitterConfig submitterConfig = configBuilder.build();

        auto logger = new SubmitterLogger(loggerEngine_);
        auto evaluator = new BatchEvaluator(os_, logger);
        auto scorer = new DiffScorer(os_, logger);
        auto testCaseSubmitter = new TestCaseSubmitter(evaluator, scorer, logger);
        auto submitter = submitterFactory_->create(testCaseSubmitter, logger);

        set<int> subtaskIds;
        for (const Subtask& subtask : spec.constraintSuite().constraints()) {
            subtaskIds.insert(subtask.id());
        }

        submitter->submit(spec.testSuite(), subtaskIds, submitterConfig);
        return 0;
    }

    void cleanUp() {
        os_->execute(ExecutionRequestBuilder().setCommand("rm _*.out").build());
    }
};

}
