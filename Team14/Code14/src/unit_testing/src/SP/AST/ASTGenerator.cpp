#include "SP/SP.h"
#include "SP/SPTokenizer.h"
#include "SP/SPParser.h"
#include "SP/SyntacticValidator.h"

class ASTGenerator {
public:
	static std::shared_ptr<ProgramNode> generate(std::string& s);
};

std::shared_ptr<ProgramNode> ASTGenerator::generate(std::string& input) {
	//Tokenize the string input
	SPTokenizer tokenizer = SPTokenizer(input);
	std::vector<SPToken> tokens = tokenizer.tokenize();

	//Syntactic Validator takes in tokens
	SyntacticValidator syntacticValidator = SyntacticValidator(tokens);
	syntacticValidator.validate();

	//Parse the tokens
	SPParser parser = SPParser();
	std::shared_ptr<ProgramNode> root = parser.parse(tokens);
	return root;
}