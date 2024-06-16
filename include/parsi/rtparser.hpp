#ifndef PARSI_PARSER_HPP
#define PARSI_PARSER_HPP

#include <concepts>
#include <functional>

#include "parsi/base.hpp"

namespace parsi {

/** Runtime Parser (Polymorphic) */
class RTParser {
public:
    template <is_parser ParserT>
        requires (!std::same_as<std::remove_cvref_t<ParserT>, RTParser>)
    RTParser(ParserT&& parser) : _parser(std::forward<ParserT>(parser)) {}

    auto operator()(Stream stream) const -> Result
    {
        return _parser(stream);
    }

private:
    std::function<Result(Stream)> _parser;
};

}  // namespace parsi

#endif  // PARSI_PARSER_HPP
