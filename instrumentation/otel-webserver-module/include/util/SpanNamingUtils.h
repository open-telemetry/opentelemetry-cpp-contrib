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
#pragma once

#include <string>

/**
Character used to separate the segments of a path
in a URI.
*/
static const char URI_SEGMENT_SEPARATOR = '/';

/**
Helper function template that will return an iterator to the
beginning of the nth segment in a string where each segment in the string
is separated by the segmentSeparator character.

@param t_StringIteratorType The character iterator type of the string
@param t_CharType the character type of the string ( eg. char or wchar_t ).
@param segmentSeparator The character that delimits segment boundaries in the string.
@param start An iterator that points to the beginning of the string
@param limit An iterator that points past the end of the string.
@param nSegments The number of segments to iterate past.
@return An iterator pointing to the character that starts the nSegments segment.
*/
template <class t_StringIteratorType,
class t_CharType,
    t_CharType segmentSeparator>
    t_StringIteratorType findNthSegment(const t_StringIteratorType& start, const t_StringIteratorType& limit, unsigned nSegments)
{
    t_StringIteratorType current = start;
    if (*current == segmentSeparator)
        ++current;

    unsigned currentSegment = 0;
    while (current != limit)
    {
        if (*current == segmentSeparator)
            ++currentSegment;
        if (currentSegment == nSegments)
            return current;
        ++current;
    }
    return limit;
}

/**
Function template the returns a string that contains
the first nSegments segments of the input string.

@param t_StringType The string class.
@param t_CharType The character type of the string class ( eg. char or wchar_t ).
@param segmentSeparator The character that delimits segment boundaries in the string.
@param t_StringIteratorType The const_iterator type of the string class.
@param s The string whose first nSegments segments should be returned.
@param nSegments The number of segments of the first string that should be in the
returned string
@return A string containing the first nSegments segments of s.
*/
template <class t_StringType,
class t_CharType = char,
    t_CharType segmentSeparator = URI_SEGMENT_SEPARATOR,
class t_StringIteratorType = typename t_StringType::const_iterator>
    t_StringType getFirstNSegments(const t_StringType& s, unsigned nSegments)
{
    auto start = s.begin();
    auto endOfNthSegment =
        findNthSegment<t_StringIteratorType, t_CharType, segmentSeparator>(start, s.end(), nSegments);
    return t_StringType(start, endOfNthSegment);
}

/**
Function template the returns a string that contains
the last nSegments segments of the input string.

@param t_StringType The string class.
@param t_CharType The character type of the string class ( eg. char or wchar_t ).
@param segmentSeparator The character that delimits segment boundaries in the string.
@param t_StringIteratorType The const_iterator type of the string class.
@param s The string whose last nSegments segments should be returned.
@param nSegments The number of segments of the first string that should be in the
returned string
@return A string containing the first nSegments segments of s.
*/
template <class t_StringType,
class t_CharType = char,
    t_CharType segmentSeparator = URI_SEGMENT_SEPARATOR,
class t_StringIteratorType = typename t_StringType::const_reverse_iterator>
    t_StringType getLastNSegments(const t_StringType& s, unsigned nSegments)
{
    auto start = s.rbegin();
    auto endOfNthSegment =
        findNthSegment<t_StringIteratorType, t_CharType, segmentSeparator>(start, s.rend(), nSegments);
    if (endOfNthSegment != s.rend()) {
        ++endOfNthSegment;
    }
    return t_StringType(endOfNthSegment.base(), start.base());
}

std::string transformNameWithURISegments(const std::string baseName,
  const std::string& urlPath,
  const std::string& suffixKey,
  const std::string& delimiter);

