#include "full_tokenizer.h"
#include <fstream>
#include <map>
#include <vector>

#include <unicode/ustream.h>


FullTokenizer::FullTokenizer(const std::string& vocabFname,
                             const std::string& lowercaseFname)
    : unicoder (*(new UnicodeConverter(uErr))),
      basicTokenizer (*(new BasicTokenizer(getDoLowercase(lowercaseFname)))),
      wordPieceTokenizer (*(new WordPieceTokenizer(readVocabulary(vocabFname).first, "[UNK]", 200))) {};

FullTokenizer::~FullTokenizer() {
  delete &unicoder;
  delete &basicTokenizer;
  delete &wordPieceTokenizer;
};

bool FullTokenizer::getDoLowercase(const std::string& lowercaseFname) const {
  std::ifstream file(lowercaseFname);
  // If there exists file named `lowercase`, do lowercase
  if (file.is_open()) return true;
  return false;
}

Vocabulary FullTokenizer::readVocabulary(const std::string& vocabFname) {
  std::ifstream file(vocabFname);
  if (!file.is_open()) {
    throw std::runtime_error(vocabFname + " not found");
  }
  std::string line;
  icu::UnicodeString uLine;
  long i = 0;
  while (std::getline(file, line)) {
    uLine = unicoder.process(line, uErr);
    vocab.insert({uLine, i});
    invVocab.insert({i, uLine});
    i++;
  }
  if (vocab.empty()) {
    throw std::runtime_error("Vocabulary is empty");
  }
  return std::make_pair(vocab, invVocab);
}

std::vector<icu::UnicodeString> FullTokenizer::tokenize(const std::string &s) {
  icu::UnicodeString us = unicoder.process(s, uErr);
  std::vector<icu::UnicodeString> outputWordPieces;
  std::vector<icu::UnicodeString> tokenWordPieces;
  // Get each token from basicTokenizer (whitespace and punctuation tokenized),
  // split that into word pieces (wordPieceTokenizer), and collect all word
  // pieces together in `outputWordPieces`
  #ifdef DEBUG
  std::cout << "Tokenizing `" << s << std::endl;
  #endif
  for (icu::UnicodeString& token : basicTokenizer.tokenize(us)){
    tokenWordPieces = wordPieceTokenizer.tokenize(token);
    outputWordPieces.insert(
      outputWordPieces.end(), tokenWordPieces.begin(), tokenWordPieces.end()
    );
    #ifdef DEBUG
      std::cout << "\t`" << token << "` ->";
      for (const auto t : wordPieces) {
        std::cout << "`" << t << "`,";
      }
      std::cout << std::endl;
    #endif
  }
  return outputWordPieces;
}

std::vector<long> FullTokenizer::tokenizeToIds (const std::string &s) {
  return wordPieceTokenizer.tokensToIds(tokenize(s));
}

long FullTokenizer::tokenToId(const icu::UnicodeString &s) const {
  return wordPieceTokenizer.tokenToId(s);
}
