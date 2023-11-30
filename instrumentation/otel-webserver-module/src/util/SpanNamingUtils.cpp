/*
* Copyright 2022, OpenTelemetry Authors. 
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include <boost/dynamic_bitset.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include <boost/tokenizer.hpp>
#include "SpanNamingUtils.h"

/**
Null terminated string containing the URI_SEGMENT_SEPARATOR character,
which is used to separate the segments of a path in a URI.
*/
static const char URI_SEGMENT_SEPARATOR_STR[] = { URI_SEGMENT_SEPARATOR, '\0' };

/**
Character used to separate multiple values in the URI_NAMING_KEY_SUFFIX_KEY
entry in the string dictionary the configures the automatic business transaction
logic
*/
static const char URI_SUFFIX_KEY_SEPARATOR = ',';

/**
Null terminated string containing the URI_SUFFIX_KEY_SEPARATOR, which is
used to separate multiple values in the URI_NAMING_KEY_SUFFIX_KEY
entry in the string dictionary the configures the automatic business transaction
logic
*/
static const char URI_SUFFIX_KEY_SEPARATOR_STR[] = { URI_SUFFIX_KEY_SEPARATOR, '\0' };
static const char URI_PARAMETER_DELIMITER = '.';

/**
Parse a comma separated list of integers greater than 0 from the specified string into
the specified boost::dynamic_bitset.

This function is used to parse the the url components list from the UI.

@param s The string to parse.  Segment numbers in this string should be separated by
URI_SUFFIX_KEY_SEPARATOR_STR.  Any segment number that contains a character other than
a decimal digit will be ignored.
@param result A bitset where each integer parsed out of the specified string s, will
have its corresponding bit in the bitset set.  Since the integers parsed out
of s can not be 0, the index of a bit in the bitset for a given segment number n
is n - 1.
*/
static void parseSegmentNumbers(const std::string& s, boost::dynamic_bitset<>* result)
{
  typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
  boost::char_separator<char> sep(URI_SUFFIX_KEY_SEPARATOR_STR);
  tokenizer keyTokens(s.begin(), s.end(), sep);
  auto i = keyTokens.begin();
  const auto end = keyTokens.end();
  while (i != end)
  {
    unsigned segmentNumber = 0;
    std::string segmentNumberString = *i;
    ++i;
    try
    {
      segmentNumber = boost::lexical_cast<unsigned>(segmentNumberString);
      //0 segment means nothing. Segment numbers
      //are 1 indexed, not 0 indexed.
      if (segmentNumber == 0)
        continue;
      --segmentNumber; // convert to 0 indexing.
      if (result->size() <= segmentNumber)
        result->resize(segmentNumber + 1, false);
      result->set(segmentNumber, true);
    }
    catch (const boost::bad_lexical_cast& e)
    {
      // Ignore things we can't convert to an unsigned int.
    }
  }
}

/**
Builds a business transaction name using the segments of specified the URI path
specified by the comma separated list of integers in the specified suffix key.
The extracted segments are joined together with the specified delimiter string.

For example:
transformNameWithURISegments("/foo/bar/baz/a.php", "/foo/bar/baz/a.php",
"3,1", ".") will yield "foo.baz".  Notice that the order of
the integers in the suffixKey does not matter.

If the result of this function would otherwise be empty, the specified
baseName string is returned.

@param baseName The name of the business transaction computed using the
URI_NAMING_KEY_SEGMENT_SCHEME.  This is the name that will be returned
if no valid segment numbers are parsed out of the specified suffixKey string.
@param urlPath The path of a the URL used to request the currently executing php
script.  This is the path from which segments are extracted.
@param suffixKey A string containing a comma separated list of integers which
specifies which segments to extract from the specified urlPath string to build
the business transaction name returned by this function.
@param delimiter A string that a should be used to join the list of url segments
together to build the business transaction name returned by this function.

@return A business transaction name.
*/
std::string transformNameWithURISegments(const std::string baseName,
  const std::string& urlPath,
  const std::string& suffixKey,
  const std::string& delimiter)
{
  std::string result;
  // as good as guess as any as to the size of the result...
  result.reserve(urlPath.length());

  boost::dynamic_bitset<> segmentNumbersBitSet;
  parseSegmentNumbers(suffixKey, &segmentNumbersBitSet);

  typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
  boost::char_separator<char> sep(URI_SEGMENT_SEPARATOR_STR, "", boost::keep_empty_tokens);
  tokenizer uriTokens(urlPath.begin(), urlPath.end(), sep);
  auto i = uriTokens.begin();
  const auto end = uriTokens.end();

  // Skip the first token if it exists and is an empty token.
  // There is almost certainly always an empty token at the
  // front of the URL, because all URLs are absolute and
  // thus start with a '/' ( URI_SEGMENT_SEPARATOR ).
  if ((i != end) && (((*i).length()) == 0))
    ++i;

  unsigned currentSegmentNumber = 0;
  while ((i != end) && (currentSegmentNumber < segmentNumbersBitSet.size()))
  {
    if (segmentNumbersBitSet.test(currentSegmentNumber))
    {
      if (!result.empty())
        result += delimiter;
      result += *i;
    }
    ++currentSegmentNumber;
    ++i;
  }
  // If there were no valid segment numbers
  // in the suffix key, just return the original base name.
  if (result.empty())
    return baseName;
  return result;
}
