#include "otc/newick.h"
#include "otc/test_harness.h"
using namespace otc;

char testSingleCharLabelPoly(const TestHarness &);
char testWordLabelPoly(const TestHarness &);
char testBifurcating(const TestHarness &);
char testMonotypic(const TestHarness &);
char testUnderscores(const TestHarness &);
char testWhitespaceHandling(const TestHarness &);
char testQuotedWordLabelPoly(const TestHarness &);
char testCommentPoly(const TestHarness &);
char testBranchLengths(const TestHarness &);
char testUnbalanced(const TestHarness &);
char testUnbalancedToManyClose(const TestHarness &);
char testEmptyClade(const TestHarness &);
char testEmptySib(const TestHarness &);
char testEmptyBranchLength(const TestHarness &);
char genericTokenTest(const TestHarness &th, const std::string &fn, const std::vector<std::string> & expected);
char genericOTCParsingErrorTest(const TestHarness &th, const std::string &fn);

char genericTokenTest(const TestHarness &th, const std::string &fn, const std::vector<std::string> & expected){
    std::ifstream inp;
    if (!th.openTestFile(fn, inp)) {
        return 'U';
    }
    std::vector<std::string> obtained;
    ConstStrPtr filenamePtr = ConstStrPtr(new std::string(th.getFilePath(fn)));
    FilePosStruct pos(filenamePtr);
    NewickTokenizer tokenizer(inp, pos);
    for (const auto & token : tokenizer) {
        obtained.push_back(token.content());
    }
    if (testVecElementEquality(expected, obtained)) {
        return '.';
    }
    return 'F';
}

char genericOTCParsingErrorTest(const TestHarness &th, const std::string &fn) {
    std::ifstream inp;
    if (!th.openTestFile(fn, inp)) {
        return 'U';
    }
    ConstStrPtr filenamePtr = ConstStrPtr(new std::string(th.getFilePath(fn)));
    FilePosStruct pos(filenamePtr);
    NewickTokenizer tokenizer(inp, pos);
    try {
        for (const auto & token : tokenizer) {
            assert(token.content().c_str());
        }
    } catch (const OTCParsingError &) {
        return '.';
    }
    std::cerr << "No OTCParsingError was raised!\n";
    return 'F';
}

char testSingleCharLabelPoly(const TestHarness &th) {
    const std::vector<std::string> expected = {"(", "A", ",", "B", ",", "C", ")", ";"};
    return genericTokenTest(th, "noids-abcnewick.tre", expected);
}

char testWordLabelPoly(const TestHarness &th) {
    const std::vector<std::string> expected = {"(", "AB", ",", "BC", ",", "CD", ")", ";"};
    return genericTokenTest(th, "noids-wordspolytomy.tre", expected);
}
char testBifurcating(const TestHarness &th) {
    const std::vector<std::string> expected = {"(", "AB",
                                                ",",
                                                    "(", 
                                                        "(", "BC", 
                                                             ",", "EF", 
                                                        ")",
                                                         ",",
                                                         "CD",
                                                     ")",
                                                ")", ";"};
    return genericTokenTest(th, "noids-bifurcating.tre", expected);
}
char testMonotypic(const TestHarness &th) {
    const std::vector<std::string> expected = {"(", "(", "AB", ")",
                                                ",",
                                                    "(", "(", 
                                                        "(", "(", "BC", ")", 
                                                             ",", "EF", 
                                                        ")",
                                                         ",",
                                                         "CD",
                                                     ")",")",
                                                ")", ";"};
    return genericTokenTest(th, "noids-monotypic.tre", expected);
}

char testBranchLengths(const TestHarness &th) {
    const std::vector<std::string> expected = {"(", "AB", ":", "0.1",
                                               ",", "BC", ":", "2",
                                               ",", "CD", ":", "7E-8",
                                               ",", "(",
                                                    "E 'H", ":", "75.246",
                                                    ",", "\'", ":", "7.315E-20",
                                               ")", "label for 5", ":", "5",
                                                ")", "some label", ":", "3", ";"};
    return genericTokenTest(th, "noids-branchlengths.tre", expected);
}

