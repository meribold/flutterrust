#include <algorithm>
#include <array>
#include <iostream>
#include <utility>

namespace {
	// http://stackoverflow.com/questions/1198260/iterate-over-tuple
	// http://en.wikipedia.org/wiki/Substitution_failure_is_not_an_error
	template<std::size_t i = 0, typename... Entries, typename... Extractors>
	inline typename std::enable_if<i == sizeof... (Entries), void>::type
	loadCreatureType(std::tuple<Entries...>&,
	                 std::tuple<Extractors...>&,
	                 const std::array<std::string, sizeof... (Entries)>&) { }

	template<std::size_t i = 0, typename... Entries, typename... Extractors>
	inline typename std::enable_if<i < sizeof...(Entries), void>::type
	loadCreatureType(std::tuple<Entries...>& creatureType,
	                 std::tuple<Extractors...>& extractors,
	                 const std::array<std::string, sizeof... (Entries)>& fields)
	{
		try {
			std::get<i>(creatureType) = std::get<i>(extractors)(fields[i]);
		}
		catch (const std::exception& e) {
			throw std::string{"token " + std::to_string(i) + ": " + e.what()};
		}
		catch (const std::string& s) {
			throw std::string{"token " + std::to_string(i) + ": " + s};
		}
		catch (...) {
			throw std::string{"token " + std::to_string(i)};
		}
		loadCreatureType<i + 1, Entries...>(creatureType, extractors, fields);
	}
}

template <typename CreatureType, typename Extractors>
std::vector<CreatureType>
loadCreatureTypes(std::istream&& iStream, Extractors extractors,
                  std::vector<std::string>& errors)
{
	constexpr auto fieldCount = std::tuple_size<CreatureType>::value;

	std::vector<CreatureType> creatureTypes;
	std::istream::char_type nextChar;

	int line       = 1;
	int column     = 1;
	int fieldIndex = 0;

	std::array<std::string, fieldCount> fields;

	//while (iStream && !iStream.eof()) {
	while ((nextChar = iStream.peek()) != std::istream::traits_type::eof()) {
		if (nextChar == '\n') {
			if (fieldIndex < fieldCount - 1 /*&& column > 1*/) {
				// Missing tokens.
				std::cerr << line << ':' << column
				          << ": additional entry expected before linebreak" << std::endl;
			}
			else if (fieldIndex == fieldCount - 1) {
				creatureTypes.push_back(CreatureType{});
				try {
					loadCreatureType(creatureTypes.back(), extractors, fields);
				} catch (const std::string& s) {
					creatureTypes.pop_back();
					std::cerr << line << ": parsing error: " << s
					          << std::endl;
				} catch (...) {
					creatureTypes.pop_back();
					std::cerr << line << ": parsing error" << std::endl;
				}
			}
			for (auto& s : fields) {
				s.clear();
			}
			++line; column = 0; fieldIndex = 0;
		}
		else if (nextChar == ',') {
			if (++fieldIndex > fieldCount - 1) {
				// To many tokens.
				std::cerr << line << ':' << column
				          << ": entry expected to be final; got \'" << nextChar << '\'' << std::endl;
				iStream.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
				iStream.unget();
				continue;
			}
		}
		else {
			fields[fieldIndex].push_back(nextChar);
		}

		iStream.seekg(1, std::ios_base::cur);
		++column;
	}

	return creatureTypes;
}

// http://stackoverflow.com/questions/1198260/iterate-over-tuple
// http://en.wikipedia.org/wiki/Substitution_failure_is_not_an_error
template<std::size_t i = 0, typename... Tp>
inline typename std::enable_if<i == sizeof...(Tp) - 1, void>::type
printCreatureType(const std::tuple<Tp...>& t)
{
	std::cout << '(' << std::get<i>(t) << ')' << std::endl;
}

template<std::size_t i = 0, typename... Tp>
inline typename std::enable_if<i < sizeof...(Tp) - 1, void>::type
printCreatureType(const std::tuple<Tp...>& t)
{
	std::cout << '(' << std::get<i>(t) << "), ";
	printCreatureType<i + 1, Tp...>(t);
}

template <typename CreatureType>
void printCreatureTypes(const std::vector<CreatureType>& vector) {
	for (const auto& creatureType : vector) {
		printCreatureType(creatureType);
	}
}

