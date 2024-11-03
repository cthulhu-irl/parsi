#include <iostream>
#include <fstream>
#include <sstream>

#include <parsi/parsi.hpp>

namespace {

constexpr static auto create_json_string_validator_parser()
{
    constexpr auto oct_charset = parsi::Charset("01234567");
    constexpr auto hex_charset = parsi::Charset("0123456789abcdefABCDEF");

    constexpr auto valid_printable_unescaped_character_parser = [](parsi::Stream stream) -> parsi::Result {
        if (stream.size() <= 0) {
            return parsi::Result{stream, false};
        }
        if (0x1F >= stream.front() || stream.front() >= 0x7F) {
            return parsi::Result{stream, false};
        }
        if (stream.front() == '\\' || stream.front() == '"') {
            return parsi::Result{stream, false};
        }
        stream.advance(1);
        return parsi::Result{stream, true};
    };

    constexpr auto hex_escaped_character_parser = parsi::sequence(
        parsi::expect('\\'),
        parsi::expect('x'),
        parsi::expect(hex_charset),
        parsi::expect(hex_charset)
    );
    constexpr auto unicode_escaped_character_parser = parsi::sequence(
        parsi::expect('\\'),
        parsi::expect('u'),
        parsi::expect(hex_charset),
        parsi::expect(hex_charset),
        parsi::expect(hex_charset),
        parsi::expect(hex_charset)
    );
    constexpr auto octal_escaped_character_parser = parsi::sequence(
        parsi::expect('\\'),
        parsi::expect('o'),
        parsi::expect(oct_charset),
        parsi::expect(oct_charset),
        parsi::expect(oct_charset)
    );

    constexpr auto single_unit_parser = parsi::anyof(
        valid_printable_unescaped_character_parser,
        parsi::sequence(parsi::expect('\\'), parsi::expect(parsi::Charset("\\nrbtf\"\'"))),
        hex_escaped_character_parser,
        unicode_escaped_character_parser,
        octal_escaped_character_parser
    );

    return parsi::sequence(
        parsi::expect('"'),
        parsi::repeat(single_unit_parser),
        parsi::expect('"')
    );
}

template <parsi::is_parser JoinParserF, parsi::is_parser ItemParserF>
constexpr static auto create_joined_repeated_parser(JoinParserF&& join_parser, ItemParserF&& item_parser)
{
    return [join_parser=std::forward<JoinParserF>(join_parser),
            item_parser=std::forward<ItemParserF>(item_parser)](parsi::Stream stream) -> parsi::Result {
        parsi::Result res = item_parser(stream);
        if (!res) {
            // allow for zero items
            return parsi::Result{stream, true};
        }

        stream = res.stream();
        while ((res = join_parser(stream))) {
            stream = res.stream();
            res = item_parser(stream);
            if (!res)
            {
                return parsi::Result{stream, false};
            }
            stream = res.stream();
        }

        return parsi::Result{stream, true};
    };
}

class JsonValidator {
    constexpr static auto whitespaces = parsi::repeat(parsi::expect(parsi::Charset(" \t\n\a\v")));
    constexpr static auto digit_seq_parser = parsi::repeat<1>(parsi::expect(parsi::Charset("0123456789")));

    constexpr static auto json_null_parser = parsi::expect("null");
    constexpr static auto json_boolean_parser = parsi::anyof(parsi::expect("true"), parsi::expect("false"));
    constexpr static auto json_number_parser = parsi::sequence(
        parsi::optional(parsi::expect('-')),
        digit_seq_parser,
        parsi::optional(parsi::sequence(parsi::expect('.'), digit_seq_parser))
    );
    constexpr static auto json_string_parser = create_json_string_validator_parser();

public:
    constexpr JsonValidator() noexcept {}

    constexpr auto operator()(parsi::Stream stream) const -> parsi::Result
    {
        auto self_parser = [&self=*this](parsi::Stream stream) -> parsi::Result {
            return self(stream);
        };

        auto json_array_parser = parsi::sequence(
            parsi::expect('['),
            whitespaces,
            create_joined_repeated_parser(
                parsi::expect(','),
                parsi::sequence(whitespaces, self_parser, whitespaces)
            ),
            whitespaces,
            parsi::expect(']')
        );

        auto json_object_parser = parsi::sequence(
            parsi::expect('{'),
            whitespaces,
            create_joined_repeated_parser(
                parsi::expect(','),
                parsi::sequence(
                    whitespaces,
                    json_string_parser,
                    whitespaces,
                    parsi::expect(':'),
                    whitespaces,
                    self_parser,
                    whitespaces
                )
            ),
            whitespaces,
            parsi::expect('}')
        );

        auto parser = parsi::sequence(
            whitespaces,
            parsi::anyof(
                json_null_parser,
                json_boolean_parser,
                json_number_parser,
                json_string_parser,
                json_array_parser,
                json_object_parser
            ),
            whitespaces
        );

        return parser(stream);
    }
};

static auto create_json_validator_parser() -> JsonValidator
{
    static const JsonValidator parser{};
    return parser;
}

}  // namespace

int main(int argc, char** argv)
{
    std::cerr << "(NOTE: currently there is no support for unicode.)\n";

    if (argc != 2)
    {
        std::cerr << "Usage:\n\t" << argv[0] << " <json_file_path>\n";
        return 1;
    }

    std::ifstream file;
    file.open(argv[1]);
    if (file.fail())
    {
        std::cerr << "An error occured when reading the following file: " << argv[1] << '\n';
        return 1;
    }

    std::stringstream file_content_stream;
    file_content_stream << file.rdbuf();

    std::string file_content = file_content_stream.str();

    auto parser = parsi::sequence(
        create_json_validator_parser(),
        parsi::eos()
    );

    if (auto res = parser(std::string_view(file_content)); !res) {
        std::cout << " [Syntax Error] Remaining buffer that couldn't be parsed: " << res.stream().as_string_view() << '\n';
        return 1;
    }

    std::cout << "Given json file is valid.\n";

    return 0;
}