char testQuotedWordLabelPoly(const TestHarness &th) {
    const std::vector<std::string> expected = {"(", "AB",
                                               ",", "BC",
                                               ",", "CD",
                                               ",", "E \'H",
                                               ",", "\'",
                                               ")", ";"};
    return genericTokenTest(th, "noids-quotedwordspolytomy.tre", expected);
}

char testCommentPoly(const TestHarness &th) {
    const std::vector<std::string> expected = {"(", "AB", ",", "BC", ",", "CD", ")", ";"};
    return genericTokenTest(th, "noids-polytomywithcomments.tre", expected);
}
char testUnderscores(const TestHarness &th) {
    const std::vector<std::string> expected = {"(", "A B", ",", "B C", ",", "C_D", ")", ";"};
    return genericTokenTest(th, "noids-underscorehandling.tre", expected);
}
char testWhitespaceHandling(const TestHarness &th) {
    const std::vector<std::string> expected = {"(", "A B",
                                               ",", "B C",
                                               ",", "C_D",
                                               ",", "EX FZ", //warn but tolerate unquoted..
                                               ",", "G HY", //warn but normalize unquoted..
                                               ",", "I J", //warn but normalize unquoted even with newlines, I guess...
                                               ",", "Aembedded quoteJ", // ugh. I'll allowit
                                               ")", ";"};
    return genericTokenTest(th, "noids-whitespacehandling.tre", expected);
}
char testUnbalanced(const TestHarness &th) {
    return genericOTCParsingErrorTest(th, "noids-unbalanced.tre");
}
char testUnbalancedToManyClose(const TestHarness &th) {
    return genericOTCParsingErrorTest(th, "noids-unbalancedtoomanyclosed.tre");
}
char testEmptyClade(const TestHarness &th) {
    char r = '.';
    for (const auto & i : {"1", "2", "3", "4"} ) {
        std::string fn = "noids-emptyclade";
        fn += i;
        fn += ".tre";
        auto c = genericOTCParsingErrorTest(th, fn);
        if (c != '.') {
            r = c;
        }
    }
    return r;
}
char testEmptySib(const TestHarness &th) {
    char r = '.';
    for (const auto & i : {"1", "2", "3", "4"} ) {
        std::string fn = "noids-emptysib";
        fn += i;
        fn += ".tre";
        auto c = genericOTCParsingErrorTest(th, fn);
        if (c != '.') {
            r = c;
        }
    }
    return r;
}
char testEmptyBranchLength(const TestHarness &th) {
    char r = '.';
    for (const auto & i : {"1", "2", "3", "4", "5"} ) {
        std::string fn = "noids-emptybranchlength";
        fn += i;
        fn += ".tre";
        auto c = genericOTCParsingErrorTest(th, fn);
        if (c != '.') {
            r = c;
        }
    }
    return r;
}

int main(int argc, char *argv[]) {
    TestHarness th(argc, argv);
    TestsVec tests{TestFn("testSingleCharLabelPoly", testSingleCharLabelPoly)
                   , TestFn("testWordLabelPoly", testWordLabelPoly)
                   , TestFn("testBifurcating", testBifurcating)
                   , TestFn("testMonotypic", testMonotypic)
                   , TestFn("testUnderscores", testUnderscores)
                   , TestFn("testQuotedWordLabelPoly", testQuotedWordLabelPoly)
                   , TestFn("testCommentPoly", testCommentPoly)
                   , TestFn("testUnbalanced", testUnbalanced)
                   , TestFn("testUnbalancedToManyClose", testUnbalancedToManyClose)
                   , TestFn("testWhitespaceHandling", testWhitespaceHandling)
                   , TestFn("testBranchLengths", testBranchLengths)
                   , TestFn("testEmptyClade", testEmptyClade)
                   , TestFn("testEmptySib", testEmptySib)
                   , TestFn("testEmptyBranchLength", testEmptyBranchLength)
                   
                  };
    return th.runTests(tests);
}

